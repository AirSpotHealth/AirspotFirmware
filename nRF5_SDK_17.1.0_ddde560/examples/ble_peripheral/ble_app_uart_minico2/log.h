#ifndef __LOG_H__
#define __LOG_H__

#define USE_SEGGER_RTT 1

#ifndef USE_SEGGER_RTT
#define print(...)
#define log_hex_dump(...)
#else

#include "SEGGER_RTT.h"
#include "stdarg.h"

#define print(fmt, ...) SEGGER_RTT_printf(0, "[%s] " fmt, __func__, ##__VA_ARGS__)

extern int SEGGER_RTT_vprintf(unsigned BufferIndex, const char *sFormat, va_list *pParamList);

#define log_hex_dump(info, data, len)      \
    do                                     \
    {                                      \
        print("[%s] ", (info));            \
        for (uint16_t i = 0; i < len; i++) \
        {                                  \
            print("%02X ", (data)[i]);     \
        }                                  \
        print("\n");                       \
    } while (0);

#endif

#endif
