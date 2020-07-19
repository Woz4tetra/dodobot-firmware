#include <Arduino.h>
#include "dodobot.h"
#include "ir-remote-dodobot.h"
#include "i2c-dodobot.h"
#include "power-monitor-dodobot.h"
#include "display-dodobot.h"
#include "gripper-dodobot.h"
#include "tilter-dodobot.h"
#include "linear-dodobot.h"


void set_active(bool state)
{
    dodobot::set_motors_active(state);
    dodobot_gripper::set_active(state);
    dodobot_tilter::set_active(state);
    dodobot_linear::set_active(state);
}

void dodobot_serial::packet_callback(DodobotSerial* serial_obj, String category, String packet)
{
    // dodobot_serial::println_info("category: %s, packet: %s", category.c_str(), packet.c_str());

    // get_ready
    if (category.equals("?")) {
        CHECK_SEGMENT(serial_obj);
        if (serial_obj->get_segment().equals("dodobot")) {
            dodobot_serial::println_info("Received ready signal!");
            DODOBOT_SERIAL_WRITE_BOTH("ready", "us", CURRENT_TIME, "jetson-nano");
        }
        else {
            dodobot_serial::println_error("Invalid ready segment supplied: %s", serial_obj->get_segment().c_str());
        }
    }

    // toggle reporting
    else if (category.equals("[]")) {
        CHECK_SEGMENT(serial_obj);
        int reporting_state = serial_obj->get_segment().toInt();
        // dodobot_serial::println_info("toggle_reporting %d", reporting_state);
        switch (reporting_state)
        {
            case 0: dodobot::robot_state.is_reporting_enabled = false; break;
            case 1: dodobot::robot_state.is_reporting_enabled = true; break;
            // case 2: reset(); break;
            default:
                dodobot_serial::println_error("Invalid reporting flag received: %d", reporting_state);
                break;
        }
    }

    // toggle motors active
    else if (category.equals("<>")) {
        CHECK_SEGMENT(serial_obj);
        int active_state = serial_obj->get_segment().toInt();
        // dodobot_serial::println_info("toggle_active %d", active_state);
        switch (active_state)
        {
            case 0: set_active(false); break;
            case 1: set_active(true); break;
            // case 2: dodobot::soft_restart(); break;
            default:
                break;
        }
    }

    // set gripper
    else if (category.equals("grip")) {
        CHECK_SEGMENT(serial_obj);
        int gripper_state = serial_obj->get_segment().toInt();
        int grip_threshold = -1;
        switch (gripper_state)
        {
            case 0: dodobot_gripper::open_gripper(); break;
            case 1:
                CHECK_SEGMENT(serial_obj);  grip_threshold = serial_obj->get_segment().toInt();
                dodobot_gripper::close_gripper(grip_threshold);
                break;
            case 2:
                CHECK_SEGMENT(serial_obj);  grip_threshold = serial_obj->get_segment().toInt();
                dodobot_gripper::toggle_gripper(grip_threshold);
                break;
            default:
                break;
        }
    }

    // set tilter
    else if (category.equals("tilt")) {
        CHECK_SEGMENT(serial_obj);
        int tilt_state = serial_obj->get_segment().toInt();
        int tilt_pos = 0;
        switch (tilt_state)
        {
            case 0: dodobot_tilter::tilter_up(); break;
            case 1: dodobot_tilter::tilter_down(); break;
            case 2: dodobot_tilter::tilter_toggle(); break;
            case 3:
                CHECK_SEGMENT(serial_obj);  tilt_pos = serial_obj->get_segment().toInt();
                dodobot_tilter::set_tilter(tilt_pos);
                break;
            default:
                break;
        }
    }

    // set linear
    else if (category.equals("linear")) {
        CHECK_SEGMENT(serial_obj);
        int linear_state = serial_obj->get_segment().toInt();
        int linear_value = 0;
        switch (linear_state)
        {
            case 0:
                CHECK_SEGMENT(serial_obj);  linear_value = serial_obj->get_segment().toInt();
                dodobot_linear::set_position(linear_value);
                break;
            case 1:
                CHECK_SEGMENT(serial_obj);  linear_value = serial_obj->get_segment().toInt();
                dodobot_linear::set_velocity(linear_value);
                break;
            case 2: dodobot_linear::stop(); break;
            case 3: dodobot_linear::reset(); break;
            case 4: dodobot_linear::home_stepper(); break;
            default:
                break;
        }
    }
}

void dodobot_ir_remote::callback_ir(uint8_t remote_type, uint16_t value)
{
    switch (value) {
        case 0x00ff: dodobot_serial::println_info("IR: VOL-"); break;  // VOL-
        case 0x807f: dodobot_serial::println_info("IR: Play/Pause"); break;  // Play/Pause
        case 0x40bf: dodobot_serial::println_info("IR: VOL+"); break;  // VOL+
        case 0x20df: dodobot_serial::println_info("IR: SETUP"); break;  // SETUP
        case 0xa05f: dodobot_serial::println_info("IR: ^"); break;  // ^
        case 0x609f: dodobot_serial::println_info("IR: MODE"); break;  // MODE
        case 0x10ef: dodobot_serial::println_info("IR: <"); break;  // <
        case 0x906f: dodobot_serial::println_info("IR: ENTER"); break;  // ENTER
        case 0x50af: dodobot_serial::println_info("IR: >"); break;  // >
        case 0x30cf: dodobot_serial::println_info("IR: 0 10+"); break;  // 0 10+
        case 0xb04f: dodobot_serial::println_info("IR: v"); break;  // v
        case 0x708f: dodobot_serial::println_info("IR: Del"); break;  // Del
        case 0x08f7: dodobot_serial::println_info("IR: 1"); break;  // 1
        case 0x8877: dodobot_serial::println_info("IR: 2"); break;  // 2
        case 0x48B7: dodobot_serial::println_info("IR: 3"); break;  // 3
        case 0x28D7: dodobot_serial::println_info("IR: 4"); break;  // 4
        case 0xA857: dodobot_serial::println_info("IR: 5"); break;  // 5
        case 0x6897: dodobot_serial::println_info("IR: 6"); break;  // 6
        case 0x18E7: dodobot_serial::println_info("IR: 7"); break;  // 7
        case 0x9867: dodobot_serial::println_info("IR: 8"); break;  // 8
        case 0x58A7: dodobot_serial::println_info("IR: 9"); break;  // 9
    }
}

void setup()
{
    dodobot::setup();
    dodobot_serial::setup_serial();
    dodobot_ir_remote::setup_IR();
    dodobot_i2c::setup_i2c();
    dodobot_power_monitor::setup_INA219();
    dodobot_display::setup_display();
    dodobot_gripper::setup_gripper();
    dodobot_tilter::setup_tilter();
    dodobot_linear::setup_linear();
}

void loop()
{
    dodobot_serial::data->read();
    dodobot_serial::info->read();

    dodobot::report_structs();

    if (dodobot_ir_remote::read_IR()) {
        dodobot_ir_remote::report_IR();
    }
    if (dodobot_power_monitor::read_INA219()) {
        dodobot_power_monitor::report_INA219();
    }
    if (dodobot_gripper::read_fsrs()) {
        dodobot_gripper::report_fsrs();
    }
    dodobot_gripper::update();
    dodobot_linear::update();
}
