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

#ifndef POWER_METER_ETHERNETNETWORK_HPP
#define POWER_METER_ETHERNETNETWORK_HPP

#include <Arduino.h>

#ifdef ARDUINO_ARCH_ESP8266
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-parameter"
# include <ESP8266WebServer.h>
# pragma GCC diagnostic pop
#endif

class EthernetNetwork: public Print {
public:
	EthernetNetwork();
	virtual ~EthernetNetwork();
	virtual size_t write(uint8_t c);
	void loop();
	void setConfigurationMode(bool configure);
	operator bool() const;

protected:
	enum class Mode {
		DISABLED,
		CONFIGURE,
		RUNNING
	};

	void configureNetwork();
	void sendPacket();

	// Elementary charge is about 1.60217×10⁻¹⁹ coulombs
	static constexpr uint32_t ADDRESS = 0xEFC0A0D9; // 239.192.160.217
	static constexpr uint16_t PORT = 16021;
	static constexpr int TTL = 1;

	static constexpr const char *HOSTNAME = "ESP8266-PowerMeter-%08x";
	static constexpr const char *SSID = "🔌 %08x";

#ifdef ARDUINO_AVR_MICRO
	static constexpr size_t MAX_LENGTH = 1;
#else
	static constexpr size_t ETH_DATA_LEN = 1500;
	static constexpr size_t IPV4_HLEN = 20;
	static constexpr size_t UDP_HLEN = 8;
	static constexpr size_t MAX_LENGTH = ETH_DATA_LEN - IPV4_HLEN - UDP_HLEN;
#endif

	Mode mode = Mode::DISABLED;
	char buffer[MAX_LENGTH];
	size_t bufferLength = 0;

private:
#ifdef ARDUINO_ARCH_ESP8266
	static void webServerRootPage();
	static void webServerConfigPage();
	static void webServerSavePage();
	static void webServerResetPage();

	ESP8266WebServer webServer{80};
#endif
};

extern EthernetNetwork ethernetNetwork;

#endif
