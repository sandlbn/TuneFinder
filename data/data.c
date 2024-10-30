#include <stdlib.h>
#include <string.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include "../include/data.h"
#include "../include/config.h"

void *allocate(size_t size, int type) {
    return AllocVec(size, MEMF_CLEAR);
}

void deallocate(void *ptr, int type) {
    FreeVec(ptr);
}

void free_stations(struct RadioStation *stations, int count) {
    int i;
    
    if (!stations) return;
    
    DEBUG("Freeing %d stations", count);
    for (i = 0; i < count; i++) {
        if (stations[i].name) free(stations[i].name);
        if (stations[i].url) free(stations[i].url);
        if (stations[i].codec) free(stations[i].codec);
    }
    free(stations);
    DEBUG("Memory cleanup complete");
}

char* FormatStationEntry(const char *name, const char *url, const char *codec, int bitrate) {
    char *entry;
    char trimmedName[40];
    char codecBuf[10];
    char bitrateBuf[10];
    size_t nameLen;
    
    entry = AllocVec(256, MEMF_CLEAR);
    if (!entry) return NULL;
    
    if (name) {
        nameLen = strlen(name);
        if (nameLen > 30) {
            strncpy(trimmedName, name, 27);
            trimmedName[27] = '.';
            trimmedName[28] = '.';
            trimmedName[29] = '.';
            trimmedName[30] = '\0';
        } else {
            strcpy(trimmedName, name);
            while (nameLen < 30) {
                trimmedName[nameLen++] = ' ';
            }
            trimmedName[30] = '\0';
        }
    } else {
        strcpy(trimmedName, "Unknown");
        memset(trimmedName + 7, ' ', 23);
        trimmedName[30] = '\0';
    }
    
    if (codec) {
        strncpy(codecBuf, codec, 8);
        codecBuf[8] = '\0';
        size_t codecLen = strlen(codecBuf);
        while (codecLen < 8) {
            codecBuf[codecLen++] = ' ';
        }
        codecBuf[8] = '\0';
    } else {
        strcpy(codecBuf, "???     ");
    }
    
    snprintf(bitrateBuf, sizeof(bitrateBuf), "%4d", bitrate);
    size_t bitrateLen = strlen(bitrateBuf);
    while (bitrateLen < 6) {
        bitrateBuf[bitrateLen++] = ' ';
    }
    bitrateBuf[6] = '\0';
    
    sprintf(entry, "%-30s  %-8s  %-6s", 
        trimmedName,
        codecBuf,
        bitrateBuf);
    
    DEBUG("Formatted entry: '%s'\n", entry);
    return entry;
}