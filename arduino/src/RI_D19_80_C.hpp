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

#ifndef POWER_METER_RI_D19_80_C_HPP
#define POWER_METER_RI_D19_80_C_HPP

#include <stdint.h>

#include "PowerMeter.hpp"
#include "ModbusMaster.h"
#include "Comms.hpp"

/**
Rayleigh Instruments RI-D19-80-C
230V 5/80A LCD Single Phase Energy modbus – 80A Direct With RS485 Output

Registers (values are Big-endian):
	0x0000 16-bit Voltage in dV
	0x0001 16-bit Current in dA
	0x0002 16-bit Frequency in dHz
	0x0003 16-bit Active Power in W
	0x0004 16-bit Reactive Power in var
	0x0005 16-bit Apparent Power in VA
	0x0006 16-bit Power Factor in d%
	0x0007 32-bit Active Energy (Total) in daW·h
	0x0009 32-bit Active Energy (T1) in daW·h
	0x000B 32-bit Active Energy (T2) in daW·h
	0x000D 32-bit Active Energy (T3) in daW·h
	0x000F 32-bit Active Energy (T4) in daW·h
	0x0011 32-bit Reactive Energy (Total) in daW·h
	0x0013 32-bit Reactive Energy (T1) in daW·h
	0x0015 32-bit Reactive Energy (T2) in daW·h
	0x0017 32-bit Reactive Energy (T3) in daW·h
	0x0019 32-bit Reactive Energy (T4) in daW·h
	0x001B Unused
	0x001C Unused
	0x001D Unused
	0x001E Unused
	0x001F Unused
	0x0020 Unused
	0x0021 8-byte Date/Time in BCD (CC yy mm dd ww HH MM SS)
	0x0025 16-bit Temperature in °C (8-bit signed value in LSB)
	0x0026 16-bit Unknown
	0x0027 6-byte Serial number in Packed BCD (## ## ## ## ## ##)
	0x002A 16-bit Baud rate (1=1200, 2=2400, 3=4800, 4=9600)
	0x002B 16-bit Modbus address
	0x002C 32-bit Password
	0x002E Unknown
	0x002F Unknown
*/
class RI_D19_80_C: public PowerMeter {
public:
	RI_D19_80_C(ModbusMaster &modbus, Stream *io, uint8_t address, Comms &comms);
	virtual ~RI_D19_80_C();
	void setPassword(uint32_t value);
	bool writeActiveEnergy(unsigned int count, uint32_t value1, uint32_t value2, uint32_t value3, uint32_t value4);
	bool writeReactiveEnergy(unsigned int count, uint32_t value1, uint32_t value2, uint32_t value3, uint32_t value4);
	bool resetEnergy();
	bool writeDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t day_of_week, uint8_t hour, uint8_t minute, uint8_t second);
	bool writeBaudRate(unsigned int baudRate);
	bool writeAddress(uint8_t address);
	bool writePassword(uint32_t value);

protected:
	virtual bool readSerialNumber();
	virtual bool readMeasurements();
	virtual String model() const;

	static constexpr uint32_t maximumEnergy = 99999999; // daW·h (6+2 record, 5+1 display)
	static constexpr bool debug = false;

	ModbusMaster &modbus;
	Stream *io;
	uint8_t address;
	Comms &comms;

private:
	bool writeEnergy(uint16_t reg, unsigned int count, uint32_t value1, uint32_t value2, uint32_t value3, uint32_t value4);
	/**
	Transmit password:
		## 28 FE 01 00 02 04 ** ** ** ** [CRC16]

	Success:
		## 28 FE 01 00 01 [CRC16]

	Failure:
		## A8 FE 01 00 02 [CRC16]

	Write registers within 10 seconds of success.
	*/
	bool transmitPassword();

	uint32_t password;
};

#endif
