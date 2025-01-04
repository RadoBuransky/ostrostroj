#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "display.hpp"

SystemScreen::SystemScreen(Canvas& _canvas):
    Screen(_canvas),
    init(true),
    mem_usage(0) {    
}

bool SystemScreen::draw() {    
    if (init) {
        canvas.point(0, 0, palette_red);
        canvas.point(1, 0, palette_red);
        canvas.point(2, 0, palette_red);
        init = false;
        return true;
    }
    size_t mem_usage_cols = std::ceil(mem_usage*UNICORN_HAT_MINI_COLS);
    for (size_t x = 0; x < mem_usage_cols; x++) {
        canvas.point(x, 0, palette_cyan);
    }
    return true;
}

void SystemScreen::set_mem_usage(float _mem_usage) {
    mem_usage = std::max(0.0f, std::min(1.0f, _mem_usage));
}

Display::Display(std::chrono::milliseconds _refresh):
    refresh(_refresh),
    unicorn_hat_mini(),
    main_screen(unicorn_hat_mini),
    system_screen(unicorn_hat_mini),
    active_screen(system_screen),
    next_refresh() {
    tick(true);
}

void Display::tick(bool force) {
    if (!force && std::chrono::steady_clock::now() < next_refresh) {
        return;
    }
    next_refresh = std::chrono::steady_clock::now() + refresh;
    if (active_screen.get().draw()) {
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

Canvas& Display::get_canvas() {
    return unicorn_hat_mini;
}