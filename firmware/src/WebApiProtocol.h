#ifndef WEB_API_PROTOCOL_H
#define WEB_API_PROTOCOL_H

#include <Arduino.h>
#include <WebServer.h>

String buildWebStatusJson(bool includeHistory = false);
String buildWebLogsJson();
String buildWebActionResponseJson(bool success, const char *code,
                                  const char *message = nullptr);
void sendWebActionResponse(WebServer &server, int httpStatus, bool success,
                           const char *code, const char *message = nullptr);

#endif // WEB_API_PROTOCOL_H
