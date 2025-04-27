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
import collections
import logging
import os
import powermeter
import re
import rrdtool
import sys
import systemd.daemon
import traceback

log = logging.getLogger("readings")


class RRD:
	def __init__(self, directory, filename):
		self.filename_supply = os.path.join(directory, re.sub("[^A-Za-z0-9 ]", "_", filename) + ",supply.rrd")
		self.filename_load = os.path.join(directory, re.sub("[^A-Za-z0-9 ]", "_", filename) + ",load.rrd")

		if not os.path.exists(self.filename_supply):
			rrdtool.create(self.filename_supply,
				"--start", "0", "--step", "1",

				"DS:voltage:GAUGE:2:50:U",
				"DS:frequency:GAUGE:2:25:U",
				"DS:temperature:GAUGE:2:U:U",

				"RRA:AVERAGE:0:1s:13w",

				"RRA:MIN:0.5:1h:100y",
				"RRA:MAX:0.5:1h:100y",
				"RRA:AVERAGE:0.5:1h:100y"
			)

		if not os.path.exists(self.filename_load):
			rrdtool.create(self.filename_load,
				"--start", "0", "--step", "1",

				"DS:current:GAUGE:2:0:U",
				"DS:activePower:GAUGE:2:0:U",
				"DS:reactivePower:GAUGE:2:0:U",
				"DS:apparentPower:GAUGE:2:0:U",
				"DS:powerFactor:GAUGE:2:-100:100",

				"RRA:AVERAGE:0:1s:13w",

				"RRA:MIN:0.5:1m:2y",
				"RRA:MAX:0.5:1m:2y",
				"RRA:AVERAGE:0.5:1m:2y",

				"RRA:MIN:0.5:1h:100y",
				"RRA:MAX:0.5:1h:100y",
				"RRA:AVERAGE:0.5:1h:100y"
			)

	def update(self, reading):
		if reading["apparentPower"] is None:
			apparentPower = reading["activePower"] / max(0.01, status["powerFactor"])
			reading["apparentPower"] = round(apparentPower, 1)

			if reading["reactivePower"] is None:
				reading["reactivePower"] = round(
					math.sqrt(abs(apparentPower ** 2 - status["activePower"] ** 2)), 1
				)
		elif reading["reactivePower"] is None:
			reading["reactivePower"] = round(
				math.sqrt(abs(reading["apparentPower"] ** 2 - status["activePower"] ** 2)), 1
			)

		__update(self.filename_supply, reading, ["voltage", "frequency", "temperature"])
		__update(self.filename_load, reading, ["current", "activePower", "reactivePower", "apparentPower", "powerFactor"])
		systemd.daemon.notify("STATUS=Updated reading at " + str(reading.ts))

def _RRD__update(filename, reading, fields):
	template = ":".join(fields)
	data = ":".join([str(x) for x in [int(reading.ts.timestamp())] + [reading[field] if reading[field] is not None else "U" for field in fields]])
	log.info("Update %s: %s = %s", filename, template, data)
	try:
		rrdtool.update(filename, "-s", "-t", template, data)
	except KeyboardInterrupt:
		raise
	except SystemExit:
		raise
	except:
		for line in traceback.format_exc().split("\n"):
			log.error(line)

		systemd.daemon.notify("STATUS=Update ({0}: {1} = {2}) error: {3}".format(filename, template, data, sys.exc_info()[1]))
		raise


def receive_loop(output_directory, serial_numbers=None, ip4_numbers=None):
	meter = powermeter.PowerMeter(serial_numbers, ip4_numbers)
	rrds = {}

	systemd.daemon.notify("READY=1")

	for reading in meter.readings:
		if reading.serialNumber not in rrds:
			rrds[reading.serialNumber] = RRD(output_directory, reading.serialNumber)
		rrds[reading.serialNumber].update(reading)

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description="Power Meter receiver for RRD file store")
	parser.add_argument("-v", "--verbose", action="store_const", default=logging.ERROR, const=logging.INFO, help="verbose mode")
	parser.add_argument("-m", "--meter", metavar="SERIAL_NUMBER", type=str, action="append", help="filter power meter by serial number")
	parser.add_argument("-s", "--source", metavar="IP_ADDRESS", type=str, action="append", help="filter power meter by IP address")
	parser.add_argument("-o", "--output", metavar="DIRECTORY", type=str, default=".", help="output directory")
	args = parser.parse_args()

	logging.basicConfig(level=args.verbose, format="%(asctime)s.%(msecs)03d  %(levelname)5s  %(message)s", datefmt="%F %T")

	receive_loop(args.output, args.meter, args.source)
