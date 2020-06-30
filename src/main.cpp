#include <Arduino.h>
#include <Servo.h>
#include <Tic_Teensy.h>
#include <Adafruit_INA219_Teensy.h>
#include <TB6612.h>

#define PROG_SERIAL Serial

// TIC Stepper controller
#define TIC_SERIAL Serial4

TicSerial tic(TIC_SERIAL);

// FSRs
#define FSR1 A0
#define FSR2 A1
#define FSR_ACTIVATED 30

int fsr1_val = 0;
int fsr2_val = 0;
bool reporting_enabled = false;

// Servos
#define MAX_POS 180
#define MIN_POS 0

#define OPEN_POS 20
#define CLOSE_POS 170

#define TILTER_DOWN 10
#define TILTER_UP 170

#define GRIPPER_PIN 9
#define TILTER_PIN 10

Servo gripper_servo;  // create servo object to control a servo
Servo tilter_servo;

int gripper_pos = 0;
int tilter_pos = 0;

bool LED_STATE = false;

// INA219
#define I2C_BUS_1 Wire
Adafruit_INA219 ina219;
float shuntvoltage = 0;
float busvoltage = 0;
float current_mA = 0;
float loadvoltage = 0;
float power_mW = 0;

// TB6612
#define MOTORA_DR1 4
#define MOTORA_DR2 3
#define MOTORB_DR1 5
#define MOTORB_DR2 6
#define MOTORA_PWM 2
#define MOTORB_PWM 7

TB6612 motorA(MOTORA_PWM, MOTORA_DR1, MOTORA_DR2);
TB6612 motorB(MOTORB_PWM, MOTORB_DR1, MOTORB_DR2);

void set_led(bool state)
{
    LED_STATE = state;
    digitalWrite(LED_BUILTIN, LED_STATE);
}


void read_fsrs()
{
    fsr1_val = analogRead(FSR1);
    fsr2_val = analogRead(FSR2);
}


void set_servo(int new_pos, int force_threshold = FSR_ACTIVATED)
{
    if (new_pos > MAX_POS) {
        new_pos = MAX_POS;
    }
    if (new_pos < MIN_POS) {
        new_pos = MIN_POS;
    }

    gripper_pos = new_pos;
    gripper_servo.write(gripper_pos);
    // while (gripper_pos != new_pos) {
    //     if (gripper_pos < new_pos) {
    //         gripper_pos += 1;
    //          read_fsrs();
    //          if (fsr1_val > force_threshold || fsr2_val > force_threshold) {
    //              break;
    //          }
    //          delay(5);
    //     }
    //     else if (gripper_pos > new_pos) {
    //         gripper_pos -= 1;
    //     }
    //     gripper_servo.write(gripper_pos);
    // }
}

void wait_for_force(int force_threshold)
{
    do
    {
        read_fsrs();
        set_servo(gripper_pos + 1);
        delay(20);
        if (gripper_pos >= 180) {
            break;
        }
    }
    while (fsr1_val < force_threshold && fsr2_val < force_threshold);
}

void home_gripper()
{
    set_servo(90);
    delay(150);
    wait_for_force(FSR_ACTIVATED);

    set_servo(gripper_pos - 20);
    delay(150);
    wait_for_force(FSR_ACTIVATED);

    PROG_SERIAL.print("homed gripper_pos: ");
    PROG_SERIAL.println(gripper_pos);
}



// Sends a "Reset command timeout" command to the Tic.  We must
// call this at least once per second, or else a command timeout
// error will happen.  The Tic's default command timeout period
// is 1000 ms, but it can be changed or disabled in the Tic
// Control Center.
void resetCommandTimeout()
{
    tic.resetCommandTimeout();
}

// Delays for the specified number of milliseconds while
// resetting the Tic's command timeout so that its movement does
// not get interrupted.
void delayWhileResettingCommandTimeout(uint32_t ms)
{
    uint32_t start = millis();
    do
    {
        resetCommandTimeout();
    } while ((uint32_t)(millis() - start) <= ms);
}


void get_ina()
{
    shuntvoltage = ina219.getShuntVoltage_mV();
    busvoltage = ina219.getBusVoltage_V();
    current_mA = ina219.getCurrent_mA();
    power_mW = ina219.getPower_mW();
    loadvoltage = busvoltage + (shuntvoltage / 1000);
}

