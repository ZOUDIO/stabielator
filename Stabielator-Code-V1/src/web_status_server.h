/**
 * @file web_status_server.h
 * @brief Interface voor de lokale webserver en de gedeelde captive-portalroutes.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#pragma once
#include <WebServer.h>
class WebStatusServer {
 public:
  void begin();
  void attach(WebServer& server, bool captive);
  void update(bool connected, bool pairing);
  String statusJson() const;
 private:
  WebServer _server{80};
  bool _active = false;
};
extern WebStatusServer webStatusServer;
