#ifndef UTILS_H
#define UTILS_H

#include <exec/types.h>
#include <exec/lists.h>
#include "data.h"

void free_labels(struct List* l);
BOOL SaveToPLS(const char *filename);
void UpdateStatusMessage(const char *message);

#endif /* UTILS_H */