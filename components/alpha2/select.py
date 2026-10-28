import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.types import ConfigType

from . import CONF_ALPHA2_ID, SETTINGS, Alpha2, alpha2_ns

CONF_SETTING = "setting"

Alpha2SettingSelect = alpha2_ns.class_(
    "Alpha2SettingSelect", select.Select, cg.Parented.template(Alpha2)
)

# The settings alpha2.set_setting accepts, plus "advanced" for any other set point or control
# mode. "advanced" is only shown; it cannot be selected.
SETTING_OPTIONS = [*SETTINGS, "advanced"]

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ALPHA2_ID): cv.use_id(Alpha2),
        cv.Optional(CONF_SETTING): select.select_schema(Alpha2SettingSelect, icon="mdi:tune"),
    }
)


async def to_code(config: ConfigType) -> None:
    parent = await cg.get_variable(config[CONF_ALPHA2_ID])
    if conf := config.get(CONF_SETTING):
        sel = await select.new_select(conf, options=SETTING_OPTIONS)
        await cg.register_parented(sel, parent)
        cg.add(parent.set_setting_select(sel))
