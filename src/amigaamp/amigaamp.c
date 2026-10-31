#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <rexx/storage.h>
#include <stdio.h>
#include <string.h>
#include "../../include/amigaamp.h"
#include "../../include/config.h"
#include "../../include/utils.h"


#define AMIGAAMP_PORT_NAME "AMIGAAMP"
#define TEMP_PLS_PATH "RAM:amigaamp.pls"

// Ticks are 1/50 s
#define REXX_STATUS_TIMEOUT (2 * 50)
#define REXX_COMMAND_TIMEOUT (10 * 50)
#define REXX_CLEANUP_TIMEOUT (2 * 50)

// One reply port for all commands. A message that timed out is still owned
// by AmigaAMP until it is replied, so the port must stay alive and the late
// reply is freed when it arrives.
static struct MsgPort *rexxReplyPort = NULL;
static LONG pendingReplies = 0;

// OPEN is sent without waiting: AmigaAMP only replies once the stream is
// connected, which takes seconds (minutes for a dead server).
static struct RexxMsg *openMsg = NULL;
static LONG openResult = AMIGAAMP_OPEN_NONE;

// GETTRACKINFO polled while playing, also without waiting
static struct RexxMsg *trackInfoMsg = NULL;
static char trackInfo[256];
static BOOL trackInfoNew = FALSE;

static void FreeRexxReply(struct RexxMsg *msg) {
    if (!msg) return;
    if (msg->rm_Args[0]) {
        DeleteArgstring((UBYTE *)msg->rm_Args[0]);
    }
    // Result2 is only a string when one was requested and the command succeeded
    if ((msg->rm_Action & RXFF_RESULT) && msg->rm_Result1 == 0 && msg->rm_Result2) {
        DeleteArgstring((UBYTE *)msg->rm_Result2);
    }
    DeleteRexxMsg(msg);
}

// Handles a reply that nobody is waiting for synchronously
static void HandleAsyncReply(struct RexxMsg *msg) {
    if (msg == openMsg) {
        openResult = (msg->rm_Result1 == 0) ? AMIGAAMP_OPEN_OK : AMIGAAMP_OPEN_FAILED;
        openMsg = NULL;
        DEBUG("OPEN replied: %ld", (long)msg->rm_Result1);
    } else if (msg == trackInfoMsg) {
        const char *info = (msg->rm_Result1 == 0 && msg->rm_Result2)
                               ? (const char *)msg->rm_Result2 : "";
        if (strcmp(info, trackInfo) != 0) {
            strncpy(trackInfo, info, sizeof(trackInfo) - 1);
            trackInfo[sizeof(trackInfo) - 1] = '\0';
            trackInfoNew = TRUE;
        }
        trackInfoMsg = NULL;
    }
    FreeRexxReply(msg);
}

// Creates and sends an ARexx command. Returns the message, now owned by
// AmigaAMP until it is replied, or NULL if it could not be sent.
static struct RexxMsg *PutRexxCommand(const char *command, BOOL wantResult) {
    struct MsgPort *amigaampPort;
    struct RexxMsg *rexxMsg;

    if (!command || !*command) {
        DEBUG("Invalid command (NULL or empty)");
        return NULL;
    }

    if (!rexxReplyPort) {
        rexxReplyPort = CreateMsgPort();
        if (!rexxReplyPort) {
            DEBUG("Failed to create reply port");
            return NULL;
        }
    }

    rexxMsg = CreateRexxMsg(rexxReplyPort, NULL, AMIGAAMP_PORT_NAME);
    if (!rexxMsg) {
        DEBUG("Failed to create RexxMsg");
        return NULL;
    }
    rexxMsg->rm_Args[0] = (STRPTR)CreateArgstring((STRPTR)command, strlen(command));
    if (!rexxMsg->rm_Args[0]) {
        DeleteRexxMsg(rexxMsg);
        return NULL;
    }
    rexxMsg->rm_Action = RXCOMM | (wantResult ? RXFF_RESULT : 0);

    DEBUG("Sending command to AmigaAMP: %s", command);

    // The port may vanish at any time, so look it up and send atomically
    Forbid();
    amigaampPort = FindPort(AMIGAAMP_PORT_NAME);
    if (amigaampPort) {
        PutMsg(amigaampPort, (struct Message *)rexxMsg);
    }
    Permit();

    if (!amigaampPort) {
        DEBUG("AmigaAMP port not found");
        FreeRexxReply(rexxMsg);
        return NULL;
    }
    pendingReplies++;
    return rexxMsg;
}

// Sends an ARexx command to AmigaAMP and waits up to timeoutTicks for the
// reply. If result is not NULL, the result string is returned in an
// AllocVec'd buffer (or NULL if there was none).
static BOOL SendRexxCommand(const char *command, char **result, ULONG timeoutTicks) {
    struct RexxMsg *rexxMsg;
    struct Message *reply;
    ULONG waited = 0;
    BOOL replied = FALSE;
    BOOL success = FALSE;

    if (result) *result = NULL;

    rexxMsg = PutRexxCommand(command, result != NULL);
    if (!rexxMsg) return FALSE;

    for (;;) {
        while ((reply = GetMsg(rexxReplyPort))) {
            pendingReplies--;
            if (reply == (struct Message *)rexxMsg) {
                replied = TRUE;
            } else {
                HandleAsyncReply((struct RexxMsg *)reply);
            }
        }
        if (replied || waited >= timeoutTicks) break;
        if (SetSignal(0, 0) & SIGBREAKF_CTRL_C) break;
        Delay(1);
        waited++;
    }

    if (!replied) {
        DEBUG("No reply from AmigaAMP for: %s", command);
        return FALSE;
    }

    if (rexxMsg->rm_Result1 == 0) {
        success = TRUE;
        if (result && rexxMsg->rm_Result2) {
            const char *resultString = (const char *)rexxMsg->rm_Result2;
            ULONG len = strlen(resultString) + 1;
            *result = AllocVec(len, MEMF_ANY);
            if (*result) {
                CopyMem((APTR)resultString, *result, len);
            }
        }
    } else {
        DEBUG("Command '%s' failed with error: %ld", command, (long)rexxMsg->rm_Result1);
    }

    FreeRexxReply(rexxMsg);
    return success;
}

