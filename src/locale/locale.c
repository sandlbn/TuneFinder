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
    "Name",             // MSG_NAME
    "Country",          // MSG_COUNTRY
    "Codec",           // MSG_CODEC
    "Tags",            // MSG_TAGS
    "HTTPS Only",      // MSG_HTTPS_ONLY
    "Hide Broken",     // MSG_HIDE_BROKEN
    "Search",          // MSG_SEARCH
    "Save",            // MSG_SAVE
    "Ready",           // MSG_READY
    "Settings",        // MSG_SETTINGS
    "API Host",        // MSG_HOST
    "API Port",        // MSG_PORT
    "Cancel",          // MSG_CANCEL
    "About",           // MSG_ABOUT
    "Quit",            // MSG_QUIT
    "Bitrate",         // MSG_BITRATE
    NULL               // Array terminator
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
