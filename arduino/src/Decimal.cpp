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

#include "Decimal.hpp"

Decimal::Decimal() : set_(false) {

}

Decimal::Decimal(int8_t coefficient, int8_t exponent)
	: Decimal((int32_t)coefficient, exponent) {

}

Decimal::Decimal(uint8_t coefficient, int8_t exponent)
	: Decimal((uint32_t)coefficient, exponent) {

}

Decimal::Decimal(int16_t coefficient, int8_t exponent)
	: Decimal((int32_t)coefficient, exponent) {

}

Decimal::Decimal(uint16_t coefficient, int8_t exponent)
	: Decimal((uint32_t)coefficient, exponent) {

}

Decimal::Decimal(int32_t coefficient, int8_t exponent)
	: set_(true), coefficient_((uint32_t)coefficient), coefficientSigned_(true), exponent_(exponent) {

}

Decimal::Decimal(uint32_t coefficient, int8_t exponent)
	: set_(true), coefficient_(coefficient), coefficientSigned_(false), exponent_(exponent) {

}

Decimal::~Decimal() {

}

bool Decimal::hasValue() const {
	return set_;
}

uint32_t Decimal::coefficient() const {
	return coefficient_;
}

size_t Decimal::printTo(Print &p) const {
	size_t n = 0;

	if (coefficientSigned_) {
		n += p.print((int32_t)coefficient_);
	} else {
		n += p.print(coefficient_);
	}

	n += p.print(".0");

	if (exponent_) {
		n += p.print('e');
		n += p.print(exponent_);
	}

	return n;
}