ULONG GetAmigaAMPSignal(void) {
    return rexxReplyPort ? (1UL << rexxReplyPort->mp_SigBit) : 0;
}

BOOL RequestTrackInfo(void) {
    // One at a time, and not while AmigaAMP is busy opening a stream
    if (trackInfoMsg || openMsg) return FALSE;
    trackInfoMsg = PutRexxCommand("GETTRACKINFO", TRUE);
    return trackInfoMsg != NULL;
}

BOOL GetNewTrackInfo(char *buffer, size_t bufferLen) {
    if (!trackInfoNew || !buffer || bufferLen == 0) return FALSE;
    strncpy(buffer, trackInfo, bufferLen - 1);
    buffer[bufferLen - 1] = '\0';
    trackInfoNew = FALSE;
    return TRUE;
}

LONG PollAmigaAMPOpen(void) {
    struct Message *reply;
    LONG result;

    if (rexxReplyPort) {
        while ((reply = GetMsg(rexxReplyPort))) {
            pendingReplies--;
            HandleAsyncReply((struct RexxMsg *)reply);
        }
    }

    result = openResult;
    openResult = AMIGAAMP_OPEN_NONE;
    return result;
}

BOOL IsAmigaAMPRunning(void) {
    BOOL isRunning;

    Forbid();
    isRunning = FindPort(AMIGAAMP_PORT_NAME) != NULL;
    Permit();

    DEBUG("AmigaAMP running status: %s", isRunning ? "YES" : "NO");
    return isRunning;
}

BOOL SendCommandToAmigaAMP(const char *command) {
    return SendRexxCommand(command, NULL, REXX_COMMAND_TIMEOUT);
}

void CleanupAmigaAMP(void) {
    ULONG waited = 0;
    struct Message *reply;

    if (!rexxReplyPort) return;

    while (pendingReplies > 0 && waited < REXX_CLEANUP_TIMEOUT) {
        while ((reply = GetMsg(rexxReplyPort))) {
            pendingReplies--;
            HandleAsyncReply((struct RexxMsg *)reply);
        }
        if (pendingReplies > 0) {
            Delay(1);
            waited++;
        }
    }

    // Never free a port AmigaAMP may still reply to
    if (pendingReplies == 0) {
        DeleteMsgPort(rexxReplyPort);
        rexxReplyPort = NULL;
    }
}

static BOOL CreateTemporaryPLS(const char *streamURL, const char *stationName) {
    BPTR fh;

    DEBUG("Creating temporary PLS file: %s", TEMP_PLS_PATH);

    fh = Open(TEMP_PLS_PATH, MODE_NEWFILE);
    if (!fh) {
        DEBUG("Failed to create PLS file");
        return FALSE;
    }

    WritePLSHeader(fh, 1);
    WritePLSEntry(fh, 1, streamURL, stationName);
    Close(fh);
    DEBUG("PLS file created successfully");
    return TRUE;
}

BOOL OpenStreamInAmigaAMPWithName(const char *streamURL, const char *stationName) {
    struct RexxMsg *msg;

    if (!streamURL) {
        DEBUG("Invalid stream URL (NULL)");
        return FALSE;
    }

    if (!IsAmigaAMPRunning()) {
        DEBUG("AmigaAMP is not running");
        return FALSE;
    }

    if (!CreateTemporaryPLS(streamURL, stationName)) {
        return FALSE;
    }

    msg = PutRexxCommand("OPEN " TEMP_PLS_PATH, FALSE);
    if (!msg) return FALSE;

    // A previous OPEN that is still pending no longer matters
    openMsg = msg;
    openResult = AMIGAAMP_OPEN_NONE;
    return TRUE;
}

BOOL StopAmigaAMP(void) {
    return SendCommandToAmigaAMP("STOP");
}

BOOL WaitAndIconifyAmigaAMP(void) {
    BOOL success = FALSE;
    int retries = 20;  // Wait up to 2 seconds (20 * 100ms)
    
    // Wait for AmigaAMP to start and open its ARexx port
    while (retries > 0) {
        if (IsAmigaAMPRunning()) {
            // Try to iconify
            success = SendCommandToAmigaAMP("ICONIFY");
            break;
        }
        Delay(5);  // Wait 100ms
        retries--;
    }
    
    return success;
}

/* Get track information from AmigaAMP
 * Returns a dynamically allocated string with track info
 * Caller must free the returned string with FreeVec()
 * Returns NULL on error
 */
char *GetTrackInfo(void) {
    char *trackInfo = NULL;

    if (!SendRexxCommand("GETTRACKINFO", &trackInfo, REXX_STATUS_TIMEOUT)) {
        return NULL;
    }
    DEBUG("Track info retrieved: %s", trackInfo ? trackInfo : "(none)");
    return trackInfo;
}

BOOL QuitAmigaAMP(void) {
    return SendCommandToAmigaAMP("QUIT");
}
