import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cc2500
from esphome.const import CONF_ID

DEPENDENCIES = ["cc2500"]
CODEOWNERS = ["@rrooggiieerr"]

livingcolors1_ns = cg.esphome_ns.namespace("livingcolors1")
LivingColors1Component = livingcolors1_ns.class_(
    "LivingColors1Component", cc2500.CC2500Device, cg.Component
)

CONF_LIVINGCOLORS1_ID = "livingcolors1_id"
CONF_ADDRESS = "address"
CONF_SEND_REPEATS = "send_repeats"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LivingColors1Component),
        }
    )
    .extend(cc2500.CC2500_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    await cg.register_component(var, config)
    await cc2500.register_cc2500_device(var, config)


LIVINGCOLORS1_DEVICE_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_LIVINGCOLORS1_ID): cv.use_id(LivingColors1Component),
        cv.Required(CONF_ADDRESS): cv.hex_uint64_t,
        cv.Optional(CONF_SEND_REPEATS): cv.positive_int,
    }
)


async def register_livingcolors1_device(var, config):
    parent = await cg.get_variable(config[CONF_LIVINGCOLORS1_ID])
    cg.add(var.set_parent(parent))
    if CONF_ADDRESS in config:
        cg.add(var.set_address(config[CONF_ADDRESS]))
    if CONF_SEND_REPEATS in config:
        cg.add(var.set_send_repeats(config[CONF_SEND_REPEATS]))
