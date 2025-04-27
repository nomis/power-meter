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

#include "PZEM_004T_100A.hpp"
#include "Main.hpp"

PZEM_004T_100A::PZEM_004T_100A(ModbusMaster &modbus, Stream *io, uint8_t address)
	: modbus(modbus), io(io), address(address) {
}

PZEM_004T_100A::~PZEM_004T_100A() {

}

String PZEM_004T_100A::model() const {
	return "PZEM-004T-100A";
}

bool PZEM_004T_100A::readSerialNumber() {
#ifdef FIXED_SERIAL_NUMBER
	serialNumber = FIXED_SERIAL_NUMBER;
#endif
	return true;
};

bool PZEM_004T_100A::readMeasurements() {
	uint8_t ret;

	modbus.begin(address, *io);

	ret = modbus.readInputRegisters(0x0000, 9);
	if (ret != ModbusMaster::ku8MBSuccess) {
		success = 1;
		return false;
	}

	voltage = Decimal(modbus.getResponseBuffer(0x0000), -1);
	current = Decimal(
		((uint32_t)modbus.getResponseBuffer(0x0002) << 16)
		| (uint32_t)modbus.getResponseBuffer(0x0001), -3);
	activePower = Decimal(
		((uint32_t)modbus.getResponseBuffer(0x0004) << 16)
		| (uint32_t)modbus.getResponseBuffer(0x0003), -1);
	activeEnergy = Decimal(
		((uint32_t)modbus.getResponseBuffer(0x0006) << 16)
		| (uint32_t)modbus.getResponseBuffer(0x0005), -3);
	frequency = Decimal(modbus.getResponseBuffer(0x0007), -1);
	powerFactor = Decimal(modbus.getResponseBuffer(0x0008), -2);

	/* Ignore the first 20 readings (about 2-3 seconds) to avoid invalid readings on startup */
	if (success < 20) {
		success++;
		return false;
	}

	/*
	 * Invalid readings are reported across all values on startup for a few seconds:
	 * {model: "PZEM-004T-100A",reading: {voltage: 3477.0e-1,current: 1229079690.0e-3,frequency: 42216.0e-1,activePower: 219717939.0e-1,powerFactor: 54580.0e-2,activeEnergy: 3797280931.0}}
	 * 
	 * Incorrect readings are reported on shutdown when the voltage goes away:
	 * meter: {model: "PZEM-004T-100A",reading: {voltage: 2468.0e-1,current: 1788.0e-3,frequency: 499.0e-1,activePower: 2485.0e-1,powerFactor: 56.0e-2,activeEnergy: 2875.0}}
	 * meter: {model: "PZEM-004T-100A",reading: {voltage: 47.0e-1,current: 1761.0e-3,frequency: 500.0e-1,activePower: 103.0e-1,powerFactor: 100.0e-2,activeEnergy: 2875.0}}
	 */
	if (voltage.coefficient() < 800 /* 80.0V */
			|| voltage.coefficient() > 3000 /* 300.0V */
			|| frequency.coefficient() < 250 /* 25.0Hz */
			|| frequency.coefficient() > 750 /* 75.0Hz */
			|| activePower.coefficient() > 500000 /* 50,000.0W */
			|| powerFactor.coefficient() > 10000 /* 100.00% */) {
		return false;
	}

  	return true;
}

bool PZEM_004T_100A::resetEnergy() {
	uint8_t ret;

	modbus.begin(address, *io);

	ret = modbus.resetEnergy();
	if (ret != ModbusMaster::ku8MBSuccess) {
		return false;
	}

	return true;
}
