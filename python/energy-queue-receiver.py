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
import logging.handlers
import os
import posix_ipc
import powermeter
import struct
import systemd.daemon

log = logging.getLogger("readings")

def receive_loop(mq_name, serial_numbers=None, ip4_numbers=None):
	queue = posix_ipc.MessageQueue(mq_name, flags=posix_ipc.O_CREAT, max_messages=8192//100, max_message_size=8+8+8, read=False)
	meter = powermeter.PowerMeter(serial_numbers, ip4_numbers)
	last = None

	systemd.daemon.notify("READY=1")

	for reading in meter.readings:
		current = [reading.activeEnergy, reading.reactiveEnergy]
		if last and current == last:
			continue
		last = current

		ts = int(reading.ts.timestamp())
		data = struct.pack("=Qd", ts, reading.activeEnergy)
		message = [str(ts), str(reading.activeEnergy)]
		if reading.reactiveEnergy is not None:
			data += struct.pack("=d", reading.reactiveEnergy)
			message += [str(reading.reactiveEnergy)]

		try:
			queue.send(data, timeout=0)
			log.info("wrote {0} to queue".format(" ".join(message)))
			systemd.daemon.notify("STATUS=Reading at {0}: ".format(reading.ts) + ", ".join(message[1:]) + " OK")
		except posix_ipc.BusyError:
			log.error("queue full writing {0}".format(" ".join(message)))
			systemd.daemon.notify("STATUS=Reading at {0}: ".format(reading.ts) + ", ".join(message[1:]) + " QUEUE FULL")
		except Exception:
			log.exception("queue error writing {0}".format(" ".join(message)))
			systemd.daemon.notify("STATUS=Reading at {0}: ".format(reading.ts) + ", ".join(message[1:]) + " QUEUE ERROR")

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description="Power Meter energy queue receiver")
	parser.add_argument("-m", "--meter", metavar="SERIAL_NUMBER", type=str, action="append", help="filter power meter by serial number")
	parser.add_argument("-s", "--source", metavar="IP_ADDRESS", type=str, action="append", help="filter power meter by IP address")
	parser.add_argument("-q", "--queue", metavar="NAME", type=str, required=True, help="message queue to store energy readings in")
	args = parser.parse_args()

	syslog = logging.handlers.SysLogHandler("/dev/log")
	syslog.ident = "energy-queue-receiver[{0}]: ".format(os.getpid())
	logging.basicConfig(level=logging.INFO, format="%(message)s", handlers=[syslog])

	receive_loop(args.queue, args.meter, args.source)
