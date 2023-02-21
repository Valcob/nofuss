/*

NOFUSS Client
Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "NoFUSSClientSecure.h"
#include <pgmspace.h>
#include "debug.h"
#include <ArduinoJson.h>
#include <ESP8266httpUpdate.h>

#include <functional>
// #include <CertStoreBearSSL.h>
// BearSSL::CertStore certStore;

const char NoFUSSUserAgent[] PROGMEM = "NoFussClient";
constexpr unsigned long NoFUSSTimeout { 1000ul };

namespace {
String normalizeUrl(const String& server, const String& path) {
    if (path.startsWith("http")) {
        return path;
    }

    return server + '/' + path;
}

} // namespace

void NoFUSSClientSecureClass::setServer(String server) {
    _server = server;
}

void NoFUSSClientSecureClass::setServerFingerprint(const char *serverFingerprint) {
    _serverFingerprint = serverFingerprint;
}

void NoFUSSClientSecureClass::setDevice(String device) {
    _device = device;
}

void NoFUSSClientSecureClass::setVersion(String version) {
    _version = version;
}

void NoFUSSClientSecureClass::setBuild(String build) {
    _build = build;
}

void NoFUSSClientSecureClass::setFirmwareType(bool isCore) {
    _isCore = isCore;
}

void NoFUSSClientSecureClass::onMessage(TMessageFunction fn) {
    _callback = fn;
}

String NoFUSSClientSecureClass::getNewVersion() {
    return _newVersion;
}

String NoFUSSClientSecureClass::getNewFirmware() {
    return _newFirmware;
}

String NoFUSSClientSecureClass::getNewFileSystem() {
    return _newFileSystem;
}

int NoFUSSClientSecureClass::getErrorNumber() {
    return _errorNumber;
}

String NoFUSSClientSecureClass::getErrorString() {
    return _errorString;
}

void NoFUSSClientSecureClass::_doCallback(nofuss_t message) {
    if (_callback != NULL) _callback(message);
}

String NoFUSSClientSecureClass::_getPayload() {

    String payload = "";

    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
#if NOFUSS_SECURE_CERTSTORE
    bool mfln = client->probeMaxFragmentLength(_server.c_str(), 443, 1024);  // server must be the same as in ESPhttpUpdate.update()
    NOFUSS_DEBUG_MSG_P(PSTR("MFLN supported: %s\n"), mfln ? "yes" : "no");
    if (mfln) { client->setBufferSizes(1024, 1024); }
    client->setCertStore(&certStore);
#else
    client->setFingerprint(_serverFingerprint);
#endif

    HTTPClient http;
    http.begin(*client, _server.c_str());

    http.useHTTP10(true);
    http.setReuse(false);
    http.setFollowRedirects(true);
    http.setTimeout(NoFUSSTimeout);
    http.setUserAgent(FPSTR(NoFUSSUserAgent));
    http.addHeader(F("X-ESP8266-MAC"), WiFi.macAddress());
    http.addHeader(F("X-ESP8266-DEVICE"), _device);
    http.addHeader(F("X-ESP8266-VERSION"), _version);
    http.addHeader(F("X-ESP8266-BUILD"), _build);
    http.addHeader(F("X-ESP8266-COREBUILD"), String(_isCore));
    http.addHeader(F("X-ESP8266-CHIPID"), String(ESP.getChipId()));
    http.addHeader(F("X-ESP8266-CHIPSIZE"), String(ESP.getFlashChipRealSize()));
    http.addHeader(F("X-ESP8266-OTASIZE"), String((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000));

    yield();
    int httpCode = http.GET();
    NOFUSS_DEBUG_MSG_P(PSTR("[NOFUSS] Get... %d\n"), httpCode);
    if (httpCode == HTTP_CODE_OK) {
        payload = http.getString();
    } else {
        _errorNumber = httpCode;
        _errorString = http.errorToString(httpCode);
    }
    http.end();

    auto ptr = client.release();
    delete ptr;

    return payload;

}

bool NoFUSSClientSecureClass::_checkUpdates() {

    String payload = _getPayload();
    yield();

    if (payload.length() == 0) {
        NOFUSS_DEBUG_MSG_P(PSTR("[NOFUSS] There is no update.\n"));
        _doCallback(NOFUSS_NO_RESPONSE_ERROR);
        return false;
    }

    StaticJsonDocument<500> jsonBuffer;
    auto error = deserializeJson(jsonBuffer, payload);

    if (error) {
        NOFUSS_DEBUG_MSG_P(PSTR("[NOFUSS] Failed to get a successfull response\n%s"), error.c_str());
        _doCallback(NOFUSS_PARSE_ERROR);
        return false;
    }

    if (jsonBuffer.size() == 0) {
        NOFUSS_DEBUG_MSG_P(PSTR("[NOFUSS] The firmware is the latest one"));
        _doCallback(NOFUSS_UPTODATE);
        return false;
    }

    _newVersion = jsonBuffer["version"].as<String>();
    _newFirmware = jsonBuffer["firmware"].as<String>();

    if (jsonBuffer.containsKey("fs")) {
        _newFileSystem = jsonBuffer["fs"].as<String>();
    } else if (jsonBuffer.containsKey("spiffs")) {
        _newFileSystem = jsonBuffer["spiffs"].as<String>();
    }

    NOFUSS_DEBUG_MSG_P(PSTR("[NOFUSS] There is a new firmware %s\n"), _newVersion.c_str());
    _doCallback(NOFUSS_UPDATE_AVAILABLE);
    return true;
}

bool NoFUSSClientSecureClass::_doUpdateCallbacks(t_httpUpdate_return result, nofuss_t success, nofuss_t error) {
    if (result == HTTP_UPDATE_OK) {
        _doCallback(success);
        return true;
    }

    _errorNumber = ESPhttpUpdate.getLastError();
    _errorString = ESPhttpUpdate.getLastErrorString();
    _doCallback(error);

    return false;
}

void NoFUSSClientSecureClass::_doUpdate() {

    bool error = false;
    uint8_t updates = 0;

    ESPhttpUpdate.rebootOnUpdate(false);

    if (_newFileSystem.length() > 0) {
        auto url = normalizeUrl(_server, _newFileSystem);

        std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
#if NOFUSS_SECURE_CERTSTORE
        bool mfln = client->probeMaxFragmentLength(_server.c_str(), 443, 1024);  // server must be the same as in ESPhttpUpdate.update()
        NOFUSS_DEBUG_MSG_P(PSTR("MFLN supported: %s\n"), mfln ? "yes" : "no");
        if (mfln) { client->setBufferSizes(1024, 1024); }
        client->setCertStore(&certStore);
#else
        client->setFingerprint(_serverFingerprint);
#endif
        t_httpUpdate_return ret = ESPhttpUpdate.updateFS(*client, url);
        if (_doUpdateCallbacks(ret, NOFUSS_FILESYSTEM_UPDATED, NOFUSS_FILESYSTEM_UPDATE_ERROR)) {
            ++updates;
        } else {
            error = true;
        }
        auto ptr = client.release();
        delete ptr;

    }

    if (!error && (_newFirmware.length() > 0)) {
        auto url = normalizeUrl(_server, _newFirmware);

        std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
#if NOFUSS_SECURE_CERTSTORE
        bool mfln = client->probeMaxFragmentLength(_server.c_str(), 443, 1024);  // server must be the same as in ESPhttpUpdate.update()
        NOFUSS_DEBUG_MSG_P(PSTR("MFLN supported: %s\n"), mfln ? "yes" : "no");
        if (mfln) { client->setBufferSizes(1024, 1024); }
        client->setCertStore(&certStore);
#else
        client->setFingerprint(_serverFingerprint);
#endif
        t_httpUpdate_return ret = ESPhttpUpdate.update(*client, url);

        if (_doUpdateCallbacks(ret, NOFUSS_FIRMWARE_UPDATED, NOFUSS_FIRMWARE_UPDATE_ERROR)) {
            ++updates;
        } else {
            error = true;
        }
        auto ptr = client.release();
        delete ptr;
    }

    if (!error && (updates > 0)) {
        _doCallback(NOFUSS_RESET);
        ESP.restart();
    }

}

void NoFUSSClientSecureClass::handle(bool autoUpdate) {

#if NOFUSS_SECURE_CERTSTORE
    SPIFFS.begin();
    int numCerts = certStore.initCertStore(SPIFFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
    NOFUSS_DEBUG_MSG_P(PSTR("[NoFUSS] Number of CA certs read: %d\n"), numCerts);

    if (numCerts == 0) {
        NOFUSS_DEBUG_MSG_P(PSTR("[NoFUSS] No certs found. Did you run certs-from-mozill.py and upload the SPIFFS directory before running?\n"));
        _doCallback(NOFUSS_CONFIGURATION_CRT_ERROR);
        return;  // Can't connect to anything w/o certs!
    }
#endif
    _doCallback(NOFUSS_START);
    NOFUSS_DEBUG_MSG_P(PSTR("[NOFUSS] Checking for firmware updates...\n"));
    if (_checkUpdates()){
        NOFUSS_DEBUG_MSG_P(PSTR("[NOFUSS] Should update? %s\n"), (_isCore || autoUpdate)? "yes": "no");
        if(autoUpdate || _isCore){
            NOFUSS_DEBUG_MSG_P(PSTR("[NOFUSS] Updating...\n"));
            _doUpdate();
        }
    }
    NOFUSS_DEBUG_MSG_P(PSTR("[NOFUSS] Done!!!\n"));
    _doCallback(NOFUSS_END);
}
#if NOFUSS_SECURE
NoFUSSClientSecureClass NoFUSSClient;
#endif