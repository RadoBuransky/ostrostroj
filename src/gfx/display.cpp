#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "display.hpp"

SystemScreen::SystemScreen():
    init(true),
    mem_usage(0) {    
}

bool SystemScreen::draw(unicorn_hat_mini_canvas& canvas) {    
    if (init) {
        canvas.at(0).at(0) = palette_red;
        canvas.at(1).at(0) = palette_red;
        canvas.at(2).at(0) = palette_red;
        init = false;
        return true;
    }
    size_t mem_usage_cols = std::ceil(mem_usage*UNICORN_HAT_MINI_COLS);
    for (size_t x = 0; x < mem_usage_cols; x++) {
        canvas.at(x).at(0) = palette_cyan;
    }
    return true;
}

void SystemScreen::set_mem_usage(float _mem_usage) {
    mem_usage = std::max(0.0f, std::min(1.0f, _mem_usage));
}

Display::Display(std::chrono::milliseconds _refresh):
    refresh(_refresh),
    unicorn_hat_mini(),
    main_screen(),
    system_screen(),
    active_screen(system_screen),
    next_refresh() {
    tick(true);
}

void Display::tick(bool force) {
    if (!force && std::chrono::steady_clock::now() < next_refresh) {
        return;
    }
    next_refresh = std::chrono::steady_clock::now() + refresh;
    if (active_screen.get().draw(unicorn_hat_mini.get_canvas())) {
        unicorn_hat_mini.show();
    }
}

MainScreen& Display::get_main_screen() {
    return main_screen;
}

SystemScreen& Display::get_system_screen() {
    return system_screen;
}

void Display::set_active_screen(Screen& _screen) {
    active_screen = _screen;
    unicorn_hat_mini.clear();
}