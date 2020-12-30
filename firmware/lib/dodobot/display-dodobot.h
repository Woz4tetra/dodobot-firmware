
#ifndef __DODOBOT_DISPLAY_H__
#define __DODOBOT_DISPLAY_H__

#include <Arduino.h>

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>

#include "dodobot.h"


/*
 * Adafruit TFT 1.8" display, 160x128
 * ST7735
 */

#define TFT_CS    10
#define TFT_RST    9
#define TFT_DC     8
#define TFT_LITE   6

namespace dodobot_display
{
    const float TFT_PI = 3.1415926;
    uint32_t tft_display_timer = 0;

    Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
    uint8_t tft_brightness;

    void set_display_brightness(int brightness)
    {
        analogWrite(TFT_LITE, brightness);
        tft_brightness = brightness;
    }

    void black_display() {
        tft.fillScreen(ST77XX_BLACK);
    }

    void setup_display()
    {
        pinMode(TFT_LITE, OUTPUT);
        tft.initR(INITR_BLACKTAB);      // Init ST7735S chip, black tab
        delay(10);
        set_display_brightness(255);
        black_display();
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);

        tft.setTextWrap(false);
        tft.setTextSize(1);
        tft.setRotation(3); // horizontal display

        tft.print("Hello!\n");
        dodobot_serial::println_info("Display ready");
    }

    void pushRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data)
    {
        tft.startWrite();
        tft.setAddrWindow(x, y, w, h);
        for (int32_t row = 0; row < w; row++) {
            for (int32_t col = 0; col < h; col++) {
                tft.writeColor(*data++, 1);
            }
        }
        tft.endWrite();
    }

    void textBounds(String s, uint16_t& w, uint16_t& h) {
        int16_t x1, y1;
        tft.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
    }
};

#endif  // __DODOBOT_DISPLAY_H__
