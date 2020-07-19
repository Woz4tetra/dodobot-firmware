#ifndef __DODOBOT_CHASSIS_H__
#define __DODOBOT_CHASSIS_H__

#include <Arduino.h>
#include <TB6612.h>
#include <Encoder.h>

#include "dodobot.h"

namespace dodobot_chassis
{
    bool is_active = false;

    // DC motors
    const int MOTORA_DR1 = 27;
    const int MOTORA_DR2 = 28;
    const int MOTORB_DR1 = 38;
    const int MOTORB_DR2 = 37;
    const int MOTORA_PWM = 29;
    const int MOTORB_PWM = 30;

    const int MOTOR_COMMAND_TIMEOUT_MS = 1000;

    uint32_t prev_commandA_time = 0;
    uint32_t prev_commandB_time = 0;

    TB6612 motorA(MOTORA_PWM, MOTORA_DR1, MOTORA_DR2);
    TB6612 motorB(MOTORB_PWM, MOTORB_DR2, MOTORB_DR1);

    // Encoders
    const int MOTORA_ENCA = 23;
    const int MOTORA_ENCB = 22;
    const int MOTORB_ENCA = 21;
    const int MOTORB_ENCB = 20;

    const int ENCODER_SAMPLERATE_DELAY_MS = 33;

    Encoder motorA_enc(MOTORA_ENCB, MOTORA_ENCA);
    Encoder motorB_enc(MOTORB_ENCA, MOTORB_ENCB);

    long encA_pos, encB_pos = 0;
    double enc_speedA, enc_speedB = 0.0;  // ticks/s, smoothed
    double enc_speedA_raw, enc_speedB_raw = 0.0;  // ticks/s

    uint32_t prev_enc_time = 0;

    double speed_smooth_kA = 1.0;
    double speed_smooth_kB = 1.0;

    // Bumpers
    // const int REAR_BUMPER_1 = 0;
    // const int REAR_BUMPER_2 = 1;

    // DC motor functions

    void setup_motors()
    {
        motorA.begin();
        motorB.begin();
    }


    void reset_motor_timeouts()
    {
        prev_commandA_time = CURRENT_TIME;
        prev_commandB_time = CURRENT_TIME;
    }

    void set_motorA(int speed) {
        if (!is_active) {
            return;
        }
        reset_motor_timeouts();
        motorA.setSpeed(speed);
    }

    void set_motorB(int speed) {
        if (!is_active) {
            return;
        }
        reset_motor_timeouts();
        motorB.setSpeed(speed);
    }


    bool check_motor_timeout()
    {
        if (!is_active) {
            return;
        }
        bool timedout = false;
        if (CURRENT_TIME - prev_commandA_time > MOTOR_COMMAND_TIMEOUT_MS) {
            motorA.setSpeed(0);
            timedout = true;
        }
        if (CURRENT_TIME - prev_commandB_time > MOTOR_COMMAND_TIMEOUT_MS) {
            motorB.setSpeed(0);
            timedout = true;
        }

        return timedout;
    }


    void stop_motors() {
        if (!is_active) {
            return;
        }
        motorA.setSpeed(0);
        motorB.setSpeed(0);
    }

    // Encoder functions
    void reset_encoders()
    {
        encA_pos = 0;
        encB_pos = 0;
        motorA_enc.write(0);
        motorB_enc.write(0);
    }

    bool read_encoders()
    {
        if (CURRENT_TIME - prev_enc_time < ENCODER_SAMPLERATE_DELAY_MS) {
            return false;
        }

        long new_encA_pos = motorA_enc.read();
        long new_encB_pos = motorB_enc.read();

        // bool should_report = false;
        // if (new_encA_pos != encA_pos || new_encB_pos != encB_pos) {
        //     should_report = true;
        // }

        enc_speedA_raw = (double)(new_encA_pos - encA_pos) / (CURRENT_TIME - prev_enc_time) * 1000.0;
        enc_speedB_raw = (double)(new_encB_pos - encB_pos) / (CURRENT_TIME - prev_enc_time) * 1000.0;
        enc_speedA += speed_smooth_kA * (enc_speedA_raw - enc_speedA);
        enc_speedB += speed_smooth_kB * (enc_speedB_raw - enc_speedB);

        encA_pos = new_encA_pos;
        encB_pos = new_encB_pos;

        prev_enc_time = CURRENT_TIME;

        // return should_report;
        return true;
    }

    void report_encoders()
    {
        if (!dodobot::robot_state.is_reporting_enabled) {
            return;
        }
        dodobot_serial::data->write("enc", "uddff", CURRENT_TIME, encA_pos, encB_pos, enc_speedA, enc_speedB);
    }

    void setup_chassis()
    {
        reset_encoders();
        setup_motors();
        // pinMode(REAR_BUMPER_1, INPUT);
        // pinMode(REAR_BUMPER_2, INPUT);
    }

    void update()
    {
        if (read_encoders()) {
            report_encoders();
        }
        check_motor_timeout();
    }

    void set_active(bool state)
    {
        if (state == is_active) {
            return;
        }
        is_active = state;

        stop_motors();
        reset_encoders();
    }
}

#endif  // __DODOBOT_CHASSIS_H__
