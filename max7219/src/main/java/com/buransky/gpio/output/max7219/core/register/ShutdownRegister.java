package com.buransky.gpio.output.max7219.core.register;

import com.buransky.gpio.output.max7219.core.Register;

/**
 * Table 3. Shutdown Register Format (Address (Hex) = 0xXC)
 */
public enum ShutdownRegister implements Register {
    ShutdownMode((short)0x00),
    NormalOperation((short)0x01);

    private static final RegisterAddress registerAddress = RegisterAddress.Shutdown;
    private final short data;

    ShutdownRegister(final short data) {
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
