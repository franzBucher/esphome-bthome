#include "bthome_receiver.h"
#include "esphome/core/log.h"
#include "mbedtls/ccm.h"

#include <cstring>
#include <cmath>

namespace esphome {
namespace bthome_receiver {

static const char *const TAG = "bthome_receiver";

// BTHome v2 object type lookup table
// Format: object_id -> (data_bytes, is_signed, factor, is_sensor, is_binary_sensor)
static const std::map<uint8_t, ObjectTypeInfo> OBJECT_TYPE_MAP = {
    // Basic sensors
    {0x00, {1, false, 1, true, false}},           // packet_id
    {0x01, {1, false, 1, true, false}},           // battery
    {0x02, {2, true, 0.01, true, false}},         // temperature
    {0x03, {2, false, 0.01, true, false}},        // humidity
    {0x04, {3, false, 0.01, true, false}},        // pressure
    {0x05, {3, false, 0.01, true, false}},        // illuminance
    {0x06, {2, false, 0.01, true, false}},        // mass_kg
    {0x07, {2, false, 0.01, true, false}},        // mass_lb
    {0x08, {2, true, 0.01, true, false}},         // dewpoint
    {0x09, {1, false, 1, true, false}},           // count_uint8
    {0x0A, {3, false, 0.001, true, false}},       // energy
    {0x0B, {3, false, 0.01, true, false}},        // power
    {0x0C, {2, false, 0.001, true, false}},       // voltage
    {0x0D, {2, false, 1, true, false}},           // pm2_5
    {0x0E, {2, false, 1, true, false}},           // pm10
    {0x12, {2, false, 1, true, false}},           // co2
    {0x13, {2, false, 1, true, false}},           // tvoc
    {0x14, {2, false, 0.01, true, false}},        // moisture
    {0x2E, {1, false, 1, true, false}},           // humidity_uint8
    {0x2F, {1, false, 1, true, false}},           // moisture_uint8

    // Binary sensors
    {0x0F, {1, false, 1, false, true}},           // generic_boolean
    {0x10, {1, false, 1, false, true}},           // power
    {0x11, {1, false, 1, false, true}},           // opening
    {0x15, {1, false, 1, false, true}},           // battery_low
    {0x16, {1, false, 1, false, true}},           // battery_charging
    {0x17, {1, false, 1, false, true}},           // carbon_monoxide
    {0x18, {1, false, 1, false, true}},           // cold
    {0x19, {1, false, 1, false, true}},           // connectivity
    {0x1A, {1, false, 1, false, true}},           // door
    {0x1B, {1, false, 1, false, true}},           // garage_door
    {0x1C, {1, false, 1, false, true}},           // gas
    {0x1D, {1, false, 1, false, true}},           // heat
    {0x1E, {1, false, 1, false, true}},           // light
    {0x1F, {1, false, 1, false, true}},           // lock
    {0x20, {1, false, 1, false, true}},           // moisture_binary
    {0x21, {1, false, 1, false, true}},           // motion
    {0x22, {1, false, 1, false, true}},           // moving
    {0x23, {1, false, 1, false, true}},           // occupancy
    {0x24, {1, false, 1, false, true}},           // plug
    {0x25, {1, false, 1, false, true}},           // presence
    {0x26, {1, false, 1, false, true}},           // problem
    {0x27, {1, false, 1, false, true}},           // running
    {0x28, {1, false, 1, false, true}},           // safety
    {0x29, {1, false, 1, false, true}},           // smoke
    {0x2A, {1, false, 1, false, true}},           // sound
    {0x2B, {1, false, 1, false, true}},           // tamper
    {0x2C, {1, false, 1, false, true}},           // vibration
    {0x2D, {1, false, 1, false, true}},           // window

    // Extended sensors
    {0x3D, {2, false, 1, true, false}},           // count_uint16
    {0x3E, {4, false, 1, true, false}},           // count_uint32
    {0x3F, {2, true, 0.1, true, false}},          // rotation
    {0x40, {2, false, 1, true, false}},           // distance_mm
    {0x41, {2, false, 0.1, true, false}},         // distance_m
    {0x42, {3, false, 0.001, true, false}},       // duration
    {0x43, {2, false, 0.001, true, false}},       // current
    {0x44, {2, false, 0.01, true, false}},        // speed
    {0x45, {2, true, 0.1, true, false}},          // temperature_01
    {0x46, {1, false, 0.1, true, false}},         // uv_index
    {0x47, {2, false, 0.1, true, false}},         // volume_l_01
    {0x48, {2, false, 1, true, false}},           // volume_ml
    {0x49, {2, false, 0.001, true, false}},       // volume_flow_rate
    {0x4A, {2, false, 0.1, true, false}},         // voltage_01
    {0x4B, {3, false, 0.001, true, false}},       // gas
    {0x4C, {4, false, 0.001, true, false}},       // gas_uint32
    {0x4D, {4, false, 0.001, true, false}},       // energy_uint32
    {0x4E, {4, false, 0.001, true, false}},       // volume_l
    {0x4F, {4, false, 0.001, true, false}},       // water
    {0x50, {4, false, 1, true, false}},           // timestamp
    {0x51, {2, false, 0.001, true, false}},       // acceleration
    {0x52, {2, false, 0.001, true, false}},       // gyroscope
    {0x55, {4, false, 0.001, true, false}},       // volume_storage
    {0x56, {2, false, 1, true, false}},           // conductivity
    {0x57, {1, true, 1, true, false}},            // temperature_sint8
    {0x58, {1, true, 0.35, true, false}},         // temperature_sint8_035
    {0x59, {1, true, 1, true, false}},            // count_sint8
    {0x5A, {2, true, 1, true, false}},            // count_sint16
    {0x5B, {4, true, 1, true, false}},            // count_sint32
    {0x5C, {4, true, 0.01, true, false}},         // power_sint32
    {0x5D, {2, true, 0.001, true, false}},        // current_sint16
    {0x5E, {2, false, 0.01, true, false}},        // direction
    {0x5F, {2, false, 0.1, true, false}},         // precipitation
    {0x60, {1, false, 1, true, false}},           // channel
    {0x61, {2, false, 1, true, false}},           // rotational_speed
};

// ============================================================================
// BTHomeReceiverHub Implementation
// ============================================================================

void BTHomeReceiverHub::setup() {
  ESP_LOGCONFIG(TAG, "Setting up BTHome Receiver...");
}

void BTHomeReceiverHub::dump_config() {
  ESP_LOGCONFIG(TAG, "BTHome Receiver:");
  ESP_LOGCONFIG(TAG, "  Registered Devices: %d", this->devices_.size());
  for (const auto &pair : this->devices_) {
    auto *device = pair.second;
    ESP_LOGCONFIG(TAG, "    MAC: %012llX", device->get_mac_address());
  }
}

void BTHomeReceiverHub::register_device(BTHomeDevice *device) {
  this->devices_[device->get_mac_address()] = device;
  ESP_LOGD(TAG, "Registered device: %012llX", device->get_mac_address());
}

bool BTHomeReceiverHub::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  // Check if this device has BTHome service data (UUID 0xFCD2)
  for (const auto &service_data : device.get_service_datas()) {
    if (service_data.uuid.get_uuid().uuid.uuid16 == BTHOME_SERVICE_UUID) {
      // Look up registered device by MAC address
      uint64_t address = device.address_uint64();
      auto it = this->devices_.find(address);
      if (it != this->devices_.end()) {
        ESP_LOGV(TAG, "Processing BTHome advertisement from %012llX", address);
        return it->second->parse_advertisement(service_data.data);
      } else {
        ESP_LOGV(TAG, "Ignoring BTHome advertisement from unregistered device %012llX", address);
      }
      return false;
    }
  }
  return false;
}

// ============================================================================
// BTHomeDevice Implementation
// ============================================================================

void BTHomeDevice::set_encryption_key(const std::array<uint8_t, 16> &key) {
  this->encryption_enabled_ = true;
  this->encryption_key_ = key;
}

bool BTHomeDevice::parse_advertisement(const std::vector<uint8_t> &service_data) {
  if (service_data.size() < 1) {
    ESP_LOGW(TAG, "Invalid service data: too short");
    return false;
  }

  // First byte is device_info
  uint8_t device_info = service_data[0];
  bool is_encrypted = (device_info & BTHOME_DEVICE_INFO_ENCRYPTED_MASK) != 0;

  ESP_LOGV(TAG, "Device info: 0x%02X, encrypted: %s", device_info, is_encrypted ? "yes" : "no");

  const uint8_t *payload_data;
  size_t payload_len;
  uint8_t decrypted_buffer[256];

  if (is_encrypted) {
    if (!this->encryption_enabled_) {
      ESP_LOGW(TAG, "Received encrypted data but no encryption key configured");
      return false;
    }

    // Encrypted format: device_info(1) + ciphertext + counter(4) + MIC(4)
    // The counter and MIC are at the end: [...ciphertext...][counter(4)][MIC(4)]
    if (service_data.size() < 9) {  // device_info(1) + min_ciphertext(0) + counter(4) + MIC(4)
      ESP_LOGW(TAG, "Encrypted data too short");
      return false;
    }

    // Extract counter from bytes [-8:-4] (4 bytes before the MIC)
    size_t counter_offset = service_data.size() - 8;
    uint32_t counter = service_data[counter_offset] | (service_data[counter_offset + 1] << 8) |
                       (service_data[counter_offset + 2] << 16) | (service_data[counter_offset + 3] << 24);

    ESP_LOGV(TAG, "Counter: %u, last counter: %u", counter, this->last_counter_);

    // Validate counter (replay protection)
    if (counter <= this->last_counter_) {
      ESP_LOGW(TAG, "Counter not increased (replay attack?): %u <= %u", counter, this->last_counter_);
      return false;
    }

    // Ciphertext is between device_info and counter
    const uint8_t *ciphertext = service_data.data() + 1;
    size_t ciphertext_len = service_data.size() - 1 - 4;  // Exclude device_info and counter+MIC

    // Get MAC address (6 bytes)
    uint8_t mac[6];
    for (int i = 0; i < 6; i++) {
      mac[i] = (this->address_ >> (i * 8)) & 0xFF;
    }

    size_t plaintext_len;
    if (!this->decrypt_payload_(ciphertext, ciphertext_len, mac, device_info, counter, decrypted_buffer,
                                 &plaintext_len)) {
      ESP_LOGW(TAG, "Decryption failed");
      return false;
    }

    // Update last counter after successful decryption
    this->last_counter_ = counter;

    payload_data = decrypted_buffer;
    payload_len = plaintext_len;
    ESP_LOGV(TAG, "Decrypted %d bytes", plaintext_len);
  } else {
    // Unencrypted: just skip device_info byte
    payload_data = service_data.data() + 1;
    payload_len = service_data.size() - 1;
  }

  // Parse measurements
  this->parse_measurements_(payload_data, payload_len);
  return true;
}

bool BTHomeDevice::decrypt_payload_(const uint8_t *ciphertext, size_t ciphertext_len, const uint8_t *mac,
                                     uint8_t device_info, uint32_t counter, uint8_t *plaintext,
                                     size_t *plaintext_len) {
  // BTHome v2 AES-CCM decryption
  // Nonce: MAC(6) + UUID(2, little-endian) + device_info(1) + counter(4) = 13 bytes
  uint8_t nonce[13];
  memcpy(nonce, mac, 6);
  nonce[6] = BTHOME_SERVICE_UUID & 0xFF;         // 0xD2
  nonce[7] = (BTHOME_SERVICE_UUID >> 8) & 0xFF;  // 0xFC
  nonce[8] = device_info;
  nonce[9] = counter & 0xFF;
  nonce[10] = (counter >> 8) & 0xFF;
  nonce[11] = (counter >> 16) & 0xFF;
  nonce[12] = (counter >> 24) & 0xFF;

  // The ciphertext_len includes the MIC (4 bytes)
  if (ciphertext_len < 4) {
    ESP_LOGE(TAG, "Ciphertext too short for MIC");
    return false;
  }

  size_t actual_ciphertext_len = ciphertext_len - 4;
  const uint8_t *mic = ciphertext + actual_ciphertext_len;

  // Prepare combined buffer for mbedtls (ciphertext + tag)
  uint8_t combined[256];
  if (ciphertext_len > sizeof(combined)) {
    ESP_LOGE(TAG, "Ciphertext too long");
    return false;
  }
  memcpy(combined, ciphertext, ciphertext_len);

  mbedtls_ccm_context ctx;
  mbedtls_ccm_init(&ctx);

  int ret = mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, this->encryption_key_.data(), 128);
  if (ret != 0) {
    ESP_LOGE(TAG, "mbedtls_ccm_setkey failed: %d", ret);
    mbedtls_ccm_free(&ctx);
    return false;
  }

