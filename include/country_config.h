#ifndef COUNTRY_CONFIG_H
#define COUNTRY_CONFIG_H

#include <settings.h> 
#define MAX_COUNTRIES 255
#define COUNTRY_CODE_LEN 3
#define COUNTRY_NAME_LEN 50

struct CountryEntry {
    char code[COUNTRY_CODE_LEN];
    char name[COUNTRY_NAME_LEN];
};

struct CountryConfig {
    STRPTR *choices;           // Array of strings for GUI
    struct CountryEntry *entries;  // Array of country entries
    int count;                 // Number of countries
};

BOOL LoadCountryConfig(const char *filename, struct CountryConfig *config);
void FreeCountryConfig(struct CountryConfig *config);
BOOL SaveCountryConfig(const char *filename, struct CountryConfig *config);
BOOL AddCountry(struct CountryConfig *config, const char *code, const char *name);

#endif
