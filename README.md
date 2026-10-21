# ESPHome Grundfos Alpha2 Go

ESPHome component for reading sensor data from, and sending commands to, **Grundfos Alpha2 Go** circulation pumps via Bluetooth Low Energy (BLE) using an ESP32-C3 Super Mini. It talks GENIbus over the pump's BLE GENI characteristic.

> ⚠️ **Writing to the pump changes how your heating system runs.** The write actions are new and have only been checked against the protocol, not tested on every pump. Watch the ESPHome log the first time you use one: every write logs the bytes sent and the pump's ACK. The experimental `alpha2.write_object` is off by default; leave it off unless you are capturing and comparing HCI snoop logs.

## Sensors

All sensors are optional. Only the objects needed by the configured sensors are polled.

| Key | Unit | Source object |
| --- | --- | --- |
| `flow` | m³/h | 93/290 offset 24 |
| `head` | m | 93/290 offset 28 |
| `media_temperature` | °C | 93/290 offset 16 |
| `differential_temperature` | K | 93/290 offset 20 |
| `flow_coefficient_kv` | – (unit not confirmed) | 93/290 offset 32 |
| `power` | W | 87/69 offset 12 |
| `current` | A | 87/69 offset 8 |
| `voltage` / `grid_voltage` | V | 87/69 offset 0 (same value) |
| `dc_link_voltage` | V | 87/69 offset 4 |
| `speed` / `absolute_speed` | RPM | 87/69 offset 20 (same value) |
| `converter_temperature` | °C | 87/69 offset 24 |
| `inverter_temperature` | °C | 87/69 offset 28 |
| `setpoint` | – (Pa in pressure modes, see below) | 86/8 offset 3 |
| `setpoint_head` | m | 86/8 offset 3 ÷ 9804, only in pressure control modes |

Text sensors (`text_sensor` platform `alpha2`, with `alpha2_id` pointing at the sensor platform `id`), read from 86/8, which is what the pump actually runs:

- `control_mode` – Constant pressure, Proportional pressure, Constant speed, AUTOadapt, Proportional pressure AUTOADAPT (13), Constant pressure AUTOADAPT (14), Constant temperature, Constant flow, FlowAdapt
- `operation_mode` – Auto, Stop, Min, Max, User defined
- `control_source` – Panel, Bus

The realized `setpoint` is in Pa in the pressure control modes. On an ALPHA2 GO, constant pressure settings I/II/III gave exactly 24510, 49020 and 73530 Pa, i.e. 2.5, 5.0 and 7.5 m at the pump's 9804 Pa/m. `setpoint_head` shows it in m, so in constant pressure the panel setting can be read from it. 86/8 has no field for the setting itself, and in proportional pressure the realized set point moves with the flow. Temperatures read `nan` on the ALPHA2 GO, which apparently does not measure them.

`flow` and `head` now use the m³/h and m values the pump reports directly. Earlier versions converted the SI values with approximate factors, so `head` may read about 2 % higher than before.

## Actions

The sensor platform needs an `id` for these (`id: pump` in `alpha2go.yaml`).

| Action | What it sends |
| --- | --- |
| `alpha2.start`, `alpha2.stop` | class 3 command START (6) / STOP (5) |
| `alpha2.set_min`, `alpha2.set_max` | class 3 command MIN (25) / MAX (26) |
| `alpha2.remote`, `alpha2.local` | class 3 command REMOTE (7) / LOCAL (8) |
| `alpha2.set_control_mode` | class 3 command; `mode`: `const_pressure`, `prop_pressure`, `autoadapt`, `const_temp`, `const_flow`, `flow_adapt`. There is no confirmed command back to Constant pressure AUTOADAPT (14), so switching away from it may only be undone on the panel or in the Grundfos GO app. On the ALPHA2 GO, LED 4 on the panel lights up when the pump has been set via Grundfos GO; the panel's control mode LEDs are then off |
| `alpha2.set_setpoint_rem` | class 5 `ref_rem` (id 1); `value` 0–100 % of the active set point range, sent as 0–254 |
| `alpha2.write_object` | **experimental** class 10 SET; needs `experimental_class10_write: true` |

```yaml
- alpha2.set_setpoint_rem:
    id: pump
    value: 60
- alpha2.set_control_mode:
    id: pump
    mode: autoadapt
```

`alpha2.set_setpoint_rem` is the recommended way to change the set point. The pump may only follow the remote set point after `alpha2.remote`.

### Safe write test

`alpha2.write_test` tests the write path without changing anything on the pump. It reads the current state, sends a write that should leave it unchanged, reads again and logs `write test ... PASSED` or `FAILED` with the reason. Nothing is written if the first read fails or the precondition is not met.

| `target` | Write | Precondition | If the state changed anyway |
| --- | --- | --- | --- |
| `ref_rem` (default) | class 5 `ref_rem` = current value | `ref_rem` is not 255 (only set in remote mode) | original value written back |
| `start` | class 3 START | 86/8 shows operation mode Auto | LOCAL sent if the control source changed |
| `autonight` | **UNVERIFIED** class 10 SET of 90/8 = current value; needs `experimental_class10_write: true` | – | original value written back |

```yaml
- alpha2.write_test: pump
- alpha2.write_test:
    id: pump
    target: start
```

When the pump is controlled from its panel, `ref_rem` reads 255 and the default test stops without writing. Use `target: start` instead.

