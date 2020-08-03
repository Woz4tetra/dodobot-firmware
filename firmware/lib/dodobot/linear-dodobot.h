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
    const int baudrate = 115385;
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
    const int POSITION_BUFFER = 500;
    // const uint32_t MAX_SPEED = 420000000;
    const int MAX_SPEED = 200000000;
    int target_position = 0;
    int target_velocity = 0;

    uint32_t update_timer = 0;
    const uint32_t UPDATE_DELAY_MS = 10;

    enum TicPlanningMode planning_mode = TicPlanningMode::Off;  // 0 = off, 1 = position, 2 = velocity
    uint32_t planning_mode_timer = 0;
    const uint32_t PLANNING_MODE_DELAY_MS = 100;

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
        if (is_active == state) {
            return;
        }
        is_active = state;
        if (state && dodobot::robot_state.motors_active) {
            TIC_SERIAL.begin(baudrate);
            // Give the Tic some time to start up.
            delay(20);
            tic.exitSafeStart();
            tic.setMaxSpeed(MAX_SPEED);
            // stepper_enc.write(0);
            is_homed = true;
        }
        else {
            TIC_SERIAL.end();
            is_homed = false;
        }
    }

    void setup_linear()
    {
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

    void print_stepper_error(uint16_t errors)
    {
        if (errors & (1 << (uint8_t)TicError::IntentionallyDeenergized))
            dodobot_serial::println_error("Tic Error: IntentionallyDeenergized, %d", errors);
        if (errors & (1 << (uint8_t)TicError::MotorDriverError))
            dodobot_serial::println_error("Tic Error: MotorDriverError, %d", errors);
        if (errors & (1 << (uint8_t)TicError::LowVin))
            dodobot_serial::println_error("Tic Error: LowVin, %d", errors);
        if (errors & (1 << (uint8_t)TicError::KillSwitch))
            dodobot_serial::println_error("Tic Error: KillSwitch, %d", errors);
        if (errors & (1 << (uint8_t)TicError::RequiredInputInvalid))
            dodobot_serial::println_error("Tic Error: RequiredInputInvalid, %d", errors);
        if (errors & (1 << (uint8_t)TicError::SerialError))
            dodobot_serial::println_error("Tic Error: SerialError, %d", errors);
        if (errors & (1 << (uint8_t)TicError::CommandTimeout))
            dodobot_serial::println_error("Tic Error: CommandTimeout, %d", errors);
        if (errors & (1 << (uint8_t)TicError::SafeStartViolation))
            dodobot_serial::println_error("Tic Error: SafeStartViolation, %d", errors);
        if (errors & (1 << (uint8_t)TicError::ErrLineHigh))
            dodobot_serial::println_error("Tic Error: ErrLineHigh, %d", errors);
        if (errors & (1 << (uint8_t)TicError::SerialFraming))
            dodobot_serial::println_error("Tic Error: SerialFraming, %d", errors);
        if (errors & (1 << (uint8_t)TicError::RxOverrun))
            dodobot_serial::println_error("Tic Error: RxOverrun, %d", errors);
        if (errors & (1 << (uint8_t)TicError::Format))
            dodobot_serial::println_error("Tic Error: Format, %d", errors);
        if (errors & (1 << (uint8_t)TicError::Crc))
            dodobot_serial::println_error("Tic Error: Crc, %d", errors);
        if (errors & (1 << (uint8_t)TicError::EncoderSkip))
            dodobot_serial::println_error("Tic Error: EncoderSkip, %d", errors);
    }

    void home_stepper()
    {
        if (!is_active) {
            dodobot_serial::println_error("Can't home stepper. Active flag not set.");
            return;
        }
        if (is_errored()) {
            uint16_t errors = tic.getErrorStatus();

            if (errors & (1 << (uint8_t)TicError::SafeStartViolation)) {
                tic.exitSafeStart();
                tic.setMaxSpeed(MAX_SPEED);
            }
            else {
                dodobot_serial::println_error("Can't home stepper. Stepper is errored: %d", errors);
                print_stepper_error(errors);
                return;
            }
        }
        dodobot_serial::println_info("Running home sequence.");
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
        delay(200);

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
        dodobot_serial::println_info("Homing complete.");
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
        target_position = position;
        tic.setTargetPosition(target_position);
    }

    void set_velocity(int velocity) {
        if (!is_homed || !is_active) {
            return;
        }
        target_velocity = velocity;
        tic.setTargetVelocity(velocity);
    }

    void stop() {
        target_velocity = 0;
        tic.haltAndHold();
    }

    bool is_position_invalid(int position) {
        return (
            (target_position >= MAX_POSITION && position >= MAX_POSITION) ||
            (target_position < 0 && position < 0)
        );
    }

    bool is_velocity_invalid(int position) {
        return (
            (target_velocity > 0 && position >= (MAX_POSITION - POSITION_BUFFER)) ||
            (target_velocity < 0 && position < POSITION_BUFFER)
        );
    }

    enum TicPlanningMode get_planning_mode() {
        if (CURRENT_TIME - planning_mode_timer > PLANNING_MODE_DELAY_MS) {
            planning_mode_timer = CURRENT_TIME;
            planning_mode = tic.getPlanningMode();

        }
        return planning_mode;
    }

    void update() {
        if (!is_homed || !is_active) {
            return;
        }

        if (CURRENT_TIME - update_timer < UPDATE_DELAY_MS) {
            return;
        }
        update_timer = CURRENT_TIME;

        int current_pos = tic.getCurrentPosition();
        switch (get_planning_mode()) {
            case TicPlanningMode::Off: break;
            case TicPlanningMode::TargetPosition: if (is_position_invalid(current_pos))  { stop(); } break;
            case TicPlanningMode::TargetVelocity: if (is_velocity_invalid(current_pos))  { stop(); } break;
        }


        tic.resetCommandTimeout();

        if (!dodobot::robot_state.is_reporting_enabled) {
            return;
        }
        dodobot_serial::data->write("linear", "udddd", CURRENT_TIME, current_pos, is_errored(), is_homed, is_active);
    }
}

#endif  // __DODOBOT_LINEAR_H__
