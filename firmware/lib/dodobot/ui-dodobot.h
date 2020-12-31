
#pragma once

#include "serial-dodobot.h"
#include "sd-dodobot.h"
#include "display-dodobot.h"
#include "latch-circuit-dodobot.h"


using namespace dodobot_display;



namespace dodobot_ui
{
    const uint32_t UI_DELAY_MS_DEFAULT = 150;
    uint32_t UI_DELAY_MS = UI_DELAY_MS_DEFAULT;
    uint32_t ui_timer = 0;

    enum EventType {
        UP_EVENT,
        DOWN_EVENT,
        LEFT_EVENT,
        RIGHT_EVENT,
        BACK_EVENT,
        ENTER_EVENT,
        NONE_EVENT
    };


    void load(String name);

    class ViewController
    {
    public:
        ViewController()  {}
        virtual void draw()  {}
        virtual void on_load()  {}
        virtual void on_unload()  {}
        virtual void on_up()  {}
        virtual void on_down()  {}
        virtual void on_left()  {}
        virtual void on_right()  {}
        virtual void on_back()  {}
        virtual void on_enter()  {}
        virtual void on_repeat(EventType e)  {}
    };

    class ViewWithOverlayController : public ViewController
    {
    public:
        ViewWithOverlayController(ViewController* overlay) : ViewController() {
            this->overlay = overlay;
        }
        void draw() {
            overlay->draw();
            draw_with_overlay();
        }
        virtual void draw_with_overlay() {}

        void on_load() {
            overlay->on_load();
            on_load_with_overlay();
        }
        virtual void on_load_with_overlay() {}

        void on_unload() {
            overlay->on_unload();
            on_unload_with_overlay();
        }
        virtual void on_unload_with_overlay() {}

    protected:
        ViewController* overlay;
    };

    const size_t max_num_views = 100;
    size_t num_views = 0;
    ViewController* views[max_num_views];
    String names[max_num_views];

    ViewController* current_view;

    class SplashScreenController : public ViewController
    {
    public:
        SplashScreenController() {

        }
        void draw()  {}
        void on_load()
        {
            dodobot_serial::println_info("Splash view loaded");
            dodobot_sd::loadGIF("BW_BOT~1.GIF");
            while (dodobot_sd::drawGIFframe())  {}
            dodobot_sd::closeGIF();

            for (int i = 255; i >= 0; i--) {
                dodobot_display::set_display_brightness(i);
                delay(1);
            }

            load("main-menu");

            for (int i = 0; i <= 255; i++) {
                dodobot_display::set_display_brightness(i);
                delay(1);
            }
        }

        void on_unload()  {}
        void on_up()  {}
        void on_down()  {}
        void on_left()  {}
        void on_right()  {}
        void on_back()  {}
        void on_enter()  {}

    };


    enum NotificationLevel {
        INFO,
        WARN,
        ERROR
    };

    struct Notification {
        NotificationLevel level;
        String text;
        uint32_t timeout;
        bool spent;
    };

    Notification make_notif(NotificationLevel level, String text, uint32_t timeout)
    {
        Notification notif;
        notif.level = level;
        notif.text = text;
        notif.timeout = timeout;
        notif.spent = false;
        return notif;
    }

    static const int max_notifs = 5;
    class NotificationQueue
    {
    public:
        NotificationQueue()  {}

        bool empty() {
            return len == 0;
        }

        bool queue_full() {
            return len == max_notifs;
        }

        int size() {
            return len;
        }

        bool queue(Notification notif)
        {
            if (queue_full()) {
                return false;
            }
            if (is_notif_present(notif)) {
                return false;
            }

            if (back == max_notifs - 1) {
                back = -1;
            }
            buffer[++back] = notif;
            len++;

            return true;
        }

        int deque()
        {
            if (empty()) {
                return -1;
            }
            int index = front;
            front++;
            if (front == max_notifs) {
                front = 0;
            }
            len--;
            return index;
        }
        int peek() {
            if (empty()) {
                return -1;
            }
            return front;
        }

        bool is_notif_present(Notification notif)
        {
            for (size_t index = 0; index < max_notifs; index++) {
                if (buffer[index].spent) {
                    continue;
                }
                if (notif.text.equals(buffer[index].text) && notif.level == buffer[index].level) {
                    return true;
                }
            }
            return false;
        }

