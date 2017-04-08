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

	fig, ax = plt.subplots()
	fig.canvas.set_window_title("Power Meter")
	fig.autofmt_xdate()

	ax.fmt_xdata = mdates.DateFormatter("%H:%M:%S")
	ax.xaxis.set_major_locator(ticker.LinearLocator(numticks=history.seconds // 10 + 1))
	ax.xaxis.set_major_formatter(ax.fmt_xdata)

	ax.set_ylabel("Voltage (V)")
	voltage, = ax.plot_date(np.zeros(0), np.zeros(0), "r-")
	voltage.axes.tick_params(labelright=True)

	def animate(i):
		reading = next(readings)
		if reading is not None:
			voltage.set_data(reading.ts, reading.voltage)
			voltage.axes.set_ylim(np.nanmin(reading.voltage) - 0.5, np.nanmax(reading.voltage) + 0.5)
			plt.xlim(reading.ts[0], reading.ts[-1])

		return voltage,

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
