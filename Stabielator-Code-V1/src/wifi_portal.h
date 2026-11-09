/**
 * @file wifi_portal.h
 * @brief Interface voor WiFi-instellingen, hersteltoegang en fabrieksherstel.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include "config.h"

/**
 * @class WifiPortal
 * @brief Beheert WiFi-credentials, reconnects, captive portal en factory reset.
 */
class WifiPortal {
 public:
  /**
   * @brief Initialiseert WiFi en start zo nodig de captive portal.
   */
  void begin();

  /**
   * @brief Verwerkt captive portal en WiFi reconnects niet-blokkerend.
   */
  void update();

  /**
   * @brief Start pairing mode met captive portal.
   */
  void startPairing();
  bool setAccessPointEnabled(bool enabled);
  bool accessPointEnabled() const { return _apEnabled; }
  bool accessPointPending() const { return _apChangePending; }
  void printDiagnostics() const;

  /**
   * @brief Slaat WiFi-credentials op en start een reconnect.
   * @param ssid SSID van het WiFi-netwerk.
   * @param password Wachtwoord, of lege string voor open netwerk.
   * @return True wanneer de invoer geldig was en de connectie gestart is.
   */
  bool saveStationCredentialsAndReconnect(const char* ssid,
                                          const char* password);
  bool queueStationCredentials(const char* ssid, const char* password);
  bool credentialsPending() const { return _credentialsPending || _stationConnecting; }
  bool isAccessPointHidden() const;

  /**
   * @brief Wist WiFi- en applicatie-instellingen en herstart de ESP32.
   */
  void factoryResetAndReboot();

  /**
   * @brief Geeft terug of WiFi verbonden is.
   */
  bool isConnected() const;

  /**
   * @brief Geeft terug of pairing/captive portal actief is.
   */
  bool isPairingActive() const;

  /**
   * @brief Geeft RSSI terug wanneer WiFi verbonden is.
   * @return RSSI in dBm, of 0 wanneer WiFi niet verbonden is.
   */
  int32_t rssi() const;

  /**
   * @brief Geeft een korte tekstuele WiFi-status terug.
   */
  const char* statusText() const;

  /**
   * @brief Geeft de unieke accesspointnaam voor de captive portal terug.
   */
  const char* accessPointName() const;

  /**
   * @brief Schrijft de opgeslagen SSID naar een caller-owned buffer.
   */
  void ssid(char* buffer, size_t bufferSize) const;

 private:
  void startAccessPoint();
  void stopAccessPoint();
  void configureManager();
  void buildAccessPointName();
  void applyStationHostname();
  void configureStationMode();
  bool hasStoredCredentials() const;
  void startStationConnect();
  void clearApplicationSettings();

  WiFiManager _manager;
  char _accessPointName[WIFI_AP_NAME_BUFFER_SIZE] = "";
  bool _managerConfigured = false;
  bool _pairingActive = false;
  bool _apEnabled = true, _apChangePending = false;
  uint32_t _apChangeAt = 0, _lastApAttemptMs = 0;
  uint32_t _lastReconnectAttemptMs = 0;
  bool _credentialsPending = false;
  bool _stationWanted = false, _stationConnecting = false;
  uint32_t _stationAttemptAt = 0;
  uint32_t _credentialsQueuedAt = 0;
  char _pendingSsid[WIFI_SSID_MAX_LENGTH + 1] = "";
  char _pendingPassword[WIFI_PASSWORD_MAX_LENGTH + 1] = "";
};

/**
 * @brief Globale WiFi-portaalinstantie.
 */
extern WifiPortal wifiPortal;
