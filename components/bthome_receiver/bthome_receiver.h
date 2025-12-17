#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/automation.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif

#include <vector>
#include <map>
#include <array>

namespace esphome {
namespace bthome_receiver {

// BTHome v2 constants
static const uint16_t BTHOME_SERVICE_UUID = 0xFCD2;

// Device info byte format: bit 0 = encryption, bit 2 = trigger-based
static const uint8_t BTHOME_DEVICE_INFO_ENCRYPTED_MASK = 0x01;

// Special object IDs for events and variable-length data
static const uint8_t OBJECT_ID_BUTTON = 0x3A;
static const uint8_t OBJECT_ID_DIMMER = 0x3C;
static const uint8_t OBJECT_ID_TEXT = 0x53;
static const uint8_t OBJECT_ID_RAW = 0x54;

// Button event types (BTHome v2 spec object ID 0x3A)
static const uint8_t BUTTON_EVENT_NONE = 0x00;
static const uint8_t BUTTON_EVENT_PRESS = 0x01;
static const uint8_t BUTTON_EVENT_DOUBLE_PRESS = 0x02;
static const uint8_t BUTTON_EVENT_TRIPLE_PRESS = 0x03;
static const uint8_t BUTTON_EVENT_LONG_PRESS = 0x04;
static const uint8_t BUTTON_EVENT_LONG_DOUBLE_PRESS = 0x05;
static const uint8_t BUTTON_EVENT_LONG_TRIPLE_PRESS = 0x06;
static const uint8_t BUTTON_EVENT_HOLD_PRESS = 0x80;

// Encryption constants
static const size_t AES_KEY_SIZE = 16;

// Object type info for parsing BTHome data
struct ObjectTypeInfo {
  uint8_t data_bytes;
  bool is_signed;
  float factor;
  bool is_sensor;
  bool is_binary_sensor;
};

// Forward declarations
class BTHomeReceiverHub;
class BTHomeDevice;

// =============================================================================
// BTHomeSensor - Represents a numeric sensor value from a BTHome device
// =============================================================================
#ifdef USE_SENSOR
class BTHomeSensor {
 public:
  BTHomeSensor(uint8_t object_id, sensor::Sensor *sensor) : object_id_(object_id), sensor_(sensor) {}

  uint8_t get_object_id() const { return this->object_id_; }
  sensor::Sensor *get_sensor() { return this->sensor_; }

 protected:
  uint8_t object_id_;
  sensor::Sensor *sensor_;
};
#endif

// =============================================================================
// BTHomeBinarySensor - Represents a boolean sensor value from a BTHome device
// =============================================================================
#ifdef USE_BINARY_SENSOR
class BTHomeBinarySensor {
 public:
  BTHomeBinarySensor(uint8_t object_id, binary_sensor::BinarySensor *sensor)
      : object_id_(object_id), sensor_(sensor) {}

  uint8_t get_object_id() const { return this->object_id_; }
  binary_sensor::BinarySensor *get_sensor() { return this->sensor_; }

 protected:
  uint8_t object_id_;
  binary_sensor::BinarySensor *sensor_;
};
#endif

// =============================================================================
// BTHomeTextSensor - Represents text/raw data from a BTHome device
// =============================================================================
#ifdef USE_TEXT_SENSOR
class BTHomeTextSensor {
 public:
  BTHomeTextSensor(uint8_t object_id, text_sensor::TextSensor *sensor)
      : object_id_(object_id), sensor_(sensor) {}

  uint8_t get_object_id() const { return this->object_id_; }
  text_sensor::TextSensor *get_sensor() { return this->sensor_; }

 protected:
  uint8_t object_id_;
  text_sensor::TextSensor *sensor_;
};
#endif

// =============================================================================
// BTHomeButtonTrigger - Automation trigger for button events
// =============================================================================
class BTHomeButtonTrigger : public Trigger<>, public Parented<BTHomeDevice> {
 public:
  explicit BTHomeButtonTrigger(BTHomeDevice *parent) : Parented(parent) {}

  void set_button_index(uint8_t index) { this->button_index_ = index; }
  void set_event_type(uint8_t event_type) { this->event_type_ = event_type; }

  uint8_t get_button_index() const { return this->button_index_; }
  uint8_t get_event_type() const { return this->event_type_; }

