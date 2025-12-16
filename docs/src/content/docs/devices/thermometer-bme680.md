---
title: Thermometer (BME680)
description: Battery-powered environmental sensor with temperature, humidity, pressure, and gas
---

A comprehensive environmental sensor using BME680 that broadcasts temperature, humidity, pressure, and air quality via BTHome.

## Hardware

- Seeed XIAO BLE (nRF52840)
- BME680 sensor module (I2C)
- 3.7V LiPo battery

## Wiring

| BME680 | XIAO Pin |
|--------|----------|
| VIN | 3V3 |
| GND | GND |
| SDA | D4 (pin 4) |
| SCL | D5 (pin 5) |

```mermaid
graph LR
    subgraph XIAO["Seeed XIAO BLE"]
        V3["3V3"]
        GND["GND"]
        D4["D4 (SDA)"]
        D5["D5 (SCL)"]
    end

    subgraph BME680["BME680 Sensor"]
        VIN["VIN"]
        SGND["GND"]
        SDA["SDA"]
        SCL["SCL"]
    end

    V3 --- VIN
    GND --- SGND
    D4 --- SDA
    D5 --- SCL

    style BME680 fill:#e6ffe6,stroke:#333
```

## Configuration

```yaml
esphome:
  name: thermometer-bme680
  friendly_name: Environment Sensor

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

# I2C bus
i2c:
  sda: 4
  scl: 5
  scan: true

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

  # BME680 sensor
  - platform: bme680
    address: 0x77
    temperature:
      id: bme_temperature
      name: "Temperature"
      oversampling: 16x
    humidity:
      id: bme_humidity
      name: "Humidity"
      oversampling: 16x
    pressure:
      id: bme_pressure
      name: "Pressure"
    gas_resistance:
      id: gas_resistance
      name: "Gas Resistance"
    update_interval: 60s

  # Air Quality Index (simplified calculation)
  - platform: template
    id: air_quality
    name: "Air Quality"
    unit_of_measurement: "%"
    lambda: |-
      // Simple AQI: higher resistance = better air quality
      float resistance = id(gas_resistance).state;
      // Typical range: 10k-300k ohms
      float quality = (resistance - 10000) / (300000 - 10000) * 100;
      if (quality > 100) quality = 100;
      if (quality < 0) quality = 0;
      return quality;
    update_interval: 60s

# BTHome broadcast
bthome:
  min_interval: 30s
  max_interval: 60s
  tx_power: 4
  sensors:
    - type: battery
      id: battery_percent
    - type: temperature
      id: bme_temperature
    - type: humidity
      id: bme_humidity
    - type: pressure
      id: bme_pressure

# Deep sleep for battery savings
deep_sleep:
  run_duration: 30s
  sleep_duration: 5min
```

## Sensor Specifications

| Measurement | Range | Accuracy |
|-------------|-------|----------|
| Temperature | -40 to +85°C | ±1.0°C |
| Humidity | 0 to 100% | ±3% |
| Pressure | 300 to 1100 hPa | ±1 hPa |
| Gas | VOC detection | Relative |

## Power Consumption

| State | Current |
|-------|---------|
| Deep sleep | ~3µA |
| Sensor reading | ~12mA |
| BLE TX | ~15mA |

**Expected battery life**: 3-6 months with 400mAh battery (5min intervals)

:::tip[Extend Battery Life]
Increase `sleep_duration` to 15min or 30min for sensors where real-time data isn't critical.
:::
