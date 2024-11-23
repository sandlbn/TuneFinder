#include <stdio.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <ctype.h> 
#include "../../include/country_config.h"
#include "../../include/config.h"
#include "../../include/utils.h"

BOOL LoadCountryConfig(const char *filename, struct CountryConfig *config) {
    BPTR file;
    char buffer[256];

    // Initialize config
    config->count = 0;
    config->entries = AllocVec(sizeof(struct CountryEntry) * MAX_COUNTRIES, MEMF_CLEAR);
    config->choices = AllocVec(sizeof(STRPTR) * (MAX_COUNTRIES + 1), MEMF_CLEAR);
    
    if (!config->entries || !config->choices) {
        DEBUG("Failed to allocate memory for country config");
        return FALSE;
    }

    // Add empty choice as first option
    config->choices[0] = AllocVec(1, MEMF_CLEAR);
    if (!config->choices[0]) {
        return FALSE;
    }
    config->choices[0][0] = '\0';
    config->count = 1;

    file = Open(filename, MODE_OLDFILE);
    if (!file) {
        AddCountry(config, "AT", "Austria");
        AddCountry(config, "AU", "Australia");
        AddCountry(config, "CA", "Canada");
        AddCountry(config, "CZ", "Czech Republic");
        AddCountry(config, "DE", "Germany");
        AddCountry(config, "DK", "Denmark");
        AddCountry(config, "ES", "Spain");
        AddCountry(config, "FR", "France");
        AddCountry(config, "GB", "United Kingdom");
        AddCountry(config, "IT", "Italy");
        AddCountry(config, "JP", "Japan");
        AddCountry(config, "NL", "Netherlands");
        AddCountry(config, "NO", "Norway");
        AddCountry(config, "NZ", "New Zealand");
        AddCountry(config, "PL", "Poland");
        AddCountry(config, "SE", "Sweden");
        AddCountry(config, "US", "United States");
        SaveCountryConfig(filename, config);
        return TRUE;
    }

    while (FGets(file, buffer, sizeof(buffer))) {
        char code[COUNTRY_CODE_LEN];
        char name[COUNTRY_NAME_LEN];
        char *ptr, *nameStart;

        // Remove newline
        char *newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';

        // Find colon separator
        ptr = strchr(buffer, ':');
        if (ptr) {
            // Extract code (first 2 characters)
            strncpy(code, buffer, 2);
            code[2] = '\0';

            // Skip colon and get full name
            nameStart = ptr + 1;
            strncpy(name, nameStart, COUNTRY_NAME_LEN - 1);
            name[COUNTRY_NAME_LEN - 1] = '\0';

            // Trim trailing whitespace
            ptr = name + strlen(name) - 1;
            while (ptr >= name && isspace((unsigned char)*ptr)) {
                *ptr = '\0';
                ptr--;
            }

            if (strlen(code) == 2 && strlen(name) > 0) {
                AddCountry(config, code, name);
            }
        }
    }

    Close(file);
    return TRUE;
}

BOOL SaveCountryConfig(const char *filename, struct CountryConfig *config) {
    BPTR file;
    char dirlock;
    
    // Ensure config directory exists
    BOOL settingsPath = EnsureSettingsPath();
    if (!settingsPath) {
        return FALSE;
    }
    
    file = Open(filename, MODE_NEWFILE);
    if (!file) {
        DEBUG("Failed to create country config file");
        return FALSE;
    }
    
    // Skip first empty entry when saving
    for (int i = 1; i < config->count; i++) {
        char buffer[256];
        sprintf(buffer, "%s:%s\n", 
                config->entries[i].code, 
                config->entries[i].name);
        if (FPuts(file, buffer) == -1) {
            Close(file);
            return FALSE;
        }
    }
    
    Close(file);
    return TRUE;
}

BOOL AddCountry(struct CountryConfig *config, const char *code, const char *name) {
    if (config->count >= MAX_COUNTRIES) {
        return FALSE;
    }
    
    // Add entry
    strncpy(config->entries[config->count].code, code, COUNTRY_CODE_LEN-1);
    strncpy(config->entries[config->count].name, name, COUNTRY_NAME_LEN-1);
    
    // Create display string: "CODE - Name"
    int len = strlen(code) + strlen(name) + 4; // +4 for " - " and null terminator
    config->choices[config->count] = AllocVec(len, MEMF_CLEAR);
    if (!config->choices[config->count]) {
        return FALSE;
    }
    
    sprintf(config->choices[config->count], "%s - %s", code, name);
    config->count++;
    config->choices[config->count] = NULL; // NULL terminate array
    
    return TRUE;
}

void FreeCountryConfig(struct CountryConfig *config) {
    if (config) {
        if (config->choices) {
            for (int i = 0; i < config->count; i++) {
                if (config->choices[i]) {
                    FreeVec(config->choices[i]);
                }
            }
            FreeVec(config->choices);
        }
        if (config->entries) {
            FreeVec(config->entries);
        }
        config->count = 0;
    }
}