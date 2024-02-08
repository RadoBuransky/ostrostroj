package com.buransky.gpio.output;

import com.buransky.gpio.output.max7219.EightSegmentDisplay;
import com.buransky.gpio.output.max7219.LedMatrixDisplay;

public interface GpioGraphics {
    LedMatrixDisplay ledMatrixDisplay(int dinPin, int csPin, int clkPin, int width, int height);
    EightSegmentDisplay eightSegmentDisplay(int dinPin, int csPin, int clkPin, int digits);
    void flip();
}
