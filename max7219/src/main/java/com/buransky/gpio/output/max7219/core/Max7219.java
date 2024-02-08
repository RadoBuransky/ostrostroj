package com.buransky.gpio.output.max7219.core;

import com.buransky.gpio.output.max7219.core.impl.FastLedMatrix;
import com.buransky.gpio.output.max7219.core.impl.SevenSegmentsAdapter;

import java.util.Collection;

/**
 * Maxim Integrated MAX7219 chip library API. Can be used to control 8-digit 7-segment LED displays or LED matrix
 * displays. Cascading of unlimited number of chips is supported as well. Individual cascaded chips are referred to as
 * "displays". Consumer of this library is expected to execute returned pin commands using any library - typically Pi4j
 * or WiringPi. This interface is stateful and not thread-safe for performance reasons.
 */
public interface Max7219 {
    /**
     * Prepares Max7219 interface for use as a LED matrix. Usually MAX7219 chips are cascaded just horizontally forming
     * a line. For example if you stack 4 displays horizontally where each display has 8x8 LEDs then you call this
     * method with parameters displayRows = 8, displayColumns = 8, displaysHorizontally = 4, displaysVertically = 1.
     * But this library supports any rectangular arrangement of displays. For example you can stack 12 displays as 4x3
     * resulting in LED matrix totalling 32x24 LEDs.
     * @param displayRows Number of rows of a single display in the LED matrix. Typically 8.
     * @param displayColumns Number of columns in a single display in the LED matrix. Typically 8.
     * @param displaysVertically Number of MAX7219 chips cascaded in vertical direction. Typically 1.
     * @param displaysHorizontally Number of MAX7219 chips cascaded in horizontal direction. Typically 4.
     * @return LedMatrix instance.
     */
    static LedMatrix initLedMatrix(final int displayRows,
                                   final int displayColumns,
                                   final int displaysVertically,
                                   final int displaysHorizontally) {
        return new FastLedMatrix(displayRows, displayColumns, displaysVertically, displaysHorizontally);
    }

    /**
     * Prepares Max7219 interface for use with "classical" 7-segment displays. In fact 8-segments are supported too and
     * each display can have up to 8 digits. Unlimited number of displays can be cascaded together.
     * @param digitSegments Number of segments of a single digit. Typically 7.
     * @param displayDigits Number of digits in a single display. Typically 8 I guess.
     * @param displayCount Number of MAX7219 cascaded together.
     * @return SevenSegments instance.
     */
    static SevenSegments initSevenSegments(final int digitSegments, final int displayDigits, final int displayCount) {
        return new SevenSegmentsAdapter(digitSegments, displayDigits, displayCount);
    }

    /**
     * Initializes MAX7219 setting the default values and clearing the display.
     * @return Ordered sequence of pin states to be executed.
     */
    Iterable<PinState> reset();

    /**
     * Executes the provided command described as a pair of register address and register data on a single display.
     * @param register Register address and data to be executed.
     * @return Ordered sequence of pin states to be executed.
     */
    Iterable<PinState> execute(final Collection<Register> register);

    /**
     * Executes the provided command described as a pair of register address and register data on all displays.
     * @param register Register address and data to be executed.
     * @return Ordered sequence of pin states to be executed.
     */
    Iterable<PinState> executeAll(final Register register);

    /**
     * Draws current state (LED matrix or 7-segment display) and internally keeps reference to what has been drawn for
     * the next call so that it can compute difference for optimization reasons.
     * @return Ordered sequence of pin states to be executed.
     */
    Iterable<PinState> draw();

    /**
     * Pin state to be set.
     */
    enum PinState {
        LOADCS_HIGH,
        LOADCS_LOW,
        CLK_HIGH,
        CLK_LOW,
        DIN_HIGH,
        DIN_LOW;
    }
}