package com.buransky.gpio.pi4j;

import com.buransky.gpio.output.GpioGraphics;
import com.buransky.gpio.output.max7219.EightSegmentDisplay;
import com.buransky.gpio.output.max7219.LedMatrixDisplay;
import com.pi4j.io.gpio.GpioController;

public class Pi4jGpioGraphics implements GpioGraphics {
    public Pi4jGpioGraphics(final GpioController gpioController) {

    }

    @Override
    public LedMatrixDisplay ledMatrixDisplay(int dinPin, int csPin, int clkPin, int width, int height) {
        return null;
    }

    @Override
    public EightSegmentDisplay eightSegmentDisplay(int dinPin, int csPin, int clkPin, int digits) {
        return null;
    }

    @Override
    public void flip() {

    }
}
