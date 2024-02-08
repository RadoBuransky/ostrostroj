package com.buransky.gpio.output.max7219.core;

/**
 * Table 1. Serial-Data Format (16 Bits). D15 to D12 are "don't care" bits.
 */
public interface Register {
    /**
     * 4 bits - from D11 down to D8.
     * @return Address value.
     */
    short getAddress();

    /**
     * 8 bits - from D7 (MSG) down to D0 (LSB)
     * @return Address value.
     */
    short getData();
}
