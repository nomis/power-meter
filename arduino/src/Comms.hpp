/*
 * power-meter - Arduino Power Meter Modbus Client
 * Copyright 2022  Simon Arlott
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

#ifndef POWER_METER_COMMS_HPP
#define POWER_METER_COMMS_HPP

#include <Arduino.h>
#include <WiFiUdp.h>
#include <aes.hpp>
#include <sha/sha256.h>

#include <array>
#include <list>

struct Data {
	uint32_t timestamp;
	uint8_t data[23];
	uint8_t uptime_s[3];
	uint16_t rtt_16us;
};

constexpr unsigned long MIN_TIME = 1651955510;

class Comms {
public:
	Comms();
	void add(uint32_t timestamp, const std::array<uint8_t,23> &data);
	void transmit();
	void receive();

private:
	static constexpr size_t MAX_DATA = 40;

	WiFiUDP udp;
	uint8_t enc_key_[AES_KEYLEN] = { 0 };
	uint8_t mac_key_[SHA256_HASH_LEN] = { 0 };
	std::list<Data> data_;
	std::array<uint32_t,AES_BLOCKLEN / 4> token_;
	unsigned long tx_micros_ = 0;
	bool sync_time_ = false;
	unsigned long rtt_us_ = 0;
};

#endif
