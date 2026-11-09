/**
 * @file firmware_update.h
 * @brief Toestandsbeheer voor begrensde firmware-uploads naar een vrije OTA-partitie.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#pragma once
#include <WebServer.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>

class FirmwareUpdate {
 public:
  void attach(WebServer& server);
  void update();
  bool busy() const { return _receiving || _restartAt; }
 private:
  bool local(WebServer& server) const;
  void upload(WebServer& server);
  void fail(const String& message);
  bool _receiving=false, _ended=false, _safe=false;
  uint32_t _restartAt=0, _lastData=0;
  size_t _prefixSize=0, _received=0, _expected=0;
  uint8_t _prefix[152]={}, _digest[32]={};
  String _token, _error;
  const esp_partition_t* _partition=nullptr;
  esp_ota_handle_t _handle=0;
  mbedtls_sha256_context _sha{};
};
extern FirmwareUpdate firmwareUpdate;
