// povfan: a uC application for reprogramming POV fans
// Copyright (C) 2013 Michael Leibowitz
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "font.h"

#include <stdint.h>
#include <Wire.h>

#define MAXSTRING 16 //kinda a guess
#define MAXSCREENS 4 //guess

#define SEPERATOR F("*************")

#define PROMPTSTATE  0
#define NEWLINESTATE 1
#define READSTATE    2
#define CONFIRMSTATE 3
#define BURNSTATE    4

char buf[MAXSCREENS * MAXSTRING + MAXSCREENS];
uint8_t state = PROMPTSTATE;
uint8_t screen = 0;
uint8_t nextscreen = screen;
uint8_t i= 0;
uint8_t linelen = 0;

static int _putc( char c, FILE *t) {
  Serial.write( c );
  return 1;
}

static void addr(const uint16_t _addr, uint8_t &addr8, uint8_t &devaddr)
{
  if (_addr <= 0xFF) {
    devaddr = 0x50;
    addr8 = _addr;
  } else {
    devaddr = 0x51;
    addr8 = (_addr - 255);
  }
}

static void print_write_ee(const uint16_t addr, const uint8_t val)
{
  printf("0x%04X:\t\t0x%02X", addr, val);
  if (isalnum(val)) {
    printf("\t%c", val);
  }
  printf("\n\r");
}

static void write_ee(const uint16_t _addr, const uint8_t d)
{
  uint8_t dev, addr8;
  addr(_addr, addr8, dev);

  print_write_ee(_addr, d);
  Wire.beginTransmission(dev);
  Wire.write(addr8);
  Wire.write(d);
  Wire.endTransmission();
  delay(100);
}

static uint8_t find_start_of_screen(const uint8_t _screen)
{
  uint8_t ret= 0;
  for (uint8_t foo = 0; foo < _screen; foo++) {
    while(buf[ret] != '\n') ret++;
    ret++;
  }
  return ret;
}

static uint8_t find_end_of_screen(const uint8_t _screen)
{
  uint8_t foo = find_start_of_screen(_screen);
  while (buf[foo] != '\n') foo++;
  return foo;
}

static void prompt_state()
{
  Serial.println(F("\n\nEnter the screens you want.  To make a new screen, hit return"));

  buf[i] = '\0';
  i= 0;
  screen= 0;

  state = NEWLINESTATE;
}

static void newline_state()
{
  Serial.print(screen);
  Serial.print(F("> "));

  linelen = 0;

  state = READSTATE;
}

static void read_state()
{
  while(!Serial.available());
  char c = Serial.read();
  switch(c) {
  case 0xA:
  case 0xD:
    Serial.println();
    buf[i] = '\n';
    if ((linelen == 0) || (screen == MAXSCREENS)) {
      // if you just hit return or have the maximum number of screens, you're done
      buf[i+1] = '\0';

      state = CONFIRMSTATE;
    } else {
      i++;
      screen++;

      state = NEWLINESTATE;
    }
    break;


  case 0x8:  //backspace
  case 0x7f: //delete
    if (linelen > 0) {
      Serial.print(F("\x1B[1D\x1B[K")); //move cursor left and clear line
      buf[i] = '\0';
      i--;
      linelen--;
    }
    break;


  default:
    Serial.print(c);

    buf[i]= c;
    i++;
    linelen++;
    if (linelen == MAXSTRING) {
      Serial.println();

      buf[i] = '\n';
      screen++;

      state = NEWLINESTATE;
    }
    break;
  }
}

static void confirm_state()
{
  Serial.println(F("These are the lines I think you mean:"));
  Serial.println(SEPERATOR);
  for (i = 0; buf[i]; i++) {
    const char c = buf[i];
    if (c == '\n') {
      Serial.println();
    } else {
      Serial.print(c);
    }
  }
  Serial.println(SEPERATOR);

  Serial.println(F("Is this what you want to program?"));
  Serial.print(F("y/N> "));

  while(!Serial.available());
  char c = Serial.read();
  if (c == 'y' || c == 'Y') {
    Serial.println(c);

    state = BURNSTATE;
  } else {
    state = PROMPTSTATE;
  }
}

static void burn_state()
{
  uint16_t addr = 0; //current address in eeprom

  Serial.println(F("burning to eeprom...."));
  write_ee(addr, screen);
  addr++;

  for (uint8_t s = 0; s < screen; s++) {
    const uint8_t start = find_start_of_screen(s);
    const uint8_t end = find_end_of_screen(s);

    write_ee(addr, end - start); //length of screen
    addr++;

    //screens are printed right to left
    for (int8_t pos = end-1; pos >= start; pos--) {
      const uint16_t c = (buf[pos] - 0x20)*5;  //font array starts at
      //space (0x20) and are 5
      //chars
      //we print right to left by column, too;
      for (int8_t col = 4; col >= 0; col--) {
        const uint8_t fontbyte = pgm_read_byte_near(font_5x7 + c + col);
        write_ee(addr, ~fontbyte);
        addr++;
      }
    }
  }
  Serial.println(F("done!"));
  state = PROMPTSTATE;
}

void setup()
{
  Serial.begin(9600);
  Serial.flush();
  fdevopen(&_putc, 0);
  Wire.begin();
}

void loop()
{
  switch(state) {

  case PROMPTSTATE:
    prompt_state();
    break;

  case NEWLINESTATE:
    newline_state();
    break;

  case READSTATE:
    read_state();
    break;

  case CONFIRMSTATE:
    confirm_state();
    break;

  case BURNSTATE:
    burn_state();
    break;
  }
}

//
// Editor modelines  -  http://www.wireshark.org/tools/modelines.html
//
// Local variables:
// mode: c++
// c-basic-offset: 2
// tab-width: 2
// indent-tabs-mode: nil
// End:
//
// vi: set shiftwidth=2 tabstop=2 expandtab:
// :indentSize=2:tabSize=2:noTabs=true:
//
