#define SPDLOG_ACTIVE_LEVEL 2

#include "common.hpp"
#include <alsa/asoundlib.h>
#include "alsamidi.hpp"

static constexpr int POLL_TIMEOUT_MS = 200;
static constexpr uint8_t PGMCHANGE_ADVANCE_CLOCKS = 11; // Number of SND_SEQ_EVENT_CLOCK messages before actual program change happens

bool AlsaMidi::process(snd_seq_event_t& event) {
    if (event.type == SND_SEQ_EVENT_CLOCK) {
        clock_counter++;
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        clock_interval = now - last_clock;
        last_clock = now;
    }
    event.time.tick = clock_counter;

#ifndef NDEBUG
    if (event.type != SND_SEQ_EVENT_CLOCK) {
        std::chrono::milliseconds clock_interval_ms = std::chrono::duration_cast<std::chrono::milliseconds>(clock_interval);
        SPDLOG_DEBUG("AMIDI event [type={},clock_counter={},clock_interval={}ms]", (int)event.type, clock_counter, clock_interval_ms.count());
    } else {
        SPDLOG_TRACE("AMIDI clock [queue={}, 0={},1={}]", event.data.queue.queue, event.data.queue.param.d32[0], event.data.queue.param.d32[1]);
    }
#endif
    bool pass = true;
    bool push = false;
    switch (event.type) {
        case SND_SEQ_EVENT_PGMCHANGE:
            push = true;
            event.data.control.unused[0] = PGMCHANGE_ADVANCE_CLOCKS;
            event.data.control.unused[1] = (uint8_t)std::chrono::duration_cast<std::chrono::milliseconds>(clock_interval).count();
            break;
        case SND_SEQ_EVENT_NOTEON:
            push = true;
            SPDLOG_DEBUG("AMIDI NOTEON [ch={},note={},velocity={},off_velocity={}]", event.data.note.channel, event.data.note.note,
                event.data.note.velocity, event.data.note.off_velocity);
            break;
        case SND_SEQ_EVENT_START:
        case SND_SEQ_EVENT_CONTINUE:
        case SND_SEQ_EVENT_STOP:
        case SND_SEQ_EVENT_SETPOS_TICK:
        case SND_SEQ_EVENT_SETPOS_TIME:        
        case SND_SEQ_EVENT_CONTROLLER:
            push = true;
            break;
    }
    if (push) {
        if (!fifo.push(std::move(event))) {
            SPDLOG_ERROR("AMIDI FIFO overrun!");
        }
        callback();
    }
    return pass;
}

void AlsaMidi::thru(unsigned char* raw, size_t size) {
    ssize_t written_size = snd_rawmidi_write(handle_out, raw, size);
    if (written_size < 0) {
        throw OstrostrojException(fmt::format("AMIDI snd_rawmidi_write failed! [err={}]", snd_strerror(written_size)));
    }
    if ((size_t)written_size < size) {
        SPDLOG_WARN("AMIDI not all data written thru! [written_size={},size={}]", written_size, size);
    }
    int err;
    if ((err = snd_rawmidi_drain(handle_out)) < 0) {
        throw OstrostrojException(fmt::format("AMIDI snd_rawmidi_drain failed! [err={}]", snd_strerror(err)));
    }
}

void AlsaMidi::parse(unsigned char* raw, size_t read_size) {
    snd_seq_event_t event;
    while (read_size > 0) {
        ssize_t consumed_size = snd_midi_event_encode(parser, raw, read_size, &event);
        if (consumed_size < 0) {
            SPDLOG_ERROR("AMIDI snd_midi_event_encode_byte failed [err={}]", snd_strerror(consumed_size));
            snd_midi_event_reset_encode(parser);
            return;
        }
        if (consumed_size > 0 && event.type != SND_SEQ_EVENT_NONE) {
            if (process(event)) {
                thru(raw, consumed_size);
            }
        }
        read_size -= consumed_size;
        raw += consumed_size;
    }
}

size_t AlsaMidi::read(unsigned char* raw, size_t size) {
    ssize_t read_size = snd_rawmidi_read(handle_in, raw, size);
    if (read_size == -EAGAIN) {
        return 0;
    }
    if (read_size < 0) {
        throw OstrostrojException(fmt::format("AMIDI snd_rawmidi_read failed! [err={}]", snd_strerror(read_size)));
    }
    return read_size;
}

