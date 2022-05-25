/*
 * power-meter - Arduino Power Meter Modbus Client
 * Copyright 2022  Simon Arlott
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Comms.hpp"

#include <ESP8266WiFi.h>
#include <IPAddress.h>

#include <cstdio>
#include <string>
#include <vector>

#include <uuid/common.h>

#include "Main.hpp"

Comms::Comms() {
	std::string key = uuid::read_flash_string(F(ENC_KEY));

	for (uint8_t i = 0; i < key.length() && i / 2 < sizeof(enc_key_); i += 2) {
		unsigned int value;
		std::sscanf(&key[i], "%02x", &value);
		enc_key_[i / 2] = value;
	}

	key = uuid::read_flash_string(F(MAC_KEY));

	for (uint8_t i = 0; i < key.length() && i / 2 < sizeof(mac_key_); i += 2) {
		unsigned int value;
		std::sscanf(&key[i], "%02x", &value);
		mac_key_[i / 2] = value;
	}

	memset(&token_, 0, sizeof(token_));

	udp.begin(16021);
}

void Comms::add(uint32_t timestamp, const std::array<uint8_t,23> &data) {
	Data value;

	if (timestamp > MIN_TIME) {
		value.timestamp = htonl(timestamp);
		memcpy(value.data, data.data(), sizeof(value.data));
		uint32_t uptime_s = uuid::get_uptime_ms() / 1000;
		value.uptime_s[0] = uptime_s >> 16;
		value.uptime_s[1] = uptime_s >> 8;
		value.uptime_s[2] = uptime_s;
		rtt_us_ /= 16;
		if (rtt_us_ > UINT16_MAX) {
			rtt_us_ = 0;
		}
		value.rtt_16us = htons(rtt_us_);
		rtt_us_ = 0;

		if (data_.size() == MAX_DATA) {
			data_.pop_front();
		}

		data_.emplace_back(value);
	}
}

void Comms::transmit() {
	unsigned long start = micros();

	union {
		uint8_t padding8[AES_BLOCKLEN];
		uint32_t padding32[AES_BLOCKLEN / 4];
	} padding{};
	union {
		uint8_t iv8[AES_BLOCKLEN];
		uint32_t iv32[AES_BLOCKLEN / 4];
	} iv{};

	for (uint8_t i = 0; i < sizeof(iv.iv32) / sizeof(iv.iv32[0]); i++) {
		iv.iv32[i] = RANDOM_REG32;
	}

	for (uint8_t i = 0; i < sizeof(padding.padding32) / sizeof(padding.padding32[0]); i++) {
		padding.padding32[i] = RANDOM_REG32;
	}

	static_assert(sizeof(Data) % AES_BLOCKLEN == 0);
	std::vector<uint8_t> buffer(AES_BLOCKLEN + AES_BLOCKLEN + data_.size() * sizeof(Data));

	for (uint8_t i = 0; i < sizeof(token_.token32) / sizeof(token_.token32[0]); i++) {
		token_.token32[i] = RANDOM_REG32;
	}
	token_.token8[0] &= ~0x80; /* client */

	uint16_t pos = 0;
	memcpy(&buffer[pos], padding.padding8, sizeof(padding.padding8));
	pos += sizeof(padding.padding8);
	memcpy(&buffer[pos], token_.token8, sizeof(token_.token8));
	pos += AES_BLOCKLEN;

	for (const auto &value : data_) {
		memcpy(&buffer[pos], &value, sizeof(value));
		pos += sizeof(value);
	}

	output->print(F("# Transmit "));
	output->print(data_.size());
	output->print(' ');
	output->print(buffer.size());
	output->print(F(" ["));

	struct AES_ctx ctx{};
	AES_init_ctx_iv(&ctx, enc_key_, iv.iv8);
	AES_CBC_encrypt_buffer(&ctx, buffer.data(), pos);
	Sha256.initHmac(mac_key_, sizeof(mac_key_));
	Sha256.write(buffer.data(), pos);
	uint8_t *hmac = Sha256.resultHmac();

	int ret = udp.beginPacket(IPAddress(HOST_IP), HOST_PORT);
	if (ret == 1) {
		size_t len = udp.write(buffer.data(), pos);

		if (len == pos) {
			len = udp.write(hmac, SHA256_HASH_LEN);

			if (len == SHA256_HASH_LEN) {
				tx_micros_ = micros();
				token_.token8[0] |= 0x80; /* server: easier to memcmp later */
				token_valid_ = true;
				sync_time_ = true;
			}
		}

		udp.endPacket();
	}

	output->print(micros() - start);
	output->println(']');
}

