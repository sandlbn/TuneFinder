#ifndef UTILS_H
#define UTILS_H

#include <exec/types.h>
#include <exec/lists.h>
#include "data.h"

void free_labels(struct List* l);
BOOL SaveToPLS(const char *filename);
void UpdateStatusMessage(const char *message);
void SanitizeAmigaFilename(const char *input, char *output, size_t maxLen);
BOOL EnsureSettingsPath(void);
void cleanNonAscii(char *dst, const char *src, size_t maxLen);
#endif /* UTILS_H */