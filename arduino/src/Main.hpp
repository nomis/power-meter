/*
 * power-meter - Arduino Power Meter Modbus Client
 * Copyright 2017  Simon Arlott
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef POWER_METER_MAIN_HPP
#define POWER_METER_MAIN_HPP

#include <stdint.h>
#include <Arduino.h>

#ifdef ARDUINO_ARCH_ESP8266
# define POWER_METER_HAS_NETWORK
#endif

// Output
#ifdef ARDUINO_AVR_MICRO
constexpr int LED_PIN = 13;
constexpr int CONFIGURE_PIN = -1;
constexpr auto *output = &SerialUSB;
constexpr unsigned long OUTPUT_BAUD_RATE = 115200;
#endif

#ifdef ARDUINO_ESP8266_ESP12
constexpr int LED_PIN = -1;
constexpr int CONFIGURE_PIN = 14;
constexpr auto *output = &Serial1;
constexpr unsigned long OUTPUT_BAUD_RATE = 115200;
#endif

// RS485
#ifdef ARDUINO_AVR_MICRO
constexpr int DE_PIN = 4;
constexpr int RE_PIN = 5;
constexpr auto *input = &Serial1;
#endif

#ifdef ARDUINO_ESP8266_ESP12
constexpr int DE_PIN = 4;
constexpr int RE_PIN = 5;
constexpr auto *input = &Serial;
#endif

// Modbus
constexpr unsigned long INPUT_BAUD_RATE = 9600;
constexpr uint8_t METER_ADDRESS = 0x01;
constexpr bool LOG_MESSAGES = false;

constexpr float INTER_FRAME_CHARS = 3.5;
constexpr unsigned int CHAR_BITS = 10; // 8N1
constexpr unsigned int INTER_FRAME_BITS = INTER_FRAME_CHARS * CHAR_BITS;
constexpr unsigned int MS_PER_S = 1000;
constexpr unsigned long INTER_FRAME_MILLIS = (INTER_FRAME_BITS * MS_PER_S / INPUT_BAUD_RATE) + 1;

bool resetMeter(uint32_t password);

#endif
