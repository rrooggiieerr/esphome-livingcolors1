#include "esphome/core/log.h"
#include "light.h"

#include <cinttypes>

namespace esphome {
namespace livingcolors1 {

static const char *const TAG = "livingcolors1.light";

// Keeps the stored address apart from other preferences
static const uint32_t ADDRESS_PREFERENCE_VERSION = 0x4C433141;

void LivingColors1Light::setup() {
  // Lights without address are hidden, keep them out of the log too
  if (this->has_address_)
    ESP_LOGCONFIG(TAG, "Setting up Living Colors gen 1 light...");
}

void LivingColors1Light::dump_config() {
  if (!this->has_address_)
    return;
  ESP_LOGCONFIG(TAG, "Living Colors gen 1 light:");
  ESP_LOGCONFIG(TAG, "  Address: 0x%016" PRIX64, this->address_);
  ESP_LOGCONFIG(TAG, "  Send repeats: %d", this->send_repeats_);
}

void LivingColors1LightState::dump_config() {
  if (static_cast<LivingColors1Light *>(this->get_output())->has_address())
    light::LightState::dump_config();
}

void LivingColors1Light::setup_state(light::LightState *state) {
  state_ = state;
  state_->set_default_transition_length(0);
  // Per light, not static: add_effects() keeps the pointer and the effect must not be shared
  auto **effects = new light::LightEffect *[1]{new LivingColors1CycleLightEffect()};
  state_->add_effects(effects, 1);

  // Without address in the YAML, use the stored one; hide the light until it's paired
  if (!this->has_address_) {
    this->pairable_ = true;
    this->address_pref_ =
        global_preferences->make_preference<uint64_t>(this->address_key_ ^ ADDRESS_PREFERENCE_VERSION, true);
    uint64_t address;
    // 0 is stored when the lamp was unpaired
    if (this->address_pref_.load(&address) && address != 0)
      this->set_address(address);
    else
      state->set_internal(true);
  }
}

bool LivingColors1Light::store_address(uint64_t address) {
  if (!this->pairable_ || this->has_address_)
    return false;
  if (!this->address_pref_.save(&address) || !global_preferences->sync()) {
    ESP_LOGE(TAG, "Failed to store address 0x%016" PRIX64, address);
    return false;
  }
  this->set_address(address);
  return true;
}

bool LivingColors1Light::replace_address(uint64_t address) {
  // Same lamp ID, the upper 4 bytes; only for a stored address, one in the YAML can't be changed
  if (!this->pairable_ || !this->has_address_ || (this->address_ >> 32) != (address >> 32))
    return false;
  if (!this->address_pref_.save(&address) || !global_preferences->sync()) {
    ESP_LOGE(TAG, "Failed to store address 0x%016" PRIX64, address);
    return false;
  }
  // The light is shown already, so no reboot is needed
  ESP_LOGI(TAG, "Replaced 0x%016" PRIX64 " with 0x%016" PRIX64, this->address_, address);
  this->set_address(address);
  return true;
}

void LivingColors1Light::unpair(uint32_t remote_id) {
  // Lamps paired with a remote control aren't owned by the ESP
  if (!this->has_address_ || uint32_t(this->address_) != remote_id)
    return;
  ESP_LOGD(TAG, "Unpairing request to 0x%016" PRIX64, this->address_);
  uint8_t data[5] = {(uint8_t) Command::UNPAIRING_REQUEST, 0x00, 0x00, 0x00, 0x00};
  this->send(data, 5);
}

void LivingColors1Light::forget_address_() {
  if (!this->pairable_) {
    // The address is in the YAML, so the light stays
    ESP_LOGI(TAG, "Unpaired 0x%016" PRIX64 ", remove the light from the YAML", this->address_);
    return;
  }
  // Without address nothing is sent, also not the state reset below
  this->has_address_ = false;
  // An unpaired lamp is off, and so is a lamp after pairing; don't restore the old state on re-pairing
  auto call = this->state_->turn_off();
  call.set_rgb(1.0f, 1.0f, 1.0f);
  call.set_brightness(1.0f);
  call.set_save(true);
  call.perform();
  uint64_t none = 0;
  if (!this->address_pref_.save(&none) || !global_preferences->sync()) {
    // Keep the light working, it's still stored
    this->has_address_ = true;
    ESP_LOGE(TAG, "Failed to forget address 0x%016" PRIX64, this->address_);
    return;
  }
  ESP_LOGI(TAG, "Unpaired 0x%016" PRIX64 ", rebooting", this->address_);
  this->parent_->schedule_reboot();
}

void LivingColors1Light::write_state(light::LightState *state) {
  if (!this->has_address_)
    return;
  if (this->receive_ == true) {
    ESP_LOGV(TAG, "Receive mode, don't write state to light");
    this->receive_ = false;
    return;
  }
  // The Color Cycle effect sends ON and CYCLE itself; sending ON here would replace the queued CYCLE
  if (state->get_effect_name() == "Color Cycle") {
    ESP_LOGV(TAG, "Color cycling, don't write state to light");
    return;
  }

  float brightness;
  state->current_values_as_brightness(&brightness);

  Command command;
  uint8_t hue, saturation, value;
  if (brightness > 0.0) {
    float red, green, blue;
    state->current_values_as_rgb(&red, &green, &blue);

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

  ESP_LOGV(TAG, "Setting light on address 0x%016" PRIX64 " to 0x%02X 0x%02X 0x%02X 0x%02X", this->address_,
           (uint8_t) command, hue, saturation, value);

  uint8_t data[5];

  // Command
  data[0] = (uint8_t) command;

  // HSV Color values
  data[2] = hue;
  data[3] = saturation;
  data[4] = value;

  this->send(data, 5);
}

bool LivingColors1Light::receive(uint64_t address, uint8_t *data, uint8_t length) {
  if (!this->has_address_ || length != 5)
    return false;

  // Command
  Command command = (Command) data[0];

  if (this->is_special_(address) && command == Command::PAIRING_REQUEST) {
    ESP_LOGV(TAG, "Pairing request: 0x%016" PRIX64, address);
    return true;
  }

  if (this->is_response_(address) && command == Command::PAIRING_RESPONSE) {
    ESP_LOGD(TAG, "Pairing response: 0x%016" PRIX64, address);
    return true;
  }

  if (this->is_address_(address) && command == Command::CYCLE)
    ESP_LOGD(TAG, "Color cycling request");
  else if (this->is_special_(address) && command == Command::CYCLE_SYNC)
    ESP_LOGD(TAG, "Color cycling sync");
  else if (this->is_address_(address) && command == Command::UNPAIRING_REQUEST) {
    ESP_LOGI(TAG, "Unpairing request for 0x%016" PRIX64, this->address_);
    return true;
  } else if (this->is_response_(address) && command == Command::ACK_UNPAIRING) {
    this->forget_address_();
    return true;
  } else if (this->is_response_(address)) {
    ESP_LOGD(TAG, "Response");
    ESP_LOGD(TAG, "Command : 0x%02X", (uint8_t) command);
  } else if (!this->is_address_(address))
    return false;

  // HSV Color values
  uint8_t hue = data[2];
  uint8_t saturation = data[3];
  uint8_t value = data[4];

  ESP_LOGD(TAG, "Received command 0x%02X for address 0x%016" PRIX64 " HSV: 0x%02X 0x%02X 0x%02X", (uint8_t) command,
           this->address_, hue, saturation, value);

  int _hue = int(round(float(hue) / (255.0 / 360.0)));
  float _saturation = float(saturation) / 255;
  float _value = float(value) / 255;
  float red, green, blue;
  hsv_to_rgb(_hue, _saturation, _value, red, green, blue);

  if (command == Command::OFF || command == Command::ACK_OFF) {
    this->receive_ = true;
    if (this->state_->get_effect_name() == "Color Cycle") {
      auto call = this->state_->make_call();
      call.set_effect("none");
      call.set_save(false);

      call.perform();
    }

    auto call = this->state_->turn_off();

    call.perform();
    return true;
  } else if (command == Command::CYCLE || command == Command::ACK_CYCLE || command == Command::CYCLE_SYNC) {
    this->receive_ = true;

    auto call = this->state_->make_call();
    call.set_state(true);
    call.set_rgb(red, green, blue);
    call.set_brightness(_value);
    call.set_effect("Color Cycle");
    call.set_save(false);

    call.perform();
    return true;
  } else if (this->state_->get_effect_name() == "Color Cycle") {
    ESP_LOGD(TAG, "Light is color cycling, can't set color");
    return true;
  } else if (command == Command::ON || command == Command::ACK_ON) {
    this->receive_ = true;

    auto call = this->state_->turn_on();
    call.set_rgb(red, green, blue);
    call.set_brightness(_value);
    if (command == Command::ON)
      call.set_effect("none");

    call.perform();
    return true;
  } else if (!this->state_->current_values.is_on()) {
    ESP_LOGD(TAG, "Light is off, can't set color");
    return true;
  } else if (command == Command::HSV || command == Command::ACK_HSV) {
    this->receive_ = true;

    auto call = this->state_->make_call();
    call.set_state(true);
    call.set_rgb(red, green, blue);
    call.set_brightness(_value);
    call.set_effect("none");

    call.perform();
    return true;
  }

  ESP_LOGE(TAG, "Received unknown command 0x%02X", (uint8_t) command);
  return false;
}

void LivingColors1CycleLightEffect::apply() {
  if (!this->applied_) {
    ESP_LOGV(TAG, "LivingColors1CycleLightEffect::apply");

    LivingColors1Light *output = (LivingColors1Light *) this->state_->get_output();
    Command command;
    uint8_t hue, saturation, value;
    float red, green, blue;
    this->state_->current_values_as_rgb(&red, &green, &blue);

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

    data[0] = (uint8_t) Command::CYCLE;
    output->send(&data[0], 5, true);

    this->applied_ = true;
  }
}

void LivingColors1CycleLightEffect::stop() {
  ESP_LOGV(TAG, "LivingColors1CycleLightEffect::stop");
  LivingColors1Light *output = (LivingColors1Light *) this->state_->get_output();
  this->applied_ = false;
  // Stopped by unpairing, the lamp no longer listens
  if (!output->has_address())
    return;
  uint8_t data[5];

  // Command
  data[0] = (uint8_t) Command::OFF;

  // HSV Color values
  data[2] = 0x00;
  data[3] = 0x00;
  data[4] = 0x00;

  output->send(&data[0], 5);
}

}  // namespace livingcolors1
}  // namespace esphome
