/**
 * @file web_status_server.cpp
 * @brief HTTP-routes voor instellingen, live status, GUI-tests, netwerk en OTA.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#include "web_status_server.h"
#include <cstdlib>
#include <cmath>
#include <limits>
#include <type_traits>
#include "config.h"
#include "device_identity.h"
#include "orientation.h"
#include "stabilizer_control.h"
#include "wifi_portal.h"
#include "web_ui.h"
#include "firmware_update.h"
#include "orientation_viewer.h"
#include "gpio_control.h"
WebStatusServer webStatusServer;
namespace {
String jsonString(const String& value) {
  String result = "\"";
  for (size_t n=0;n<value.length();++n) {
    const unsigned char c=value[n];
    if(c=='"'||c=='\\'){result+='\\';result+=char(c);}
    else if(c<32){char escaped[7];snprintf(escaped,sizeof(escaped),"\\u%04x",unsigned(c));result+=escaped;}
    else result+=char(c);
  }
  return result+'"';
}
String anglesJson(float roll, float pitch, float yaw) {
  if (!std::isfinite(roll) || !std::isfinite(pitch) || !std::isfinite(yaw)) return "null";
  return "{\"roll\":" + String(roll,1) + ",\"pitch\":" + String(pitch,1) + ",\"yaw\":" + String(yaw,1) + "}";
}
String settingsJson(const StabilizerSettings& s) {
  String json = "{";
#define FIELD(name) json += "\"" #name "\":" + String(s.name) + ",";
  FIELD(onDegrees) FIELD(offDegrees) FIELD(strength) FIELD(pattern) FIELD(inactivityMinutes) FIELD(activityDegrees)
  FIELD(ledEnabled) FIELD(ledBrightness) FIELD(bootDelayMs) FIELD(calibrationMs) FIELD(diagonal) FIELD(pulseOnMs)
  FIELD(pulseOffMs) FIELD(maxFeedbackMs) FIELD(maxContinuousOrDoubleMs) FIELD(axisRotationDegrees) FIELD(batteryLowVolts)
  FIELD(batteryCriticalVolts) FIELD(debugEnabled) FIELD(testMode)
#undef FIELD
  for (unsigned m=0; m<4; ++m) json += "\"motor" + String(m) + "\":" + String(s.motorMap[m]) + (m<3 ? "," : "}");
  return json;
}
bool parseNumber(WebServer& server, const char* key, double& value) {
  const String text = server.arg(key);
  if (text.isEmpty() || text.length()>24) return false;
  char* end = nullptr;
  value = std::strtod(text.c_str(), &end);
  return end != text.c_str() && *end == '\0' && std::isfinite(value);
}
template<typename T> bool readField(WebServer& server, const char* key, T& field) {
  if (!server.hasArg(key)) return true;
  double value;
  if (!parseNumber(server,key,value)) return false;
  if (std::is_integral<T>::value && (value<0 || value>double(std::numeric_limits<T>::max()) || std::floor(value)!=value)) return false;
  field = static_cast<T>(value);
  return true;
}
}
String WebStatusServer::statusJson() const {
  const auto s = stabilizerControl.snapshot();
  const auto& r = s.imu;
  String json = "{\"name\":\"Stabielator\",\"version\":\"" + String(APP_VERSION) +
    "\",\"device\":\"" + String(deviceIdentity.serialNumber()) + "\",\"uptime_s\":" + String(millis()/1000);
  json += ",\"wifi_connected\":" + String(wifiPortal.isConnected()?"true":"false");
  json += ",\"wifi_station_ip\":\"" + WiFi.localIP().toString() + "\"";
  const auto partition=esp_ota_get_running_partition();
  json += ",\"firmware_target\":\""+String(OTA_TARGET)+"\",\"firmware_partition\":\""+String(partition?partition->label:"")+"\",\"firmware_update_active\":"+String(firmwareUpdate.busy()?"true":"false");
  json += ",\"debug_enabled\":" + String(s.settings.debugEnabled?"true":"false");
  json += ",\"state\":\"" + String(stateName(s.state)) + "\"";
  json += ",\"paused\":"+String(s.userPaused?"true":"false")+",\"pause_saved\":"+String(s.pauseStorageHealthy?"true":"false");
  const auto led=gpioControl.ledColor();
  json += ",\"led_rgb\":["+String(led.red)+","+String(led.green)+","+String(led.blue)+"]";
  json += ",\"manual_test\":" + String(s.manualTest?"true":"false");
  json += ",\"test_mode\":" + String(s.testMode?"true":"false");
  json += ",\"feedback_cue\":" + String(s.feedbackCue?"true":"false");
  json += ",\"inactivity_ms\":" + String(s.inactivityMs);
  json += ",\"settings_storage_ok\":" + String(s.storageHealthy?"true":"false");
  json += ",\"imu_hardware_ready\":" + String(s.imuReady?"true":"false");
  json += ",\"imu_healthy\":" + String(s.imuHealthy?"true":"false");
  json += ",\"imu_sample_age_ms\":" + (r.received?String(millis()-r.timestampMs):String("null"));
  json += ",\"quaternion\":";
  if (s.imuHealthy) json += "{\"w\":"+String(r.real,6)+",\"x\":"+String(r.i,6)+",\"y\":"+String(r.j,6)+",\"z\":"+String(r.k,6)+"}";
  else json += "null";
  const auto a = quaternionToDegrees(r.real,r.i,r.j,r.k);
  json += ",\"orientation_deg\":" + (s.imuHealthy?anglesJson(a.roll,a.pitch,a.yaw):String("null"));
  json += ",\"reference_valid\":" + String(s.referenceValid?"true":"false");
  json += ",\"calibration_samples\":" + String(s.calibrationSamples);
  json += ",\"relative_orientation_deg\":" + (s.referenceValid&&s.imuHealthy?anglesJson(s.roll,s.pitch,s.yaw):String("null"));
  json += ",\"tilt_deg\":";
  if (s.referenceValid&&s.imuHealthy) json += "{\"roll\":"+String(s.tiltRoll,1)+",\"pitch\":"+String(s.tiltPitch,1)+"}";
  else json += "null";
  json += ",\"haptics_enabled\":" + String(HAPTIC_PROFILE_CONFIGURED?"true":"false");
  json += ",\"motor_stop_failed\":" + String(s.motorStopFailed?"true":"false");
  json += ",\"haptic_type\":\"" + String(HAPTIC_IS_LRA?"LRA":"ERM") + "\"";
  json += ",\"haptic_motor_model\":\"" + String(HAPTIC_MOTOR_MODEL) + "\"";
  json += ",\"haptic_control_mode\":\"" + String(HAPTIC_IS_LRA?"closed_loop":"open_loop") + "\"";
  json += ",\"haptic_frequency_hz\":null,\"haptic_frequency_adjustable\":false";
  json += ",\"motors_present\":[";
  for(unsigned m=0;m<4;++m){if(m)json+=',';json+=s.present[m]?"true":"false";}
  json += "],\"motors_hardware_present\":[";
  for(unsigned m=0;m<4;++m){if(m)json+=',';json+=s.hardwarePresent[m]?"true":"false";}
  json += "],\"motor_levels\":[";
  for(unsigned m=0;m<4;++m){if(m)json+=',';json+=String(s.levels[m]);}
  json += "],\"power\":{\"battery_v\":" + (s.power.available?String(s.power.volts,2):String("null"));
  json += ",\"estimated_percent\":" + (s.power.available?String(s.power.estimatedPercent()):String("null"));
  json += ",\"low\":" + String(s.power.low?"true":"false") + ",\"critical\":" + String(s.power.critical?"true":"false");
  json += ",\"shutting_down\":" + String(s.power.shuttingDown?"true":"false") + "}}";
  return json;
}
void WebStatusServer::attach(WebServer& server, bool captive) {
  firmwareUpdate.attach(server);
  server.on("/orientation.js",HTTP_GET,[&server]() {
    server.sendHeader("Cache-Control","no-store");
    server.send_P(200,"application/javascript; charset=utf-8",ORIENTATION_VIEWER_SCRIPT);
  });
  server.on("/api/orientation",HTTP_GET,[&server]() {
    const auto s=stabilizerControl.orientationSnapshot();
    char absolute[80]="null",relative[80]="null",json[420];
    if (s.valid) snprintf(absolute,sizeof(absolute),"[%.6f,%.6f,%.6f,%.6f]",s.absolute.w,s.absolute.x,s.absolute.y,s.absolute.z);
    if (s.referenceValid) snprintf(relative,sizeof(relative),"[%.6f,%.6f,%.6f,%.6f]",s.relative.w,s.relative.x,s.relative.y,s.relative.z);
    snprintf(json,sizeof(json),"{\"debug\":%s,\"valid\":%s,\"seq\":%lu,\"generation\":%lu,\"sample_ms\":%lu,\"device_ms\":%lu,\"age_ms\":%lu,\"q\":%s,\"relative\":%s}",
             s.debugEnabled?"true":"false",s.valid?"true":"false",
             (unsigned long)s.sequence,(unsigned long)s.generation,(unsigned long)s.sampleMs,
             (unsigned long)s.nowMs,(unsigned long)(s.nowMs-s.sampleMs),absolute,relative);
    server.sendHeader("Cache-Control","no-store");
    server.send(200,"application/json",json);
  });
  auto page = [&server]() { server.sendHeader("Cache-Control","no-store"); server.send(200,"text/html; charset=utf-8",STABILIZER_PAGE); };
  if (!captive) server.on("/",HTTP_GET,page);
  server.on("/basis",HTTP_GET,page);
  server.on("/stabielator",HTTP_GET,page);
  server.on("/advanced",HTTP_GET,page);
  server.on("/wifi",HTTP_GET,[&server]() { server.sendHeader("Location","/advanced#wifiAdvanced"); server.send(302,"text/plain",""); });
  server.on("/api/wifi",HTTP_GET,[&server]() {
    char ssid[WIFI_SSID_MAX_LENGTH+1];wifiPortal.ssid(ssid,sizeof(ssid));
    if(String(ssid)=="(geen opgeslagen SSID)")ssid[0]='\0';
    String json="{\"ssid\":"+jsonString(ssid);
    json+=",\"connected\":"+String(wifiPortal.isConnected()?"true":"false");
    json+=",\"station_ip\":"+jsonString(WiFi.localIP().toString());
    json+=",\"rssi\":"+String(wifiPortal.rssi());
    json+=",\"ap_active\":"+String(wifiPortal.isPairingActive()?"true":"false");
    json+=",\"ap_enabled\":"+String(wifiPortal.accessPointEnabled()?"true":"false");
    json+=",\"ap_pending\":"+String(wifiPortal.accessPointPending()?"true":"false");
    json+=",\"ap_ssid\":"+jsonString(wifiPortal.accessPointName());
    json+=",\"ap_ip\":"+jsonString(WiFi.softAPIP().toString());
    json+=",\"ap_hidden\":"+String(wifiPortal.isAccessPointHidden()?"true":"false");
    json+=",\"ap_clients\":"+String(WiFi.softAPgetStationNum());
    json+=",\"pending\":"+String(wifiPortal.credentialsPending()?"true":"false")+"}";
    server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",json);
  });
  server.on("/api/wifi",HTTP_POST,[&server]() {
    if(firmwareUpdate.busy()){server.send(409,"text/plain","Firmware-update actief.");return;}
    if(!server.hasArg("ssid")||!server.hasArg("password")||
       !wifiPortal.queueStationCredentials(server.arg("ssid").c_str(),server.arg("password").c_str())) {
      server.send(400,"text/plain; charset=utf-8","Gebruik een netwerknaam van 1-32 bytes en een wachtwoord van 8-63 tekens, of leeg voor een open netwerk.");return;
    }
    server.send(202,"application/json","{\"ok\":true,\"reconnecting\":true}");
  });
  server.on("/api/ap",HTTP_POST,[&server]() {
    if(firmwareUpdate.busy()){server.send(409,"text/plain","Firmware-update actief.");return;}
    const String value=server.arg("enabled");
    if(value!="0"&&value!="1"){server.send(400,"text/plain","Kies AP aan (1) of uit (0).");return;}
    if(!wifiPortal.setAccessPointEnabled(value=="1")){server.send(500,"text/plain","AP-keuze opslaan mislukt; niet gewijzigd.");return;}
    server.send(202,"application/json","{\"ok\":true}");
  });
  server.on("/api/status",HTTP_GET,[this,&server]() {
    server.sendHeader("Cache-Control","no-store"); server.send(200,"application/json",statusJson());
  });
  server.on("/api/settings",HTTP_GET,[&server]() {
    server.sendHeader("Cache-Control","no-store"); server.send(200,"application/json",settingsJson(stabilizerControl.snapshot().settings));
  });
  server.on("/api/settings",HTTP_POST,[&server]() {
    if(firmwareUpdate.busy()){server.send(409,"text/plain","Firmware-update actief.");return;}
    auto s = stabilizerControl.snapshot().settings;
    bool ok = server.args()>0 && !server.hasArg("sessionMinutes") && !server.hasArg("frequencyHz");
#define READ(name) ok = readField(server,#name,s.name) && ok;
    READ(onDegrees) READ(offDegrees) READ(strength) READ(pattern) READ(inactivityMinutes) READ(activityDegrees) READ(ledEnabled) READ(ledBrightness)
    READ(bootDelayMs) READ(calibrationMs) READ(diagonal) READ(pulseOnMs) READ(pulseOffMs)
    READ(maxFeedbackMs) READ(maxContinuousOrDoubleMs) READ(axisRotationDegrees) READ(batteryLowVolts) READ(batteryCriticalVolts) READ(debugEnabled) READ(testMode)
#undef READ
    for(unsigned m=0;m<4;++m)ok=readField(server,("motor"+String(m)).c_str(),s.motorMap[m])&&ok;
    if(!ok||!validSettings(s)){server.send(400,"text/plain; charset=utf-8","Ongeldige instellingen: uitschakelhoek moet lager zijn dan inschakelhoek; elke motor moet uniek zijn; controleer alle bereiken.");return;}
    if(!stabilizerControl.saveSettings(s)){server.send(500,"text/plain","Opslaan in permanent geheugen mislukt.");return;}
    server.send(200,"application/json",settingsJson(s));
  });
  server.on("/api/control",HTTP_POST,[&server]() {
    if(firmwareUpdate.busy()){server.send(409,"text/plain","Firmware-update actief.");return;}
    const String action=server.arg("action");
    if(action=="stop") {
      if(!stabilizerControl.pause(true)){server.send(500,"text/plain; charset=utf-8","Motoren gepauzeerd, maar opslaan mislukt. Pauze na herstart niet gegarandeerd; probeer opnieuw.");return;}
    }
    else if(action=="calibrate")stabilizerControl.calibrate();
    else if(action=="resume") {
      if(!stabilizerControl.pause(false)){server.send(500,"text/plain; charset=utf-8","Hervatten niet opgeslagen; apparaat blijft gepauzeerd. Probeer opnieuw.");return;}
    }
    else if(action=="test_mode_on"||action=="test_mode_off") {
      if(!stabilizerControl.setTestMode(action=="test_mode_on")){server.send(500,"text/plain","Ontwikkeltest opslaan mislukt; stand niet gewijzigd.");return;}
    }
    else{server.send(400,"text/plain","Onbekende opdracht.");return;}
    server.send(200,"application/json","{\"ok\":true}");
  });
  server.on("/api/test",HTTP_POST,[&server]() {
    if(firmwareUpdate.busy()){server.send(409,"text/plain","Firmware-update actief.");return;}
    unsigned motor=0,duration=0;
    bool ok=readField(server,"motor",motor)&&readField(server,"duration",duration);
    if(!ok||motor<1||motor>4||duration<20||duration>3000){server.send(400,"text/plain","Test: motor 1-4, duur 20-3000 ms.");return;}
    const auto settings=stabilizerControl.snapshot().settings;
    if(!stabilizerControl.testPulse(motor-1,settings.strength,duration,true)){server.send(409,"text/plain","Geen test mogelijk: controleer IMU, voeding en motorprofiel.");return;}
    server.send(200,"application/json","{\"ok\":true}");
  });
}
void WebStatusServer::begin() {
  attach(_server,false);
  _server.onNotFound([this]() {_server.send(404,"text/plain","Niet gevonden");});
}
void WebStatusServer::update(bool connected, bool pairing) {
  const bool wanted=connected&&!pairing;
  if(_active&&!wanted){_server.stop();_active=false;}
  if(!_active&&wanted){_server.begin();_active=true;}
  if(_active)_server.handleClient();
}
