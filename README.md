# Esplora: LoRa lab for XIAO ESP32S3 + Wio-SX1262

This project flashes a fresh firmware onto the Seeed XIAO ESP32S3 + Wio-SX1262 kit and turns it into a small LoRa lab node:

- serial CLI on `COM5`
- LoRa chat packets
- periodic beacons for nearby node discovery
- ping/pong over LoRa
- neighbor list with RSSI, SNR and rough distance estimate
- Wi-Fi helper with fixed target IP `192.168.1.201`

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

SSID and password are saved in NVS. After you set them once, the node will try to come up on `192.168.1.201` after reboot.

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
