package com.buransky.gpio.output.max7219.core.register;

import com.buransky.gpio.output.max7219.core.Register;

/**
 * Table 7. Intensity Register Format (Address (Hex) = 0xXA)
 */
public class IntensityRegister implements Register {
    private static final RegisterAddress registerAddress = RegisterAddress.Intensity;
    private final short data;

    public IntensityRegister(final short data) {
        this.data = data;
    }

    @Override
    public short getAddress() {
        return registerAddress.getAddress();
    }

    @Override
    public short getData() {
        return data;
    }
}
