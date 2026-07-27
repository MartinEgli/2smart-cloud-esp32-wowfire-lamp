#include "web_server.h"

#include <ArduinoJson.h>

#include "file_system/src/file_system.h"
#include "gpio.h"
#include "lenta.h"
#include "utils/src/utils.h"

static bool ParseUintInRange(const String &value, uint16_t min_value, uint16_t max_value, uint16_t *result) {
    if (!result || value.length() == 0) return false;
    for (uint16_t i = 0; i < value.length(); i++) {
        if (!isDigit(value[i])) return false;
    }
    uint32_t parsed = value.toInt();
    if (parsed < min_value || parsed > max_value) return false;
    *result = parsed;
    return true;
}

static bool ParseStateValue(const String &value, bool *result) {
    if (!result) return false;
    if (value == "1" || value == "true") {
        *result = true;
        return true;
    }
    if (value == "0" || value == "false") {
        *result = false;
        return true;
    }
    return false;
}

static const char *GetStaticContentType(const String &path) {
    if (path.endsWith(".css")) return "text/css";
    if (path.endsWith(".png")) return "image/png";
    if (path.endsWith(".svg")) return "image/svg+xml";
    if (path.endsWith(".js")) return "application/javascript";
    return "application/octet-stream";
}

WebServer::WebServer(Device *device) {
    device_ = device;
    // cppcheck-suppress noCopyConstructor
    // cppcheck-suppress noOperatorEq
    server_ = new AsyncWebServer(kPort_);
}

void WebServer::Init() {
    LoadWebAuthUsers(auth_users_, &auth_user_count_, http_username, web_auth_password);
    SetupWebServer();
}

String WebServer::FillPlaceholders(const String &var) {
    Serial.println(var);
    if (var == "LOGIN") {
        return person_mail;
    }
    if (var == "TOKEN") {
        return token;
    }
    if (var == "HOSTNAME") {
        return host;
    }
    if (var == "BRPORT") {
        return broker_port;
    }
    if (var == "PRODUCTID") {
        return product_id;
    }
    if (var == "DEVICEID") {
        return device_id;
    }

    if (var == "FIRMWARE") {
        return firmware_name;
    }
    return String();
}

void WebServer::OnRequestWithAuth(AsyncWebServerRequest *request, ArRequestHandlerFunction onRequest) {
    if (!AuthenticateWebUser(request, auth_users_, auth_user_count_)) return request->requestAuthentication();

    onRequest(request);
}

bool WebServer::HasRequestValue(AsyncWebServerRequest *request, const char *name) {
    return request->hasParam(name) || request->hasParam(name, true);
}

String WebServer::GetRequestValue(AsyncWebServerRequest *request, const char *name) {
    if (request->hasParam(name, true)) return request->getParam(name, true)->value();
    if (request->hasParam(name)) return request->getParam(name)->value();
    return "";
}

bool WebServer::TryServeStaticAsset(AsyncWebServerRequest *request) {
    String path = request->url();
    if (path.indexOf("..") >= 0) return false;

    bool is_supported_asset = (path.startsWith("/styles.") && path.endsWith(".css")) ||
                              (path.startsWith("/favicon.") && path.endsWith(".png")) ||
                              (path.startsWith("/logo.") && path.endsWith(".svg")) ||
                              path.endsWith(".js");
    if (!is_supported_asset || !SPIFFS.exists(path)) return false;

    request->send(SPIFFS, path, GetStaticContentType(path));
    return true;
}

