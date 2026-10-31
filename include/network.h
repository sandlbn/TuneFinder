#ifndef NETWORK_H
#define NETWORK_H

#include "api.h"
#include "data.h"
#include "settings.h"

// Returns the body of a "200 OK" response (free with free()), or NULL
char* make_http_request(const struct APISettings *settings, const char *path);
BOOL ReportStationClick(const struct APISettings *settings, const char *uuid);

#endif /* NETWORK_H */
