#include <clib/alib_protos.h>
#include <ctype.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/config.h"
#include "../../include/gui.h"
#include "../../include/settings.h"
#include "../../include/utils.h"

static inline int isPrintableASCII(unsigned char c) {
  return c >= 32 && c <= 126;  // Standard ASCII printable range
}

void SanitizeAmigaFilename(const char *input, char *output, size_t maxLen) {
  size_t i, j = 0;       // Changed to size_t to match maxLen type
  int lastWasSpace = 1;  // Start true to prevent leading space

  if (strncmp(input, "Title", 5) == 0) {
    const char *p = strchr(input, '=');
    if (p) input = p + 1;
  }

  // Skip leading spaces
  while (*input == ' ') input++;

  for (i = 0; input[i] && j < (maxLen - 5);
       i++) {  // Added parentheses for clarity
    unsigned char c = (unsigned char)input[i];

    // Only process ASCII printable characters
    if (!isPrintableASCII(c)) {
      continue;  // Skip non-ASCII characters
    }

    // Convert character to valid filename character
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9')) {
      // Direct copy of alphanumeric
      if (lastWasSpace) {
        output[j++] = (char)toupper(c);  // Cast result of toupper
      } else {
        output[j++] = c;
      }
      lastWasSpace = 0;
    } else if (c == ' ' || c == '-' || c == '_' || c == '.') {
      // Convert spaces and similar to underscore, but collapse multiple
      if (!lastWasSpace && j < (maxLen - 5)) {
        output[j++] = '_';
        lastWasSpace = 1;
      }
    }
    // Ignore all other characters
  }

  // Remove trailing underscore if present
  if (j > 0 && output[j - 1] == '_') {
    j--;
  }

  output[j] = '\0';

  // Ensure we have at least one character
  if (j == 0) {
    strcpy(output, "station");
  }

  // Truncate if too long (leaving room for .pls)
  if (strlen(output) > 14) {
    output[14] = '\0';
  }
}

void UpdateStatusMessage(const char *message) {
  if (window && statusMsgGad) {
    GT_SetGadgetAttrs(statusMsgGad, window, NULL, GTTX_Text, (STRPTR)message,
                      TAG_DONE);
  }

#ifdef DEBUG_BUILD
  DEBUG("Status: %s", message);  // Use original message for debug
#endif
}
void free_labels(struct List *l) {
  struct Node *n;
  while ((n = RemHead(l))) {
    struct ExtNode *ext = (struct ExtNode *)n;
    if (ext->name) free(ext->name);
    if (ext->url) free(ext->url);
    if (ext->codec) free(ext->codec);
    if (ext->country) free(ext->country);
    deallocate(n->ln_Name, V_cstr);
    deallocate(n, V_Node);
  }
  deallocate(l, V_List);
}

BOOL SaveToPLS(const char *filename) {
  FILE *fp;
  struct Node *node;
  int count = 0;

  if (!browserList) return FALSE;
  for (node = browserList->lh_Head; node->ln_Succ; node = node->ln_Succ) {
    count++;
  }

  if (count == 0) {
    DEBUG("No stations to save");
    return FALSE;
  }

  fp = fopen(filename, "w");
  if (!fp) {
    DEBUG("Failed to open file: %s", filename);
    return FALSE;
  }

  fprintf(fp, PLS_HEADER);
  fprintf(fp, PLS_NUMBER_OF_ENTRIES, count);

  count = 1;
  for (node = browserList->lh_Head; node->ln_Succ; node = node->ln_Succ) {
    struct ExtNode *ext = (struct ExtNode *)node;
    fprintf(fp, PLS_FILE_ENTRY, count, ext->url);
    fprintf(fp, PLS_TITLE_ENTRY, count, ext->displayText);
    fprintf(fp, PLS_LENGTH_ENTRY, count);
    count++;
  }

  fclose(fp);
  return TRUE;
}
BOOL EnsureSettingsPath(void) {
  BPTR lock;
  char msg[MAX_STATUS_MSG_LEN];
  const char *ENV_ROOT = "ENVARC:";

  lock = Lock(ENV_ROOT, ACCESS_READ);
  if (!lock) {
    snprintf(msg, MAX_STATUS_MSG_LEN, "Failed to access %s", ENV_ROOT);
    UpdateStatusMessage(msg);
    DEBUG("%s", msg);
    return FALSE;
  }
  UnLock(lock);

  // Check if settings directory exists
  lock = Lock(ENV_PATH, ACCESS_READ);
  if (lock) {
    UnLock(lock);
    return TRUE;
  }

  // Try to create settings directory
  lock = CreateDir(ENV_PATH);
  if (lock) {
    UnLock(lock);
    snprintf(msg, MAX_STATUS_MSG_LEN, "Created settings directory: %s",
             ENV_PATH);
    UpdateStatusMessage(msg);
    DEBUG("%s", msg);
    return TRUE;
  }

  snprintf(msg, MAX_STATUS_MSG_LEN, "Failed to create directory: %s", ENV_PATH);
  UpdateStatusMessage(msg);
  DEBUG("%s", msg);
  return FALSE;
}

void cleanNonAscii(char *dst, const char *src, size_t maxLen) {
  if (!dst || !src || maxLen == 0) {
    DEBUG("Invalid parameters in cleanNonAscii");
    return;
  }

  int i, j;
  for (i = 0, j = 0; src[i] != '\0' && j < maxLen - 1; i++) {
    // Only copy printable ASCII characters (32-126)
    if (src[i] >= 32 && src[i] <= 126) {
      dst[j++] = src[i];
    }
  }
  dst[j] = '\0';  // Ensure null termination

  if (j == 0) {
    DEBUG("No valid characters found in source string");
    strcpy(dst, "Unknown");
  }
}
