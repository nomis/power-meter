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

#ifndef POWER_METER_SETTINGS_HPP
#define POWER_METER_SETTINGS_HPP

#include <Arduino.h>

class Settings {
public:
	Settings() = delete;
	virtual ~Settings() = delete;
	static void init();
	static const char* readWiFiSSID();
	static void writeWiFiSSID(const String &value);
	static const char* readWiFiPassphrase();
	static void writeWiFiPassphrase(const String &value);
	static void commit();

	static constexpr unsigned int IEEE80211_MAX_SSID_LEN = 32;
	static constexpr unsigned int WPA2_PSK_MAX_PASSPHRASE_LEN = 63;

protected:
	static constexpr uint32_t EEPROM_MAGIC = 0x16021766;

	struct Data {
		uint32_t magic;
		uint16_t length;
		char wifiSSID[IEEE80211_MAX_SSID_LEN + 1];
		char wifiPassphrase[WPA2_PSK_MAX_PASSPHRASE_LEN + 1];
	} __attribute__((packed));

	static Data data;
};

#endif
