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
// locale.c
static const char *built_in_strings[] = {
    // GUI Labels (1-6)
    "Name",
    "Country",
    "Codec",
    "Tags",
    "Bitrate",
    "Unknown",
    
    // Padding to align with IDs 7-9
    NULL,
    NULL,
    NULL,
    
    // GUI Actions (10-14)
    "Search",
    "Save",
    "Cancel",
    "Play",
    "Quit",
    
    // Padding to align with IDs 15-19
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    
    // GUI States (20-25)
    "Ready",
    "Settings",
    "API Settings",
    "Station Details",
    "Project",
    "About",
    
    // Padding to align with IDs 26-29
    NULL,
    NULL,
    NULL,
    NULL,
    
    // Options and Settings (30-33)
    "API Host",
    "API Port",
    "HTTPS Only",
    "Hide Broken",
    
    // Padding to align with IDs 34-39
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    
    // Status Messages with Parameters (40-46)
    "Found %d stations",
    "Playing: %s",
    "Settings loaded: %s:%d",
    "Settings saved: %s:%d",
    "Search completed. Found %d stations.",
    "File saved: %s",
    "Settings saved: %s:%u",
    
    // Padding to align with IDs 47-49
    NULL,
    NULL,
    NULL,
    
    // Error Messages - Settings Related (50-52)
    "Invalid port number, keeping current: %ld",
    "Failed to write port setting",
    "Failed to write host setting",
    
    // Padding to align with IDs 53-59
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    
    // Error Messages - File Operations (60-63)
    "Failed to save file",
    "Failed to access %s",
    "Failed to create port settings file: %s",
    "Failed to create host settings file: %s",
    
    // Padding to align with IDs 64-69
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    
    // Error Messages - Directory Operations (70-71)
    "Created settings directory: %s",
    "Failed to create directory: %s",
    
    // Padding to align with IDs 72-79
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    
    // Error Messages - Network Related (80-86)
    "HTTP request failed",
    "Failed to resolve host",
    "Failed to connect to server",
    "Failed to send request",
    "Timeout waiting for data",
    "Failed to create socket",
    "Failed to allocate buffers",
    
    // Padding to align with IDs 87-89
    NULL,
    NULL,
    NULL,
    
    // Error Messages - External Programs (90-91)
    "Failed to start playback",
    "AmigaAMP is not running",
    
    NULL  // Final terminator
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
