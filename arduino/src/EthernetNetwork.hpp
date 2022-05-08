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
#include "Main.hpp"

#ifdef POWER_METER_HAS_NETWORK
#include <WiFiUdp.h>

class EthernetNetwork {
public:
	EthernetNetwork();
	virtual ~EthernetNetwork();
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
	static constexpr uint16_t PORT = 16021;

	static constexpr const char *HOSTNAME = "ESP8266-PowerMeter-%08x";
	static constexpr const char *SSID = "🔌 %08x";

	Mode mode = Mode::DISABLED;
};

extern EthernetNetwork ethernetNetwork;
#endif

#endif