        Notification get_index(int index) {
            if (index < 0) {
                index = 0;
            }
            if (index >= max_notifs) {
                index = max_notifs - 1;
            }
            return buffer[index];
        }
        void set_spent(int index) {
            buffer[index].spent = true;
        }
    private:
        Notification buffer[max_notifs];
        int front = 0;
        int back = -1;
        int len = 0;
    };

    enum ConnectState {
        NO_USB_POWER,
        USB_POWER_DETECTED,
        USB_STABLE,
        SHUTTING_DOWN,
        USB_UNKNOWN
    };
    String date_string = "00:00:00AM";
    uint32_t prev_date_str_update = 0;

    void update_date(String date) {
        date_string = date;
        prev_date_str_update = CURRENT_TIME;
    }

    class TopbarController : public ViewController
    {
    public:
        TopbarController()
        {
            text = starting_text;
            usb_state = NO_USB_POWER;
            prev_text_w = 0;
            prev_text_h = 0;
        }

        bool notify(NotificationLevel level, String text, uint32_t timeout) {
            return notify(make_notif(level, text, timeout));
        }

        bool notify(Notification notif) {
            if (notif.text.length() == 0) {
                dodobot_serial::println_error("Notification has no text. Skipping");
                return false;
            }
            if (notif.timeout == 0) {
                dodobot_serial::println_error("Timeout is 0. Skipping notification: %s", notif.text.c_str());
                return false;
            }

            return queue.queue(notif);
        }

        void fillBottomScreen() {
            tft.fillRect(0, get_height(), tft.width(), tft.height(), ST77XX_BLACK);
        }
        void draw()
        {
            update_usb_state();
            update_text();
            draw_text();
            draw_status_icons();
            draw_battery();
        }
        void on_load()
        {
            width = tft.width();

            top_icon_r = height / 2;
            motor_icon_r = top_icon_r / 2;
            status_arc_w = top_icon_r - motor_icon_r - 1;

            battery_V_cy = height / 3;
            battery_mA_cy = 2 * height / 3;

            report_icon_cx = top_icon_r + 2;
            report_icon_cy = height / 2;
            connection_icon_cx = report_icon_cx + 5;
            connection_icon_cy = height / 2;
            battery_x = width - 5;

            prev_reporting = !dodobot::robot_state.is_reporting_enabled;
            prev_motors = !dodobot::robot_state.motors_active;
            prev_usb_state = USB_UNKNOWN;
        }
        void on_unload()  {}
        void on_up()  {}
        void on_down()  {}
        void on_left()  {}
        void on_right()  {}
        void on_back()  {}
        void on_enter()  {}

        int get_height() {
            return height + 1;
        }

        int get_width() {
            return width;
        }

    private:
        int height = 21;
        int width;

        int report_icon_cx = 0;
        int report_icon_cy = 0;

        int connection_icon_cx = 0;
        int connection_icon_cy = 0;

        int top_icon_r;
        int motor_icon_r;
        int status_arc_w;

        int battery_x = 0;
        int battery_V_cy;
        int battery_mA_cy;

        int notif_box_width = 70;
        int notif_box_height = 5;
        int notif_height_offset = 3;

        String starting_text = "Connecting...";
        String text;

        uint16_t notif_color = 0;
        uint32_t notif_timer = 0;

        uint16_t prev_text_w, prev_text_h;

        NotificationQueue queue;
        int current_notif_index = -1;

        bool prev_reporting = false;
        bool prev_motors = false;
        ConnectState usb_state = USB_UNKNOWN;
        ConnectState prev_usb_state = USB_UNKNOWN;

        void check_notif_queue()
        {
            if (current_notif_index == -1) {
                current_notif_index = queue.deque();
                if (current_notif_index == -1) {
                    return;
                }
                notif_timer = CURRENT_TIME;
            }
        }

        void update_text()
        {
            if (queue.empty() && current_notif_index == -1)
            {
                if (usb_state == USB_STABLE) {
                    text = date_string;
                }
                else {
                    text = starting_text;
                }
                notif_color = ST77XX_WHITE;
            }
            else {
                check_notif_queue();

                switch (queue.get_index(current_notif_index).level) {
                    case INFO:  notif_color = ST77XX_WHITE; break;
                    case WARN:  notif_color = ST77XX_YELLOW; break;
                    case ERROR:  notif_color = ST77XX_RED; break;
                }

                text = queue.get_index(current_notif_index).text;

                if (CURRENT_TIME - notif_timer > queue.get_index(current_notif_index).timeout) {
                    queue.set_spent(current_notif_index);
                    current_notif_index = -1;
                }
            }
        }

