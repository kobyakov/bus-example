#ifndef BUS_COMMON_H
#define BUS_COMMON_H

#include <unistd.h>
#include <stdint.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>

#define BUS_STATUS int
#define STATUS_OK 0
#define STATUS_FALSE 1
#define STATUS_ERR -1

#define MSG_BUS_HELLO   0x11
#define MSG_BUS_QUERY   0x12
#define MSG_BUS_REPLY   0x13
#define MSG_BUS_EVENT   0x14
#define MSG_BUS_OK      0x15
#define MSG_BUS_ERROR   0x16

enum bus_status_message_type {
    HELLO_MESSAGE,
    QUERY_MESSAGE,
    REPLY_MESSAGE,
    EVENT_MESSAGE,
    OK_MESSAGE,
    ERROR_MESSAGE,
    UNDEF_MESSAGE
};


void bus_debug(int level, const char* format, ...);


#define lprintf(level, format...)\
    do                           \
    {                            \
        bus_debug(level, format);   \
    } while (0)


#define BUS_ASSERT_WITH(result, label, message) \
do {                             \
    if ((result)) {              \
        lprintf(LOG_ERR          \
                , "%s (%s)"      \
                , message        \
                , strerror(errno)\
               );                \
        goto label;              \
    }                            \
} while (0)

#define BUS_CHECK_WITH(result, label, message) \
do {                             \
    BUS_STATUS rres = (result);   \
    if ((rres) < 0) {            \
        lprintf(LOG_ERR          \
                , "%s (%s)"      \
                , message        \
                , strerror(errno)\
               );                \
        goto label;              \
    }                            \
} while (0)

#define BUS_ASSERT(result,message) RM_ASSERT_WITH((result), error, message)
#define BUS_CHECK(result,message) BUS_CHECK_WITH((result), error, message)
#define BUS_FAIL(message) BUS_CHECK(STATUS_ERR, message)
#define BUS_FAIL_WITH(label, message) BUS_CHECK_WITH(STATUS_ERR, label, message)
#define BUS_NULL_POINTER_CHECK(p) BUS_CHECK(p != NULL, #p " is NULL")

#ifdef __GNUC__
#   define UNUSED(x) UNUSED_##x __attribute__((__unused__))
#else
#   define UNUSED(x) UNUSED_##x
#endif

#ifndef     TRUE
#define     TRUE                 (1==1)
#endif

#ifndef     FALSE
#define     FALSE                0
#endif

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#endif // BUS_COMMON
