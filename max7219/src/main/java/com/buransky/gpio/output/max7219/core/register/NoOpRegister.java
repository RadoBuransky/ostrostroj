package com.buransky.gpio.output.max7219.core.register;

import com.buransky.gpio.output.max7219.core.Register;

public class NoOpRegister implements Register {
    public static final NoOpRegister INSTANCE = new NoOpRegister();
    private static final RegisterAddress registerAddress = RegisterAddress.NoOp;

    private NoOpRegister() {
    }

    @Override
    public short getAddress() {
        return registerAddress.getAddress();
    }

    @Override
    public short getData() {
        return 0;
    }
}