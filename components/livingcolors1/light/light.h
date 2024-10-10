#pragma once

#include "esphome/core/color.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/light/esp_hsv_color.h"
#include "esphome/components/livingcolors1/livingcolors1.h"

namespace esphome {
namespace livingcolors1 {

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
	bool receive_command(Command command, uint8_t hue, uint8_t saturation, uint8_t value) override;

protected:
	light::LightState *state_ { nullptr };
};

}
}
