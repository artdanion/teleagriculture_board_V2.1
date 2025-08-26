import os
import shutil
import re
from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()

platformio_env = env['PIOENV']        # z. B. EU, US, ...
project_dir = env['PROJECT_DIR']
build_dir = env.subst("$BUILD_DIR")   # .pio/build/<env>

version = "unknown"
freq = platformio_env  # fallback falls CUSTOM_LORA_FQZ fehlt

for flag in env.get("BUILD_FLAGS", []):
    # TAC_VERSION extrahieren
    m_ver = re.search(r'TAC_VERSION="?([\d\.]+)"?', flag)
    if m_ver:
        version = m_ver.group(1).replace(".", "_")

    # CUSTOM_LORA_FQZ extrahieren (z.B. EU 868 MHz)
    m_fqz = re.search(r'CUSTOM_LORA_FQZ="?([A-Z/]+)\s+(\d+)\s*MHz"?', flag)
    if m_fqz:
        region = m_fqz.group(1).replace("/", "_")   # US/CAN → US_CAN
        mhz = m_fqz.group(2)
        freq = f"{region}_{mhz}"

# Ausgabeverzeichnis
firmware_dir = os.path.join(project_dir, "Firmware")
os.makedirs(firmware_dir, exist_ok=True)

def copy_firmware(source, target, env):
    firmware_src = os.path.join(build_dir, "firmware.bin")
    firmware_dst = os.path.join(
        firmware_dir,
        f"firmware_{freq}_v{version}.bin"
    )

    print(f"\n>>> Kopiere Firmware nach: {firmware_dst}")
    shutil.copy(firmware_src, firmware_dst)

# Hook nach Build
env.AddPostAction("$BUILD_DIR/firmware.bin", copy_firmware)
