import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_SENSOR_ID,
    CONF_SIGNAL_STRENGTH,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_DECIBEL_MILLIWATT,
)

from .. import (
    LIVINGCOLORS1_DEVICE_SCHEMA,
    livingcolors1_ns,
    register_livingcolors1_device,
)

DEPENDENCIES = ["livingcolors1"]
CODEOWNERS = ["@rrooggiieerr"]

CONF_RSSI_VALUE = "rssi"
CONF_LQI_VALUE = "lqi"

LivingColors1RSSISensor = livingcolors1_ns.class_(
    "LivingColors1RSSISensor", cg.Component, sensor.Sensor
)
LivingColors1LQISensor = livingcolors1_ns.class_(
    "LivingColors1LQISensor", cg.Component, sensor.Sensor
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.Optional(CONF_RSSI_VALUE): sensor.sensor_schema(
                LivingColors1RSSISensor,
                unit_of_measurement=UNIT_DECIBEL_MILLIWATT,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                accuracy_decimals=1,
            ),
            cv.Optional(CONF_LQI_VALUE): sensor.sensor_schema(
                LivingColors1LQISensor,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                accuracy_decimals=0,
            ),
        },
    ).extend(LIVINGCOLORS1_DEVICE_SCHEMA),
    cv.has_at_least_one_key(CONF_RSSI_VALUE, CONF_LQI_VALUE),
)


async def to_code(config):
    if rssi_config := config.get(CONF_RSSI_VALUE):
        var = await sensor.new_sensor(rssi_config)
        await cg.register_component(var, rssi_config)
        await register_livingcolors1_device(var, config)
        await sensor.register_sensor(var, rssi_config)
    if lqi_config := config.get(CONF_LQI_VALUE):
        var = await sensor.new_sensor(lqi_config)
        await cg.register_component(var, lqi_config)
        await register_livingcolors1_device(var, config)
        await sensor.register_sensor(var, lqi_config)
