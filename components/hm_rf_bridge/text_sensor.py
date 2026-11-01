import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import ENTITY_CATEGORY_DIAGNOSTIC

from . import CONF_HM_RF_BRIDGE_ID, HmRFBridge

DEPENDENCIES = ["hm_rf_bridge"]

TEXT_SENSOR_SETTERS = {
    "radio_module_type": "set_radio_module_type_sensor",
    "firmware_version": "set_firmware_version_sensor",
    "serial": "set_serial_sensor",
    "sgtin": "set_sgtin_sensor",
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_HM_RF_BRIDGE_ID): cv.use_id(HmRFBridge),
        **{
            cv.Optional(key): text_sensor.text_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC
            )
            for key in TEXT_SENSOR_SETTERS
        },
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_HM_RF_BRIDGE_ID])
    for key, setter in TEXT_SENSOR_SETTERS.items():
        if key in config:
            sens = await text_sensor.new_text_sensor(config[key])
            cg.add(getattr(hub, setter)(sens))
