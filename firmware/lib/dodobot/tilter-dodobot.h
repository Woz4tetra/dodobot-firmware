
#ifndef __DODOBOT_TILTER_H__
#define __DODOBOT_TILTER_H__

#include <Arduino.h>
#include <Servo.h>

#include "dodobot.h"


namespace dodobot_tilter
{
    // Gripper servo
    Servo tilter_servo;
    const int TILTER_PIN = 16;

    const int TILTER_DOWN = 10;
    const int TILTER_UP = 170;
    int tilter_pos = 0;

    bool tilter_attached = false;

    void report_tilter_pos();

    void set_tilter(int pos) {
        if (pos > TILTER_UP) {
            pos = TILTER_UP;
        }
        if (pos < TILTER_DOWN) {
            pos = TILTER_DOWN;
        }
        tilter_pos = pos;
        tilter_servo.write(tilter_pos);
        report_tilter_pos();
    }

    void tilter_up() {
        set_tilter(TILTER_UP);
    }

    void tilter_down() {
        set_tilter(TILTER_DOWN);
    }

    void tilter_toggle() {
        if (tilter_pos == TILTER_UP) {
            set_tilter(TILTER_DOWN);
        }
        else {
            set_tilter(TILTER_UP);
        }
    }

    void set_active(bool state) {
        if (state && dodobot::robot_state.motors_active) {
            if (!tilter_attached) {
                tilter_servo.attach(TILTER_PIN);
                tilter_attached = true;
            }
            tilter_up();
        }
    }


    void setup_tilter() {
        set_active(false);
    }

    void report_tilter_pos()
    {
        if (!dodobot::robot_state.is_reporting_enabled) {
            return;
        }
        dodobot_serial::data->write("tilt", "ud", CURRENT_TIME, tilter_pos);
        // dodobot_serial::info->write("tilt", "ud", CURRENT_TIME, tilter_pos);
    }
};


#endif  // __DODOBOT_TILTER_H__
