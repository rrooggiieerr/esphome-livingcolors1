import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import (
    ENTITY_CATEGORY_CONFIG,
    ENTITY_CATEGORY_DIAGNOSTIC as ENTITY_CATEGORY_DIAGNOSTIC,
)
from esphome.core import CORE

from .. import (
    CONF_ADDRESS,
    LIVINGCOLORS1_DEVICE_SCHEMA,
    livingcolors1_ns,
    register_livingcolors1_device,
)

DEPENDENCIES = ["livingcolors1"]
CODEOWNERS = ["@rrooggiieerr"]

CONF_PAIR_ACTION = "pair"
CONF_LEARN_ACTION = "learn"
CONF_UNPAIR_ACTION = "unpair"
# CONF_TEST_ACTION = "test"

LivingColors1ButtonsComponent = livingcolors1_ns.class_(
    "LivingColors1ButtonComponent", cg.Component, button.Button
)
LivingColors1PairButton = livingcolors1_ns.class_(
    "LivingColors1PairButton", cg.Component, button.Button
)
LivingColors1LearnButton = livingcolors1_ns.class_(
    "LivingColors1LearnButton", cg.Component, button.Button
)
LivingColors1UnpairButton = livingcolors1_ns.class_(
    "LivingColors1UnpairButton", cg.Component, button.Button
)
# LivingColors1TestButton = livingcolors1_ns.class_(
#     "LivingColors1TestButton", cg.Component, button.Button
# )


_BASE_SCHEMA = LIVINGCOLORS1_DEVICE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(LivingColors1ButtonsComponent),
        cv.Optional(CONF_ADDRESS): cv.hex_uint64_t,
        cv.Optional(CONF_PAIR_ACTION): button.button_schema(
            LivingColors1PairButton,
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
        cv.Optional(CONF_UNPAIR_ACTION): button.button_schema(
            LivingColors1UnpairButton,
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
        # cv.Optional(CONF_TEST_ACTION): button.button_schema(
        #     LivingColors1TestButton,
        #     entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        # ),
    }
)
_LEARNING_SCHEMA = _BASE_SCHEMA.extend(
    {
        cv.Optional(CONF_LEARN_ACTION): button.button_schema(
            LivingColors1LearnButton,
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
    }
)


def _buttons_schema(config):
    # Learned addresses are stored in lights without address, the ESP8266 has too little storage
    if CORE.is_esp8266:
        return _BASE_SCHEMA(config)
    return _LEARNING_SCHEMA(config)


CONFIG_SCHEMA = cv.All(
    _buttons_schema,
    cv.has_at_least_one_key(CONF_PAIR_ACTION, CONF_LEARN_ACTION, CONF_UNPAIR_ACTION),
)


async def to_code(config):
    if pair_config := config.get(CONF_PAIR_ACTION):
        var = await button.new_button(pair_config)
        await cg.register_component(var, pair_config)
        await register_livingcolors1_device(var, config)
        await button.register_button(var, pair_config)
    if learn_config := config.get(CONF_LEARN_ACTION):
        var = await button.new_button(learn_config)
        await cg.register_component(var, learn_config)
        await register_livingcolors1_device(var, config)
        await button.register_button(var, learn_config)
    if unpair_config := config.get(CONF_UNPAIR_ACTION):
        var = await button.new_button(unpair_config)
        await cg.register_component(var, unpair_config)
        await register_livingcolors1_device(var, config)
        await button.register_button(var, unpair_config)
    # if test_config := config.get(CONF_TEST_ACTION):
    #     var = await button.new_button(test_config)
    #     await cg.register_component(var, test_config)
    #     await register_livingcolors1_device(var, config)
    #     await button.register_button(var, test_config)
