package com.buransky.gpio.output.max7219.core.register;

import com.buransky.gpio.output.max7219.core.Register;

import static com.buransky.gpio.output.max7219.core.register.RegisterAddress.*;
import static com.google.common.base.Preconditions.checkArgument;

public class DigitRegister implements Register {
    public static final RegisterAddress[] DIGITS = { Digit0, Digit1, Digit2, Digit3, Digit4, Digit5, Digit6, Digit7 };
    private final RegisterAddress registerAddress;
    private final short data;

    public DigitRegister(final RegisterAddress registerAddress, final short data) {
        checkArgument(registerAddress.getAddress() >= Digit0.getAddress());
        checkArgument(registerAddress.getAddress() <= Digit7.getAddress());
        this.registerAddress = registerAddress;
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