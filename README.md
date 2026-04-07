# Esplora: LoRa lab for XIAO ESP32S3 + Wio-SX1262

This project flashes a fresh firmware onto the Seeed XIAO ESP32S3 + Wio-SX1262 kit and turns it into a small LoRa lab node:

- serial CLI on `COM5`
- web panel on `http://192.168.1.201`
- LoRa chat packets
- periodic beacons for nearby node discovery
- ping/pong over LoRa
- neighbor list with RSSI, SNR and rough distance estimate
- Wi-Fi helper with fixed target IP `192.168.1.201`
- MQTT bridge for Home Assistant

## What is configured now

- Board: `Seeed Studio XIAO ESP32S3`
- Radio autodetect: first `b2b`, then `header`
- Working radio mapping on this unit: `b2b`
- Default LoRa profile: `balanced`
- Frequency: `868.100 MHz`
- Bandwidth: `125 kHz`
- Spreading factor: `SF9`
- Coding rate: `4/7` (`cr=7` in RadioLib)
- TX power: `14 dBm`
- Fixed Wi-Fi IP target: `192.168.1.201/24`
- Default gateway assumption: `192.168.1.1`
- Default MQTT base topic: `esplora/<NODE_ID>`

## Flash

```powershell
cd C:\Users\Biuro\Documents\Playground\wio-sx1262-lab
pio run
esptool --chip esp32s3 --port COM5 --baud 460800 --before default-reset --after hard-reset write-flash -z --flash-mode dio --flash-freq 80m --flash-size detect 0x0000 .pio\build\seeed_xiao_esp32s3\bootloader.bin 0x8000 .pio\build\seeed_xiao_esp32s3\partitions.bin 0xe000 C:\Users\Biuro\.platformio\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin 0x10000 .pio\build\seeed_xiao_esp32s3\firmware.bin
```

`pio run -t upload` also works, but in this Windows environment it occasionally leaves a hanging `esptool` process. Direct `esptool` is more predictable here.

## PC console

Install the one dependency:

```powershell
pip install -r tools\requirements.txt
```

Interactive console:

```powershell
python tools\esplora_console.py --port COM5
```

One-shot examples:

```powershell
python tools\esplora_console.py --port COM5 --command status --command "wifi status"
python tools\esplora_console.py --port COM5 --command "send test eteru"
python tools\esplora_console.py --port COM5 --command ping
```

## Web panel

When Wi-Fi is connected, open:

```text
http://192.168.1.201
```

The panel provides:

- live node status
- LoRa chat send box
- scanner for nearby compatible nodes
- live event tape
- radio profile switching
- beacon/raw log toggles
- Wi-Fi credential form
- MQTT / Home Assistant broker form

The panel talks directly to the node. No extra backend is needed.

## Serial commands on the device

- `status`
- `neighbors`
- `send <tekst>`
- `ping`
- `cad`
- `beacon on`
- `beacon off`
- `beacon now`
- `beacon <sekundy>`
- `profile fast|balanced|long`
- `set freq <MHz>`
- `set bw <kHz>`
- `set sf <7-12>`
- `set cr <5-8>`
- `set power <dBm>`
- `set name <nazwa>`
- `raw on`
- `raw off`
- `restart radio`

Wi-Fi commands:

- `wifi status`
- `wifi ssid <twoja_siec>`
- `wifi pass <twoje_haslo>`
- `wifi connect`
- `wifi off`
- `mqtt status`
- `mqtt host <host>`
- `mqtt port <port>`
- `mqtt user <user>`
- `mqtt pass <haslo>`
- `mqtt topic <base/topic>`
- `mqtt on`
- `mqtt off`
- `mqtt toggle`
- `mqtt connect`
- `mqtt ha on`
- `mqtt ha off`
- `mqtt ha toggle`

SSID and password are saved in NVS. After you set them once, the node will try to come up on `192.168.1.201` after reboot.

## MQTT / Home Assistant

The node can publish telemetry to any MQTT broker and expose Home Assistant discovery topics.

Minimum setup from serial:

```text
mqtt host 192.168.1.10
mqtt port 1883
mqtt topic esplora/61AE3D98
mqtt on
mqtt connect
```

Or use the web panel section `MQTT / Home Assistant`.

Published topics:

- `<baseTopic>/availability`
- `<baseTopic>/status`
- `<baseTopic>/neighbors`
- `<baseTopic>/events`

Subscribed command topics:

- `<baseTopic>/cmd`
- `<baseTopic>/send`
- `<baseTopic>/ha/ping/command`
- `<baseTopic>/ha/beacon_now/command`
- `<baseTopic>/ha/beacon_enabled/set`

Home Assistant discovery currently publishes:

- sensor: neighbor count
- sensor: Wi-Fi RSSI
- sensor: last RX RSSI
- sensor: last RX SNR
- binary sensor: Wi-Fi connected
- switch: beacon enabled
- button: ping
- button: beacon now

The discovery payloads are retained. Once your HA broker is configured, the entities should appear automatically after `mqtt on` and `mqtt connect`.

## Notes about "nearby nodes"

This firmware can discover and estimate distance only for nodes speaking the same Esplora packet format on the same LoRa settings. It can still log unknown packets as raw payload when modulation matches, but LoRa is not like Bluetooth scanning: if nearby devices use different frequency, bandwidth, spreading factor or protocol, they will not be decoded automatically.

Distance is only a rough estimate from RSSI. Indoors it can easily be off by a lot. Treat it as "same room / same building / nearby / farther away", not as a precise meter reading.

## Fixed IP

The fixed IP is baked in as:

- local IP: `192.168.1.201`
- gateway: `192.168.1.1`
- subnet: `255.255.255.0`
- DNS1: `1.1.1.1`
- DNS2: `8.8.8.8`

If your LAN uses a different gateway or subnet, adjust `WifiConfig` in [src/main.cpp](C:/Users/Biuro/Documents/Playground/wio-sx1262-lab/src/main.cpp).
