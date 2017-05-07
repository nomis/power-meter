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

#include "RI_D19_80_C.hpp"
#include "Main.hpp"

RI_D19_80_C::RI_D19_80_C(ModbusMaster &modbus, Stream *io, uint8_t address)
	: modbus(modbus), io(io), address(address) {
}

RI_D19_80_C::~RI_D19_80_C() {

}

String RI_D19_80_C::model() const {
	return "RI-D19-80-C";
}

static char bcd2char(uint16_t value) {
	return value < 10 ? ('0' + value) : ('A' + (value - 10));
}

static uint16_t dec2bcd(uint16_t value) {
	uint16_t tmp = 0;

	tmp |= (value % 10);
	value /= 10;
	tmp |= (value % 10) << 4;
	value /= 10;
	tmp |= (value % 10) << 8;
	value /= 10;
	tmp |= (value % 10) << 12;

	return tmp;
}

bool RI_D19_80_C::readSerialNumber() {
	constexpr uint8_t len = 3;
	uint8_t ret;

	modbus.begin(address, *io);

	ret = modbus.readHoldingRegisters(0x0027, len);
	if (ret != ModbusMaster::ku8MBSuccess) {
		return false;
	}

	for (uint8_t i = 0; i < len; i++) {
		uint16_t value = modbus.getResponseBuffer(i);

		serialNumber += bcd2char((value >> 12) & 0xF);
		serialNumber += bcd2char((value >> 8) & 0xF);
		serialNumber += bcd2char((value >> 4) & 0xF);
		serialNumber += bcd2char(value & 0xF);
	}

	return true;
}

bool RI_D19_80_C::readMeasurements() {
	uint8_t ret;

	modbus.begin(address, *io);

	ret = modbus.readHoldingRegisters(0x0000, debug ? 0x0027 : 0x0026);
	if (ret != ModbusMaster::ku8MBSuccess) {
		return false;
	}

	voltage = Decimal(modbus.getResponseBuffer(0x0000), -1);
	current = Decimal(modbus.getResponseBuffer(0x0001), -1);
	frequency = Decimal(modbus.getResponseBuffer(0x0002), -1);
	activePower = Decimal(modbus.getResponseBuffer(0x0003), 0);
	reactivePower = Decimal(modbus.getResponseBuffer(0x0004), 0);
	apparentPower = Decimal(modbus.getResponseBuffer(0x0005), 0);
	powerFactor = Decimal((int16_t)modbus.getResponseBuffer(0x0006), -1);
	activeEnergy = Decimal(
		((uint32_t)modbus.getResponseBuffer(0x0007) << 16)
		| (uint32_t)modbus.getResponseBuffer(0x0008), -2);
	reactiveEnergy = Decimal(
		((uint32_t)modbus.getResponseBuffer(0x0011) << 16)
		| (uint32_t)modbus.getResponseBuffer(0x0012), -2);
	temperature = Decimal((int8_t)modbus.getResponseBuffer(0x0025), 0);

	if (debug) {
		bool first = true;

		// Check if Active Energy (Total) doesn't match Active Energy (T1)
		if (modbus.getResponseBuffer(0x0007) != modbus.getResponseBuffer(0x0009)
				|| modbus.getResponseBuffer(0x0008) != modbus.getResponseBuffer(0x000A)) {
			output->print(first ? "# " : "; ");
			first = false;
			output->print("0x0007..0x000A = ");
			for (uint8_t i = 0x0007; i <= 0x000A; i++) {
				if (i > 0x0007) {
					output->print(" ");
				}

				output->print(modbus.getResponseBuffer(i), HEX);
			}
		}

		// Check if any of the registers 0x000B..0x0020 are ever non-zero
		constexpr uint8_t zero_start = 0x000B;
		constexpr uint8_t zero_end = 0x0020;
		bool all_zeros = true;

		for (uint8_t i = zero_start; i <= zero_end; i++) {
			all_zeros &= (modbus.getResponseBuffer(i) == 0);
		}

		if (!all_zeros) {
			output->print(first ? "# " : "; ");
			first = false;
			output->print("0x000B..0x0020 = ");
			for (uint8_t i = zero_start; i <= zero_end; i++) {
				if (i > zero_start) {
					output->print(" ");
				}

				output->print(modbus.getResponseBuffer(i), HEX);
			}
		}

		// Check for new values of unknown register 0x0026
		// (this is probably a software version)
		uint8_t unknown = modbus.getResponseBuffer(0x0026);
		if (unknown != 0xF6 && unknown != 0xFB) {
			output->print(first ? "# " : "; ");
			first = false;
			output->print("0x0026 = 0x");
			output->print(modbus.getResponseBuffer(0x0026), HEX);
		}

		ret = modbus.readHoldingRegisters(0x002E, 2);
		if (ret == ModbusMaster::ku8MBSuccess) {
			output->print(first ? "# " : "; ");
			first = false;
			output->print("0x002E..0x002F = ");

			for (uint8_t i = 0; i <= 1; i++) {
				if (i > 0) {
					output->print(" ");
				}

				output->print(modbus.getResponseBuffer(i), HEX);
			}
		}

		if (!first) {
			output->println();
		}
	}

	return true;
}

