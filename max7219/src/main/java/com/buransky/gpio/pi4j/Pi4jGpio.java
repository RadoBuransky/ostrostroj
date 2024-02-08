package com.buransky.gpio.pi4j;

import com.buransky.gpio.Gpio;
import com.buransky.gpio.input.GpioSwitches;
import com.buransky.gpio.output.GpioGraphics;
import com.pi4j.io.gpio.GpioController;
import com.pi4j.io.gpio.GpioFactory;

public class Pi4jGpio implements Gpio {
    private final GpioController gpioController;
    private final Pi4jGpioSwitches gpioSwitches;
    private final Pi4jGpioGraphics gpioGraphics;

    public Pi4jGpio() {
        gpioController = GpioFactory.getInstance();
        gpioSwitches = new Pi4jGpioSwitches(gpioController);
        gpioGraphics = new Pi4jGpioGraphics(gpioController);
    }

    @Override
    public GpioSwitches getGpioSwitches() {
        return gpioSwitches;
    }

    @Override
    public GpioGraphics getGpioGraphics() {
        return gpioGraphics;
    }
}
