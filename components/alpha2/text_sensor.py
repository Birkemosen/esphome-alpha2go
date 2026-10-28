import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.types import ConfigType

from . import CONF_ALPHA2_ID, Alpha2

CONF_CONTROL_MODE = "control_mode"
CONF_OPERATION_MODE = "operation_mode"
CONF_CONTROL_SOURCE = "control_source"
CONF_SETTING = "setting"
CONF_SELECTED_SETTING = "selected_setting"
CONF_CONST_PRESSURE_LEVELS = "const_pressure_levels"
CONF_PROP_PRESSURE_LEVELS = "prop_pressure_levels"
CONF_CONST_SPEED_LEVELS = "const_speed_levels"

# Panel settings I/II/III read from one ALPHA2 GO. They are only used until the pump's own
# values (86/351..359) have been read after connecting.
DEFAULT_CONST_PRESSURE_LEVELS = [2.5, 5.0, 7.5]  # m
DEFAULT_PROP_PRESSURE_LEVELS = [4.0, 6.0, 8.0]  # m
DEFAULT_CONST_SPEED_LEVELS = [5225, 5877, 6350]  # RPM


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
    # Panel setting (86/350) as reported by the pump, e.g. "Proportional pressure II".
    CONF_SELECTED_SETTING: text_sensor.text_sensor_schema(icon="mdi:numeric"),
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
    # Both setting sensors compare against the levels; the YAML ones live under setting.
    if CONF_SETTING in config or CONF_SELECTED_SETTING in config:
        setting = config.get(CONF_SETTING, {})
        levels = (
            (CONF_CONST_PRESSURE_LEVELS, DEFAULT_CONST_PRESSURE_LEVELS, "set_const_pressure_levels"),
            (CONF_PROP_PRESSURE_LEVELS, DEFAULT_PROP_PRESSURE_LEVELS, "set_prop_pressure_levels"),
            (CONF_CONST_SPEED_LEVELS, DEFAULT_CONST_SPEED_LEVELS, "set_const_speed_levels"),
        )
        for key, default, setter in levels:
            cg.add(getattr(parent, setter)(*setting.get(key, default)))