void print_ina()
{
    PROG_SERIAL.print("Bus Voltage:   "); PROG_SERIAL.print(busvoltage); PROG_SERIAL.println(" V");
    PROG_SERIAL.print("Shunt Voltage: "); PROG_SERIAL.print(shuntvoltage); PROG_SERIAL.println(" mV");
    PROG_SERIAL.print("Load Voltage:  "); PROG_SERIAL.print(loadvoltage); PROG_SERIAL.println(" V");
    PROG_SERIAL.print("Current:       "); PROG_SERIAL.print(current_mA); PROG_SERIAL.println(" mA");
    PROG_SERIAL.print("Power:         "); PROG_SERIAL.print(power_mW); PROG_SERIAL.println(" mW");
    PROG_SERIAL.println("");
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    set_led(HIGH);

    PROG_SERIAL.begin(9600);
    PROG_SERIAL.println("Setup");

    I2C_BUS_1.begin(I2C_MASTER, 0x00, I2C_PINS_16_17, I2C_PULLUP_EXT, 400000);
    I2C_BUS_1.setDefaultTimeout(200000); // 200ms
    ina219.begin(&I2C_BUS_1);
    PROG_SERIAL.println("INA219 ready");

    TIC_SERIAL.begin(9600);
    // Give the Tic some time to start up.
    delay(20);
    tic.exitSafeStart();

    gripper_servo.attach(GRIPPER_PIN);  // attaches the servo on pin 9 to the servo object
    tilter_servo.attach(TILTER_PIN);

    set_servo(CLOSE_POS);
    tilter_servo.write(TILTER_UP);

    motorA.begin();
    motorB.begin();
}

void loop()
{
    if (PROG_SERIAL.available()) {
        String command = PROG_SERIAL.readStringUntil('\n');

        if (command.length() == 0) {
            return;
        }
        char c = command.charAt(0);
        int cmd_pos = 0;
        if (c == 'c') {
            cmd_pos = CLOSE_POS;
            set_servo(cmd_pos);
            PROG_SERIAL.println(cmd_pos);
        }
        else if (c == 'o') {
            cmd_pos = OPEN_POS;
            set_servo(cmd_pos);
            PROG_SERIAL.println(cmd_pos);
        }
        else if (c == 't') {
            if (gripper_pos == OPEN_POS) {
                cmd_pos = CLOSE_POS;
            }
            else {
                cmd_pos = OPEN_POS;
            }
            set_servo(cmd_pos);
            PROG_SERIAL.println(cmd_pos);
        }
        else if (c == 'h') {
            home_gripper();
            set_servo(cmd_pos);
            PROG_SERIAL.println(cmd_pos);
        }
        else if (c == 'f') {
            reporting_enabled = !reporting_enabled;
        }
        else if (c == 'a') {
            cmd_pos = command.substring(1).toInt();
            set_servo(cmd_pos);
            PROG_SERIAL.println(cmd_pos);
        }
        else if (c == 'b') {
            // int tilter_pos = command.substring(1).toInt();
            // if (tilter_pos > MAX_POS) {
            //     tilter_pos = MAX_POS;
            // }
            // if (tilter_pos < MIN_POS) {
            //     tilter_pos = MIN_POS;
            // }
            // tilter_servo.write(tilter_pos);
            if (tilter_pos == TILTER_UP) {
                tilter_pos = TILTER_DOWN;
            }
            else {
                tilter_pos = TILTER_UP;
            }
            tilter_servo.write(tilter_pos);
        }
        else if (c == 'u') {
            tic.setTargetVelocity(420000000);
            delayWhileResettingCommandTimeout(500);
            tic.setTargetVelocity(0);
        }
        else if (c == 'd') {
            tic.setTargetVelocity(-420000000);
            delayWhileResettingCommandTimeout(500);
            tic.setTargetVelocity(0);
        }
        else if (c == 'm') {
            char motor_c = command.charAt(1);
            int speed = command.substring(2).toInt();
            if (motor_c == 'a') {
                motorA.setSpeed(speed);
                PROG_SERIAL.print("Motor A: ");
                PROG_SERIAL.println(speed);
            }
            else if (motor_c == 'b') {
                motorB.setSpeed(speed);
                PROG_SERIAL.print("Motor B: ");
                PROG_SERIAL.println(speed);
            }
            else {
                motorA.setSpeed(0);
                motorB.setSpeed(0);
            }
        }
    }

    read_fsrs();
    get_ina();

    if (reporting_enabled) {
        PROG_SERIAL.print(fsr1_val);
        PROG_SERIAL.print('\t');
        PROG_SERIAL.println(fsr2_val);

        print_ina();

        delay(100);
    }
}
