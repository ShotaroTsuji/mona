/*!
    \file  VesaConsole.cpp
    \brief class VesaConsole

    class VesaConsole

    Copyright (c) 2004 Gaku
    All rights reserved.
    License=MIT/X Licnese

    \author  Gaku
    \version $Revision$
    \date   create:2004/01/22 update:$Date$
*/

#include <stdafx.h>

#include "Gothn16x.h"

extern void RefreshMainWindow();

dword VesaConsole::palette[] = {
                        0x00000000, // black
                        0x000000ff, // blue
                        0x0000ff00, // green
                        0x0000ffff, // skyblue
                        0x00ff0000, // red
                        0x00ff00ff, // pink
                        0x00808000, // orange
                        0x00c0c0c0, // gray
                        0x00808080, // darkgray
                        0x00000080, // lightblue
                        //                        0x00008080, // lightbue
                        0x00D9C4D1, // lightbue
                        //                        0x00008000, // lightgreen
                        0x0096A48E, // lightgreen
                        0x00800000, // lightred
                        0x00800080, // darkpink
                        0x00ffff00, // yellow
                        0x00ffffff  // white
                };

VesaConsole::VesaConsole (VesaInfoDetail *info)
    : screen(info)
{
    xResolution_ = info->xResolution;
    yResolution_ = info->yResolution;
    pos_x_ = 0;
    pos_y_ = 0;

    font_x_ = font[14];
    font_y_ = font[15];
    font_ = font + 17;
    console_x_ = xResolution_ / font_x_;
    console_y_ = yResolution_ / font_y_;

    int len = console_x_ * console_y_;
    char_buffer_ = new char[len];
    palette_buffer_ = new dword[len];

    bg_ = getColor(GP_BLACK);
    ch_ = getColor(GP_WHITE);

    clearScreen();
}

void VesaConsole::printf(const char *format, ...)
{
    void** list = (void **)&format;

    list++;
    for (int i = 0; format[i] != '\0'; i++) {

        if (format[i] == '%') {
            i++;

            switch (format[i]) {
              case 's':
                  print((char *)*list);
				  list++;
                  break;
              case 'd':
                  putInt(*(int*)list, 10);
				  list++;
                  break;
              case 'x':
                  print("0x");
                  putInt(*(int*)list, 16);
				  list++;
                  break;
              case 'c':
                  putCharacter((char)*(int*)(list));
				  list++;
                  break;
              case '%':
                  putCharacter('%');
                  break;
              case '\0':
                  i--;
                  break;
            }
        } else {
            putCharacter(format[i]);
        }
    }
	RefreshMainWindow();
}

void VesaConsole::putInt(size_t n, int base) {

    static char buf[256];
    int geta;
    int num = (int)n;

    if (base != 16) {

        for (geta = 0; num; num /= 10, geta++);
        if ((int)n < 0) {
            geta++;
            base *= -1;
        }
    } else {
        geta = 8;
    }

    char* p = ltona((long)n, buf, geta, base);
    print(p);
}

void VesaConsole::setBGColor(const char color)
{
    bg_ = getColor(color);
}

void VesaConsole::setCHColor(const char color)
{
    ch_ = getColor(color);
}

void VesaConsole::putCharacter(char ch)
{

    ch = (unsigned char)ch;

    if ('\n' == ch) {
        newLine();
        return;
    }

    byte* p = font_ + 16 * ch;
    screen.fillPat(pos_x_ * font_x_, pos_y_ * font_y_, font_x_, font_y_, ch_, bg_, p);
    int idx = pos_x_ + pos_y_ * console_x_;
    char_buffer_[idx] = ch;
    palette_buffer_[idx] = ch_;
    nextCursor();
}

void VesaConsole::print(char* str)
{
    for (int i = 0; '\0' != str[i]; i++) {
        putCharacter(str[i]);
    }
}

void VesaConsole::locate(int x, int y)
{
    if (0 > x) x = 0;
    if (0 > y) y = 0;
    if (console_x_ <= x) x = console_x_ - 1;
    if (console_y_ <= y) y = console_y_ - 1;

    pos_x_ = x;
    pos_y_ = y;
}

void VesaConsole::setCursor(int x, int y)
{
    locate(x, y);
}

void VesaConsole::getCursor(int* x, int* y)
{
    *x = pos_x_;
    *y = pos_y_;
}

void VesaConsole::clearScreen()
{
    int len = console_x_ * console_y_;
    for (int i = 0; i < len; i++) char_buffer_[i] = '\0';
    screen.fill(0, 0, xResolution_, yResolution_, bg_);
	RefreshMainWindow();
}

dword VesaConsole::getColor (char c)
{
    if (GP_BLACK > c) c = GP_BLACK;
    if (GP_WHITE < c) c = GP_WHITE;

    return palette[(int)c];
}

