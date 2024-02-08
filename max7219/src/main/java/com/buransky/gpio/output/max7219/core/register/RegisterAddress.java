package com.buransky.gpio.output.max7219.core.register;

/**
 * Table 2. Register Address Map
 */
public enum RegisterAddress {
    NoOp((short)0x00),
    Digit0((short)0x01),
    Digit1((short)0x02),
    Digit2((short)0x03),
    Digit3((short)0x04),
    Digit4((short)0x05),
    Digit5((short)0x06),
    Digit6((short)0x07),
    Digit7((short)0x08),
    DecodeMode((short)0x09),
    Intensity((short)0x0A),
    ScanLimit((short)0x0B),
    Shutdown((short)0x0C),
    DisplayTest((short)0x0F);

    private final short address;

    RegisterAddress(final short address) {
        this.address = address;
    }

    public short getAddress() {
        return address;
    }
}