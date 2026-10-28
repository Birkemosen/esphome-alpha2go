import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.types import ConfigType

from . import CONF_ALPHA2_ID, Alpha2

CONF_AUTOADAPT = "autoadapt"
CONF_AUTONIGHT = "autonight"
CONF_MAX_FLOW_LIMIT_ENABLED = "max_flow_limit_enabled"
CONF_MIN_FLOW_LIMIT_ENABLED = "min_flow_limit_enabled"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ALPHA2_ID): cv.use_id(Alpha2),
        # On while the pump runs CP or PP AUTOADAPT (86/8 control mode 14 or 13).
        cv.Optional(CONF_AUTOADAPT): binary_sensor.binary_sensor_schema(icon="mdi:auto-mode"),
        # AutoNight (night setback) enabled (90/8).
        cv.Optional(CONF_AUTONIGHT): binary_sensor.binary_sensor_schema(icon="mdi:weather-night"),
        # Max / min flow limiter switched on (86/600, 86/601).
        cv.Optional(CONF_MAX_FLOW_LIMIT_ENABLED): binary_sensor.binary_sensor_schema(
            icon="mdi:car-speed-limiter"
        ),
        cv.Optional(CONF_MIN_FLOW_LIMIT_ENABLED): binary_sensor.binary_sensor_schema(
            icon="mdi:car-speed-limiter"
        ),
    }
)


async def to_code(config: ConfigType) -> None:
    parent = await cg.get_variable(config[CONF_ALPHA2_ID])
    for key in (CONF_AUTOADAPT, CONF_AUTONIGHT, CONF_MAX_FLOW_LIMIT_ENABLED, CONF_MIN_FLOW_LIMIT_ENABLED):
        if conf := config.get(key):
            sens = await binary_sensor.new_binary_sensor(conf)
            cg.add(getattr(parent, f"set_{key}_binary_sensor")(sens))
