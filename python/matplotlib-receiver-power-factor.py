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

from datetime import datetime, timedelta
from math import ceil
import argparse
import logging
import matplotlib
import matplotlib.animation as animation
import matplotlib.dates as mdates
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
import powermeter

history = timedelta(seconds=60)
matplotlib.rcParams["toolbar"] = "None"

def receive_loop(serial_numbers=None, ip4_numbers=None):
	meter = powermeter.PowerMeterNumPy(serial_numbers, ip4_numbers, always_yield=True, history=history)
	readings = meter.readings

	fig = plt.figure()
	fig.canvas.set_window_title("Power Meter")

	ax = plt.subplot()
	ax.set_xlabel("Active Power (W)")
	ax.set_ylabel("Reactive Power (W)")
	active_power, = ax.plot(np.zeros(0), np.zeros(0), "r-")
	reactive_power, = ax.plot(np.zeros(0), np.zeros(0), "b-")
	apparent_power, = ax.plot(np.zeros(0), np.zeros(0), "g-")
	ax.tick_params(labelleft=False, labelright=True)
	ax.yaxis.set_label_position("right")
	ax.grid(True)
	ax.legend([active_power, reactive_power, apparent_power], ["active", "reactive", "apparent"], loc="best")

	def animate(i):
		reading = next(readings)
		if reading is not None:
			active_power.set_data([0, reading.activePower[-1]], [0, 0])
			reactive_power.set_data([reading.activePower[-1], reading.activePower[-1]], [0, reading.reactivePower[-1]])
			apparent_power.set_data([0, reading.activePower[-1]], [0, reading.reactivePower[-1]])

			ax.set_xlim(-5, np.max([reading.activePower[-1], reading.reactivePower[-1]]) + 5)
			ax.set_ylim(-5, np.max([reading.activePower[-1], reading.reactivePower[-1]]) + 5)

		return (active_power, reactive_power, apparent_power)

	ani = animation.FuncAnimation(fig, animate, interval=100, blit=False)
	plt.show()

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description="Power Meter receiver")
	parser.add_argument("-d", "--debug", action="store_const", default=logging.INFO, const=logging.DEBUG, help="enable debug")
	parser.add_argument("-m", "--meter", metavar="SERIAL_NUMBER", type=str, action="append", help="filter power meter by serial number")
	parser.add_argument("-s", "--source", metavar="IP_ADDRESS", type=str, action="append", help="filter power meter by IP address")
	args = parser.parse_args()

	logging.basicConfig(level=args.debug, format="%(asctime)s.%(msecs)03d  %(levelname)5s  %(message)s", datefmt="%F %T")

	receive_loop(args.meter, args.source)
