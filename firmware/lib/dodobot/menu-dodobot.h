#ifndef __DODOBOT_MENU_H__
#define __DODOBOT_MENU_H__

#include "display-dodobot.h"
#include "chassis-dodobot.h"
#include "linear-dodobot.h"
#include "tilter-dodobot.h"
#include "speed-pid-dodobot.h"
#include "gripper-dodobot.h"
#include "breakout-dodobot.h"

using namespace dodobot_display;

namespace dodobot_menu
{
    uint32_t menu_display_timer = 0;
    const uint32_t MENU_UPDATE_DELAY_MS_DEFAULT = 300;
    uint32_t MENU_UPDATE_DELAY_MS = MENU_UPDATE_DELAY_MS_DEFAULT;

    unsigned int ROW_SIZE = 10;
    unsigned int BORDER_OFFSET_W = 3;
    unsigned int BORDER_OFFSET_H = 1;

    unsigned int TOP_BAR_H = 20;
    int SCREEN_MID_W = 0;
    int SCREEN_MID_H = 0;

    String date_string = "00:00:00AM";
    uint32_t prev_date_str_update = 0;

    String network_ip = "";
    String network_netmask = "";
    String network_broadcast = "";
    String network_name = "";
    String network_error = "";
    uint32_t network_str_update = 0;

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
        NETWORK_MENU,
        DRIVE_MENU,
        LINEAR_MENU,
        TILTER_MENU,
        GRIPPER_MENU,
        SHUTDOWN_MENU,
        NONE_MENU,
        BREAKOUT_MENU
    };

    const menu_names MAIN_MENU_ENUM_MAPPING[] PROGMEM = {
        NETWORK_MENU,
        DRIVE_MENU,
        LINEAR_MENU,
        TILTER_MENU,
        GRIPPER_MENU,
        BREAKOUT_MENU,
        SHUTDOWN_MENU
    };

    const char* const MAIN_MENU_ENTRIES[] PROGMEM = {
        "Networking",
        "Drive Motors",
        "Linear",
        "Camera Tilter",
        "Gripper",
        "Breakout",
        "Shutdown/restart"
    };
    const int MAIN_MENU_ENTRIES_LEN = 7;

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
    // Network menu
    //

    void draw_network_menu()
    {
        if (CURRENT_TIME - network_str_update > 300000) {
            tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
            if (CURRENT_TIME - network_str_update > 400000) {
                tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
            }
        }
        if (network_ip.length() == 0) {
            tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
        }

        int y_offset = TOP_BAR_H + 5;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("interface: " + network_name); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("IP: " + network_ip); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("NM: " + network_netmask); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("BC: " + network_broadcast); y_offset += ROW_SIZE;
        if (network_error.length() == 0 || network_error.equals(" ")) {
            tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("                        "); y_offset += ROW_SIZE;
        }
        else {
            tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("error: " + network_error); y_offset += ROW_SIZE;
        }

        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
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

    int drive_column_offset = 80;
    void draw_drive_menu()
    {
        int y_offset = TOP_BAR_H + 5;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("A: " + String(dodobot_chassis::encA_pos) + "       ");
        tft.setCursor(BORDER_OFFSET_W + drive_column_offset, y_offset); tft.println("B: " + String(dodobot_chassis::encB_pos) + "       "); y_offset += ROW_SIZE * 2;

        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("c A: " + String(dodobot_chassis::motorA.getSpeed()) + "       ");
        tft.setCursor(BORDER_OFFSET_W + drive_column_offset, y_offset); tft.println("B: " + String(dodobot_chassis::motorB.getSpeed()) + "       "); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("t A: " + String((int)dodobot_speed_pid::motorA_pid.get_target()) + "       ");
        tft.setCursor(BORDER_OFFSET_W + drive_column_offset, y_offset); tft.println("B: " + String((int)dodobot_speed_pid::motorB_pid.get_target()) + "       "); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("m A: " + String((int)dodobot_chassis::enc_speedA) + "       ");
        tft.setCursor(BORDER_OFFSET_W + drive_column_offset, y_offset); tft.println("B: " + String((int)dodobot_chassis::enc_speedB) + "       "); y_offset += ROW_SIZE;

        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("1: " + String(dodobot_chassis::is_bump1_active()) + "       ");
        tft.setCursor(BORDER_OFFSET_W + drive_column_offset, y_offset); tft.println("2: " + String(dodobot_chassis::is_bump2_active()) + "       "); y_offset += ROW_SIZE;
    }

    //
    // Linear menu
    //

    void draw_linear_menu()
    {
        int y_offset = TOP_BAR_H + 5;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("step: " + String(dodobot_linear::stepper_pos) + "       "); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("enc: " + String(dodobot_linear::raw_encoder_pos) + "       "); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("enc as step: " + String(dodobot_linear::encoder_pos) + "       "); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("diff: " + String(dodobot_linear::stepper_pos - dodobot_linear::encoder_pos) + ", " + String(dodobot_linear::ENCODER_POSITION_ERROR) + "       "); y_offset += ROW_SIZE;

        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("linear pos mm: " + String(dodobot_linear::to_linear_pos(dodobot_linear::stepper_pos)) + "       "); y_offset += ROW_SIZE;
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
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("microstep: " + String(dodobot_linear::microsteps) + "       "); y_offset += ROW_SIZE;
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
            case LINEAR_MENU: dodobot_linear::set_position(dodobot_linear::target_position - 625 * dodobot_linear::microsteps); break;
            case DRIVE_MENU: drive_robot_forward(-3000.0); break;
            case GRIPPER_MENU: dodobot_gripper::open_gripper(dodobot_gripper::gripper_pos - 10); break;
            case SHUTDOWN_MENU: SHUTDOWN_MENU_SELECT_INDEX += 1; break;
            default: break;
        }
    }

    void up_menu_event() {
        DODOBOT_SERIAL_WRITE_BOTH("menu", "s", "^");
        switch (DISPLAYED_MENU) {
            case MAIN_MENU: MAIN_MENU_SELECT_INDEX -= 1; break;
            case LINEAR_MENU: dodobot_linear::set_position(dodobot_linear::target_position + 625 * dodobot_linear::microsteps); break;
            case DRIVE_MENU: drive_robot_forward(3000.0); break;
            case GRIPPER_MENU: dodobot_gripper::close_gripper(100, dodobot_gripper::gripper_pos + 10); break;
            case SHUTDOWN_MENU: SHUTDOWN_MENU_SELECT_INDEX -= 1; break;
            default: break;
        }
    }

    void left_menu_event() {
        DODOBOT_SERIAL_WRITE_BOTH("menu", "s", "<");
        switch (DISPLAYED_MENU) {
            case DRIVE_MENU: rotate_robot(-2000.0); break;
            case GRIPPER_MENU: dodobot_gripper::open_gripper(dodobot_gripper::gripper_pos - 1); break;
            case BREAKOUT_MENU: dodobot_breakout::left_event(); break;
            default: break;
            // add new menu entry callbacks (if needed)
        }
    }

    void right_menu_event() {
        DODOBOT_SERIAL_WRITE_BOTH("menu", "s", ">");
        switch (DISPLAYED_MENU) {
            case DRIVE_MENU: rotate_robot(2000.0); break;
            case GRIPPER_MENU: dodobot_gripper::close_gripper(100, dodobot_gripper::gripper_pos + 1); break;
            case BREAKOUT_MENU: dodobot_breakout::right_event(); break;
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
            case BREAKOUT_MENU: dodobot_breakout::enter_event(); break;
            case SHUTDOWN_MENU: shutdown_menu_enter_event(); break;
            default: break;
            // add new menu entry callbacks (if needed)
        }
    }
    void back_menu_event() {
        DODOBOT_SERIAL_WRITE_BOTH("menu", "s", "b");
        if (DISPLAYED_MENU != MAIN_MENU) {
            switch (DISPLAYED_MENU) {
                case BREAKOUT_MENU:  dodobot_breakout::on_unload();  break;
                default: break;

            }
            DISPLAYED_MENU = MAIN_MENU;
        }
    }

    void repeat_key_event() {
        DODOBOT_SERIAL_WRITE_BOTH("menu", "s", "%");
        switch (DISPLAYED_MENU) {
            case BREAKOUT_MENU: dodobot_breakout::repeat_key_event(); break;
            default: break;
        }
    }

    void screen_change_event() {
        MENU_UPDATE_DELAY_MS = MENU_UPDATE_DELAY_MS_DEFAULT;
        switch (DISPLAYED_MENU) {
            case MAIN_MENU: PREV_MAIN_MENU_SELECT_INDEX = -1; break;  // force a redraw of the menu list when switching
            case SHUTDOWN_MENU: PREV_SHUTDOWN_MENU_SELECT_INDEX = -1;
            case BREAKOUT_MENU:
                MENU_UPDATE_DELAY_MS = dodobot_breakout::UPDATE_DELAY_MS;
                dodobot_breakout::on_load();
                break;
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
            case NETWORK_MENU: draw_network_menu(); break;
            case DRIVE_MENU: draw_drive_menu(); break;
            case LINEAR_MENU: draw_linear_menu(); break;
            case TILTER_MENU: draw_tilter_menu(); break;
            case GRIPPER_MENU: draw_gripper_menu(); break;
            case BREAKOUT_MENU: dodobot_breakout::draw(); break;
            case SHUTDOWN_MENU: draw_shutdown_menu(); break;
            default: break;
            // add new menu entry callbacks
        }
        PREV_DISPLAYED_MENU = DISPLAYED_MENU;

        switch (DISPLAYED_MENU) {
            case MAIN_MENU:
            case NETWORK_MENU:
            case DRIVE_MENU:
            case LINEAR_MENU:
            case TILTER_MENU:
            case GRIPPER_MENU:
            case SHUTDOWN_MENU:  draw_topbar();

            case BREAKOUT_MENU:
            default: break;
        }
    }
};

#endif  // __DODOBOT_MENU_H__