  ret = mbedtls_ccm_auth_decrypt(&ctx, actual_ciphertext_len, nonce, sizeof(nonce), nullptr, 0, ciphertext, plaintext,
                                  mic, 4);
  mbedtls_ccm_free(&ctx);

  if (ret != 0) {
    ESP_LOGE(TAG, "mbedtls_ccm_auth_decrypt failed: %d", ret);
    return false;
  }

  *plaintext_len = actual_ciphertext_len;
  return true;
}

void BTHomeDevice::parse_measurements_(const uint8_t *data, size_t len) {
  size_t pos = 0;

  while (pos < len) {
    if (pos + 1 > len) {
      ESP_LOGW(TAG, "Incomplete measurement at offset %d", pos);
      break;
    }

    uint8_t object_id = data[pos++];
    ESP_LOGV(TAG, "Object ID: 0x%02X at offset %d", object_id, pos - 1);

    // Handle special types: button, dimmer, text, raw
    if (object_id == OBJECT_ID_BUTTON) {
      // Button event: object_id(1) + event_type(1)
      if (pos + 1 > len) {
        ESP_LOGW(TAG, "Incomplete button event");
        break;
      }
      uint8_t event_data = data[pos++];
      uint8_t button_index = (event_data >> 4) & 0x0F;  // Upper 4 bits
      uint8_t event_type = event_data & 0x0F;           // Lower 4 bits
      ESP_LOGD(TAG, "Button event: index=%d, type=0x%02X", button_index, event_type);
      this->handle_button_event_(button_index, event_type);
      continue;
    }

    if (object_id == OBJECT_ID_DIMMER) {
      // Dimmer event: object_id(1) + steps(1, signed)
      if (pos + 1 > len) {
        ESP_LOGW(TAG, "Incomplete dimmer event");
        break;
      }
      int8_t steps = static_cast<int8_t>(data[pos++]);
      ESP_LOGD(TAG, "Dimmer event: steps=%d", steps);
      this->handle_dimmer_event_(steps);
      continue;
    }

    if (object_id == OBJECT_ID_TEXT) {
      // Text: object_id(1) + length(1) + UTF-8 string
      if (pos + 1 > len) {
        ESP_LOGW(TAG, "Incomplete text length");
        break;
      }
      uint8_t text_len = data[pos++];
      if (pos + text_len > len) {
        ESP_LOGW(TAG, "Incomplete text data");
        break;
      }
      std::string text(reinterpret_cast<const char *>(data + pos), text_len);
      pos += text_len;
      ESP_LOGD(TAG, "Text: '%s'", text.c_str());
      this->publish_text_value_(object_id, text);
      continue;
    }

    if (object_id == OBJECT_ID_RAW) {
      // Raw: object_id(1) + length(1) + raw bytes (display as hex)
      if (pos + 1 > len) {
        ESP_LOGW(TAG, "Incomplete raw length");
        break;
      }
      uint8_t raw_len = data[pos++];
      if (pos + raw_len > len) {
        ESP_LOGW(TAG, "Incomplete raw data");
        break;
      }
      // Convert to hex string
      std::string hex_str;
      for (uint8_t i = 0; i < raw_len; i++) {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02X", data[pos + i]);
        if (i > 0)
          hex_str += " ";
        hex_str += hex;
      }
      pos += raw_len;
      ESP_LOGD(TAG, "Raw: %s", hex_str.c_str());
      this->publish_text_value_(object_id, hex_str);
      continue;
    }

    // Look up standard object type
    auto it = OBJECT_TYPE_MAP.find(object_id);
    if (it == OBJECT_TYPE_MAP.end()) {
      ESP_LOGW(TAG, "Unknown object ID: 0x%02X", object_id);
      // Skip this measurement - we don't know its size, so we have to stop parsing
      break;
    }

    const ObjectTypeInfo &type_info = it->second;

    // Check if we have enough data
    if (pos + type_info.data_bytes > len) {
      ESP_LOGW(TAG, "Incomplete data for object 0x%02X (need %d bytes, have %d)", object_id, type_info.data_bytes,
               len - pos);
      break;
    }

    // Decode value based on type
    if (type_info.is_binary_sensor) {
      // Binary sensor: single byte, 0x00 or 0x01
      bool value = data[pos] != 0;
      pos += type_info.data_bytes;
      ESP_LOGD(TAG, "Binary sensor 0x%02X: %s", object_id, value ? "ON" : "OFF");
      this->publish_binary_sensor_value_(object_id, value);
    } else if (type_info.is_sensor) {
      // Numeric sensor: decode based on data_bytes and signedness
      int32_t raw_value = 0;

      if (type_info.is_signed) {
        // Signed integer (little-endian)
        switch (type_info.data_bytes) {
          case 1:
            raw_value = static_cast<int8_t>(data[pos]);
            break;
          case 2:
            raw_value = static_cast<int16_t>(data[pos] | (data[pos + 1] << 8));
            break;
          case 3:
            // sint24: sign-extend from 24 bits to 32 bits
            raw_value = data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16);
            if (raw_value & 0x800000)
              raw_value |= 0xFF000000;  // Sign extend
            break;
          case 4:
            raw_value = data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24);
            break;
        }
      } else {
        // Unsigned integer (little-endian)
        uint32_t unsigned_value = 0;
        switch (type_info.data_bytes) {
          case 1:
            unsigned_value = data[pos];
            break;
          case 2:
            unsigned_value = data[pos] | (data[pos + 1] << 8);
            break;
          case 3:
            unsigned_value = data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16);
            break;
          case 4:
            unsigned_value = data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24);
            break;
        }
        raw_value = unsigned_value;
      }

      pos += type_info.data_bytes;

      // Apply factor to convert to actual value
      float value = raw_value * type_info.factor;
      ESP_LOGD(TAG, "Sensor 0x%02X: raw=%d, value=%.3f", object_id, raw_value, value);
      this->publish_sensor_value_(object_id, value);
    }
  }
}