        void draw_text()
        {
            tft.setTextSize(1);
            uint16_t w, h;
            dodobot_display::textBounds(text, w, h);
            int16_t x = (width - w) / 2;
            int16_t y = (height - h) / 2;

            int16_t prev_x = (width - prev_text_w) / 2;
            int16_t prev_y = (height - prev_text_h) / 2;

            dodobot_display::fillPrevText(prev_x, prev_y, prev_text_w, prev_text_h, x, y, w, h, ST77XX_BLACK);
            prev_text_w = w;
            prev_text_h = h;

            tft.setCursor(x, y);
            tft.setTextColor(notif_color, ST77XX_BLACK);
            // tft.fillRect(0, y, width, y + 8, ST77XX_BLACK);
            tft.print(text);
            tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);

            draw_notif_bar(h);
        }

        void draw_notif_bar(uint16_t text_h)
        {
            if (current_notif_index == -1) {
                return;
            }
            int16_t x = (width - notif_box_width) / 2;
            int16_t y = (height + text_h) / 2 + notif_height_offset;
            tft.fillRect(x, y, notif_box_width, notif_box_height, ST77XX_GRAY);

            uint32_t notif_time = queue.get_index(current_notif_index).timeout;
            uint32_t dt = CURRENT_TIME - notif_timer;
            uint16_t w = (uint16_t)(notif_box_width * (1.0 - (double)dt / notif_time));
            if (w <= 2) {
                tft.fillRect(x, y, notif_box_width, notif_box_height, ST77XX_BLACK);
            }
            else {
                tft.fillRect(x, y, w, notif_box_height, notif_color);
            }
        }
        void draw_status_icons()
        {
            if (prev_reporting != dodobot::robot_state.is_reporting_enabled)
            {
                prev_reporting = dodobot::robot_state.is_reporting_enabled;
                uint16_t status_color = ST77XX_RED;
                if (dodobot::robot_state.is_reporting_enabled) {
                    status_color = ST77XX_GREEN;
                }
                dodobot_display::drawArc(report_icon_cx, report_icon_cy, 90, 270, top_icon_r, top_icon_r, status_arc_w, status_color);
            }

            if (prev_usb_state != usb_state)
            {
                prev_usb_state = usb_state;
                uint16_t conn_color = ST77XX_WHITE;
                if (usb_state == NO_USB_POWER) {
                    conn_color = ST77XX_WHITE;
                }
                else if (usb_state == USB_POWER_DETECTED) {
                    conn_color = ST77XX_YELLOW;
                }
                else if (usb_state == USB_STABLE) {
                    conn_color = ST77XX_GREEN;
                }
                else if (usb_state == SHUTTING_DOWN) {
                    conn_color = ST77XX_WHITE;
                    notify(INFO, "Shutting down", dodobot_latch_circuit::POWER_OFF_THRESHOLD_MS);
                }
                dodobot_display::drawArc(connection_icon_cx, connection_icon_cy, 90, -90, top_icon_r, top_icon_r, status_arc_w, conn_color);
            }

            if (prev_motors != dodobot::robot_state.motors_active)
            {
                prev_motors = dodobot::robot_state.motors_active;
                uint16_t motor_color = ST77XX_GRAY;
                if (dodobot::robot_state.motors_active) {
                    motor_color = ST77XX_GREEN;
                }
                tft.fillCircle(report_icon_cx, report_icon_cy, motor_icon_r, motor_color);
                tft.fillCircle(connection_icon_cx, connection_icon_cy, motor_icon_r, motor_color);

                int w = connection_icon_cx - report_icon_cx;
                int h = 2 * motor_icon_r;
                tft.fillRect(report_icon_cx, report_icon_cy - motor_icon_r, w, h, motor_color);
            }
        }
        void draw_battery()
        {
            String voltage_text = String(dodobot_power_monitor::ina219_loadvoltage) + "V";
            String current_text = String((int)dodobot_power_monitor::ina219_current_mA) + "mA";

            tft.setTextSize(1);
            uint16_t w_V, h_V;
            dodobot_display::textBounds(voltage_text, w_V, h_V);
            uint16_t single_char_w = w_V / voltage_text.length();
            int16_t x_V = battery_x - w_V;
            int16_t y_V = battery_V_cy - h_V / 2;

            uint16_t w_mA, h_mA;
            dodobot_display::textBounds(current_text, w_mA, h_mA);
            int16_t x_mA = battery_x - w_mA;
            int16_t y_mA = battery_mA_cy - h_mA / 2 + 1;

            int gauge_count = dodobot_power_monitor::power_level();
            uint16_t battery_color = ST77XX_DARKER_GREEN;
            if (gauge_count <= 1) {
                battery_color = ST77XX_RED;
            }
            uint16_t battery_nub_color = ST77XX_GRAY;
            if (gauge_count == dodobot_power_monitor::max_power_level) {
                battery_nub_color = ST77XX_DARKER_GREEN;
            }

            int gauge_len = dodobot_power_monitor::max_power_level;
            int h_gauge = h_V + h_mA + 1;

            int batt_nub_h = 7;
            int batt_nub_y = (height - batt_nub_h) / 2 + 1;
            tft.fillRect(battery_x, batt_nub_y, 4, batt_nub_h, battery_nub_color);

            int stop_index = max(voltage_text.length(), current_text.length());
            stop_index = max(stop_index, gauge_len);
            for (int index = 0; index < stop_index; index++)
            {
                if (index < gauge_len)
                {
                    int gauge_x = battery_x - single_char_w * (index + 1);
                    int inv_index = gauge_len - index;
                    uint16_t capacity_color = ST77XX_GRAY;
                    if (inv_index <= gauge_count) {
                        capacity_color = battery_color;
                    }

                    tft.fillRect(gauge_x, y_V, single_char_w, h_gauge, capacity_color);
                }
            }

            tft.setTextColor(ST77XX_WHITE, ST77XX_WHITE);  // transparent text background
            tft.setCursor(x_V, y_V);
            tft.print(voltage_text);

            tft.setCursor(x_mA, y_mA);
            tft.print(current_text);
            tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        }

