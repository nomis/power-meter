#!/usr/bin/env python3
#
# power-meter - Arduino Power Meter Modbus Client
# Copyright 2022  Simon Arlott
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

from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes
from datetime import timedelta
import argparse
import hmac
import logging
import socket
import struct
import systemd.daemon
import time
import yaml

import powermeter

AES_BLOCKLEN = 16
SHA256_HASH_LEN = 32
DATA_LEN = AES_BLOCKLEN * 2

log = logging.getLogger("readings")

def parse(data):
	reading = struct.unpack("!LHHHHHHhLLbBHH", data)

	return {
		"timestamp": reading[0],
		"voltage": reading[1] / 10,
		"current": reading[2] / 10,
		"frequency": reading[3] / 10,
		"activePower": reading[4],
		"reactivePower": reading[5],
		"apparentPower": reading[6],
		"powerFactor": reading[7] / 10,
		"activeEnergy": reading[8] / 100,
		"reactiveEnergy": reading[9] / 100,
		"temperature": reading[10],
		"uptime": (reading[11] << 16) | reading[12],
		"rtt": reading[13] * 16 / 1000,
	}

def receive_loop(port, interface, meter):
	with open("config.yaml", "r") as f:
		config = yaml.safe_load(f)
	enc_key = bytes.fromhex(config["enc_key"])
	mac_key = bytes.fromhex(config["mac_key"])

	ai = socket.getaddrinfo(None, port, socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP, socket.AI_PASSIVE | socket.AI_NUMERICHOST | socket.AI_NUMERICSERV)[0]
	with socket.socket(*ai[0:3]) as input:
		input.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		input.bind(ai[4])

		last_reply = time.monotonic() - 1
		seen_timestamps = []

		with socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP) as output:
			output.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 1)

			# Transmit on a specific interface from any IP
			mcast_if = (socket.inet_pton(socket.AF_INET, powermeter.IP4_GROUP)
				+ struct.pack("!I", socket.INADDR_ANY)
				+ struct.pack("@i", socket.if_nametoindex(interface)))
			output.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, mcast_if)

			systemd.daemon.notify("READY=1")

			while True:
				(data, sender) = input.recvfrom(1480)

				if len(data) < SHA256_HASH_LEN:
					continue

				data_len = len(data) - SHA256_HASH_LEN

				if data_len % AES_BLOCKLEN != 0:
					continue

				# First block is unusable
				# Second block is the token
				# Third block is usable data
				if data_len < AES_BLOCKLEN * 2:
					continue

				if not hmac.compare_digest(data[data_len:], hmac.new(mac_key, data[0:data_len], "SHA256").digest()):
					continue

				aes = AES.new(enc_key, AES.MODE_CBC, b"\x00" * AES_BLOCKLEN)
				data = aes.decrypt(data[0:data_len])

				log.debug(" <- ".join((repr(sender), data.hex())))

				pos = AES_BLOCKLEN * 2
				timestamps = []
				readings = []
				if (data_len - pos) % DATA_LEN != 0:
					continue
				elif data_len - pos < DATA_LEN:
					# No more than 2 replies per second to time sync
					if time.monotonic() - last_reply < 0.5:
						continue
				else:
					while data_len - pos > 0:
						reading = parse(data[pos:pos + DATA_LEN])
						pos += DATA_LEN

						timestamps.append(reading["timestamp"])
						readings.append(reading)

					new_timestamps = set(timestamps) - set(seen_timestamps)
					if not new_timestamps:
						# No new timestamps
						continue
					if max(timestamps) < time.time() - 40:
						# Out of date
						continue

				resp = get_random_bytes(AES_BLOCKLEN) + data[AES_BLOCKLEN:2 * AES_BLOCKLEN]
				now = time.time()
				resp += struct.pack("!LL", int(now), int((now % 1) * 1000000))
				for timestamp in timestamps:
					resp += struct.pack("!L", timestamp)
				resp += bytes(AES_BLOCKLEN - (len(resp) % AES_BLOCKLEN))

				log.debug(" -> ".join((repr(sender), resp.hex())))

				aes = AES.new(enc_key, AES.MODE_CBC)
				resp = aes.encrypt(resp)
				resp += hmac.new(mac_key, resp, "SHA256").digest()

				input.sendto(resp, sender)
				last_reply = time.monotonic()

				for reading in readings:
					if reading["timestamp"] not in seen_timestamps:
						reading_copy = reading.copy()
						del reading_copy["timestamp"]
						del reading_copy["rtt"]
						del reading_copy["uptime"]
						data = {
							"meter": { "model": "RI-D19-80-C", "serialNumber": meter, "reading": reading_copy },
							"timestamp": reading["timestamp"],
							"uptime": reading["uptime"],
							"rtt": reading["rtt"],
						}
						output.sendto(yaml.dump(data).encode("ascii"), (powermeter.IP4_GROUP, powermeter.PORT))

				seen_timestamps = list(sorted(set(timestamps) | set(seen_timestamps)))[-60:]

				if readings:
					status = f"Received {len(timestamps)} with delay {(time.time() - readings[-1]['timestamp']) * 1000:.1f}ms (uptime {[str(timedelta(seconds=reading['uptime'])) for reading in readings]}; rtt {[reading['rtt'] for reading in readings]})"
				else:
					status = "Receive time sync request"
				log.debug(" == ".join((repr(sender), status)))
				systemd.daemon.notify(f"STATUS={status}")

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description="Power Meter receiver")
	parser.add_argument("-d", "--debug", action="store_const", default=logging.INFO, const=logging.DEBUG, help="enable debug")
	parser.add_argument("-p", "--port", type=int, default=16021, help="port")
	parser.add_argument("-m", "--meter", metavar="SERIAL_NUMBER", type=str, required=True, help="power meter serial number")
	parser.add_argument("-i", "--interface", metavar="INTERFACE", type=str, required=True, help="network interface to use")
	args = parser.parse_args()

	logging.basicConfig(level=args.debug, format="%(asctime)s.%(msecs)03d  %(levelname)5s  %(message)s", datefmt="%F %T")

	receive_loop(args.port, args.interface, args.meter)
