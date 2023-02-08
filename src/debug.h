#pragma once

#if DEBUG_SUPPORT
#include <Arduino.h>
#include <functional>
#include <pgmspace.h>
#ifndef DEBUG_PORT
    #define DEBUG_PORT              Serial          // Default debugging port
#endif

void debugSend(const char * format, ...);
void debugSend_P(PGM_P format, ...);


#endif // DEBUG_SUPPORT

#if DEBUG_SUPPORT
    #define DEBUG_MSG(...) debugSend(__VA_ARGS__)
    #define DEBUG_MSG_P(...) debugSend_P(__VA_ARGS__)
#else
    #define DEBUG_MSG(...)
    #define DEBUG_MSG_P(...)
#endif
