package com.buransky.gpio.output.max7219.core.register;

import com.buransky.gpio.output.max7219.core.Register;

/**
 * Table 8. Scan-Limit Register Format (Address (Hex) = 0xXB)
 */
public enum ScanLimitRegister implements Register {
    Digit0((short)0x00),
    Digits0to1((short)0x01),
    Digits0to2((short)0x02),
    Digits0to3((short)0x03),
    Digits0to4((short)0x04),
    Digits0to5((short)0x05),
    Digits0to6((short)0x06),
    Digits0to7((short)0x07);

    private static final RegisterAddress registerAddress = RegisterAddress.ScanLimit;
    private final short data;

    ScanLimitRegister(final short data) {
        this.data = data;
    }

    @Override
    public short getAddress() {
        return registerAddress.getAddress();
    }

    public short getData() {
        return data;
    }
}
