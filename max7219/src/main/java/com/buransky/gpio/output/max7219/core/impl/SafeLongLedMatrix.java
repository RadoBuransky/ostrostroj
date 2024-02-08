package com.buransky.gpio.output.max7219.core.impl;

import static com.google.common.base.Preconditions.checkArgument;

public class SafeLongLedMatrix extends FastLedMatrix {
    public SafeLongLedMatrix(final int displayRows,
                             final int displayColumns,
                             final int displaysVertically,
                             final int displaysHorizontally) {
        super(displayRows, displayColumns, displaysVertically, displaysHorizontally);
    }

    public SafeLongLedMatrix(final int displayRows,
                             final int displayColumns,
                             final int displaysVertically,
                             final int displaysHorizontally,
                             final long[] displays) {
        super(displayRows, displayColumns, displaysVertically, displaysHorizontally, displays);
    }

    @Override
    public boolean getLedStatus(final int row, final int column) {
        checkArgument(row >= 0);
        checkArgument(row < displayRows);
        checkArgument(column >= 0);
        checkArgument(column < displayColumns);
        synchronized(this) {
            return super.getLedStatus(row, column);
        }
    }

    @Override
    public void setLedStatus(final int row, final int column, final boolean ledOn) {
        checkArgument(row >= 0);
        checkArgument(row < displayRows);
        checkArgument(column >= 0);
        checkArgument(column < displayColumns*displays.length);
        synchronized(this) {
            super.setLedStatus(row, column, ledOn);
        }
    }
}
