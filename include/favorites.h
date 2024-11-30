#ifndef FAVORITES_H
#define FAVORITES_H

#include <exec/types.h>
#include "data.h"

// Function prototypes
BOOL SaveFavorite(const struct ExtNode *station);
BOOL RemoveFavorite(const struct ExtNode *station);
struct List *LoadFavorites(void);
BOOL IsStationInFavorites(const struct ExtNode *station);

// Format for favorites file entries:
// name|url|codec|country|bitrate\n
// Example:
// Radio 1|http://example.com/stream|MP3|US|128

#endif /* FAVORITES_H */