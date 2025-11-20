#include "esphome/core/log.h"
#include "sensor.h"

#include <cinttypes>

namespace esphome {
namespace livingcolors1 {

static const char *const TAG = "livingcolors1.sensor";

void LivingColors1Sensor::setup() {
	ESP_LOGCONFIG(TAG, "Setting up Living Colors gen 1 sensor...");
}

void LivingColors1Sensor::dump_config() {
	LOG_SENSOR("", "Living Colors gen 1 sensor:", this);
	ESP_LOGCONFIG(TAG, "  Address: 0x%016" PRIX64, this->address_);
}

bool LivingColors1RSSISensor::receive(uint64_t address, uint8_t *data, uint8_t length, float rssi, uint8_t lqi) {
	if(this->is_response_(address)) {
		ESP_LOGD(TAG, "RSSI: %d", rssi);
		this->publish_state(rssi);

		return true;
	}

	return false;
}

bool LivingColors1LQISensor::receive(uint64_t address, uint8_t *data, uint8_t length, float rssi, uint8_t lqi) {
	if(this->is_response_(address)) {
		ESP_LOGD(TAG, "LQI: %d", lqi);
		this->publish_state(lqi);

		return true;
	}

	return false;
}

}  // namespace livingcolors1
}  // namespace esphome
