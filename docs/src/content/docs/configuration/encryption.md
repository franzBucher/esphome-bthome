---
title: Encryption
description: Secure your BTHome sensor data with AES-CCM encryption
---

BTHome supports AES-CCM encryption to protect your sensor data from eavesdropping and spoofing.

## Why Use Encryption?

Without encryption:
- Anyone within Bluetooth range can read your sensor data
- Malicious actors could potentially spoof your sensors

With encryption:
- Sensor data is encrypted with AES-128-CCM
- Only devices with the encryption key can read the data
- Message authentication prevents spoofing

## Generating an Encryption Key

Generate a random 16-byte (32 hex character) key:

```bash
# Using OpenSSL
openssl rand -hex 16

# Example output: a1b2c3d4e5f6071829304a5b6c7d8e9f
```

Or using Python:

```bash
python3 -c "import secrets; print(secrets.token_hex(16))"
```

:::caution[Keep Your Key Safe]
Store your encryption key securely. You'll need it for both your ESPHome device and Home Assistant.
:::

## Configuring Encryption

Add the `encryption_key` to your BTHome configuration:

```yaml
bthome:
  encryption_key: "a1b2c3d4e5f6071829304a5b6c7d8e9f"
  sensors:
    - type: temperature
      id: my_temperature
    - type: humidity
      id: my_humidity
```

The key must be:
- Exactly 32 hexadecimal characters (16 bytes)
- Lowercase or uppercase (will be normalized)
- Can include spaces or dashes for readability (will be stripped)

Valid formats:
```yaml
encryption_key: "a1b2c3d4e5f6071829304a5b6c7d8e9f"
encryption_key: "A1B2C3D4E5F6071829304A5B6C7D8E9F"
encryption_key: "a1b2c3d4-e5f60718-29304a5b-6c7d8e9f"
```

## Home Assistant Configuration

When adding an encrypted BTHome device in Home Assistant:

1. Go to **Settings** → **Devices & Services**
2. Find your BTHome device (it will show as encrypted)
3. Click **Configure**
4. Enter the same encryption key you used in ESPHome

![Home Assistant BTHome encryption setup](/images/ha-bthome-encryption.png)

## Complete Example

### ESP32 with Encryption

```yaml
esphome:
  name: secure-sensor
  friendly_name: Secure Sensor

esp32:
  board: esp32dev
  framework:
    type: esp-idf

logger:

external_components:
  - source:
      type: git
      url: https://github.com/dz0ny/esphome-bthome
      ref: main
    components: [bthome]

i2c:
  sda: GPIO21
  scl: GPIO22

sensor:
  - platform: bme280_i2c
    temperature:
      id: temp
      name: "Temperature"
    humidity:
      id: humidity
      name: "Humidity"
    update_interval: 30s

bthome:
  encryption_key: "a1b2c3d4e5f6071829304a5b6c7d8e9f"
  min_interval: 30s
  max_interval: 60s
  sensors:
    - type: temperature
      id: temp
    - type: humidity
      id: humidity
```

### nRF52 with Encryption

```yaml
esphome:
  name: secure-sensor
  friendly_name: Secure Sensor

nrf52:
  board: xiao_ble
  bootloader: adafruit

logger:

external_components:
  - source:
      type: git
      url: https://github.com/dz0ny/esphome-bthome
      ref: main
    components: [bthome]

binary_sensor:
  - platform: gpio
    pin:
      number: 3
      mode: INPUT_PULLUP
    id: door
    name: "Door"

bthome:
  encryption_key: "a1b2c3d4e5f6071829304a5b6c7d8e9f"
  binary_sensors:
    - type: door
      id: door
      advertise_immediately: true
```

## Technical Details

BTHome uses AES-128-CCM encryption with:
- **Key size**: 128 bits (16 bytes)
- **Nonce**: MAC address + packet counter
- **MIC**: 4-byte message integrity code
- **Counter**: Prevents replay attacks

The encrypted packet structure:

```mermaid
packet-beta
  0-7: "Device Info"
  8-39: "Counter (4B)"
  40-71: "Encrypted Data"
  72-103: "MIC (4B)"
```

## Troubleshooting

### "Decryption failed" in Home Assistant

- Verify the encryption key matches exactly
- Check that the device is within Bluetooth range
- Restart both the ESPHome device and Home Assistant

### Device not appearing as encrypted

- Ensure `encryption_key` is set in your configuration
- Recompile and reflash the device
- The device info byte should show encryption enabled (bit 0 set)
