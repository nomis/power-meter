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

#include "Main.hpp"
#include "EthernetNetwork.hpp"

#ifdef ARDUINO_ARCH_ESP8266
# include <ESP8266WiFi.h>
# include <IPAddress.h>
# include <WiFiUdp.h>
extern "C" {
	#include <user_interface.h>
	#include <sntp.h>
}
#endif

#ifdef POWER_METER_HAS_NETWORK
EthernetNetwork ethernetNetwork;

EthernetNetwork::EthernetNetwork() {

}

EthernetNetwork::~EthernetNetwork() {

}

void EthernetNetwork::setConfigurationMode(bool configure) {
	switch (mode) {
	case Mode::RUNNING:
		if (!configure) {
			return;
		} else {
			mode = Mode::CONFIGURE;
		}
		break;

	case Mode::CONFIGURE:
		if (configure) {
			return;
		} else {
			mode = Mode::RUNNING;
		}
		break;

	case Mode::DISABLED:
		if (configure) {
			mode = Mode::CONFIGURE;
		} else {
			mode = Mode::RUNNING;
		}
		break;
	}

	configureNetwork();
}

void EthernetNetwork::configureNetwork() {
#ifdef ARDUINO_ARCH_ESP8266
	static bool apEnabled = false;
	static bool staEnabled = false;
	constexpr int MAXHOSTNAMELEN = 32;
	char hostname[MAXHOSTNAMELEN + 1];

	snprintf(hostname, sizeof(hostname), HOSTNAME, ESP.getChipId());
	wifi_station_set_hostname(hostname);
	output->print("# Hostname = ");
	output->println(hostname);

	if (mode == Mode::CONFIGURE) {
		char ssid[WL_SSID_MAX_LENGTH + 1];
		//uint8_t mode = 0;

		if (staEnabled) {
			output->println("# Disconnecting WiFi STA");
			WiFi.disconnect(true);
			staEnabled = false;
		}

		snprintf(ssid, sizeof(ssid), SSID, ESP.getChipId());

		output->println("# Enabling WiFi AP");
		WiFi.mode(WIFI_AP);
		WiFi.softAP(ssid);
		apEnabled = true;

		//wifi_softap_set_dhcps_offer_option(OFFER_ROUTER, &mode);
	} else {
		if (apEnabled) {
			output->println("# Disabling WiFi AP");
			WiFi.softAPdisconnect(true);
			apEnabled = false;
		}

		sntp_servermode_dhcp(0);

		output->println("# Connecting WiFi STA");
		WiFi.mode(WIFI_STA);

		WiFi.begin(FIXED_WIFI_SSID, FIXED_WIFI_PASSWORD);
		staEnabled = true;
	}
#endif
}

EthernetNetwork::operator bool() const {
	if (mode == Mode::RUNNING) {
#ifdef ARDUINO_ARCH_ESP8266
		static uint8_t lastStatus = 255;
		uint8_t status = WiFi.status();

		if (status != lastStatus) {
			output->print("# WiFi status ");
			output->print(lastStatus);
			output->print(" -> ");
			output->print(status);
			if (status == WL_CONNECTED) {
				output->print(" (");
				output->print(WiFi.localIP());
				output->println(")");
			} else {
				output->println();
			}
			lastStatus = status;
		}
		return (status == WL_CONNECTED);
#endif
	}
	return false;
}

#endif
