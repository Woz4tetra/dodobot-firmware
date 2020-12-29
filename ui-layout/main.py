import os
import random
import pygame
from datetime import datetime
from adafruit_gfx import Adafruit_ST77XX
from adafruit_gfx.color565 import *
from ui_elements.view_controller import ViewController
from view_controllers.main_menu import MainMenuController
from view_controllers.splash_screen import SplashScreenController
from view_controllers.topbar import TopbarController
from view_controllers import topbar
from view_controllers.robot_status import RobotStatusController

VOLTAGE_V = 11.0


def on_event(tft, event):
    global VOLTAGE_V

    if event.type != pygame.KEYDOWN:
        return
    topbar_inst = tft.controllers[1].topbar
    if event.key == pygame.K_q:
        if topbar_inst.reporting_status == topbar.REPORTING_DISABLED:
            topbar_inst.reporting_status = topbar.REPORTING_ENABLED
        else:
            topbar_inst.reporting_status = topbar.REPORTING_DISABLED

    elif event.key == pygame.K_e:
        if topbar_inst.connection_status == topbar.NO_USB_POWER:
            topbar_inst.connection_status = topbar.USB_POWER_DETECTED
        elif topbar_inst.connection_status == topbar.USB_POWER_DETECTED:
            topbar_inst.connection_status = topbar.USB_CONNECTION_STABLE
        else:
            topbar_inst.connection_status = topbar.NO_USB_POWER

    elif event.key == pygame.K_w:
        if topbar_inst.motor_status == topbar.MOTORS_INACTIVE:
            topbar_inst.motor_status = topbar.MOTORS_ACTIVE
        else:
            topbar_inst.motor_status = topbar.MOTORS_INACTIVE

    elif event.key == pygame.K_h:
        topbar_inst.notify(2, "Not homed", 3.0)

    elif event.key == pygame.K_g:
        topbar_inst.notify(2, "Position error", 3.0)

    elif event.key == pygame.K_f:
        topbar_inst.notify(2, "Low battery", 6.0)

    elif event.key == pygame.K_v:
        VOLTAGE_V -= 1.0
        if VOLTAGE_V < topbar.CRITICAL_VOLTAGE - 1.0:
            VOLTAGE_V = topbar.FULL_VOLTAGE

def update_topbar_time(tft):
    topbar_inst = tft.controllers[1].topbar
    if topbar_inst.connection_status == topbar.USB_CONNECTION_STABLE:
        topbar_inst.set_datetime_text(datetime.now().strftime("%I:%M:%S%p"))


def update_topbar_battery(tft):
    global VOLTAGE_V
    topbar_inst = tft.controllers[1].topbar
    topbar_inst.voltage_V = random.random() + VOLTAGE_V
    topbar_inst.current_mA = random.randint(900, 1100)


def main():
    topbar = TopbarController()
    splash_screen = SplashScreenController()
    main_menu = MainMenuController(topbar)
    robot_status = RobotStatusController(topbar)

    os.environ['SDL_VIDEO_WINDOW_POS'] = '%i,%i' % (0, 0)

    tft = Adafruit_ST77XX(on_event)

    tft.add_controller(splash_screen)
    tft.add_controller(main_menu)
    tft.add_controller(robot_status, index=2)
    tft.add_controller(robot_status, index=3)
    tft.add_controller(robot_status, index=4)
    tft.add_controller(robot_status, index=5)

    tft.init(160, 128)
    # tft.init(320, 256)
    tft.set_delay(300)
    tft.set_scale(3.0)

    tft.set_display_brightness(255)
    tft.fillScreen(ST77XX_BLACK)
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK)

    tft.setTextWrap(False)
    tft.setTextSize(1)
    tft.setRotation(3)

    tft.print("Hello!\n")
    print("Display ready")

    tft.fillScreen(ST77XX_BLACK)

    tft.load_view(1)

    while tft.running:
        if not tft.update():
            continue

        # tft.fillScreen(ST77XX_BLACK)
        update_topbar_time(tft)
        update_topbar_battery(tft)

    pygame.quit()


if __name__ == '__main__':
    main()
