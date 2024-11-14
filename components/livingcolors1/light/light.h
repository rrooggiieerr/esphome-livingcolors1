#pragma once

#include "esphome/core/color.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/light/base_light_effects.h"
#include "esphome/components/light/esp_hsv_color.h"
#include "esphome/components/livingcolors1/livingcolors1.h"

namespace esphome {
namespace livingcolors1 {

enum class Command : uint8_t {
	UNKNOWN = 0x01, HSV_VALUE = 0x03, ON = 0x05, OFF = 0x07, CYCLE = 0x12
};

class LivingColors1Light: public Component, public LivingColors1Client, public light::LightOutput {
public:
	void setup() override;

	uint8_t hue = 0;
//	// For testing
//	void loop() {
//		hue++;
//		this->parent_->queue_light(this->address_, Command::ON, hue, 255, 255);
//		delayMicroseconds(500);
//	}
	void dump_config() override;
	void setup_state(light::LightState *state) override;
	void write_state(light::LightState *state) override;
	light::LightTraits get_traits() override {
		auto traits = light::LightTraits();
		traits.set_supported_color_modes( { light::ColorMode::RGB });
		return traits;
	}
	bool receive(uint8_t *data, uint8_t length) override;

protected:
	light::LightState *state_ { nullptr };
	bool receive_ = false;
};

class LivingColors1CycleLightEffect : public light::LightEffect {
public:
	LivingColors1CycleLightEffect()
	: light::LightEffect("Color Cycle") {}

	/// Initialize this LightEffect. Will be called once after creation.
	void start() {}

	void start_internal() { this->start(); }

	/// Called when this effect is about to be removed
	void stop() {}

	/// Apply this effect. Use the provided state for starting transitions, ...
	void apply() {}

	/// Internal method called by the LightState when this light effect is registered in it.
	void init() {}
protected:

};

}
}
