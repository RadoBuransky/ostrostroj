package com.buransky.gpio.output.max7219;

public interface EightSegmentDisplay {
    void number(byte[] digits, boolean[] points);
}
