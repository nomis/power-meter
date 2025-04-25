/*
 * power-meter - Arduino Power Meter Modbus Client
 * Copyright 2025  Simon Arlott
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

#ifndef POWER_METER_PZEM_004T_100A
#define POWER_METER_PZEM_004T_100A

#include <stdint.h>

#include "PowerMeter.hpp"
#include "ModbusMaster.h"

/**
Peacefair PZEM_004T_100A
80-260V 100A Single Phase Energy Meter with RS485 Modbus

Registers (values are 16-bit Big-endian, combined for 32-bit in Little-endian order):
	0x0000 16-bit Voltage in dV
	0x0001 32-bit Current in mA
	0x0003 32-bit Active Power in dW
	0x0005 32-bit Active Energy (Total) in W·h
	0x0007 16-bit Frequency in dHz
	0x0008 16-bit Power Factor in c%
	0x0009 16-bit Alarm status (0xFFFF = active, 0x0000 = inactive)
*/
class PZEM_004T_100A: public PowerMeter {
public:
    PZEM_004T_100A(ModbusMaster &modbus, Stream *io, uint8_t address);
	~PZEM_004T_100A() override;
    void setPassword(uint32_t) {}
	bool resetEnergy();

protected:
    bool readSerialNumber() override;
    bool readMeasurements() override;
	String model() const override;

	ModbusMaster &modbus;
	Stream *io;
	uint8_t address;
    uint8_t success{1};
};

#endif
