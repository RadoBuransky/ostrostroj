#pragma once

#include "unicornhatmini.hpp"
#include "pattern.hpp"
#include "screen.hpp"
#include "main_screen.hpp"

class SystemScreen : public Screen {
    private:
        bool init;
        float mem_usage;
    public:
        SystemScreen();
        virtual ~SystemScreen() = default;
        virtual bool draw(Canvas& canvas);
        void set_mem_usage(float _mem_usage);
};

class Display {
    private:
        const std::chrono::milliseconds refresh;
        UnicornHatMini unicorn_hat_mini;
        MainScreen main_screen;
        SystemScreen system_screen;
        std::reference_wrapper<Screen> active_screen;
        std::chrono::time_point<std::chrono::steady_clock> next_refresh;

    public:
        Display(std::chrono::milliseconds _refresh);
        virtual ~Display() = default;

        /**
         * Call this as often as you want, it won't refresh the screen faster than "refresh" period.
        */
        void tick(bool force);

        MainScreen& get_main_screen();
        SystemScreen& get_system_screen();
        void set_active_screen(Screen& _screen);
};