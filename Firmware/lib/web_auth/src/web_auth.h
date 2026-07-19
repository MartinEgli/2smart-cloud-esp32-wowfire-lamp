#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

const uint8_t kMaxWebUsers = 3;
const uint8_t kMaxWebUsernameLength = 16;
const uint8_t kMaxWebPasswordLength = 31;

typedef struct {
    char username[kMaxWebUsernameLength + 1];
    char password[kMaxWebPasswordLength + 1];
} WebAuthUser;

bool LoadWebAuthUsers(WebAuthUser *users, uint8_t *count, const char *fallback_username,
                      const String &fallback_password);
bool SaveWebAuthUsers(const WebAuthUser *users, uint8_t count);
bool AuthenticateWebUser(AsyncWebServerRequest *request, const WebAuthUser *users, uint8_t count);
int FindWebAuthUser(const WebAuthUser *users, uint8_t count, const String &username);
bool UpsertWebAuthUser(WebAuthUser *users, uint8_t *count, const String &username, const String &password);
bool DeleteWebAuthUser(WebAuthUser *users, uint8_t *count, const String &username);
