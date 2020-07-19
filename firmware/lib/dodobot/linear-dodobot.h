#ifndef __DODOBOT_LINEAR_H__
#define __DODOBOT_LINEAR_H__

#include <Arduino.h>
#include <Tic_Teensy.h>
#include <Encoder.h>

#include "dodobot.h"

#define TIC_SERIAL Serial4

#define RESET_PIN A22

namespace dodobot_linear
{
    // TIC Stepper controller
    TicSerial tic(TIC_SERIAL);

    const int ERROR_PIN = 15;

    // Homing switch
    const int HOMING_PIN = 39;

    // Encoder
    // const int STEPPER_ENCA = 24;
    // const int STEPPER_ENCB = 25;
    // Encoder stepper_enc(STEPPER_ENCA, STEPPER_ENCB);

    bool is_homed = false;
    bool is_active = false;
    const int MAX_POSITION = 85000;
    // const uint32_t MAX_SPEED = 420000000;
    const int MAX_SPEED = 200000000;

    uint32_t update_timer = 0;
    const uint32_t UPDATE_DELAY_MS = 33;


    bool is_home_pin_active() {
        return digitalRead(HOMING_PIN) == LOW;
    }

    bool is_errored() {
        return digitalRead(ERROR_PIN) == HIGH;
    }

    void reset()
    {
        digitalWrite(RESET_PIN, LOW);
        delay(20);
        digitalWrite(RESET_PIN, HIGH);
    }

    void set_active(bool state) {
        if (state && dodobot::robot_state.motors_active) {
            tic.exitSafeStart();
            tic.setMaxSpeed(MAX_SPEED);
            // stepper_enc.write(0);
            is_active = true;
            is_homed = true;
        }
        else {
            is_active = false;
            is_homed = false;
        }
    }

    void setup_linear()
    {
        TIC_SERIAL.begin(9600);
        // Give the Tic some time to start up.
        delay(20);
        pinMode(HOMING_PIN, INPUT);
        pinMode(ERROR_PIN, INPUT);
        pinMode(RESET_PIN, OUTPUT);

        digitalWrite(RESET_PIN, HIGH);

        set_active(false);
    }


    void waitForPosition(int32_t targetPosition)
    {
        tic.setTargetPosition(targetPosition);
        do
        {
            tic.resetCommandTimeout();
        } while (tic.getCurrentPosition() != targetPosition);
    }

    void home_stepper()
    {
        // dodobot_serial::println_info("is_active: %d, is_errored: %d", is_active, is_errored());
        if (!is_active) {
            return;
        }
        if (is_errored()) {
            return;
        }
        // Drive down until the limit switch is found
        tic.setTargetVelocity(-200000000);
        while (!is_home_pin_active()) {
            tic.resetCommandTimeout();
        }
        tic.haltAndHold();
        delay(50);

        // Move off the homing switch
        tic.setTargetVelocity(200000000);
        while (is_home_pin_active()) {
            tic.resetCommandTimeout();
        }

        // Move down more slowly to get a more accurate reading
        tic.setTargetVelocity(-50000000);
        while (!is_home_pin_active()) {
            tic.resetCommandTimeout();
        }

        tic.haltAndSetPosition(0);
        delay(50);
        // stepper_enc.write(0);
        delay(50);
        waitForPosition(10000);

        tic.setMaxSpeed(MAX_SPEED);
        is_homed = true;
    }

    void set_position(int position) {
        if (!is_homed || !is_active) {
            return;
        }
        if (position > MAX_POSITION) {
            position = MAX_POSITION;
        }
        if (position < 0) {
            position = 0;
        }
        tic.setTargetPosition(position);
    }

    void set_velocity(int velocity) {
        if (!is_homed || !is_active) {
            return;
        }
        tic.setTargetVelocity(velocity);
    }

    void stop() {
        tic.haltAndHold();
    }

    void report_linear()
    {
        if (!dodobot::robot_state.is_reporting_enabled) {
            return;
        }
        // dodobot_serial::info->data("linear", "udddd", CURRENT_TIME, tic.getCurrentPosition(), is_errored(), is_homed, is_active);
        dodobot_serial::info->write("linear", "udddd", CURRENT_TIME, tic.getCurrentPosition(), is_errored(), is_homed, is_active);
    }

    void update() {
        if (CURRENT_TIME - update_timer < UPDATE_DELAY_MS) {
            return;
        }

        if (tic.getCurrentPosition() >= MAX_POSITION) {
            stop();
        }
        if (tic.getCurrentPosition() <= 0) {
            stop();
        }

        tic.resetCommandTimeout();

        report_linear();
    }
}

#endif  // __DODOBOT_LINEAR_H__
