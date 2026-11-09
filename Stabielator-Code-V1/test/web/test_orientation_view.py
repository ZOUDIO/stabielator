## @file test_orientation_view.py
# @brief Controleert live IMU-weergave en stoppen van polling zonder gesimuleerde API.
# @date 2026-11-08
# @author ProtoIoT Wietse Houwers
# @note Prototype software for evaluation use only.

import argparse
from playwright.sync_api import sync_playwright, expect


def main():
    """@brief Controleert echte IMU-weergave en polling; --enable-debug herstelt de oorspronkelijke instelling."""
    parser = argparse.ArgumentParser(description="Controleer live 3D op een aangesloten Stabielator.")
    parser.add_argument("--base-url", required=True)
    parser.add_argument("--enable-debug", action="store_true", help="Schakel debug tijdelijk in en herstel de oude keuze.")
    args = parser.parse_args()
    base = args.base_url.rstrip("/")
    with sync_playwright() as p:
        browser = p.chromium.launch(channel="chrome", headless=True)
        page = browser.new_page()
        errors, requests = [], []
        page.on("pageerror", lambda error: errors.append(str(error)))
        page.on("request", lambda request: requests.append(request.url) if request.url.endswith("/api/orientation") else None)
        settings = None
        try:
            response = page.request.get(base + "/api/settings")
            assert response.ok
            settings = response.json()
            if args.enable_debug:
                assert page.request.post(base + "/api/settings", form={"debugEnabled": "1"}).ok
            page.goto(base + "/advanced")
            page.wait_for_function("typeof orientationMath !== 'undefined' && typeof window.StabielatorCube !== 'undefined'")
            assert page.evaluate("orientationMath.normalize([0,0,0,0]) === null")
            assert page.evaluate("orientationMath.rotate([1,0,0,0],[1,2,3]).every((v,i)=>v===[1,2,3][i])")
            panel = page.locator("#orientationCard")
            if not (args.enable_debug or settings["debugEnabled"]):
                expect(panel).to_be_hidden()
                page.wait_for_timeout(600)
                assert not requests, "Ori?ntatieverkeer terwijl debug uit staat."
                print("Debug uit: liveverkeer gestopt PASS; gebruik --enable-debug voor live IMU-controle.")
            else:
                expect(panel).to_be_visible()
                page.wait_for_timeout(1500)
                assert requests, "Geen liveverzoeken ontvangen."
                data = page.request.get(base + "/api/orientation").json()
                assert data["debug"] and data["valid"], "De echte IMU moet actuele meetwaarden leveren."
                expect(page.locator("#orientationSource")).to_have_text("Live IMU")
                expect(page.locator("#orientationState")).to_contain_text("Absolute houding")
                if data["relative"] is not None:
                    page.locator("#orientationMode").select_option("relative")
                    expect(page.locator("#orientationState")).to_contain_text("gekalibreerde referentie")
                panel.locator("summary").click()
                page.wait_for_timeout(300)
                count = len(requests)
                page.wait_for_timeout(600)
                assert len(requests) == count, "Polling loopt door bij een gesloten paneel."
                print("Live IMU, relatieve houding en stoppen van polling: PASS")
            assert not errors, errors
        finally:
            try:
                if args.enable_debug and settings is not None:
                    response = page.request.post(base + "/api/settings", form={"debugEnabled": str(int(settings["debugEnabled"]))})
                    assert response.ok, "Debugkeuze herstellen mislukt."
            finally:
                browser.close()


if __name__ == "__main__":
    main()
