#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "screen.hpp"

Screen::Screen(Canvas& _canvas):
    canvas(_canvas) {
};