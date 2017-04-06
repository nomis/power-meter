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

#include "Settings.hpp"
#include "Main.hpp"

#ifdef POWER_METER_HAS_NETWORK
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include <EEPROM.h>
#pragma GCC diagnostic pop

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

const char* Settings::readWiFiSSID(unsigned int id) {
	if (id >= MAX_NETWORKS) {
		return "";
	}

	data.networks[id].content.wifiSSID[sizeof(data.networks[id].content.wifiSSID) - 1] = 0;
	return data.networks[id].content.wifiSSID;
}

void Settings::writeWiFiSSID(unsigned int id, const String &value) {
	if (id < MAX_NETWORKS) {
		value.toCharArray(data.networks[id].content.wifiSSID, sizeof(data.networks[id].content.wifiSSID));
	}
}

const char* Settings::readWiFiPassphrase(unsigned int id) {
	if (id >= MAX_NETWORKS) {
		return "";
	}

	data.networks[id].content.wifiPassphrase[sizeof(data.networks[id].content.wifiPassphrase) - 1] = 0;
	return data.networks[id].content.wifiPassphrase;
}

void Settings::writeWiFiPassphrase(unsigned int id, const String &value) {
	if (id < MAX_NETWORKS) {
		value.toCharArray(data.networks[id].content.wifiPassphrase, sizeof(data.networks[id].content.wifiPassphrase));
	}
}

const char* Settings::readNTPHostname(unsigned int id) {
	if (id >= MAX_NETWORKS) {
		return "";
	}

	data.networks[id].content.ntpHostname[sizeof(data.networks[id].content.ntpHostname) - 1] = 0;
	return data.networks[id].content.ntpHostname;
}

void Settings::writeNTPHostname(unsigned int id, const String &value) {
	if (id < MAX_NETWORKS) {
		value.toCharArray(data.networks[id].content.ntpHostname, sizeof(data.networks[id].content.ntpHostname));
	}
}

void Settings::commit() {
	output->println("# EEPROM commit");
	EEPROM.put(0, data);
#ifdef ARDUINO_ARCH_ESP8266
	EEPROM.commit();
#endif
}

#endif
