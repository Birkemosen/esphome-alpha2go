from esphome import automation
import esphome.codegen as cg
from esphome.components import ble_client
import esphome.config_validation as cv
from esphome.const import (
    CONF_DATA,
    CONF_ID,
    CONF_MODE,
    CONF_TARGET,
    CONF_TYPE,
    CONF_VALUE,
    CONF_VERSION,
)

CODEOWNERS = ["@parameter-pollution"]

CONF_ALPHA2_ID = "alpha2_id"
CONF_LIMIT = "limit"
CONF_ENABLED = "enabled"
CONF_FLOW = "flow"
CONF_HEAD = "head"
CONF_OBJECT_ID = "object_id"
CONF_SUB_ID = "sub_id"

alpha2_ns = cg.esphome_ns.namespace("alpha2")
Alpha2 = alpha2_ns.class_("Alpha2", ble_client.BLEClientNode, cg.PollingComponent)

CommandAction = alpha2_ns.class_("CommandAction", automation.Action, cg.Parented.template(Alpha2))
SetControlModeAction = alpha2_ns.class_(
    "SetControlModeAction", automation.Action, cg.Parented.template(Alpha2)
)
SetSetpointRemAction = alpha2_ns.class_(
    "SetSetpointRemAction", automation.Action, cg.Parented.template(Alpha2)
)
SetSettingAction = alpha2_ns.class_("SetSettingAction", automation.Action, cg.Parented.template(Alpha2))
SetSetpointAction = alpha2_ns.class_(
    "SetSetpointAction", automation.Action, cg.Parented.template(Alpha2)
)
SetDutyPointAction = alpha2_ns.class_(
    "SetDutyPointAction", automation.Action, cg.Parented.template(Alpha2)
)
SetFlowLimitAction = alpha2_ns.class_(
    "SetFlowLimitAction", automation.Action, cg.Parented.template(Alpha2)
)
SetAutonightAction = alpha2_ns.class_(
    "SetAutonightAction", automation.Action, cg.Parented.template(Alpha2)
)
WriteObjectAction = alpha2_ns.class_(
    "WriteObjectAction", automation.Action, cg.Parented.template(Alpha2)
)
WriteTestAction = alpha2_ns.class_(
    "WriteTestAction", automation.Action, cg.Parented.template(Alpha2)
)
WriteTestTarget = alpha2_ns.enum("WriteTestTarget")
WRITE_TEST_TARGETS = {
    # class 5 ref_rem: the verified write path
    "ref_rem": WriteTestTarget.WRITE_TEST_REF_REM,
    # class 10 SET of AutoNight 90/8, needs experimental_class10_write: true
    "autonight": WriteTestTarget.WRITE_TEST_AUTONIGHT,
    # class 3 START, only sent if 86/8 shows the pump already running in Auto
    "start": WriteTestTarget.WRITE_TEST_START,
}

# Class 3 commands.
COMMANDS = {
    "start": 6,
    "stop": 5,
    "set_min": 25,
    "set_max": 26,
    "remote": 7,
    "local": 8,
}

# Values accepted by alpha2.set_control_mode, mapped to class 3 commands in C++.
# There is no command for AUTOADAPT (52 was rejected with "unknown id"); use alpha2.set_setting.
CONTROL_MODES = [
    "const_pressure",
    "prop_pressure",
    "const_speed",
    "const_flow",  # ALPHA2 GO only
]

# Values accepted by alpha2.set_setting. Written as observed on the BLE link (86/10 and the user
# config of the control mode), see README.
SETTINGS = [
    "const_pressure_autoadapt",
    "prop_pressure_autoadapt",
    "const_pressure_1",
    "const_pressure_2",
    "const_pressure_3",
    "prop_pressure_1",
    "prop_pressure_2",
    "prop_pressure_3",
    "const_speed_1",
    "const_speed_2",
    "const_speed_3",
]

# Modes accepted by alpha2.set_setpoint: pressure in m, constant speed in RPM, constant flow in m³/h.
SETPOINT_MODES = ["const_pressure", "prop_pressure", "const_speed", "const_flow"]

FLOW_LIMITS = ["max", "min"]

DUTY_POINT_MODES = ["const_pressure", "prop_pressure"]

# class10_set header (id, sub, type, version, size) is 9 bytes of the 63 byte APDU limit.
CLASS10_SET_MAX_DATA = 63 - 9

ALPHA2_ACTION_SCHEMA = automation.maybe_simple_id({cv.GenerateID(): cv.use_id(Alpha2)})


def _register_command(name: str, command: int) -> None:
    @automation.register_action(
        f"alpha2.{name}", CommandAction, ALPHA2_ACTION_SCHEMA, synchronous=True
    )
    async def _to_code(config, action_id, template_arg, args):
        var = cg.new_Pvariable(action_id, template_arg)
        await cg.register_parented(var, config[CONF_ID])
        cg.add(var.set_command(command, name))
        return var


for _name, _command in COMMANDS.items():
    _register_command(_name, _command)


@automation.register_action(
    "alpha2.set_control_mode",
    SetControlModeAction,
    cv.maybe_simple_value(
        {
            cv.GenerateID(): cv.use_id(Alpha2),
            cv.Required(CONF_MODE): cv.templatable(cv.one_of(*CONTROL_MODES, lower=True)),
        },
        key=CONF_MODE,
    ),
    synchronous=True,
)
async def set_control_mode_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_MODE], args, cg.std_string)
    cg.add(var.set_mode(templ))
    return var


