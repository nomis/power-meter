#!/usr/bin/env python3
#
# power-meter - Arduino Power Meter Modbus Client
# Copyright 2020  Simon Arlott
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
import collections
import logging
import os
import powermeter
import re
import socket
import sys
import systemd.daemon
import traceback

log = logging.getLogger("readings")
hostname = socket.getfqdn()
udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
udp.connect(("127.0.0.1", 8089))

class InfluxDB:
	def __init__(self, location, fields, interval, serial_number):
		self.location = location
		self.fields = set(fields) & set(["voltage", "frequency", "temperature", "current", "activePower", "reactivePower", "apparentPower", "powerFactor"])
		self.interval = interval
		self.serial_number = serial_number

	def update(self, reading):
		now = int(reading.ts.timestamp())
		if now % self.interval != 0:
			return

		data = ""
		for field in self.fields:
			key = {
				"voltage": "electricity:supply,traits=metric:gauge",
				"frequency": "electricity:supply,traits=metric:gauge",
				"temperature": "temperature,traits=metric:gauge,sensor=external:power-meter",
				"current": "electricity:load,traits=metric:gauge",
				"activePower": "electricity:meter,traits=metric:counter",
				"reactivePower": "electricity:meter,traits=metric:counter",
				"apparentPower": "electricity:meter,traits=metric:counter",
				"powerFactor": "electricity:load,traits=metric:gauge"
			}[field] + ",host=" + hostname + ",location=" + self.location + ",power-meter:serial_number=" + self.serial_number
			name = {
				"voltage": "voltage:V",
				"frequency": "frequency:Hz",
				"temperature": "celsius",
				"current": "current:A",
				"activePower": "power:kWh",
				"reactivePower": "power:kWh",
				"apparentPower": "power:kWh",
				"powerFactor": "power-factor:percent"
			}[field]
			if reading[field] is not None:
				data += "{0} {1}={2} {3}000000000\n".format(key, name, reading[field], now)

		if data:
			try:
				udp.send(data.encode("utf-8"))
				systemd.daemon.notify("STATUS=Updated reading at " + str(reading.ts))
			except ConnectionRefusedError as e:
				systemd.daemon.notify("STATUS=" + str(e))

def receive_loop(location, fields, interval, serial_numbers=None, ip4_numbers=None):
	meter = powermeter.PowerMeter(serial_numbers, ip4_numbers)
	meters = {}

	systemd.daemon.notify("READY=1")

	for reading in meter.readings:
		if reading.serialNumber not in meters:
			meters[reading.serialNumber] = InfluxDB(location, fields, interval, reading.serialNumber)
		meters[reading.serialNumber].update(reading)

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description="Power Meter receiver for InfluxDB file store")
	parser.add_argument("-v", "--verbose", action="store_const", default=logging.ERROR, const=logging.INFO, help="verbose mode")
	parser.add_argument("-m", "--meter", metavar="SERIAL_NUMBER", type=str, action="append", help="filter power meter by serial number")
	parser.add_argument("-s", "--source", metavar="IP_ADDRESS", type=str, action="append", help="filter power meter by IP address")
	parser.add_argument("-l", "--location", metavar="LOCATION", type=str, required=True, help="location")
	parser.add_argument("-f", "--field", metavar="FIELD", type=str, required=True, action="append", help="field to output")
	parser.add_argument("-i", "--interval", metavar="SECONDS", type=int, default=60, help="output interval")
	args = parser.parse_args()

	logging.basicConfig(level=args.verbose, format="%(asctime)s.%(msecs)03d  %(levelname)5s  %(message)s", datefmt="%F %T")

	receive_loop(args.location, args.field, args.interval, args.meter, args.source)
