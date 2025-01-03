#pragma once
#include "canvas.hpp"

class Screen {
    public:
        virtual ~Screen() = default;
        virtual bool draw(Canvas& canvas) = 0;
};