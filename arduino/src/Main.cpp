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

#include <ModbusMaster.h>

#include <uuid/common.h>

#include "Main.hpp"
#include "EthernetNetwork.hpp"
#include "RI_D19_80_C.hpp"
#include "Comms.hpp"

ModbusMaster modbus;
Comms comms;
RI_D19_80_C meter(modbus, input, METER_ADDRESS, comms);

static void enableTx() {
	digitalWrite(RE_PIN, HIGH);
	digitalWrite(DE_PIN, HIGH);
	delay(INTER_FRAME_MILLIS);
}

static void disableTx() {
	delay(INTER_FRAME_MILLIS);
	digitalWrite(DE_PIN, LOW);
	digitalWrite(RE_PIN, LOW);
}

static void logTransmit(const uint8_t *data, size_t length) {
	output->print(F("# TX"));
	for (size_t i = 0; i < length; i++) {
		output->print(' ');
		output->print(data[i], HEX);
	}
	output->println();
}

static void logReceive(const uint8_t *data, size_t length, uint8_t status) {
	output->print(F("# RX"));
	for (size_t i = 0; i < length; i++) {
		output->print(' ');
		output->print(data[i], HEX);
	}
	output->print(F(" ("));
	output->print(status, HEX);
	output->println(')');
}

static void indicateStatus(bool success) {
	if (LED_PIN >= 0) {
		digitalWrite(LED_PIN, success ? HIGH : LOW);
	}
}

void setup() {
	if (LED_PIN >= 0) {
		pinMode(LED_PIN, OUTPUT);
	}

	if (CONFIGURE_PIN >= 0) {
		pinMode(CONFIGURE_PIN, INPUT_PULLUP);
	}

	pinMode(DE_PIN, OUTPUT);
	digitalWrite(DE_PIN, LOW);
	pinMode(RE_PIN, OUTPUT);
	digitalWrite(RE_PIN, LOW);

	modbus.preTransmission(enableTx);
	modbus.postTransmission(disableTx);
	if (LOG_MESSAGES) {
		modbus.logTransmit(logTransmit);
		modbus.logReceive(logReceive);
	}

	input->begin(INPUT_BAUD_RATE);
	output->begin(OUTPUT_BAUD_RATE);

#ifdef ARDUINO_ESP8266_ESP12
	output->println();
#endif

#ifdef POWER_METER_HAS_NETWORK
	ethernetNetwork.setConfigurationMode(false);
#endif
}

void loop() {
	static unsigned long last_millis = millis() - 1000;
	unsigned long now = millis();
	static struct timeval last_tv = { 0, 0 };
	struct timeval tv;

	uuid::loop();

	if (gettimeofday(&tv, nullptr) != 0) {
		tv.tv_sec = 0;
	}

	if (tv.tv_sec > MIN_TIME) {
		if (tv.tv_sec > last_tv.tv_sec) {
			output->print(F("# Read "));
			output->print(tv.tv_sec);
			output->print(' ');
			output->print(tv.tv_usec);
			output->print(F(" ["));

			unsigned long start = micros();
			bool ok = meter.read();
			output->print(micros() - start);
			output->println(']');

			if (ok) {
				output->println(meter);
				indicateStatus(true);
			}

			if (ethernetNetwork) {
				comms.transmit();
				last_millis = now;
			}

			last_tv.tv_sec = tv.tv_sec;
		}
	} else if (now - last_millis >= 1000) {
		if (ethernetNetwork) {
			comms.transmit();
			last_millis = now;
		}
	}

	if (ethernetNetwork) {
		comms.receive();
	}
}

bool resetMeter(uint32_t password) {
	meter.setPassword(password);
	return meter.resetEnergy();
}
