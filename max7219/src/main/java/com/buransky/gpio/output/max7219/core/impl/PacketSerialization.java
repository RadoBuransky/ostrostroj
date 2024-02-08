package com.buransky.gpio.output.max7219.core.impl;

import com.buransky.gpio.output.max7219.core.Max7219;

import java.util.ArrayList;
import java.util.List;

/**
 * Transforms list of 16-bit packets into linear sequence of LOAD/CS, CLK and DIN triplets.
 */
class PacketSerialization {
    private PacketSerialization() {
    }

    /**
     * Transforms list of 16-bit packets (one for each display) into bit state changes.
     * @param packets One 16-bit packet for each display.
     * @return Ordered sequence bit state changes.
     */
    static List<Max7219.PinState> serialize(final List<Short> packets) {
        final ArrayList<Max7219.PinState> result = new ArrayList<>(packets.size()*16*3+2);
        result.add(Max7219.PinState.LOADCS_LOW);
        short previousDin = -1;
        for (int i = packets.size() - 1; i >= 0; i--) {
            previousDin = packetToClkDin(packets.get(i), result, previousDin);
        }
        result.add(Max7219.PinState.LOADCS_HIGH);
        return result;
    }

    /**
     * Transforms single 16-bit into bit state changes. Max result length = 16*3 = 48 bytes.
     */
    private static short packetToClkDin(final short packet, final ArrayList<Max7219.PinState> dest, short previousDin) {
        short shiftedPacket = packet;
        for (int i = 0; i < 16; i++) {
            final short din = (short)(shiftedPacket & 0x8000);
            if (din != previousDin) {
                dest.add((din != 0) ? Max7219.PinState.DIN_HIGH : Max7219.PinState.DIN_LOW);
                previousDin = din;
            }
            dest.add(Max7219.PinState.CLK_HIGH);
            dest.add(Max7219.PinState.CLK_LOW);
            shiftedPacket <<= 1;
        }
        return previousDin;
    }
}
