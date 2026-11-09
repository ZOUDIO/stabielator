/**
 * @file firmware_update.cpp
 * @brief Ontvangt en valideert lokale OTA-updates met motorblokkering en SHA-256-controle.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#include "firmware_update.h"
#include "config.h"
#include "stabilizer_control.h"
#include "wifi_portal.h"
#include "firmware_update_ui.h"
#include <WiFi.h>
#include <esp_random.h>
#include <algorithm>
#include <cstring>

FirmwareUpdate firmwareUpdate;
/** @brief Accepteert alleen de eigen AP-interface met bijpassend hostadres en clientsubnet. */
bool FirmwareUpdate::local(WebServer& s) const {
  const auto ap=WiFi.softAPIP(), remote=s.client().remoteIP();
  // Alleen eigen AP, ook wanneer AP en router hetzelfde subnet gebruiken.
  return uint32_t(ap) && s.client().localIP()==ap && remote!=ap &&
    (uint32_t(remote)&uint32_t(WiFi.softAPSubnetMask()))==(uint32_t(ap)&uint32_t(WiFi.softAPSubnetMask())) &&
    (s.hostHeader()==ap.toString() || s.hostHeader()==ap.toString()+":80");
}
void FirmwareUpdate::fail(const String& message) {
  if (_handle) { esp_ota_abort(_handle); _handle=0; }
  mbedtls_sha256_free(&_sha);
  _error=message; _receiving=false; _ended=false;
  if (_safe) { stabilizerControl.endFirmwareUpdate(); _safe=false; }
  Serial.println("[OTA] "+message);
}
void FirmwareUpdate::attach(WebServer& s) {
  if (_token.isEmpty()) { char token[33]; snprintf(token,sizeof(token),"%08lx%08lx%08lx%08lx",(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random()); _token=token; }
  const char* headers[]={"X-Stabielator-Token","Origin"};
  s.collectHeaders(headers,2);
  s.on("/update",HTTP_GET,[this,&s]() {
    if(!local(s)){s.send(403,"text/plain","Verbind met het Stabielator-instelnetwerk en open http://192.168.4.1/update");return;}
    s.sendHeader("Cache-Control","no-store");
    s.sendHeader("Content-Security-Policy","default-src 'self'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; connect-src 'self'; frame-ancestors 'none'");
    s.send(200,"text/html; charset=utf-8",FIRMWARE_UPDATE_PAGE);
  });
  s.on("/api/update",HTTP_GET,[this,&s]() {
    if(!local(s)){s.send(403,"text/plain","Firmware bijwerken kan alleen via het eigen instelnetwerk.");return;}
    const auto p=esp_ota_get_next_update_partition(nullptr);
    s.sendHeader("Cache-Control","no-store");
    s.send(200,"application/json","{\"version\":\""+String(APP_VERSION)+"\",\"target\":\""+String(OTA_TARGET)+"\",\"capacity\":"+String(p?p->size:0)+",\"busy\":"+String(busy()?"true":"false")+",\"token\":\""+_token+"\"}");
  });
  s.on("/api/update",HTTP_POST,[this,&s]() {
    // Pas na volledig geparseerd HTTP-verzoek valideren en activeren.
    if (!local(s) || s.header("X-Stabielator-Token")!=_token) {s.send(403,"text/plain","Geen toegang tot firmware-update.");return;}
    if (_restartAt) {s.send(409,"text/plain","Update al geslaagd; herstart volgt.");return;}
    if (!_receiving || !_ended || !_handle) {s.send(400,"text/plain; charset=utf-8",_error.isEmpty()?"Geen volledig firmwarebestand ontvangen.":_error);return;}
    uint8_t digest[32];
    mbedtls_sha256_finish(&_sha,digest); mbedtls_sha256_free(&_sha);
    if(memcmp(digest,_digest,32)){fail("Bestandscontrole mislukt (SHA-256). Upload het originele updatebestand opnieuw.");s.send(400,"text/plain; charset=utf-8",_error);return;}
    esp_err_t result=esp_ota_end(_handle); _handle=0;
    if(result!=ESP_OK){fail("Firmware ongeldig of incompatibel: "+String(esp_err_to_name(result)));s.send(400,"text/plain",_error);return;}
    result=esp_ota_set_boot_partition(_partition);
    if(result!=ESP_OK){fail("Activeren mislukt: "+String(esp_err_to_name(result)));s.send(500,"text/plain",_error);return;}
    Serial.printf("[OTA] Succes: %u bytes naar %s; herstart volgt.\n",unsigned(_received),_partition->label);
    s.sendHeader("Connection","close");
    s.send(200,"application/json","{\"ok\":true,\"restarting\":true}");
    // De wachttijd begint na het antwoord; USB-logging kan hiervoor vertragen.
    _receiving=false; _restartAt=millis()+1500;
  },[this,&s](){upload(s);});
  // Overschrijf ook de ingebouwde, onbeveiligde WiFiManager-uploadroute.
  s.on("/u",HTTP_ANY,[&s](){s.send(403,"text/plain","Gebruik /update op het instelnetwerk.");},[](){});
}
/**
 * @brief Verwerkt een multipart-upload zonder de huidige bootpartitie te overschrijven.
 * @details De 128-byte envelop bevat magic, hardwaredoel, versie, lengte en SHA-256.
 * De eerste 24 applicatiebytes worden vooraf op ESP32-C3-herkenning gecontroleerd.
 * Activeren gebeurt pas na een volledig verzoek, lengtecontrole en overeenkomende hash.
 * SHA-256 controleert integriteit; deze envelop bevat geen digitale handtekening.
 */
void FirmwareUpdate::upload(WebServer& s) {
  auto& u=s.upload();
  if(u.status==UPLOAD_FILE_START){
    if(busy()){if(_receiving)fail("Meerdere bestanden/updates tegelijk zijn niet toegestaan.");return;}
    _error=""; _ended=false; _prefixSize=0; _received=0; _expected=0;
    if(!local(s) || s.header("X-Stabielator-Token")!=_token ||
       (s.hasHeader("Origin") && s.header("Origin")!="http://"+s.hostHeader())) { _error="Geen toegang: gebruik de updatepagina op het instelnetwerk."; return; }
    if(u.name!="firmware" || !u.filename.endsWith(".stab.bin")){_error="Kies een Stabielator .stab.bin-updatebestand, geen USB/bootloaderbestand.";return;}
    _partition=esp_ota_get_next_update_partition(nullptr);
    if(!_partition || _partition==esp_ota_get_running_partition()){_error="Geen vrije OTA-partitie. Installeer de OTA-build eerst via USB.";return;}
    _receiving=true; _lastData=millis();
  } else if(u.status==UPLOAD_FILE_WRITE && _receiving){
    if(_ended){fail("Meerdere bestanden zijn niet toegestaan.");return;}
    _lastData=millis(); size_t offset=0;
    if(_prefixSize<sizeof(_prefix)){
      const size_t count=std::min(u.currentSize,sizeof(_prefix)-_prefixSize);
      memcpy(_prefix+_prefixSize,u.buf,count); _prefixSize+=count; offset=count;
      if(_prefixSize<sizeof(_prefix))return;
      memcpy(&_expected,_prefix+72,4); memcpy(_digest,_prefix+76,32);
      if(memcmp(_prefix,"STABOTA1",8) || _prefix[39] || strcmp(reinterpret_cast<char*>(_prefix+8),OTA_TARGET) ||
         !_prefix[40] || !memchr(_prefix+40,0,32) || _prefix[128]!=0xE9 || _prefix[140]!=5 || _prefix[141]!=0){fail("Ongeldig bestand of verkeerde hardwarevariant. Kies de passende Stabielator-update.");return;}
      if(_expected<288 || _expected>_partition->size){fail("Firmware is te groot of heeft een ongeldige lengte.");return;}
      if(!stabilizerControl.beginFirmwareUpdate()){fail("Motorstop of voeding niet veilig; update niet gestart.");return;}
      _safe=true;
      const auto result=esp_ota_begin(_partition,_expected,&_handle);
      if(result!=ESP_OK){_handle=0;fail("OTA starten mislukt: "+String(esp_err_to_name(result)));return;}
      mbedtls_sha256_init(&_sha); mbedtls_sha256_starts(&_sha,0);
      if(esp_ota_write(_handle,_prefix+128,24)!=ESP_OK){fail("Schrijven firmwareheader mislukt.");return;}
      mbedtls_sha256_update(&_sha,_prefix+128,24); _received=24;
      Serial.printf("[OTA] Start naar %s, %u bytes; motoren geblokkeerd.\n",_partition->label,unsigned(_expected));
    }
    const size_t count=u.currentSize-offset;
    if(count>_expected-_received){fail("Bestand bevat meer bytes dan opgegeven.");return;}
    if(count){
      const auto result=esp_ota_write(_handle,u.buf+offset,count);
      if(result!=ESP_OK){fail("Flash schrijven mislukt: "+String(esp_err_to_name(result)));return;}
      mbedtls_sha256_update(&_sha,u.buf+offset,count); _received+=count;
    }
  } else if(u.status==UPLOAD_FILE_END && _receiving){
    if(!_handle || _received!=_expected){fail("Upload onvolledig. De bestaande firmware blijft actief.");return;}
    _ended=true;
  } else if(u.status==UPLOAD_FILE_ABORTED && _receiving) fail("Upload afgebroken. De bestaande firmware blijft actief; motoren blijven gepauzeerd.");
}
void FirmwareUpdate::update() {
  if(_receiving && millis()-_lastData>15000)fail("Upload timeout. De bestaande firmware blijft actief.");
  if(_restartAt && int32_t(millis()-_restartAt)>=0)ESP.restart();
}
