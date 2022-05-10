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
 *
 * NTP client code (was public domain):
 *   created 4 Sep 2010
 *   by Michael Margolis
 *   modified 9 Apr 2012
 *   by Tom Igoe
 *   updated for the ESP8266 12 Apr 2015
 *   by Ivan Grokhotkov
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

#ifdef POWER_METER_HAS_NETWORK
EthernetNetwork ethernetNetwork;

EthernetNetwork::EthernetNetwork() {
	ntpSocket.begin(NTP_PORT);

#ifdef ARDUINO_ARCH_ESP8266
	webServer.on("/", webServerRootPage);
	webServer.on("/config", webServerConfigPage);
	webServer.on("/save", webServerSavePage);
	webServer.on("/reset", webServerResetPage);
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
		int ret;

		ret = udp.beginPacketMulticast(
			IPAddress(ADDRESS[0], ADDRESS[1], ADDRESS[2], ADDRESS[3]),
			PORT, WiFi.localIP(), TTL);
		if (ret == 1) {
			size_t len = udp.write(buffer, bufferLength);

			if (len == bufferLength) {
				ret = udp.endPacket();
			}
		}
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

		output->println("# Connecting WiFi STA");
		WiFi.mode(WIFI_STA);

		unsigned int n = WiFi.scanNetworks();
		output->print("# WiFi scan found ");
		output->print(n);
		output->println(" networks");

		for (unsigned int i = 0; i < n; i++) {
			bool found = false;

			for (unsigned int j = 0; j < Settings::MAX_NETWORKS; j++) {
				if (WiFi.SSID(i) == Settings::readWiFiSSID(j)) {
					found = true;

					output->print("# Found known network ");
					output->print(WiFi.SSID(i));
					output->print(" (");
					output->print(WiFi.RSSI(i));
					output->println(")");

					if (!staEnabled) {
						output->print("# Connecting to network ");
						output->println(WiFi.SSID(i));

						ntpHostname = Settings::readNTPHostname(j);

						WiFi.begin(Settings::readWiFiSSID(j), Settings::readWiFiPassphrase(j));
						staEnabled = true;
					}

					break;
				}
			}

			if (!found) {
				output->print("# Unknown network ");
				output->print(WiFi.SSID(i));
				output->print(" (");
				output->print(WiFi.RSSI(i));
				output->println(")");
			}
		}
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

void EthernetNetwork::loop() {
	ntpMillis();

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
		"<body><form method=\"POST\" action=\"/save\">";

	for (unsigned int id = 0; id < Settings::MAX_NETWORKS; id++) {
		page += "SSID ";
		page += id;
		page += ": <input type=\"text\" name=\"ssid_";
		page += id;
		page += "\" value=\"";
		page += Settings::readWiFiSSID(id);
		page += "\"><br>";
		page += "Passphrase ";
		page += id;
		page += ": <input type=\"text\" name=\"passphrase_";
		page += id;
		page += "\" value=\"";
		if (strlen(Settings::readWiFiSSID(id)) > 0) {
			page += "*";
		}
		page += "\"><br>";
		page += "NTP Hostname ";
		page += id;
		page += ": <input type=\"text\" name=\"ntphostname_";
		page += id;
		page += "\" value=\"";
		page += Settings::readNTPHostname(id);
		page += "\"><hr>";
	}

	page += "<input type=\"submit\" value=\"Save\"></form></body></html>";

	ethernetNetwork.webServer.send(200, "text/html", page);
}

void EthernetNetwork::webServerSavePage() {
	constexpr ESP8266WebServer &server = ethernetNetwork.webServer;

	String page = "<!DOCTYPE html>"
		"<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head>"
		"<body><p>Settings updated</p></body></html>";

	for (unsigned int id = 0; id < Settings::MAX_NETWORKS; id++) {
		String argName;
		String argValue;

 		argName = "ssid_";
		argName += id;
		argValue = server.arg(argName);
		Settings::writeWiFiSSID(id, argValue);

		if (argValue == "") {
			Settings::writeWiFiPassphrase(id, "");
			Settings::writeNTPHostname(id, "");
		} else {
			argName = "passphrase_";
			argName += id;
			argValue = server.arg(argName);
			if (argValue != "*") {
				Settings::writeWiFiPassphrase(id, argValue);
			}

			argName = "ntphostname_";
			argName += id;
			argValue = server.arg(argName);
			Settings::writeNTPHostname(id, argValue);
		}
	}
	Settings::commit();

	server.send(200, "text/html", page);
}

void EthernetNetwork::webServerResetPage() {
	constexpr ESP8266WebServer &server = ethernetNetwork.webServer;

	for (int i = 0; i < server.args(); i++) {
		if (server.argName(i) == "password") {
			String page = "<!DOCTYPE html>"
				"<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head>"
				"<body><p>Meter reset ";

			if (resetMeter((uint32_t)server.arg(i).toInt())) {
				page += "successful";
			} else {
				page += "failed";
			}

			page += "</p></body></html>";
			ethernetNetwork.webServer.send(200, "text/html", page);
			return;
		}
	}

	String page = "<!DOCTYPE html>"
		"<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head>"
		"<body><form method=\"POST\" action=\"/reset\">"
		"Password: <input type=\"number\" name=\"password\" min=\"0\" max=\"4294967295\"><br>"
		"<input type=\"submit\"></form></body></html>";

	ethernetNetwork.webServer.send(200, "text/html", page);
}
#endif

bool EthernetNetwork::isTimeValid() {
	if (!ntpValid) {
		ntpMillis();
	}
	return ntpValid;
}

unsigned long EthernetNetwork::ntpMillis() {
	static constexpr int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
	static unsigned long lastQuery;
	static unsigned long offset = 0;
	static bool ntpStart = false;
	unsigned long interval = ntpValid ? NTP_VALID_INTERVAL : NTP_START_INTERVAL;

	if (!ntpStart || millis() - lastQuery >= (1000UL << interval)) {
		if (WiFi.status() == WL_CONNECTED) {
			const char *serverName = ntpHostname;
			IPAddress serverAddress;

			if (strlen(ntpHostname) == 0) {
				ntpHostname = NTP_DEFAULT_HOSTNAME;
			}

			if (WiFi.hostByName(serverName, serverAddress)) {
				uint8_t packetBuffer[NTP_PACKET_SIZE];

				memset(packetBuffer, 0, NTP_PACKET_SIZE);

				// Initialize values needed to form NTP request
				// https://en.wikipedia.org/wiki/Network_Time_Protocol
				packetBuffer[0] = 0b11100011;         // LI, Version, Mode
				packetBuffer[1] = 0;                  // Stratum, or type of clock
				packetBuffer[2] = NTP_VALID_INTERVAL; // Polling Interval
				packetBuffer[3] = 0xEC;               // Peer Clock Precision
				// 8 bytes of zero for Root Delay & Root Dispersion
				packetBuffer[12] = 49;
				packetBuffer[13] = 0x4E;
				packetBuffer[14] = 49;
				packetBuffer[15] = 52;

				ntpSocket.beginPacket(serverAddress, NTP_PORT);
				ntpSocket.write(packetBuffer, NTP_PACKET_SIZE);
				ntpSocket.endPacket();

				lastQuery = millis();

				while (millis() - lastQuery <= NTP_TIMEOUT) {
					if (ntpSocket.parsePacket()) {
						memset(packetBuffer, 0, NTP_PACKET_SIZE);
						if (ntpSocket.read(packetBuffer, NTP_PACKET_SIZE) == NTP_PACKET_SIZE) {
							unsigned long now = millis();
							uint32_t fraction = word(packetBuffer[44], packetBuffer[45]) << 16 | word(packetBuffer[46], packetBuffer[47]);

							fraction /= 4294967;
							fraction += (now - lastQuery) / 2;
							fraction %= 1000;
							offset = fraction - now;

							ntpValid = true;
						}
						break;
					}
					yield();
				}
			} else {
				lastQuery = millis();
			}

			ntpStart = true;
		}
	}

	return millis() + offset;
}

#endif
