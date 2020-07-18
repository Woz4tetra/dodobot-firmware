#include <Arduino.h>
#include <Servo.h>

#define COMM_SERIAL Serial

Servo gripper_servo;
#define GRIPPER_PIN 17

#define MAX_POS 180
#define MIN_POS 0
#define OPEN_POS 20
#define CLOSE_POS 170
int gripper_pos = 0;

// FSRs
#define FSR_LEFT_PIN 36
#define FSR_RIGHT_PIN 35

uint32_t fsr_report_timer = 0;
#define FSR_REPORT_INTERVAL 100


int get_left_fsr() {
    return analogRead(FSR_LEFT_PIN);
}

int get_right_fsr() {
    return analogRead(FSR_RIGHT_PIN);
}


bool fsrs_activated(int threshold) {
    return get_left_fsr() > threshold || get_right_fsr() > threshold;
}

void open_gripper()
{
    gripper_pos = OPEN_POS;
    gripper_servo.write(gripper_pos);
}

void close_gripper(int threshold)
{
    while (!fsrs_activated(threshold)) {
        gripper_pos += 1;
        if (gripper_pos > CLOSE_POS) {
            return;
        }
        gripper_servo.write(gripper_pos);
        delay(5);
    }
}

void setup()
{
    COMM_SERIAL.begin(9600);
    gripper_servo.attach(GRIPPER_PIN);
    open_gripper();

    pinMode(FSR_LEFT_PIN, INPUT);
    pinMode(FSR_RIGHT_PIN, INPUT);
}

void loop()
{
    if (COMM_SERIAL.available()) {
        String command = COMM_SERIAL.readStringUntil('\n');

        char c = command.charAt(0);

        if (c == 't') {
            if (gripper_pos == OPEN_POS) {
                int grip_threshold = 40;
                if (command.length() > 2) {
                    grip_threshold = command.substring(1).toInt();
                }
                close_gripper(grip_threshold);
            }
            else {
                open_gripper();
            }
        }
    }

    if (millis() - fsr_report_timer > FSR_REPORT_INTERVAL) {
        COMM_SERIAL.print("left:\t");
        COMM_SERIAL.print(get_left_fsr());
        COMM_SERIAL.print("\tright:\t");
        COMM_SERIAL.println(get_right_fsr());

        fsr_report_timer = millis();
    }
}
