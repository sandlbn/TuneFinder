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
#define MSG_UNKNOWN         17
#define MSG_FOUND_STATIONS        18  // "Found %d stations"
#define MSG_PLAYING_STATION       19  // "Playing: %s"
#define MSG_SETTINGS_LOADED       20  // "Settings loaded: %s:%d"
#define MSG_INVALID_PORT          21  // "Invalid port number, keeping current: %u"
#define MSG_SEARCH_COMPLETED      22  // "Search completed. Found %d stations."
#define MSG_PROJECT         23
#define MSG_PLAY         24
#define MSG_STATION_DETAILS 25
#define MSG_FILE_SAVED 26
#define MSG_FAILED_FILE_SAVE 27
#define MSG_FAILED_START_PLAYBACK 28
#define MSG_AMIGAAMP_NOT_RUNNING 29
#define MSG_HTTP_REQ_FAILED 30
#define MSG_SETTINGS_SAVED 31
#define MSG_FAILED_RESOLV_HOST 32
#define MSG_FAILED_CONN_SERV 33
#define MSG_FAILED_SEND_REQ 34
#define MSG_TIMEOUT 35
#define MSG_FAILED_ALL_BUFF 36
#define MSG_FAILED_CR_SOC 37
#define MSG_FAILED_ACCESS 38
#define MSG_CREATED_SET_DIR 39
#define MSG_FAIL_CREATE_SET_DIR 40
#define MSG_FAIL_WRITE_PORT_SET 41
#define MSG_SET_SAVED 42
#define MSG_FAILED_CREAT_PORT_FILE 43
#define MSG_FAILED_WRITE_HOST_SETTING 44
#define MSG_FAILED_CREAT_HOST_SET_FILE 45
#define MSG_API_SETTINGS 46

BOOL InitLocaleSystem(void);
void CleanupLocaleSystem(void);
const char* GetTFString(LONG stringNum);
void GetTFFormattedString(char *buffer, size_t bufSize, LONG stringNum, ...);

#endif