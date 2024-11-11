#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cc2500/cc2500.h"

namespace esphome {
namespace livingcolors1 {

class LivingColors1Client;

class LivingColors1Component: public Component, public cc2500::CC2500Device<0, 3> {
public:
	void setup() override;

//	void dump_config() override;
	bool receive(uint8_t *data, uint8_t length) override;

	void add_device(LivingColors1Client *device) { this->devices_.push_back(device); }
	void send(uint8_t *data, uint8_t length);
protected:
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
	void set_send_repeats(uint16_t n_times) {
		this->send_repeats_ = n_times;
	}
	void send(uint8_t *data, uint8_t length);
	virtual bool receive(uint8_t *data, uint8_t length);
	uint64_t address;
protected:
	LivingColors1Component *parent_ { nullptr };
	uint16_t send_repeats_ = 7;
};

}
}
