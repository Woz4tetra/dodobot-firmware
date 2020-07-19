#include <Arduino.h>
#include <Tic_Teensy.h>
#include <Encoder.h>

#define COMM_SERIAL Serial

uint32_t prev_report_time = 0;
const uint32_t report_interval_ms = 250;
#define CURRENT_TIME millis()

// TIC Stepper controller
#define TIC_SERIAL Serial4
TicSerial tic(TIC_SERIAL);

// Homing switch
#define HOMING_PIN 39

#define MOTOR_STBY 26


// Encoder
// #define STEPPER_ENCA 24
// #define STEPPER_ENCB 25
// Encoder stepper_enc(STEPPER_ENCA, STEPPER_ENCB);

bool is_homed = false;
const int max_position = 85000;
// const uint32_t max_speed = 420000000;
const int max_speed = 200000000;

bool are_motors_active = false;


void set_motors_active(bool active)
{
    if (are_motors_active == active) {
        return;
    }
    are_motors_active = active;
    if (active) {  // bring motors out of standby mode
        digitalWrite(MOTOR_STBY, HIGH);
    }
    else {  // set motors to low power
        digitalWrite(MOTOR_STBY, LOW);
    }
}

bool is_home_pin_active() {
    return digitalRead(HOMING_PIN) == LOW;
}

void setup_stepper()
{
    TIC_SERIAL.begin(9600);
    // Give the Tic some time to start up.
    delay(20);
    tic.exitSafeStart();
    tic.setMaxSpeed(max_speed);

    // stepper_enc.write(0);

    pinMode(HOMING_PIN, INPUT);
    // pinMode(MOTOR_STBY, OUTPUT);
    // set_motors_active(true);
}

void waitForPosition(int32_t targetPosition)
{
    tic.setTargetPosition(targetPosition);
    do
    {
        tic.resetCommandTimeout();
    } while (tic.getCurrentPosition() != targetPosition);
}

void delayWhileResettingCommandTimeout(uint32_t ms)
{
    uint32_t start = millis();
    do
    {
        tic.resetCommandTimeout();
    } while ((uint32_t)(millis() - start) <= ms);
}

void home_stepper()
{
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

    tic.setMaxSpeed(max_speed);
    is_homed = true;
}

void setup()
{
    COMM_SERIAL.begin(9600);
    setup_stepper();
}

void loop()
{
    if (COMM_SERIAL.available()) {
        String command = COMM_SERIAL.readStringUntil('\n');
        char c = command.charAt(0);

        if (c == 'h') {
            home_stepper();
        }
        else if (c == 'p') {
            if (is_homed) {
                int goal_position = command.substring(1).toInt();
                if (goal_position > max_position) {
                    goal_position = max_position;
                }
                if (goal_position < 0) {
                    goal_position = 0;
                }
                COMM_SERIAL.print("Goal:\t");
                COMM_SERIAL.print(goal_position);
                waitForPosition(goal_position);
            }
            else {
                COMM_SERIAL.println("Stepper isn't homed!");
            }
        }
        else if (c == 'd') {
            tic.setTargetVelocity(-max_speed);
            delayWhileResettingCommandTimeout(500);
            tic.haltAndHold();
        }
        else if (c == 'u') {
            tic.setTargetVelocity(max_speed);
            delayWhileResettingCommandTimeout(500);
            tic.haltAndHold();
        }
        else {
            tic.haltAndHold();
        }
    }
    if (CURRENT_TIME - prev_report_time > report_interval_ms) {
        COMM_SERIAL.print("homing pin:\t");
        COMM_SERIAL.println(is_home_pin_active());
        COMM_SERIAL.print("position:\t");
        COMM_SERIAL.println(tic.getCurrentPosition());

        prev_report_time = CURRENT_TIME;
    }
}
