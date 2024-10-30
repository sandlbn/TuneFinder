#ifndef NETWORK_H
#define NETWORK_H

#include "data.h"

char* make_http_request(const char *host, const char *path);
char* build_search_url(const struct SearchParams *params);
char* url_encode(const char *str);
struct RadioStation* parse_stations_json(const char *json_str, int *count);

#endif /* NETWORK_H */