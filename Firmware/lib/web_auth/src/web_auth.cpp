#include "web_auth.h"

#include "file_system/src/file_system.h"

const char *kWebAuthPath = "/webauth.txt";

typedef struct {
    uint8_t version;
    uint8_t count;
    WebAuthUser users[kMaxWebUsers];
} WebAuthConfig;

bool IsValidUsername(const String &username) {
    if (username.length() == 0 || username.length() > kMaxWebUsernameLength) return false;

    for (uint8_t i = 0; i < username.length(); i++) {
        const char c = username[i];
        if (!isalnum(c) && c != '_' && c != '-' && c != '.') return false;
    }
    return true;
}

bool IsValidPassword(const String &password) {
    return password.length() > 0 && password.length() <= kMaxWebPasswordLength;
}

bool LoadWebAuthUsers(WebAuthUser *users, uint8_t *count, const char *fallback_username,
                      const String &fallback_password) {
    WebAuthConfig config = {};
    if (ReadSettings(kWebAuthPath, reinterpret_cast<byte *>(&config), sizeof(config)) && config.version == 1 &&
        config.count <= kMaxWebUsers) {
        *count = config.count;
        memcpy(users, config.users, sizeof(config.users));
        return true;
    }

    *count = 1;
    memset(users, 0, sizeof(WebAuthUser) * kMaxWebUsers);
    strlcpy(users[0].username, fallback_username, sizeof(users[0].username));
    fallback_password.toCharArray(users[0].password, sizeof(users[0].password));
    return SaveWebAuthUsers(users, *count);
}

bool SaveWebAuthUsers(const WebAuthUser *users, uint8_t count) {
    WebAuthConfig config = {};
    config.version = 1;
    config.count = min(count, kMaxWebUsers);
    memcpy(config.users, users, sizeof(config.users));
    return WriteSettings(kWebAuthPath, reinterpret_cast<byte *>(&config), sizeof(config));
}

bool AuthenticateWebUser(AsyncWebServerRequest *request, const WebAuthUser *users, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        if (users[i].username[0] != '\0' && request->authenticate(users[i].username, users[i].password)) {
            return true;
        }
    }
    return false;
}

int FindWebAuthUser(const WebAuthUser *users, uint8_t count, const String &username) {
    for (uint8_t i = 0; i < count; i++) {
        if (username == users[i].username) return i;
    }
    return -1;
}

bool UpsertWebAuthUser(WebAuthUser *users, uint8_t *count, const String &username, const String &password) {
    if (!IsValidUsername(username) || !IsValidPassword(password)) return false;

    int index = FindWebAuthUser(users, *count, username);
    if (index < 0) {
        if (*count >= kMaxWebUsers) return false;
        index = *count;
        (*count)++;
    }

    username.toCharArray(users[index].username, sizeof(users[index].username));
    password.toCharArray(users[index].password, sizeof(users[index].password));
    return true;
}

bool DeleteWebAuthUser(WebAuthUser *users, uint8_t *count, const String &username) {
    int index = FindWebAuthUser(users, *count, username);
    if (index < 0 || *count <= 1) return false;

    for (uint8_t i = index; i + 1 < *count; i++) {
        users[i] = users[i + 1];
    }
    memset(&users[*count - 1], 0, sizeof(WebAuthUser));
    (*count)--;
    return true;
}
