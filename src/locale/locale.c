#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/locale.h>
#include <stdarg.h>  // For va_list
#include <stdio.h>   // For vsnprintf

#include "../../include/config.h"
#include "../../include/locale.h"

#define CATALOG_NAME "tunefinder.catalog"
#define CATALOG_VERSION 1

// static struct Library *LocaleBase = NULL;
static struct Catalog *Catalog = NULL;

// Built-in default strings (English)
// locale.c
static const char *built_in_strings[] = {
    // GUI Labels (1-6)
    "Name", "Country", "Codec", "Tags", "Bitrate", "Unknown", "Limit", "URL", 
    // Padding to align with IDs 9
    NULL,

    // GUI Actions (10-14)
    "Search", "Save All", "Cancel", "Play", "Quit", "Save One", "Stop", "Fav+", "Fav-",
    // Padding to align with IDs 17-19
    NULL,

    // GUI States (20-25)
    "Ready", "Settings...", "API Settings", "Station Details", "Project",
    "About...", "Searching", "Favorites", "Iconify",
    // Padding to align with IDs 29
    NULL,

    // Options and Settings (30-35)
    "API Host", "API Port", "HTTPS Only", "Hide Broken", "Autostart Program", "Browse",
    "Select Program", "Iconify AmigaAmp",
    // Padding to align with IDs 39
    NULL, NULL, 

    // Status Messages with Parameters (40-46)
    "Found %d stations", "Playing: %s", "Settings loaded.", "Settings saved.",
    "Search completed. Found %d stations.", "File saved: %s",
    "Settings saved: %s:%u", "No stations found", "Added to favorites", "Station removed from favorites",
    // No more padding

    // Error Messages - Settings Related (50-52)
    "Invalid port number, keeping current: %ld", "Failed to write port setting",
    "Failed to write host setting",
    "Invalid limit number, keeping current: %ld",
    "Failed to write limit setting",
    "Failed to add to favorites",
    "Failed to remove from favorites",
    // Padding to align with IDs 56-59
    NULL, NULL, NULL,

    // Error Messages - File Operations (60-63)
    "Failed to save file", "Failed to access %s",
    "Failed to create port settings file: %s",
    "Failed to create host settings file: %s",
    "Failed to create limit settings file: %s",
    "Failed to create autostart settings file: %s",
    "Failed to write autostart setting",
    // Padding to align with IDs 67-69
    NULL, NULL, NULL,

    // Error Messages - Directory Operations (70-71)
    "Created settings directory: %s", "Failed to create directory: %s",

    // Padding to align with IDs 72-79
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    // Error Messages - Network Related (80-86)
    "HTTP request failed", "Failed to resolve host",
    "Failed to connect to server", "Failed to send request",
    "Timeout waiting for data", "Failed to create socket",
    "Failed to allocate buffers", "Invalid host",
    // Padding to align with IDs 88-89
    NULL, NULL,

    // Error Messages - External Programs (90-91)
    "Failed to start playback", "AmigaAMP is not running", "Playback stopped",
    NULL  // Final terminator
};
BOOL InitLocaleSystem(void) {
  // Open locale.library
  LocaleBase = OpenLibrary("locale.library", 38);
  if (!LocaleBase) {
    DEBUG("Failed to open locale.library");
    return FALSE;
  }

  // Try to open catalog for current locale
  Catalog = OpenCatalog(NULL, CATALOG_NAME,
                        TAG_DONE);  // No tags needed

  // Note: It's OK if catalog doesn't open - we'll use built-in strings
  if (!Catalog) {
    DEBUG("No catalog found - using built-in strings");
  }

  return TRUE;
}

void CleanupLocaleSystem(void) {
  if (Catalog) {
    CloseCatalog(Catalog);
    Catalog = NULL;
  }
  if (LocaleBase) {
    CloseLibrary(LocaleBase);
    LocaleBase = NULL;
  }
}

const char *GetTFString(LONG stringNum) {
  const char *str;

  // Adjust for 0-based array vs 1-based catalog IDs
  LONG arrayIndex = stringNum - 1;

  // Check bounds
  if (arrayIndex < 0 ||
      arrayIndex >= (LONG)(sizeof(built_in_strings) / sizeof(char *) - 1)) {
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

void GetTFFormattedString(char *buffer, size_t bufSize, LONG stringNum, ...) {
  va_list args;
  const char *format;

  // Get the format string (either from catalog or built-in)
  format = GetTFString(stringNum);

  // Format the string with the variable arguments
  va_start(args, stringNum);
  vsnprintf(buffer, bufSize, format, args);
  va_end(args);
}
