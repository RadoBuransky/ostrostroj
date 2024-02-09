package com.buransky.gpio.output.max7219.core.impl;

import com.buransky.gpio.output.max7219.core.LedMatrix;
import com.buransky.gpio.output.max7219.core.Register;
import com.buransky.gpio.output.max7219.core.register.*;

import java.util.*;

import static com.google.common.base.Preconditions.checkArgument;
import static com.google.common.base.Preconditions.checkNotNull;

public class FastLedMatrix implements LedMatrix {
    public static final long MAX_DISPLAY_ROWS = 8;
    public static final long MAX_DISPLAY_COLUMNS = 8;

    protected final long displayRows;
    protected final long displayColumns;
    protected final long displaysVertically;
    protected final long displaysHorizontally;
    protected final long[] displays;
    protected final long[] previousDisplays;
    protected boolean anyChange;

    public FastLedMatrix(final int displayRows,
                         final int displayColumns,
                         final int displaysVertically,
                         final int displaysHorizontally) {
        this(displayRows, displayColumns, displaysVertically, displaysHorizontally,
                new long[displaysVertically*displaysHorizontally]);
        checkArgument(displaysVertically > 0);
        checkArgument(displaysHorizontally > 0);
    }

    public FastLedMatrix(final int displayRows,
                         final int displayColumns,
                         final int displaysVertically,
                         final int displaysHorizontally,
                         final long[] displays) {
        checkArgument(displayRows > 0);
        checkArgument(displayRows <= MAX_DISPLAY_ROWS);
        checkArgument(displayColumns > 0);
        checkArgument(displayColumns <= MAX_DISPLAY_COLUMNS);
        this.displayColumns = displayColumns;
        this.displayRows = displayRows;
        this.displaysVertically = displaysVertically;
        this.displaysHorizontally = displaysHorizontally;
        this.displays = displays;
        this.previousDisplays = displays.clone();
        this.anyChange = false;
    }

    @Override
    public boolean getLedStatus(final int row, final int column) {
        final int displayIndex = (int)getDisplayIndex(row, column);
        final int bitPosition = (int)getBitPosition(row % displayRows, column % displayColumns);
        return getBit(displays[displayIndex], bitPosition);
    }

    @Override
    public void setLedStatus(final int row, final int column, final boolean ledOn) {
        final int displayIndex = (int)getDisplayIndex(row, column);
        if (displayIndex < displaysHorizontally*displaysVertically) {
            final int bitPosition = (int) getBitPosition(row % displayRows, column % displayColumns);
            if (bitPosition < displayRows*displayColumns) {
                displays[displayIndex] = setBit(displays[displayIndex], bitPosition, ledOn);
                anyChange = true;
            }
        }
    }

    @Override
    public List<PinState> reset() {
        final ArrayList<PinState> result = new ArrayList<>();

        // Initialize control registers
        result.addAll(executeAll(DisplayTestRegister.NormalOperation));
        result.addAll(executeAll(DecodeModeRegister.NoDecode));
        result.addAll(executeAll(ScanLimitRegister.Digits0to7));
        result.addAll(executeAll(new IntensityRegister((short)0)));
        result.addAll(executeAll(ShutdownRegister.NormalOperation));

        // Clear screen
        Arrays.fill(displays, 0);
        Arrays.fill(previousDisplays, -1);
        anyChange = true;
        result.addAll(draw());

        return result;
    }

    @Override
    public List<PinState> execute(final Collection<Register> registers) {
        checkNotNull(registers);
        checkArgument(registers.size() == displays.length);

        final ArrayList<Short> packets = new ArrayList<>(displays.length);
        for (final Register register : registers) {
            packets.add(registerToPacket(register));
        }

        return PacketSerialization.serialize(packets);
    }

    @Override
    public List<PinState> executeAll(final Register register) {
        checkNotNull(register);
        return execute(Collections.nCopies(displays.length, register));
    }

    @Override
    public List<PinState> draw() {
        if (!anyChange) {
            return Collections.EMPTY_LIST;
        }

        final ArrayList<List<DigitRegister>> digitRegisters = new ArrayList<>(displays.length);
        final int stepCount = getDigitRegisters(digitRegisters);
        anyChange = false;
        System.arraycopy(displays, 0, previousDisplays, 0, displays.length);
        return executeDigitRegisters(digitRegisters, stepCount);
    }

    private int getDigitRegisters(final List<List<DigitRegister>> digitRegisters) {
        int stepCount = 0;
        for (int display = 0; display < displays.length; display++) {
            final List<DigitRegister> displayDigitRegisters = getDisplayDigitRegisters(display);
            if (displayDigitRegisters.size() > stepCount) {
                stepCount = displayDigitRegisters.size();
            }
            digitRegisters.add(display, displayDigitRegisters);
        }
        return stepCount;
    }

    private List<DigitRegister> getDisplayDigitRegisters(final int display) {
        long displayData = displays[display];
        long displayDiffMask = displayData ^ previousDisplays[display];
        if (displayDiffMask == 0) {
            return Collections.EMPTY_LIST;
        }
        return getDisplayDigitRegisters(displayData, displayDiffMask);
    }

    private List<DigitRegister> getDisplayDigitRegisters(long displayData, long displayDiffMask) {
        final long rowMask = (0xFFL >>> (8L - displayColumns));
        final ArrayList<DigitRegister> result = new ArrayList<>(8);
        for (int row = 0; row < displayRows; row++) {
            if ((displayDiffMask & rowMask) != 0) {
                result.add(new DigitRegister(DigitRegister.DIGITS[row], (short)(displayData & rowMask)));
            }
            displayData >>>= displayColumns;
            displayDiffMask >>>= displayColumns;
        }
        return result;
    }

    private List<PinState> executeDigitRegisters(final ArrayList<List<DigitRegister>> digitRegisters,
                                                 final int stepCount) {
        final ArrayList<PinState> result = new ArrayList<>();
        for (int step = 0; step < stepCount; step++) {
            final Collection<Register> stepRegisters = getStepRegisters(digitRegisters, step);
            final List<PinState> stepPinStates = execute(stepRegisters);
            result.addAll(stepPinStates);
        }
        return result;
    }

    private Collection<Register> getStepRegisters(final ArrayList<List<DigitRegister>> digitRegisters, final int step) {
        final Collection<Register> stepRegisters = new ArrayList<>(displays.length);
        for (int display = 0; display < displays.length; display++) {
            final List<DigitRegister> displayRegisters = digitRegisters.get(display);
            if (step < displayRegisters.size()) {
                stepRegisters.add(displayRegisters.get(step));
            } else {
                stepRegisters.add(NoOpRegister.INSTANCE);
            }
        }
        return stepRegisters;
    }

    private short registerToPacket(final Register register) {
        return (short)(((register.getAddress() & 0x0F) << 8) | (register.getData()));
    }

    private long getDisplayIndex(final int row, final long column) {
        return (row / displayRows) * displaysHorizontally + (column / displayColumns);
    }

    private long getBitPosition(final long row, final long column) {
        return column + (row * displayColumns);
    }

    private boolean getBit(final long number, final long position) {
        return ((number >> position) & 1L) == 1L;
    }

    private long setBit(final long number, final long position, final boolean value) {
        if (value) {
            return number | (1L << position);
        }
        return number & (~(1L << position));
    }
}
