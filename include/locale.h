#ifndef TUNEFINDER_LOCALE_H
#define TUNEFINDER_LOCALE_H

#include <exec/types.h>
#include <libraries/locale.h>
#include <stdarg.h>


// GUI Labels (Simple labels for interface elements)
#define MSG_NAME                1
#define MSG_COUNTRY            2
#define MSG_CODEC              3
#define MSG_TAGS               4
#define MSG_BITRATE            5
#define MSG_UNKNOWN            6
#define MSG_LIMIT              7
#define MSG_URL                8
// GUI Actions (Buttons and checkboxes)
#define MSG_SEARCH             10
#define MSG_SAVE              11
#define MSG_CANCEL            12
#define MSG_PLAY              13
#define MSG_QUIT              14
#define MSG_SAVE_SINGLE       15
#define MSG_STOP              16
#define MSG_FAVORITE          17
#define MSG_UNFAVORITE        18
// GUI States (Status messages and titles)
#define MSG_READY             20
#define MSG_SETTINGS          21
#define MSG_API_SETTINGS      22
#define MSG_STATION_DETAILS   23
#define MSG_PROJECT           24
#define MSG_ABOUT             25
#define MSG_SEARCHING         26
#define MSG_FAVORITES         27
// Options and Settings
#define MSG_HOST              30
#define MSG_PORT              31
#define MSG_HTTPS_ONLY        32
#define MSG_HIDE_BROKEN       33

// Status Messages with Parameters
#define MSG_FOUND_STATIONS     40  // "Found %d stations"
#define MSG_PLAYING_STATION    41  // "Playing: %s"
#define MSG_SETTINGS_LOADED    42  // "Settings loaded."
#define MSG_SETTINGS_SAVED     43  // "Settings saved."
#define MSG_SEARCH_COMPLETED   44  // "Search completed. Found %d stations."
#define MSG_FILE_SAVED         45  // "File saved: %s"
#define MSG_SET_SAVED          46  // "Settings saved: %s:%u"
#define MSG_NO_STATION_FOUND   47 // "No stations found"
#define MSG_FAVORITE_ADDED     48 // "Added to favorites"
#define MSG_FAVORITE_REMOVED   49 // "Station removed from favorites"
// Error Messages - Settings Related
#define MSG_INVALID_PORT           50  // "Invalid port number, keeping current: %ld"
#define MSG_FAILED_WRITE_PORT_SET  51  // "Failed to write port setting"
#define MSG_FAILED_WRITE_HOST_SETTING 52  // "Failed to write host setting"
#define MSG_INVALID_LIMIT          53  // "Invalid limit number, keeping current: %ld"
#define MSG_FAILED_WRITE_LIMIT_SET  54  // "Failed to write limit setting"
#define MSG_FAILED_ADD_FAVORITE 55 // "Failed to add to favorites"
#define MSG_FAILED_REMOVE_FAVORITE 56 // "Failed to remove from favorites"
// Error Messages - File Operations
#define MSG_FAILED_FILE_SAVE       60  // "Failed to save file"
#define MSG_FAILED_ACCESS          61  // "Failed to access %s"
#define MSG_FAILED_CREAT_PORT_FILE 62  // "Failed to create port settings file: %s"
#define MSG_FAILED_CREAT_HOST_SET_FILE 63  // "Failed to create host settings file: %s"
#define MSG_FAILED_CREAT_LIMIT_FILE  64  // "Failed to create limit settings file: %s"
// Error Messages - Directory Operations
#define MSG_CREATED_SET_DIR        70  // "Created settings directory: %s"
#define MSG_FAIL_CREATE_SET_DIR    71  // "Failed to create directory: %s"

// Error Messages - Network Related
#define MSG_HTTP_REQ_FAILED        80  // "HTTP request failed"
#define MSG_FAILED_RESOLV_HOST     81  // "Failed to resolve host"
#define MSG_FAILED_CONN_SERV       82  // "Failed to connect to server"
#define MSG_FAILED_SEND_REQ        83  // "Failed to send request"
#define MSG_TIMEOUT                84  // "Timeout waiting for data"
#define MSG_FAILED_CR_SOC          85  // "Failed to create socket"
#define MSG_FAILED_ALL_BUFF        86  // "Failed to allocate buffers"
#define MSG_INVALID_HOST           87  // Invalid host
// Error Messages - External Programs
#define MSG_FAILED_START_PLAYBACK  90  // "Failed to start playback"
#define MSG_AMIGAAMP_NOT_RUNNING   91  // "AmigaAMP is not running"
#define MSG_STOPPING_PLAYBACK      92 // Playback stopped

BOOL InitLocaleSystem(void);
void CleanupLocaleSystem(void);
const char* GetTFString(LONG stringNum);
void GetTFFormattedString(char *buffer, size_t bufSize, LONG stringNum, ...);

#endif