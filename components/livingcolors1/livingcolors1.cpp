#include "livingcolors1.h"

#include "esphome/core/log.h"
#include "esphome/components/cc2500/cc2500.h"

#include <cinttypes>

namespace esphome {
namespace livingcolors1 {

static const char *TAG = "livingcolors1";

void LivingColors1Component::setup() {
	ESP_LOGCONFIG(TAG, "Setting up Living Colors 1st generation component...");

	uint8_t data[15];
	memset(data, 0, sizeof(data));
	data[0] = 0x0E;
	data[9] = 0x11;

	((CC2500Device*)this)->send(data, 15);
}

bool LivingColors1Component::receive(uint8_t *data, uint8_t length) {
	ESP_LOGV(TAG, "LivingColors1Component::receive");
	// Validate data length, 17 bytes
	if(length != 17) {
		// Invalid data length can happen when data from other cc2500 devices is received.
		ESP_LOGV(TAG, "Invalid data length: %d", length);
		return false;
	}

	// Validate packet length, 14 bytes
	if(data[0] != 14) {
		ESP_LOGE(TAG, "Invalid packet length: %d", data[0]);
		return false;
	}

	// Validate fixed value
	if(data[9] != 0x11) {
		ESP_LOGE(TAG, "Invalid fixed value: 0x%02X", data[9]);
		return false;
	}

	// Get the address
	uint64_t address = 0;
	for(int i = 1; i <= 8; i++) {
		address += uint64_t(data[i]) << ((8 - i) * 8);
	}
	ESP_LOGV(TAG, "address: 0x%016" PRIX64, address);

	// Get the payload
	uint64_t payload = 0;
	for(int i = 3; i <= 8; i++) {
		payload += uint64_t(data[i + 6]) << ((8 - i) * 8);
	}
	ESP_LOGV(TAG, "payload: 0x%06" PRIX64, payload);

	// Serial number
	uint8_t serial_number = data[11];
	ESP_LOGV(TAG, "serial number: %d", serial_number);

	// If the first 4 bytes of the address are all OxFF it seems to be a special address for color
	// cycling (and more?). The last 4 bytes of the address seem to be part of the device address.
	if((address & 0xFFFFFFFF00000000) == 0xFFFFFFFF00000000) {
		ESP_LOGV(TAG, "Special address");
		bool success = false;
		for (auto device : this->devices_) {
			if((device->address & 0x00000000FFFFFFFF) == (address & 0x00000000FFFFFFFF)) {
				ESP_LOGV(TAG, "Device address matches special address");
				if (device->receive(&data[10], 5))
					success = true;
			}
		}

		if(success)
			return true;

		ESP_LOGV(TAG, "No matching device address for special address");
		return false;
	}

	// Check if the address is handled by a device
	for (auto device : this->devices_) {
		if(device->address == address)
			return device->receive(&data[10], 5);
	}

	// If the address is not yet handled log the address as detected
	ESP_LOGI(TAG, "Address detected: 0x%016" PRIX64, address);
	return false;
}

void LivingColors1Component::send(uint8_t *data, uint8_t length) {
	data[11] = this->serial_number_++;
	((CC2500Device*)this)->send(data, length);
}

void LivingColors1Client::set_parent(LivingColors1Component *parent) {
	this->parent_ = parent;
	this->parent_->add_device(this);
}

void LivingColors1Client::send(uint8_t *data, uint8_t length) {
	uint8_t data_[10 + length];

	// Packet length
	data_[0] = 10 + length - 1;

	// Addresses
	data_[1] = uint8_t(address >> 8 * 7);
	data_[2] = uint8_t(address >> 8 * 6);
	data_[3] = uint8_t(address >> 8 * 5);
	data_[4] = uint8_t(address >> 8 * 4);
	data_[5] = uint8_t(address >> 8 * 3);
	data_[6] = uint8_t(address >> 8 * 2);
	data_[7] = uint8_t(address >> 8 * 1);
	data_[8] = uint8_t(address >> 8 * 0);

	// Fixed
	data_[9] = 0x11;

	// Command
	for(int j = 0; j < length; j++)
		data_[10+j] = data[j];

	for (int i = 0; i < this->send_repeats_; i++) {
		this->parent_->send(data_, 10 + length);
		esphome::delay(14);
	}
}


}
}
