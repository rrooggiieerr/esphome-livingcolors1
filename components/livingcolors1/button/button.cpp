#include "esphome/core/log.h"
#include "button.h"

#include <cinttypes>

namespace esphome {
namespace livingcolors1 {

static const char *const TAG = "livingcolors1.button";

void LivingColors1ButtonComponent::setup() { ESP_LOGCONFIG(TAG, "Setting up Living Colors gen 1 button..."); }

void LivingColors1ButtonComponent::dump_config() {
  LOG_BUTTON("", "Living Colors gen 1 button:", this);
  ESP_LOGCONFIG(TAG, "  Address: 0x%016" PRIX64, this->address_);
}

void LivingColors1PairButton::press_action() {
  ESP_LOGV(TAG, "Pair Button press for address 0x%016" PRIX64, this->special_address_());
  this->parent_->start_pairing(uint32_t(this->address_));

  uint8_t data[5];

  // Command
  data[0] = (uint8_t) Command::PAIRING_REQUEST;

  // HSV Color values
  data[2] = 0x00;
  data[3] = 0x00;
  data[4] = 0x00;

  this->send_(this->special_address_(), &data[0], 5);
}

void LivingColors1LearnButton::press_action() {
  ESP_LOGV(TAG, "Learn Button press");
  this->parent_->start_learning();
}

void LivingColors1UnpairButton::press_action() {
  // Only lamps paired with the ESP's remote ID, the same as Pair uses; not those of a remote control
  ESP_LOGV(TAG, "Unpair Button press");
  this->parent_->unpair(uint32_t(this->address_));
}

void LivingColors1TestButton::press_action() {
  ESP_LOGV(TAG, "Test Button press for address 0x%016" PRIX64, this->address_);

  uint8_t data[5];

  // Command
  data[0] = (uint8_t) Command::ON;
  data[2] = 0x10;
  data[3] = 0x10;
  data[4] = 0x10;

  this->send(&data[0], 5);

  ESP_LOGI(TAG, "Testing command 0x%02X", this->command);
  data[0] = this->command;
  data[2] = 0x00;
  data[3] = 0xFF;
  data[4] = 0x33;
  this->send(&data[0], 5);
  this->command++;
}

}  // namespace livingcolors1
}  // namespace esphome
