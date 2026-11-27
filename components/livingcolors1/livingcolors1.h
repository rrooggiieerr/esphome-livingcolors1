#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cc2500/cc2500.h"

namespace esphome {
namespace livingcolors1 {

enum class Command : uint8_t {
  PAIRING_REQUEST = 0x01,
  PAIRING_RESPONSE = 0x02,
  HSV = 0x03,
  ACK_HSV = 0x04,
  ON = 0x05,
  ACK_ON = 0x06,
  OFF = 0x07,
  ACK_OFF = 0x08,
  UNPAIRING_REQUEST = 0x0A,
  ACK_UNPAIRING = 0x0B,
  CYCLE = 0x0C,
  ACK_CYCLE = 0x0D,
  CYCLE_SYNC = 0x12,
};

class LivingColors1ClientComponent;

/// Swap the lamp and remote halves of an address, as in a lamp's answer
inline uint64_t swap_address(uint64_t address) { return (address << 32) | (address >> 32); }

static const size_t MAX_DETECTED_ADDRESSES = 16;
// Packets a client can have queued for sending, e.g. ON followed by CYCLE
static const uint8_t SEND_QUEUE_SIZE = 2;
// Time between repeats of a command
static const uint32_t SEND_INTERVAL_MS = 14;
// How long after pressing Pair a pairing response is stored
static const uint32_t PAIRING_TIMEOUT_MS = 10000;
// How long after pressing Learn a command from a remote is stored
static const uint32_t LEARNING_TIMEOUT_MS = 30000;
// Remote commands awaiting a lamp's answer while learning, a remote has up to 8 lamps
static const uint8_t MAX_LEARN_CANDIDATES = 8;

class LivingColors1Component : public Component, public cc2500::CC2500Device<0, 3, 14> {
 public:
  void setup() override;

  //	void dump_config() override;
  bool receive(uint8_t *data, uint8_t length, float rssi, uint8_t lqi) override;

  void add_device(LivingColors1ClientComponent *device) { this->devices_.push_back(device); }
  void send(uint8_t *data, uint8_t length);
  /// Store the next pairing response for this remote ID in a light without address
  void start_pairing(uint32_t remote_id);
  /// Store lamps answering a remote's commands in lights without address
  void start_learning();
  /// Send an unpairing request to every light paired with this remote ID, the ESP's own
  void unpair(uint32_t remote_id);
  /// Reboot shortly, so lights that got or lost an address are shown or hidden
  void schedule_reboot();

 protected:
  std::vector<LivingColors1ClientComponent *> devices_;
  std::vector<uint64_t> detected_addresses_;

  uint8_t serial_number_ = 0;
  bool pairing_ = false;
  uint32_t pairing_remote_id_ = 0;
  bool learning_ = false;
  struct LearnCandidate {
    uint64_t address;
    uint8_t command;
  };
  LearnCandidate learn_candidates_[MAX_LEARN_CANDIDATES];
  uint8_t learn_candidates_length_ = 0;

  void log_detected_address_(uint64_t address);
  bool can_store_();
  void pair_(uint64_t address);
  void learn_(uint64_t address, Command command);
  void store_(uint64_t address);
};

class LivingColors1ClientComponent : public Component {
 public:
  LivingColors1ClientComponent();
  void loop() override;
  void set_parent(LivingColors1Component *parent);
  void set_address(uint64_t address) {
    this->address_ = address;
    this->has_address_ = true;
  }
  bool has_address() const { return this->has_address_; }
  /// Whether this device can store a paired or learned address
  virtual bool can_store_address() const { return false; }
  /// Store a paired address; false when this device doesn't take one
  virtual bool store_address(uint64_t address) { return false; }
  /// Replace a stored address of the same lamp, which is now paired with another remote
  virtual bool replace_address(uint64_t address) { return false; }
  /// Send an unpairing request to this device's lamp, if it's paired with this remote ID
  virtual void unpair(uint32_t remote_id) {}
  void set_send_repeats(uint16_t n_times) { this->send_repeats_ = n_times; }
  /// Send a command; it replaces pending commands unless append is set
  void send(uint8_t *data, uint8_t length, bool append = false) { this->send_(this->address_, data, length, append); }
  /// Stop repeating the current command when this is its acknowledgement
  void check_acknowledgement(uint64_t address, Command command);
  virtual bool receive(uint64_t address, uint8_t *data, uint8_t length, float rssi, uint8_t lqi) {
    return this->receive(address, data, length);
  }
  virtual bool receive(uint64_t address, uint8_t *data, uint8_t length) { return false; }

 protected:
  LivingColors1Component *parent_{nullptr};
  uint64_t address_;
  bool has_address_ = false;
  uint16_t send_repeats_ = 7;
  uint16_t send_repeat_counter_ = 0;
  uint32_t last_send_ = 0;
  // Packets being repeated, the first one is sent; kept as members as the repeats are sent from loop()
  struct SendPacket {
    uint8_t data[32];
    uint8_t length;
  };
  SendPacket send_queue_[SEND_QUEUE_SIZE];
  uint8_t send_queue_length_ = 0;

  void send_(uint64_t address, uint8_t *data, uint8_t length, bool append = false);
  void send_();
  void next_packet_();
  uint64_t swapped_address_() { return swap_address(this->address_); }
  uint64_t special_address_() { return 0xFFFFFFFF00000000 | (0x00000000FFFFFFFF & this->address_); }
  bool is_address_(uint64_t address) { return address == this->address_; }
  // A response has the light address and remote address swapped.
  bool is_response_(uint64_t address) { return address == this->swapped_address_(); }
  // If the first 4 bytes of the address are all 0xFF it seems to be a special address for
  // pairing and color cycling (and more?).
  bool is_special_(uint64_t address) { return address == this->special_address_(); }
};

}  // namespace livingcolors1
}  // namespace esphome
