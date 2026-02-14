/*
 * N64_Controller.h — N64 controller protocol and HID report for 4dapter
 *
 * Copyright (C) 2026 4dapter project
 *
 * N64 protocol and adapter work by Michele Perla (N64-To-USB), Andrew Brown
 * (GameCube–N64 adapter), and Peter Den Hartog (N64 to HID). This version
 * modified for 4dapter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef N64Controller_h
#define N64Controller_h

#define N64_PIN 10
#define N64_PIN_DIR DDRB

// these two macros set arduino pin 10 to input or output, which with an
// external 1K pull-up resistor to the 3.3V rail, is like pulling it high or
// low. These operations translate to 1 op code, which takes 2 cycles
#define N64_HIGH DDRB &= ~B01000000
#define N64_LOW DDRB |= B01000000
#define N64_QUERY (PINB & B01000000)

// 8 bytes of data that we get from the controller
typedef struct state
{
  char stick_x;
  char stick_y;

  // bits: 0, 0, 0, start, y, x, b, a
  unsigned char data1;

  // bits: 1, L, R, Z, Dup, Ddown, Dright, Dleft
  unsigned char data2;
} N64_status_packet;

class N64Controller
{
 public:
  N64Controller();
  void N64_init();
  void translate_N64_data();
  void N64_get_data_from_controller();
  void N64_send_data_request(unsigned char *buffer, char length);
  void print_N64_status();
  void getN64Packet();
  N64_status_packet N64_status;

 private:
  char N64_raw_dump[33]; // 1 received bit per byte
};

#endif
