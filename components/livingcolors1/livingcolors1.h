#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cc2500/cc2500.h"

namespace esphome {
namespace livingcolors1 {

enum class Command : uint8_t {
	UNKNOWN = 0x01, HSV_VALUE = 0x03, ON = 0x05, OFF = 0x07
};

class LivingColors1: public Component, public cc2500::CC2500Device<0, 3> {
public:
	void dump_config() override;

//	void set_command_repeats(uint16_t n_times) {
//		this->command_repeats_ = n_times;
//	}

	void set_light(uint64_t address, Command command, uint8_t hue,
			uint8_t saturation, uint8_t value);

protected:
//	optional<uint16_t> command_repeats_;

	uint8_t serial_number_ = 0;
};

}
}
