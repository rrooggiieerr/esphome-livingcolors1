#include "livingcolors1.h"

#include "esphome/core/log.h"
#include "esphome/components/cc2500/cc2500.h"

#include <cinttypes>

namespace esphome {
namespace livingcolors1 {

static const char *TAG = "livingcolors1";

void LivingColors1::dump_config() {
	ESP_LOGCONFIG(TAG, "Living Colors 1st generation component:");
//	if (this->command_repeats_.has_value())
//		ESP_LOGCONFIG(TAG, "  Command repeats: %d", this->command_repeats_.value());
}

void LivingColors1::set_light(uint64_t address, Command command, uint8_t hue,
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

}
}
