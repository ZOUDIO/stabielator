/**
 * @file wifi_portal_branding.h
 * @brief Voegt productidentiteit en netwerkstatusuitleg aan de captive portal toe.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#pragma once
#include "config.h"

static constexpr char WIFI_PORTAL_CUSTOM_HEAD[] = R"HTML(
<script>
document.addEventListener('DOMContentLoaded', function () {
  const appVersion = 'Stabielator v)HTML" APP_VERSION_TEXT R"HTML(';
  const appReference = 'ProtoIoT.nl - 2026 | Prototype software for evaluation use only.';

  if (window.location.pathname === '/wifisave') {
    const message = document.querySelector('.msg');
    if (message) message.textContent = 'WiFi-invoer ontvangen. Verbinden wordt geprobeerd; het instelnetwerk volgt de opgeslagen AP-keuze. Het routeradres staat in de seriele monitor.';
  }

  const items = document.querySelectorAll('dt');
  for (const item of items) {
    if (item.textContent.trim() === 'WiFiManager' &&
        !document.getElementById('protoiot-version-dt')) {
      const label = document.createElement('dt');
      label.id = 'protoiot-version-dt';
      label.textContent = 'Version';
      const value = document.createElement('dd');
      value.textContent = appVersion;
      item.parentNode.insertBefore(label, item);
      item.parentNode.insertBefore(value, item);
    }
  }

  const links = document.querySelectorAll('a[href*="github.com/tzapu/WiFiManager"]');
  for (const link of links) {
    if (link.parentNode) link.parentNode.textContent = appReference;
  }


});
</script>
)HTML";
