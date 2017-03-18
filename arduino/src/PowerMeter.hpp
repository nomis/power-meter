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

#ifndef POWER_METER_POWERMETER_HPP
#define POWER_METER_POWERMETER_HPP

#include <Arduino.h>

#include <Decimal.hpp>

class PowerMeter: public Printable {
public:
	PowerMeter();
	virtual ~PowerMeter();
	bool read();
	virtual size_t printTo(Print &p) const __attribute__((warn_unused_result));

protected:
	void clearMeasurements();
	virtual bool readSerialNumber() = 0;
	virtual bool readMeasurements() = 0;
	virtual String model() const = 0;

	String serialNumber;

	// Gauge values
	Decimal voltage; ///< V
	Decimal current; ///< A
	Decimal frequency; ///< Hz
	Decimal activePower; ///< W
	Decimal reactivePower; ///< var
	Decimal apparentPower; ///< VA
	Decimal powerFactor; ///< %

	Decimal temperature; ///< °C

	// Counter values
	Decimal activeEnergy; ///< kW·h
	Decimal reactiveEnergy; ///< kW·h

private:
	static size_t printReading(Print &p, bool &first, const char *name, const Decimal &value) __attribute__((warn_unused_result));
};

#endif
