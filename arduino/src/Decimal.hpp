/*
 * power-meter - Arduino Power Meter Modbus Client
 * Copyright 2017,2025  Simon Arlott
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

#ifndef POWER_METER_DECIMAL_HPP
#define POWER_METER_DECIMAL_HPP

#include <stdint.h>
#include <Arduino.h>

class Decimal final: public Printable {
public:
	Decimal();
	Decimal(int8_t coefficient, int8_t exponent);
	Decimal(uint8_t coefficient, int8_t exponent);
	Decimal(int16_t coefficient, int8_t exponent);
	Decimal(uint16_t coefficient, int8_t exponent);
	Decimal(int32_t coefficient, int8_t exponent);
	Decimal(uint32_t coefficient, int8_t exponent);
	virtual ~Decimal();
	bool hasValue() const;
	uint32_t coefficient() const;
	virtual size_t printTo(Print &p) const __attribute__((warn_unused_result));

private:
	bool set_;
	uint32_t coefficient_;
	bool coefficientSigned_; ///< To be able to store both uint32_t without using int64_t
	int8_t exponent_;
};

#endif
