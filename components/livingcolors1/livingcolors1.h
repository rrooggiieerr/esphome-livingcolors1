#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cc2500/cc2500.h"

namespace esphome {
namespace livingcolors1 {

enum class Command : uint8_t {
	UNKNOWN = 0x01, HSV_VALUE = 0x03, ON = 0x05, OFF = 0x07
};

class LivingColors1Client;

class LivingColors1Component: public Component, public cc2500::CC2500Device<0, 3> {
public:
	void setup() override;

	void dump_config() override;
	bool receive_command(uint8_t *data, uint8_t length) override;

//	void set_command_repeats(uint16_t n_times) {
//		this->command_repeats_ = n_times;
//	}

	void add_device(LivingColors1Client *device) { this->devices_.push_back(device); }
	void set_light(uint64_t address, Command command, uint8_t hue,
			uint8_t saturation, uint8_t value);

protected:
//	optional<uint16_t> command_repeats_;
	std::vector<LivingColors1Client *> devices_;

	uint8_t serial_number_ = 0;
};

class LivingColors1Client {
public:
	LivingColors1Client() {}

	void set_parent(LivingColors1Component *parent);
	void set_address(uint64_t address) {
		this->address = address;
	}
	void set_light(Command command, uint8_t hue,
			uint8_t saturation, uint8_t value);
	virtual bool receive_command(Command command, uint8_t hue,
			uint8_t saturation, uint8_t value);
	uint64_t address;
protected:
	LivingColors1Component *parent_ { nullptr };
};

}
}
