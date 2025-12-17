---
title: BTHome Scanner App
description: Android app for discovering and debugging BTHome devices
---

The BTHome Scanner is a companion Android app for discovering, monitoring, and debugging BTHome devices in real-time.

## Features

- **Device Discovery** - Automatically scans for nearby BTHome v2 devices
- **Real-time Updates** - See sensor values update as devices broadcast
- **Encryption Support** - Decrypt encrypted BTHome devices with your encryption key
- **Signal Strength** - Monitor Bluetooth signal quality for each device
- **Debug Info** - Copy raw advertisement data for troubleshooting

## Download

Download the latest APK from the [GitHub Releases](https://github.com/dz0ny/esphome-bthome/releases) page.

:::note[Android Only]
The app is currently available for Android devices only. iOS support may be added in the future.
:::

## Usage

### Scanning for Devices

1. Open the app and grant Bluetooth permissions when prompted
2. The app will automatically start scanning for nearby BTHome devices
3. Devices appear in a list with their name, MAC address, and signal strength

### Viewing Sensor Data

Tap on any BTHome device to see:
- All sensor measurements (temperature, humidity, battery, etc.)
- Signal strength (RSSI)
- Last advertisement time
- Encryption status

### Adding Encryption Keys

For encrypted BTHome devices:

1. Tap on the encrypted device (shown with a lock icon)
2. Enter the 32-character hex encryption key
3. The app will decrypt and display sensor values

The encryption key is stored securely on your device for future use.

:::tip[Finding Your Encryption Key]
The encryption key is the same one configured in your ESPHome YAML under `bthome: encryption_key`.
:::

### Debugging

Long-press any device to copy the raw BLE advertisement data to clipboard. This is useful for:
- Verifying your device is broadcasting correctly
- Debugging encryption issues
- Reporting bugs

## Requirements

- Android 6.0 (Marshmallow) or higher
- Bluetooth Low Energy (BLE) support
- Location permission (required by Android for BLE scanning)

## Building from Source

The app source code is in the `flutter/` directory. To build:

```bash
cd flutter
flutter pub get
flutter build apk --release
```

The APK will be at `build/app/outputs/flutter-apk/app-release.apk`.
