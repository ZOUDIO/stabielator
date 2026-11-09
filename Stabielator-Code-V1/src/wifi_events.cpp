/**
 * @file wifi_events.cpp
 * @brief Verplaatst WiFi-events via een begrensde queue naar logging in de hoofdloop.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#include "wifi_events.h"
#include <Arduino.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <cstring>

namespace {
struct WifiLogEvent {
  arduino_event_id_t id;
  uint32_t timestampMs;
  uint32_t ip = 0;
  uint16_t reason = 0;
  uint8_t mac[6] = {};
  char ssid[33] = {};
};
QueueHandle_t eventQueue = nullptr;

const char* reasonText(uint16_t reason) {
  switch (reason) {
    case WIFI_REASON_AUTH_FAIL: return "authenticatie mislukt; controleer wachtwoord/beveiliging";
    case WIFI_REASON_NO_AP_FOUND: return "router/netwerk niet gevonden";
    case WIFI_REASON_HANDSHAKE_TIMEOUT: return "handshake timeout; controleer wachtwoord/signaal";
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT: return "4-way handshake timeout";
    case WIFI_REASON_BEACON_TIMEOUT: return "signaal van router verloren";
    case WIFI_REASON_ASSOC_LEAVE: return "verbinding verlaten (ook bij moduswissel)";
    default: return "zie ESP32 WiFi-reasoncode";
  }
}
void captureEvent(arduino_event_id_t id, arduino_event_info_t info) {
  // Callback draait in een andere task. Alleen kopieren; Serial en WiFi-API in loop().
  WifiLogEvent item = {};
  item.id = id;
  item.timestampMs = millis();
  switch (id) {
    case ARDUINO_EVENT_WIFI_AP_START:
    case ARDUINO_EVENT_WIFI_AP_STOP:
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      memcpy(item.mac, info.wifi_ap_staconnected.mac, sizeof(item.mac));
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      memcpy(item.mac, info.wifi_ap_stadisconnected.mac, sizeof(item.mac));
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED: {
      const size_t length = info.wifi_sta_connected.ssid_len < 32 ? info.wifi_sta_connected.ssid_len : 32;
      memcpy(item.ssid, info.wifi_sta_connected.ssid, length);
      break;
    }
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      item.ip = info.got_ip.ip_info.ip.addr;
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      item.reason = info.wifi_sta_disconnected.reason;
      break;
    default: return;
  }
  xQueueSend(eventQueue, &item, 0); // Niet blokkeren op logging bij een volle queue.
}
}

void beginWifiEventLogging() {
  if (eventQueue) return;
  eventQueue = xQueueCreate(24, sizeof(WifiLogEvent));
  if (!eventQueue) {
    Serial.println("[WiFi] Eventlog kon niet worden gestart (geen queuegeheugen).");
    return;
  }
  WiFi.onEvent(captureEvent);
}
void processWifiEventLog() {
  if (!eventQueue) return;
  WifiLogEvent item;
  // Begrens de logging zodat motor- en seriele afhandeling tijd houden.
  for (unsigned budget = 0; budget < 24 && xQueueReceive(eventQueue, &item, 0) == pdTRUE; ++budget) {
    Serial.printf("[WiFi %lu ms] ", static_cast<unsigned long>(item.timestampMs));
    switch (item.id) {
      case ARDUINO_EVENT_WIFI_AP_START:
        Serial.println("AP gestart."); break;
      case ARDUINO_EVENT_WIFI_AP_STOP:
        Serial.println("AP gestopt."); break;
      case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
        Serial.printf("AP %s: client %02X:%02X:%02X:%02X:%02X:%02X\n",
                      item.id == ARDUINO_EVENT_WIFI_AP_STACONNECTED ? "join" : "leave",
                      item.mac[0], item.mac[1], item.mac[2], item.mac[3], item.mac[4], item.mac[5]);
        break;
      case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        Serial.printf("Router join: %s; wachten op IP-adres.\n", item.ssid); break;
      case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
        const String ip = IPAddress(item.ip).toString();
        Serial.printf("WiFi verbonden; IP-adres: %s\n", ip.c_str());
        Serial.printf("[WiFi] Lokale statuspagina: http://%s/ (op hetzelfde netwerk).\n", ip.c_str());
        break;
      }
      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.printf("Routerverbinding verbroken/mislukt: reason=%u (%s).\n",
                      item.reason, reasonText(item.reason)); break;
      case ARDUINO_EVENT_WIFI_STA_LOST_IP:
        Serial.println("IP-adres verloren."); break;
      default: break;
    }
  }
}