bool WebServer::ApplyLentaSettings(AsyncWebServerRequest *request) {
    const char *required_fields[] = {"state", "brightness", "mode", "r", "g", "b"};
    for (uint8_t i = 0; i < 6; i++) {
        if (!HasRequestValue(request, required_fields[i])) {
            request->send(400, "text/plain", "Missing setting");
            return false;
        }
    }

    bool state = false;
    uint16_t brightness = 0;
    uint16_t mode = 0;
    uint16_t red = 0;
    uint16_t green = 0;
    uint16_t blue = 0;

    Lenta *node = static_cast<Lenta *>(device_->GetNode("lenta"));
    if (!node || !ParseStateValue(GetRequestValue(request, "state"), &state) ||
        !ParseUintInRange(GetRequestValue(request, "brightness"), 0, 100, &brightness) ||
        !ParseUintInRange(GetRequestValue(request, "mode"), 0, 255, &mode) || !node->IsValidMode(mode) ||
        !ParseUintInRange(GetRequestValue(request, "r"), 0, 255, &red) ||
        !ParseUintInRange(GetRequestValue(request, "g"), 0, 255, &green) ||
        !ParseUintInRange(GetRequestValue(request, "b"), 0, 255, &blue)) {
        request->send(400, "text/plain", "Invalid setting");
        return false;
    }

    if (HasRequestValue(request, "rotation")) {
        uint16_t rotation = 0;
        if (!ParseUintInRange(GetRequestValue(request, "rotation"), 0, 270, &rotation) ||
            !(rotation == 0 || rotation == 90 || rotation == 180 || rotation == 270)) {
            request->send(400, "text/plain", "Invalid rotation");
            return false;
        }
    }

    if (HasRequestValue(request, "speed")) {
        uint16_t speed = 0;
        if (!ParseUintInRange(GetRequestValue(request, "speed"), 1, 100, &speed)) {
            request->send(400, "text/plain", "Invalid speed");
            return false;
        }
    }

    Property *property = node->GetProperty("brightness");
    property->SetValue(String(brightness));

    node->PublishMode(mode);

    property = node->GetProperty("state");
    property->SetValue(state ? "true" : "false");

    char message_buffer[12];
    snprintf(message_buffer, sizeof(message_buffer), "%d,%d,%d", red, green, blue);
    property = node->GetProperty("color");
    property->SetValue(message_buffer);

    if (HasRequestValue(request, "rotation")) {
        property = node->GetProperty("rotation");
        property->SetValue(GetRequestValue(request, "rotation"));
    }
    if (HasRequestValue(request, "speed")) {
        property = node->GetProperty("speed");
        property->SetValue(GetRequestValue(request, "speed"));
    }
    if (HasRequestValue(request, "text")) {
        property = node->GetProperty("text");
        property->SetValue(GetRequestValue(request, "text"));
    }

    request->send(200, "text/plain", "OK");
    return true;
}

void WebServer::SendLentaSettings(AsyncWebServerRequest *request) {
    Serial.println("in settings");
    Lenta *node = static_cast<Lenta *>(device_->GetNode("lenta"));
    Property *property = node->GetProperty("brightness");
    StaticJsonDocument<4096> doc;

    doc["data"]["brightness"] = property->GetValue().toInt();
    property = node->GetProperty("state");
    doc["data"]["state"] = property->GetValue() == "true";
    property = node->GetProperty("mode");
    doc["data"]["mode"] = property->GetValue();
    property = node->GetProperty("color");
    doc["data"]["color"] = property->GetValue();
    property = node->GetProperty("rotation");
    doc["data"]["rotation"] = property->GetValue().toInt();
    property = node->GetProperty("speed");
    doc["data"]["speed"] = property->GetValue().toInt();
    property = node->GetProperty("text");
    doc["data"]["text"] = property->GetValue();

    doc["data"]["states"] = node->GetModes();
    JsonArray modes = doc["data"].createNestedArray("modes");
    for (const auto &item : node->GetModeMetadata()) {
        JsonObject mode = modes.createNestedObject();
        mode["id"] = item.first;
        mode["name"] = item.second.name;
        mode["usesColor"] = item.second.uses_color;
        mode["usesText"] = item.second.uses_text;
        mode["usesSpeed"] = item.second.uses_speed;
        mode["usesRotation"] = item.second.uses_rotation;
    }

    String response;
    serializeJson(doc, response);
    Serial.println(response);
    request->send(200, "application/json", response);
}

