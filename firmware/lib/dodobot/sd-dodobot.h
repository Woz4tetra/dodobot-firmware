#ifndef __DODOBOT_SD_H__
#define __DODOBOT_SD_H__

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <AnimatedGIF.h>

#include "dodobot.h"
#include "display-dodobot.h"


using namespace dodobot_display;


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

    //
    // Animated GIFs
    //

    AnimatedGIF gif;
    GIFINFO gif_info;
    static File FSGifFile; // temp gif file holder

    static int xOffset = 0;
    static int yOffset = 0;

    static void * GIFOpenFile(const char *fname, int32_t *pSize)
    {
        //log_d("GIFOpenFile( %s )\n", fname );
        FSGifFile = SD.open(fname);
        if (FSGifFile) {
            *pSize = FSGifFile.size();
            return (void *)&FSGifFile;
        }
        return NULL;
    }


    static void GIFCloseFile(void *pHandle)
    {
        File *f = static_cast<File *>(pHandle);
        if (f != NULL) {
            f->close();
        }
    }


    static int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen)
    {
        int32_t iBytesRead;
        iBytesRead = iLen;
        File *f = static_cast<File *>(pFile->fHandle);
        // Note: If you read a file all the way to the last byte, seek() stops working
        if ((pFile->iSize - pFile->iPos) < iLen) {
            iBytesRead = pFile->iSize - pFile->iPos - 1; // <-- ugly work-around
        }
        if (iBytesRead <= 0) {
            return 0;
        }
        iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
        pFile->iPos = f->position();
        return iBytesRead;
    }


    static int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition)
    {
        int i = micros();
        File *f = static_cast<File *>(pFile->fHandle);
        f->seek(iPosition);
        pFile->iPos = (int32_t)f->position();
        i = micros() - i;
        //log_d("Seek time = %d us\n", i);
        return pFile->iPos;
    }


    static void TFTDraw(int x, int y, int w, int h, uint16_t* lBuf )
    {
        dodobot_display::pushRect(x + xOffset, y + yOffset, w, h, lBuf);
    }


    // Draw a line of image directly on the LCD
    void GIFDraw(GIFDRAW *pDraw)
    {
        uint8_t *s;
        uint16_t *d, *usPalette, usTemp[320];
        int x, y, iWidth;

        iWidth = pDraw->iWidth;
        if (iWidth > tft.width()) {
            iWidth = tft.width();
        }
        usPalette = pDraw->pPalette;
        y = pDraw->iY + pDraw->y; // current line

        s = pDraw->pPixels;
        if (pDraw->ucDisposalMethod == 2) {// restore to background color
            for (x=0; x<iWidth; x++) {
                if (s[x] == pDraw->ucTransparent) {
                    s[x] = pDraw->ucBackground;
                }
            }
            pDraw->ucHasTransparency = 0;
        }
        // Apply the new pixels to the main image
        if (pDraw->ucHasTransparency) { // if transparency used
            uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
            int x, iCount;
            pEnd = s + iWidth;
            x = 0;
            iCount = 0; // count non-transparent pixels
            while (x < iWidth) {
                c = ucTransparent-1;
                d = usTemp;
                while (c != ucTransparent && s < pEnd) {
                    c = *s++;
                    if (c == ucTransparent) { // done, stop
                        s--; // back up to treat it like transparent
                    }
                    else { // opaque
                        *d++ = usPalette[c];
                        iCount++;
                    }
                } // while looking for opaque pixels
                if (iCount) { // any opaque pixels?
                    TFTDraw( pDraw->iX+x, y, iCount, 1, (uint16_t*)usTemp );
                    x += iCount;
                    iCount = 0;
                }
                // no, look for a run of transparent pixels
                c = ucTransparent;
                while (c == ucTransparent && s < pEnd) {
                    c = *s++;
                    if (c == ucTransparent) {
                        iCount++;
                    }
                    else {
                        s--;
                    }
                }
                if (iCount) {
                    x += iCount; // skip these
                    iCount = 0;
                }
            }
        } else {
            s = pDraw->pPixels;
            // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
            for (x=0; x<iWidth; x++) {
                usTemp[x] = usPalette[*s++];
            }
            TFTDraw( pDraw->iX, y, iWidth, 1, (uint16_t*)usTemp );
        }
    } /* GIFDraw() */

    void setGIFoffset(int x, int y)
    {
        xOffset = x;
        yOffset = y;
    }

    bool is_gif_loaded = false;
    int32_t current_frame = 0;

    int loadGIF(const char* path)
    {
        if (!SD.exists(path)) {
            dodobot_serial::println_error("Path %s doesn't exist", path);
            return -1;
        }

        gif.begin(LITTLE_ENDIAN_PIXELS);

        if (!gif.open(path, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw)) {
            dodobot_serial::println_error("Could not open gif %s", path);
            return -1;
        }
        setGIFoffset(0, 0);
        gif.getInfo(&gif_info);
        if (gif_info.iFrameCount <= 0) {
            dodobot_serial::println_error("Loaded GIF has no frames %s!", path);
        }
        is_gif_loaded = true;
        return gif_info.iFrameCount;
    }

    int singleFrameGIF()
    {
        if (!is_gif_loaded) {
            return -1;
        }
        int frameDelay = 0;
        int result = gif.playFrame(true, &frameDelay);
        current_frame++;
        if (result != -1) {
            return result;
        }
        if (current_frame >= gif_info.iFrameCount) {
            return 0;
        }

        return 1;
    }

    void closeGIF() {
        if (!is_gif_loaded) {
            return;
        }
        is_gif_loaded = false;
        gif.close();
    }
}

#endif  // __DODOBOT_SD_H__
