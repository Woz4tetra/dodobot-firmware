#ifndef __DODOBOT_BREAKOUT_H__
#define __DODOBOT_BREAKOUT_H__

#include <Arduino.h>
#include <math.h>

#include "dodobot.h"
#include "display-dodobot.h"


namespace dodobot_breakout
{
    const int8_t X_MIN = 0;
    const int8_t X_MAX = ST7735_TFTHEIGHT_160;
    const int8_t Y_MIN = 0;
    const int8_t Y_MAX = ST7735_TFTWIDTH_128;

    class Ball {
    private:
        int8_t x, y;
        uint_t radius;
        float vx, vy;

        void bounce(char side)
        {
            // double angle = atan2(vy, vx);
            // double speed = sqrt(vy * vy + vx * vx);
            switch (side) {
                case 'n': vy = -vy; break;
                case 'e': vx = -vx; break;
                case 's': vy = -vy; break;
                case 'w': vx = -vx; break;
            }
        }
    public:
        Ball(uint8_t r): x(0), y(0), radius(r) {
            vx = 2.0;
            vy = 3.0;
        }

        void update()
        {
            int8_t new_x = x + (int8_t)vx;
            int8_t new_y = y + (int8_t)vy;
            if (new_x > X_MAX) {
                bounce('e');
                new_x = x + (int8_t)vx;
            }
            if (new_x < X_MIN) {
                bounce('w');
                new_x = x + (int8_t)vx;
            }

            if (new_y > Y_MAX) {
                bounce('n');
                new_y = y + (int8_t)vy;
            }
            if (new_x < Y_MIN) {
                bounce('s');
                new_y = y + (int8_t)vy;
            }

            x = new_x;
            y = new_y;
        }

        void draw() {
            tft.fillCircle(x, y, radius, ST77XX_GREEN);
        }
    };

    void draw();
    void on_load();
    void right_event();
    void left_event();

    Ball ball;

    void draw()
    {
        ball.update();
        ball.draw();
    }
}

#endif  // __DODOBOT_BREAKOUT_H__
