package com.buransky.gpio.output.max7219.core.register;

import com.buransky.gpio.output.max7219.core.Register;

/**
 * Table 4. Decode-Mode Register Examples (Address (Hex) = 0xX9)
 */
public enum DecodeModeRegister implements Register {
    NoDecode((short)0x00),
    CodeBDecodeFor0((short)0x01),
    CodeBDecodeFor3to0((short)0x0F),
    CodeBDecodeFor7to0((short)0xFF);

    private static final RegisterAddress registerAddress = RegisterAddress.DecodeMode;
    private final short data;

    DecodeModeRegister(final short data) {
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
