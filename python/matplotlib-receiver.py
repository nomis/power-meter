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
import matplotlib.gridspec as gspec
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
	gs = gspec.GridSpec(3, 1)

	ax_supply = plt.subplot(gs[0, :])
	ax_supply.set_title("Supply")
	ax_supply.fmt_xdata = mdates.DateFormatter("%H:%M:%S")
	ax_supply.xaxis.set_major_formatter(ax_supply.fmt_xdata)
	ax_supply.set_ylabel("V")
	voltage, = ax_supply.plot_date(np.zeros(0), np.zeros(0), "r-")

	ax_supply_2 = ax_supply.twinx()
	ax_supply_2.set_ylabel("Hz")
	frequency, = ax_supply_2.plot_date(np.zeros(0), np.zeros(0), "b-.")

	ax_current = plt.subplot(gs[1, :])
	ax_current.set_title("Current")
	ax_current.fmt_xdata = mdates.DateFormatter("%H:%M:%S")
	ax_current.xaxis.set_major_formatter(ax_current.fmt_xdata)
	ax_current.set_ylabel("A")
	ax_current.set_yscale("log")
	ax_current.grid(True)
	ax_current.yaxis.set_major_formatter(ticker.ScalarFormatter())
	current, = ax_current.plot_date(np.zeros(0), np.zeros(0), "m-")
	current.axes.tick_params(labelright=True)

	ax_power = plt.subplot(gs[2, :])
	ax_power.set_title("Power")
	ax_power.fmt_xdata = mdates.DateFormatter("%H:%M:%S")
	ax_power.xaxis.set_major_formatter(ax_power.fmt_xdata)
	ax_power.set_ylabel("W")
	ax_power.set_yscale("log")
	ax_power.grid(True)
	ax_power.yaxis.set_major_formatter(ticker.ScalarFormatter())
	active_power, = ax_power.plot_date(np.zeros(0), np.zeros(0), "r-")
	active_power.axes.tick_params(labelright=True)
	reactive_power, = ax_power.plot_date(np.zeros(0), np.zeros(0), "b-")
	reactive_power.axes.tick_params(labelright=True)
	apparent_power, = ax_power.plot_date(np.zeros(0), np.zeros(0), "g-")
	apparent_power.axes.tick_params(labelright=True)
	ax_power.legend([active_power, reactive_power, apparent_power], ["active", "reactive", "apparent"], loc="best")

	fig.autofmt_xdate(rotation=45)

	def animate(i):
		reading = next(readings)
		if reading is not None:
			voltage.set_data(reading.ts, reading.voltage)
			frequency.set_data(reading.ts, reading.frequency)
			hz = round(np.nanmedian(reading.frequency))
			ax_supply.set_xlim(reading.ts[0], reading.ts[-1])
			ax_supply.set_ylim(np.nanmin(reading.voltage) - 0.5, np.nanmax(reading.voltage) + 0.5)
			ax_supply_2.set_ylim(hz * 0.99, hz * 1.01)

			current.set_data(reading.ts, reading.current)
			ax_current.set_xlim(reading.ts[0], reading.ts[-1])
			ax_current.set_ylim(np.nanmin(reading.current) * 0.5, ceil(np.nanmax(reading.current)) * 2)

			active_power.set_data(reading.ts, reading.activePower)
			reactive_power.set_data(reading.ts, reading.reactivePower)
			apparent_power.set_data(reading.ts, reading.apparentPower)

			ax_power.set_xlim(reading.ts[0], reading.ts[-1])
			all_power = np.concatenate((reading.activePower, reading.reactivePower, reading.apparentPower))
			ax_power.set_ylim(np.nanmin(all_power) * 0.5, ceil(np.nanmax(all_power)) * 2)

		return (voltage, frequency, current, active_power, reactive_power, apparent_power)

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
