## @file test_ui.py
# @brief Controleert de echte OTA-GUI met optionele bestandsselectie en expliciete upload.
# @date 2026-11-08
# @author ProtoIoT Wietse Houwers
# @note Prototype software for evaluation use only.

import argparse
from pathlib import Path
from playwright.sync_api import sync_playwright, expect


def main():
    """@brief Controleert de echte updatepagina; --upload staat expliciet een firmware-update toe."""
    parser = argparse.ArgumentParser(description="Controleer OTA via het eigen Stabielator-AP.")
    parser.add_argument("--base-url", required=True)
    parser.add_argument("--firmware", type=Path, help="Passend .stab.bin-bestand voor bestandscontrole.")
    parser.add_argument("--upload", action="store_true", help="Upload de gekozen firmware en controleer de herstart.")
    args = parser.parse_args()
    if args.upload and not args.firmware:
        parser.error("--upload vereist --firmware")
    with sync_playwright() as p:
        browser = p.chromium.launch(channel="chrome", headless=True)
        page = browser.new_page()
        errors = []
        page.on("pageerror", lambda error: errors.append(str(error)))
        try:
            page.goto(args.base_url.rstrip("/") + "/update")
            expect(page.locator("#file")).to_be_enabled()
            expect(page.locator("#start")).to_be_disabled()
            expect(page.locator("#current")).to_contain_text("Actieve firmware")
            if args.firmware:
                page.locator("#file").set_input_files(str(args.firmware.resolve()))
                expect(page.locator("#start")).to_be_enabled()
                expect(page.locator("#selected")).to_contain_text("Nieuwe versie")
            if args.upload:
                page.locator("#start").click()
                expect(page.locator("#message")).to_contain_text("Opnieuw verbonden met firmware", timeout=120000)
                expect(page.locator("#file")).to_be_enabled()
            assert not errors, errors
            print("OTA-GUI en opgegeven bestands-/uploadcontrole: PASS")
        finally:
            browser.close()


if __name__ == "__main__":
    main()
