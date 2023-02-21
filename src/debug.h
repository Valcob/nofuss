#pragma once

#ifdef DEBUG_NOFUSS
    #ifndef DEBUG_PORT
        #define DEBUG_PORT              Serial          // Default debugging port
    #endif

    #ifndef DEBUG_SUPPORT
    #include <Arduino.h>
    #include <functional>
    #include <pgmspace.h>

    void debugSend(const char * format, ...);
    void debugSend_P(PGM_P format, ...);

    #define NOFUSS_DEBUG_MSG(...) debugSend(__VA_ARGS__)
    #define NOFUSS_DEBUG_MSG_P(...) debugSend_P(__VA_ARGS__)
    #else
        #define NOFUSS_DEBUG_MSG(...) DEBUG_MSG(__VA_ARGS__)
        #define NOFUSS_DEBUG_MSG_P(...) DEBUG_MSG_P(__VA_ARGS__)
    #endif // DEBUG_SUPPORT

#endif

#ifndef DEBUG_NOFUSS
    #define NOFUSS_DEBUG_MSG(...)
    #define NOFUSS_DEBUG_MSG_P(...)
#endif 