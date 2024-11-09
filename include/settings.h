#ifndef SETTINGS_H
#define SETTINGS_H

#include <exec/types.h>

#define ENV_PATH "ENVARC:TuneFinder"
#define TUNEFINDER_DIR "ENVARC:TuneFinder/"
#define CONFIG_PATH "ENVARC:TuneFinder/"
#define FULL_COUNTRY_CONFIG_PATH "ENVARC:TuneFinder/countries.cfg"

#define ENV_HOST "apihost"
#define ENV_PORT "apiport"
#define MAX_HOST_LEN 256
#define MAX_PORT_LEN 6

struct APISettings {
    char host[MAX_HOST_LEN];
    int port;
};

BOOL LoadSettings(struct APISettings *settings);
BOOL SaveSettings(const struct APISettings *settings);
BOOL CreateSettingsWindow(struct APISettings *settings, struct Window *parent);

#endif /* SETTINGS_H */