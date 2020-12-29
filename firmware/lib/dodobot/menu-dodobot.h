#ifndef __DODOBOT_MENU_H__
#define __DODOBOT_MENU_H__

#include <JPEGDecoder.h>  // JPEG decoder library

#include "display-dodobot.h"
#include "chassis-dodobot.h"
#include "linear-dodobot.h"
#include "tilter-dodobot.h"
#include "speed-pid-dodobot.h"
#include "gripper-dodobot.h"
#include "breakout-dodobot.h"
#include "latch-circuit-dodobot.h"
#include "sd-dodobot.h"

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

    uint16_t COLOR_GRAY = tft.color565(128, 128, 128);

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

    uint8_t topbar_icon_x = 5;
    // uint8_t topbar_icon_x = 10;
    // uint8_t topbar_icon_y = TOP_BAR_H / 2;
    uint8_t topbar_icon_y = TOP_BAR_H / 4;
    uint8_t topbar_icon_r = 4;
    void draw_reporting_icon()
    {
        if (dodobot::robot_state.is_reporting_enabled) {
            tft.fillCircle(topbar_icon_x, topbar_icon_y, topbar_icon_r, ST77XX_GREEN);
        }
        else {
            tft.fillCircle(topbar_icon_x, topbar_icon_y, topbar_icon_r, ST77XX_RED);
        }
    }

    uint8_t topbar_active_icon_x = 5;
    // uint8_t topbar_active_icon_x = 30;
    // uint8_t topbar_active_icon_y = topbar_icon_y;
    uint8_t topbar_active_icon_y = 3 * TOP_BAR_H / 4;
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
        x1 = SCREEN_MID_W - w / 2;
        y1 = TOP_BAR_H / 2 - h / 2;
        if (!dodobot_latch_circuit::state.usb_voltage_state) {
            tft.fillRect(x1, y1, w, h, ST77XX_BLACK);
            return;
        }

        tft.setCursor(x1, y1);
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

    uint8_t topbar_latch_usb_icon_x = 15;
    uint8_t topbar_latch_usb_icon_y = TOP_BAR_H / 2;
    uint8_t topbar_latch_usb_icon_r = 4;
    void draw_latch_debug()
    {
        if (dodobot_latch_circuit::state.usb_connected_once) {
            if (dodobot_latch_circuit::state.usb_voltage_state) {
                tft.fillCircle(topbar_latch_usb_icon_x, topbar_latch_usb_icon_y, topbar_latch_usb_icon_r, ST77XX_GREEN);
            }
            else {
                tft.fillCircle(topbar_latch_usb_icon_x, topbar_latch_usb_icon_y, topbar_latch_usb_icon_r, ST77XX_YELLOW);
            }
        }
        else {
            tft.fillCircle(topbar_latch_usb_icon_x, topbar_latch_usb_icon_y, topbar_latch_usb_icon_r, ST77XX_RED);
        }

        if (dodobot_latch_circuit::state.usb_connected_once &&
                !dodobot_latch_circuit::state.usb_voltage_state &&
                dodobot_latch_circuit::state.shutdown_timer != UINT_MAX) {
            int16_t  x1, y1;
            uint16_t w, h;
            float seconds = (float)dodobot_latch_circuit::state.shutdown_timer * 1E-3;
            tft.getTextBounds(seconds, 0, 0, &x1, &y1, &w, &h);
            tft.setCursor(SCREEN_MID_W - w / 2, (TOP_BAR_H - h) / 2);
            tft.print(seconds);
        }
    }

    void draw_topbar()
    {
        draw_battery();

        draw_reporting_icon();
        draw_active_icon();

        draw_datestr();
        draw_latch_debug();
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
        BREAKOUT_MENU,
        IMAGE_MENU
    };

    const menu_names MAIN_MENU_ENUM_MAPPING[] PROGMEM = {
        NETWORK_MENU,
        DRIVE_MENU,
        LINEAR_MENU,
        TILTER_MENU,
        GRIPPER_MENU,
        BREAKOUT_MENU,
        IMAGE_MENU,
        SHUTDOWN_MENU
    };

    const char* const MAIN_MENU_ENTRIES[] PROGMEM = {
        "Networking",
        "Drive Motors",
        "Linear",
        "Camera Tilter",
        "Gripper",
        "Breakout",
        "Image",
        "Shutdown/restart"
    };
    const int MAIN_MENU_ENTRIES_LEN = 8;

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

        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("L: " + String(dodobot_chassis::is_bump2_active()) + "       ");
        tft.setCursor(BORDER_OFFSET_W + drive_column_offset, y_offset); tft.println("R: " + String(dodobot_chassis::is_bump1_active()) + "       "); y_offset += ROW_SIZE;
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
        // tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("microstep: " + String(dodobot_linear::microsteps) + "       "); y_offset += ROW_SIZE;
        tft.setCursor(BORDER_OFFSET_W, y_offset); tft.println("limit: " + String(dodobot_linear::is_home_pin_active()) + "       "); y_offset += ROW_SIZE;
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

    // Image menu

    void jpegInfo()
    {
        dodobot_serial::println_info("JPEG image info");
        dodobot_serial::println_info("  Width      : %d", JpegDec.width);
        dodobot_serial::println_info("  Height     : %d", JpegDec.height);
        dodobot_serial::println_info("  Components : %d", JpegDec.comps);
        dodobot_serial::println_info("  MCU / row  : %d", JpegDec.MCUSPerRow);
        dodobot_serial::println_info("  MCU / col  : %d", JpegDec.MCUSPerCol);
        dodobot_serial::println_info("  Scan type  : %d", JpegDec.scanType);
        dodobot_serial::println_info("  MCU width  : %d", JpegDec.MCUWidth);
        dodobot_serial::println_info("  MCU height : %d", JpegDec.MCUHeight);
    }

    uint32_t image_array_max_len = 0x10000;
    #define minimum(a,b)     (((a) < (b)) ? (a) : (b))
    void renderJPEG(const char* path, int xpos, int ypos)
    {
        File file = SD.open(path);

        uint8_t* image_array = new uint8_t[minimum(file.size(), image_array_max_len)];
        uint32_t image_array_size = 0;
        while (file.available()) {
        	image_array[image_array_size++] = file.read();
        }
        JpegDec.decodeArray(image_array, image_array_size);
        jpegInfo();

        // retrieve infomration about the image
        uint16_t *pImg;
        uint16_t mcu_w = JpegDec.MCUWidth;
        uint16_t mcu_h = JpegDec.MCUHeight;
        uint32_t max_x = JpegDec.width;
        uint32_t max_y = JpegDec.height;

        // Jpeg images are drawn as a set of image block (tiles) called Minimum Coding Units (MCUs)
        // Typically these MCUs are 16x16 pixel blocks
        // Determine the width and height of the right and bottom edge image blocks
        uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
        uint32_t min_h = minimum(mcu_h, max_y % mcu_h);

        // save the current image block size
        uint32_t win_w = mcu_w;
        uint32_t win_h = mcu_h;

        uint32_t screen_w = tft.width();
        uint32_t screen_h = tft.height();

        // record the current time so we can measure how long it takes to draw an image
        uint32_t drawTime = millis();

        // save the coordinate of the right and bottom edges to assist image cropping
        // to the screen size
        max_x += xpos;
        max_y += ypos;

        // read each MCU block until there are no more
        tft.startWrite();
        // tft.setAddrWindow(xpos, ypos, minimum(max_x, screen_w), minimum(max_y, screen_h));
        while ( JpegDec.read()) {

            // save a pointer to the image block
            pImg = JpegDec.pImage;

            // calculate where the image block should be drawn on the screen
            unsigned int mcu_x = JpegDec.MCUx * mcu_w + xpos;
            unsigned int mcu_y = JpegDec.MCUy * mcu_h + ypos;

            // check if the image block size needs to be changed for the right and bottom edges
            if (mcu_x + mcu_w <= max_x) { win_w = mcu_w; }
            else { win_w = min_w; }

            if (mcu_y + mcu_h <= max_y) { win_h = mcu_h; }
            else { win_h = min_h; }

            // calculate how many pixels must be drawn
            uint32_t mcu_pixels = win_w * win_h;

            // draw image block if it will fit on the screen
            if ( ( mcu_x + win_w) <= screen_w && ( mcu_y + win_h) <= screen_h) {
                // open a window onto the screen to paint the pixels into
                // tft.startWrite();
                tft.setAddrWindow(mcu_x, mcu_y, win_w, win_h);
                // push all the image block pixels to the screen
                while (mcu_pixels--) {
                    tft.writeColor(*pImg++, 1); // Send to TFT 16 bits at a time
                }
                // tft.endWrite();
            }

            // stop drawing blocks if the bottom of the screen has been reached
            // the abort function will close the file
            else if ( ( mcu_y + win_h) >= screen_h) {
                JpegDec.abort();
            }

        }
        tft.endWrite();

        // calculate how long it took to draw the image
        drawTime = millis() - drawTime; // Calculate the time it took

        // print the results to the serial port
        dodobot_serial::println_info("Total render time was: %d ms", drawTime);
    }

    const char* splash_image_path = "DBSPLASH.JPG";
    void load_image_event()
    {
        if (DISPLAYED_MENU != IMAGE_MENU) {
            return;
        }
        renderJPEG(splash_image_path, 0, TOP_BAR_H);
    }

    void draw_image_menu()
    {

    }

    void draw_image_on_load()
    {
        renderJPEG(splash_image_path, 0, TOP_BAR_H);
    }

    //
    // Menu events
    //

    void down_menu_event() {
        DODOBOT_SERIAL_WRITE_BOTH("menu", "s", "v");
        switch (DISPLAYED_MENU) {
            case MAIN_MENU: MAIN_MENU_SELECT_INDEX += 1; break;
            case LINEAR_MENU: dodobot_linear::set_position(dodobot_linear::target_position - 625 * dodobot_linear::microsteps); break;
            // case DRIVE_MENU: drive_robot_forward(-3000.0); break;
            case DRIVE_MENU: dodobot_chassis::set_motors(-200, -200); break;
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
            // case DRIVE_MENU: drive_robot_forward(3000.0); break;
            case DRIVE_MENU: dodobot_chassis::set_motors(200, 200); break;
            case GRIPPER_MENU: dodobot_gripper::close_gripper(100, dodobot_gripper::gripper_pos + 10); break;
            case SHUTDOWN_MENU: SHUTDOWN_MENU_SELECT_INDEX -= 1; break;
            default: break;
        }
    }

    void left_menu_event() {
        DODOBOT_SERIAL_WRITE_BOTH("menu", "s", "<");
        switch (DISPLAYED_MENU) {
            // case DRIVE_MENU: rotate_robot(-2000.0); break;
            case DRIVE_MENU: dodobot_chassis::set_motors(-200, 200); break;
            case GRIPPER_MENU: dodobot_gripper::open_gripper(dodobot_gripper::gripper_pos - 1); break;
            case BREAKOUT_MENU: dodobot_breakout::left_event(); break;
            default: break;
            // add new menu entry callbacks (if needed)
        }
    }

    void right_menu_event() {
        DODOBOT_SERIAL_WRITE_BOTH("menu", "s", ">");
        switch (DISPLAYED_MENU) {
            // case DRIVE_MENU: rotate_robot(2000.0); break;
            case DRIVE_MENU: dodobot_chassis::set_motors(200, -200); break;
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
            case IMAGE_MENU:
                draw_image_on_load();
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
            case IMAGE_MENU: draw_image_menu(); break;
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
            case IMAGE_MENU:
            case SHUTDOWN_MENU:  draw_topbar();

            case BREAKOUT_MENU:
            default: break;
        }
    }
};

#endif  // __DODOBOT_MENU_H__
