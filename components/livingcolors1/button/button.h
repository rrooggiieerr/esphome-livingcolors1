#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/button/button.h"
#include "../livingcolors1.h"

namespace esphome {
namespace livingcolors1 {

class LivingColors1ButtonComponent: public LivingColors1ClientComponent, public button::Button {
public:
	void setup() override;
	void dump_config() override;

protected:
};

class LivingColors1PairButton: public LivingColors1ButtonComponent {
public:
	bool receive(uint64_t address, uint8_t *data, uint8_t length) override;

protected:
	void press_action() override;
};

class LivingColors1UnpairButton: public LivingColors1ButtonComponent {
protected:
	void press_action() override;
};

class LivingColors1TestButton: public LivingColors1ButtonComponent {
public:
protected:
	void press_action() override;
	uint8_t command = 0x00;
};

}  // namespace livingcolors1
}  // namespace esphome