void BTHomeDevice::publish_sensor_value_(uint8_t object_id, float value) {
#ifdef USE_SENSOR
  for (auto *sensor_obj : this->sensors_) {
    if (sensor_obj->get_object_id() == object_id) {
      sensor_obj->get_sensor()->publish_state(value);
      return;
    }
  }
#endif
  ESP_LOGV(TAG, "No sensor registered for object ID 0x%02X", object_id);
}

void BTHomeDevice::publish_binary_sensor_value_(uint8_t object_id, bool value) {
#ifdef USE_BINARY_SENSOR
  for (auto *sensor_obj : this->binary_sensors_) {
    if (sensor_obj->get_object_id() == object_id) {
      sensor_obj->get_sensor()->publish_state(value);
      return;
    }
  }
#endif
  ESP_LOGV(TAG, "No binary sensor registered for object ID 0x%02X", object_id);
}

void BTHomeDevice::publish_text_value_(uint8_t object_id, const std::string &value) {
#ifdef USE_TEXT_SENSOR
  for (auto *sensor_obj : this->text_sensors_) {
    if (sensor_obj->get_object_id() == object_id) {
      sensor_obj->get_sensor()->publish_state(value);
      return;
    }
  }
#endif
  ESP_LOGV(TAG, "No text sensor registered for object ID 0x%02X", object_id);
}

void BTHomeDevice::handle_button_event_(uint8_t button_index, uint8_t event_type) {
  for (auto *trigger : this->button_triggers_) {
    if (trigger->get_button_index() == button_index && trigger->get_event_type() == event_type) {
      trigger->trigger();
    }
  }
}

void BTHomeDevice::handle_dimmer_event_(int8_t steps) {
  for (auto *trigger : this->dimmer_triggers_) {
    trigger->trigger(steps);
  }
}

}  // namespace bthome_receiver
}  // namespace esphome
