import logging

import esphome.codegen as cg
from esphome.components import binary_sensor, output, socket, text_sensor, uart
from esphome.components.mdns import CONF_TXT
import esphome.config_validation as cv
from esphome.const import (
    CONF_DISABLED,
    CONF_ID,
    CONF_PORT,
    CONF_PROTOCOL,
    CONF_SERVICE,
    CONF_SERVICES,
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


RAW_UART_PORT = 3008
RAW_UART_SERVICE = "_raw-uart"
RAW_UART_PROTOCOL = "_udp"


def _consume_sockets(config):
    """Register socket needs for this component."""
    # One UDP socket for the raw-uart protocol
    socket.consume_sockets(1, "hm_rf_bridge")(config)
    return config


CONFIG_SCHEMA = cv.All(
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


def _add_mdns_service(full_config):
    """Announce the raw-uart service so the CCU can discover the bridge.

    The mdns component sizes its service storage from its own `services` list, so the service
    is added there instead of registering it from C++. An entry the user already has is kept.
    """
    mdns_config = full_config.get("mdns")
    if mdns_config is None or mdns_config.get(CONF_DISABLED):
        return
    services = mdns_config.setdefault(CONF_SERVICES, [])
    if any(
        s[CONF_SERVICE] == RAW_UART_SERVICE and s[CONF_PROTOCOL] == RAW_UART_PROTOCOL
        for s in services
    ):
        return
    services.append(
        {
            CONF_SERVICE: RAW_UART_SERVICE,
            CONF_PROTOCOL: RAW_UART_PROTOCOL,
            CONF_PORT: RAW_UART_PORT,
            CONF_TXT: {},
        }
    )


def _final_validate(config):
    full_config = fv.full_config.get()
    if full_config.get("wifi"):
        _LOGGER.warning(
            "Because of latency requirements, it is not recommended to use this component with WiFi"
        )

    _add_mdns_service(full_config)

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
