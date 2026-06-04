#!/usr/bin/env python3
"""
flash_board.py - Flash a TeleAgriculture board with the credentials of a Kit ID.

Reads Boards_List.csv, looks up the given Kit ID, renders
include/board_credentials.template.h into include/board_credentials.h with the
board's credentials, then builds & uploads the firmware with PlatformIO using
the LoRa region of that board as the build environment.

Usage:
    python flash_board.py <KIT_ID> [options]

Examples:
    python flash_board.py 1014                 # build + upload board 1014
    python flash_board.py 1014 --port COM7     # pin the serial port
    python flash_board.py 1014 --env US        # override the LoRa region/env
    python flash_board.py 1014 --no-upload     # only generate header + build
    python flash_board.py 1014 --dry-run       # show what would happen, do nothing
"""

import argparse
import csv
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
CSV_PATH = ROOT / "Boards_List.csv"
TEMPLATE_PATH = ROOT / "include" / "board_credentials.template.h"
HEADER_PATH = ROOT / "include" / "board_credentials.h"

DEFAULT_MQTT_IP = "158.255.212.248"

# CSV LoRa-Region label -> PlatformIO env name (see platformio.ini)
REGION_TO_ENV = {
    "EU": "EU",
    "US": "US",
    "AU": "AU",
    "AUS": "AU",
    "AS": "AS",
    "ASIA": "AS",
    "JP": "JP",
    "KR": "KR",
    "IN": "IN",
    "IND": "IN",
}

# logical field name -> set of possible CSV column headers (matched case/space-insensitively)
FIELD_ALIASES = {
    "kit_id": ["kit id", "kitid"],
    "name": ["name in the app", "name"],
    "api_key": ["api key", "apikey"],
    "region": ["lora region", "region"],
    "appeui": ["otaa_appeui"],
    "deveui": ["otaa_deveui"],
    "appkey": ["otaa_appkey"],
    "mqtt_ip": ["mqtt_server_ip", "mqtt server ip"],
}


def clean(value):
    """Strip whitespace and the leading Excel text-marker apostrophe."""
    if value is None:
        return ""
    return value.strip().lstrip("'").strip()


def build_column_map(header):
    """Map our logical field names to the actual column index in the CSV."""
    norm = [clean(h).lower() for h in header]
    colmap = {}
    for field, aliases in FIELD_ALIASES.items():
        for alias in aliases:
            if alias in norm:
                colmap[field] = norm.index(alias)
                break
    missing = {"kit_id", "api_key"} - colmap.keys()
    if missing:
        sys.exit(f"ERROR: CSV is missing required column(s): {', '.join(missing)}")
    return colmap


def load_board(kit_id):
    """Return merged credential dict for kit_id, or exit with an error."""
    if not CSV_PATH.exists():
        sys.exit(f"ERROR: {CSV_PATH.name} not found next to this script.")

    # latin-1 never raises on odd bytes; board names may contain non-UTF8 chars.
    with open(CSV_PATH, "r", encoding="latin-1", newline="") as fh:
        rows = list(csv.reader(fh))
    if not rows:
        sys.exit("ERROR: Boards_List.csv is empty.")

    colmap = build_column_map(rows[0])

    def cell(row, field):
        idx = colmap.get(field)
        if idx is None or idx >= len(row):
            return ""
        return clean(row[idx])

    # A Kit ID may be spread over several rows (name/key in one, LoRa data in
    # another). Merge: for each field keep the first non-empty value found.
    merged = {}
    found = False
    for row in rows[1:]:
        if cell(row, "kit_id") != str(kit_id):
            continue
        found = True
        for field in FIELD_ALIASES:
            if not merged.get(field):
                merged[field] = cell(row, field)

    if not found:
        sys.exit(f"ERROR: Kit ID {kit_id} not found in {CSV_PATH.name}.")
    if not merged.get("api_key"):
        sys.exit(f"ERROR: Kit ID {kit_id} has no API key in the CSV.")
    return merged


