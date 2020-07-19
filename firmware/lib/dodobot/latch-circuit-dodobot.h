#ifndef __DODOBOT_LATCH_CIRCUIT_H__
#define __DODOBOT_LATCH_CIRCUIT_H__

#include <Arduino.h>

#include "dodobot.h"

namespace dodobot_latch_circuit
{
    const int UNLATCH_PIN = 24;
    const int LATCH_STAT_PIN = 25;
    const int BUTTON_LED_PIN = 14;
    const int USB_SENSE_PIN = 7;


    bool usb_connected_once = false;
    const uint32_t USB_CHECK_INTERVAL_MS = 1000;
    uint32_t usb_check_timer = 0;

    const uint32_t LED_CYCLE_INTERVAL_MS = 1;
    uint32_t led_cycle_timer = 0;

    int led_val = 0;
    bool prev_button_state = false;
    bool is_shutting_down = false;


    void unlatch() {
        digitalWrite(UNLATCH_PIN, HIGH);
    }


    bool is_usb_connected() {
        return digitalRead(USB_SENSE_PIN) == HIGH;
    }

    bool is_button_pushed() {
        return digitalRead(LATCH_STAT_PIN) == LOW;
    }

    void set_button_led(int val) {
        led_val = val;
        if (led_val < 0) {
            led_val = 0;
        }
        else if (led_val > 255) {
            led_val = 255;
        }
        analogWrite(BUTTON_LED_PIN, val);
    }

    void setup_latch()
    {
        pinMode(UNLATCH_PIN, OUTPUT);
        pinMode(LATCH_STAT_PIN, INPUT_PULLUP);
        pinMode(USB_SENSE_PIN, INPUT_PULLUP);
        pinMode(BUTTON_LED_PIN, OUTPUT);

        digitalWrite(UNLATCH_PIN, LOW);
        set_button_led(255);

        prev_button_state = is_button_pushed();
    }

    bool is_incrementing = false;
    void cycle_led()
    {
        if (CURRENT_TIME - led_cycle_timer < LED_CYCLE_INTERVAL_MS) {
            return;
        }
        led_cycle_timer = CURRENT_TIME;

        if (is_incrementing) {
            set_button_led(led_val + 1);
        }
        else {
            set_button_led(led_val - 1);
        }
        if (led_val == 0) {
            is_incrementing = true;
        }
        else if (led_val == 255) {
            is_incrementing = false;
        }
    }

    void shutdown() {
        is_shutting_down = true;
        dodobot::set_motors_active(false);
    }

    void update()
    {
        if (CURRENT_TIME - usb_check_timer > USB_CHECK_INTERVAL_MS) {
            if (usb_connected_once) {
                if (!is_usb_connected()) {
                    unlatch();
                }
            }
            else {
                if (is_usb_connected()) {
                    usb_connected_once = true;
                }
            }
        }

        bool button_state = is_button_pushed();
        if (button_state) {
            if (!is_shutting_down) {
                cycle_led();
            }
        }
        else {
            set_button_led(0);
        }
        if (prev_button_state != button_state) {
            dodobot_serial::info->write("latch", "ud", CURRENT_TIME, button_state);

            prev_button_state = button_state;
        }
    }
};

#endif  // __DODOBOT_LATCH_CIRCUIT_H__
