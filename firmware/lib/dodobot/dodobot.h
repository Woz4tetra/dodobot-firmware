
#ifndef __DODOBOT_H__
#define __DODOBOT_H__

#include <Arduino.h>
#include "serial-dodobot.h"

#define CURRENT_TIME millis()

namespace dodobot
{
    const int MOTOR_STANDBY_PIN = 26;

    uint32_t state_report_timer = 0;
    const uint32_t STATE_SAMPLERATE_DELAY_MS = 100;

    uint32_t prev_loop_timer = 0;
    uint32_t loop_report_sum = 0;
    uint32_t loop_report_counter = 0;

    void soft_restart()
    {
        DATA_SERIAL.end();  // clears the serial monitor  if used
        SCB_AIRCR = 0x05FA0004;  // write value for restart
    }

    struct state {
        bool is_active;
        bool is_reporting_enabled;
        bool battery_ok;
        bool motors_active;
        bool is_speed_pid_enabled;
    } robot_state;

    void init_structs() {
        robot_state.is_active = false;
        robot_state.is_reporting_enabled = false;
        robot_state.battery_ok = true;
        robot_state.motors_active = false;
        robot_state.is_speed_pid_enabled = false;
    }

    void set_motors_active(bool state)
    {
        if (robot_state.motors_active == state) {
            return;
        }
        robot_state.motors_active = state;
        if (state) {  // bring motors out of standby mode
            digitalWrite(MOTOR_STANDBY_PIN, HIGH);
        }
        else {  // set motors to low power
            digitalWrite(MOTOR_STANDBY_PIN, LOW);
        }
        delay(100);  // wait for modules to power off/on
    }

    void setup() {
        init_structs();
        pinMode(MOTOR_STANDBY_PIN, OUTPUT);
        set_motors_active(false);
        // set_motors_active(true);
        // robot_state.is_reporting_enabled = true;
    }

    void report_structs()
    {
        if (!robot_state.is_reporting_enabled) {
            return;
        }
        uint32_t current_time = micros();
        loop_report_sum += current_time - prev_loop_timer;
        loop_report_counter++;
        prev_loop_timer = current_time;

        if (CURRENT_TIME - state_report_timer < STATE_SAMPLERATE_DELAY_MS) {
            return;
        }
        state_report_timer = CURRENT_TIME;
        double loop_rate = (double)loop_report_counter / (double)loop_report_sum * 1E6;

        dodobot_serial::data->write("state", "udddf", CURRENT_TIME,
            robot_state.is_active,
            robot_state.battery_ok,
            robot_state.motors_active,
            loop_rate
        );

        loop_report_sum = 0;
        loop_report_counter = 0;

    }
}

#endif  // __DODOBOT_H__
