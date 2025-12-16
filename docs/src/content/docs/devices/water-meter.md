---
title: Water Meter (Pulse Counter)
description: Battery-powered water usage monitor using pulse counter
---

A water consumption monitor that counts pulses from a water meter and broadcasts usage via BTHome.

## Hardware

- Seeed XIAO BLE (nRF52840)
- Water meter with pulse output (reed switch or Hall effect)
- 3.7V LiPo battery

Common water meters output 1 pulse per liter or 1 pulse per 10 liters.

## Wiring

| Water Meter | XIAO Pin |
|-------------|----------|
| Pulse output | D2 (pin 2) |
| GND | GND |

Most water meter pulse outputs are open-drain/open-collector, so we use the internal pull-up.

```mermaid
graph LR
    subgraph XIAO["Seeed XIAO BLE"]
        D2["D2 (Pin 2)"]
        GND["GND"]
        V3["3V3"]
    end

    subgraph Meter["Water Meter"]
        PULSE["Pulse Output<br/>(Open Drain)"]
        MGND["GND"]
    end

    D2 --- PULSE
    D2 -.- PULLUP["Internal<br/>Pull-up"]
    PULLUP -.- V3
    GND --- MGND

    style Meter fill:#e6f3ff,stroke:#333
    style PULLUP fill:#fff,stroke:#999,stroke-dasharray: 5 5
```

## Configuration

```yaml
esphome:
  name: water-meter
  friendly_name: Water Meter

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

# Global for persistent counter
globals:
  - id: total_pulses
    type: int
    restore_value: true
    initial_value: '0'

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

  # Pulse counter for water meter
  - platform: pulse_counter
    pin:
      number: 2
      mode: INPUT_PULLUP
    id: water_pulses
    name: "Water Flow"
    unit_of_measurement: "L/min"
    update_interval: 60s
    filters:
      # Adjust multiplier based on your meter
      # 1 pulse = 1 liter: multiply by 1
      # 1 pulse = 10 liters: multiply by 10
      - multiply: 1.0
    total:
      id: water_total_pulses
      name: "Water Pulses Total"

  # Total water consumption in liters
  - platform: template
    id: water_total
    name: "Water Total"
    unit_of_measurement: "L"
    accuracy_decimals: 1
    lambda: |-
      return id(water_total_pulses).state;
    update_interval: 60s

# BTHome broadcast
bthome:
  min_interval: 30s
  max_interval: 60s
  tx_power: 4
  sensors:
    - type: battery
      id: battery_percent
    - type: water
      id: water_total
    - type: volume_flow_rate
      id: water_pulses

# No deep sleep - need to count pulses continuously
# For battery operation, consider a supercapacitor backup
```

## Configuration for 10L/pulse Meters

If your water meter outputs 1 pulse per 10 liters:

```yaml
sensor:
  - platform: pulse_counter
    pin:
      number: 2
      mode: INPUT_PULLUP
    id: water_pulses
    name: "Water Flow"
    unit_of_measurement: "L/min"
    update_interval: 60s
    filters:
      - multiply: 10.0  # Each pulse = 10 liters
    total:
      id: water_total_pulses
      name: "Water Pulses Total"
      filters:
        - multiply: 10.0
```

## Battery Considerations

:::caution[Continuous Operation Required]
Unlike other sensors, water meters require continuous pulse counting. Deep sleep is **not recommended** as pulses would be missed.
:::

For battery operation, consider:

1. **USB power** when possible
2. **Solar panel** with battery backup
3. **Large battery** (2000mAh+) with reduced TX power

```yaml
bthome:
  tx_power: -4  # Reduce power to extend battery
  min_interval: 60s
  max_interval: 120s
```

## Meter Types

| Meter Type | Typical Output | Notes |
|------------|----------------|-------|
| Residential | 1 pulse/L | Most common |
| Industrial | 1 pulse/10L | Higher flow |
| Garden | 1 pulse/L | Brass body |
| Plastic inline | 1 pulse/L | Budget option |

## Power Consumption

| State | Current |
|-------|---------|
| Idle (counting) | ~3mA |
| BLE TX | ~15mA |

**Expected battery life**: 1-2 months with 2000mAh battery (no sleep)

:::tip[Extend Battery Life]
Use a solar panel or connect to USB power for continuous operation. Battery-only is suitable for short-term monitoring.
:::
