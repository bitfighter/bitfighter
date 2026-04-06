#include <cstdio>
#include <cstdarg>
#include "tnlLog.h"
#include "tnlPlatform.h"

namespace TNL {
    void logprintf(LogConsumer::MsgType msgType, const char *format, ...) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        printf("\n");
        va_end(args);
    }

    void logprintf(const char *format, ...) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        printf("\n");
        va_end(args);
    }

    S32 dSprintf(char *buffer, U32 bufferSize, const char *format, ...) {
        va_list args;
        va_start(args, format);
        int result = vsnprintf(buffer, bufferSize, format, args);
        va_end(args);
        return result;
    }
}

int stricmp(const char *s1, const char *s2) {
    while (*s1 && (tolower(*s1) == tolower(*s2))) {
        s1++;
        s2++;
    }
    return tolower(*(unsigned char *)s1) - tolower(*(unsigned char *)s2);
}
