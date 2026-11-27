#pragma once

#include "esphome/core/color.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/preferences.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/light/base_light_effects.h"
#include "esphome/components/light/esp_hsv_color.h"
#include "esphome/components/livingcolors1/livingcolors1.h"

namespace esphome {
namespace livingcolors1 {

class LivingColors1Light : public LivingColors1ClientComponent, public light::LightOutput {
 public:
  void setup() override;

  void dump_config() override;
  void setup_state(light::LightState *state) override;
  void write_state(light::LightState *state) override;
  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::RGB});
    return traits;
  }
  bool receive(uint64_t address, uint8_t *data, uint8_t length) override;
  bool can_store_address() const override { return this->pairable_; }
  bool store_address(uint64_t address) override;
  bool replace_address(uint64_t address) override;
  void unpair(uint32_t remote_id) override;
  void set_address_key(uint32_t key) { this->address_key_ = key; }

 protected:
  light::LightState *state_{nullptr};
  bool receive_ = false;
  // Set for a light without address in the YAML, it uses the address stored when pairing
  bool pairable_ = false;
  // Hash of the light's id, so the light can be renamed without losing its address
  uint32_t address_key_ = 0;
  ESPPreferenceObject address_pref_;

  void forget_address_();
};

/// Leaves lights without address out of the config log, they are hidden
class LivingColors1LightState : public light::LightState {
 public:
  using light::LightState::LightState;
  void dump_config() override;
};

class LivingColors1CycleLightEffect : public light::LightEffect {
 public:
  LivingColors1CycleLightEffect() : light::LightEffect("Color Cycle") {}

  /// Called when this effect is about to be removed
  void stop() override;

  /// Apply this effect. Use the provided state for starting transitions, ...
  void apply() override;

  /// Internal method called by the LightState when this light effect is registered in it.
  void init() {}

 protected:
  bool applied_ = false;
};

}  // namespace livingcolors1
}  // namespace esphome
