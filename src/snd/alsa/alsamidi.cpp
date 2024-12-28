#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "alsamidi.hpp"

static constexpr char MIDI_DEVICE_NAME[] = "UMC1820";
static constexpr int POLL_TIMEOUT_MS = 200;
static constexpr uint8_t PGMCHANGE_ADVANCE_CLOCKS = 11; // Number of SND_SEQ_EVENT_CLOCK messages before actual program change happens

bool AlsaMidi::process(snd_seq_event_t& event) {
    if (event.type == SND_SEQ_EVENT_CLOCK) {
        clock_counter++;
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        clock_interval = now - last_clock;
        last_clock = now;

        if (clock_counter == 2) {
            // This is an ugly hack. We need latency to initialize engine correctly but I am lazy to do it properly.
            event.type = SND_SEQ_EVENT_PGMCHANGE;
            event.data.control.param = 0;
            event.data.control.value = 0;
        }
    }
    event.time.tick = clock_counter;

    if (event.type != SND_SEQ_EVENT_CLOCK) {
        std::chrono::milliseconds clock_interval_ms = std::chrono::duration_cast<std::chrono::milliseconds>(clock_interval);
        SPDLOG_DEBUG("AMIDI event [type={},clock_counter={},clock_interval={}ms]", (int)event.type, clock_counter, clock_interval_ms.count());
    } else {
        SPDLOG_TRACE("AMIDI clock [queue={}, 0={},1={}]", event.data.queue.queue, event.data.queue.param.d32[0], event.data.queue.param.d32[1]);
    }

    bool push = false;
    switch (event.type) {
        case SND_SEQ_EVENT_START:
            quarter_note_counter = 0;
            push = true;
            event.data.control.param = 0;
            event.data.control.value = 0;
            event.data.control.unused[0] = PGMCHANGE_ADVANCE_CLOCKS;
            event.data.control.unused[1] = (uint8_t)std::chrono::duration_cast<std::chrono::milliseconds>(clock_interval).count();
            break;
        case SND_SEQ_EVENT_PGMCHANGE:
            push = true;
            event.data.control.unused[0] = PGMCHANGE_ADVANCE_CLOCKS;
            event.data.control.unused[1] = (uint8_t)std::chrono::duration_cast<std::chrono::milliseconds>(clock_interval).count();
            break;
        case SND_SEQ_EVENT_NOTEON:
            push = true;
            SPDLOG_TRACE("AMIDI NOTEON [ch={},note={},velocity={},off_velocity={}]", event.data.note.channel, event.data.note.note,
                event.data.note.velocity, event.data.note.off_velocity);
            break;
        case SND_SEQ_EVENT_NOTEOFF:
        case SND_SEQ_EVENT_CONTINUE:
        case SND_SEQ_EVENT_STOP:
        case SND_SEQ_EVENT_SETPOS_TICK:
        case SND_SEQ_EVENT_SETPOS_TIME:        
        case SND_SEQ_EVENT_CONTROLLER:
            push = true;
            break;
        case SND_SEQ_EVENT_CLOCK:
            // 24 events per quarter note (MIDI specification)
            quarter_note_counter = (quarter_note_counter + 1) % 24;
            if (quarter_note_counter == 0) {
                push = true;
            }
            break;
    }
    if (push) {
        if (!fifo_in.push(std::move(event))) {
            SPDLOG_ERROR("AMIDI FIFO overrun!");
        }
        callback();
    }
    return true;
}

void AlsaMidi::write(unsigned char* raw, size_t size) {
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
        ssize_t consumed_size = snd_midi_event_encode(parser_in, raw, read_size, &event);
        if (consumed_size < 0) {
            SPDLOG_ERROR("AMIDI snd_midi_event_encode_byte failed [err={}]", snd_strerror(consumed_size));
            snd_midi_event_reset_encode(parser_in);
            return;
        }
        if (consumed_size > 0 && event.type != SND_SEQ_EVENT_NONE) {
            if (process(event)) {
                write(raw, consumed_size);
            }
        }
        read_size -= consumed_size;
        raw += consumed_size;
    }
}

