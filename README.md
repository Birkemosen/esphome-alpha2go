# ESPHome Grundfos Alpha2 Go

ESPHome component that reads and controls **Grundfos ALPHA2 GO** circulation pumps over Bluetooth Low Energy, using an ESP32 (tested on an ESP32-C3 Super Mini).

> ⚠️ **Writing to the pump changes how your heating system runs.** Every write logs the bytes sent and the pump's ACK; check the log the first time you use one.

> **No warranty.** This is an unofficial component, not made, endorsed or supported by Grundfos. It is based on observing the pump's BLE traffic, not on official documentation, and has been tested on one ALPHA2 GO 25-90. It is provided as is, without warranty of any kind (see sections 15 and 16 of the [GPL-3.0](LICENSE)). You use it at your own risk: the authors are not liable for damage to the pump, your heating system or anything else, and changing pump settings this way may affect the manufacturer's warranty.

## What it can do

| What | Read | Write | Entity / action |
| --- | :---: | :---: | --- |
| Flow, head, power, current, speed, voltages | ✓ | – | sensors |
| Media / electronics temperatures (unit not confirmed) | ✓ | – | sensors |
| Control mode, operation mode, control source | ✓ | – | text sensors |
| Setting: CP / PP / CC I–III, CP / PP AUTOADAPT | ✓ | ✓ | `setting` select, `alpha2.set_setting` |
| AUTOADAPT on | ✓ | via setting | `autoadapt` binary sensor |
| Set point: constant pressure, proportional pressure (Hset), constant curve (RPM), constant flow | ✓ | ✓ | `*_setpoint` sensors, `alpha2.set_setpoint` |
| Duty point (flow and head) | – | ✓ | `alpha2.set_duty_point` |
| Max / min flow limit | ✓ | ✓ | `*_flow_limit` sensors and binary sensors, `alpha2.set_flow_limit` |
| AutoNight (night setback) | ✓ | ✓ | `autonight` binary sensor, `alpha2.set_autonight` |
| Start, stop, min / max curve | – | ✓ | `alpha2.start`, `stop`, `set_min`, `set_max` |

All writes above were tested from ESPHome on an ALPHA2 GO 25-90. Not possible over BLE: clearing the panel's Advanced icon (it lights after any external change and goes off when a setting is chosen on the panel), and a remote set point (`ref_rem`, which the ALPHA2 GO does not support).

## Configuration

`alpha2go.yaml` is a complete example with every entity. The short version:

```yaml
external_components:
  - source: github://birkemosen/esphome-alpha2go
    components: [alpha2]

ble_client:
  - mac_address: XX:XX:XX:XX:XX:XX
    id: heatingpump

sensor:
  - platform: alpha2
    id: pump
    ble_client_id: heatingpump
    flow:
      name: "Pump Flow"
    head:
      name: "Pump Head"
    power:
      name: "Pump Power"

select:
  - platform: alpha2
    alpha2_id: pump
    setting:
      name: "Pump Setting"
```

### Sensors (`sensor: platform: alpha2`)

All optional; only the objects needed by the configured sensors are polled (default every 15 s).

| Key | Unit | Source |
| --- | --- | --- |
| `flow`, `head` | m³/h, m | 93/290 |
| `media_temperature`, `differential_temperature`, `flow_coefficient_kv` | °C, K, – | 93/290 |
| `power`, `current`, `speed`, `absolute_speed`, `voltage`, `grid_voltage`, `dc_link_voltage` | W, A, RPM, V | 87/69 |
| `converter_temperature`, `inverter_temperature` | °C | 87/69 |
| `setpoint` (raw), `setpoint_head` (pressure modes), `setpoint_speed` (constant curve) | –, m, RPM | 86/8, what the pump runs now |
| `const_pressure_setpoint`, `prop_pressure_setpoint` | m | 86/16, 86/18 |
| `const_speed_setpoint`, `const_flow_setpoint` | RPM, m³/h | 86/14, 86/40 |
| `autoadapt_radiator_setpoint`, `autoadapt_underfloor_setpoint` (unverified) | m | 86/20, 86/22 |
| `max_flow_limit`, `min_flow_limit` | m³/h | 86/600, 86/601 |

`experimental_class10_write: true` enables `alpha2.write_object` (any class 10 SET with data you give). Leave it off unless you know what you are writing.

### Text sensors, binary sensors and select

