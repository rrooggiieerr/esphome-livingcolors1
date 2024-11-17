import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.components import livingcolors1
from esphome.components.livingcolors1 import LIVINGCOLORS1_DEVICE_SCHEMA
from esphome.const import CONF_ID

from .. import livingcolors1_ns

DEPENDENCIES = ["livingcolors1"]
CODEOWNERS = ["@rrooggiieerr"]

LivingColors1PairButton = livingcolors1_ns.class_("LivingColors1PairButton", cg.Component, button.Button)
# LivingColors1TestButton = livingcolors1_ns.class_("LivingColors1TestButton", cg.Component, button.Button)

CONFIG_SCHEMA = cv.All(button.BUTTON_SCHEMA.extend(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(LivingColors1PairButton),
    }
).extend(LIVINGCOLORS1_DEVICE_SCHEMA).extend(cv.COMPONENT_SCHEMA))


async def to_code(config):
    var = await button.new_button(config)
    await cg.register_component(var, config)
    await livingcolors1.register_livingcolors1_device(var, config)
    await button.register_button(var, config)
