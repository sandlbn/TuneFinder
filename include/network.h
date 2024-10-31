#ifndef NETWORK_H
#define NETWORK_H

#include "data.h"
#include "settings.h"

char* make_http_request(const struct APISettings *settings, const char *path);
char* build_search_url(const struct SearchParams *params);
char* url_encode(const char *str);
struct RadioStation* parse_stations_json(const char *json_str, int *count);

#endif /* NETWORK_H */