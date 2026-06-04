#!/usr/bin/env python3
"""
build_firmware.py - Build credential-free release firmware, one binary per LoRa region.

It renders a GENERIC board_credentials.h (no board's credentials baked in),
then builds each region environment. copyFirmware.py drops the result into
Firmware/firmware_<region>_v<version>.bin.

Use flash_board.py instead when you want to flash a specific board with its
real credentials.

Usage:
    python build_firmware.py                 # build all regions (EU US AU AS JP KR IN)
    python build_firmware.py EU US           # build only these regions
    python build_firmware.py --dry-run       # show the plan, build nothing
"""

import argparse
import os
import subprocess
import sys

import flash_board as fb

ALL_REGIONS = ["EU", "US", "AU", "AS", "JP", "KR", "IN"]

# Generic, credential-free values for the public release firmware.
# Boards receive their real credentials later (via flash_board.py or runtime config).
GENERIC_BOARD = {
    "kit_id": "1000",
    "api_key": "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
    "appeui": "0000000000000000",
    "deveui": "0000000000000000",
    "appkey": "00000000000000000000000000000000",
    "mqtt_ip": fb.DEFAULT_MQTT_IP,
}


def main():
    ap = argparse.ArgumentParser(description="Build credential-free release firmware per LoRa region.")
    ap.add_argument("regions", nargs="*", help=f"Regions to build. Default: {' '.join(ALL_REGIONS)}")
    ap.add_argument("--dry-run", action="store_true", help="Show the plan without rendering or building.")
    args = ap.parse_args()

    regions = [r.upper() for r in args.regions] or list(ALL_REGIONS)
    unknown = [r for r in regions if r not in ALL_REGIONS]
    if unknown:
        sys.exit(f"ERROR: unknown region(s): {', '.join(unknown)}. Valid: {', '.join(ALL_REGIONS)}")

    print("== Release build (generic, NO credentials) ==")
    print(f"   Regions: {', '.join(regions)}")
    print(f"   boardID: {GENERIC_BOARD['kit_id']}  (API key = placeholder)")

    if args.dry_run:
        print("-- dry-run: would render generic header and build each region --")
        return

    fb.render_header(GENERIC_BOARD)
    print(f"   Wrote generic {fb.HEADER_PATH.relative_to(fb.ROOT)}")

    failures = []
    for region in regions:
        cmd = ["pio", "run", "-e", region]
        print(f"\n-- running: {' '.join(cmd)}")
        try:
            result = subprocess.run(cmd)
        except FileNotFoundError:
            sys.exit("ERROR: 'pio' not found. Run from a terminal where PlatformIO is on PATH "
                     "(or use the PlatformIO IDE terminal).")
        if result.returncode != 0:
            failures.append(region)

    print("\n== Done ==")
    if failures:
        sys.exit(f"FAILED regions: {', '.join(failures)}")
    print(f"Built {len(regions)} region(s). Binaries in Firmware/.")


if __name__ == "__main__":
    main()
