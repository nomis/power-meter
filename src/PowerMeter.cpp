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

#include <PowerMeter.hpp>

PowerMeter::PowerMeter() {

}

PowerMeter::~PowerMeter() {

}

void PowerMeter::clearReadings() {
	voltage = Decimal();
	current = Decimal();
	frequency = Decimal();
	activePower = Decimal();
	reactivePower = Decimal();
	apparentPower = Decimal();
	powerFactor = Decimal();

	temperature = Decimal();

	activeEnergy = Decimal();
	reactiveEnergy = Decimal();
}

size_t PowerMeter::printTo(Print &p) const {
	size_t n = 0;
	bool first = true;

	n += p.print("meter: {model: \"");
	n += p.print(model());
	n += p.print("\"");

	if (serialNumber.length() > 0) {
		n += p.print(",serialNumber: \"");
		n += p.print(serialNumber);
		n += p.print("\"");
	}

	n += p.print(",reading: {");

	n += printReading(p, first, "voltage", voltage);
	n += printReading(p, first, "current", current);
	n += printReading(p, first, "frequency", frequency);
	n += printReading(p, first, "activePower", activePower);
	n += printReading(p, first, "reactivePower", reactivePower);
	n += printReading(p, first, "apparentPower", apparentPower);
	n += printReading(p, first, "powerFactor", powerFactor);

	n += printReading(p, first, "temperature", temperature);

	n += printReading(p, first, "activeEnergy", activeEnergy);
	n += printReading(p, first, "reactiveEnergy", reactiveEnergy);

	n += p.print("}}");

	return n;
}

size_t PowerMeter::printReading(Print &p, bool &first, const char *name, const Decimal &value) {
	size_t n = 0;

	if (value.hasValue()) {
		if (first) {
			first = false;
		} else {
			n += p.print(',');
		}

		n += p.print(name);
		n += p.print(": ");
		n += p.print(value);
	}

	return n;
}
