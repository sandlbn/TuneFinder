#ifndef UTILS_H
#define UTILS_H

#include <exec/types.h>
#include <exec/lists.h>
#include "data.h"
#include <dos/dos.h>

void free_labels(struct List* l);
BOOL SaveToPLS(const char *filename);
void UpdateStatusMessage(const char *message);
const char *GetStatusMessage(void);
void WritePLSHeader(BPTR file, LONG count);
void WritePLSEntry(BPTR file, LONG index, const char *url, const char *title);
void SanitizeAmigaFilename(const char *input, char *output, size_t maxLen);
BOOL EnsureSettingsPath(void);
void cleanNonAscii(char *dst, const char *src, size_t maxLen);
#endif /* UTILS_H */