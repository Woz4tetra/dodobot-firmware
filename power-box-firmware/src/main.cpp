#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <dodobot_serial/dodobot_serial.h>
#include <dodobot_power_monitor/dodobot_power_monitor.h>
#include <dodobot_neopixel_ring/dodobot_neopixel_ring.h>


#define DEVICE_KEYWORD "dodobot_power_box"

void packet_callback(DodobotSerial* interface, String category);
DodobotSerial* serial_inferface = new DodobotSerial(packet_callback);

DodobotPowerMonitor* power_monitor = new DodobotPowerMonitor(serial_inferface);
DodobotNeopixelRing* neopixel_ring = new DodobotNeopixelRing(serial_inferface, 9, 24, NEO_GRBW + NEO_KHZ800);

void setup()
{
    DODOBOT_SERIAL.begin(115200);
    
    power_monitor->begin();
    neopixel_ring->begin();
}


void loop()
{
    power_monitor->update();
    serial_inferface->read();
}

void packet_callback(DodobotSerial* interface, String category)
{
    if (category.equals("?"))  // ready command
    {
        String keyword;
        CHECK_SEGMENT(interface, interface->segment_as_string(keyword));
        if (keyword.equals("dodobot"))
        {
            DODOBOT_SERIAL.println("Received ready signal!");
            interface->write("ready", "us", millis(), DEVICE_KEYWORD);
        }
    }
    else if (category.equals("!"))  // stop command
    {
        String keyword;
        CHECK_SEGMENT(interface, interface->segment_as_string(keyword));
        if (keyword.equals("dodobot"))
        {
            neopixel_ring->off();
        }
    }
    else if (category.equals("pix"))
    {
        neopixel_ring->packet_callback();
    }
}