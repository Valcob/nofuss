#include "debug.h"
#include <Arduino.h>
#include <cstring>

#if DEBUG_SUPPORT

void _debugSendSerial(const char* prefix, const char* data) {
    if (prefix && (prefix[0] != '\0')) {
        DEBUG_PORT.print(prefix);
    }
    DEBUG_PORT.print(data);

}

void _debugSend(const char * message) {

    char timestamp[10] = {0};

    #if DEBUG_ADD_TIMESTAMP
        const size_t msg_len = strlen(message);
        static bool add_timestamp = true;
        if (add_timestamp) {
            snprintf_P(timestamp, sizeof(timestamp), PSTR("[%06lu] "), millis() % 1000000);
        }
        add_timestamp = (message[msg_len - 1] == 10) || (message[msg_len - 1] == 13);
    #endif

    _debugSendSerial(timestamp, message);

}

// -----------------------------------------------------------------------------

void debugSend(const char * format, ...) {

    va_list args;
    va_start(args, format);
    char test[1];
    int len = ets_vsnprintf(test, 1, format, args) + 1;
    char * buffer = new char[len];
    ets_vsnprintf(buffer, len, format, args);
    va_end(args);

    _debugSend(buffer);

    delete[] buffer;

}

void debugSend_P(PGM_P format_P, ...) {

    char format[strlen_P(format_P)+1];
    memcpy_P(format, format_P, sizeof(format));

    va_list args;
    va_start(args, format_P);
    char test[1];
    int len = ets_vsnprintf(test, 1, format, args) + 1;
    char * buffer = new char[len];
    ets_vsnprintf(buffer, len, format, args);
    va_end(args);

    _debugSend(buffer);

    delete[] buffer;

}

#endif // DEBUG_SUPPORT
