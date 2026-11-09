## @file test_web.py
# @brief Controleert bediening, invoergrenzen en mobiele GUI op een echt apparaat.
# @date 2026-11-08
# @author ProtoIoT Wietse Houwers
# @note Prototype software for evaluation use only.

import argparse
from playwright.sync_api import sync_playwright, expect


def main():
    """@brief Controleert de echte GUI; motorbediening vereist --exercise-controls."""
    parser = argparse.ArgumentParser(description="Controleer de GUI op een aangesloten Stabielator.")
    parser.add_argument("--base-url", required=True, help="Bijvoorbeeld http://192.168.4.1")
    parser.add_argument("--exercise-controls", action="store_true", help="Sta pauze-, teststand- en korte motortests toe.")
    args = parser.parse_args()
    base = args.base_url.rstrip("/")
    with sync_playwright() as p:
        browser = p.chromium.launch(channel="chrome", headless=True)
        page = browser.new_page()
        errors = []
        page.on("pageerror", lambda error: errors.append(str(error)))
        original = None
        controls_changed = False
        try:
            page.goto(base + "/basis")
            expect(page.locator("#save")).to_be_enabled()
            expect(page.locator("#test")).to_be_enabled()
            original = page.request.get(base + "/api/status").json()
            assert isinstance(original["paused"], bool)
            expect(page.locator("#testMotor option")).to_have_count(4)
            for action in ("calibrate", "stop", "resume"):
                expect(page.locator(f'[data-action="{action}"]')).to_be_visible()
            # Een ongeldige testduur wordt lokaal geweigerd, zonder motoropdracht.
            page.locator("#testDuration").fill("0")
            page.locator("#test").click()
            expect(page.locator("#message")).to_contain_text("20 tot 3000 ms")
            if args.exercise_controls:
                assert original["pause_saved"], "Bewaar eerst een geldige pauzekeuze."
                assert original["imu_healthy"] and all(original["motors_present"]), "Hardware niet gereed."
                controls_changed = True
                page.locator('[data-action="stop"]').click()
                expect(page.locator("#message")).to_contain_text("Pauze opgeslagen")
                page.locator("#testDuration").fill("100")
                for motor in range(1, 5):
                    page.locator("#testMotor").select_option(str(motor))
                    page.locator("#test").click()
                    expect(page.locator("#message")).to_contain_text("Patroontest gestart")
                    page.wait_for_timeout(400)
                    state = page.request.get(base + "/api/status").json()
                    assert state["paused"] and state["pause_saved"] and not state["manual_test"]
                    assert not any(state["motor_levels"])
            page.goto(base + "/advanced")
            expect(page.locator("#save")).to_be_enabled()
            for selector in ("#developmentCard", "#firmwareCard", "#openDiagnostics"):
                expect(page.locator(selector)).to_be_visible()
            if args.exercise_controls:
                for enabled in (True, False):
                    action = "test_mode_on" if enabled else "test_mode_off"
                    page.locator(f'[data-action="{action}"]').click()
                    expect(page.locator("#message")).to_contain_text("opgeslagen")
                    assert page.request.get(base + "/api/status").json()["test_mode"] == enabled
            page.locator("#openDiagnostics").click()
            expect(page.locator("#diagnostics")).to_be_visible()
            page.locator("#closeDiagnostics").click()
            page.set_viewport_size({"width": 390, "height": 844})
            assert page.evaluate("document.documentElement.scrollWidth <= innerWidth"), "Horizontale overflow."
            assert not errors, errors
            print("GUI, invoervalidatie en mobiele layout: PASS")
        finally:
            if controls_changed:
                try:
                    response = page.request.post(base + "/api/control", form={"action": "test_mode_on" if original["test_mode"] else "test_mode_off"})
                    assert response.ok, "Oorspronkelijke teststand herstellen mislukt."
                    response = page.request.post(base + "/api/control", form={"action": "stop" if original["paused"] else "resume"})
                    assert response.ok, "Oorspronkelijke pauzekeuze herstellen mislukt."
                finally:
                    browser.close()
            else:
                browser.close()


if __name__ == "__main__":
    main()
