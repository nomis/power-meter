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

import collections
import logging
import socket
import struct
import yaml

IP4_GROUP = "239.192.160.217"
PORT = 16021

ETH_DATA_LEN = 1500;
IPV4_HLEN = 20;
UDP_HLEN = 8;
MAX_LENGTH = ETH_DATA_LEN - IPV4_HLEN - UDP_HLEN

_PowerMeter__log = logging.getLogger("powermeter")


class PowerMeter:
	def __init__(self, serial_numbers=None, ip4_sources=None):
		self.serial_numbers = serial_numbers
		self.ip4_sources = ip4_sources

		ai = socket.getaddrinfo(IP4_GROUP, PORT, socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP, socket.AI_NUMERICHOST | socket.AI_NUMERICSERV)[0]
		self.s = socket.socket(*ai[0:3])
		self.s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		self.s.bind((IP4_GROUP, PORT))

		mreq = socket.inet_pton(ai[0], ai[4][0]) + struct.pack('=I', socket.INADDR_ANY)
		self.s.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

	@property
	def readings(self):
		while True:
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
						yield Reading(serial_number, data["meter"]["reading"])


_Reading__fields = [
	("voltage", "V", ".1f"),
	("current", "A", ".1f"),
	("frequency", "Hz", ".1f"),
	("activePower", "W", ".0f"),
	("reactivePower", "var", ".0f"),
	("apparentPower", "VA", ".0f"),
	("powerFactor", "%", ".1f"),
	("temperature", "°C", ".0f"),
	("activeEnergy", "kW·h", "09.2f"),
	("reactiveEnergy", "kW·h", "09.2f"),
]

class Reading:
	def __init__(self, serial_number, data):
		self.serial_number = serial_number
		self.data = data

	def __str__(self):
		fields = collections.OrderedDict()
		fields["serialNumber"] = self.serial_number

		for (name, unit, fmt) in __fields:
			if name in self.data:
				fields[name] = ("{0:" + fmt + "} {1}").format(self.data[name], unit)

		return ", ".join(["{0}={1}".format(k, v) for (k,v) in fields.items()])
