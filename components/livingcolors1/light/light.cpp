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
	ESP_LOGCONFIG(TAG, "  Address: 0x%016" PRIX64, this->address_);
	ESP_LOGCONFIG(TAG, "  Send repeats: %d", this->send_repeats_);
}

void LivingColors1Light::setup_state(light::LightState *state) {
	state_ = state;
	state_->set_gamma_correct(0);
	state_->set_default_transition_length(0);
	std::vector<light::LightEffect *> effects;
	effects.push_back(new LivingColors1CycleLightEffect());
	state_->add_effects(effects);
}

void LivingColors1Light::write_state(light::LightState *state) {
	if (this->receive_ == true) {
		ESP_LOGV(TAG, "Receive mode, don't write state");
		this->receive_ = false;
		return;
	}

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

	ESP_LOGV(TAG, "Setting light on address 0x%016" PRIX64 " to 0x%02X 0x%02X 0x%02X 0x%02X", this->address_, (uint8_t) command, hue, saturation, value);
	uint8_t data[5];

	// Command
	data[0] = (uint8_t) command;

	// Counter
	// data[1] = 0;

	// HSV Color values
	data[2] = hue;
	data[3] = saturation;
	data[4] = value;

	this->send(data, 5);
}

bool LivingColors1Light::receive(uint64_t address, uint8_t *data, uint8_t length) {
	if (length != 5)
		return false;

	// Command
	Command command = (Command) data[0];

	if(this->is_special_(address) && command == Command::PAIRING_REQUEST) {
		ESP_LOGV(TAG, "Pairing request: 0x%016" PRIX64, address);
		return true;
	}

	if(this->is_response_(address) && command == Command::PAIRING_RESPONSE) {
		ESP_LOGD(TAG, "Pairing response: 0x%016" PRIX64, address);
		return true;
	}

	if(this->is_address_(address) && command == Command::CYCLE_REQUEST)
		ESP_LOGE(TAG, "Color cycling request");
	else if(this->is_response_(address) && command == Command::ACK)
		ESP_LOGD(TAG, "Acknowledge");
	else if(this->is_special_(address) && command == Command::CYCLE_SYNC)
		ESP_LOGD(TAG, "Color cycling sync");
	else if (!this->is_address_(address))
		return false;

	// HSV Color values
	uint8_t hue = data[2];
	uint8_t saturation = data[3];
	uint8_t value = data[4];

	ESP_LOGD(TAG, "Received command 0x%02X for address 0x%016" PRIX64 " HSV: 0x%02X 0x%02X 0x%02X", command, this->address_, hue, saturation, value);

	int _hue = int(round(float(hue) / (255.0 / 360.0)));
	float _saturation = float(saturation) / 255;
	float _value = float(value) / 255;
	float red, green, blue;
	hsv_to_rgb(_hue, _saturation, _value, red, green, blue);

	if(command == Command::OFF) {
		this->receive_ = true;
		auto call = this->state_->turn_off();
		call.set_effect("none");
		call.perform();
		return true;
	} else if(command == Command::ON) {
		this->receive_ = true;

		auto call = this->state_->turn_on();
		call.set_rgb(red, green, blue);
		call.set_brightness(_value);
		call.set_effect("none");

		call.perform();
		return true;
	} else if(command == Command::ACK) {
		this->receive_ = true;

		auto call = this->state_->make_call();
		call.set_state(true);
		call.set_rgb(red, green, blue);
		call.set_brightness(_value);
		call.set_save(false);

		call.perform();
		return true;
	} else if(command == Command::CYCLE_REQUEST || command == Command::CYCLE_SYNC) {
		this->receive_ = true;

		auto call = this->state_->make_call();
		call.set_state(true);
		call.set_rgb(red, green, blue);
		call.set_brightness(_value);
		call.set_effect("Color Cycle");
		call.set_save(false);

		call.perform();
		return true;
	} else if(!this->state_->current_values.is_on()) {
		ESP_LOGD(TAG, "Light is off");
		return true;
	} else if(this->state_->get_effect_name() == "Color Cycle" && command == Command::HSV_VALUE) {
		ESP_LOGD(TAG, "Light is color cycling, can't set color");
		return true;
	} else if (command == Command::HSV_VALUE) {
		this->receive_ = true;

		auto call = this->state_->make_call();
		call.set_state(true);
		call.set_rgb(red, green, blue);
		call.set_brightness(_value);
		call.set_effect("none");

		call.perform();
		return true;
	}

	ESP_LOGE(TAG, "Received unknown command 0x%02X", command);
	return false;
}

void LivingColors1CycleLightEffect::apply() {
	if(!this->applied_) {
		ESP_LOGV(TAG, "LivingColors1CycleLightEffect::apply");

		LivingColors1Light *output = (LivingColors1Light*) this->state_->get_output();
		Command command;
		uint8_t hue, saturation, value;
		float red, green, blue;
		this->state_->current_values_as_rgb(&red, &green, &blue, false);

		int _hue;
		float _saturation, _value;
		rgb_to_hsv(red, green, blue, _hue, _saturation, _value);

		hue = (uint8_t) lroundf(_hue * (255.0 / 360.0));
		saturation = (uint8_t) lroundf(_saturation * 255.0);
		value = (uint8_t) lroundf(_value * 255.0);

		uint8_t data[5];

		// Command
		data[0] = (uint8_t) Command::ON;

		// HSV Color values
		data[2] = hue;
		data[3] = saturation;
		data[4] = value;

		output->send(&data[0], 5);

		data[0] = (uint8_t) Command::CYCLE_REQUEST;
		output->send(&data[0], 5);

		this->applied_ = true;
	}
}

}
}
