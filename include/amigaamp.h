// amigaamp.h
#ifndef AMIGAAMP_H
#define AMIGAAMP_H

#include <exec/types.h>
#include <stddef.h>

BOOL IsAmigaAMPRunning(void);
// Starts playback asynchronously: returns TRUE once OPEN has been sent.
// The outcome is reported by PollAmigaAMPOpen() when GetAmigaAMPSignal()
// is set.
BOOL OpenStreamInAmigaAMPWithName(const char *streamURL, const char *stationName);

enum {
    AMIGAAMP_OPEN_NONE = 0,  // no OPEN finished since the last poll
    AMIGAAMP_OPEN_OK,
    AMIGAAMP_OPEN_FAILED
};
ULONG GetAmigaAMPSignal(void);
LONG PollAmigaAMPOpen(void);

// Asks AmigaAMP for the current track without waiting; when the reply has
// been handled (PollAmigaAMPOpen) a changed title is returned once by
// GetNewTrackInfo().
BOOL RequestTrackInfo(void);
BOOL GetNewTrackInfo(char *buffer, size_t bufferLen);
BOOL StopAmigaAMP(void);
BOOL WaitAndIconifyAmigaAMP(void);
BOOL SendCommandToAmigaAMP(const char *command);
char *GetTrackInfo(void);
void CleanupAmigaAMP(void);
BOOL QuitAmigaAMP(void);
#endif