`alpha2go.yaml` has a "Pump Write Test" button for this. Start with `ref_rem`. Run `autonight` only to check the class 10 SET layout, ideally while capturing an HCI snoop log.

How writes are handled:

- Every frame gets its CRC computed and is checked again before it is sent. If the built-in self test of the frame builder fails at boot, all requests are disabled.
- All requests, polls included, are queued and sent one at a time from `loop()`. The next one goes out only after the previous reply arrived or timed out (1 s), so a reply always belongs to the request before it. The pump drops requests that arrive while it is still answering.
- The log shows the request bytes and the ACK (`OK`, `unknown class`, `unknown id`, `illegal operation`), or a warning if no ACK arrives within 1 s.
- After an ACK of `OK` the pump is polled again so the sensors show the new state.

## Verified vs experimental

| Feature | Status |
| --- | --- |
| Frame format, CRC-16/CCITT | Verified: the frame builder reproduces the known request frames byte for byte (self test at boot) |
| Class 10 GET 93/290, 87/69 | Verified on hardware (the existing requests) |
| Class 10 GET 86/8 | Verified on an ALPHA2 GO. Control modes 0, 1, 2, 13 and 14 seen while switching on the panel; 13 = Proportional pressure AUTOADAPT, 14 = Constant pressure AUTOADAPT. Set point in Pa confirmed for constant pressure I/II/III |
| Offsets in 93/290 and 87/69 | From the Grundfos profiles |
| Temperature units | **Unverified**: values above 200 are treated as Kelvin and converted to °C |
| Reply CRC check | Verified on an ALPHA2 GO: replies use the same CRC |
| Class 3 commands | Verified in the Grundfos GO app and the Ontrol driver. START answered with ACK OK on an ALPHA2 GO in panel mode (`write_test target: start` PASSED). Commands that change the state are not tested on hardware yet |
| Class 5 `ref_rem` | Verified in the Grundfos GO app and the Ontrol driver |
| Classic GET (used by `alpha2.write_test`) | Verified on an ALPHA2 GO (class 5 `ref_rem` read) |
| Class 10 SET (`alpha2.write_object`) | **Experimental**: layout guessed from the GET reply, still needs confirmation with an HCI snoop log. Off by default |
| ALPHA3 | Not tested with this component |

## Credits

This project is based on work by:

- **[parameter-pollution/esphome-grundfos-alpha2-go](https://github.com/parameter-pollution/esphome-grundfos-alpha2-go)** — Alpha2 Go component with adjusted GENI request/response bytes discovered via Android HCI logging + Wireshark.
- **[esphome/esphome — alpha3 component](https://github.com/esphome/esphome/tree/dev/esphome/components/alpha3)** — The original ESPHome Alpha3 component that the Alpha2 variant is derived from.

GENI frame format, object layouts and command ids were checked against the Grundfos GO app and the Ontrol GENI driver.

## Hardware

- **ESP32-C3 Super Mini** (or any ESP32 board with BLE support)
- **Grundfos Alpha2 Go** circulation pump

Requires **ESPHome 2026.9.0** or newer.

## Setup

### 1. Pull the component from GitHub

In your ESPHome device configuration, load `alpha2` as an external component. ESPHome clones this repository and reads `components/alpha2`:

```yaml
external_components:
  - source: github://birkemosen/esphome-alpha2go
    components: [alpha2]
```

`alpha2go.yaml` in this repository is a complete device example that already uses that source.

To compile against a local checkout instead of GitHub, point `source` at the `components` folder next to your YAML:

```yaml
external_components:
  - source:
      type: local
      path: components
    components: [alpha2]
```

### 2. Configure secrets

Edit `secrets.yaml` with your credentials:

```yaml
wifi_ssid: "YOUR_WIFI_SSID"
wifi_password: "YOUR_WIFI_PASSWORD"
api_encryption_key: "GENERATE_A_BASE64_KEY_HERE"
ota_password: "YOUR_OTA_PASSWORD"
ap_password: "alpha2go-fallback"
```

Generate an API encryption key:

```bash
python3 -c "import secrets, base64; print(base64.b64encode(secrets.token_bytes(32)).decode())"
```

### 3. Find your pump's Bluetooth MAC address

Use an app like [nRF Connect](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-mobile) to scan for BLE devices. You may need to press the "wifi" button on the pump to put it into pairing mode.

### 4. Update configuration

Replace `XX:XX:XX:XX:XX:XX` in `alpha2go.yaml` with your pump's Bluetooth MAC address.

### 5. Flash the ESP32-C3

```bash
esphome run alpha2go.yaml
```

For the first flash, connect the ESP32-C3 via USB. Subsequent updates can be done over-the-air (OTA).

## Usage with Home Assistant

Once flashed and connected, the sensor entities will automatically appear in Home Assistant via the ESPHome integration.

`alpha2go.yaml` also adds a set point slider (`number`), a control mode `select` and start/stop/min/max/remote/local buttons that call the actions above. Remove them if you only want to read values.

## Notes

- The pump polls every **15 seconds** by default (configurable via `update_interval` in the YAML).
- For first-time BLE pairing, press the "wifi" button on the pump.
- The ESP-IDF framework is used for stable BLE operation on the ESP32-C3.

## License

This project uses code originally licensed under [GPL-3.0](https://github.com/parameter-pollution/esphome-grundfos-alpha2-go/blob/main/LICENSE).