- `text_sensor`: `control_mode`, `operation_mode`, `control_source`, `setting` (I / II / III / AUTOADAPT / Advanced) and `selected_setting` (the last setting chosen on the panel, 86/350; shows Advanced when the pump runs something else).
- `binary_sensor`: `autoadapt`, `autonight`, `max_flow_limit_enabled`, `min_flow_limit_enabled`.
- `select`: `setting` shows what the pump runs (`const_pressure_1`–`3`, `prop_pressure_1`–`3`, `const_speed_1`–`3`, `const_pressure_autoadapt`, `prop_pressure_autoadapt`, or `advanced` for any other set point) and calls `alpha2.set_setting` when changed.

The I / II / III levels are read from the pump after connecting (86/351–359). On an ALPHA2 GO they are CC 5225 / 5877 / 6350 RPM, CP 2.5 / 5.0 / 7.5 m and PP 4.0 / 6.0 / 8.0 m.

### Actions

| Action | Parameters |
| --- | --- |
| `alpha2.set_setting` | `value`: one of the select options above |
| `alpha2.set_setpoint` | `mode`: `const_pressure` / `prop_pressure` (m), `const_speed` (RPM), `const_flow` (m³/h); `value` |
| `alpha2.set_duty_point` | `mode`: `const_pressure` or `prop_pressure`; `flow` (m³/h), `head` (m) |
| `alpha2.set_flow_limit` | `limit`: `max` or `min`; `enabled` (default true); optional `value` (m³/h). Without `value` the stored limit is kept; with `enabled: false` a new limit is stored while the limiter stays off |
| `alpha2.set_autonight` | `enabled` |
| `alpha2.start`, `stop`, `set_min`, `set_max`, `remote`, `local` | class 3 commands |
| `alpha2.set_control_mode` | class 3 command: changes what the pump runs but not the panel setting |
| `alpha2.write_test` | `target: start`: sends START only if the pump already runs, and checks nothing changed |
| `alpha2.write_object` | experimental, see above |

```yaml
- alpha2.set_setting:
    id: pump
    value: prop_pressure_2
- alpha2.set_duty_point:
    id: pump
    mode: prop_pressure
    flow: 1.02
    head: 5.0
```

## How writes work

- A setting or set point is written as the control mode (86/10), then the user config of that mode with only the set point changed. Values are first checked against the min / max the pump reports for that mode; nothing is written if a read fails or a value is out of range. Each step waits for the pump's ACK, and the log shows `setting ... done` or `FAILED` with the reason.
- Proportional pressure is a straight line from Hset / 2 at no flow (never below 1 m) to Hset where it meets the max curve. The pump stores the shape of that line (90/4); `alpha2.set_duty_point` uses it to find the Hset whose curve goes through the given point, rounded to 0.1 m.
- All requests, polls included, go through a queue and are sent one at a time from `loop()`. Frames are written in pieces of at most 20 bytes; the pump ignores longer frames sent in one write. Every frame's CRC is checked before sending, and a self test of the frame builder at boot disables all requests if it fails.
- The control source stays "Local (panel or app)" whoever changes the setting: the panel, Grundfos GO and these actions all use the same local request.

[docs/ble-traffic.md](docs/ble-traffic.md) lists the observed frames and object values.

## Home Assistant

The entities appear through the ESPHome integration. [`ha_dashboard.yaml`](ha_dashboard.yaml) is a dashboard card for `alpha2go.yaml` that only shows the controls for the control mode the pump runs, plus a duty point card and the options (AutoNight, flow limits). Paste it via "Add card" → "Manual" and adjust the entity ids if your device has another `friendly_name`.

## Setup

1. Hardware: an ESP32 board with BLE (ESP32-C3 Super Mini tested), ESPHome 2026.9.0 or newer, ESP-IDF framework.
2. Put your Wi-Fi credentials, `api_encryption_key` and `ota_password` in `secrets.yaml`.
3. Find the pump's Bluetooth MAC address with a BLE scanner such as [nRF Connect](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-mobile); press the pump's Bluetooth button if it does not show up. Put it in `alpha2go.yaml`.
4. Flash with `esphome run alpha2go.yaml` (USB the first time, OTA after that).

The pump accepts one BLE connection at a time, so close Grundfos GO while the ESP is connected.

## Credits

- [parameter-pollution/esphome-grundfos-alpha2-go](https://github.com/parameter-pollution/esphome-grundfos-alpha2-go): the ALPHA2 GO component this is based on.
- [ESPHome Grundfos Alpha3 sensor](https://esphome.io/components/sensor/alpha3) ([source](https://github.com/esphome/esphome/tree/dev/esphome/components/alpha3)): the original ESPHome component it is derived from.

## License

[GPL-3.0](LICENSE), without any warranty; see the note at the top.
