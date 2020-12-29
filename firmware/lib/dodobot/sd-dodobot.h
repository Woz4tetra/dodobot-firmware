#ifndef __DODOBOT_SD_H__
#define __DODOBOT_SD_H__

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

#include "dodobot.h"


namespace dodobot_sd
{
    const int chipSelect = BUILTIN_SDCARD;
    bool initialized = false;

    void print_dir(File dir, int num_spaces);
    void print_spaces(int num);

    void setup()
    {
        if (SD.begin(chipSelect)) {
            initialized = true;
        }
        else {
            dodobot_serial::println_error("SD card failed to initialize!");
        }
    }

    void list_all_files()
    {
        if (!initialized) {
            return;
        }
        File root = SD.open("/");
        print_dir(root, 0);
    }

    void print_dir(File dir, int num_spaces)
    {
        while (true) {
            File entry = dir.openNextFile();
            if (!entry) {
                break;
            }
            print_spaces(num_spaces);
            INFO_SERIAL.print(entry.name());
            if (entry.isDirectory()) {
                INFO_SERIAL.println("/");
                print_dir(entry, num_spaces+2);
            } else {
                // files have sizes, directories do not
                print_spaces(48 - num_spaces - strlen(entry.name()));
                INFO_SERIAL.print("  ");
                INFO_SERIAL.println(entry.size(), DEC);
            }
            entry.close();
        }
    }

    void print_spaces(int num) {
        for (int i=0; i < num; i++) {
            INFO_SERIAL.print(" ");
        }
    }
}

#endif  // __DODOBOT_SD_H__
