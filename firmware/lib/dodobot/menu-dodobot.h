#ifndef __DODOBOT_MENU_H__
#define __DODOBOT_MENU_H__

#include "display-dodobot.h"
#include "chassis-dodobot.h"
#include "linear-dodobot.h"
#include "tilter-dodobot.h"
#include "speed-pid-dodobot.h"
#include "gripper-dodobot.h"

using namespace dodobot_display;

namespace dodobot_menu
{
    uint32_t menu_display_timer = 0;
    const uint32_t MENU_UPDATE_DELAY_MS = 300;

    unsigned int ROW_SIZE = 10;
    unsigned int BORDER_OFFSET_W = 3;
    unsigned int BORDER_OFFSET_H = 1;

    unsigned int TOP_BAR_H = 20;
    int SCREEN_MID_W = 0;
    int SCREEN_MID_H = 0;

    String date_string = "00:00:00AM";
    uint32_t prev_date_str_update = 0;

    void init_menus()
    {
        int16_t  x1, y1;
        uint16_t w, h;
        tft.getTextBounds("A", 0, 0, &x1, &y1, &w, &h);
        ROW_SIZE = h + BORDER_OFFSET_H;
        SCREEN_MID_W = tft.width() / 2;
        SCREEN_MID_H = tft.height() / 2;
    }

    //
    // Top bar
    //

    uint8_t topbar_icon_x = 10;
    uint8_t topbar_icon_y = TOP_BAR_H / 2;
    uint8_t topbar_icon_r = (TOP_BAR_H - 4) / 2;
    void draw_icon()
    {
        if (dodobot::robot_state.is_reporting_enabled) {
            tft.fillCircle(topbar_icon_x, topbar_icon_y, topbar_icon_r, ST77XX_GREEN);
        }
        else {
            tft.fillCircle(topbar_icon_x, topbar_icon_y, topbar_icon_r, ST77XX_RED);
        }
    }

    uint8_t topbar_active_icon_x = 30;
    uint8_t topbar_active_icon_y = topbar_icon_y;
    uint8_t topbar_active_icon_r = topbar_icon_r;
    void draw_active_icon()
    {
        if (dodobot::robot_state.motors_active) {
            tft.fillCircle(topbar_active_icon_x, topbar_active_icon_y, topbar_active_icon_r, ST77XX_GREEN);
        }
        else {
            tft.fillCircle(topbar_active_icon_x, topbar_active_icon_y, topbar_active_icon_r, ST77XX_RED);
        }
    }

    void draw_datestr()
    {
        int16_t  x1, y1;
        uint16_t w, h;
        tft.getTextBounds(date_string, 0, 0, &x1, &y1, &w, &h);
        tft.setCursor(SCREEN_MID_W - w / 2, TOP_BAR_H / 2 - h / 2);
        if (CURRENT_TIME - prev_date_str_update > 1000) {
            tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
            if (CURRENT_TIME - prev_date_str_update > 5000) {
                tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
            }
        }

        tft.print(date_string);
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    }

    String battery_mA_menu_str;
    String battery_V_menu_str;
    void draw_battery()
    {
        battery_mA_menu_str = "  " + String((int)dodobot_power_monitor::ina219_current_mA) + "mA";
        battery_V_menu_str = "  " + String(dodobot_power_monitor::ina219_loadvoltage) + "V";
        int16_t  x1, y1;
        uint16_t w, h;
        tft.getTextBounds(battery_mA_menu_str, 0, 0, &x1, &y1, &w, &h);
        tft.setCursor(tft.width() - w - 2, TOP_BAR_H / 2 - h);
        tft.print(battery_mA_menu_str);

        tft.getTextBounds(battery_V_menu_str, 0, 0, &x1, &y1, &w, &h);
        tft.setCursor(tft.width() - w - 2, TOP_BAR_H / 2);
        tft.print(battery_V_menu_str);
    }

    void draw_topbar()
    {
        draw_battery();
        draw_icon();
        draw_active_icon();
        draw_datestr();
    }

