#include <clib/alib_protos.h>
#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <intuition/intuition.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/config.h"
#include "../include/data.h"
#include "../include/favorites.h"
#include "../include/gui.h"
#include "../include/locale.h"
#include "../include/utils.h"


BOOL SaveFavorite(const struct ExtNode *station) {
    BPTR file;
    char filepath[256];
    BOOL success = FALSE;
    
    sprintf(filepath, FAVORITES_CONFIG);
    
    // Open file in append mode
    file = Open(filepath, MODE_OLDFILE);
    if (!file) {
        file = Open(filepath, MODE_NEWFILE);
        if (!file) {
            UpdateStatusMessage(GetTFString(MSG_FAILED_ADD_FAVORITE));
            return FALSE;
        }
    }
    
    // Seek to end of file
    Seek(file, 0, OFFSET_END);
    
    // Write station data in format: name|url|codec|country|bitrate
    char buffer[1024];
    sprintf(buffer, "%s|%s|%s|%s|%d\n", 
            station->name ? station->name : "",
            station->url ? station->url : "",
            station->codec ? station->codec : "",
            station->country ? station->country : "",
            station->bitrate);
            
    LONG len = strlen(buffer);
    if (Write(file, buffer, len) == len) {
        success = TRUE;
        UpdateStatusMessage(GetTFString(MSG_FAVORITE_ADDED));
    } else {
        UpdateStatusMessage(GetTFString(MSG_FAILED_ADD_FAVORITE));
    }
    
    Close(file);
    return success;
}

struct List *LoadFavorites(void) {
  BPTR file;
  char buffer[1024];
  struct List *favoritesList;
  char *token;
  char filepath[256];

  sprintf(filepath, FAVORITES_CONFIG);

  favoritesList = CreateInitialList();
  if (!favoritesList) return NULL;

  file = Open(filepath, MODE_OLDFILE);
  if (!file) return favoritesList;  // Return empty list if no favorites file

  while (FGets(file, buffer, sizeof(buffer))) {
    struct ExtNode *ext;
    char *saveptr;

    // Remove newline
    char *newline = strchr(buffer, '\n');
    if (newline) *newline = '\0';

    ext = AllocVec(sizeof(struct ExtNode), MEMF_CLEAR);
    if (!ext) continue;

    // Parse line: name|url|codec|country|bitrate
    token = strtok_r(buffer, "|", &saveptr);
    if (token) ext->name = strdup(token);

    token = strtok_r(NULL, "|", &saveptr);
    if (token) ext->url = strdup(token);

    token = strtok_r(NULL, "|", &saveptr);
    if (token) ext->codec = strdup(token);

    token = strtok_r(NULL, "|", &saveptr);
    if (token) ext->country = strdup(token);

    token = strtok_r(NULL, "|", &saveptr);
    if (token) ext->bitrate = atoi(token);

    // Create display text
    ext->displayText = FormatStationEntry(ext->name, ext->url, ext->codec,
                                          ext->country, ext->bitrate);
    if (!ext->displayText) {
      // Clean up if display text creation failed
      if (ext->name) free(ext->name);
      if (ext->url) free(ext->url);
      if (ext->codec) free(ext->codec);
      if (ext->country) free(ext->country);
      FreeVec(ext);
      continue;
    }

    ext->node.ln_Name = ext->displayText;
    AddTail(favoritesList, (struct Node *)ext);
  }

  Close(file);
  return favoritesList;
}

BOOL RemoveFavorite(const struct ExtNode *station) {
  BPTR oldFile, newFile;
  char buffer[1024];
  char tempPath[256];
  char filepath[256];
  BOOL found = FALSE;

  sprintf(filepath, FAVORITES_CONFIG);
  sprintf(tempPath, TUNEFINDER_DIR "favorites.tmp");

  oldFile = Open(filepath, MODE_OLDFILE);
  if (!oldFile) return FALSE;

  newFile = Open(tempPath, MODE_NEWFILE);
  if (!newFile) {
    Close(oldFile);
    return FALSE;
  }

  // Copy all lines except the one to remove
  while (FGets(oldFile, buffer, sizeof(buffer))) {
    char *url_start = strchr(buffer, '|');
    if (url_start) {
      url_start++;  // Skip the separator
      char *url_end = strchr(url_start, '|');
      if (url_end) {
        *url_end = '\0';
        if (strcmp(url_start, station->url) != 0) {
          // Not the station to remove, write it to new file
          *url_end = '|';  // Restore separator
          Write(newFile, buffer, strlen(buffer));
        } else {
          found = TRUE;
        }
      }
    }
  }

  Close(oldFile);
  Close(newFile);

  if (found) {
    // Replace old file with new one
    DeleteFile(filepath);
    Rename(tempPath, filepath);
    UpdateStatusMessage(GetTFString(MSG_FAVORITE_REMOVED));
    return TRUE;
  } else {
    DeleteFile(tempPath);
    return FALSE;
  }
}
BOOL IsStationInFavorites(const struct ExtNode *station) {
  BPTR file;
  char buffer[1024];
  char filepath[256];
  BOOL found = FALSE;

  if (!station || !station->url) return FALSE;

  sprintf(filepath, FAVORITES_CONFIG);

  file = Open(filepath, MODE_OLDFILE);
  if (!file) return FALSE;

  while (FGets(file, buffer, sizeof(buffer))) {
    char *url_start = strchr(buffer, '|');
    if (url_start) {
      url_start++;  // Skip the separator
      char *url_end = strchr(url_start, '|');
      if (url_end) {
        *url_end = '\0';
        if (strcmp(url_start, station->url) == 0) {
          found = TRUE;
          break;
        }
      }
    }
  }

  Close(file);
  return found;
}
