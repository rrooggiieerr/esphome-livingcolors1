import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.components.cc2500 import CC2500Device, CC2500_DEVICE_SCHEMA

DEPENDENCIES = ["cc2500"]
CODEOWNERS = ["@rrooggiieerr"]

livingcolors1_ns = cg.esphome_ns.namespace("livingcolors1")
LivingColors1Component = livingcolors1_ns.class_("LivingColors1Component", CC2500Device, cg.Component)

CONF_LIVINGCOLORS1_ID = 'livingcolors1_id'
# CONF_COMMAND_REPEATS = 'command_repeats'

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(LivingColors1Component),
    }
).extend(CC2500_DEVICE_SCHEMA).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    # if CONF_COMMAND_REPEATS in config:
    #     cg.add(var.set_command_repeats(config[CONF_COMMAND_REPEATS]))
    
    await cg.register_component(var, config)
    await cc2500.register_cc2500_device(var, config)
