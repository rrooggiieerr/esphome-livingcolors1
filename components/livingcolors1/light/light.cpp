#include "esphome/core/log.h"
#include "light.h"

#include <cinttypes>

namespace esphome {
namespace livingcolors1 {

static const char *TAG = "livingcolors1.light";

void LivingColors1Light::setup() {
	ESP_LOGCONFIG(TAG, "Setting up Living Colors gen 1 light...");
}

void LivingColors1Light::dump_config() {
	ESP_LOGCONFIG(TAG, "Living Colors gen 1 light:");
	ESP_LOGCONFIG(TAG, "  Address: 0x%016" PRIX64, this->address);
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
	} else {
		command = Command::OFF;
		hue = 0;
		saturation = 0;
		value = 0;
	}

	this->parent_->set_light(this->address, command, hue, saturation, value);
}

bool LivingColors1Light::receive_command(Command command, uint8_t hue, uint8_t saturation, uint8_t value) {
	if(command == Command::OFF) {
		auto call = this->state_->make_call();
		call.set_state(false);
		call.perform();
	} else {
		int _hue = int(round(float(hue) / (255.0 / 360.0)));
		float _saturation = float(saturation) / 255;
		float _value = float(value) / 255;

		ESP_LOGV(TAG, "hue: %d", hue);
		ESP_LOGV(TAG, "_hue: %d", _hue);
		ESP_LOGV(TAG, "saturation: %d", saturation);
		ESP_LOGV(TAG, "_saturation: %f", _saturation);
		ESP_LOGV(TAG, "value: %d", value);
		ESP_LOGV(TAG, "_value: %f", _value);

		float red, green, blue;
		hsv_to_rgb(_hue, _saturation, _value, red, green, blue);
		ESP_LOGV(TAG, "red: %f", red);
		ESP_LOGV(TAG, "green: %f", green);
		ESP_LOGV(TAG, "blue: %f", blue);

		auto call = this->state_->make_call();
		call.set_state(true);
		call.set_rgb(red, green, blue);
		call.perform();
	}

	return true;
}

}
}
