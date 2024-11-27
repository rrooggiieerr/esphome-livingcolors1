import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import ENTITY_CATEGORY_CONFIG, ENTITY_CATEGORY_DIAGNOSTIC

from .. import (
    LIVINGCOLORS1_DEVICE_SCHEMA,
    livingcolors1_ns,
    register_livingcolors1_device,
)

DEPENDENCIES = ["livingcolors1"]
CODEOWNERS = ["@rrooggiieerr"]

CONF_PAIR_ACTION = "pair"
CONF_UNPAIR_ACTION = "unpair"
# CONF_TEST_ACTION = "test"

LivingColors1PairButton = livingcolors1_ns.class_(
    "LivingColors1PairButton", cg.Component, button.Button
)
LivingColors1UnpairButton = livingcolors1_ns.class_(
    "LivingColors1UnpairButton", cg.Component, button.Button
)
# LivingColors1TestButton = livingcolors1_ns.class_(
#     "LivingColors1TestButton", cg.Component, button.Button
# )

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
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
    ).extend(LIVINGCOLORS1_DEVICE_SCHEMA),
    cv.has_at_least_one_key(CONF_PAIR_ACTION, CONF_UNPAIR_ACTION),
)


async def to_code(config):
    if pair_config := config.get(CONF_PAIR_ACTION):
        var = await button.new_button(pair_config)
        await cg.register_component(var, pair_config)
        await register_livingcolors1_device(var, config)
        await button.register_button(var, pair_config)
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