bool AlsaMidi::poll_in(std::vector<pollfd>& poll_descriptors) {
    unsigned short revents;
    int res = poll(poll_descriptors.data(), poll_descriptors.capacity(), POLL_TIMEOUT_MS);
    if (res < 0) {
        if (errno == EINTR) {
            throw OstrostrojException("AMIDI poll interrupted.");
        }
        throw OstrostrojException(fmt::format("AMIDI poll failed. [err={}]", strerror(errno)));
    }
    if (res == 0) {
        return false;
    }
    if ((res = snd_rawmidi_poll_descriptors_revents(handle_in, poll_descriptors.data(), poll_descriptors.capacity(), &revents)) < 0) {
        throw OstrostrojException(fmt::format("AMIDI cannot get poll events [err={}]", snd_strerror(errno)));
    }
    if (revents & (POLLERR | POLLHUP)) {
        throw OstrostrojException(fmt::format("AMIDI poll error [revents={}]", revents));
    }
    return revents & POLLIN;
}

void AlsaMidi::run() {
    try {
        std::array<unsigned char, 4> raw;
        std::vector<pollfd> poll_descriptors = create_poll_descriptors(handle_in);
        SPDLOG_INFO("AMIDI started.");
        while (!stop) {
            if (poll_in(poll_descriptors)) {
                size_t read_size = read(raw.data(), raw.size());
                if (read_size > 0) {
                    parse(raw.data(), read_size);
                }
            }
        }
        SPDLOG_INFO("AMIDI stopped.");
    } catch(std::exception const& e) {
        SPDLOG_ERROR("AMIDI failed [e={}]", e.what());
    }
}

void* run_midi(void* context) {
    ((AlsaMidi*)context)->run();
    return 0;
}

std::vector<pollfd> AlsaMidi::create_poll_descriptors(snd_rawmidi_t *handle) {
    std::vector<pollfd> poll_descriptors = std::vector<pollfd>();
    int count = snd_rawmidi_poll_descriptors_count(handle);
    if (count < 1) {
        throw OstrostrojException(fmt::format("AMIDI No poll descriptors! [count={}]", count));
    }
    poll_descriptors.reserve(count);
    int filled = snd_rawmidi_poll_descriptors(handle, poll_descriptors.data(), poll_descriptors.capacity());
    if (filled < 0) {
        throw OstrostrojException(fmt::format("AMIDI snd_rawmidi_poll_descriptors failed! [err={}]", snd_strerror(filled)));
    }
    if ((size_t)filled != poll_descriptors.capacity()) {
        throw OstrostrojException(fmt::format("AMIDI poll descriptor init failed! [filled={}]", filled));
    }
    return poll_descriptors;
}

snd_rawmidi_t* AlsaMidi::open_midi_in(const std::string& device_name) {
    snd_rawmidi_t* result;
    int err;
    err = snd_rawmidi_open(&result, NULL, device_name.c_str(), SND_RAWMIDI_NONBLOCK);    
    if (err) {
        throw OstrostrojException(fmt::format("AMIDI snd_rawmidi_open {} failed! [err={}]", device_name, snd_strerror(err)));
    }
    err = snd_midi_event_new(256, &parser);
    if (err < 0) {
        throw OstrostrojException(fmt::format("AMIDI snd_midi_event_new failed! [err={}]", snd_strerror(err)));
    }
    SPDLOG_INFO("AMIDI input open. [{}]", device_name);
    return result;
}

snd_rawmidi_t* AlsaMidi::open_midi_out(const std::string& device_name) {
    snd_rawmidi_t* result;
    int err;
    err = snd_rawmidi_open(NULL, &result, device_name.c_str(), 0);    
    if (err) {
        throw OstrostrojException(fmt::format("AMIDI snd_rawmidi_open {} failed! [err={}]", device_name, snd_strerror(err)));
    }
    SPDLOG_INFO("AMIDI output open. [{}]", device_name);
    return result;
}

AlsaMidi::AlsaMidi():
    handle_in(open_midi_in(MIDI_DEVICE_NAME)),
    handle_out(open_midi_out(MIDI_DEVICE_NAME)),
    stop(false),
    fifo(AlsaMidiFifo(256)),
    clock_counter(0),
    last_clock(std::chrono::steady_clock::now()),
    clock_interval(std::chrono::steady_clock::duration::min()),
    thru_thread(0),
    callback(0) {
}

AlsaMidi::~AlsaMidi() {
    shutdown();
    if (parser) {
        snd_midi_event_free(parser);
        parser = nullptr;
    }
    if (handle_in) {
        snd_rawmidi_close(handle_in);
        handle_in = nullptr;
    }
    if (handle_out) {
        snd_rawmidi_close(handle_out);
        handle_out = nullptr;
    }
}

AlsaMidiFifo& AlsaMidi::get_fifo() {
    return fifo;
}

void AlsaMidi::start(std::function<void(void)> _callback) {
    if (thru_thread || callback) {
        throw OstrostrojException(fmt::format("AMIDI thread already started! [{}]", thru_thread));
    }
    callback = _callback;
    thru_thread = create_rt_thread("alsa_midi", 80, run_midi, this);
}

void AlsaMidi::shutdown() {
    if (thru_thread) {
        stop = true;
        void* status;
        pthread_join(thru_thread, &status);
        thru_thread = 0;
    }
}