    //
    // Notifications
    //

    void draw_prompt(String title, int x0, int y0, int row_size, uint8_t entry_count, ...)
    {
        int16_t  x1, y1;
        uint16_t w, h;
        tft.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);

        int y_row = y0;
        tft.setCursor(SCREEN_MID_W - w / 2, y_row);
        tft.print(title);
        y_row += row_size * 2;

        va_list args;
        va_start(args, entry_count);
        for (size_t i = 0; i < entry_count; i++) {
            char* entry_text = va_arg(args, char*);
            tft.setCursor(x0, y_row);
            tft.print(entry_text);
            y_row += row_size;
        }
        va_end(args);
    }

    //
    // Main menu
    //

    enum menu_names {
        MAIN_MENU,
        SENSORS_MENU,
        DRIVE_MENU,
        LINEAR_MENU,
        TILTER_MENU,
        GRIPPER_MENU,
        SHUTDOWN_MENU,
        NONE_MENU,
        NOTIFICATION_MENU
    };

    const menu_names MAIN_MENU_ENUM_MAPPING[] PROGMEM = {
        SENSORS_MENU,
        DRIVE_MENU,
        LINEAR_MENU,
        TILTER_MENU,
        GRIPPER_MENU,
        SHUTDOWN_MENU
    };

    const char* const MAIN_MENU_ENTRIES[] PROGMEM = {
        "Sensors",
        "Drive Motors",
        "Linear",
        "Camera Tilter",
        "Gripper",
        "Shutdown/restart"
    };
    const int MAIN_MENU_ENTRIES_LEN = 6;

    menu_names DISPLAYED_MENU = MAIN_MENU;
    menu_names PREV_DISPLAYED_MENU = NONE_MENU;  // for detecting screen change events

    int MAIN_MENU_SELECT_INDEX = 0;
    int PREV_MAIN_MENU_SELECT_INDEX = -1;
    void draw_main_menu()
    {
        if (MAIN_MENU_SELECT_INDEX < 0) {
            MAIN_MENU_SELECT_INDEX = MAIN_MENU_ENTRIES_LEN - 1;
        }
        if (MAIN_MENU_SELECT_INDEX >= MAIN_MENU_ENTRIES_LEN) {
            MAIN_MENU_SELECT_INDEX = 0;
        }

        if (PREV_MAIN_MENU_SELECT_INDEX == MAIN_MENU_SELECT_INDEX) {
            return;
        }

        // draw_sensor_data();

        // tft.fillScreen(ST7735_BLACK);
        if (PREV_MAIN_MENU_SELECT_INDEX >= 0)
        {
            tft.drawRect(
                BORDER_OFFSET_W - 1,
                ROW_SIZE * PREV_MAIN_MENU_SELECT_INDEX + BORDER_OFFSET_H - 1 + TOP_BAR_H,
                tft.width() - BORDER_OFFSET_W - 1,
                ROW_SIZE - BORDER_OFFSET_H + 1, ST7735_BLACK
            );
        }

        for (int i = 0; i < MAIN_MENU_ENTRIES_LEN; i++)
        {
            tft.setCursor(BORDER_OFFSET_W, ROW_SIZE * i + BORDER_OFFSET_H + TOP_BAR_H);
            tft.print(MAIN_MENU_ENTRIES[i]);
        }

        tft.drawRect(
            BORDER_OFFSET_W - 1,
            ROW_SIZE * MAIN_MENU_SELECT_INDEX + BORDER_OFFSET_H - 1 + TOP_BAR_H,
            tft.width() - BORDER_OFFSET_W - 1,
            ROW_SIZE - BORDER_OFFSET_H + 1, ST7735_WHITE
        );

        PREV_MAIN_MENU_SELECT_INDEX = MAIN_MENU_SELECT_INDEX;
    }

    //
    // Sensors menu
    //

    void draw_sensors_menu()
    {
        // tft.setCursor(BORDER_OFFSET_W, TOP_BAR_H + 5); tft.println("X: " + String(dodobot_bno::orientationData.orientation.x) + "   ");
    }

    //
    // Drive Motor menu
    //

    void drive_robot_forward(double speed_tps)
    {
        dodobot_speed_pid::update_setpointA(speed_tps);  // ticks per s
        dodobot_speed_pid::update_setpointB(speed_tps);  // ticks per s
    }

    void rotate_robot(double speed_tps)
    {
        dodobot_speed_pid::update_setpointA(speed_tps);  // ticks per s
        dodobot_speed_pid::update_setpointB(-speed_tps);  // ticks per s
    }

    void draw_drive_menu()
    {
        int y_offset = TOP_BAR_H + 5;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("A: " + String(dodobot_chassis::encA_pos) + "  " + String(dodobot_chassis::enc_speedA) + "       "); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("B: " + String(dodobot_chassis::encB_pos) + "  " + String(dodobot_chassis::enc_speedB) + "       "); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("m: " + String(dodobot_chassis::motorA.getSpeed()) + "  " + String(dodobot_chassis::motorB.getSpeed()) + "       "); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("1: " + String(dodobot_chassis::is_bump1_active()) + "       "); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("2: " + String(dodobot_chassis::is_bump2_active()) + "       "); y_offset += ROW_SIZE;
    }

    //
    // Linear menu
    //

    void draw_linear_menu()
    {
        int y_offset = TOP_BAR_H + 5;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("step: " + String(dodobot_linear::enc_as_step_ticks()) + "       "); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("enc: " + String(dodobot_linear::stepper_pos) + "       "); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("vel: " + String(dodobot_linear::stepper_vel) + "            "); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("error: " + String(dodobot_linear::is_errored()) + "       "); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("moving: " + String(dodobot_linear::is_moving) + "       "); y_offset += ROW_SIZE;

        tft.setCursor(BORDER_OFFSET_W, y_offset);
        switch (dodobot_linear::planning_mode) {
            case TicPlanningMode::Off: tft.println("mode: Off       "); break;
            case TicPlanningMode::TargetPosition: tft.println("mode: Position       "); break;
            case TicPlanningMode::TargetVelocity: tft.println("mode: Velocity       "); break;
        }
        y_offset += ROW_SIZE;

        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("homed: " + String(dodobot_linear::is_homed) + "       "); y_offset += ROW_SIZE;
    }

    //
    // Tilter menu
    //

    void draw_tilter_menu()
    {
        int y_offset = TOP_BAR_H + 5;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("pos: " + String(dodobot_tilter::tilter_pos) + "       "); y_offset += ROW_SIZE;
    }

    //
    // Gripper menu
    //
    void draw_gripper_menu()
    {
        int y_offset = TOP_BAR_H + 5;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("L: " + String(dodobot_gripper::get_left_fsr()) + "       "); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("R: " + String(dodobot_gripper::get_right_fsr()) + "       "); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("pos: " + String(dodobot_gripper::gripper_pos) + "       "); y_offset += ROW_SIZE;
    }

    //
    // Safety menu
    //
    /*struct safety_diagram {
        const int w = 10;
        const int h = 15;
        const int corner_r = 2;
        int origin_x;
        int origin_y;

        int bar_h = 8;
        int error_bar_w = 8;
        uint16_t max_display_val_mm = 500;
        uint16_t front_bar_color;
        uint16_t back_bar_color;
        int bar_max_w;
        double mm_to_pixels;

        int front_bar_w = 0;
        int front_bar_origin_x;
        int front_bar_origin_y;

        int back_bar_w = 0;
        int back_bar_origin_x;
        int back_bar_origin_y;

        int servo_indicator_r = 20;
        int servo_ind_ball_r = 3;
        double front_servo_angle = -1.0;
        int front_servo_origin_x;
        int front_servo_origin_y;
        int front_servo_x;
        int front_servo_y;

        double back_servo_angle = -1.0;
        int back_servo_origin_x;
        int back_servo_origin_y;
        int back_servo_x;
        int back_servo_y;

        int front_lower_threshold_x;
        int back_lower_threshold_x;
        int front_upper_threshold_x;
        int back_upper_threshold_x;
        int threshold_y;
        int threshold_len = 18;

    } sd_vals;

    int update_front_lower_threshold_x() {
        return SCREEN_MID_W + sd_vals.w / 2 + (int)(sd_vals.mm_to_pixels * dodobot_tof::LOX_FRONT_OBSTACLE_LOWER_THRESHOLD_MM);
    }

    int update_back_lower_threshold_x() {
        return SCREEN_MID_W - sd_vals.w / 2 - (int)(sd_vals.mm_to_pixels * dodobot_tof::LOX_BACK_OBSTACLE_LOWER_THRESHOLD_MM);
    }

    int update_front_upper_threshold_x() {
        return SCREEN_MID_W + sd_vals.w / 2 + (int)(sd_vals.mm_to_pixels * dodobot_tof::LOX_FRONT_OBSTACLE_UPPER_THRESHOLD_MM);
    }

    int update_back_upper_threshold_x() {
        return SCREEN_MID_W - sd_vals.w / 2 - (int)(sd_vals.mm_to_pixels * dodobot_tof::LOX_BACK_OBSTACLE_UPPER_THRESHOLD_MM);
    }


    void init_robot_safety_diagram()
    {
        int origin_offset_h = 25;
        sd_vals.origin_x = SCREEN_MID_W;
        sd_vals.origin_y = SCREEN_MID_H + origin_offset_h;
        sd_vals.front_bar_origin_x = SCREEN_MID_W + sd_vals.w / 2;
        sd_vals.front_bar_origin_y = SCREEN_MID_H - sd_vals.bar_h / 2 + origin_offset_h;
        sd_vals.back_bar_origin_x = SCREEN_MID_W - sd_vals.w / 2 - 1;
        sd_vals.back_bar_origin_y = sd_vals.front_bar_origin_y;

        sd_vals.front_servo_origin_x = SCREEN_MID_W + 20;
        sd_vals.front_servo_origin_y = SCREEN_MID_H + 40;
        sd_vals.back_servo_origin_x = SCREEN_MID_W - 20;
        sd_vals.back_servo_origin_y = sd_vals.front_servo_origin_y;

        sd_vals.front_servo_angle = -1.0;
        sd_vals.back_servo_angle = -1.0;

        sd_vals.threshold_y = SCREEN_MID_H - sd_vals.threshold_len / 2 + origin_offset_h;
        sd_vals.front_lower_threshold_x = update_front_lower_threshold_x();
        sd_vals.back_lower_threshold_x = update_back_lower_threshold_x();
        sd_vals.front_upper_threshold_x = update_front_upper_threshold_x();
        sd_vals.back_upper_threshold_x = update_back_upper_threshold_x();

        sd_vals.bar_max_w = tft.width() - sd_vals.front_bar_origin_x;
        sd_vals.mm_to_pixels = (double)sd_vals.bar_max_w / sd_vals.max_display_val_mm;

        tft.fillRoundRect(
            sd_vals.origin_x - sd_vals.w / 2, sd_vals.origin_y - sd_vals.h / 2,
            sd_vals.w, sd_vals.h, sd_vals.corner_r, ST7735_WHITE
        );
    }

    void draw_tof_sensor_bars()
    {
        int new_threshold = update_front_lower_threshold_x();
        if (sd_vals.front_lower_threshold_x != new_threshold) {
            tft.drawFastVLine(sd_vals.front_lower_threshold_x, sd_vals.threshold_y, sd_vals.threshold_len, ST7735_BLACK);
            sd_vals.front_lower_threshold_x = new_threshold;
        }
        new_threshold = update_back_lower_threshold_x();
        if (sd_vals.back_lower_threshold_x != new_threshold) {
            tft.drawFastVLine(sd_vals.back_lower_threshold_x, sd_vals.threshold_y, sd_vals.threshold_len, ST7735_BLACK);
            sd_vals.back_lower_threshold_x = new_threshold;
        }

        new_threshold = update_front_upper_threshold_x();
        if (sd_vals.front_upper_threshold_x != new_threshold) {
            tft.drawFastVLine(sd_vals.front_upper_threshold_x, sd_vals.threshold_y, sd_vals.threshold_len, ST7735_BLACK);
            sd_vals.front_upper_threshold_x = new_threshold;
        }
        new_threshold = update_back_upper_threshold_x();
        if (sd_vals.back_upper_threshold_x != new_threshold) {
            tft.drawFastVLine(sd_vals.back_upper_threshold_x, sd_vals.threshold_y, sd_vals.threshold_len, ST7735_BLACK);
            sd_vals.back_upper_threshold_x = new_threshold;
        }

        tft.fillRect(sd_vals.front_bar_origin_x, sd_vals.front_bar_origin_y,
            sd_vals.front_bar_w, sd_vals.bar_h, ST7735_BLACK);
        tft.fillRect(sd_vals.back_bar_origin_x, sd_vals.back_bar_origin_y,
            sd_vals.back_bar_w, sd_vals.bar_h, ST7735_BLACK);

        if (dodobot_tof::is_front_ok_VL53L0X()) {
            sd_vals.front_bar_w = (int)(sd_vals.mm_to_pixels * dodobot_tof::measure1.RangeMilliMeter);
            if (dodobot::is_obstacle_in_front()) {
                sd_vals.front_bar_color = ST7735_YELLOW;
            }
            else {
                if (dodobot_tof::measure1.RangeMilliMeter < sd_vals.max_display_val_mm) {
                    sd_vals.front_bar_color = ST7735_GREEN;
                }
                else {
                    sd_vals.front_bar_color = ST7735_BLUE;
                }
            }

        }
        else {
            sd_vals.front_bar_color = ST7735_RED;
            sd_vals.front_bar_w = sd_vals.error_bar_w;
        }

        if (dodobot_tof::is_back_ok_VL53L0X()) {
            sd_vals.back_bar_w = -(int)(sd_vals.mm_to_pixels * dodobot_tof::measure2.RangeMilliMeter);
            if (dodobot::is_obstacle_in_back()) {
                sd_vals.back_bar_color = ST7735_YELLOW;
            }
            else {
                if (dodobot_tof::measure2.RangeMilliMeter < sd_vals.max_display_val_mm) {
                    sd_vals.back_bar_color = ST7735_GREEN;
                }
                else {
                    sd_vals.back_bar_color = ST7735_BLUE;
                }
            }
        }
        else {
            sd_vals.back_bar_color = ST7735_RED;
            sd_vals.back_bar_w = -sd_vals.error_bar_w;
        }

        tft.fillRect(sd_vals.front_bar_origin_x, sd_vals.front_bar_origin_y,
            sd_vals.front_bar_w, sd_vals.bar_h, sd_vals.front_bar_color);
        tft.fillRect(sd_vals.back_bar_origin_x, sd_vals.back_bar_origin_y,
            sd_vals.back_bar_w, sd_vals.bar_h, sd_vals.back_bar_color);

        tft.drawFastVLine(sd_vals.front_lower_threshold_x, sd_vals.threshold_y, sd_vals.threshold_len, ST7735_WHITE);
        tft.drawFastVLine(sd_vals.back_lower_threshold_x, sd_vals.threshold_y, sd_vals.threshold_len, ST7735_WHITE);
        tft.drawFastVLine(sd_vals.front_upper_threshold_x, sd_vals.threshold_y, sd_vals.threshold_len, ST7735_WHITE);
        tft.drawFastVLine(sd_vals.back_upper_threshold_x, sd_vals.threshold_y, sd_vals.threshold_len, ST7735_WHITE);
    }*/

    /*void draw_safety_servo_diagrams()
    {
        double new_front_servo_angle = dodobot_servos::tilter_servo_cmd_to_angle(
            dodobot_servos::servo_positions[FRONT_TILTER_SERVO_NUM]
        ) * PI / 180.0;
        double new_back_servo_angle = dodobot_servos::tilter_servo_cmd_to_angle(
            dodobot_servos::servo_positions[BACK_TILTER_SERVO_NUM]
        ) * PI / 180.0;

        // erase previous and redraw lines if the value changed
        if (new_front_servo_angle != sd_vals.front_servo_angle)
        {
            if (sd_vals.front_servo_angle != -1.0)
            {
                tft.drawLine(
                    sd_vals.front_servo_origin_x, sd_vals.front_servo_origin_y,
                    sd_vals.front_servo_x, sd_vals.front_servo_y, ST7735_BLACK
                );
                tft.drawCircle(
                    sd_vals.front_servo_x, sd_vals.front_servo_y,
                    sd_vals.servo_ind_ball_r, ST7735_BLACK
                );
            }

            sd_vals.front_servo_angle = new_front_servo_angle;
            sd_vals.front_servo_x = -cos(sd_vals.front_servo_angle) * sd_vals.servo_indicator_r + sd_vals.front_servo_origin_x;
            sd_vals.front_servo_y = sin(sd_vals.front_servo_angle) * sd_vals.servo_indicator_r + sd_vals.front_servo_origin_y;

            tft.drawFastHLine(sd_vals.front_servo_origin_x, sd_vals.front_servo_origin_y, sd_vals.servo_indicator_r, ST7735_WHITE);
            tft.drawFastVLine(sd_vals.front_servo_origin_x, sd_vals.front_servo_origin_y, sd_vals.servo_indicator_r, ST7735_WHITE);

            tft.drawLine(
                sd_vals.front_servo_origin_x, sd_vals.front_servo_origin_y,
                sd_vals.front_servo_x, sd_vals.front_servo_y, ST7735_WHITE
            );
            tft.drawCircle(
                sd_vals.front_servo_x, sd_vals.front_servo_y,
                sd_vals.servo_ind_ball_r, ST7735_WHITE
            );
        }
        if (new_back_servo_angle != sd_vals.back_servo_angle)
        {
            if (sd_vals.back_servo_angle != -1.0)
            {
                tft.drawLine(
                    sd_vals.back_servo_origin_x, sd_vals.back_servo_origin_y,
                    sd_vals.back_servo_x, sd_vals.back_servo_y, ST7735_BLACK
                );
                tft.drawCircle(
                    sd_vals.back_servo_x, sd_vals.back_servo_y,
                    sd_vals.servo_ind_ball_r, ST7735_BLACK
                );
            }

            sd_vals.back_servo_angle = new_back_servo_angle;
            sd_vals.back_servo_x = cos(sd_vals.back_servo_angle) * sd_vals.servo_indicator_r + sd_vals.back_servo_origin_x;
            sd_vals.back_servo_y = sin(sd_vals.back_servo_angle) * sd_vals.servo_indicator_r + sd_vals.back_servo_origin_y;

            tft.drawFastHLine(sd_vals.back_servo_origin_x, sd_vals.back_servo_origin_y, -sd_vals.servo_indicator_r, ST7735_WHITE);
            tft.drawFastVLine(sd_vals.back_servo_origin_x, sd_vals.back_servo_origin_y, sd_vals.servo_indicator_r, ST7735_WHITE);

            tft.drawLine(
                sd_vals.back_servo_origin_x, sd_vals.back_servo_origin_y,
                sd_vals.back_servo_x, sd_vals.back_servo_y, ST7735_WHITE
            );
            tft.drawCircle(
                sd_vals.back_servo_x, sd_vals.back_servo_y,
                sd_vals.servo_ind_ball_r, ST7735_WHITE
            );
        }
    }*/

    /*void draw_safety_menu()
    {
        int y_offset = TOP_BAR_H + 5;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println(
            "tof f: " +
            String(dodobot_tof::measure1.RangeMilliMeter) + ", " +
            String(dodobot_tof::measure1.RangeStatus) + ", " +
            String(dodobot_tof::LOX_FRONT_OBSTACLE_LOWER_THRESHOLD_MM) + ", " +
            String(dodobot_tof::LOX_FRONT_OBSTACLE_UPPER_THRESHOLD_MM) +
            "   ");  y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println(
            "tof b: " +
            String(dodobot_tof::measure2.RangeMilliMeter) + ", " +
            String(dodobot_tof::measure2.RangeStatus) + ", " +
            String(dodobot_tof::LOX_BACK_OBSTACLE_LOWER_THRESHOLD_MM) + ", " +
            String(dodobot_tof::LOX_BACK_OBSTACLE_UPPER_THRESHOLD_MM) +
            "   "); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("fsr l: " + String(dodobot_fsr::fsr_1_val) + "   "); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("fsr r: " + String(dodobot_fsr::fsr_2_val) + "   "); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("servo f: " + String(dodobot_servos::servo_positions[FRONT_TILTER_SERVO_NUM]) + "   "); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("servo b: " + String(dodobot_servos::servo_positions[BACK_TILTER_SERVO_NUM]) + "   "); // y_offset += ROW_SIZE;

        draw_tof_sensor_bars();
        // draw_safety_servo_diagrams();
    }*/

    //
    // Shutdown menu
    //

    int SHUTDOWN_MENU_SELECT_INDEX = 0;
    int PREV_SHUTDOWN_MENU_SELECT_INDEX = -1;
    const int NUM_SHUTDOWN_MENU_ENTRIES = 2;
    void draw_shutdown_menu()
    {
        if (SHUTDOWN_MENU_SELECT_INDEX < 0) {
            SHUTDOWN_MENU_SELECT_INDEX = 0;
        }
        if (SHUTDOWN_MENU_SELECT_INDEX >= NUM_SHUTDOWN_MENU_ENTRIES) {
            SHUTDOWN_MENU_SELECT_INDEX = NUM_SHUTDOWN_MENU_ENTRIES - 1;
        }
        if (SHUTDOWN_MENU_SELECT_INDEX != PREV_SHUTDOWN_MENU_SELECT_INDEX)
        {
            int border_y0 = TOP_BAR_H + 10 + ROW_SIZE * 2 - 1;
            tft.drawRect(
                BORDER_OFFSET_W - 1,
                ROW_SIZE * PREV_SHUTDOWN_MENU_SELECT_INDEX + border_y0,
                tft.width() - BORDER_OFFSET_W - 1,
                ROW_SIZE - BORDER_OFFSET_H + 1, ST7735_BLACK
            );
            draw_prompt("Shutdown?", BORDER_OFFSET_W, TOP_BAR_H + 10, ROW_SIZE, NUM_SHUTDOWN_MENU_ENTRIES, "Yes", "No");
            tft.drawRect(
                BORDER_OFFSET_W - 1,
                ROW_SIZE * SHUTDOWN_MENU_SELECT_INDEX + border_y0,
                tft.width() - BORDER_OFFSET_W - 1,
                ROW_SIZE - BORDER_OFFSET_H + 1, ST7735_WHITE
            );
            PREV_SHUTDOWN_MENU_SELECT_INDEX = SHUTDOWN_MENU_SELECT_INDEX;
        }
    }

    void shutdown_menu_enter_event()
    {
        if (SHUTDOWN_MENU_SELECT_INDEX == 0) {
            dodobot_serial::info->write("shutdown", "s", "dodobot");
        }
        DISPLAYED_MENU = MAIN_MENU;
    }

    //
    // Menu events
    //

    void down_menu_event() {
        DODOBOT_SERIAL_WRITE_BOTH("menu", "s", "v");
        switch (DISPLAYED_MENU) {
            case MAIN_MENU: MAIN_MENU_SELECT_INDEX += 1; break;
            case LINEAR_MENU: dodobot_linear::set_position(dodobot_linear::target_position - 5000); break;
            case DRIVE_MENU: drive_robot_forward(-3000.0); break;
            case SHUTDOWN_MENU: SHUTDOWN_MENU_SELECT_INDEX += 1; break;
            default: break;
        }
    }

    void up_menu_event() {
        DODOBOT_SERIAL_WRITE_BOTH("menu", "s", "^");
        switch (DISPLAYED_MENU) {
            case MAIN_MENU: MAIN_MENU_SELECT_INDEX -= 1; break;
            case LINEAR_MENU: dodobot_linear::set_position(dodobot_linear::target_position + 5000); break;
            case DRIVE_MENU: drive_robot_forward(3000.0); break;
            case SHUTDOWN_MENU: SHUTDOWN_MENU_SELECT_INDEX -= 1; break;
            default: break;
        }
    }

    void left_menu_event() {
        DODOBOT_SERIAL_WRITE_BOTH("menu", "s", "<");
        switch (DISPLAYED_MENU) {
            case DRIVE_MENU: rotate_robot(-2000.0); break;
            default: break;
            // add new menu entry callbacks (if needed)
        }
    }

    void right_menu_event() {
        DODOBOT_SERIAL_WRITE_BOTH("menu", "s", ">");
        switch (DISPLAYED_MENU) {
            case DRIVE_MENU: rotate_robot(2000.0); break;
            default: break;
            // add new menu entry callbacks (if needed)
        }
    }

    void enter_menu_event() {
        DODOBOT_SERIAL_WRITE_BOTH("menu", "s", "e");
        switch (DISPLAYED_MENU) {
            case MAIN_MENU: DISPLAYED_MENU = MAIN_MENU_ENUM_MAPPING[MAIN_MENU_SELECT_INDEX]; break;
            case DRIVE_MENU: drive_robot_forward(0.0); break;
            case LINEAR_MENU: dodobot_linear::home_stepper(); break;
            case TILTER_MENU: dodobot_tilter::tilter_toggle(); break;
            case GRIPPER_MENU: dodobot_gripper::toggle_gripper(100); break;
            case SHUTDOWN_MENU: shutdown_menu_enter_event(); break;
            default: break;
            // add new menu entry callbacks (if needed)
        }
    }
    void back_menu_event() {
        DODOBOT_SERIAL_WRITE_BOTH("menu", "s", "b");
        if (DISPLAYED_MENU != MAIN_MENU) {
            DISPLAYED_MENU = MAIN_MENU;
        }
    }

    void screen_change_event() {
        switch (DISPLAYED_MENU) {
            case MAIN_MENU: PREV_MAIN_MENU_SELECT_INDEX = -1; break;  // force a redraw of the menu list when switching
            case SHUTDOWN_MENU: PREV_SHUTDOWN_MENU_SELECT_INDEX = -1;
            default: break;
            // add new menu entry callbacks (if needed)
        }
    }

    void draw_menus()
    {
        if (CURRENT_TIME - menu_display_timer < MENU_UPDATE_DELAY_MS) {
            return;
        }
        menu_display_timer = CURRENT_TIME;

        if (PREV_DISPLAYED_MENU != DISPLAYED_MENU) {
            tft.fillScreen(ST7735_BLACK);
            screen_change_event();
        }
        switch (DISPLAYED_MENU) {
            case MAIN_MENU: draw_main_menu(); break;
            case SENSORS_MENU: draw_sensors_menu(); break;
            case DRIVE_MENU: draw_drive_menu(); break;
            case LINEAR_MENU: draw_linear_menu(); break;
            case TILTER_MENU: draw_tilter_menu(); break;
            case GRIPPER_MENU: draw_gripper_menu(); break;
            case SHUTDOWN_MENU: draw_shutdown_menu(); break;
            default: break;
            // add new menu entry callbacks
        }
        PREV_DISPLAYED_MENU = DISPLAYED_MENU;

        draw_topbar();
    }
};

#endif  // __DODOBOT_MENU_H__
