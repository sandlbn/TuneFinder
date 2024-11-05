#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <rexx/storage.h>
#include <stdio.h>
#include <string.h>
#include "../../include/amigaamp.h"
#include "../../include/config.h"

#define AMIGAAMP_PORT_NAME "AMIGAAMP"
#define MAX_COMMAND_LENGTH 512
#define TEMP_PLS_PATH "RAM:amigaamp.pls"

static void CleanupRexxMessage(struct MsgPort *replyPort, struct RexxMsg *rexxMsg) {
    if (rexxMsg) {
        DeleteRexxMsg(rexxMsg);
    }
    if (replyPort) {
        DeleteMsgPort(replyPort);
    }
}

BOOL IsAmigaAMPRunning(void) {
    struct MsgPort *amigaampPort = FindPort(AMIGAAMP_PORT_NAME);
    return amigaampPort != NULL;
}

BOOL SendCommandToAmigaAMP(const char *command) {
    struct MsgPort *replyPort = NULL;
    struct MsgPort *amigaampPort = NULL;
    struct RexxMsg *rexxMsg = NULL;
    BOOL success = FALSE;
    
    DEBUG("Sending command to AmigaAMP: %s", command);
    
    replyPort = CreateMsgPort();
    if (!replyPort) {
        DEBUG("Failed to create reply port");
        return FALSE;
    }
    
    amigaampPort = FindPort(AMIGAAMP_PORT_NAME);
    if (!amigaampPort) {
        DEBUG("AmigaAMP port not found");
        DeleteMsgPort(replyPort);
        return FALSE;
    }
    
    rexxMsg = CreateRexxMsg(replyPort, NULL, AMIGAAMP_PORT_NAME);
    if (!rexxMsg) {
        DEBUG("Failed to create RexxMsg");
        DeleteMsgPort(replyPort);
        return FALSE;
    }
    
    rexxMsg->rm_Args[0] = (STRPTR)command;
    rexxMsg->rm_Action = RXCOMM;
    
    PutMsg(amigaampPort, (struct Message *)rexxMsg);
    
    WaitPort(replyPort);
    GetMsg(replyPort);
    
    if (rexxMsg->rm_Result1 == 0) {
        DEBUG("Command executed successfully");
        success = TRUE;
    } else {
        DEBUG("Command failed with error: %ld", rexxMsg->rm_Result1);
    }
    
    CleanupRexxMessage(replyPort, rexxMsg);
    
    return success;
}

static BOOL CreateTemporaryPLS(const char *streamURL, const char *stationName) {
    BPTR fh;
    BOOL success = FALSE;
    char buffer[512];
    
    DEBUG("Creating temporary PLS file: %s", TEMP_PLS_PATH);
    
    fh = Open(TEMP_PLS_PATH, MODE_NEWFILE);
    if (fh) {
        // Write playlist header
        FPuts(fh, "[playlist]\n");
        FPuts(fh, "NumberOfEntries=1\n");
        
        // Write stream URL
        strcpy(buffer, "File1=");
        strcat(buffer, streamURL);
        strcat(buffer, "\n");
        FPuts(fh, buffer);
        
        // Write station name if provided
        if (stationName && *stationName) {
            strcpy(buffer, "Title1=");
            strcat(buffer, stationName);
            strcat(buffer, "\n");
            FPuts(fh, buffer);
        }
        
        FPuts(fh, "Length1=-1\n");
        Close(fh);
        success = TRUE;
        DEBUG("PLS file created successfully");
    } else {
        DEBUG("Failed to create PLS file");
    }
    
    return success;
}

BOOL OpenStreamInAmigaAMP(const char *streamURL) {
    char command[MAX_COMMAND_LENGTH];
    
    if (!streamURL) {
        DEBUG("Invalid stream URL (NULL)");
        return FALSE;
    }
    
    if (!IsAmigaAMPRunning()) {
        DEBUG("AmigaAMP is not running");
        return FALSE;
    }
    
    // Create temporary PLS file
    if (!CreateTemporaryPLS(streamURL, NULL)) {
        return FALSE;
    }
    
    // Build command to open the PLS file
    strcpy(command, "OPEN ");
    strcat(command, TEMP_PLS_PATH);
    DEBUG("Command %s ", command);
    return SendCommandToAmigaAMP(command);
}

BOOL OpenStreamInAmigaAMPWithName(const char *streamURL, const char *stationName) {
    char command[MAX_COMMAND_LENGTH];
    
    if (!streamURL) {
        DEBUG("Invalid stream URL (NULL)");
        return FALSE;
    }
    
    if (!IsAmigaAMPRunning()) {
        DEBUG("AmigaAMP is not running");
        return FALSE;
    }
    
    // Create temporary PLS file with station name
    if (!CreateTemporaryPLS(streamURL, stationName)) {
        return FALSE;
    }
    
    // Build command to open the PLS file
    strcpy(command, "OPEN ");
    strcat(command, TEMP_PLS_PATH);
    
    return SendCommandToAmigaAMP(command);
}

BOOL StopAmigaAMP(void) {
    return SendCommandToAmigaAMP("STOP");
}

BOOL PauseAmigaAMP(void) {
    return SendCommandToAmigaAMP("PAUSE");
}

BOOL ResumeAmigaAMP(void) {
    return SendCommandToAmigaAMP("RESUME");
}

BOOL NextTrackAmigaAMP(void) {
    return SendCommandToAmigaAMP("NEXT");
}

BOOL PreviousTrackAmigaAMP(void) {
    return SendCommandToAmigaAMP("PREV");
}

BOOL SetVolumeAmigaAMP(LONG volume) {
    char command[MAX_COMMAND_LENGTH];
    char volStr[16];
    
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    
    // Convert number to string manually
    sprintf(volStr, "%ld", volume);
    
    strcpy(command, "VOLUME ");
    strcat(command, volStr);
    
    return SendCommandToAmigaAMP(command);
}

BOOL QuitAmigaAMP(void) {
    return SendCommandToAmigaAMP("QUIT");
}