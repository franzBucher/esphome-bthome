---
title: Temperature Probe (DS18B20)
description: Battery-powered temperature sensor using DS18B20 waterproof probe
---

A waterproof temperature sensor using DS18B20 probe, ideal for outdoor, liquid, or soil temperature monitoring.

## Hardware

- Seeed XIAO BLE (nRF52840)
- DS18B20 waterproof temperature probe
- 4.7kΩ pull-up resistor
- 3.7V LiPo battery

## Wiring

| DS18B20 Wire | XIAO Pin |
|--------------|----------|
| Red (VCC) | 3V3 |
| Black (GND) | GND |
| Yellow (Data) | D2 (pin 2) |

**Important**: Add a 4.7kΩ resistor between Data (Yellow) and VCC (Red).

```mermaid
graph LR
    subgraph XIAO["Seeed XIAO BLE"]
        V3["3V3"]
        D2["D2 (Pin 2)"]
        GND["GND"]
    end

    subgraph DS18B20["DS18B20 Probe"]
        RED["Red (VCC)"]
        YELLOW["Yellow (Data)"]
        BLACK["Black (GND)"]
    end

    V3 --- RED
    V3 --- R["4.7kΩ"]
    R --- YELLOW
    D2 --- YELLOW
    GND --- BLACK

    style R fill:#ff9,stroke:#333
    style DS18B20 fill:#e6f3ff,stroke:#333
```

## Configuration

```yaml
esphome:
  name: temp-probe-ds18b20
  friendly_name: Temperature Probe

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

# OneWire bus for DS18B20
one_wire:
  - platform: gpio
    pin: 2

# Battery monitoring
sensor:
  - platform: adc
    pin: 29
    id: battery_voltage
    name: "Battery Voltage"
    update_interval: 60s
    filters:
      - multiply: 2.0

  - platform: template
    id: battery_percent
    name: "Battery"
    unit_of_measurement: "%"
    lambda: |-
      float voltage = id(battery_voltage).state;
      float percent = (voltage - 3.0) / (4.2 - 3.0) * 100.0;
      if (percent > 100) percent = 100;
      if (percent < 0) percent = 0;
      return percent;
    update_interval: 60s

  # DS18B20 temperature sensor
  - platform: dallas_temp
    id: probe_temperature
    name: "Temperature"
    update_interval: 60s
    # Optional: specify address if multiple sensors
    # address: 0x1234567890ABCDEF

# BTHome broadcast
bthome:
  min_interval: 30s
  max_interval: 60s
  tx_power: 4
  sensors:
    - type: battery
      id: battery_percent
    - type: temperature
      id: probe_temperature

# Deep sleep for battery savings
deep_sleep:
  run_duration: 20s
  sleep_duration: 5min
```

## Multiple Probes

You can connect multiple DS18B20 sensors to the same data pin:

```yaml
one_wire:
  - platform: gpio
    pin: 2

sensor:
  # First probe
  - platform: dallas_temp
    address: 0x1234567890ABCDEF
    id: temp_probe_1
    name: "Temperature 1"
    update_interval: 60s

  # Second probe
  - platform: dallas_temp
    address: 0xFEDCBA0987654321
    id: temp_probe_2
    name: "Temperature 2"
    update_interval: 60s

bthome:
  sensors:
    - type: temperature
      id: temp_probe_1
    - type: temperature
      id: temp_probe_2
```

:::tip[Finding Addresses]
Set `update_interval: 10s` temporarily and check the logs. ESPHome will print the addresses of all connected sensors.
:::

## Sensor Specifications

| Parameter | Value |
|-----------|-------|
| Temperature Range | -55°C to +125°C |
| Accuracy | ±0.5°C (-10°C to +85°C) |
| Resolution | 12-bit (0.0625°C) |
| Waterproof | Yes (probe version) |

## Power Consumption

| State | Current |
|-------|---------|
| Deep sleep | ~3µA |
| Sensor reading | ~1.5mA |
| BLE TX | ~15mA |

**Expected battery life**: 6-12 months with 400mAh battery

## Use Cases

- **Pool/Hot tub**: Monitor water temperature
- **Aquarium**: Track fish tank temperature
- **Soil**: Measure soil temperature for gardening
- **Freezer**: Monitor freezer temperature
- **Outdoor**: Weather station temperature
