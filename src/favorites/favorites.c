#include <clib/alib_protos.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/config.h"
#include "../../include/data.h"
#include "../../include/favorites.h"
#include "../../include/gui.h"
#include "../../include/locale.h"
#include "../../include/utils.h"

#define FAV_LINE_LEN 2048

static BOOL WriteFavoriteField(BPTR file, const char *value, BOOL last) {
  char clean[FAV_LINE_LEN];
  CleanFavoriteField(clean, value, sizeof(clean));
  return FPuts(file, clean) == 0 && FPuts(file, last ? "\n" : "|") == 0;
}

BOOL SaveFavorite(const struct ExtNode *station) {
  BPTR file;
  char bitrate[16];
  BOOL ok;

  if (!station) return FALSE;
  if (!EnsureSettingsPath()) {
    UpdateStatusMessage(GetTFString(MSG_FAILED_ADD_FAVORITE));
    return FALSE;
  }

  // Open file in append mode
  file = Open(FAVORITES_CONFIG, MODE_OLDFILE);
  if (!file) {
    file = Open(FAVORITES_CONFIG, MODE_NEWFILE);
    if (!file) {
      UpdateStatusMessage(GetTFString(MSG_FAILED_ADD_FAVORITE));
      return FALSE;
    }
  }

  Seek(file, 0, OFFSET_END);

  // Write station data: name|url|codec|country|bitrate[|uuid]
  snprintf(bitrate, sizeof(bitrate), "%d", station->bitrate);
  ok = WriteFavoriteField(file, station->name, FALSE) &&
       WriteFavoriteField(file, station->url, FALSE) &&
       WriteFavoriteField(file, station->codec, FALSE) &&
       WriteFavoriteField(file, station->country, FALSE) &&
       WriteFavoriteField(file, bitrate, station->uuid[0] == '\0') &&
       (station->uuid[0] == '\0' || WriteFavoriteField(file, station->uuid, TRUE));

  // Close flushes buffered output, so it can fail too
  if (!Close(file)) ok = FALSE;

  UpdateStatusMessage(GetTFString(ok ? MSG_FAVORITE_ADDED : MSG_FAILED_ADD_FAVORITE));
  return ok;
}

struct List *LoadFavorites(void) {
  BPTR file;
  char buffer[FAV_LINE_LEN];
  char *fields[FAV_FIELD_COUNT];
  char *uuid;
  struct List *favoritesList;

  favoritesList = CreateInitialList();
  if (!favoritesList) return NULL;

  file = Open(FAVORITES_CONFIG, MODE_OLDFILE);
  if (!file) return favoritesList;  // Return empty list if no favorites file

  while (FGets(file, buffer, sizeof(buffer))) {
    struct ExtNode *station;

    if (!ParseFavoriteLine(buffer, fields, &uuid)) continue;

    station = CreateStationNode(fields[0], fields[1], fields[2], fields[3],
                                atoi(fields[4]));
    if (station) {
      SetStationUUID(station, uuid);
      AddTail(favoritesList, (struct Node *)station);
    }
  }

  Close(file);
  return favoritesList;
}

BOOL RemoveFavorite(const struct ExtNode *station) {
  BPTR oldFile, newFile;
  char buffer[FAV_LINE_LEN];
  char parsed[FAV_LINE_LEN];
  char *fields[FAV_FIELD_COUNT];
  const char *tempPath = TUNEFINDER_DIR "favorites.tmp";
  BOOL found = FALSE;
  BOOL writeOk = TRUE;

  if (!station || !station->url) return FALSE;

  oldFile = Open(FAVORITES_CONFIG, MODE_OLDFILE);
  if (!oldFile) return FALSE;

  newFile = Open(tempPath, MODE_NEWFILE);
  if (!newFile) {
    Close(oldFile);
    return FALSE;
  }

  // Copy all lines except the one to remove
  while (FGets(oldFile, buffer, sizeof(buffer))) {
    strcpy(parsed, buffer);
    if (!found && ParseFavoriteLine(parsed, fields, NULL) &&
        strcmp(fields[1], station->url) == 0) {
      found = TRUE;
      continue;
    }
    if (FPuts(newFile, buffer) != 0) {
      writeOk = FALSE;
    }
  }

  Close(oldFile);
  if (!Close(newFile)) writeOk = FALSE;

  if (found && writeOk) {
    // Replace old file with new one
    DeleteFile(FAVORITES_CONFIG);
    Rename(tempPath, FAVORITES_CONFIG);
    UpdateStatusMessage(GetTFString(MSG_FAVORITE_REMOVED));
    return TRUE;
  }

  DeleteFile(tempPath);
  return FALSE;
}

BOOL IsStationInFavorites(const struct ExtNode *station) {
  BPTR file;
  char buffer[FAV_LINE_LEN];
  char *fields[FAV_FIELD_COUNT];
  BOOL found = FALSE;

  if (!station || !station->url) return FALSE;

  file = Open(FAVORITES_CONFIG, MODE_OLDFILE);
  if (!file) return FALSE;

  while (FGets(file, buffer, sizeof(buffer))) {
    if (ParseFavoriteLine(buffer, fields, NULL) && strcmp(fields[1], station->url) == 0) {
      found = TRUE;
      break;
    }
  }

  Close(file);
  return found;
}
