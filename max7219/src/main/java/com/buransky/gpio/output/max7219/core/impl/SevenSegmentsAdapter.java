package com.buransky.gpio.output.max7219.core.impl;

import com.buransky.gpio.output.max7219.core.Register;
import com.buransky.gpio.output.max7219.core.SevenSegments;

import java.util.Collection;
import java.util.List;

/**
 * Adapter forwarding calls to fast LED matrix implementation. It's only about semantics.
 */
public class SevenSegmentsAdapter implements SevenSegments {
    private final FastLedMatrix ledMatrix;

    public SevenSegmentsAdapter(final int digitSegments, final int displayDigits, final int displayCount) {
        ledMatrix = new FastLedMatrix(displayDigits, digitSegments, 1, displayCount);
    }

    @Override
    public Iterable<PinState> reset() {
        throw new UnsupportedOperationException("TODO");
    }

    @Override
    public List<PinState> execute(final Collection<Register> register) {
        return ledMatrix.execute(register);
    }

    @Override
    public List<PinState> executeAll(Register register) {
        return ledMatrix.executeAll(register);
    }

    @Override
    public List<PinState> draw() {
        return ledMatrix.draw();
    }

    @Override
    public void setSegmentStatus(int segment, int digit, int display, boolean segmentOn) {
        throw new UnsupportedOperationException("TODO");
    }

    @Override
    public void setDigitValue(int digit, int display, int value) {
        throw new UnsupportedOperationException("TODO");
    }

    @Override
    public void setDisplayValue(int display, int value) {
        throw new UnsupportedOperationException("TODO");
    }
}
