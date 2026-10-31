#include <exec/memory.h>
#include <proto/exec.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/config.h"
#include "../../include/data.h"

void *allocate(size_t size, int type) { return AllocVec(size, MEMF_CLEAR); }

void deallocate(void *ptr, int type) { FreeVec(ptr); }

char *StrDupVec(const char *str) {
  size_t len;
  char *copy;

  if (!str) str = "";
  len = strlen(str);
  copy = AllocVec(len + 1, MEMF_ANY);
  if (copy) {
    memcpy(copy, str, len + 1);
  }
  return copy;
}

struct ExtNode *CreateStationNode(const char *name, const char *url,
                                  const char *codec, const char *country,
                                  int bitrate) {
  struct ExtNode *station = AllocVec(sizeof(struct ExtNode), MEMF_CLEAR);
  if (!station) return NULL;

  station->name = StrDupVec(name);
  station->url = StrDupVec(url);
  station->codec = StrDupVec(codec);
  station->country = StrDupVec(country);
  station->bitrate = bitrate;
  station->displayText = FormatStationEntry(name, url, codec, country, bitrate);

  if (!station->name || !station->url || !station->codec ||
      !station->country || !station->displayText) {
    FreeStationNode(station);
    return NULL;
  }
  station->node.ln_Name = station->displayText;
  return station;
}

void FreeStationNode(struct ExtNode *station) {
  if (!station) return;
  if (station->displayText) FreeVec(station->displayText);
  if (station->name) FreeVec(station->name);
  if (station->url) FreeVec(station->url);
  if (station->codec) FreeVec(station->codec);
  if (station->country) FreeVec(station->country);
  FreeVec(station);
}

void FreeStationArray(struct ExtNode **stations, LONG count) {
  LONG i;
  if (!stations) return;
  for (i = 0; i < count; i++) {
    FreeStationNode(stations[i]);
  }
  FreeVec(stations);
}

char *FormatStationEntry(const char *name, const char *url, const char *codec,
                         const char *country, int bitrate) {
  char *entry;
  char sanitizedName[31];  // 30 chars + null terminator
  int i, j = 0;

  entry = AllocVec(256, MEMF_CLEAR);
  if (!entry) return NULL;

  if (!name || !*name) name = "Unknown";

  // Printable characters only, at most 30; long names end with "..."
  for (i = 0; name[i] && j < 30; i++) {
    unsigned char c = (unsigned char)name[i];
    if (c >= 32 && c <= 126) {
      sanitizedName[j++] = c;
    }
  }
  if (name[i] && j == 30) {
    sanitizedName[27] = '.';
    sanitizedName[28] = '.';
    sanitizedName[29] = '.';
  }
  sanitizedName[j] = '\0';

  snprintf(entry, 255, "%-30s / %-6s / %-2s / %3d", sanitizedName,
           (codec && *codec) ? codec : "???", (country && *country) ? country : "??",
           bitrate);
  return entry;
}

// Radio Browser station ids look like "960e57c5-0601-11e8-ae97-52543be04c81"
BOOL IsStationUUID(const char *str) {
  int i;

  if (!str || strlen(str) != STATION_UUID_LEN - 1) return FALSE;
  for (i = 0; i < STATION_UUID_LEN - 1; i++) {
    char c = str[i];
    if (i == 8 || i == 13 || i == 18 || i == 23) {
      if (c != '-') return FALSE;
    } else if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
                 (c >= 'A' && c <= 'F'))) {
      return FALSE;
    }
  }
  return TRUE;
}

void SetStationUUID(struct ExtNode *station, const char *uuid) {
  if (!station) return;
  if (IsStationUUID(uuid)) {
    strcpy(station->uuid, uuid);
  } else {
    station->uuid[0] = '\0';
  }
}

// Restores '|' characters that were stored as "%7C".
static void DecodeFavoriteField(char *field) {
  char *src = field, *dst = field;
  while (*src) {
    if (src[0] == '%' && src[1] == '7' && (src[2] == 'C' || src[2] == 'c')) {
      *dst++ = '|';
      src += 3;
    } else {
      *dst++ = *src++;
    }
  }
  *dst = '\0';
}

// Splits a favorites line in place. Fields are taken from the right so that
// older files containing '|' inside station names still load correctly, and
// empty fields (e.g. a station without codec) keep their position.
BOOL ParseFavoriteLine(char *line, char *fields[FAV_FIELD_COUNT], char **uuid) {
  char *end;
  char *sep;
  int i;

  if (uuid) *uuid = NULL;
  if (!line) return FALSE;

  end = line + strlen(line);
  while (end > line && (end[-1] == '\n' || end[-1] == '\r')) {
    *--end = '\0';
  }

  // Optional trailing station id
  sep = strrchr(line, '|');
  if (sep && IsStationUUID(sep + 1)) {
    *sep = '\0';
    if (uuid) *uuid = sep + 1;
  }

  for (i = FAV_FIELD_COUNT - 1; i > 0; i--) {
    sep = strrchr(line, '|');
    if (!sep) return FALSE;
    *sep = '\0';
    fields[i] = sep + 1;
  }
  fields[0] = line;

  for (i = 0; i < FAV_FIELD_COUNT; i++) {
    DecodeFavoriteField(fields[i]);
  }

  // A favorite without URL is useless
  return fields[1][0] != '\0';
}

// Copies a field for writing to the favorites file. '|' is the separator,
// so it is stored as "%7C"; line breaks become spaces.
void CleanFavoriteField(char *dst, const char *src, size_t dstLen) {
  size_t j = 0;

  if (!dst || dstLen == 0) return;
  if (!src) src = "";

  for (; *src && j < dstLen - 1; src++) {
    char c = *src;
    if (c == '|') {
      if (j + 3 > dstLen - 1) break;
      dst[j++] = '%';
      dst[j++] = '7';
      dst[j++] = 'C';
    } else {
      dst[j++] = (c == '\n' || c == '\r') ? ' ' : c;
    }
  }
  dst[j] = '\0';
}
