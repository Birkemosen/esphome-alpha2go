# ESPHome Grundfos Alpha2 Go

ESPHome component for reading sensor data from **Grundfos Alpha2 Go** circulation pumps via Bluetooth Low Energy (BLE) using an ESP32-C3 Super Mini.

Reads the following values from the pump:
- **Flow** (m³/h)
- **Head pressure** (m)
- **Power** (W)
- **Voltage** (V)
- **Current** (A)
- **Speed** (RPM)

## Credits

This project is based on work by:

- **[parameter-pollution/esphome-grundfos-alpha2-go](https://github.com/parameter-pollution/esphome-grundfos-alpha2-go)** — Alpha2 Go component with adjusted GENI request/response bytes discovered via Android HCI logging + Wireshark.
- **[esphome/esphome — alpha3 component](https://github.com/esphome/esphome/tree/dev/esphome/components/alpha3)** — The original ESPHome Alpha3 component that the Alpha2 variant is derived from.

## Hardware

- **ESP32-C3 Super Mini** (or any ESP32 board with BLE support)
- **Grundfos Alpha2 Go** circulation pump

## Setup

### 1. Clone the repository

```bash
git clone git@github.com:birkemosen/esphome-alpha2go.git
cd esphome-alpha2go
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

## Notes

- The pump polls every **15 seconds** by default (configurable via `update_interval` in the YAML).
- For first-time BLE pairing, press the "wifi" button on the pump.
- The ESP-IDF framework is used for stable BLE operation on the ESP32-C3.

## License

This project uses code originally licensed under [GPL-3.0](https://github.com/parameter-pollution/esphome-grundfos-alpha2-go/blob/main/LICENSE).
