import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.types import ConfigType

from . import CONF_ALPHA2_ID, Alpha2

CONF_CONTROL_MODE = "control_mode"
CONF_OPERATION_MODE = "operation_mode"
CONF_CONTROL_SOURCE = "control_source"
CONF_SETTING = "setting"
CONF_CONST_PRESSURE_LEVELS = "const_pressure_levels"
CONF_PROP_PRESSURE_LEVELS = "prop_pressure_levels"
CONF_CONST_SPEED_LEVELS = "const_speed_levels"

# Panel settings I/II/III read from one ALPHA2 GO. Other pump sizes may use other values;
# the log shows the set points, so they can be checked and overridden in YAML.
DEFAULT_CONST_PRESSURE_LEVELS = [2.5, 5.0, 7.5]  # m
DEFAULT_PROP_PRESSURE_LEVELS = [4.0, 6.0, 8.0]  # m
DEFAULT_CONST_SPEED_LEVELS = [5225, 5877, 6350]  # 86/8 set point, probably RPM


def _levels(value):
    value = cv.ensure_list(cv.positive_float)(value)
    if len(value) != 3:
        raise cv.Invalid("exactly 3 values are needed (I, II, III)")
    return value


# Read from overall_operation_realized_request (86/8), i.e. what the pump actually runs.
TEXT_SENSORS = {
    CONF_CONTROL_MODE: text_sensor.text_sensor_schema(icon="mdi:tune"),
    CONF_OPERATION_MODE: text_sensor.text_sensor_schema(icon="mdi:pump"),
    CONF_CONTROL_SOURCE: text_sensor.text_sensor_schema(icon="mdi:remote"),
    # I, II, III, AUTOADAPT or Advanced (set in Grundfos GO), worked out from the set points.
    CONF_SETTING: text_sensor.text_sensor_schema(icon="mdi:numeric").extend(
        {
            cv.Optional(
                CONF_CONST_PRESSURE_LEVELS, default=DEFAULT_CONST_PRESSURE_LEVELS
            ): _levels,
            cv.Optional(
                CONF_PROP_PRESSURE_LEVELS, default=DEFAULT_PROP_PRESSURE_LEVELS
            ): _levels,
            cv.Optional(
                CONF_CONST_SPEED_LEVELS, default=DEFAULT_CONST_SPEED_LEVELS
            ): _levels,
        }
    ),
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
    if setting := config.get(CONF_SETTING):
        cg.add(parent.set_const_pressure_levels(*setting[CONF_CONST_PRESSURE_LEVELS]))
        cg.add(parent.set_prop_pressure_levels(*setting[CONF_PROP_PRESSURE_LEVELS]))
        cg.add(parent.set_const_speed_levels(*setting[CONF_CONST_SPEED_LEVELS]))
