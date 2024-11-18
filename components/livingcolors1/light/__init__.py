import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_LIGHT_ID

from .. import (
    LIVINGCOLORS1_DEVICE_SCHEMA,
    livingcolors1_ns,
    register_livingcolors1_device,
)

DEPENDENCIES = ["livingcolors1"]
CODEOWNERS = ["@rrooggiieerr"]

LivingColors1Light = livingcolors1_ns.class_(
    "LivingColors1Light", cg.Component, light.LightOutput
)

CONFIG_SCHEMA = cv.All(
    light.LIGHT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_LIGHT_ID): cv.declare_id(LivingColors1Light),
        }
    )
    .extend(LIVINGCOLORS1_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_LIGHT_ID])
    await cg.register_component(var, config)
    await register_livingcolors1_device(var, config)
    await light.register_light(var, config)
