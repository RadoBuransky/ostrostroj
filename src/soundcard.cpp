#include <string>
#include <format>
#include "soundcard.h"
#include "common.h"
#include "spdlog/spdlog.h"

static jack_client_t * create_client(const std::string &name) {
    jack_status_t status;
    auto jack_client = jack_client_open(name.c_str(), JackNoStartServer, &status);
    if (nullptr == jack_client) {        
        throw OstrostrojException(std::format("Jack client open failed! [status=0x{:x}]", static_cast<int>(status)));
    }
    spdlog::info("Jack client open.");
    jack_activate(jack_client);
    spdlog::info("Jack client activated.");
    return jack_client;
};

SoundCard::SoundCard(const std::string &name) : jack_client(create_client(name)) {
};

SoundCard::~SoundCard() {
    if (jack_client != nullptr) {
        jack_client_close(jack_client);
        spdlog::info("Jack client closed.");        
    }
};