        void update_usb_state()
        {
            if (!dodobot_latch_circuit::state.usb_voltage_state) {
                if (dodobot_latch_circuit::state.usb_connected_once) {
                    usb_state = SHUTTING_DOWN;
                }
                else {
                    usb_state = NO_USB_POWER;
                }
            }
            else {
                if (prev_date_str_update == 0 || CURRENT_TIME - prev_date_str_update > 2000) {
                    usb_state = USB_POWER_DETECTED;
                }
                else {
                    usb_state = USB_STABLE;
                }
            }
        }
    };

    const int num_main_menu_entries = 4;

    enum MainMenuEntry {
        NETWORK = 0,
        ROBOT = 1,
        SYSTEM = 2,
        SHUTDOWN = 3
    };

    class MainMenuController : public ViewWithOverlayController
    {
    public:
        MainMenuController(ViewController* topbar) : ViewWithOverlayController(topbar) {
            draw_slots = new int[num_draw_slots];
        }

        TopbarController* topbar() {
            return (TopbarController*) overlay;
        }
        void draw_with_overlay()  {}
        void on_load_with_overlay()
        {
            // tft.fillScreen(ST77XX_BLUE);
            draw_slots[0] = tft.width() / 4 - 30;
            draw_slots[1] = tft.width() / 2;
            draw_slots[2] = 3 * tft.width() / 4 + 30;
            draw_height = tft.height() / 2;

            icon_text_height = draw_height + selected_h / 2 + 10;

            overview_x = tft.width() / 2;
            overview_y = draw_height + selected_h / 2 + 25;

            draw_all();

            // topbar()->notify(INFO, "1", 2000);
            // topbar()->notify(INFO, "2", 2000);
            // topbar()->notify(INFO, "3", 2000);
            // topbar()->notify(INFO, "4", 2000);
            // topbar()->notify(INFO, "5", 2000);
            // topbar()->notify(INFO, "6", 2000);
        }

        void move_select_left()
        {
            selected_index--;
            if (selected_index < 0) {
                selected_index = num_main_menu_entries - 1;
            }
        }
        void move_select_right()
        {
            selected_index++;
            if (selected_index >= num_main_menu_entries) {
                selected_index = 0;
            }
        }

        void on_unload_with_overlay()  {}
        void on_up()  {}
        void on_down()  {}
        void on_left()  {
            move_select_left();
            draw_all();
        }
        void on_right()  {
            move_select_right();
            draw_all();
        }
        void on_back()  {
            load("splash");
        }
        void on_enter()  {
            if (selected_index < 0 || selected_index >= num_main_menu_entries) {
                return;
            }
            load(view_name((MainMenuEntry)selected_index));
        }

        void draw_all()
        {
            topbar()->fillBottomScreen();

            for (int index = 0; index < num_draw_slots; index++) {
                int cx = draw_slots[index];
                int draw_index = (selected_index + index - 1) % num_main_menu_entries;
                if (draw_index < 0) {
                    draw_index = num_main_menu_entries - abs(draw_index);
                }
                draw_icon(cx, (MainMenuEntry)draw_index);
            }

            int cx = draw_slots[1];
            int cy = draw_height;
            int x = cx - selected_w / 2;
            int y = cy - selected_h / 2;
            tft.drawRoundRect(x, y, selected_w, selected_h, selected_r, ST77XX_WHITE);

            draw_selection_overview();
        }

        void draw_selection_overview()
        {
            int total_width = (overview_r + 2) * 2 * num_main_menu_entries;
            int offset = overview_x - total_width / 2;
            int increment = total_width / (num_main_menu_entries - 1);
            uint16_t color;
            for (int index = 0; index < num_main_menu_entries; index++)
            {
                if (index == selected_index) {
                    color = ST77XX_WHITE;
                }
                else {
                    color = ST77XX_GRAY;
                }
                tft.fillCircle(offset, overview_y, overview_r, color);
                offset += increment;
            }
        }

        void draw_icon(int cx, MainMenuEntry index)
        {
            int cy = draw_height;
            int x = cx - icon_w / 2;
            int y = cy - icon_h / 2;

            switch (index) {
                case NETWORK: draw_network_icon(x, y, cx, cy);  break;
                case ROBOT:  draw_robot_status_icon(x, y, cx, cy);  break;
                case SYSTEM:  draw_system_icon(x, y, cx, cy);  break;
                case SHUTDOWN:  draw_shutdown_icon(x, y, cx, cy);  break;
                default: tft.fillRoundRect(x, y, icon_w, icon_h, icon_r, ST77XX_WHITE); break;
            }

            tft.setTextSize(1);
            tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
            String text = entry_name(index);
            uint16_t w, h;
            dodobot_display::textBounds(text, w, h);
            x = cx - w / 2;
            y = icon_text_height - h / 2;
            tft.setCursor(x, y);
            tft.print(text);
        }

        void draw_network_icon(int x, int y, int cx, int cy)
        {
            tft.fillRoundRect(x, y, icon_w, icon_h, icon_r, ST77XX_WHITE);
            int fan_angle = 35;
            int center_angle = 270;
            int max_fan_w = icon_h - 10;
            int mid_fan_w = 2 * max_fan_w / 3 + 3;
            int low_fan_w = max_fan_w / 3 + 5;
            int thickness = 6;
            x = cx;
            y = cy + icon_h / 2 - 5;
            dodobot_display::drawArc(x, y, center_angle - fan_angle, center_angle + fan_angle, max_fan_w, max_fan_w, thickness, ST77XX_GRAY);
            dodobot_display::drawArc(x, y, center_angle - fan_angle, center_angle + fan_angle, mid_fan_w, mid_fan_w, thickness, ST77XX_GRAY);
            dodobot_display::drawArc(x, y, center_angle - fan_angle, center_angle + fan_angle, low_fan_w, low_fan_w, thickness, ST77XX_GRAY);
            tft.fillCircle(x, y - 3, 5, ST77XX_GRAY);
        }

        void draw_robot_status_icon(int x, int y, int cx, int cy)
        {
            tft.fillRoundRect(x, y, icon_w, icon_h, icon_r, ST77XX_LIGHT_BLUE);
            int triangle_cx = cx + 2;
            int triangle_cy = cy + 5;
            int x0 = triangle_cx;
            int y0 = triangle_cy;
            int x1 = cx - icon_w / 2 + 7;
            int y1 = cy + icon_h / 2 - 15;
            int x2 = cx + icon_w / 2 - 15;
            int y2 = cy - icon_h / 2 + 7;
            int x3 = cx + icon_w / 2 - 15;
            int y3 = cy + icon_h / 2 - 7;
            tft.fillTriangle(x0, y0, x1, y1, x2, y2, ST77XX_DARKER_BLUE);
            tft.fillTriangle(x0, y0, x2, y2, x3, y3, ST77XX_DARKER_BLUE);
        }

        void draw_system_icon(int x, int y, int cx, int cy)
        {
            tft.fillRoundRect(x, y, icon_w, icon_h, icon_r, ST77XX_GRAY);

            int outer_r = icon_w / 2 - 7;
            int inner_r = icon_w / 7;
            dodobot_display::drawCircle(cx, cy, outer_r, 5, ST77XX_BLACK);
            tft.fillCircle(cx, cy, inner_r, ST77XX_BLACK);

            int center_angle = 0;
            for (int index = 0; index < 8; index++)
            {
                int tooth_cx = (float)outer_r * cos(TO_RADIANS(center_angle)) + cx;
                int tooth_cy = (float)outer_r * sin(TO_RADIANS(center_angle)) + cy;

                tft.fillCircle(tooth_cx, tooth_cy, 5, ST77XX_BLACK);

                center_angle += 45;
            }
        }

        void draw_shutdown_icon(int x, int y, int cx, int cy)
        {
            tft.fillRoundRect(x, y, icon_w, icon_h, icon_r, ST77XX_LIGHT_PINK);

            int outer_r = icon_w / 2 - 7;
            int rect_w = 5;
            int rect_h = 20;
            int rect_border = 6;

            dodobot_display::drawCircle(cx, cy, outer_r, rect_w, ST77XX_BLACK);

            x = cx - rect_w / 2;
            y = cy - outer_r - 3;
            tft.fillRect(x - rect_border / 2, y - rect_border / 2, rect_w + rect_border, rect_h + rect_border, ST77XX_LIGHT_PINK);
            tft.fillRoundRect(x, y, rect_w, rect_h, 2, ST77XX_BLACK);
        }

        String view_name(MainMenuEntry entry) {
            switch (entry) {
                case NETWORK: return "network";
                case ROBOT:  return "robot";
                case SYSTEM:  return "system";
                case SHUTDOWN:  return "shutdown";
                default: return "";
            }
        }
    private:
        int num_draw_slots = 3;
        int* draw_slots;

        int draw_height = 0;
        int icon_text_height = 0;
        int icon_w = 50;
        int icon_h = 50;
        int icon_r = 10;

        int selected_w = 60;
        int selected_h = 60;
        int selected_r = 15;

        int overview_x = 0;
        int overview_y = 0;
        int overview_r = 5;

        int selected_index = 1;

        String entry_name(MainMenuEntry entry) {
            switch (entry) {
                case NETWORK: return "Network";
                case ROBOT:  return "Robot";
                case SYSTEM:  return "Systems";
                case SHUTDOWN:  return "Shutdown";
                default: return "";
            }
        }

    };

    class NetworkScreenController : public ViewWithOverlayController
    {
    public:
        NetworkScreenController(ViewController* topbar) : ViewWithOverlayController(topbar) {

        }

        TopbarController* topbar() {
            return (TopbarController*) overlay;
        }
        void draw_with_overlay()  {}
        void on_load_with_overlay()
        {
            topbar()->fillBottomScreen();
        }

        void on_unload_with_overlay()  {}
        void on_up()  {}
        void on_down()  {}
        void on_left()  {}
        void on_right()  {}
        void on_back()  {
            load("main-menu");
        }
        void on_enter()  {}
    };

    class RobotScreenController : public ViewWithOverlayController
    {
    public:
        RobotScreenController(ViewController* topbar) : ViewWithOverlayController(topbar) {

        }

        TopbarController* topbar() {
            return (TopbarController*) overlay;
        }
        void draw_with_overlay()  {}
        void on_load_with_overlay()
        {
            topbar()->fillBottomScreen();
        }

        void on_unload_with_overlay()  {}
        void on_up()  {}
        void on_down()  {}
        void on_left()  {}
        void on_right()  {}
        void on_back()  {
            load("main-menu");
        }
        void on_enter()  {}
    };

    class SystemScreenController : public ViewWithOverlayController
    {
    public:
        SystemScreenController(ViewController* topbar) : ViewWithOverlayController(topbar) {

        }

        TopbarController* topbar() {
            return (TopbarController*) overlay;
        }
        void draw_with_overlay() {
            dodobot_sd::drawGIFframe();
        }
        void on_load_with_overlay()
        {
            topbar()->fillBottomScreen();
            dodobot_sd::loadGIF("CHANSEY.GIF");
            dodobot_sd::setGIFoffset(0, topbar()->get_height());
        }

        void on_unload_with_overlay() {
            dodobot_sd::closeGIF();
        }
        void on_up()  {}
        void on_down()  {}
        void on_left()  {}
        void on_right()  {}
        void on_back()  {
            load("main-menu");
        }
        void on_enter()  {}
    };

    class ShutdownScreenController : public ViewWithOverlayController
    {
    public:
        ShutdownScreenController(ViewController* topbar) : ViewWithOverlayController(topbar) {

        }

        TopbarController* topbar() {
            return (TopbarController*) overlay;
        }
        void draw_with_overlay()  {}
        void on_load_with_overlay()
        {
            topbar()->fillBottomScreen();
        }

        void on_unload_with_overlay()  {}
        void on_up()  {}
        void on_down()  {}
        void on_left()  {}
        void on_right()  {}
        void on_back()  {
            load("main-menu");
        }
        void on_enter()  {}
    };


    void add_view(String name, ViewController* view)
    {
        names[num_views] = name;
        views[num_views++] = view;
    }

    ViewController* view_from_name(String name) {
        for (size_t i = 0; i < num_views; i++) {
            if (names[i].equals(name)) {
                return views[i];
            }
        }
        return NULL;
    }

    void load(String name)
    {
        if (current_view != NULL) {
            current_view->on_unload();
        }
        current_view = view_from_name(name);

        if (current_view != NULL) {
            dodobot_serial::println_info("Loading view %s", name.c_str());
            current_view->on_load();
        }
        else {
            dodobot_serial::println_error("Loaded view %s is NULL", name.c_str());
        }
    }

    SplashScreenController* splash = new SplashScreenController();
    TopbarController* topbar = new TopbarController();
    MainMenuController* main_menu = new MainMenuController(topbar);
    NetworkScreenController* network_screen = new NetworkScreenController(topbar);
    RobotScreenController* robot_screen = new RobotScreenController(topbar);
    SystemScreenController* system_screen = new SystemScreenController(topbar);
    ShutdownScreenController* shutdown_screen = new ShutdownScreenController(topbar);

    void init()
    {
        add_view("splash", splash);
        add_view("main-menu", main_menu);
        add_view(main_menu->view_name(NETWORK), network_screen);
        add_view(main_menu->view_name(ROBOT), robot_screen);
        add_view(main_menu->view_name(SYSTEM), system_screen);
        add_view(main_menu->view_name(SHUTDOWN), shutdown_screen);
        load("splash");
        dodobot_serial::println_info("UI ready");
    }
    void draw()
    {
        if (CURRENT_TIME - ui_timer < UI_DELAY_MS) {
            return;
        }
        ui_timer = CURRENT_TIME;

        if (current_view == NULL)  return;
        current_view->draw();
    }

    void notify(NotificationLevel level, String text, uint32_t timeout) {
        topbar->notify(level, text, timeout);
    }

    EventType last_event;

    void on_up() {
        if (current_view == NULL)  return;
        current_view->on_up();
        last_event = UP_EVENT;
    }

    void on_down() {
        if (current_view == NULL)  return;
        current_view->on_down();
        last_event = DOWN_EVENT;
    }

    void on_left() {
        if (current_view == NULL)  return;
        current_view->on_left();
        last_event = LEFT_EVENT;
    }

    void on_right() {
        if (current_view == NULL)  return;
        current_view->on_right();
        last_event = RIGHT_EVENT;
    }

    void on_back() {
        if (current_view == NULL)  return;
        current_view->on_back();
        last_event = BACK_EVENT;
    }

    void on_enter() {
        if (current_view == NULL)  return;
        current_view->on_enter();
        last_event = ENTER_EVENT;
    }

    void on_repeat() {
        if (current_view == NULL)  return;
        current_view->on_repeat(last_event);
    }
}
