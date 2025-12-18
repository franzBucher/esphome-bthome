---
title: BTHome for ESPHome
description: BTHome v2 BLE Protocol Component for ESPHome
---

**BTHome for ESPHome** is a component that enables your ESP32 or nRF52 devices to broadcast sensor data over Bluetooth Low Energy using the [BTHome v2 protocol](https://bthome.io/). This allows seamless integration with Home Assistant auto-discovery and other BTHome-compatible receivers, including **Shelly devices** for direct device-to-device automations without requiring a hub.

## What is BTHome?

BTHome is an open standard for broadcasting sensor data over Bluetooth Low Energy (BLE). It's designed to be:

- **Energy efficient** - Perfect for battery-powered sensors
- **Simple** - Easy to implement and decode
- **Extensible** - Supports 60+ sensor types
- **Secure** - Optional AES-CCM encryption

## Features

This component provides:

- **BTHome v2 protocol compliance** - Full support for the latest BTHome specification
- **Multi-platform support** - Works on ESP32 (ESP-IDF) and nRF52 (Zephyr)
- **60+ sensor types** - Temperature, humidity, pressure, motion, and many more
- **28 binary sensor types** - Door, window, motion, smoke, and others
- **Optional AES-CCM encryption** - Secure your sensor data
- **Immediate advertising** - Trigger instant updates on sensor changes
- **Configurable TX power** - Optimize range vs battery life
- **Home Assistant auto-discovery** - No manual configuration needed
- **Shelly device compatibility** - Direct device-to-device automations without a hub

## Supported Platforms

| Platform | Framework | Status |
|----------|-----------|--------|
| ESP32 | ESP-IDF | Supported |
| ESP32-C3 | ESP-IDF | Supported |
| ESP32-S3 | ESP-IDF | Supported |
| nRF52840 (XIAO BLE) | Zephyr | Supported |

## How It Works

```
                                          ┌─────────────────┐
                                     ┌──► │  Home Assistant │
┌─────────────┐     BLE Advertisement│    │  (BTHome Int.)  │
│  ESP32 or   │ ─────────────────────┤    └─────────────────┘
│   nRF52     │     BTHome v2 Format │    ┌─────────────────┐
└─────────────┘                      └──► │  Shelly Devices │
      │                                   │  (Direct Auto.) │
      │ Reads sensors                     └─────────────────┘
      ▼
┌─────────────┐
│  BME280,    │
│  PIR, etc.  │
└─────────────┘
```

The component:
1. Reads sensor values from connected sensors
2. Encodes the data according to BTHome v2 specification
3. Broadcasts the data via BLE advertisements
4. Home Assistant's BTHome integration automatically discovers and displays the sensors
5. Shelly devices can receive broadcasts directly for local automations

## Next Steps

- [Installation](/getting-started/installation) - Add the component to your project
- [Quick Start](/getting-started/quick-start) - Get up and running in minutes
