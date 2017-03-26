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

#ifndef POWER_METER_NTPCLIENT_HPP
#define POWER_METER_NTPCLIENT_HPP

void ntp_init();
bool ntp_valid();
unsigned long ntp_millis();

#endif
