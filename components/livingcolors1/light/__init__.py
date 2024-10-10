import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_OUTPUT_ID

from .. import livingcolors1_ns, CONF_LIVINGCOLORS1_ID, LivingColors1Component

DEPENDENCIES = ["livingcolors1"]
CODEOWNERS = ["@rrooggiieerr"]

CONF_ADDRESS = "address"

LivingColors1Light = livingcolors1_ns.class_("LivingColors1Light", cg.Component, light.LightOutput)

CONFIG_SCHEMA = cv.All(light.LIGHT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(LivingColors1Light),
        cv.GenerateID(CONF_LIVINGCOLORS1_ID): cv.use_id(LivingColors1Component),
        cv.Required(CONF_ADDRESS): cv.hex_uint64_t,
    }
).extend(cv.COMPONENT_SCHEMA))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await light.register_light(var, config)

    cg.add(var.set_address(config[CONF_ADDRESS]))

    parent = await cg.get_variable(config[CONF_LIVINGCOLORS1_ID])
    cg.add(var.set_parent(parent))
