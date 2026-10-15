"""Six-speed fan with optional physical-remote reception."""

from esphome import automation
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import fan
from esphome.const import CONF_ID

from . import WindcalmHub, windcalm_ns

DEPENDENCIES = ["windcalm"]
WindcalmFan = windcalm_ns.class_("WindcalmFan", cg.Component, fan.Fan)
SendAction = windcalm_ns.class_("SendAction", automation.Action)
SyncAction = windcalm_ns.class_("SyncAction", automation.Action)

COMMANDS = {
    "SPEED_1": 0x04,
    "SPEED_2": 0x10,
    "SPEED_3": 0x0C,
    "SPEED_4": 0x06,
    "SPEED_5": 0x09,
    "SPEED_6": 0x15,
    "POWER_TOGGLE": 0x0A,
    "TIMER_1H": 0x16,
    "TIMER_2H": 0x0F,
    "TIMER_4H": 0x02,
    "DISABLE_SOUND": 0x08,
    "ROTATION_TOGGLE": 0x11,
    "LIGHT_TOGGLE": 0x12,
    "LIGHT_TEMPERATURE": 0x1C,
}
command = cv.Any(cv.enum(COMMANDS, upper=True), cv.int_range(min=0, max=31))

CONFIG_SCHEMA = (
    fan.fan_schema(WindcalmFan)
    .extend(
        {
            cv.Required("windcalm_id"): cv.use_id(WindcalmHub),
            cv.Required("remote_id"): cv.hex_int_range(min=0, max=0xFFFFF),
            cv.Optional("speed_turns_on", default=True): cv.boolean,
            cv.Optional("restore_mode", default="RESTORE_DEFAULT_OFF"): cv.enum(
                {key: fan.RESTORE_MODES[key] for key in ("RESTORE_DEFAULT_OFF", "NO_RESTORE")},
                upper=True,
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await fan.new_fan(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config["windcalm_id"])
    cg.add(var.set_parent(parent))
    cg.add(var.set_remote_id(config["remote_id"]))
    cg.add(var.set_speed_turns_on(config["speed_turns_on"]))
    cg.add(parent.register_fan(var))


@automation.register_action(
    "windcalm.send",
    SendAction,
    cv.Schema(
        {
            cv.Required("id"): cv.use_id(WindcalmFan),
            cv.Required("command"): cv.templatable(command),
        }
    ),
    synchronous=True,
)
async def send_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config["command"], args, cg.int_)
    cg.add(var.set_command(value))
    return var


@automation.register_action(
    "windcalm.sync_state",
    SyncAction,
    cv.Schema(
        {
            cv.Required("id"): cv.use_id(WindcalmFan),
            cv.Required("state"): cv.templatable(cv.boolean),
        }
    ),
    synchronous=True,
)
async def sync_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config["state"], args, cg.bool_)
    cg.add(var.set_state(value))
    return var
