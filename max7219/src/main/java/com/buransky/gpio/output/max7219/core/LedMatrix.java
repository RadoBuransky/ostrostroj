package com.buransky.gpio.output.max7219.core;

/**
 * Stateful bit matrix where each bit represents a LED in the matrix.
 */
public interface LedMatrix extends Max7219 {
    /**
     * Retrieves LED status at the provided position.
     * @param row Row in the LED matrix.
     * @param column Column in the LED matrix.
     * @return `true` if the LED is on, false otherwise.
     */
    boolean getLedStatus(final int row, final int column);

    /**
     * Sets LED status at the provided position.
     * @param row Row in the LED matrix.
     * @param column Column in the LED matrix.
     * @param ledOn `true` if the LED should be turned on, false otherwise.
     */
    void setLedStatus(final int row, final int column, final boolean ledOn);
}
