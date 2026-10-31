#ifndef DATA_H
#define DATA_H

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <stddef.h>

struct SearchParams {
    const char *name;
    const char *country_code;
    const char *state;
    const char *codec;
    const char *tag_list;
    LONG limit;
    int hidebroken;
    int is_https;    // HTTPS_ALL, HTTPS_TRUE or HTTPS_FALSE
    const char *order;   // API sort field, NULL for the server default
    int reverse;
    LONG offset;         // for paging, 0 for the first page
};

#define STATION_UUID_LEN 37   // 36 characters + NUL

// A station in the list view. The node, its display text and all strings
// are allocated with AllocVec and owned by the node: release with
// FreeStationNode(). String fields are never NULL.
struct ExtNode {
    struct Node node;            // ln_Name points to displayText
    char *displayText;
    char *name;
    char *url;
    char *codec;
    char *country;
    int bitrate;
    char uuid[STATION_UUID_LEN]; // Radio Browser stationuuid, may be empty
    BOOL hls;                    // HLS stream, AmigaAMP cannot play these
    BOOL broken;                 // failed the last Radio Browser check
};

void *allocate(size_t size, int type);
void deallocate(void *ptr, int type);

char *StrDupVec(const char *str);
struct ExtNode *CreateStationNode(const char *name, const char *url,
                                  const char *codec, const char *country,
                                  int bitrate);
void FreeStationNode(struct ExtNode *station);
void FreeStationArray(struct ExtNode **stations, LONG count);
char *FormatStationEntry(const char *name, const char *url, const char *codec,
                         const char *country, int bitrate);

BOOL IsStationUUID(const char *str);
void SetStationUUID(struct ExtNode *station, const char *uuid);

// Favorites file line format: name|url|codec|country|bitrate[|uuid]
// '|' inside a field is stored as "%7C". The uuid field was added later
// and is recognised by its format.
#define FAV_FIELD_COUNT 5
BOOL ParseFavoriteLine(char *line, char *fields[FAV_FIELD_COUNT], char **uuid);
void CleanFavoriteField(char *dst, const char *src, size_t dstLen);

#endif /* DATA_H */