void VesaConsole::nextCursor ()
{
    pos_x_++;

    if (pos_x_ >= console_x_) newLine();
}

void VesaConsole::newLine ()
{
    pos_x_ = 0;
    pos_y_++;
    if (pos_y_ < console_y_) return;

    pos_y_ = console_y_ - 1;
#if 1  // Tino: speed up
    int len = console_x_ * console_y_;
    for (int i = console_x_, j = 0, x = 0, y = 0; i < len; i++, j++)
    {
        char c_i = char_buffer_[i], c_j = char_buffer_[j];
        dword p_i = palette_buffer_[i], p_j = palette_buffer_[j];
        if (c_i != '\0')
        {
            if (c_i != c_j || p_i != p_j)
            {
                byte* p = font_ + 16 * c_i;
                screen.fillPat(x * font_x_, y * font_y_, font_x_, font_y_, p_i, bg_, p);
            }
        }
        else if (c_j != '\0')
        {
            screen.fill(x * font_x_, y * font_y_, font_x_, font_y_, bg_);
        }
        char_buffer_[j] = c_i;
        palette_buffer_[j] = p_i;
        x++;
        if (x >= console_x_)
        {
            x = 0;
            y++;
        }
    }
    for (int i = 0, j = pos_y_ * console_x_; i < console_x_; i++, j++)
    {
        char_buffer_[j] = '\0';
    }
#else
    screen.scrollUp(font_y_, pos_y_ * font_y_);
#endif
    screen.fill(0, pos_y_ * font_y_, xResolution_, font_y_, bg_);
}

VesaConsole::VesaScreen::VesaScreen (VesaInfoDetail *info)
{
    vramAddress = info->physBasePtr;
    bytesPerScanLine = info->bytesPerScanLine;
    bitsPerPixel = info->bitsPerPixel;

    selectMethod(info);
}

void VesaConsole::VesaScreen::scrollUp (int y, int h)
{
    byte *dst = vramAddress;
    byte *src = vramAddress + bytesPerScanLine * y;

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < bytesPerScanLine; j++) {
            if (dst[j] != src[j]) dst[j] = src[j];
        }
        dst += bytesPerScanLine;
        src += bytesPerScanLine;
    }
}

void VesaConsole::VesaScreen::fill (int x, int y, int w, int h, dword c)
{
    dword bytesPerPixel = bitsPerPixel/8;
    byte *bits = vramAddress + bytesPerScanLine * y + bytesPerPixel * x;
    byte *temp = bits;

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            (this->*packColor)(temp, c);
            temp += bytesPerPixel;
        }
        bits += bytesPerScanLine;
        temp = bits;
    }
}

void VesaConsole::VesaScreen::fillPat
    (int x, int y, int w, int h, dword c, dword b, byte* p)
{
    dword bytesPerPixel = bitsPerPixel/8;
    byte *bits = vramAddress + bytesPerScanLine * y + bytesPerPixel * x;
    byte *temp = bits;

    int index = 0;
    int k = 0x80;

    for (int i = 0; i < h; i++) {

        for (int j = 0; j < w; j++) {
            if (k & p[index]) {
                (this->*packColor)(temp, c);
            } else {
                (this->*packColor)(temp, b);
            }
            temp += bytesPerPixel;

            k >>= 1;
            if (0 == k) {
                k = 0x80;
                index++;
            }
        }
        bits += bytesPerScanLine;
        temp = bits;

        if (0x80 != k) {
            k = 0x80;
            index++;
        }
    }
}

void VesaConsole::VesaScreen::selectMethod (VesaInfoDetail *info)
{
    packColor = &VesaConsole::VesaScreen::packColor24;
}

void VesaConsole::VesaScreen::packColor8 (byte *bits, dword c)
{
    *bits = ((c>>16) & 0xe0) | ((c>>8) & 0x18) | (c & 0x3);
}

void VesaConsole::VesaScreen::packColor15 (byte *bits, dword c)
{
    *((word*)bits) = ((c>>9) & 0x7c00)
                   | ((c>>6) & 0x03e0)
                   | ((c>>3) & 0x001f);
}

void VesaConsole::VesaScreen::packColor16 (byte *bits, dword c)
{
    *((word*)bits) = ((c>>8) & 0xf800)
                   | ((c>>5) & 0x07e0)
                   | ((c>>3) & 0x001f);
}

void VesaConsole::VesaScreen::packColor24 (byte *bits, dword c)
{
    //    *((dword*)bits) = c;
    bits[0] = (  c      & 0xff );
    bits[1] = ( (c>> 8) & 0xff );
    bits[2] = ( (c>>16) & 0xff );
}