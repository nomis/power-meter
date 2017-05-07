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
import datetime
import decimal
import logging
import logging.handlers
import os
import posix_ipc
import psycopg2
import psycopg2.extras
import psycopg2.pool
import pytz
import struct
import sys
import systemd.daemon
import time
import traceback

log = logging.getLogger("readings")
db = None

def cursor_insert_value(c, meter, ts, name, value):
	exists = False

	c.execute("SELECT value FROM readings_" + name + " WHERE meter = %(meter)s ORDER BY ts DESC LIMIT 1", { "meter": meter })
	row = c.fetchone()
	if row:
		if row["value"] == decimal.Decimal(value).quantize(row["value"]):
			exists = True

	if not exists:
		c.execute("INSERT INTO readings_" + name + " (meter, ts, value) VALUES(%(meter)s, %(ts)s, %(value)s)", { "meter": meter, "ts": ts, "value": value })

	return "{1:.3f} ({0}) {2}".format(name, value, "exists" if exists else "inserted")

def database_insert(meter, dsn, ts, active_energy, reactive_energy):
	global db

	try:
		backoff = 0
		conn = None

		while True:
			try:
				if not db:
					db = psycopg2.pool.ThreadedConnectionPool(1, 5, dsn, connection_factory=psycopg2.extras.RealDictConnection)

				conn = db.getconn()
				c = conn.cursor()

				msg = cursor_insert_value(c, meter, ts, "active", active_energy)
				if reactive_energy is not None:
					msg += "; "
					msg += cursor_insert_value(c, meter, ts, "reactive", reactive_energy)

				systemd.daemon.notify("STATUS=Meter {0} at {1}: {2}".format(meter, ts, msg))

				conn.commit()
				c.close()

				return
			except KeyboardInterrupt:
				raise
			except SystemExit:
				raise
			except:
				for line in traceback.format_exc().split("\n"):
					log.error(line)

				systemd.daemon.notify("STATUS=Database access error: " + str(sys.exc_info()[1]))

				time.sleep(30 + backoff)
				if backoff < 30:
					backoff += 1

				continue
			finally:
				if db and conn:
					db.putconn(conn)
	except:
		for line in traceback.format_exc().split("\n"):
			log.error(line)

		raise

def receive_loop(mq_name, dsn, meter):
	queue = posix_ipc.MessageQueue(mq_name, flags=posix_ipc.O_CREAT, max_messages=8192, max_message_size=4+4+4, write=False)

	systemd.daemon.notify("READY=1")

	while True:
		(data, priority) = queue.receive()

		if len(data) >= 12:
			(ts, active_energy, reactive_energy) = struct.unpack("=Iff", data)
		elif len(data) >= 8:
			(ts, active_energy) = struct.unpack("=If", data)
			reactiveEnergy = None
		else:
			continue

		ts = pytz.utc.localize(datetime.datetime.utcfromtimestamp(ts))
		database_insert(meter, dsn, ts, active_energy, reactive_energy)

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description="Power Meter energy queue database client")
	parser.add_argument("-m", "--meter", metavar="METER", type=str, required=True, help="meter identifier")
	parser.add_argument("-q", "--queue", metavar="NAME", type=str, required=True, help="message queue to read energy readings from")
	parser.add_argument("-d", "--database", metavar="NAME", type=str, required=True, help="message queue to read energy readings from")
	args = parser.parse_args()

	syslog = logging.handlers.SysLogHandler("/dev/log")
	syslog.ident = "energy-queue-database[{0}]: ".format(os.getpid())
	logging.basicConfig(level=logging.INFO, format="%(message)s", handlers=[syslog])

	receive_loop(args.queue, args.database, args.meter)
