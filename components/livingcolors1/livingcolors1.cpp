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

	this->send_command(data, 15);
}

//void LivingColors1Component::dump_config() {
//	ESP_LOGCONFIG(TAG, "Living Colors 1st generation component:");
////	if (this->command_repeats_.has_value())
////		ESP_LOGCONFIG(TAG, "  Command repeats: %d", this->command_repeats_.value());
//}

bool LivingColors1Component::receive_command(uint8_t *data, uint8_t length) {
	ESP_LOGV(TAG, "LivingColors1Component::receive_command");
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

	// Get the first 4 bytes of the address
	uint64_t address = 0;
	for(int i = 1; i <= 4; i++) {
		address += uint64_t(data[i]) << ((8 - i) * 8);
	}

	// If the first 4 bytes of the address are all OxFF it seems to be a special address (pairing?)
	if(address == 0xFFFFFFFF00000000) {
		ESP_LOGV(TAG, "Special address");
		uint64_t payload = 0;
		for(int i = 9; i <= 14; i++) {
			payload += uint64_t(data[i]) << ((8 - i) * 8);
		}
		ESP_LOGV(TAG, "  payload: 0x%016" PRIX64, payload);
		return true;
	}

	// Validate fixed value
	if(data[9] != 0x11) {
		ESP_LOGE(TAG, "Invalid fixed value: 0x%02X", data[9]);
		return false;
	}

	// Get the remaining 4 bytes of the address
	for(int i = 5; i <= 8; i++) {
		address += uint64_t(data[i]) << ((8 - i) * 8);
	}
	ESP_LOGV(TAG, "address: 0x%016" PRIX64, address);

	// Check if the address is handled by a device
	bool success = false;
	for (auto device : this->devices_) {
		if(device->address == address) {
			success = true;
			break;
		}
	}

	// If the address is not yet handled log the address as detected
	if(!success) {
		ESP_LOGI(TAG, "Address detected: 0x%016" PRIX64, address);
		return true;
	}

	// Command
	uint8_t command = data[10];
	ESP_LOGV(TAG, "command: 0x%02X", command);

	// Counter
	uint8_t serial_number = data[11];
	ESP_LOGV(TAG, "serial number: %d", serial_number);

	// HSV Color values
	uint8_t hue = data[12];
	uint8_t saturation = data[13];
	uint8_t value = data[14];
	ESP_LOGV(TAG, "hue: %d", hue);
	ESP_LOGV(TAG, "saturation: %d", saturation);
	ESP_LOGV(TAG, "value: %d", value);

	ESP_LOGD(TAG, "Received command 0x%02X for address 0x%016" PRIX64 " HSV: 0x%02X 0x%02X 0x%02X", command, address, hue, saturation, value);

	for (auto device : this->devices_) {
		if(device->address == address) {
			device->receive_command((Command) command, hue, saturation, value);
			return true;
		}
	}

	return false;
}

void LivingColors1Component::set_light(uint64_t address, Command command, uint8_t hue,
		uint8_t saturation, uint8_t value) {
	ESP_LOGV(TAG, "Setting light on address 0x%016" PRIX64 " to 0x%02X 0x%02X 0x%02X 0x%02X", address, (uint8_t) command, hue, saturation, value);

//	for (int i = 0; i < this->command_repeats_.value_or(1); i++) {
		uint8_t data[15];

		// Packet length, 14 bytes
		data[0] = 0x0E;

		// Addresses
		data[1] = uint8_t(address >> 8 * 7);
		data[2] = uint8_t(address >> 8 * 6);
		data[3] = uint8_t(address >> 8 * 5);
		data[4] = uint8_t(address >> 8 * 4);
		data[5] = uint8_t(address >> 8 * 3);
		data[6] = uint8_t(address >> 8 * 2);
		data[7] = uint8_t(address >> 8 * 1);
		data[8] = uint8_t(address >> 8 * 0);

		// Fixed
		data[9] = 0x11;

		// Command
		data[10] = (uint8_t) command;

		// Counter
		data[11] = this->serial_number_++;

		// HSV Color values
		data[12] = hue;
		data[13] = saturation;
		data[14] = value;

		this->send_command(data, 15);
//	}
}

void LivingColors1Client::set_parent(LivingColors1Component *parent) {
	this->parent_ = parent;
	this->parent_->add_device(this);
}


}
}
