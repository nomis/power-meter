.PHONY: all clean install

INSTALL=install

all:
clean:

prefix=/usr
exec_prefix=$(prefix)
libdir=$(exec_prefix)/lib

install: all
	$(INSTALL) -m 755 -D python/powermeter/__init__.py $(DESTDIR)$(libdir)/power-meter/powermeter/__init__.py
	$(INSTALL) -m 755 -D python/basic-receiver.py $(DESTDIR)$(libdir)/power-meter/basic-receiver.py
	$(INSTALL) -m 755 -D python/numpy-receiver.py $(DESTDIR)$(libdir)/power-meter/numpy-receiver.py
	$(INSTALL) -m 755 -D python/matplotlib-receiver.py $(DESTDIR)$(libdir)/power-meter/matplotlib-receiver.py
	$(INSTALL) -m 755 -D python/serial-transmitter.py $(DESTDIR)$(libdir)/power-meter/serial-transmitter.py
	$(INSTALL) -m 755 -D python/energy-queue-receiver.py $(DESTDIR)$(libdir)/power-meter/energy-queue-receiver.py
	$(INSTALL) -m 755 -D python/energy-queue-database.py $(DESTDIR)$(libdir)/power-meter/energy-queue-database.py
	$(INSTALL) -m 755 -D python/rrd-receiver.py $(DESTDIR)$(libdir)/power-meter/rrd-receiver.py
