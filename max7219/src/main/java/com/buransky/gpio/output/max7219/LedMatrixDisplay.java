package com.buransky.gpio.output.max7219;

public interface LedMatrixDisplay {
    void point(int x, int y, int color);
    void line(int startX, int startY, int endX, int endY, int color);
}
