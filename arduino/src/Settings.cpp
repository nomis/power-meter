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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include <EEPROM.h>
#pragma GCC diagnostic pop

#include "Settings.hpp"
#include "Main.hpp"

Settings::Data Settings::data;

void Settings::init() {
#ifdef ARDUINO_ARCH_ESP8266
	EEPROM.begin(sizeof(data));
#endif
	EEPROM.get(0, data);

	output->print("# EEPROM magic = ");
	output->print(data.magic, HEX);
	output->print(", length = ");
	output->print(data.length);

	if (data.magic != EEPROM_MAGIC) {
		output->println("; invalid");
		data.length = 0;
	} else {
		output->println("; valid");
	}

	if (data.length < sizeof(data)) {
		memset((uint8_t *)&data + data.length, 0, sizeof(data) - data.length);
	}

	data.magic = EEPROM_MAGIC;
	data.length = sizeof(data);
}

const char* Settings::readWiFiSSID() {
	data.wifiSSID[sizeof(data.wifiSSID) - 1] = 0;
	return data.wifiSSID;
}

void Settings::writeWiFiSSID(const String &value) {
	value.toCharArray(data.wifiSSID, sizeof(data.wifiSSID));
}

const char* Settings::readWiFiPassphrase() {
	data.wifiPassphrase[sizeof(data.wifiPassphrase) - 1] = 0;
	return data.wifiPassphrase;
}

void Settings::writeWiFiPassphrase(const String &value) {
	value.toCharArray(data.wifiPassphrase, sizeof(data.wifiPassphrase));
}

void Settings::commit() {
	output->println("# EEPROM commit");
	EEPROM.put(0, data);
#ifdef ARDUINO_ARCH_ESP8266
	EEPROM.commit();
#endif
}
