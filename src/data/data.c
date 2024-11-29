#include <clib/exec_protos.h>
#include <exec/memory.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/config.h"
#include "../../include/data.h"

void *allocate(size_t size, int type) { return AllocVec(size, MEMF_CLEAR); }

void deallocate(void *ptr, int type) { FreeVec(ptr); }

void free_stations(struct RadioStation *stations, int count) {
  int i;

  if (!stations) return;

  DEBUG("Freeing %d stations", count);
  for (i = 0; i < count; i++) {
    if (stations[i].name) free(stations[i].name);
    if (stations[i].url) free(stations[i].url);
    if (stations[i].codec) free(stations[i].codec);
    if (stations[i].country) free(stations[i].country);
  }
  free(stations);
  DEBUG("Memory cleanup complete");
}

char *FormatStationEntry(const char *name, const char *url, const char *codec,
                         const char *country, int bitrate) {
  char *entry;
  char sanitizedName[31];  // 30 chars + null terminator

  entry = AllocVec(256, MEMF_CLEAR);
  if (!entry) return NULL;

  // Process station name - limit to 30 chars
  if (name) {
    WORD i, j = 0;
    WORD nameLen = strlen(name);

    if (nameLen > 27) {  // Leave room for "..."
      // Copy first 27 chars and add "..."
      for (i = 0; i < 27 && name[i]; i++) {
        if (name[i] >= 32 && name[i] <= 126) {
          sanitizedName[j++] = name[i];
        }
      }
      sanitizedName[j++] = '.';
      sanitizedName[j++] = '.';
      sanitizedName[j++] = '.';
    } else {
      // Copy full name and pad with spaces
      for (i = 0; i < nameLen; i++) {
        if (name[i] >= 32 && name[i] <= 126) {
          sanitizedName[j++] = name[i];
        }
      }
      // Pad with spaces
      while (j < 30) {
        sanitizedName[j++] = ' ';
      }
    }
    sanitizedName[j] = '\0';
  } else {
    strcpy(sanitizedName, "Unknown");
    memset(sanitizedName + 7, ' ', 23);  // Pad with spaces
    sanitizedName[30] = '\0';
  }

  // Format with delimiters
  snprintf(entry, 255, "%-30s / %-6s / %-2s / %3d", sanitizedName,
           codec ? codec : "???", country ? country : "??", bitrate);

  DEBUG("Formatted entry: '%s'\n", entry);
  return entry;
}