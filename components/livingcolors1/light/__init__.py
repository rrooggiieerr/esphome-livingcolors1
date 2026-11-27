import esphome.codegen as cg
from esphome.components import light
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_LIGHT_ID
from esphome.core import CORE
from esphome.helpers import fnv1_hash

from .. import (
    CONF_ADDRESS,
    LIVINGCOLORS1_DEVICE_SCHEMA,
    livingcolors1_ns,
    register_livingcolors1_device,
)

DEPENDENCIES = ["livingcolors1"]
CODEOWNERS = ["@rrooggiieerr"]

LivingColors1Light = livingcolors1_ns.class_(
    "LivingColors1Light", cg.Component, light.LightOutput
)
LivingColors1LightState = livingcolors1_ns.class_(
    "LivingColors1LightState", light.LightState
)


_BASE_SCHEMA = (
    light.LIGHT_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(LivingColors1LightState),
            cv.GenerateID(CONF_LIGHT_ID): cv.declare_id(LivingColors1Light),
        }
    )
    .extend(LIVINGCOLORS1_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


# Without address the light uses the address stored when pairing or learning
_STORING_SCHEMA = _BASE_SCHEMA.extend({cv.Optional(CONF_ADDRESS): cv.hex_uint64_t})


def _light_schema(config):
    # The ESP8266 has too little preference storage for lights without address
    if CORE.is_esp8266:
        return _BASE_SCHEMA(config)
    return _STORING_SCHEMA(config)


def _validate_id(config):
    # The stored address is keyed by the id, a generated one changes when lights are added
    if CONF_ADDRESS not in config and not config[CONF_ID].is_manual:
        raise cv.Invalid("id is required for a light without address")
    return config


CONFIG_SCHEMA = cv.All(_light_schema, _validate_id)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_LIGHT_ID])
    await cg.register_component(var, config)
    await register_livingcolors1_device(var, config)
    if CONF_ADDRESS not in config:
        cg.add(var.set_address_key(fnv1_hash(str(config[CONF_ID]))))
    await light.register_light(var, config)
