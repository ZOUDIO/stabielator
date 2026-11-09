## @file package_ota.py
# @brief Verpakt productiefirmware met hardwaredoel, versie, lengte en SHA-256 voor OTA.
# @date 2026-11-08
# @author ProtoIoT Wietse Houwers
# @note Prototype software for evaluation use only.

Import("env")
import hashlib
import re
import struct
from pathlib import Path


def package(source, target, env):
    """@brief Verpakt de applicatie in de 128-byte STABOTA1-envelop voor het gekozen productieprofiel."""
    root = Path(env["PROJECT_DIR"])
    version = re.search(r'#define APP_VERSION_TEXT "([^"]+)"',
                        (root / "src/config.h").read_text()).group(1)
    profile = {"esp32-c3-devkitm-1": "erm", "lra": "lra"}[env["PIOENV"]]
    image = Path(str(target[0])).read_bytes()
    if len(image) > 0x1E0000:
        raise RuntimeError("Firmware past niet in de OTA-partitie (1966080 bytes)")
    identity = "stab-c3-" + profile
    header = struct.pack("<8s32s32sI32s20x", b"STABOTA1", identity.encode(),
                         version.encode(), len(image), hashlib.sha256(image).digest())
    output = root / "dist" / "ota" / f"Stabielator-{profile}-v{version}.stab.bin"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(header + image)
    print(f"OTA: {output} ({len(image)} bytes applicatie, target {identity})")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", package)
