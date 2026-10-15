# Create Windcalm RF

Control Create Windcalm ceiling fans through an ESP32 and CC1101 radio. This
ESPHome component exposes six-speed fans and buttons for lights, timers, and
sound in Home Assistant, and tracks commands from the physical remotes.

## Requirements

- A compatible 433.92 MHz Windcalm fan and its already-paired remote.
- An ESP32-S3 DevKitC, CC1101 433 MHz module, antenna, and USB data cable.
- [ESPHome Desktop / Device Builder](https://desktop.esphome.io) and
  [Git](https://git-scm.com/downloads). Tested with ESPHome **2026.9.1**.
- Wi-Fi and, optionally, Home Assistant with its built-in ESPHome integration.

Default wiring (3.3 V supply and common ground):

| CC1101 | ESP32-S3 GPIO |
| --- | --- |
| SCK | 11 |
| MISO | 9 |
| MOSI | 10 |
| CS | 12 |
| GDO0 — TX | 13 |
| GDO2 — RX | 8 |

## Install through ESPHome Desktop

1. Download [`windcalm.yaml`](https://raw.githubusercontent.com/tducret/create-windcalm-fan/main/examples/windcalm.yaml).
2. In the app, select **Create device → Advanced set up options → Import from file**
   and choose `windcalm.yaml`. ESPHome downloads the component and packages directly from
   **`tducret/create-windcalm-fan`**; no source files need copying.
3. Edit the fan's `fan_name`, `fan_id`, and `remote_id`. The supplied ID is a
   synthetic sample—replace it with your own remote's ID (see below).
4. Review the Wi-Fi and API settings after import. You can modify them in the
   device YAML or edit the referenced `wifi_ssid`, `wifi_password`, and
   `windcalm_api_key` values in `secrets.yaml` as needed.
5. **Validate**, then **Install** over USB. Subsequent
   installs can use Wi-Fi (OTA).
6. In Home Assistant, open **Settings → Devices & services** and configure the
   discovered ESPHome device with the same API key. Fans and buttons appear
   automatically.

The template pins packages and component code to `v0.1.0`. To update, change
`windcalm_version` to an available release and reinstall.

### Find your remote ID

The template enables `logger: {level: DEBUG}` by default. Install it with the
sample ID, open **Logs**, and press a button on your paired remote. Copy the
hexadecimal ID from `Unconfigured remote ID=...` into `remote_id`, prefixed with
`0x`, then reinstall. You can switch logging to INFO afterward for quieter logs.

### Customize

- **More fans:** repeat the `packages/fan.yaml` entry under `files`, giving each
  fan a unique `fan_id` and its own `remote_id`.
- **Different wiring:** override the `windcalm_*_pin` substitutions listed in
  [`packages/radio-cc1101.yaml`](packages/radio-cc1101.yaml).
- **Speed behavior:** set the fan package variable `speed_turns_on: "false"` if
  pressing a speed button does not start your stopped fan.

RF commands have no acknowledgement, so displayed state is an estimate. Timer
expiry is not detected; use the **Mark On/Off** buttons to correct power state.
Firmware builds pass; hardware and live Home Assistant testing remain pending.

## Development

Install `esphome==2026.9.1` in `.venv`, copy `examples/secrets.yaml.example` to
`examples/secrets.yaml`, then run `make test`, `make config`, and `make compile`.
These checks use the local two-fan package example. [MIT license](LICENSE).
