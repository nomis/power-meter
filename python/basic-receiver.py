#!/usr/bin/env python3
#
# power-meter - Arduino Power Meter Modbus Client
# Copyright 2017  Simon Arlott
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import argparse
import logging
import powermeter

log = logging.getLogger("readings")

def receive_loop(serial_numbers=None, ip4_numbers=None):
	meter = powermeter.PowerMeter(serial_numbers, ip4_numbers)
	for reading in meter.readings:
		log.info(reading)

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description="Power Meter receiver")
	parser.add_argument("-d", "--debug", action="store_const", default=logging.INFO, const=logging.DEBUG, help="enable debug")
	parser.add_argument("-m", "--meter", metavar="SERIAL_NUMBER", type=str, action="append", help="filter power meter by serial number")
	parser.add_argument("-s", "--source", metavar="IP_ADDRESS", type=str, action="append", help="filter power meter by IP address")
	args = parser.parse_args()

	logging.basicConfig(level=args.debug, format="%(asctime)s  %(levelname)5s  %(message)s", datefmt="%F %T")

	receive_loop(args.meter, args.source)
