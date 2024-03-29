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

#pragma once

#include <ESP8266httpUpdate.h>
#include <functional>
#include <pgmspace.h>

typedef enum {
    NOFUSS_START,
    NOFUSS_UPTODATE,
    NOFUSS_UPDATE_AVAILABLE,
    NOFUSS_UPDATING,
    NOFUSS_FILESYSTEM_UPDATED,
    NOFUSS_FIRMWARE_UPDATED,
    NOFUSS_RESET,
    NOFUSS_END,
    NOFUSS_NO_RESPONSE_ERROR,
    NOFUSS_PARSE_ERROR,
    NOFUSS_FILESYSTEM_UPDATE_ERROR,
    NOFUSS_FIRMWARE_UPDATE_ERROR,
    NOFUSS_CONFIGURATION_CRT_ERROR
} nofuss_t;

class NoFUSSClientSecureClass {

  public:

    typedef std::function<void(nofuss_t)> TMessageFunction;

    void setServer(String server);
    void setServerFingerprint(PGM_P serverFingerprint);
    void setDevice(String device);
    void setVersion(String version);
    void setBuild(String build);
    void setFirmwareType(bool isCore);

    String getNewVersion();
    String getNewFirmware();
    String getNewFileSystem();

    int getErrorNumber();
    String getErrorString();

    void onMessage(TMessageFunction fn);
    void handle(bool autoUpdate);

  private:

    String _server;
    PGM_P  _serverFingerprint;
    String _device;
    String _version;
    String _build;
    bool   _isCore;

    String _newVersion;
    String _newFirmware;
    String _newFileSystem;

    int _errorNumber;
    String _errorString;

    TMessageFunction _callback = NULL;

    String _getPayload();
    bool _checkUpdates();
    bool _doUpdateCallbacks(t_httpUpdate_return result, nofuss_t success, nofuss_t error);
    void _doUpdate();
    void _doCallback(nofuss_t message);

};
#if NOFUSS_SECURE
extern NoFUSSClientSecureClass NoFUSSClient;
#endif