size_t AlsaMidi::read(snd_rawmidi_t* handle_in, unsigned char* raw, size_t size) {
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
    int res = poll(poll_descriptors.data(), poll_descriptors.size(), POLL_TIMEOUT_MS);
    if (res < 0) {
        if (errno == EINTR) {
            throw OstrostrojException("AMIDI poll interrupted.");
        }
        throw OstrostrojException(fmt::format("AMIDI poll failed. [err={}]", strerror(errno)));
    }
    if (res == 0) {
        return false;
    }
    for (size_t i = 0; i < handle_ins.size(); i++) {
        snd_rawmidi_t* handle_in = handle_ins.at(i);
        if ((res = snd_rawmidi_poll_descriptors_revents(handle_in, &poll_descriptors.at(i), 1, &revents)) < 0) {
            throw OstrostrojException(fmt::format("AMIDI cannot get poll events [err={}]", snd_strerror(errno)));
        }
        if (revents & (POLLERR | POLLHUP)) {
            throw OstrostrojException(fmt::format("AMIDI poll error [revents={}]", revents));
        }
        if (revents & POLLIN) {
            return true;
        }
    }
    return false;
}

void AlsaMidi::run() {
    try {
        std::array<unsigned char, 4> raw;
        std::vector<pollfd> poll_descriptors = std::vector<pollfd>();
        for (snd_rawmidi_t* handle_in : handle_ins) {
            poll_descriptors.push_back(create_poll_descriptors(handle_in));
        }       
        SPDLOG_INFO("AMIDI started. [{} devices, {} poll descriptors]", handle_ins.size(), poll_descriptors.size());
        while (!stop) {
            if (poll_in(poll_descriptors)) {
                for (snd_rawmidi_t* handle_in : handle_ins) {
                    size_t read_size = read(handle_in, raw.data(), raw.size());
                    if (read_size > 0) {
                        parse(raw.data(), read_size);
                    }
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

pollfd AlsaMidi::create_poll_descriptors(snd_rawmidi_t *handle) {
    pollfd result;
    int count = snd_rawmidi_poll_descriptors_count(handle);
    if (count != 1) {
        throw OstrostrojException(fmt::format("AMIDI One descriptor expected! [count={}]", count));
    }
    int filled = snd_rawmidi_poll_descriptors(handle, &result, 1);
    if (filled < 0) {
        throw OstrostrojException(fmt::format("AMIDI snd_rawmidi_poll_descriptors failed! [err={}]", snd_strerror(filled)));
    }
    if (filled != count) {
        throw OstrostrojException(fmt::format("AMIDI poll descriptor init failed! [filled={}]", filled));
    }
    return result;
}

std::vector<snd_rawmidi_t*> AlsaMidi::open_midi_ins() {
    std::vector<snd_rawmidi_t*> result = std::vector<snd_rawmidi_t*>();
    int err;

    void** hints;
    err = snd_device_name_hint(-1, "rawmidi", &hints);
    if (err) {
        throw OstrostrojException(fmt::format("AMIDI snd_device_name_hint failed! [err={}]", snd_strerror(err)));
    }

    void** n = hints;
    while (*n != nullptr) {
        char *device_name = snd_device_name_get_hint(*n, "NAME");
        if (device_name != nullptr) {
            if (std::string(device_name).starts_with("hw:")) {
                snd_rawmidi_t* rawmidi_handle;
                err = snd_rawmidi_open(&rawmidi_handle, NULL, device_name, SND_RAWMIDI_NONBLOCK);
                if (err) {
                    throw OstrostrojException(fmt::format("AMIDI snd_rawmidi_open {} failed! [err={}]", device_name, snd_strerror(err)));
                }
                result.push_back(rawmidi_handle);
                SPDLOG_INFO("AMIDI input open. [{}]", device_name);
            }
            free(device_name);
        }
        n++;
    }
    snd_device_name_free_hint((void**)hints);

    err = snd_midi_event_new(256, &parser_in);
    if (err < 0) {
        throw OstrostrojException(fmt::format("AMIDI snd_midi_event_new failed! [err={}]", snd_strerror(err)));
    }
    return result;
}

std::string AlsaMidi::find_midi_out_device_name() {    
    std::string result = std::string();
    int err;

    void** hints;
    err = snd_device_name_hint(-1, "rawmidi", &hints);
    if (err) {
        throw OstrostrojException(fmt::format("AMIDI snd_device_name_hint failed! [err={}]", snd_strerror(err)));
    }

    void** n = hints;
    while (*n != nullptr) {
        char *device_name = snd_device_name_get_hint(*n, "NAME");
        if (device_name != nullptr) {
            if (std::string(device_name).find(MIDI_DEVICE_NAME) != std::string::npos) {
                result.assign(device_name);
            }
            free(device_name);
        }
        n++;
    }
    snd_device_name_free_hint((void**)hints);

    if (result.empty()) {
        throw OstrostrojException(fmt::format("AMIDI out device not found! [{}]", MIDI_DEVICE_NAME));
    }
    return result;
}

snd_rawmidi_t* AlsaMidi::open_midi_out(const std::string& device_name) {
    snd_rawmidi_t* result;
    int err;
    err = snd_rawmidi_open(NULL, &result, device_name.c_str(), 0);    
    if (err) {
        throw OstrostrojException(fmt::format("AMIDI snd_rawmidi_open {} failed! [err={}]", device_name, snd_strerror(err)));
    }
    err = snd_midi_event_new(256, &parser_out);
    if (err < 0) {
        throw OstrostrojException(fmt::format("AMIDI snd_midi_event_new failed! [err={}]", snd_strerror(err)));
    }
    SPDLOG_INFO("AMIDI output open. [{}]", device_name);
    return result;
}

AlsaMidi::AlsaMidi():
    handle_ins(open_midi_ins()),
    handle_out(open_midi_out(find_midi_out_device_name())),
    stop(false),
    fifo_in(AlsaMidiFifo(256)),
    clock_counter(0),
    quarter_note_counter(0),
    last_clock(std::chrono::steady_clock::now()),
    clock_interval(std::chrono::steady_clock::duration::min()),
    thru_thread(0),
    callback(0) {
}

AlsaMidi::~AlsaMidi() {
    shutdown();
    if (parser_in) {
        snd_midi_event_free(parser_in);
        parser_in = nullptr;
    }
    if (parser_out) {
        snd_midi_event_free(parser_out);
        parser_out = nullptr;
    }
    for (snd_rawmidi_t* handle_in : handle_ins) {
        snd_rawmidi_close(handle_in);
    }
    if (handle_out) {
        snd_rawmidi_close(handle_out);
        handle_out = nullptr;
    }
}

AlsaMidiFifo& AlsaMidi::get_fifo_in() {
    return fifo_in;
}

void AlsaMidi::start(std::function<void(void)> _callback) {
    if (thru_thread || callback) {
        throw OstrostrojException(fmt::format("AMIDI thread already started! [{}]", thru_thread));
    }
    callback = _callback;
    thru_thread = create_rt_thread("alsa_midi", 80, run_midi, this);
}

void AlsaMidi::write(snd_seq_event_t event) {
    std::array<unsigned char, 12> raw;
    ssize_t written_size = snd_midi_event_decode(parser_out, raw.data(), raw.size(), &event);
    if (written_size < 0) {
        SPDLOG_ERROR("AMIDI snd_midi_event_decode failed [err={}]", snd_strerror(written_size));
        snd_midi_event_reset_decode(parser_out);
        return;
    }
    write(raw.data(), written_size);
#ifndef NDEBUG
    switch(event.type) {
        case SND_SEQ_EVENT_CONTROLLER:
            SPDLOG_DEBUG("AMIDI write[SND_SEQ_EVENT_CONTROLLER,channel={},param={},value={}]", event.data.control.channel, event.data.control.param,
                event.data.control.value);
            break;
        default:
            SPDLOG_DEBUG("AMIDI write[type={},raw0={},raw1={},raw2={}]", event.type, event.data.raw32.d[0], event.data.raw32.d[1], event.data.raw32.d[2]);
            break;
    }    
#endif
}

void AlsaMidi::shutdown() {
    if (thru_thread) {
        stop = true;
        void* status;
        pthread_join(thru_thread, &status);
        thru_thread = 0;
    }
}