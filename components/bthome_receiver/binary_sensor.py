"""
BTHome v2 BLE Receiver - Binary Sensor Platform

Receives binary sensor data from external BTHome v2 BLE devices.
Supports all 28 binary sensor types defined in the BTHome v2 specification.
"""

import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_MAC_ADDRESS,
)

from . import (
    BINARY_SENSOR_TYPES,
    BTHomeBinarySensor,
    BTHomeDevice,
    BTHomeReceiverHub,
    CONF_ENCRYPTION_KEY,
    bthome_receiver_ns,
    validate_encryption_key,
)

DEPENDENCIES = ["bthome_receiver"]

# Map binary sensor types to ESPHome device classes
DEVICE_CLASS_MAP = {
    "generic_boolean": None,
    "power": "power",
    "opening": "opening",
    "battery_low": "battery",
    "battery_charging": "battery_charging",
    "carbon_monoxide": "carbon_monoxide",
    "cold": "cold",
    "connectivity": "connectivity",
    "door": "door",
    "garage_door": "garage_door",
    "gas": "gas",
    "heat": "heat",
    "light": "light",
    "lock": "lock",
    "moisture_binary": "moisture",
    "motion": "motion",
    "moving": "moving",
    "occupancy": "occupancy",
    "plug": "plug",
    "presence": "presence",
    "problem": "problem",
    "running": "running",
    "safety": "safety",
    "smoke": "smoke",
    "sound": "sound",
    "tamper": "tamper",
    "vibration": "vibration",
    "window": "window",
}


def _binary_sensor_schema(sensor_type: str):
    """Generate schema for a single binary sensor type."""
    device_class = DEVICE_CLASS_MAP.get(sensor_type)
    if device_class:
        return binary_sensor.binary_sensor_schema(device_class=device_class)
    return binary_sensor.binary_sensor_schema()


# Build the complete config schema with all binary sensor types as optional fields
_SENSOR_TYPE_SCHEMA = {}
for _sensor_type in BINARY_SENSOR_TYPES.keys():
    _SENSOR_TYPE_SCHEMA[cv.Optional(_sensor_type)] = _binary_sensor_schema(_sensor_type)


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(BTHomeDevice),
        cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
        cv.Optional(CONF_ENCRYPTION_KEY): validate_encryption_key,
        cv.GenerateID("bthome_receiver_id"): cv.use_id(BTHomeReceiverHub),
        **_SENSOR_TYPE_SCHEMA,
    }
)


async def to_code(config):
    """Code generator for BTHome receiver binary sensor platform."""
    hub = await cg.get_variable(config["bthome_receiver_id"])
    mac = config[CONF_MAC_ADDRESS]

    # Create the BTHomeDevice
    device_var = cg.new_Pvariable(config[CONF_ID], hub)
    cg.add(device_var.set_mac_address(mac.as_hex))

    # Set encryption key if provided
    if CONF_ENCRYPTION_KEY in config:
        key = config[CONF_ENCRYPTION_KEY]
        key_bytes = [cg.RawExpression(f"0x{key[i:i+2]}") for i in range(0, len(key), 2)]
        key_array = cg.RawExpression(f"std::array<uint8_t, 16>{{{{{', '.join(str(b) for b in key_bytes)}}}}}")
        cg.add(device_var.set_encryption_key(key_array))

    # Create binary sensors for each configured type
    for sensor_type, object_id in BINARY_SENSOR_TYPES.items():
        if sensor_type in config:
            sensor_config = config[sensor_type]

            # Create the ESPHome binary sensor
            sens = await binary_sensor.new_binary_sensor(sensor_config)

            # Register binary sensor with device (object_id, binary_sensor*)
            cg.add(device_var.add_binary_sensor(object_id, sens))

    # Register device with hub
    cg.add(hub.register_device(device_var))
