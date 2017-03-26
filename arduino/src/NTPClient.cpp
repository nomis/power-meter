/*

 Udp NTP Client

 Get the time from a Network Time Protocol (NTP) time server
 Demonstrates use of UDP sendPacket and ReceivePacket
 For more on NTP time servers and the messages needed to communicate with them,
 see http://en.wikipedia.org/wiki/Network_Time_Protocol

 created 4 Sep 2010
 by Michael Margolis
 modified 9 Apr 2012
 by Tom Igoe
 updated for the ESP8266 12 Apr 2015
 by Ivan Grokhotkov

 This code is in the public domain.

 */

#ifdef ARDUINO_ARCH_ESP8266

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "NTPClient.hpp"

static const char *NTP_SERVER = "pool.ntp.org";
static const uint16_t NTP_PORT = 123;
static const unsigned long NTP_VALID_INTERVAL = 10;
static const unsigned long NTP_START_INTERVAL = 7;
static const unsigned long NTP_TIMEOUT = 2000;

static const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

static byte packetBuffer[NTP_PACKET_SIZE];
static WiFiUDP udp;
static bool start = false;
static bool valid = false;

void ntp_init() {
	udp.begin(NTP_PORT);
}

bool ntp_valid() {
	if (!valid) {
		ntp_millis();
	}
	return valid;
}

unsigned long ntp_millis() {
	static unsigned long lastQuery;
	static unsigned long offset = 0;
	unsigned long interval = valid ? NTP_VALID_INTERVAL : NTP_START_INTERVAL;
	static IPAddress serverAddress;

	if (!start || millis() - lastQuery >= (1000UL << interval)) {
		if (WiFi.status() == WL_CONNECTED) {
			WiFi.hostByName(NTP_SERVER, serverAddress);

			memset(packetBuffer, 0, NTP_PACKET_SIZE);
			// Initialize values needed to form NTP request
			// (see URL above for details on the packets)
			packetBuffer[0] = 0b11100011;         // LI, Version, Mode
			packetBuffer[1] = 0;                  // Stratum, or type of clock
			packetBuffer[2] = NTP_VALID_INTERVAL; // Polling Interval
			packetBuffer[3] = 0xEC;               // Peer Clock Precision
			// 8 bytes of zero for Root Delay & Root Dispersion
			packetBuffer[12] = 49;
			packetBuffer[13] = 0x4E;
			packetBuffer[14] = 49;
			packetBuffer[15] = 52;

			udp.beginPacket(serverAddress, NTP_PORT);
			udp.write(packetBuffer, NTP_PACKET_SIZE);
			udp.endPacket();

			lastQuery = millis();
			start = true;

			while (millis() - lastQuery <= NTP_TIMEOUT) {
				if (udp.parsePacket()) {
					memset(packetBuffer, 0, NTP_PACKET_SIZE);
					if (udp.read(packetBuffer, NTP_PACKET_SIZE) == NTP_PACKET_SIZE) {
						unsigned long now = millis();
						uint32_t fraction = word(packetBuffer[44], packetBuffer[45]) << 16 | word(packetBuffer[46], packetBuffer[47]);

						fraction /= 4294967;
						fraction += (now - lastQuery) / 2;
						fraction %= 1000;
						offset = fraction - now;

						valid = true;
					}
					break;
				}
				yield();
			}
		}
	}

	return millis() + offset;
}

#else

void ntp_init() {

}

bool ntp_valid() {
	return false;
}

unsigned long ntp_millis() {
	return 0;
}

#endif
