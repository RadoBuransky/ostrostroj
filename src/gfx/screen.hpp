#pragma once
#include "canvas.hpp"

class Screen {
    protected:
        Canvas& canvas;
    public:
        Screen(Canvas& _canvas);
        virtual ~Screen() = default;
        virtual bool draw() = 0;
};