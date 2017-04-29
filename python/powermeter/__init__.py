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

from collections import OrderedDict
from datetime import datetime, timedelta
import logging
import pytz
import socket
import struct
import tzlocal
import yaml


IP4_GROUP = "239.192.160.217"
PORT = 16021

ETH_DATA_LEN = 1500;
IPV4_HLEN = 20;
UDP_HLEN = 8;
MAX_LENGTH = ETH_DATA_LEN - IPV4_HLEN - UDP_HLEN


_PowerMeter__log = logging.getLogger("powermeter")

class PowerMeter:
	def __init__(self, serial_numbers=None, ip4_sources=None, always_yield=False):
		self.serial_numbers = serial_numbers
		self.ip4_sources = ip4_sources

		ai = socket.getaddrinfo(IP4_GROUP, PORT, socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP, socket.AI_NUMERICHOST | socket.AI_NUMERICSERV)[0]
		self.s = socket.socket(*ai[0:3])
		self.s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		self.s.bind((IP4_GROUP, PORT))

		mreq = socket.inet_pton(ai[0], ai[4][0]) + struct.pack('=I', socket.INADDR_ANY)
		self.s.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

		if always_yield:
			self.s.setblocking(False)

	@property
	def readings(self):
		while True:
			try:
				(data, sender) = self.s.recvfrom(MAX_LENGTH)
				__log.debug(": ".join((sender[0], data.decode("utf-8", "replace"))))

				try:
					data = yaml.safe_load(data)
				except yaml.YAMLError as e:
					continue

				serial_number = data.get("meter", {}).get("serialNumber", None)
				if not self.ip4_sources or sender[0] in self.ip4_sources:
					if serial_number and (not self.serial_numbers or serial_number in self.serial_numbers):
						reading = data.get("meter", {}).get("reading", {})
						if reading:
							ts = data.get("timestamp")
							if ts:
								ts = pytz.utc.localize(datetime.utcfromtimestamp(ts))
							yield Reading(serial_number, data["meter"]["reading"], ts)
			except BlockingIOError:
				yield None


_Reading__fields = OrderedDict([
	("voltage", ("V", ".1f")),
	("current", ("A", ".1f")),
	("frequency", ("Hz", ".1f")),
	("activePower", ("W", ".0f")),
	("reactivePower", ("var", ".0f")),
	("apparentPower", ("VA", ".0f")),
	("powerFactor", ("%", ".1f")),
	("temperature", ("°C", ".0f")),
	("activeEnergy", ("kW·h", "09.2f")),
	("reactiveEnergy", ("kW·h", "09.2f")),
])

class Reading:
	def __init__(self, serial_number, data, timestamp=None):
		self.serialNumber = serial_number
		self._data = data
		if timestamp:
			self.ts = timestamp
		else:
			self.ts = pytz.utc.localize(datetime.utcnow())

	def __getattr__(self, key):
		if key in __fields:
			if key in self._data:
				return self._data[key]
			return None
		raise AttributeError(key + " not found")

	def __getitem__(self, key):
		if key in __fields:
			if key in self._data:
				return self._data[key]
			return None
		raise AttributeError(key + " not found")

	def __str__(self):
		fields = OrderedDict()
		fields["serialNumber"] = self.serialNumber

		for (name, (unit, fmt)) in __fields.items():
			if name in self._data:
				fields[name] = ("{0:" + fmt + "} {1}").format(self._data[name], unit)

		return ", ".join(["{0}={1}".format(k, v) for (k,v) in fields.items()])


try:
	import numpy as np


	_PowerMeterNumPy__fields = _Reading__fields
	_PowerMeterNumPy__dtype = [("ts", "O")] + [(name, "float64") for name in _PowerMeterNumPy__fields.keys()]
	_PowerMeterNumPy__localtz = tzlocal.get_localzone()

	class PowerMeterNumPy(PowerMeter):
		def __init__(self, serial_numbers=None, ip4_sources=None, always_yield=False, history=timedelta(seconds=60)):
			super().__init__(serial_numbers, ip4_sources, always_yield)

			self.history = history

		@property
		def readings(self):
			data = None
			last = None

			while True:
				reading = next(super().readings)
				if reading is not None:
					now = reading.ts.replace(microsecond=0)
					if now == last:
						continue
					last = now
					local_now = now.astimezone(__localtz)

					np_reading = tuple([local_now] + [reading[name] for name in __fields])
					np_data = np.array([np_reading], dtype=__dtype)
					if data is None:
						data = np.array([tuple([(now - timedelta(seconds=i)).astimezone(__localtz)] + [None for name in __fields]) for i in range(self.history.seconds, 0, -1)], dtype=__dtype)
					data = np.concatenate((data, np_data))

					while data[["ts"]][0][0] < now - self.history:
						data = np.delete(data, 0, 0)

				if data is None:
					yield None
				else:
					yield np.rec.array(data)

except ImportError:
	pass
