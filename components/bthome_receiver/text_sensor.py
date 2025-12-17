"""
BTHome Receiver Text Sensor Platform

Defines text sensor entities for receiving text and raw data from BTHome devices.
Supports object IDs 0x53 (text/UTF-8) and 0x54 (raw/binary hex).
"""

import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_MAC_ADDRESS

from . import (
    bthome_receiver_ns,
    BTHomeReceiverHub,
    BTHomeDevice,
    BTHomeTextSensor,
    CONF_ENCRYPTION_KEY,
    validate_encryption_key,
)

DEPENDENCIES = ["bthome_receiver"]

CONF_TEXT = "text"
CONF_RAW = "raw"

# BTHome v2 Text/Raw Data Object IDs
TEXT_SENSOR_TYPES = {
    "text": 0x53,  # UTF-8 string
    "raw": 0x54,   # Binary data (displayed as hex)
}

# Build CONFIG_SCHEMA
CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(BTHomeDevice),
        cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
        cv.Optional(CONF_ENCRYPTION_KEY): validate_encryption_key,
        cv.GenerateID("bthome_receiver_id"): cv.use_id(BTHomeReceiverHub),
        cv.Optional(CONF_TEXT): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_RAW): text_sensor.text_sensor_schema(),
    }
)


async def to_code(config):
    """Generate C++ code for the text sensor platform."""
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

    # Create text sensors for each configured type
    for sensor_type in [CONF_TEXT, CONF_RAW]:
        if sensor_type in config:
            sensor_config = config[sensor_type]
            object_id = TEXT_SENSOR_TYPES[sensor_type]

            # Create the ESPHome text sensor
            sens = await text_sensor.new_text_sensor(sensor_config)

            # Register text sensor with device (object_id, text_sensor*)
            cg.add(device_var.add_text_sensor(object_id, sens))

    # Register device with hub
    cg.add(hub.register_device(device_var))
