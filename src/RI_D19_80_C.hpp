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

#include <stdint.h>

#include <PowerMeter.hpp>
#include <ModbusMaster.h>

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
	0x0021 8-byte Date/Time in BCD (CC yy mm dd uu HH MM SS)
	0x0025 16-bit Temperature in °C
	0x0026 Unknown
	0x0027 6-byte Serial number in Packed BCD (## ## ## ## ## ##)
	0x002A 16-bit Baud rate (1=1200, 2=2400, 3=4800, 4=9600)
	0x002B 16-bit Modbus address
	0x002C 32-bit Password
*/
class RI_D19_80_C: public PowerMeter {
public:
	RI_D19_80_C(ModbusMaster &modbus);
	virtual ~RI_D19_80_C();

protected:
	virtual bool readSerialNumber();
	virtual bool readMeasurements();
	virtual String model() const;

	static constexpr bool debug = true;

	ModbusMaster &modbus;
};
