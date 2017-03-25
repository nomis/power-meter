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
#include "Settings.hpp"

#ifdef ARDUINO_ARCH_ESP8266
# include <EEPROM.h>
# include <ESP8266WiFi.h>
# include <IPAddress.h>
# include <WiFiUdp.h>
extern "C" {
	#include <user_interface.h>
}
#endif

EthernetNetwork ethernetNetwork;

EthernetNetwork::EthernetNetwork() {
#ifdef ARDUINO_ARCH_ESP8266
	webServer.on("/", webServerRootPage);
	webServer.on("/config", webServerConfigPage);
	webServer.on("/save", webServerSavePage);
	webServer.begin();
#endif
}

EthernetNetwork::~EthernetNetwork() {

}

size_t EthernetNetwork::write(uint8_t c) {
	if (c == '\r' || c == '\n') {
		sendPacket();
		return 1;
	}

	if (bufferLength < MAX_LENGTH) {
		buffer[bufferLength++] = c;
		return 1;
	} else {
		return 0;
	}
}

void EthernetNetwork::sendPacket() {
	if (bufferLength > 0) {
#ifdef ARDUINO_ARCH_ESP8266
		WiFiUDP udp;

		udp.beginPacketMulticast(IPAddress(ADDRESS), PORT, WiFi.localIP(), TTL);
		udp.write(buffer, bufferLength);
		udp.endPacket();
#endif
	}

	bufferLength = 0;
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
		uint8_t mode = 0;

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

		wifi_softap_set_dhcps_offer_option(OFFER_ROUTER, &mode);
	} else {
		if (apEnabled) {
			output->println("# Disabling WiFi AP");
			WiFi.softAPdisconnect(true);
			apEnabled = false;
		}

		output->println("# Connecting WiFi STA");
		WiFi.mode(WIFI_STA);
		WiFi.begin(Settings::readWiFiSSID(), Settings::readWiFiPassphrase());
		staEnabled = true;
	}
#endif
}

EthernetNetwork::operator bool() const {
	if (mode == Mode::RUNNING) {
#ifdef ARDUINO_ARCH_ESP8266
		return (WiFi.status() == WL_CONNECTED);
#endif
	}
	return false;
}

void EthernetNetwork::loop() {
#ifdef ARDUINO_ARCH_ESP8266
	webServer.handleClient();
#endif
}

#ifdef ARDUINO_ARCH_ESP8266
void EthernetNetwork::webServerRootPage() {
	unsigned long uptime = millis();
	unsigned long days, hours, minutes, seconds, ms;
	char response[64];

	days = uptime / 86400000;
	uptime %= 86400000;

	hours = uptime / 3600000;
	uptime %= 3600000;

	minutes = uptime / 60000;
	uptime %= 60000;

	seconds = uptime / 1000;
	uptime %= 1000;

	ms = uptime;

	snprintf(response, sizeof(response), "%03ld+%02ld:%02ld:%02ld.%03ld\n", days, hours, minutes, seconds, ms);
	ethernetNetwork.webServer.send(200, "text/plain", response);
}

void EthernetNetwork::webServerConfigPage() {
	String page = "<!DOCTYPE html>"
		"<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head>"
		"<body><form method=\"POST\" action=\"/save\">"
		"SSID: <input type=\"text\" name=\"ssid\" value=\"*\"><br>"
		"Passphrase: <input type=\"text\" name=\"passphrase\" value=\"*\"><br>"
		"<input type=\"submit\"></form></body></html>";

	ethernetNetwork.webServer.send(200, "text/html", page);
}

void EthernetNetwork::webServerSavePage() {
	constexpr ESP8266WebServer &server = ethernetNetwork.webServer;

	String page = "<!DOCTYPE html>"
		"<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head>"
		"<body><p>Settings updated</p></body></html>";

	for (int i = 0; i < server.args(); i++) {
		if (server.argName(i) == "ssid" && server.arg(i) != "*") {
			Settings::writeWiFiSSID(server.arg(i));
		} else if (server.argName(i) == "passphrase" && server.arg(i) != "*") {
			Settings::writeWiFiPassphrase(server.arg(i));
		}
	}
	Settings::commit();

	server.send(200, "text/html", page);

	if (ethernetNetwork.mode == Mode::RUNNING) {
		ethernetNetwork.configureNetwork();
	}
}
#endif
