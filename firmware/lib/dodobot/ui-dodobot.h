
#pragma once

#include "serial-dodobot.h"
#include "sd-dodobot.h"
#include "display-dodobot.h"


using namespace dodobot_display;



namespace dodobot_ui
{
    const uint32_t UI_DELAY_MS_DEFAULT = 100;
    uint32_t UI_DELAY_MS = UI_DELAY_MS_DEFAULT;
    uint32_t ui_timer = 0;

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

    struct Notification {
        int level;
        String text;
        uint32_t timeout;
    };
    enum ConnectState {
        NO_USB_POWER,
        USB_POWER_DETECTED,
        USB_STABLE
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
        }

        void fillBottomScreen() {
            tft.fillRect(0, height + 1, tft.width(), tft.height(), ST77XX_BLACK);
        }
        void draw()
        {
            draw_text();
            draw_status_icons();
            draw_battery();
        }
        void on_load()
        {
            width = tft.width();

            top_icon_r = height / 2;
            motor_icon_r = top_icon_r / 2;

            battery_V_cy = height / 3;
            battery_mA_cy = 2 * height / 3;

            report_icon_cx = top_icon_r + 2;
            report_icon_cy = height / 2;
            connection_icon_cx = report_icon_cx + 5;
            connection_icon_cy = height / 2;
            battery_x = width - 5;

        }
        void on_unload()  {}
        void on_up()  {}
        void on_down()  {}
        void on_left()  {}
        void on_right()  {}
        void on_back()  {}
        void on_enter()  {}

        void draw_text()
        {
            update_usb_state();
            if (usb_state == USB_STABLE) {
                text = date_string;
            }
            else {
                text = starting_text;
            }
            notif_color = ST77XX_WHITE;

            tft.setTextSize(1);
            uint16_t w, h;
            dodobot_display::textBounds(text, w, h);
            int16_t x = (width - w) / 2;
            int16_t y = (height - h) / 2;
            tft.setCursor(x, y);

            tft.setTextColor(notif_color, ST77XX_BLACK);
            // tft.fillRect(0, y, width, y + 8, ST77XX_BLACK);
            tft.print("  " + text + "  ");
            tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        }
        void draw_status_icons()
        {

        }
        void draw_battery()
        {

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

        int battery_x = 0;
        int battery_V_cy;
        int battery_mA_cy;

        int notif_box_width = 60;
        int notif_height_offset = 3;

        String starting_text = "Connecting...";
        String text;

        static const size_t max_notifs = 5;
        size_t notif_index = 0;
        Notification* notif_queue[max_notifs];
        Notification* notif;
        uint16_t notif_color = 0;
        uint32_t notif_timer = 0;

        ConnectState usb_state;

        void update_usb_state()
        {
            if (!dodobot_latch_circuit::state.usb_voltage_state) {
                usb_state = NO_USB_POWER;
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

    const size_t num_main_menu_entries = 4;

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
            topbar()->fillBottomScreen();

            draw_slots[0] = tft.width() / 4 - 30;
            draw_slots[1] = tft.width() / 2;
            draw_slots[2] = 3 * tft.width() / 4 + 30;
            draw_height = tft.height() / 2;

            icon_text_height = draw_height + selected_h / 2 + 10;

            overview_x = tft.width() / 2;
            overview_y = draw_height + selected_h / 2 + 25;

            draw_all();
        }
        void on_unload_with_overlay()  {}
        void on_up()  {}
        void on_down()  {}
        void on_left()  {}
        void on_right()  {}
        void on_back()  {}
        void on_enter()  {}

        void draw_all()
        {
            tft.startWrite();
            for (int index = 0; index < num_draw_slots; index++) {
                int cx = draw_slots[index];
                int draw_index = (selected_index + index - 1) % num_main_menu_entries;
                draw_icon(cx, (MainMenuEntry)draw_index);
            }

            int cx = draw_slots[1];
            int cy = draw_height;
            int x = cx - selected_w / 2;
            int y = cy - selected_h / 2;
            tft.drawRoundRect(x, y, selected_w, selected_h, selected_r, ST77XX_WHITE);
            tft.endWrite();
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
                default: tft.fillRoundRect(x, y, icon_w, icon_h, icon_r, ST77XX_WHITE);
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
                case SYSTEM:  return "System Status";
                case SHUTDOWN:  return "Shutdown";
                default: return "";
            }
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
    void init()
    {
        add_view("splash", splash);
        add_view("main-menu", main_menu);
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

    enum EventType {
        UP_EVENT,
        DOWN_EVENT,
        LEFT_EVENT,
        RIGHT_EVENT,
        BACK_EVENT,
        ENTER_EVENT,
        NONE_EVENT
    };

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
        switch (last_event) {
            case UP_EVENT:  current_view->on_up();  break;
            case DOWN_EVENT:  current_view->on_down();  break;
            case LEFT_EVENT:  current_view->on_left();  break;
            case RIGHT_EVENT:  current_view->on_right();  break;
            case BACK_EVENT:  current_view->on_back();  break;
            case ENTER_EVENT:  current_view->on_enter();  break;
            default: return;
        }
    }
}
