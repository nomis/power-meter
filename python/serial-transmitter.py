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
import fcntl
import logging
import powermeter
import os
import serial
import socket
import struct
import termios
import time

log = logging.getLogger("readings")

def transmit_loop(device, interface):
	# Wait for a new second before opening the device
	now = time.time()
	time.sleep(1 - (now - int(now)))

	with serial.Serial(device, 115200) as input:
		# Configure read() to block until a whole line is received
		attrs = termios.tcgetattr(input.fd)
		attrs[3] |= termios.ICANON
		attrs[6][termios.VMIN] = 1
		attrs[6][termios.VTIME] = 0
		termios.tcsetattr(input.fd, termios.TCSAFLUSH, attrs)
		fcntl.fcntl(input.fd, fcntl.F_SETFL, 1)

		with socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP) as output:
			output.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 1)

			# Transmit on a specific interface from any IP
			mcast_if = (socket.inet_pton(socket.AF_INET, powermeter.IP4_GROUP)
				+ struct.pack("!I", socket.INADDR_ANY)
				+ struct.pack("@i", socket.if_nametoindex(interface)))
			output.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, mcast_if)

			last = 0
			while True:
				# Extract the last line of any data read
				line = list(filter(None, os.read(input.fd, 4096).replace(b"\r", b"").split(b"\n")))[-1]
				log.debug(line)

				now = int(time.time())
				if now != last:
					output.sendto(line, (powermeter.IP4_GROUP, powermeter.PORT))
				last = now

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description="Power Meter transmitter")
	parser.add_argument("-d", "--debug", action="store_const", default=logging.INFO, const=logging.DEBUG, help="enable debug")
	parser.add_argument("-l", "--line", metavar="DEVICE", type=str, required=True, help="serial device to open")
	parser.add_argument("-i", "--interface", metavar="INTERFACE", type=str, required=True, help="network interface to use")
	args = parser.parse_args()

	logging.basicConfig(level=args.debug, format="%(asctime)s.%(msecs)03d  %(levelname)5s  %(message)s", datefmt="%F %T")

	transmit_loop(args.line, args.interface)