void RI_D19_80_C::setPassword(uint32_t value) {
	password = value;
}

bool RI_D19_80_C::transmitPassword() {
	uint8_t ret;

	modbus.begin(address, *io);

	ret = modbus.writePassword(password);
	if (ret != ModbusMaster::ku8MBSuccess) {
		return false;
	}

	if (modbus.getResponseBuffer(0) != 0xFE01
			|| modbus.getResponseBuffer(1) != 0x0001) {
		return false;
	}

	return true;
}

bool RI_D19_80_C::writeActiveEnergy(unsigned int count, uint32_t value1, uint32_t value2, uint32_t value3, uint32_t value4) {
	return writeEnergy(0x0007, count, value1, value2, value3, value4);
}

bool RI_D19_80_C::writeReactiveEnergy(unsigned int count, uint32_t value1, uint32_t value2, uint32_t value3, uint32_t value4) {
	return writeEnergy(0x0011, count, value1, value2, value3, value4);
}

bool RI_D19_80_C::writeEnergy(uint16_t reg, unsigned int count, uint32_t value1, uint32_t value2, uint32_t value3, uint32_t value4) {
	uint8_t ret;

	if (!transmitPassword()) {
		return false;
	}

	modbus.begin(address, *io);
	modbus.beginTransmission(reg);

	// Energy (Total) is automatically calculated
	modbus.send((uint16_t)0);
	modbus.send((uint16_t)0);

	if (count >= 1) {
		value1 %= maximumEnergy;
		modbus.send((uint16_t)(value1 >> 16));
		modbus.send((uint16_t)value1);
	}
	if (count >= 2) {
		value2 %= maximumEnergy;
		modbus.send((uint16_t)(value2 >> 16));
		modbus.send((uint16_t)value2);
	}
	if (count >= 3) {
		value3 %= maximumEnergy;
		modbus.send((uint16_t)(value3 >> 16));
		modbus.send((uint16_t)value3);
	}
	if (count >= 4) {
		value4 %= maximumEnergy;
		modbus.send((uint16_t)(value4 >> 16));
		modbus.send((uint16_t)value4);
	}

	ret = modbus.writeMultipleRegisters();
	if (ret != ModbusMaster::ku8MBSuccess) {
		return false;
	}

	return true;
}

bool RI_D19_80_C::writeDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t day_of_week, uint8_t hour, uint8_t minute, uint8_t second) {
	uint8_t ret;

	if (year > 9999 || month < 1 || month > 12 || day < 1 || day > 31 || day_of_week > 6 || hour > 23 || minute > 59 || second > 59) {
		return false;
	}

	if (!transmitPassword()) {
		return false;
	}

	modbus.begin(address, *io);
	modbus.beginTransmission(0x0021);

	modbus.send(dec2bcd(year));
	modbus.send((uint16_t)((dec2bcd(month) << 8) | dec2bcd(day)));
	modbus.send((uint16_t)((dec2bcd(day_of_week) << 8) | dec2bcd(hour)));
	modbus.send((uint16_t)((dec2bcd(minute) << 8) | dec2bcd(second)));

	ret = modbus.writeMultipleRegisters();
	if (ret != ModbusMaster::ku8MBSuccess) {
		return false;
	}

	return true;
}

bool RI_D19_80_C::writeBaudRate(unsigned int baudRate) {
	uint8_t ret;

	switch (baudRate) {
		case 1200:
			baudRate = 1;
			break;
		case 2400:
			baudRate = 2;
			break;
		case 4800:
			baudRate = 3;
			break;
		case 9600:
			baudRate = 4;
			break;
		default:
			return false;
	}

	if (!transmitPassword()) {
		return false;
	}

	modbus.begin(address, *io);
	modbus.beginTransmission(0x002A);
	modbus.send((uint16_t)baudRate);

	ret = modbus.writeMultipleRegisters();
	if (ret != ModbusMaster::ku8MBSuccess) {
		return false;
	}

	return true;
}

bool RI_D19_80_C::writeAddress(uint8_t address) {
	uint8_t ret;

	if (address > 247) {
		return false;
	}

	if (!transmitPassword()) {
		return false;
	}

	modbus.begin(address, *io);
	modbus.beginTransmission(0x002B);
	modbus.send((uint16_t)address);

	ret = modbus.writeMultipleRegisters();
	if (ret != ModbusMaster::ku8MBSuccess) {
		return false;
	}

	return true;
}

bool RI_D19_80_C::writePassword(uint32_t value) {
	uint8_t ret;

	if (!transmitPassword()) {
		return false;
	}

	modbus.begin(address, *io);
	modbus.beginTransmission(0x002C);
	modbus.send((uint16_t)(value >> 16));
	modbus.send((uint16_t)value);

	ret = modbus.writeMultipleRegisters();
	if (ret != ModbusMaster::ku8MBSuccess) {
		return false;
	}

	setPassword(value);
	return true;
}
