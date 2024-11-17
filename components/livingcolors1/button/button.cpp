#include "esphome/core/log.h"
#include "button.h"

#include <cinttypes>

namespace esphome {
namespace livingcolors1 {

static const char *const TAG = "livingcolors1.button";

void LivingColors1ButtonComponent::setup() {
	ESP_LOGCONFIG(TAG, "Setting up Living Colors gen 1 button...");
}

void LivingColors1ButtonComponent::dump_config() {
	LOG_BUTTON("", "Living Colors gen 1 button:", this);
	ESP_LOGCONFIG(TAG, "  Address: 0x%016" PRIX64, this->address_);
}

void LivingColors1PairButton::press_action() {
	ESP_LOGV(TAG, "Pair Button press for address 0x%016" PRIX64, this->special_address_());

	uint8_t data[5];

	// Command
	data[0] = (uint8_t) Command::PAIRING_REQUEST;

	// HSV Color values
	data[2] = 0x00;
	data[3] = 0x00;
	data[4] = 0x00;

	this->send_(this->special_address_(), &data[0], 5);
}

bool LivingColors1PairButton::receive(uint64_t address, uint8_t *data, uint8_t length) {
	// Command
	Command command = (Command) data[0];

	if(this->is_special_(address) && command == Command::PAIRING_REQUEST) {
		ESP_LOGV(TAG, "Pairing request: 0x%016" PRIX64, address);
		return true;
	}

	if(this->is_response_(address) && command == Command::PAIRING_RESPONSE) {
		ESP_LOGD(TAG, "Pairing response: 0x%016" PRIX64, address);
		// A pairing response has the light address and remote address swapped.
		ESP_LOGI(TAG, "Address detected: 0x%016" PRIX64, this->swapped_address_());
		return true;
	}

	return false;
}

void LivingColors1TestButton::press_action() {
	ESP_LOGV(TAG, "Test Button press for address 0x%016" PRIX64, this->address_);

	uint8_t data[5];

	// Command
	data[0] = (uint8_t) Command::ON;
	data[2] = 0x10;
	data[3] = 0x10;
	data[4] = 0x10;

	this->send(&data[0], 5);

	ESP_LOGI(TAG, "Testing command 0x%02X", this->command);
	data[0] = this->command;
	data[2] = 0x00;
	data[3] = 0xFF;
	data[4] = 0x33;
	this->send(&data[0], 5);
	this->command++;
}

}  // namespace livingcolors1
}  // namespace esphome
