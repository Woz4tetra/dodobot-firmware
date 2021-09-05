#include <dodobot_neopixel_ring/dodobot_neopixel_ring.h>

DodobotNeopixelRing::DodobotNeopixelRing(DodobotSerial* serial, int pin, int num_pixels, int pixel_format)
{
    this->serial = serial;
    this->pin = pin;
    this->num_pixels = num_pixels;
    this->pixel_format = pixel_format;

    pixels = new Adafruit_NeoPixel(num_pixels, pin, pixel_format);
}

void DodobotNeopixelRing::begin()
{
    pixels->begin();
    pixels->clear();
}

void DodobotNeopixelRing::off()
{
    pixels->clear();
    pixels->show();
}

void DodobotNeopixelRing::packet_callback()
{
    pixels->clear();
    int32_t pattern_type;
    CHECK_SEGMENT(serial, serial->segment_as_int32(pattern_type));
    
    uint32_t color;
    int pattern_delay = 0;
    switch (pattern_type)
    {
        case 1: color = pixels->Color(255, 255, 255, 255); break;
        case 2: color = pixels->Color(0, 255, 0, 20); pattern_delay = 10; break;
        default: color = pixels->Color(0, 0, 0, 0); break;
    }
    for (int i = 0; i < num_pixels; i++)
    {
        pixels->setPixelColor(i, color);
        if (delay > 0) {
            pixels->show();
            delay(pattern_delay);
        }
    }
    pixels->show();
}
