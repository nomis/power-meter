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

#include <Decimal.hpp>

Decimal::Decimal() : set(false) {

}

Decimal::Decimal(int8_t coefficient_, int8_t exponent_)
	: Decimal((int32_t)coefficient_, exponent_) {

}

Decimal::Decimal(uint8_t coefficient_, int8_t exponent_)
	: Decimal((uint32_t)coefficient_, exponent_) {

}

Decimal::Decimal(int16_t coefficient_, int8_t exponent_)
	: Decimal((int32_t)coefficient_, exponent_) {

}

Decimal::Decimal(uint16_t coefficient_, int8_t exponent_)
	: Decimal((uint32_t)coefficient_, exponent_) {

}

Decimal::Decimal(int32_t coefficient_, int8_t exponent_)
	: set(true), coefficient((uint32_t)coefficient_), coefficientSigned(true), exponent(exponent_) {

}

Decimal::Decimal(uint32_t coefficient_, int8_t exponent_)
	: set(true), coefficient(coefficient_), coefficientSigned(false), exponent(exponent_) {

}

Decimal::~Decimal() {

}

bool Decimal::hasValue() const {
	return set;
}

size_t Decimal::printTo(Print &p) const {
	size_t n = 0;

	if (coefficientSigned) {
		n += p.print((int32_t)coefficient);
	} else {
		n += p.print(coefficient);
	}

	if (exponent) {
		n += p.print('e');
		n += p.print(exponent);
	}

	return n;
}
