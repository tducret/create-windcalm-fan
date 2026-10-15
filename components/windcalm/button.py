"""Stateless remote commands exposed as native Home Assistant buttons."""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from . import windcalm_ns
from .fan import WindcalmFan, command

DEPENDENCIES = ["windcalm"]
WindcalmButton = windcalm_ns.class_("WindcalmButton", button.Button)
CONFIG_SCHEMA = button.button_schema(WindcalmButton).extend(
    {
        cv.Required("fan_id"): cv.use_id(WindcalmFan),
        cv.Required("command"): command,
    }
)


async def to_code(config):
    var = await button.new_button(config)
    parent = await cg.get_variable(config["fan_id"])
    cg.add(var.set_parent(parent))
    cg.add(var.set_command(config["command"]))
