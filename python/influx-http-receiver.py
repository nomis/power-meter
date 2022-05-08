#!/usr/bin/env python3
#
# power-meter - Arduino Power Meter Modbus Client
# Copyright 2020,2022  Simon Arlott
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

from influx import InfluxDB
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

class InfluxHTTP:
	def __init__(self, url, db, key, location, interval, serial_number):
		self.client = InfluxDB(url)
		self.db = db
		self.key = key
		self.location = location
		self.fields = set(["voltage", "frequency", "temperature", "current", "activePower", "reactivePower", "apparentPower", "powerFactor", "activeEnergy", "reactiveEnergy"])
		self.interval = interval
		self.serial_number = serial_number

	def update(self, reading):
		now = int(reading.ts.timestamp())
		if now % self.interval != 0:
			return

		tags = { "host": hostname, "location": self.location, "power-meter:serial_number": self.serial_number }
		data = {}
		for field in self.fields:
			name = {
				"voltage": "voltage:V",
				"frequency": "frequency:Hz",
				"temperature": "celsius",
				"current": "current:A",
				"activePower": "activePower:W",
				"reactivePower": "reactivePower:W",
				"apparentPower": "apparentPower:W",
				"powerFactor": "power-factor:percent",
				"activeEnergy": "activeEnergy:kWh",
				"reactiveEnergy": "reactiveEnergy:kWh",
			}[field]
			data[name] = reading[field]

		data["uptime:s"] = reading.uptime
		if reading.rtt:
			data["rtt:ms"] = reading.rtt

		try:
			resp = self.client.write(self.db, self.key, fields=data, tags=tags, time=reading.ts)
			if resp is None:
				systemd.daemon.notify(systemd.daemon.Notification.STATUS, "Updated reading at " + str(reading.ts))
			else:
				log.error("Error: " + repr(resp))
				systemd.daemon.notify(systemd.daemon.Notification.STATUS, "Error at " + str(reading.ts) + ": " + repr(resp))
		except Exception as e:
			log.error("Exception", e)
			systemd.daemon.notify(systemd.daemon.Notification.STATUS, "Error at " + str(reading.ts) + ": " + str(e))

def receive_loop(location, interval, url, db, key, serial_numbers=None, ip4_numbers=None):
	meter = powermeter.PowerMeter(serial_numbers, ip4_numbers)
	meters = {}

	systemd.daemon.notify(systemd.daemon.Notification.READY)

	for reading in meter.readings:
		if reading.serialNumber not in meters:
			meters[reading.serialNumber] = InfluxHTTP(url, db, key, location, interval, reading.serialNumber)
		meters[reading.serialNumber].update(reading)

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description="Power Meter receiver for InfluxDB HTTP")
	parser.add_argument("-v", "--verbose", action="store_const", default=logging.ERROR, const=logging.INFO, help="verbose mode")
	parser.add_argument("-m", "--meter", metavar="SERIAL_NUMBER", type=str, action="append", help="filter power meter by serial number")
	parser.add_argument("-s", "--source", metavar="IP_ADDRESS", type=str, action="append", help="filter power meter by IP address")
	parser.add_argument("-l", "--location", metavar="LOCATION", type=str, required=True, help="location")
	parser.add_argument("-i", "--interval", metavar="SECONDS", type=int, default=1, help="output interval")
	parser.add_argument("-u", "--url", metavar="URL", type=str, required=True, help="destination")
	parser.add_argument("-d", "--db", metavar="DATABASE", type=str, required=True, help="database")
	parser.add_argument("-k", "--key", metavar="KEY", type=str, default="electricity", help="key")
	args = parser.parse_args()

	logging.basicConfig(level=args.verbose, format="%(asctime)s.%(msecs)03d  %(levelname)5s  %(message)s", datefmt="%F %T")

	receive_loop(args.location, args.interval, args.url, args.db, args.key, args.meter, args.source)
