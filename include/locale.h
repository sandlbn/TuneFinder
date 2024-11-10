#ifndef TUNEFINDER_LOCALE_H
#define TUNEFINDER_LOCALE_H

#include <exec/types.h>
#include <libraries/locale.h>
#include <stdarg.h>

// Message IDs
#define MSG_NAME             1
#define MSG_COUNTRY         2
#define MSG_CODEC           3
#define MSG_TAGS            4
#define MSG_HTTPS_ONLY      5
#define MSG_HIDE_BROKEN     6
#define MSG_SEARCH          7
#define MSG_SAVE            8
#define MSG_READY           9
#define MSG_SETTINGS        10
#define MSG_HOST            11
#define MSG_PORT            12
#define MSG_CANCEL          13
#define MSG_ABOUT           14
#define MSG_QUIT            15
#define MSG_BITRATE         16

// Function prototypes - renamed to avoid conflicts
BOOL InitLocaleSystem(void);
void CleanupLocaleSystem(void);
const char* GetTFString(LONG stringNum);
void GetTFFormattedString(char *buffer, size_t bufSize, LONG stringNum, ...);

#endif