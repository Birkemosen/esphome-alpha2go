import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.types import ConfigType

from . import CONF_ALPHA2_ID, Alpha2

CONF_CONTROL_MODE = "control_mode"
CONF_OPERATION_MODE = "operation_mode"
CONF_CONTROL_SOURCE = "control_source"

# Read from overall_operation_realized_request (86/8), i.e. what the pump actually runs.
TEXT_SENSORS = {
    CONF_CONTROL_MODE: text_sensor.text_sensor_schema(icon="mdi:tune"),
    CONF_OPERATION_MODE: text_sensor.text_sensor_schema(icon="mdi:pump"),
    CONF_CONTROL_SOURCE: text_sensor.text_sensor_schema(icon="mdi:remote"),
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ALPHA2_ID): cv.use_id(Alpha2),
        **{cv.Optional(key): schema for key, schema in TEXT_SENSORS.items()},
    }
)


async def to_code(config: ConfigType) -> None:
    parent = await cg.get_variable(config[CONF_ALPHA2_ID])
    for key in TEXT_SENSORS:
        if sensor_config := config.get(key):
            sens = await text_sensor.new_text_sensor(sensor_config)
            cg.add(getattr(parent, f"set_{key}_text_sensor")(sens))
