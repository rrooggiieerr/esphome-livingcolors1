#include "livingcolors1.h"

#include "esphome/core/application.h"
#include "esphome/core/log.h"
#include "esphome/components/cc2500/cc2500.h"

#include <cinttypes>

namespace esphome {
namespace livingcolors1 {

static const char *const TAG = "livingcolors1";

// Commands the lamp acknowledges with the command code + 1
static bool expects_acknowledgement(uint8_t command) {
  return command == (uint8_t) Command::HSV || command == (uint8_t) Command::ON || command == (uint8_t) Command::OFF ||
         command == (uint8_t) Command::CYCLE;
}

void LivingColors1Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Living Colors 1st generation component...");

  uint8_t data[15];
  memset(data, 0, sizeof(data));
  data[0] = 0x0E;
  data[9] = 0x11;

  ((CC2500Device *) this)->send(data, 15);
}

bool LivingColors1Component::receive(uint8_t *data, uint8_t length, float rssi, uint8_t lqi) {
  ESP_LOGV(TAG, "LivingColors1Component::receive");
  // Validate data length, 17 bytes
  if (length != 17) {
    // Invalid data length can happen when data from other cc2500 devices is received.
    ESP_LOGV(TAG, "Invalid data length: %d", length);
    return false;
  }

  // Validate packet length, 14 bytes
  if (data[0] != 14) {
    ESP_LOGD(TAG, "Invalid packet length: %d", data[0]);
    return false;
  }

  // Validate fixed value
  if (data[9] != 0x11) {
    ESP_LOGD(TAG, "Invalid fixed value: 0x%02X", data[9]);
    return false;
  }

  // Get the address
  uint64_t address = 0;
  for (int i = 1; i <= 8; i++) {
    address += uint64_t(data[i]) << ((8 - i) * 8);
  }
  ESP_LOGV(TAG, "address: 0x%016" PRIX64, address);

  // Get the payload
  uint64_t payload = 0;
  for (int i = 3; i <= 8; i++) {
    payload += uint64_t(data[i + 6]) << ((8 - i) * 8);
  }
  ESP_LOGV(TAG, "payload: 0x%012" PRIX64, payload);

  // Command
  Command command = (Command) data[10];

  // Serial number
  uint8_t serial_number = data[11];
  ESP_LOGV(TAG, "serial number: %d", serial_number);

  for (auto device : this->devices_)
    device->check_acknowledgement(address, command);

  // Check if the address is handled by a device
  bool success = false;
  for (auto device : this->devices_) {
    if (device->receive(address, &data[10], 5, rssi, lqi))
      success = true;
  }

  if (!success && (command == Command::HSV || command == Command::ON || command == Command::OFF)) {
    // If the address is not yet handled, log the address as detected
    this->log_detected_address_(address);
  }
  if (!success)
    this->learn_(address, command);
  if (!success && command == Command::PAIRING_RESPONSE) {
    // If the address is a pairing response swap the light and remote address parts and show as detected address
    uint64_t light_address = swap_address(address);
    this->log_detected_address_(light_address);
    this->pair_(light_address);
  }
  return true;
}

void LivingColors1Component::log_detected_address_(uint64_t address) {
  // Log each address once
  for (uint64_t logged : this->detected_addresses_) {
    if (logged == address)
      return;
  }
  if (this->detected_addresses_.size() < MAX_DETECTED_ADDRESSES)
    this->detected_addresses_.push_back(address);
  ESP_LOGI(TAG, "Address detected: 0x%016" PRIX64, address);
}

bool LivingColors1Component::can_store_() {
  for (auto *device : this->devices_) {
    if (device->can_store_address())
      return true;
  }
  return false;
}

void LivingColors1Component::start_pairing(uint32_t remote_id) {
  // Without lights that store addresses, e.g. on ESP8266, pairing only logs the detected address
  if (!this->can_store_())
    return;
  ESP_LOGI(TAG, "Pairing for %" PRIu32 " seconds", PAIRING_TIMEOUT_MS / 1000);
  this->pairing_ = true;
  this->pairing_remote_id_ = remote_id;
  this->set_timeout("pairing", PAIRING_TIMEOUT_MS, [this]() {
    this->pairing_ = false;
    ESP_LOGI(TAG, "Pairing ended");
  });
}

void LivingColors1Component::pair_(uint64_t address) {
  // Only right after pressing Pair, and only for a lamp answering our remote ID. Every lamp close enough
  // answers, so keep storing until the timeout; a stored lamp's answers are handled by its light.
  if (!this->pairing_ || uint32_t(address) != this->pairing_remote_id_)
    return;
  this->store_(address);
}

void LivingColors1Component::schedule_reboot() {
  // A light can only be shown or hidden during boot; wait for more lamps answering the same command
  this->set_timeout("reboot", 2000, []() { App.safe_reboot(); });
}

void LivingColors1Component::unpair(uint32_t remote_id) {
  for (auto *device : this->devices_)
    device->unpair(remote_id);
}

void LivingColors1Component::start_learning() {
  if (!this->can_store_()) {
    ESP_LOGW(TAG, "No lights to store learned addresses in");
    return;
  }
  ESP_LOGI(TAG, "Learning, use the remote control within %" PRIu32 " seconds", LEARNING_TIMEOUT_MS / 1000);
  this->learning_ = true;
  this->learn_candidates_length_ = 0;
  this->set_timeout("learning", LEARNING_TIMEOUT_MS, [this]() {
    this->learning_ = false;
    ESP_LOGI(TAG, "Learning ended");
  });
}

void LivingColors1Component::learn_(uint64_t address, Command command) {
  if (!this->learning_)
    return;
  uint8_t code = (uint8_t) command;

  // Only store lamps that answer, a remote may still send to lamps that no longer exist. The answer has the
  // address halves swapped and the command + 1, or OFF for OFF when the lamp is already off.
  uint64_t light_address = swap_address(address);
  for (uint8_t i = 0; i < this->learn_candidates_length_; i++) {
    LearnCandidate &candidate = this->learn_candidates_[i];
    if (candidate.address == light_address && (code == candidate.command + 1 || code == candidate.command)) {
      candidate = this->learn_candidates_[--this->learn_candidates_length_];
      this->store_(light_address);
      return;
    }
  }

  // A command from a remote, remember it until the lamp answers
  if (command != Command::HSV && command != Command::ON && command != Command::OFF)
    return;
  for (uint8_t i = 0; i < this->learn_candidates_length_; i++) {
    if (this->learn_candidates_[i].address == address) {
      this->learn_candidates_[i].command = code;
      return;
    }
  }
  if (this->learn_candidates_length_ < MAX_LEARN_CANDIDATES)
    this->learn_candidates_[this->learn_candidates_length_++] = {address, code};
}

void LivingColors1Component::store_(uint64_t address) {
  // A lamp is paired with one remote, so a stored address with the same lamp ID is outdated
  for (auto *device : this->devices_) {
    if (device->replace_address(address))
      return;
  }
  for (auto *device : this->devices_) {
    if (device->store_address(address)) {
      ESP_LOGI(TAG, "Stored 0x%016" PRIX64 ", rebooting", address);
      this->schedule_reboot();
      return;
    }
  }
  ESP_LOGW(TAG, "No light without address to store 0x%016" PRIX64, address);
}

void LivingColors1Component::send(uint8_t *data, uint8_t length) {
  uint8_t data_copy[length];
  data[11] = this->serial_number_++;
  memcpy(data_copy, data, length);
  ((CC2500Device *) this)->send(data_copy, length);
}

LivingColors1ClientComponent::LivingColors1ClientComponent() {
  // 12 lowercase hex characters, no separators
  char mac_buf[MAC_ADDRESS_BUFFER_SIZE];
  get_mac_address_into_buffer(mac_buf);
  std::string mac_address(mac_buf);
  std::string address = "FFFFFFFF" + mac_address.substr(mac_address.length() - 8, 8);
  this->address_ = std::strtoull(address.c_str(), nullptr, 16);
}

void LivingColors1ClientComponent::loop() {
  // Nothing to send; send_() enables the loop again
  if (this->send_queue_length_ == 0) {
    this->disable_loop();
    return;
  }
  // Wait between repeats without blocking, so an acknowledgement can be received meanwhile
  if (this->send_repeat_counter_ > 0 && millis() - this->last_send_ < SEND_INTERVAL_MS)
    return;
  this->send_();
}

void LivingColors1ClientComponent::set_parent(LivingColors1Component *parent) {
  this->parent_ = parent;
  this->parent_->add_device(this);
}

void LivingColors1ClientComponent::send_(uint64_t address, uint8_t *data, uint8_t length, bool append) {
  uint8_t length_ = 10 + length;
  if (length_ > sizeof(SendPacket::data)) {
    ESP_LOGE(TAG, "Packet too long (%u bytes), not sending", length_);
    return;
  }
  // A new command replaces pending ones, unless it's appended
  if (!append)
    this->send_queue_length_ = 0;
  if (this->send_queue_length_ >= SEND_QUEUE_SIZE) {
    ESP_LOGW(TAG, "Send queue full, dropping command");
    return;
  }
  SendPacket &packet = this->send_queue_[this->send_queue_length_];
  uint8_t *data_ = packet.data;

  // Packet length
  data_[0] = 10 + length - 1;

  // Addresses
  data_[1] = uint8_t(address >> 8 * 7);
  data_[2] = uint8_t(address >> 8 * 6);
  data_[3] = uint8_t(address >> 8 * 5);
  data_[4] = uint8_t(address >> 8 * 4);
  data_[5] = uint8_t(address >> 8 * 3);
  data_[6] = uint8_t(address >> 8 * 2);
  data_[7] = uint8_t(address >> 8 * 1);
  data_[8] = uint8_t(address >> 8 * 0);

  // Fixed
  data_[9] = 0x11;

  // Command
  for (int j = 0; j < length; j++)
    data_[10 + j] = data[j];

  packet.length = length_;
  this->send_queue_length_++;
  this->enable_loop();

  // Start right away when nothing else is being repeated
  if (this->send_queue_length_ == 1) {
    this->send_repeat_counter_ = 0;
    this->send_();
  }
}

void LivingColors1ClientComponent::send_() {
  if (this->send_queue_length_ == 0)
    return;

  if (this->send_repeat_counter_ < this->send_repeats_) {
    ESP_LOGD(TAG, "Sending...");
    this->parent_->send(this->send_queue_[0].data, this->send_queue_[0].length);
    this->last_send_ = millis();
    this->send_repeat_counter_++;
  }

  if (this->send_repeat_counter_ >= this->send_repeats_) {
    uint8_t command = this->send_queue_[0].data[10];
    if (expects_acknowledgement(command))
      ESP_LOGW(TAG, "No acknowledgement for command 0x%02X", command);
    else
      ESP_LOGD(TAG, "Finished sending...");
    this->next_packet_();
  }
}

void LivingColors1ClientComponent::next_packet_() {
  for (uint8_t i = 1; i < this->send_queue_length_; i++)
    this->send_queue_[i - 1] = this->send_queue_[i];
  this->send_queue_length_--;
  this->send_repeat_counter_ = 0;
}

void LivingColors1ClientComponent::check_acknowledgement(uint64_t address, Command command) {
  if (this->send_queue_length_ == 0 || this->send_repeat_counter_ == 0)
    return;
  const uint8_t *data = this->send_queue_[0].data;
  uint8_t sent = data[10];
  // Usually the command + 1 (ACK_...); a lamp that's already off answers OFF with OFF
  // A far away lamp ignores unpairing, so it's never warned about; only stop repeating when answered
  bool unpairing = sent == (uint8_t) Command::UNPAIRING_REQUEST;
  if ((!expects_acknowledgement(sent) && !unpairing) || ((uint8_t) command != sent + 1 && (uint8_t) command != sent))
    return;
  // The acknowledgement has the lamp and remote halves of the address swapped
  uint64_t sent_address = 0;
  for (int i = 1; i <= 8; i++)
    sent_address = (sent_address << 8) | data[i];
  if (address != swap_address(sent_address))
    return;
  ESP_LOGD(TAG, "Acknowledged after %u sends", this->send_repeat_counter_);
  this->next_packet_();
}

}  // namespace livingcolors1
}  // namespace esphome
