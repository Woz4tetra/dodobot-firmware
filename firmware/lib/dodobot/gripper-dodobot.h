
#ifndef __DODOBOT_GRIPPER_H__
#define __DODOBOT_GRIPPER_H__

#include <Arduino.h>
#include <Servo.h>

#include "dodobot.h"


namespace dodobot_gripper
{
    // Gripper servo
    Servo gripper_servo;
    const int GRIPPER_PIN = 17;

    const int MAX_POS = 180;
    const int MIN_POS = 0;
    const int OPEN_POS = 20;
    const int CLOSE_POS = 170;
    const int DEFAULT_GRIP_THRESHOLD = 40;

    uint32_t grip_report_timer = 0;
    const uint32_t GRIP_UPDATE_DELAY_MS = 5;

    int gripper_pos = 0;
    bool gripper_attached = false;

    int grip_threshold = 0;
    bool grip_reached = false;

    // FSRs
    const int FSR_LEFT_PIN = 36;
    const int FSR_RIGHT_PIN = 35;

    uint32_t fsr_report_timer = 0;
    const uint32_t FSR_SAMPLERATE_DELAY_MS = 33;

    int get_left_fsr() {
        return analogRead(FSR_LEFT_PIN);
    }

    int get_right_fsr() {
        return analogRead(FSR_RIGHT_PIN);
    }

    bool fsrs_activated(int threshold) {
        return get_left_fsr() > threshold || get_right_fsr() > threshold;
    }

    // Gripper actions

    void report_gripper_pos();

    void open_gripper()
    {
        gripper_pos = OPEN_POS;
        gripper_servo.write(gripper_pos);
        grip_reached = true;
        report_gripper_pos();
    }

    void close_gripper(int threshold = -1) {
        if (threshold < 0) {
            threshold = DEFAULT_GRIP_THRESHOLD;
        }
        grip_threshold = threshold;
        grip_reached = false;
    }

    void toggle_gripper(int threshold = -1)
    {
        if (gripper_pos == OPEN_POS) {
            close_gripper(threshold);
        }
        else {
            open_gripper();
        }
    }

    void update()
    {
        if (!gripper_attached) {
            return;
        }
        if (!dodobot::robot_state.motors_active) {
            return;
        }

        if (grip_reached) {
            return;
        }
        if (CURRENT_TIME - grip_report_timer < GRIP_UPDATE_DELAY_MS) {
            return;
        }
        grip_report_timer = CURRENT_TIME;

        if (fsrs_activated(grip_threshold)) {
            grip_reached = true;
            report_gripper_pos();
            return;
        }
        if (gripper_pos >= CLOSE_POS) {
            grip_reached = true;
            report_gripper_pos();
            return;
        }
        gripper_pos += 1;
        gripper_servo.write(gripper_pos);
    }

    void set_active(bool state) {
        if (state && dodobot::robot_state.motors_active) {
            if (!gripper_attached) {
                gripper_servo.attach(GRIPPER_PIN);
                gripper_attached = true;
            }
            open_gripper();
        }
    }

    void setup_gripper()
    {
        set_active(false);

        pinMode(FSR_LEFT_PIN, INPUT);
        pinMode(FSR_RIGHT_PIN, INPUT);
    }

    bool read_fsrs()
    {
        if (CURRENT_TIME - fsr_report_timer < FSR_SAMPLERATE_DELAY_MS) {
            return false;
        }
        fsr_report_timer = CURRENT_TIME;
        return true;
    }

    void report_fsrs()
    {
        if (!dodobot::robot_state.is_reporting_enabled) {
            return;
        }
        dodobot_serial::data->write("fsr", "udd", CURRENT_TIME, get_left_fsr(), get_right_fsr());
        // dodobot_serial::info->write("fsr", "udd", CURRENT_TIME, get_left_fsr(), get_right_fsr());
    }

    void report_gripper_pos()
    {
        if (!dodobot::robot_state.is_reporting_enabled) {
            return;
        }
        dodobot_serial::data->write("grip", "ud", CURRENT_TIME, gripper_pos);
        // dodobot_serial::info->write("grip", "ud", CURRENT_TIME, gripper_pos);
    }
};


#endif  // __DODOBOT_GRIPPER_H__