void WebServer::SetupWebServer() {
    server_->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        OnRequestWithAuth(request, [this](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/index.html", String(), false,
                          [this](const String &var) { return FillPlaceholders(var); });
        });
    });

    server_->on("/index.html", HTTP_GET, [this](AsyncWebServerRequest *request) {
        OnRequestWithAuth(request, [this](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/index.html", String(), false,
                          [this](const String &var) { return FillPlaceholders(var); });
        });
    });

    server_->on("/header.html", HTTP_GET, [this](AsyncWebServerRequest *request) {
        OnRequestWithAuth(request, [this](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/header.html", String(), false,
                          [this](const String &var) { return FillPlaceholders(var); });
        });
    });

    server_->on("/wifi.html", HTTP_GET, [this](AsyncWebServerRequest *request) {
        OnRequestWithAuth(request, [this](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/wifi.html", String(), false,
                          [this](const String &var) { return FillPlaceholders(var); });
        });
    });

    server_->on("/settings.html", HTTP_GET, [this](AsyncWebServerRequest *request) {
        OnRequestWithAuth(request, [this](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/settings.html", String(), false,
                          [this](const String &var) { return FillPlaceholders(var); });
        });
    });

    server_->on("/system.html", HTTP_GET, [this](AsyncWebServerRequest *request) {
        OnRequestWithAuth(request, [this](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/system.html", String(), false,
                          [this](const String &var) { return FillPlaceholders(var); });
        });
    });

    server_->on("/healthcheck", HTTP_GET,
                [](AsyncWebServerRequest *request) { request->send(200, "text/html", "OK"); });

    server_->on("/reboot", HTTP_GET, [this](AsyncWebServerRequest *request) {
        OnRequestWithAuth(request, [this](AsyncWebServerRequest *request) {
            request->send(200, "text/plain", "OK");
            delay(kResponseDelay_);
            ESP.restart();
        });
    });

    server_->on("/resetdefault", HTTP_GET, [this](AsyncWebServerRequest *request) {
        OnRequestWithAuth(request, [this](AsyncWebServerRequest *request) {
            if (!EraseFlash()) {
                request->send(500, "text/plain", "Server error");
                return;
            }
            request->send(200, "text/plain", "OK");
            delay(kResponseDelay_);
            ESP.restart();
        });
    });

    server_->on("/newauthpass", HTTP_ANY, [this](AsyncWebServerRequest *request) {
        OnRequestWithAuth(request, [this](AsyncWebServerRequest *request) {
            if (!HasRequestValue(request, "newpass")) {
                request->send(400);
                return;
            }

            web_auth_password = GetRequestValue(request, "newpass");
            UpsertWebAuthUser(auth_users_, &auth_user_count_, http_username, web_auth_password);
            SaveWebAuthUsers(auth_users_, auth_user_count_);
            if (!SaveConfig()) {
                request->send(500, "text/plain", "Server error");
                return;
            }

            request->send(200, "text/plain", "OK");
            delay(kResponseDelay_);
            ESP.restart();
        });
    });

    server_->on("/setwifi", HTTP_ANY, [this](AsyncWebServerRequest *request) {
        OnRequestWithAuth(request, [this](AsyncWebServerRequest *request) {
            if (!HasRequestValue(request, "ssid") || !HasRequestValue(request, "pass")) {
                request->send(400);
                return;
            }

            ssid_name = GetRequestValue(request, "ssid");
            ssid_password = GetRequestValue(request, "pass");

            if (!SaveConfig()) {
                request->send(500, "text/plain", "Server error");
                return;
            }
            request->send(200, "text/plain", "OK");
            delay(kResponseDelay_);
            ESP.restart();
        });
    });

    server_->on("/scan/v2", HTTP_GET, [this](AsyncWebServerRequest *request) {
            DynamicJsonDocument doc(1024);
            int n = WiFi.scanComplete();
            if (n == WIFI_SCAN_FAILED) {
                WiFi.scanNetworks(true);
            } else if (n) {
                for (int i = 0; i < n; ++i) {
                int8_t dBm = (WiFi.RSSI(i));
                uint8_t wifi_strenght = RSSIToPercent(dBm);

                    JsonObject doc_nested = doc.createNestedObject(WiFi.SSID(i));
                    doc_nested["encType"] = String(WiFi.encryptionType(i));
                    doc_nested["signal"] = String(wifi_strenght);
                }
                WiFi.scanDelete();
                if (WiFi.scanComplete() == WIFI_SCAN_FAILED) {
                    WiFi.scanNetworks(true);
                }
            }
            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);
    });

    server_->on("/scan", HTTP_GET, [this](AsyncWebServerRequest *request) {
        OnRequestWithAuth(request, [this](AsyncWebServerRequest *request) {
            DynamicJsonDocument doc(1024);
            int n = WiFi.scanComplete();
            if (n == WIFI_SCAN_FAILED) {
                WiFi.scanNetworks(true);
            } else if (n) {
                for (int i = 0; i < n; ++i) {
                    doc[WiFi.SSID(i)] = String(WiFi.encryptionType(i));
                }
                WiFi.scanDelete();
                if (WiFi.scanComplete() == WIFI_SCAN_FAILED) {
                    WiFi.scanNetworks(true);
                }
            }
            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);
        });
    });

    server_->on("/connectedwifi", HTTP_GET, [this](AsyncWebServerRequest *request) {
        OnRequestWithAuth(request, [](AsyncWebServerRequest *request) {
            request->send(200, "text/plain", WiFi.status() == WL_CONNECTED ? ssid_name : "NULL");
        });
    });

    server_->on("/auth/users", HTTP_GET, [this](AsyncWebServerRequest *request) {
        OnRequestWithAuth(request, [this](AsyncWebServerRequest *request) {
            DynamicJsonDocument doc(256);
            JsonArray users = doc.createNestedArray("users");
            for (uint8_t i = 0; i < auth_user_count_; i++) {
                users.add(auth_users_[i].username);
            }

            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);
        });
    });

    server_->on("/auth/users", HTTP_POST, [this](AsyncWebServerRequest *request) {
        OnRequestWithAuth(request, [this](AsyncWebServerRequest *request) {
            if (!HasRequestValue(request, "username") || !HasRequestValue(request, "password")) {
                request->send(400, "text/plain", "Incorrect data");
                return;
            }

            if (!UpsertWebAuthUser(auth_users_, &auth_user_count_, GetRequestValue(request, "username"),
                                   GetRequestValue(request, "password")) ||
                !SaveWebAuthUsers(auth_users_, auth_user_count_)) {
                request->send(400, "text/plain", "Unable to save user");
                return;
            }

            request->send(200, "text/plain", "OK");
        });
    });

    server_->on("/auth/users/delete", HTTP_POST, [this](AsyncWebServerRequest *request) {
        OnRequestWithAuth(request, [this](AsyncWebServerRequest *request) {
            if (!HasRequestValue(request, "username")) {
                request->send(400, "text/plain", "Incorrect data");
                return;
            }

            if (!DeleteWebAuthUser(auth_users_, &auth_user_count_, GetRequestValue(request, "username")) ||
                !SaveWebAuthUsers(auth_users_, auth_user_count_)) {
                request->send(400, "text/plain", "Unable to delete user");
                return;
            }

            request->send(200, "text/plain", "OK");
        });
    });

    server_->on("/setcredentials", HTTP_ANY, [this](AsyncWebServerRequest *request) {
        OnRequestWithAuth(request, [this](AsyncWebServerRequest *request) {
            if (!HasRequestValue(request, "mail") || !HasRequestValue(request, "token") ||
                !HasRequestValue(request, "hostname") || !HasRequestValue(request, "brokerPort") ||
                !HasRequestValue(request, "productId") || !HasRequestValue(request, "deviceId")) {
                request->send(400, "text/plain", "Incorrect data");
                return;
            }

            person_mail = GetRequestValue(request, "mail");
            token = GetRequestValue(request, "token");
            host = GetRequestValue(request, "hostname");
            broker_port = GetRequestValue(request, "brokerPort");
            product_id = GetRequestValue(request, "productId");
            device_id = GetRequestValue(request, "deviceId");
            person_id = Sha256(person_mail);

            Serial.println(person_mail);
            Serial.println(person_id);
            Serial.println(token);
            Serial.println(host);
            Serial.println(broker_port);
            Serial.println(product_id);
            Serial.println(device_id);
            if (!SaveConfig()) {
                request->send(500, "text/plain", "Server error");
                return;
            }

            request->send(200, "text/plain", "OK");
            delay(kResponseDelay_);
            ESP.restart();
        });
    });

    server_->on("/pair", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!request->hasParam("ssid") || !request->hasParam("psk") || !request->hasParam("wsp") ||
            !request->hasParam("token") || !request->hasParam("host") || !request->hasParam("brport")) {
            request->send(400, "text/plain", "Incorrect data");
            return;
        }

        ssid_name = request->getParam("ssid")->value();
        ssid_password = request->getParam("psk")->value();
        person_mail = request->getParam("wsp")->value();
        token = request->getParam("token")->value();
        host = request->getParam("host")->value();
        broker_port = request->getParam("brport")->value();

        String devId = WiFi.macAddress();
        devId.toLowerCase();
        devId.replace(":", "-");
        device_id = devId;
        person_id = Sha256(person_mail);
        Serial.println("WebServer update:");
        Serial.println("SSID_Name = " + ssid_name);
        Serial.println("SSID_Password = <hidden>");
        Serial.println("person_mail = " + person_mail);
        Serial.println("person_id = " + person_id);
        Serial.println("token = <hidden>");
        Serial.println("host = " + host);
        Serial.println("brport = " + broker_port);
        Serial.println("device_id = " + device_id);
        if (ssid_name == "") {
            request->send(400, "text/plain", "Wifi name is NULL");
            return;
        }
        if (!SaveConfig()) {
            request->send(500, "text/plain", "Server error");
            return;
        }

        request->send(200, "text/plain", "OK");
        delay(kResponseDelay_);
        ESP.restart();
    });

    server_->on("/update", HTTP_GET, [this](AsyncWebServerRequest *request) {
        OnRequestWithAuth(request, [this](AsyncWebServerRequest *request) {
            ApplyLentaSettings(request);
        });
    });

    server_->on("/settings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        OnRequestWithAuth(request, [this](AsyncWebServerRequest *request) {
            SendLentaSettings(request);
        });
    });

    server_->on("/settings", HTTP_POST, [this](AsyncWebServerRequest *request) {
        OnRequestWithAuth(request, [this](AsyncWebServerRequest *request) {
            ApplyLentaSettings(request);
        });
    });

    server_->on(
        "/firmware/upload", HTTP_POST,
        [this](AsyncWebServerRequest *request) {
            OnRequestWithAuth(request, [this](AsyncWebServerRequest *request) {
                request->send((Update.hasError()) ? 500 : 200);
                delay(500);
                ESP.restart();
            });
        },
        [this](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len,
               bool final) { OnFirmwareUpload(request, filename, index, data, len, final); });

    server_->onNotFound([this](AsyncWebServerRequest *request) {
        if (TryServeStaticAsset(request)) return;
        request->send(404);
    });

    server_->begin();
}

void WebServer::OnFirmwareUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data,
                                 size_t len, bool final) {
    if (!index) {
        uint32_t freeSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(freeSpace)) {
            Update.printError(Serial);
        }
    }
    if (!Update.hasError()) {
        if (Update.write(data, len) != len) {
            Update.printError(Serial);
        }
    }

    if (final) {
        if (!Update.end(true)) {
            Update.printError(Serial);
        } else {
            Serial.println("Update complete");
        }
    }
}
