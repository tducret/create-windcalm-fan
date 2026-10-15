"""Create Windcalm RF controller."""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import remote_base
from esphome.const import CONF_ID

DEPENDENCIES = ["remote_transmitter"]
AUTO_LOAD = ["remote_base", "fan"]
MULTI_CONF = True

windcalm_ns = cg.esphome_ns.namespace("windcalm")
WindcalmHub = windcalm_ns.class_(
    "WindcalmHub", cg.Component, remote_base.RemoteReceiverListener
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(WindcalmHub),
        cv.Required("transmitter_id"): cv.use_id(remote_base.RemoteTransmitterBase),
        cv.Optional("receiver_id"): cv.use_id(remote_base.RemoteReceiverBase),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await remote_base.register_transmittable(var, config)
    if "receiver_id" in config:
        await remote_base.register_listener(var, config)
