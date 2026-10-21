import esphome.codegen as cg
from esphome.components import ble_client, sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_CURRENT,
    CONF_FLOW,
    CONF_HEAD,
    CONF_ID,
    CONF_POWER,
    CONF_SPEED,
    CONF_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLTAGE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_AMPERE,
    UNIT_CELSIUS,
    UNIT_CUBIC_METER_PER_HOUR,
    UNIT_KELVIN,
    UNIT_METER,
    UNIT_REVOLUTIONS_PER_MINUTE,
    UNIT_VOLT,
    UNIT_WATT,
)
from esphome.types import ConfigType

from . import Alpha2

CONF_MEDIA_TEMPERATURE = "media_temperature"
CONF_DIFFERENTIAL_TEMPERATURE = "differential_temperature"
CONF_FLOW_COEFFICIENT_KV = "flow_coefficient_kv"
CONF_CONVERTER_TEMPERATURE = "converter_temperature"
CONF_INVERTER_TEMPERATURE = "inverter_temperature"
CONF_GRID_VOLTAGE = "grid_voltage"
CONF_DC_LINK_VOLTAGE = "dc_link_voltage"
CONF_ABSOLUTE_SPEED = "absolute_speed"
CONF_SETPOINT = "setpoint"
CONF_SETPOINT_HEAD = "setpoint_head"
CONF_CONST_PRESSURE_SETPOINT = "const_pressure_setpoint"
CONF_PROP_PRESSURE_SETPOINT = "prop_pressure_setpoint"
CONF_AUTOADAPT_RADIATOR_SETPOINT = "autoadapt_radiator_setpoint"
CONF_AUTOADAPT_UNDERFLOOR_SETPOINT = "autoadapt_underfloor_setpoint"
CONF_EXPERIMENTAL_CLASS10_WRITE = "experimental_class10_write"


def _measurement(**kwargs):
    return sensor.sensor_schema(
        accuracy_decimals=kwargs.pop("accuracy_decimals", 2),
        state_class=STATE_CLASS_MEASUREMENT,
        **kwargs,
    )


def _temperature(**kwargs):
    return _measurement(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        **kwargs,
    )


def _voltage(**kwargs):
    return _measurement(
        unit_of_measurement=UNIT_VOLT, device_class=DEVICE_CLASS_VOLTAGE, **kwargs
    )


# config key -> schema. The C++ setter is set_<key>_sensor().
SENSORS = {
    # PumpedMediaRelatedProcessValuesExtended (93/290)
    CONF_FLOW: _measurement(unit_of_measurement=UNIT_CUBIC_METER_PER_HOUR),
    CONF_HEAD: _measurement(unit_of_measurement=UNIT_METER),
    CONF_MEDIA_TEMPERATURE: _temperature(),
    CONF_DIFFERENTIAL_TEMPERATURE: _measurement(
        unit_of_measurement=UNIT_KELVIN, accuracy_decimals=1
    ),
    # TODO: unit of the Kv value is not confirmed, so none is set by default.
    CONF_FLOW_COEFFICIENT_KV: _measurement(),
    # ProtectedMotorStateDetails (87/69)
    CONF_POWER: _measurement(
        unit_of_measurement=UNIT_WATT, device_class=DEVICE_CLASS_POWER
    ),
    CONF_CURRENT: _measurement(
        unit_of_measurement=UNIT_AMPERE, device_class=DEVICE_CLASS_CURRENT
    ),
    CONF_SPEED: _measurement(unit_of_measurement=UNIT_REVOLUTIONS_PER_MINUTE),
    CONF_ABSOLUTE_SPEED: _measurement(unit_of_measurement=UNIT_REVOLUTIONS_PER_MINUTE),
    CONF_VOLTAGE: _voltage(),
    CONF_GRID_VOLTAGE: _voltage(),
    CONF_DC_LINK_VOLTAGE: _voltage(entity_category=ENTITY_CATEGORY_DIAGNOSTIC),
    CONF_CONVERTER_TEMPERATURE: _temperature(
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC
    ),
    CONF_INVERTER_TEMPERATURE: _temperature(
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC
    ),
    # overall_operation_realized_request (86/8)
    # TODO: the set point unit depends on the control mode, so none is set by default.
    CONF_SETPOINT: _measurement(),
    # Same set point in m; only published in the pressure control modes.
    CONF_SETPOINT_HEAD: _measurement(unit_of_measurement=UNIT_METER),
    # ControlModeUserConfiguration local_set_point (86/16, 86/18, 86/20, 86/22).
    # UNVERIFIED: converted from Pa like the realized set point.
    CONF_CONST_PRESSURE_SETPOINT: _measurement(
        unit_of_measurement=UNIT_METER, entity_category=ENTITY_CATEGORY_DIAGNOSTIC
    ),
    CONF_PROP_PRESSURE_SETPOINT: _measurement(
        unit_of_measurement=UNIT_METER, entity_category=ENTITY_CATEGORY_DIAGNOSTIC
    ),
    CONF_AUTOADAPT_RADIATOR_SETPOINT: _measurement(
        unit_of_measurement=UNIT_METER, entity_category=ENTITY_CATEGORY_DIAGNOSTIC
    ),
    CONF_AUTOADAPT_UNDERFLOOR_SETPOINT: _measurement(
        unit_of_measurement=UNIT_METER, entity_category=ENTITY_CATEGORY_DIAGNOSTIC
    ),
}

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Alpha2),
            # UNVERIFIED class 10 SET used by alpha2.write_object. Keep disabled unless testing.
            cv.Optional(CONF_EXPERIMENTAL_CLASS10_WRITE, default=False): cv.boolean,
            **{cv.Optional(key): schema for key, schema in SENSORS.items()},
        }
    )
    .extend(ble_client.BLE_CLIENT_SCHEMA)
    .extend(cv.polling_component_schema("15s"))
)


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await ble_client.register_ble_node(var, config)
    cg.add(var.set_experimental_class10_write(config[CONF_EXPERIMENTAL_CLASS10_WRITE]))

    for key in SENSORS:
        if sensor_config := config.get(key):
            sens = await sensor.new_sensor(sensor_config)
            cg.add(getattr(var, f"set_{key}_sensor")(sens))
