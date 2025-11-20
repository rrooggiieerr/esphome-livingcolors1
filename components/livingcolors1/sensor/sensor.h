#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/cc2500/cc2500.h"
#include "esphome/components/livingcolors1/livingcolors1.h"
//#include "../livingcolors1.h"

namespace esphome {
namespace livingcolors1 {

class LivingColors1Sensor: public LivingColors1ClientComponent, public sensor::Sensor {
public:
	void setup() override;
	void dump_config() override;
};

class LivingColors1RSSISensor: public LivingColors1Sensor {
public:
	bool receive(uint64_t address, uint8_t *data, uint8_t length, float rssi, uint8_t lqi) override;
};

class LivingColors1LQISensor: public LivingColors1Sensor {
public:
	bool receive(uint64_t address, uint8_t *data, uint8_t length, float rssi, uint8_t lqi) override;
};

}  // namespace livingcolors1
}  // namespace esphome