 protected:
  uint8_t button_index_{0};
  uint8_t event_type_{BUTTON_EVENT_PRESS};
};

// =============================================================================
// BTHomeDimmerTrigger - Automation trigger for dimmer rotation events
// =============================================================================
class BTHomeDimmerTrigger : public Trigger<int8_t>, public Parented<BTHomeDevice> {
 public:
  explicit BTHomeDimmerTrigger(BTHomeDevice *parent) : Parented(parent) {}
};

// =============================================================================
// BTHomeDevice - Represents a single BTHome BLE device being monitored
// =============================================================================
class BTHomeDevice : public Parented<BTHomeReceiverHub> {
 public:
  explicit BTHomeDevice(BTHomeReceiverHub *parent) : Parented(parent) {}

  void set_mac_address(uint64_t mac) { this->address_ = mac; }
  void set_name(const std::string &name) { this->name_ = name; }
  void set_encryption_key(const std::array<uint8_t, AES_KEY_SIZE> &key);

  uint64_t get_mac_address() const { return this->address_; }
  const std::string &get_name() const { return this->name_; }

  // Parse incoming BLE advertisement
  bool parse_advertisement(const std::vector<uint8_t> &service_data);

#ifdef USE_SENSOR
  void add_sensor(uint8_t object_id, sensor::Sensor *sensor) {
    this->sensors_.push_back(new BTHomeSensor(object_id, sensor));
  }
#endif

#ifdef USE_BINARY_SENSOR
  void add_binary_sensor(uint8_t object_id, binary_sensor::BinarySensor *sensor) {
    this->binary_sensors_.push_back(new BTHomeBinarySensor(object_id, sensor));
  }
#endif

#ifdef USE_TEXT_SENSOR
  void add_text_sensor(uint8_t object_id, text_sensor::TextSensor *sensor) {
    this->text_sensors_.push_back(new BTHomeTextSensor(object_id, sensor));
  }
#endif

  void add_button_trigger(BTHomeButtonTrigger *trigger) { this->button_triggers_.push_back(trigger); }
  void add_dimmer_trigger(BTHomeDimmerTrigger *trigger) { this->dimmer_triggers_.push_back(trigger); }

 protected:
  // Decrypt encrypted payload using AES-128-CCM
  bool decrypt_payload_(const uint8_t *ciphertext, size_t ciphertext_len, const uint8_t *mac,
                        uint8_t device_info, uint32_t counter, uint8_t *plaintext, size_t *plaintext_len);

  // Parse measurement objects from payload
  void parse_measurements_(const uint8_t *data, size_t len);

  // Publish values to registered sensors
  void publish_sensor_value_(uint8_t object_id, float value);
  void publish_binary_sensor_value_(uint8_t object_id, bool value);
  void publish_text_value_(uint8_t object_id, const std::string &value);

  // Handle events
  void handle_button_event_(uint8_t button_index, uint8_t event_type);
  void handle_dimmer_event_(int8_t steps);

  uint64_t address_{0};
  std::string name_;

  // Encryption
  bool encryption_enabled_{false};
  std::array<uint8_t, AES_KEY_SIZE> encryption_key_{};
  uint32_t last_counter_{0};

  // Sensors
#ifdef USE_SENSOR
  std::vector<BTHomeSensor *> sensors_;
#endif
#ifdef USE_BINARY_SENSOR
  std::vector<BTHomeBinarySensor *> binary_sensors_;
#endif
#ifdef USE_TEXT_SENSOR
  std::vector<BTHomeTextSensor *> text_sensors_;
#endif

  // Event triggers
  std::vector<BTHomeButtonTrigger *> button_triggers_;
  std::vector<BTHomeDimmerTrigger *> dimmer_triggers_;
};

// =============================================================================
// BTHomeReceiverHub - Main component that receives BLE advertisements
// =============================================================================
class BTHomeReceiverHub : public Component, public esp32_ble_tracker::ESPBTDeviceListener {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // Register a device to monitor
  void register_device(BTHomeDevice *device);

  // ESPBTDeviceListener interface - called when BLE advertisement is received
  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;

 protected:
  // Device registry (MAC address -> BTHomeDevice)
  std::map<uint64_t, BTHomeDevice *> devices_;
};

}  // namespace bthome_receiver
}  // namespace esphome
