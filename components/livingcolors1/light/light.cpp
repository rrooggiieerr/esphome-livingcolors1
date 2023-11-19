#include "esphome/core/log.h"
#include "light.h"

namespace esphome {
namespace livingcolors1 {

static const char *TAG = "livingcolors1.light";

void LivingColors1Light::setup() {
	ESP_LOGCONFIG(TAG, "Setting up Living Colors gen 1 light...");
}

void LivingColors1Light::dump_config() {
	ESP_LOGCONFIG(TAG, "Living Colors gen 1 light:");ESP_LOGCONFIG(TAG, "  Address: 0x%016" PRIX64, this->address_);
}

void LivingColors1Light::setup_state(light::LightState *state) {
	state_ = state;
	state_->set_gamma_correct(0);
	state_->set_default_transition_length(0);
}

void LivingColors1Light::write_state(light::LightState *state) {
	float brightness;
	state->current_values_as_brightness(&brightness);

	Command command;
	uint8_t hue, saturation, value;
	if (brightness > 0.0) {
		float red, green, blue;
		state->current_values_as_rgb(&red, &green, &blue, false);

		int _hue;
		float _saturation, _value;
		rgb_to_hsv(red, green, blue, _hue, _saturation, _value);

		command = Command::ON;
		hue = (uint8_t) lroundf(_hue * (255.0 / 360.0));
		saturation = (uint8_t) lroundf(_saturation * 255.0);
		value = (uint8_t) lroundf(_value * 255.0);

		ESP_LOGD(TAG, "_hue: %d", _hue);
		ESP_LOGD(TAG, "hue: %f", (_hue * (255.0/360.0)));
		ESP_LOGD(TAG, "hue: %d", hue);
		ESP_LOGD(TAG, "_saturation: %f", _saturation);
		ESP_LOGD(TAG, "saturation: %d", saturation);
		ESP_LOGD(TAG, "_value: %f", _value); ESP_LOGD(TAG, "value: %d", value);
	} else {
		command = Command::OFF;
		hue = 0;
		saturation = 0;
		value = 0;
	}

	this->parent_->queue_light(this->address_, command, hue, saturation, value);
}

}
}
