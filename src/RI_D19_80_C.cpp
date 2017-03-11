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

#include <RI_D19_80_C.hpp>

RI_D19_80_C::RI_D19_80_C(ModbusMaster &modbus_) : modbus(modbus_) {

}

RI_D19_80_C::~RI_D19_80_C() {

}

String RI_D19_80_C::model() const {
	return "RI-D19-80-C";
}

static char bcd2char(uint16_t value) {
	return value < 10 ? ('0' + value) : ('A' + (value - 10));
}

bool RI_D19_80_C::readSerialNumber() {
	constexpr uint8_t len = 3;
	uint8_t ret;

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
	powerFactor = Decimal(modbus.getResponseBuffer(0x0006), -1);
	activeEnergy = Decimal(
		((uint32_t)modbus.getResponseBuffer(0x0007) << 16)
		| (uint32_t)modbus.getResponseBuffer(0x0008), -2);
#if 0
	reactiveEnergy = Decimal(
		((uint32_t)modbus.getResponseBuffer(0x0011) << 16)
		| (uint32_t)modbus.getResponseBuffer(0x0012), -2);
#endif
	temperature = Decimal(modbus.getResponseBuffer(0x0025), 0);

	if (debug) {
		bool first = true;

		// Check if Active Energy (Total) doesn't match Active Energy (T1)
		if (modbus.getResponseBuffer(0x0007) != modbus.getResponseBuffer(0x0009)
				|| modbus.getResponseBuffer(0x0008) != modbus.getResponseBuffer(0x000A)) {
			SerialUSB.print(first ? "# " : "; ");
			first = false;
			SerialUSB.print("0x0007..0x000A");
			for (uint8_t i = 0x0007; i <= 0x000A; i++) {
				if (i > 0x0007) {
					SerialUSB.print(" ");
				}

				SerialUSB.print(modbus.getResponseBuffer(i), HEX);
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
			SerialUSB.print(first ? "# " : "; ");
			first = false;
			SerialUSB.print("0x000B..0x0020 = ");
			for (uint8_t i = zero_start; i <= zero_end; i++) {
				if (i > zero_start) {
					SerialUSB.print(" ");
				}

				SerialUSB.print(modbus.getResponseBuffer(i), HEX);
			}
		}

		// Check for new values of unknown register 0x0026
		if (modbus.getResponseBuffer(0x0026) != 0xF6) {
			SerialUSB.print(first ? "# " : "; ");
			first = false;
			SerialUSB.print("0x0026 = 0x");
			SerialUSB.print(modbus.getResponseBuffer(0x0026), HEX);
		}

		if (!first) {
			SerialUSB.println();
		}
	}

	return true;
}
