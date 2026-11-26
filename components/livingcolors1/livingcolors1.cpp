#include "livingcolors1.h"

#include "esphome/core/log.h"
#include "esphome/core/macros.h"
#include "esphome/core/version.h"
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
  if (!success && command == Command::PAIRING_RESPONSE) {
    // If the address is a pairing response swap the light and remote address parts and show as detected address
    this->log_detected_address_(((0x00000000FFFFFFFF & address) << 32) | ((0xFFFFFFFF00000000 & address) >> 32));
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

void LivingColors1Component::send(uint8_t *data, uint8_t length) {
  uint8_t data_copy[length];
  data[11] = this->serial_number_++;
  memcpy(data_copy, data, length);
  ((CC2500Device *) this)->send(data_copy, length);
}

LivingColors1ClientComponent::LivingColors1ClientComponent() {
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 11, 0)
  // ESPHome 2026.11 removed get_mac_address(); get_mac_address_into_buffer()
  // returns the same format (12 lowercase hex characters, no separators).
  char mac_buf[MAC_ADDRESS_BUFFER_SIZE];
  get_mac_address_into_buffer(mac_buf);
  std::string mac_address(mac_buf);
#else
  std::string mac_address = get_mac_address();
#endif
  std::string address = "FFFFFFFF" + mac_address.substr(mac_address.length() - 8, 8);
  this->address_ = std::strtoull(address.c_str(), nullptr, 16);
}

void LivingColors1ClientComponent::loop() {
  if (this->send_queue_length_ == 0)
    return;
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
  if (!expects_acknowledgement(sent) || (uint8_t) command != sent + 1)
    return;
  // The acknowledgement has the lamp and remote halves of the address swapped
  uint64_t sent_address = 0;
  for (int i = 1; i <= 8; i++)
    sent_address = (sent_address << 8) | data[i];
  if (address != ((sent_address << 32) | (sent_address >> 32)))
    return;
  ESP_LOGD(TAG, "Acknowledged after %u sends", this->send_repeat_counter_);
  this->next_packet_();
}

}  // namespace livingcolors1
}  // namespace esphome
