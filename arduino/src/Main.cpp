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

#include <Main.hpp>
#include <ModbusMaster.h>
#include <RI_D19_80_C.hpp>

ModbusMaster modbus;
RI_D19_80_C meter(modbus);

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
	output->print("# TX");
	for (size_t i = 0; i < length; i++) {
		output->print(" ");
		output->print(data[i], HEX);
	}
	output->println();
}

static void logReceive(const uint8_t *data, size_t length, uint8_t status) {
	output->print("# RX");
	for (size_t i = 0; i < length; i++) {
		output->print(" ");
		output->print(data[i], HEX);
	}
	output->print(" (");
	output->print(status, HEX);
	output->println(")");
}

void setup() {
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW);

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
	modbus.begin(METER_ADDRESS, *input);

	output->begin(OUTPUT_BAUD_RATE);
}

void loop() {
	unsigned long start = millis();

	if (*output) {
		if (meter.read()) {
			output->println(meter);
			digitalWrite(LED_PIN, HIGH);

			constexpr long wait = 1000;
			unsigned long duration = millis() - start;
			if (duration < wait) {
				delay(wait - duration);
			}
		} else {
			digitalWrite(LED_PIN, LOW);
			delay(100);
		}
	} else {
		digitalWrite(LED_PIN, LOW);
	}
}