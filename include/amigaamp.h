#ifndef AMIGAAMP_H
#define AMIGAAMP_H

#include <exec/types.h>

// Function prototypes
BOOL IsAmigaAMPRunning(void);
BOOL SendCommandToAmigaAMP(const char *command);
BOOL OpenStreamInAmigaAMP(const char *streamURL);
BOOL OpenStreamInAmigaAMPWithName(const char *streamURL, const char *stationName);

// Additional control functions
BOOL StopAmigaAMP(void);
BOOL PauseAmigaAMP(void);
BOOL ResumeAmigaAMP(void);
BOOL NextTrackAmigaAMP(void);
BOOL PreviousTrackAmigaAMP(void);
BOOL SetVolumeAmigaAMP(LONG volume);
BOOL QuitAmigaAMP(void);

// Error codes
#define AMIGAAMP_OK           0
#define AMIGAAMP_NOT_RUNNING  1
#define AMIGAAMP_COMM_ERROR   2
#define AMIGAAMP_PORT_ERROR   3

#endif /* AMIGAAMP_H */