@automation.register_action(
    "alpha2.set_setpoint_rem",
    SetSetpointRemAction,
    cv.maybe_simple_value(
        {
            cv.GenerateID(): cv.use_id(Alpha2),
            cv.Required(CONF_VALUE): cv.templatable(cv.float_range(min=0.0, max=100.0)),
        },
        key=CONF_VALUE,
    ),
    synchronous=True,
)
async def set_setpoint_rem_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_VALUE], args, cg.float_)
    cg.add(var.set_value(templ))
    return var


@automation.register_action(
    "alpha2.set_setting",
    SetSettingAction,
    cv.maybe_simple_value(
        {
            cv.GenerateID(): cv.use_id(Alpha2),
            cv.Required(CONF_VALUE): cv.templatable(cv.one_of(*SETTINGS, lower=True)),
        },
        key=CONF_VALUE,
    ),
    synchronous=True,
)
async def set_setting_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_VALUE], args, cg.std_string)
    cg.add(var.set_setting(templ))
    return var


# Values outside the min/max in the pump's factory config are refused before anything is written.
@automation.register_action(
    "alpha2.set_setpoint",
    SetSetpointAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(Alpha2),
            cv.Required(CONF_MODE): cv.templatable(cv.one_of(*SETPOINT_MODES, lower=True)),
            cv.Required(CONF_VALUE): cv.templatable(cv.float_),
        }
    ),
    synchronous=True,
)
async def set_setpoint_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    mode = await cg.templatable(config[CONF_MODE], args, cg.std_string)
    value = await cg.templatable(config[CONF_VALUE], args, cg.float_)
    cg.add(var.set_mode(mode))
    cg.add(var.set_value(value))
    return var


# Duty point: flow in m³/h, head in m. In proportional pressure Hset is worked out from the
# pump's own curve definition (90/4).
@automation.register_action(
    "alpha2.set_duty_point",
    SetDutyPointAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(Alpha2),
            cv.Required(CONF_MODE): cv.templatable(cv.one_of(*DUTY_POINT_MODES, lower=True)),
            cv.Required(CONF_FLOW): cv.templatable(cv.positive_float),
            cv.Required(CONF_HEAD): cv.templatable(cv.positive_float),
        }
    ),
    synchronous=True,
)
async def set_duty_point_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    mode = await cg.templatable(config[CONF_MODE], args, cg.std_string)
    flow = await cg.templatable(config[CONF_FLOW], args, cg.float_)
    head = await cg.templatable(config[CONF_HEAD], args, cg.float_)
    cg.add(var.set_mode(mode))
    cg.add(var.set_flow(flow))
    cg.add(var.set_head(head))
    return var


# Max / min flow limiter (86/600, 86/601). Without value the stored limit is kept.
@automation.register_action(
    "alpha2.set_flow_limit",
    SetFlowLimitAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(Alpha2),
            cv.Required(CONF_LIMIT): cv.one_of(*FLOW_LIMITS, lower=True),
            cv.Optional(CONF_ENABLED, default=True): cv.templatable(cv.boolean),
            cv.Optional(CONF_VALUE): cv.templatable(cv.positive_float),
        }
    ),
    synchronous=True,
)
async def set_flow_limit_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    cg.add(var.set_limit(config[CONF_LIMIT]))
    enabled = await cg.templatable(config[CONF_ENABLED], args, cg.bool_)
    cg.add(var.set_enabled(enabled))
    if CONF_VALUE in config:
        value = await cg.templatable(config[CONF_VALUE], args, cg.float_)
        cg.add(var.set_value(value))
    return var


# AutoNight (night setback, 90/8)
@automation.register_action(
    "alpha2.set_autonight",
    SetAutonightAction,
    cv.maybe_simple_value(
        {
            cv.GenerateID(): cv.use_id(Alpha2),
            cv.Required(CONF_ENABLED): cv.templatable(cv.boolean),
        },
        key=CONF_ENABLED,
    ),
    synchronous=True,
)
async def set_autonight_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    enabled = await cg.templatable(config[CONF_ENABLED], args, cg.bool_)
    cg.add(var.set_enabled(enabled))
    return var


# Any class 10 SET. The data is not checked, so it
# only does anything when the sensor platform has experimental_class10_write: true.
@automation.register_action(
    "alpha2.write_object",
    WriteObjectAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(Alpha2),
            cv.Required(CONF_OBJECT_ID): cv.uint8_t,
            cv.Required(CONF_SUB_ID): cv.uint16_t,
            cv.Required(CONF_TYPE): cv.uint16_t,
            cv.Required(CONF_VERSION): cv.uint8_t,
            cv.Required(CONF_DATA): cv.All(
                cv.ensure_list(cv.hex_uint8_t), cv.Length(min=1, max=CLASS10_SET_MAX_DATA)
            ),
        }
    ),
    synchronous=True,
)
async def write_object_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    cg.add(
        var.set_object(
            config[CONF_OBJECT_ID], config[CONF_SUB_ID], config[CONF_TYPE], config[CONF_VERSION]
        )
    )
    cg.add(var.set_data(config[CONF_DATA]))
    return var


# Writes a value back unchanged to test the write path without changing the pump.
@automation.register_action(
    "alpha2.write_test",
    WriteTestAction,
    automation.maybe_simple_id(
        {
            cv.GenerateID(): cv.use_id(Alpha2),
            cv.Optional(CONF_TARGET, default="ref_rem"): cv.enum(
                WRITE_TEST_TARGETS, lower=True
            ),
        }
    ),
    synchronous=True,
)
async def write_test_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    cg.add(var.set_target(config[CONF_TARGET]))
    return var