void Comms::receive() {
	int len = udp.parsePacket();
	unsigned long rx_micros = micros();
	struct timeval tv;

	if (gettimeofday(&tv, nullptr) != 0) {
		tv.tv_sec = 0;
	}

	/* timeout for syncing time */
	if (tv.tv_sec > MIN_TIME) {
		/* 100ms if already set */
		if (rx_micros - tx_micros_ >= 100000) {
			sync_time_ = false;
		}
	} else {
		/* 1s if not set */
		if (rx_micros - tx_micros_ >= 1000000) {
			sync_time_ = false;
		}
	}

	if (len < SHA256_HASH_LEN) {
		return;
	}

	int data_len = (len - SHA256_HASH_LEN);

	if (data_len % AES_BLOCKLEN != 0) {
		return;
	}

	// First block is unusable
	// Second block is the token
	// Third block is usable data
	if (data_len < AES_BLOCKLEN * 3) {
		return;
	}

	std::vector<uint8_t> buffer(len);

	if (udp.read(buffer.data(), len) != len) {
		return;
	}

	Sha256.initHmac(mac_key_, sizeof(mac_key_));
	Sha256.write(buffer.data(), data_len);
	uint8_t *hmac = Sha256.resultHmac();

	if (memcmp(&buffer[data_len], hmac, SHA256_HASH_LEN)) {
		return;
	}

	output->print(F("# Receive "));
	output->print(len);
	output->print(F(" ["));

	struct AES_ctx ctx{};
	AES_init_ctx(&ctx, enc_key_);
	AES_CBC_decrypt_buffer(&ctx, buffer.data(), data_len);

	uint16_t pos = AES_BLOCKLEN;
	bool token_match = token_valid_ && !memcmp(&buffer[pos], &token_.token8, sizeof(token_.token8));
	pos += AES_BLOCKLEN;

	if (token_match) {
		rtt_us_ = rx_micros - tx_micros_;
		token_valid_ = false;
	}

	if (token_match && sync_time_) {
		uint32_t remote_time_s = ntohl(*(uint32_t*)&buffer[pos]);
		pos += 4;
		uint32_t remote_time_us = ntohl(*(uint32_t*)&buffer[pos]);
		pos += 4;

		tv.tv_sec = remote_time_s;
		tv.tv_usec = remote_time_us;

		tv.tv_usec += micros() - rx_micros;
		tv.tv_usec += rtt_us_ / 2;

		while (tv.tv_usec >= 1000000) {
			tv.tv_usec -= 1000000;
			tv.tv_sec++;
		}

		settimeofday(&tv, nullptr);
	} else {
		pos += 8;
	}

	while (pos < data_len) {
		uint32_t timestamp_ack = *(uint32_t*)&buffer[pos];
		pos += 4;

		if (ntohl(timestamp_ack) == 0) {
			continue;
		}

		for (auto it = data_.begin(); it != data_.end(); it++) {
			if (it->timestamp == timestamp_ack) {
				data_.erase(it);
				break;
			}
		}
	}

	output->print(micros() - rx_micros);
	output->print(']');

	if (token_match && sync_time_) {
		output->print(' ');
		output->print(tv.tv_sec);
		output->print(' ');
		output->print(tv.tv_usec);

		sync_time_ = false;
	}

	if (token_match) {
		output->print(F(" <"));
		output->print(rtt_us_);
		output->print('>');
	}

	output->println();
}
