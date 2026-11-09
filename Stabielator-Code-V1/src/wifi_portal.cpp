/**
 * @file wifi_portal.cpp
 * @brief Beheert opgeslagen AP-keuze, routerverbinding en niet-blokkerende captive portal.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#include "wifi_portal.h"
#include "wifi_portal_branding.h"
#include "wifi_events.h"

#include <Preferences.h>
#include <esp_wifi.h>
#include <string.h>

#include "device_identity.h"
#include "web_status_server.h"
#include "firmware_update.h"

WifiPortal wifiPortal;

void WifiPortal::begin() {
  beginWifiEventLogging();
  deviceIdentity.begin();
  buildAccessPointName();

  Preferences prefs;
  if (prefs.begin(APP_PREFERENCES_NAMESPACE, true)) {
    // Ontbrekend of beschadigd: hersteltoegang standaard beschikbaar.
    if (prefs.getType("ap_enabled")==PT_U8) _apEnabled=prefs.getUChar("ap_enabled",1)!=0;
    prefs.end();
  }

  configureStationMode();

  configureManager();

  _stationWanted=hasStoredCredentials();
  if (_apEnabled) startAccessPoint();
  if (_stationWanted) startStationConnect();
}

bool WifiPortal::isAccessPointHidden() const {
  wifi_config_t config = {};
  return esp_wifi_get_config(WIFI_IF_AP,&config)==ESP_OK && config.ap.ssid_hidden;
}

void WifiPortal::update() {
  if (firmwareUpdate.busy()) { if (_pairingActive) _manager.process(); return; }
  if (_apChangePending && millis()-_apChangeAt>=750) {
    _apChangePending=false;
    if (_apEnabled) startAccessPoint(); // Expliciet Aan/portal herstelt ook een vastgelopen AP.
    else stopAccessPoint();
  }
  if (_credentialsPending && millis()-_credentialsQueuedAt>=750) {
    _credentialsPending=false;
    saveStationCredentialsAndReconnect(_pendingSsid,_pendingPassword);
    memset(_pendingPassword,0,sizeof(_pendingPassword));
  }
  processWifiEventLog();
  if (_pairingActive) {
    _manager.process();
    if (!_manager.getConfigPortalActive()) { _pairingActive=false; if (_apEnabled) startAccessPoint(); }
  } else if (_apEnabled && !_apChangePending && millis()-_lastApAttemptMs>=WIFI_RECONNECT_INTERVAL_MS) {
    startAccessPoint();
  }
  if (isConnected()) { _stationConnecting=false; return; }
  if (_stationConnecting && millis()-_stationAttemptAt>=WIFI_CONNECT_TIMEOUT_SECONDS*1000U) {
    WiFi.disconnect(false,false);
    _stationConnecting=false;
    Serial.println("[WiFi] Routerverbinding mislukt/timeout; AP-keuze blijft behouden.");
  }
  // Geen achtergrondscans/reconnects terwijl iemand de primaire AP-bediening gebruikt.
  if (_stationWanted && !_stationConnecting && !WiFi.softAPgetStationNum() &&
      millis()-_lastReconnectAttemptMs>=WIFI_PORTAL_FALLBACK_MS) startStationConnect();
}

void WifiPortal::startPairing() {
  if (!setAccessPointEnabled(true)) Serial.println("FOUT: AP inschakelen niet opgeslagen.");
}

bool WifiPortal::setAccessPointEnabled(bool enabled) {
  if (firmwareUpdate.busy()) return false;
  Preferences prefs;
  if (!prefs.begin(APP_PREFERENCES_NAMESPACE,false)) return false;
  const bool ok=prefs.putBool("ap_enabled",enabled)==1;
  prefs.end();
  if (!ok) return false;
  _apEnabled=enabled;
  _apChangePending=true; _apChangeAt=millis();
  return true; // Eerst HTTP-antwoord verzenden, daarna pas de verbinding wijzigen.
}

void WifiPortal::stopAccessPoint() {
  if (_manager.getConfigPortalActive()) _manager.stopConfigPortal();
  _pairingActive=false;
  WiFi.softAPdisconnect(false);
  WiFi.mode(WIFI_STA); // Routergegevens en actieve STA-verbinding behouden.
  WiFi.setAutoReconnect(false);
  Serial.println("[WiFi] AP uit (opgeslagen). Herstellen via 'ap on' of 'portal'.");
}

void WifiPortal::startAccessPoint() {
  _lastApAttemptMs=millis();
  buildAccessPointName();
  configureManager();

  // Een nieuw portal-commando herstart ook een eerder vastgelopen portal.
  if (_manager.getConfigPortalActive()) _manager.stopConfigPortal();
  _pairingActive = false;
  webStatusServer.update(false, true); // Geef poort 80 vrij voor WiFiManager.

  Serial.print(F("WiFi-instelnetwerk starten: "));
  Serial.println(_accessPointName);

  // Geen achtergrond-reconnect/scans tijdens het starten van het instelnetwerk.
  WiFi.setAutoReconnect(false);
  if (!isConnected()) WiFi.disconnect(false, false); // Bewaar opgeslagen gegevens.
  applyStationHostname();
  if (!WiFi.mode(isConnected() ? WIFI_AP_STA : WIFI_AP)) {
    Serial.println("FOUT: WiFi AP-modus kon niet starten.");
    printDiagnostics();
    return;
  }
  WiFi.setSleep(false);

  if (strlen(WIFI_AP_PASSWORD) >= 8) {
    _manager.startConfigPortal(_accessPointName, WIFI_AP_PASSWORD);
  } else {
    _manager.startConfigPortal(_accessPointName);
  }

  // WiFiManager 2.0.17 zet zijn portalvlag ook wanneer softAP() mislukt.
  // Controleer de echte softAP-start met expliciet kanaal en zichtbare SSID.
  const char* password = strlen(WIFI_AP_PASSWORD) >= 8 ? WIFI_AP_PASSWORD : nullptr;
  const bool apStarted = WiFi.softAP(_accessPointName, password, WIFI_AP_CHANNEL, false, 1);
  _pairingActive = _manager.getConfigPortalActive() && apStarted &&
                   uint32_t(WiFi.softAPIP()) != 0;
  if (!_pairingActive) {
    Serial.println("FOUT: instelnetwerk/portal niet volledig gestart.");
    if (_manager.getConfigPortalActive()) _manager.stopConfigPortal();
  } else {
    Serial.printf("Instelnetwerk gestart: %s; open http://%s\n",
                  _accessPointName, WiFi.softAPIP().toString().c_str());
  }
  // De SSID blijft zichtbaar. esp_wifi_set_config bij join/leave herstart op
  // deze core het AP en verbreekt Windows-clients; nooit dynamisch omschakelen.
  printDiagnostics();
}

void WifiPortal::printDiagnostics() const {
  const wifi_mode_t mode = WiFi.getMode();
  Serial.printf("[WiFi] AP opgeslagen=%s wijziging=%s\n",_apEnabled?"aan":"uit",_apChangePending?"wacht":"klaar");
  Serial.printf("[WiFi] mode=%u STA-status=%u portal=%s uptime=%lu s\n",
                unsigned(mode), unsigned(WiFi.status()),
                _pairingActive ? "actief" : "uit",
                static_cast<unsigned long>(millis() / 1000));
  if (mode != WIFI_OFF)
    Serial.printf("[WiFi] TX=%.2f dBm\n", int(WiFi.getTxPower()) / 4.0f);
  if (isConnected()) {
    Serial.printf("[WiFi STA] SSID=%s IP=%s RSSI=%ld dBm\n", WiFi.SSID().c_str(),
                  WiFi.localIP().toString().c_str(), static_cast<long>(WiFi.RSSI()));
    Serial.printf("[WiFi STA] Statuspagina: http://%s/\n", WiFi.localIP().toString().c_str());
  }
  wifi_config_t config = {};
  const esp_err_t result = esp_wifi_get_config(WIFI_IF_AP, &config);
  if ((mode & WIFI_AP) && result == ESP_OK) {
    Serial.printf("[WiFi AP] SSID=%.*s kanaal=%u verborgen=%u IP=%s clients=%u\n",
                  int(config.ap.ssid_len), reinterpret_cast<const char*>(config.ap.ssid),
                  unsigned(config.ap.channel), unsigned(config.ap.ssid_hidden),
                  WiFi.softAPIP().toString().c_str(), unsigned(WiFi.softAPgetStationNum()));
  } else {
    Serial.printf("[WiFi AP] niet actief; config-resultaat=%d, verwachte SSID=%s\n",
                  int(result), _accessPointName);
  }
}

bool WifiPortal::saveStationCredentialsAndReconnect(const char* ssid,
                                                    const char* password) {
  if (ssid == nullptr || ssid[0] == '\0' ||
      strlen(ssid) > WIFI_SSID_MAX_LENGTH) {
    return false;
  }

  const char* safePassword = password == nullptr ? "" : password;
  if (strlen(safePassword) > WIFI_PASSWORD_MAX_LENGTH) {
    return false;
  }

  _stationWanted=true;
  configureStationMode();
  WiFi.persistent(true);
  WiFi.begin(ssid,safePassword);
  WiFi.persistent(false);
  _stationConnecting=true; _stationAttemptAt=millis();

  _lastReconnectAttemptMs = millis();
  return true;
}

bool WifiPortal::queueStationCredentials(const char* ssid, const char* password) {
  if (!ssid || !ssid[0] || strlen(ssid) > WIFI_SSID_MAX_LENGTH || !password) return false;
  const size_t length = strlen(password);
  if (length && (length < 8 || length > 63)) return false;
  snprintf(_pendingSsid, sizeof(_pendingSsid), "%s", ssid);
  snprintf(_pendingPassword, sizeof(_pendingPassword), "%s", password);
  _credentialsQueuedAt = millis();
  _credentialsPending = true;
  return true;
}

void WifiPortal::factoryResetAndReboot() {
  Serial.println(F("Factory reset: WiFi-credentials en app-instellingen wissen."));

  _manager.resetSettings();
  WiFi.disconnect(true, true);
  clearApplicationSettings();

  Serial.println(F("Factory reset klaar. ESP32 herstart nu in pairing mode."));
  Serial.flush();
  ESP.restart();
}

bool WifiPortal::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

bool WifiPortal::isPairingActive() const {
  return _pairingActive;
}

int32_t WifiPortal::rssi() const {
  if (!isConnected()) {
    return 0;
  }

  return WiFi.RSSI();
}

const char* WifiPortal::statusText() const {
  if (_pairingActive) {
    return "pairing/captive portal";
  }

  switch (WiFi.status()) {
    case WL_CONNECTED:
      return "connected";
    case WL_NO_SSID_AVAIL:
      return "ssid unavailable";
    case WL_CONNECT_FAILED:
      return "connect failed";
    case WL_CONNECTION_LOST:
      return "connection lost";
    case WL_DISCONNECTED:
      return "disconnected";
    case WL_IDLE_STATUS:
      return "idle";
    default:
      return "unknown";
  }
}

const char* WifiPortal::accessPointName() const {
  return _accessPointName;
}

void WifiPortal::ssid(char* buffer, size_t bufferSize) const {
  if (bufferSize == 0) {
    return;
  }

  buffer[0] = '\0';

  wifi_config_t config;
  memset(&config, 0, sizeof(config));

  if (esp_wifi_get_config(WIFI_IF_STA, &config) == ESP_OK &&
      config.sta.ssid[0] != '\0') {
    snprintf(buffer, bufferSize, "%.*s", 32,
             reinterpret_cast<const char*>(config.sta.ssid));
    return;
  }

  snprintf(buffer, bufferSize, "%s", "(geen opgeslagen SSID)");
}

void WifiPortal::configureManager() {
  if (_managerConfigured) {
    return;
  }

  buildAccessPointName();

  static const char* menu[] = {"custom", "info"};

  _manager.setDebugOutput(false);
  _manager.setTitle(WIFI_PORTAL_TITLE);
  _manager.setCustomHeadElement(WIFI_PORTAL_CUSTOM_HEAD);
  _manager.setHostname(_accessPointName);
  _manager.setShowInfoUpdate(false);
  _manager.setMenu(menu, 2);
  _manager.setCustomMenuHTML("<p><a href='/stabielator'>Stabielator bedienen en instellen</a></p>");
  _manager.setWebServerCallback([this]() {
    if (_manager.server) webStatusServer.attach(*_manager.server, false);
  });
  _manager.setWiFiAPChannel(WIFI_AP_CHANNEL);
  _manager.setWiFiAPHidden(false);
  _manager.setConfigPortalBlocking(false);
  _manager.setConfigPortalTimeout(0);
  _manager.setDisableConfigPortal(false);
  _manager.setConnectTimeout(WIFI_CONNECT_TIMEOUT_SECONDS);
  _manager.setSaveConnectTimeout(WIFI_CONNECT_TIMEOUT_SECONDS);

  _managerConfigured = true;
}

void WifiPortal::buildAccessPointName() {
  deviceIdentity.begin();
  snprintf(_accessPointName, sizeof(_accessPointName), "%s-%s",
           WIFI_AP_NAME_PREFIX, deviceIdentity.serialNumber());
}

void WifiPortal::applyStationHostname() {
  if (_accessPointName[0] == '\0') {
    buildAccessPointName();
  }

  if (!WiFi.setHostname(_accessPointName)) {
    Serial.println(F("Let op: WiFi hostname kon niet ingesteld worden."));
  }
}

void WifiPortal::configureStationMode() {
  applyStationHostname();
  WiFi.mode(_pairingActive ? WIFI_AP_STA : WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(false);
}

bool WifiPortal::hasStoredCredentials() const {
  wifi_config_t config;
  memset(&config, 0, sizeof(config));

  if (esp_wifi_get_config(WIFI_IF_STA, &config) != ESP_OK) {
    return false;
  }

  return config.sta.ssid[0] != '\0';
}

void WifiPortal::startStationConnect() {
  configureStationMode();
  WiFi.begin();
  _stationConnecting=true; _stationAttemptAt=millis();
  _lastReconnectAttemptMs = millis();
}

void WifiPortal::clearApplicationSettings() {
  Preferences preferences;
  if (!preferences.begin(APP_PREFERENCES_NAMESPACE, false)) {
    Serial.println(F("Let op: Preferences namespace kon niet worden geopend."));
    return;
  }

  preferences.clear();
  preferences.end();
}
