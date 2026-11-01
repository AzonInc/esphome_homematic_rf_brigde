import logging

import esphome.codegen as cg
from esphome.components import binary_sensor, output, socket, text_sensor, uart
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_UART_ID,
    DEVICE_CLASS_CONNECTIVITY,
    ENTITY_CATEGORY_DIAGNOSTIC,
)
import esphome.final_validate as fv

_LOGGER = logging.getLogger(__name__)

AUTO_LOAD = ["binary_sensor", "text_sensor", "socket"]
DEPENDENCIES = ["uart", "network"]

hm_rf_bridge_ns = cg.esphome_ns.namespace("hm_rf_bridge")
HmRFBridge = hm_rf_bridge_ns.class_(
    "HmRFBridge", cg.PollingComponent, uart.UARTDevice
)

CONF_RESET_OUTPUT = "reset_output"
CONF_RED_LED = "red_led"
CONF_GREEN_LED = "green_led"
CONF_BLUE_LED = "blue_led"
CONF_CONNECTED = "connected"
CONF_RADIO_MODULE_TYPE = "radio_module_type"
CONF_FIRMWARE_VERSION = "firmware_version"
CONF_SERIAL = "serial"
CONF_SGTIN = "sgtin"
# Deprecated spelling, still accepted for existing configurations
CONF_SGTIN_LEGACY = "SGTIN"

LED_SETTERS = {
    CONF_RED_LED: "set_red_led",
    CONF_GREEN_LED: "set_green_led",
    CONF_BLUE_LED: "set_blue_led",
}

TEXT_SENSOR_SETTERS = {
    CONF_RADIO_MODULE_TYPE: "set_radio_module_type_sensor",
    CONF_FIRMWARE_VERSION: "set_firmware_version_sensor",
    CONF_SERIAL: "set_serial_sensor",
    CONF_SGTIN: "set_sgtin_sensor",
}


def _rename_legacy_sgtin(config):
    if CONF_SGTIN_LEGACY not in config:
        return config
    if CONF_SGTIN in config:
        raise cv.Invalid(f"Use either '{CONF_SGTIN}' or '{CONF_SGTIN_LEGACY}', not both")
    _LOGGER.warning(
        "hm_rf_bridge: '%s' is deprecated, please use '%s' instead",
        CONF_SGTIN_LEGACY,
        CONF_SGTIN,
    )
    config = config.copy()
    config[CONF_SGTIN] = config.pop(CONF_SGTIN_LEGACY)
    return config


def _consume_sockets(config):
    """Register socket needs for this component."""
    #  1 listening socket + 1 concurrent client connections
    socket.consume_sockets(2, "HmRFBridge")(config)
    return config


CONFIG_SCHEMA = cv.All(
    _rename_legacy_sgtin,
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(HmRFBridge),
            # The bridge drives the ESP-IDF UART driver directly, so it needs the native UART
            # (instead of uart.UART_DEVICE_SCHEMA, which accepts any UART implementation)
            cv.GenerateID(CONF_UART_ID): cv.use_id(uart.IDFUARTComponent),
            cv.Required(CONF_RESET_OUTPUT): cv.use_id(output.BinaryOutput),
            cv.Optional(CONF_RED_LED): cv.use_id(output.BinaryOutput),
            cv.Optional(CONF_GREEN_LED): cv.use_id(output.BinaryOutput),
            cv.Optional(CONF_BLUE_LED): cv.use_id(output.BinaryOutput),
            cv.Optional(CONF_CONNECTED): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_CONNECTIVITY,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            **{
                cv.Optional(key): text_sensor.text_sensor_schema(
                    entity_category=ENTITY_CATEGORY_DIAGNOSTIC
                )
                for key in TEXT_SENSOR_SETTERS
            },
        }
    ).extend(cv.polling_component_schema("10s")),
    cv.only_on_esp32,
    _consume_sockets,
)


def _final_validate(config):
    if fv.full_config.get().get("wifi"):
        _LOGGER.warning(
            "Because of latency requirements, it is not recommended to use this component with WiFi"
        )

    return uart.final_validate_device_schema(
        "hm_rf_bridge",
        baud_rate=115200,
        require_tx=True,
        require_rx=True,
        data_bits=8,
        parity="NONE",
        stop_bits=1,
    )(config)


FINAL_VALIDATE_SCHEMA = _final_validate


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    reset_output = await cg.get_variable(config[CONF_RESET_OUTPUT])
    cg.add(var.set_reset_output(reset_output))

    for key, setter in LED_SETTERS.items():
        if key in config:
            led = await cg.get_variable(config[key])
            cg.add(getattr(var, setter)(led))

    if CONF_CONNECTED in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_CONNECTED])
        cg.add(var.set_connected_sensor(sens))

    for key, setter in TEXT_SENSOR_SETTERS.items():
        if key in config:
            sens = await text_sensor.new_text_sensor(config[key])
            cg.add(getattr(var, setter)(sens))
