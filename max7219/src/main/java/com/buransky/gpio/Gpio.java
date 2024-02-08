package com.buransky.gpio;

import com.buransky.gpio.input.GpioSwitches;
import com.buransky.gpio.output.GpioGraphics;
import com.buransky.gpio.pi4j.Pi4jGpio;
import com.pi4j.platform.Platform;
import com.pi4j.platform.PlatformAlreadyAssignedException;
import com.pi4j.platform.PlatformManager;

public interface Gpio {
    static Gpio odroid() {
        try {
            PlatformManager.setPlatform(Platform.ODROID);
        } catch (final PlatformAlreadyAssignedException e) {
            throw new GpioException(e);
        }
        return new Pi4jGpio();
    }

    GpioSwitches getGpioSwitches();
    GpioGraphics getGpioGraphics();

    class GpioException extends RuntimeException {
        public GpioException() {
        super();
    }
        public GpioException(String message) {
            super(message);
        }
        public GpioException(String message, Throwable cause) {
            super(message, cause);
        }
        public GpioException(Throwable cause) {
            super(cause);
        }
    }
}