def resolve_env(region, override):
    if override:
        return override
    key = (region or "").upper()
    if not key:
        print(f"  ! No LoRa region in CSV -> defaulting to env 'EU'.")
        return "EU"
    env = REGION_TO_ENV.get(key)
    if not env:
        sys.exit(f"ERROR: Unknown LoRa region '{region}'. Use --env to set it manually.")
    return env


def render_header(board):
    if not TEMPLATE_PATH.exists():
        sys.exit(f"ERROR: template not found: {TEMPLATE_PATH}")
    text = TEMPLATE_PATH.read_text(encoding="utf-8")

    appeui = board.get("appeui") or ""
    # CSV stores APPEUI as "0"; LMIC expects a 16-hex-digit string.
    if appeui in ("", "0"):
        appeui = "0000000000000000"
    deveui = board.get("deveui") or "0000000000000000"
    appkey = board.get("appkey") or "00000000000000000000000000000000"
    mqtt_ip = board.get("mqtt_ip") or DEFAULT_MQTT_IP

    replacements = {
        "__BOARD_ID__": str(board["kit_id"]),
        "__API_KEY__": board["api_key"],
        "__OTAA_APPEUI__": appeui,
        "__OTAA_DEVEUI__": deveui,
        "__OTAA_APPKEY__": appkey,
        "__MQTT_SERVER_IP__": mqtt_ip,
    }
    for token, value in replacements.items():
        text = text.replace(token, value)

    leftover = [t for t in replacements if t in text]
    if leftover:
        sys.exit(f"ERROR: unreplaced placeholder(s) remain: {leftover}")

    HEADER_PATH.write_text(text, encoding="utf-8")


def main():
    ap = argparse.ArgumentParser(description="Flash a TAC board by Kit ID.")
    ap.add_argument("kit_id", help="Kit ID from Boards_List.csv, e.g. 1014")
    ap.add_argument("--port", help="Serial upload port (e.g. COM7). Default: PIO autodetect.")
    ap.add_argument("--env", help="Override PlatformIO env / LoRa region (EU, US, AU, AS, JP, KR, IN).")
    ap.add_argument("--no-upload", action="store_true", help="Generate header and build, but do not upload.")
    ap.add_argument("--dry-run", action="store_true", help="Show the plan without touching files or flashing.")
    args = ap.parse_args()

    board = load_board(args.kit_id)
    env = resolve_env(board.get("region"), args.env)

    print(f"== Kit {args.kit_id}: {board.get('name') or '(no name)'} ==")
    print(f"   Region : {board.get('region') or '(none)'}  ->  PlatformIO env '{env}'")
    print(f"   DEVEUI : {board.get('deveui') or '(none)'}")
    print(f"   MQTT   : {board.get('mqtt_ip') or DEFAULT_MQTT_IP}")
    print(f"   Port   : {args.port or 'autodetect'}")

    target = "build (no upload)" if args.no_upload else "upload"
    if args.dry_run:
        print(f"-- dry-run: would render header and run PlatformIO target '{target}' --")
        return

    render_header(board)
    print(f"   Wrote  : {HEADER_PATH.relative_to(ROOT)}")

    cmd = ["pio", "run", "-e", env]
    if not args.no_upload:
        cmd += ["-t", "upload"]
        if args.port:
            cmd += ["--upload-port", args.port]

    # A board flash bakes in real credentials - do NOT let copyFirmware.py
    # overwrite the credential-free release binaries in Firmware/.
    child_env = os.environ.copy()
    child_env["TAC_SKIP_RELEASE_COPY"] = "1"

    print(f"-- running: {' '.join(cmd)}")
    try:
        result = subprocess.run(cmd, env=child_env)
    except FileNotFoundError:
        sys.exit("ERROR: 'pio' not found. Run from a terminal where PlatformIO is on PATH "
                 "(or use the PlatformIO IDE terminal).")
    sys.exit(result.returncode)


if __name__ == "__main__":
    main()
