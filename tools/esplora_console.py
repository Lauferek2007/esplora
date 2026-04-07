#!/usr/bin/env python3
import argparse
import cmd
import queue
import threading
import time
from typing import Dict, List

import serial


def parse_kv_fields(parts: List[str]) -> Dict[str, str]:
    data: Dict[str, str] = {}
    for part in parts:
        if "=" in part:
            key, value = part.split("=", 1)
            data[key] = value
    return data


class SerialReader(threading.Thread):
    def __init__(self, ser: serial.Serial, inbox: "queue.Queue[str]") -> None:
        super().__init__(daemon=True)
        self.ser = ser
        self.inbox = inbox
        self.running = True

    def run(self) -> None:
        while self.running:
            try:
                raw = self.ser.readline()
            except serial.SerialException as exc:
                self.inbox.put(f"ERR|serial read failed: {exc}")
                return
            if not raw:
                continue
            line = raw.decode("utf-8", errors="replace").strip()
            if line:
                self.inbox.put(line)

    def stop(self) -> None:
        self.running = False


class EsploraConsole(cmd.Cmd):
    intro = "Esplora console. 'help' pokazuje komendy."
    prompt = "esplora> "

    def __init__(self, ser: serial.Serial, inbox: "queue.Queue[str]") -> None:
        super().__init__()
        self.ser = ser
        self.inbox = inbox
        self.reader = SerialReader(ser, inbox)
        self.reader.start()
        self.nodes: Dict[str, Dict[str, str]] = {}
        self.last_status: Dict[str, str] = {}
        self.last_wifi: Dict[str, str] = {}
        self._printer = threading.Thread(target=self._drain_inbox, daemon=True)
        self._printer.start()

    def _drain_inbox(self) -> None:
        while self.reader.running:
            try:
                line = self.inbox.get(timeout=0.2)
            except queue.Empty:
                continue
            self._remember(line)
            print(line)

    def _remember(self, line: str) -> None:
        parts = line.split("|")
        tag = parts[0]
        if tag == "NODE":
            data = parse_kv_fields(parts[1:])
            node_id = data.get("ID")
            if node_id:
                self.nodes[node_id] = data
        elif tag == "STAT":
            self.last_status = parse_kv_fields(parts[1:])
        elif tag == "WIFI":
            self.last_wifi = parse_kv_fields(parts[1:])

    def send_line(self, line: str) -> None:
        payload = line.strip()
        if not payload:
            return
        self.ser.write((payload + "\n").encode("utf-8"))
        self.ser.flush()

    def emptyline(self) -> bool:
        return False

    def do_status(self, arg: str) -> None:
        self.send_line("status")
        self.send_line("wifi status")

    def do_neighbors(self, arg: str) -> None:
        self.send_line("neighbors")

    def do_sightings(self, arg: str) -> None:
        self.send_line("sightings")

    def do_nodes(self, arg: str) -> None:
        if not self.nodes:
            print("(brak node'ow w cache)")
            return
        for node_id, data in sorted(self.nodes.items()):
            print(
                f"{node_id:>8}  name={data.get('NAME','')}  "
                f"rssi={data.get('RSSI','?')}  snr={data.get('SNR','?')}  "
                f"dist={data.get('DIST_M','?')}m  age={data.get('AGE_S','?')}s"
            )

    def do_send(self, arg: str) -> None:
        self.send_line(f"send {arg}")

    def do_ping(self, arg: str) -> None:
        self.send_line("ping")

    def do_cad(self, arg: str) -> None:
        self.send_line("cad")

    def do_sweep(self, arg: str) -> None:
        self.send_line("sweep")

    def do_beacon(self, arg: str) -> None:
        self.send_line(f"beacon {arg}".strip())

    def do_profile(self, arg: str) -> None:
        self.send_line(f"profile {arg}".strip())

    def do_set(self, arg: str) -> None:
        self.send_line(f"set {arg}".strip())

    def do_raw(self, arg: str) -> None:
        self.send_line(f"raw {arg}".strip())

    def do_restart(self, arg: str) -> None:
        self.send_line("restart radio")

    def do_wifi_status(self, arg: str) -> None:
        self.send_line("wifi status")

    def do_wifi_ssid(self, arg: str) -> None:
        self.send_line(f"wifi ssid {arg}".strip())

    def do_wifi_pass(self, arg: str) -> None:
        self.send_line(f"wifi pass {arg}".strip())

    def do_wifi_connect(self, arg: str) -> None:
        self.send_line("wifi connect")

    def do_wifi_off(self, arg: str) -> None:
        self.send_line("wifi off")

    def do_last(self, arg: str) -> None:
        print(f"STAT: {self.last_status}")
        print(f"WIFI: {self.last_wifi}")

    def do_rawcmd(self, arg: str) -> None:
        self.send_line(arg)

    def do_exit(self, arg: str) -> bool:
        self.reader.stop()
        time.sleep(0.2)
        self.ser.close()
        return True

    def do_quit(self, arg: str) -> bool:
        return self.do_exit(arg)

    def do_EOF(self, arg: str) -> bool:
        print()
        return self.do_exit(arg)


def open_serial(port: str, baud: int) -> serial.Serial:
    ser = serial.Serial(port, baudrate=baud, timeout=0.2)
    ser.dtr = True
    ser.rts = False
    time.sleep(0.4)
    return ser


def run_batch(ser: serial.Serial, commands: List[str]) -> None:
    inbox: "queue.Queue[str]" = queue.Queue()
    reader = SerialReader(ser, inbox)
    reader.start()
    for command in commands:
        ser.write((command.strip() + "\n").encode("utf-8"))
        ser.flush()
        time.sleep(0.2)
    deadline = time.time() + 6
    while time.time() < deadline:
        try:
            line = inbox.get(timeout=0.3)
        except queue.Empty:
            continue
        print(line)
    reader.stop()
    ser.close()


def main() -> None:
    parser = argparse.ArgumentParser(description="Serial console for Esplora firmware.")
    parser.add_argument("--port", default="COM5", help="Serial port, default: COM5")
    parser.add_argument("--baud", default=115200, type=int, help="Baud rate, default: 115200")
    parser.add_argument(
        "--command",
        action="append",
        default=[],
        help="Send one or more commands and exit instead of opening interactive console.",
    )
    args = parser.parse_args()

    ser = open_serial(args.port, args.baud)

    if args.command:
        run_batch(ser, args.command)
        return

    inbox: "queue.Queue[str]" = queue.Queue()
    shell = EsploraConsole(ser, inbox)
    shell.send_line("status")
    shell.send_line("wifi status")
    shell.cmdloop()


if __name__ == "__main__":
    main()
