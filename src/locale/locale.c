#include <proto/exec.h>
#include <proto/locale.h>
#include <proto/dos.h>
#include <stdio.h>    // For vsnprintf
#include <stdarg.h>   // For va_list
#include "../../include/locale.h"
#include "../../include/config.h"

#define CATALOG_NAME "tunefinder.catalog"
#define CATALOG_VERSION 1

//static struct Library *LocaleBase = NULL;
static struct Catalog *Catalog = NULL;

// Built-in default strings (English)
static const char *built_in_strings[] = {
    "Name",                                     // 1  MSG_NAME
    "Country",                                  // 2  MSG_COUNTRY
    "Codec",                                    // 3  MSG_CODEC
    "Tags",                                     // 4  MSG_TAGS
    "HTTPS Only",                              // 5  MSG_HTTPS_ONLY
    "Hide Broken",                             // 6  MSG_HIDE_BROKEN
    "Search",                                  // 7  MSG_SEARCH
    "Save",                                    // 8  MSG_SAVE
    "Ready",                                   // 9  MSG_READY
    "Settings",                                // 10 MSG_SETTINGS
    "API Host",                                // 11 MSG_HOST
    "API Port",                                // 12 MSG_PORT
    "Cancel",                                  // 13 MSG_CANCEL
    "About",                                   // 14 MSG_ABOUT
    "Quit",                                    // 15 MSG_QUIT
    "Bitrate",                                // 16 MSG_BITRATE
    "Unknown",                                // 17 MSG_UNKNOWN
    "Found %d stations",                      // 18 MSG_FOUND_STATIONS
    "Playing: %s",                            // 19 MSG_PLAYING_STATION
    "Settings loaded: %s:%d",                 // 20 MSG_SETTINGS_LOADED
    "Invalid port number, keeping current: %ld", // 21 MSG_INVALID_PORT
    "Search completed. Found %d stations.",    // 22 MSG_SEARCH_COMPLETED
    "Project",                                // 23 MSG_PROJECT
    "Play",                                   // 24 MSG_PLAY
    "Station Details",                        // 25 MSG_STATION_DETAILS
    "File saved: %s",                         // 26 MSG_FILE_SAVED
    "Failed to save file",                    // 27 MSG_FAILED_FILE_SAVE
    "Failed to start playback",               // 28 MSG_FAILED_START_PLAYBACK
    "AmigaAMP is not running",               // 29 MSG_AMIGAAMP_NOT_RUNNING
    "HTTP request failed",                    // 30 MSG_HTTP_REQ_FAILED
    "Settings saved: %s:%d",                  // 31 MSG_SETTINGS_SAVED
    "Failed to resolve host",                 // 32 MSG_FAILED_RESOLV_HOST
    "Failed to connect to server",            // 33 MSG_FAILED_CONN_SERV
    "Failed to send request",                 // 34 MSG_FAILED_SEND_REQ
    "Timeout waiting for data",               // 35 MSG_TIMEOUT
    "Failed to allocate buffers",             // 36 MSG_FAILED_ALL_BUFF
    "Failed to create socket",                // 37 MSG_FAILED_CR_SOC
    "Failed to access %s",                    // 38 MSG_FAILED_ACCESS
    "Created settings directory: %s",         // 39 MSG_CREATED_SET_DIR
    "Failed to create directory: %s",         // 40 MSG_FAIL_CREATE_SET_DIR
    "Failed to write port setting",           // 41 MSG_FAIL_WRITE_PORT_SET
    "Settings saved: %s:%u",                  // 42 MSG_SET_SAVED
    "Failed to create port settings file: %s", // 43 MSG_FAILED_CREAT_PORT_FILE
    "Failed to write host setting",           // 44 MSG_FAILED_WRITE_HOST_SETTING
    "Failed to create host settings file: %s", // 45 MSG_FAILED_CREAT_HOST_SET_FILE
    "API Settings",                           // 46 MSG_API_SETTINGS
    NULL
};

BOOL InitLocaleSystem(void)
{
    // Open locale.library
    LocaleBase = OpenLibrary("locale.library", 38);
    if (!LocaleBase) {
        DEBUG("Failed to open locale.library");
        return FALSE;
    }
    
    // Try to open catalog for current locale
    Catalog = OpenCatalogA(NULL, 
                          (STRPTR)CATALOG_NAME,
                          (struct TagItem *)TAG_USER);  // No tags needed
    
    // Note: It's OK if catalog doesn't open - we'll use built-in strings
    if (!Catalog) {
        DEBUG("No catalog found - using built-in strings");
    }
    
    return TRUE;
}

void CleanupLocaleSystem(void)
{
    if (Catalog) {
        CloseCatalog(Catalog);
        Catalog = NULL;
    }
    if (LocaleBase) {
        CloseLibrary(LocaleBase);
        LocaleBase = NULL;
    }
}

const char* GetTFString(LONG stringNum)
{
    const char *str;
    
    // Adjust for 0-based array vs 1-based catalog IDs
    LONG arrayIndex = stringNum - 1;
    
    // Check bounds
    if (arrayIndex < 0 || arrayIndex >= (LONG)(sizeof(built_in_strings) / sizeof(char*) - 1)) {
        DEBUG("String ID %ld out of range", stringNum);
        return "???";
    }
    
    // Try to get string from catalog
    if (Catalog) {
        str = GetCatalogStr(Catalog, stringNum, built_in_strings[arrayIndex]);
    } else {
        str = built_in_strings[arrayIndex];
    }
    
    return str;
}

void GetTFFormattedString(char *buffer, size_t bufSize, LONG stringNum, ...)
{
    va_list args;
    const char *format;
    
    // Get the format string (either from catalog or built-in)
    format = GetTFString(stringNum);
    
    // Format the string with the variable arguments
    va_start(args, stringNum);
    vsnprintf(buffer, bufSize, format, args);
    va_end(args);
}
