#ifndef AMIGAAMP_H
#define AMIGAAMP_H

#include <exec/types.h>

// Function prototypes
BOOL IsAmigaAMPRunning(void);
BOOL SendCommandToAmigaAMP(const char *command);
BOOL OpenStreamInAmigaAMP(const char *streamURL);
BOOL OpenStreamInAmigaAMPWithName(const char *streamURL,
                                  const char *stationName);

// Additional control functions
BOOL StopAmigaAMP(void);
BOOL PauseAmigaAMP(void);
BOOL ResumeAmigaAMP(void);
BOOL NextTrackAmigaAMP(void);
BOOL PreviousTrackAmigaAMP(void);
BOOL SetVolumeAmigaAMP(LONG volume);
BOOL QuitAmigaAMP(void);

#endif /* AMIGAAMP_H */