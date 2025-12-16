---
title: 1-Gang Pushbutton
description: Battery-powered single pushbutton with BTHome
---

A battery-efficient single pushbutton using nRF52 that broadcasts button presses via BTHome.

## Hardware

- Seeed XIAO BLE (nRF52840)
- Momentary pushbutton
- 3.7V LiPo battery

## Wiring

| Component | XIAO Pin |
|-----------|----------|
| Button | D2 → GND |
| Battery+ | BAT+ |
| Battery- | BAT- |

```mermaid
graph TD
    subgraph XIAO["Seeed XIAO BLE"]
        D2["D2 (Pin 2)"]
        GND["GND"]
        BATP["BAT+"]
        BATN["BAT-"]
    end

    subgraph Button["Pushbutton"]
        BTN["Momentary<br/>Switch"]
    end

    subgraph Battery["LiPo Battery"]
        VBAT["3.7V +"]
        GBAT["GND -"]
    end

    BTN --- D2
    BTN --- GND
    VBAT --- BATP
    GBAT --- BATN

    style Button fill:#e6f3ff,stroke:#333
    style Battery fill:#ffe6e6,stroke:#333
```

## Configuration

```yaml
esphome:
  name: pushbutton-1gang
  friendly_name: Pushbutton

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

# Pushbutton
binary_sensor:
  - platform: gpio
    pin:
      number: 2
      mode: INPUT_PULLUP
    id: button
    name: "Button"
    filters:
      - invert:
      - delayed_on: 10ms
      - delayed_off: 10ms

# BTHome broadcast
bthome:
  min_interval: 5s
  max_interval: 30s
  tx_power: 4
  sensors:
    - type: battery
      id: battery_percent
  binary_sensors:
    - type: generic_boolean
      id: button
      advertise_immediately: true

# Deep sleep - wake on button press
deep_sleep:
  run_duration: 10s
  sleep_duration: 5min
  wakeup_pin:
    number: 2
    mode: INPUT_PULLUP
    inverted: true
```

## Power Consumption

| State | Current |
|-------|---------|
| Deep sleep | ~3µA |
| Active | ~5mA |
| TX burst | ~15mA |

**Expected battery life**: 6-12 months with 400mAh battery
