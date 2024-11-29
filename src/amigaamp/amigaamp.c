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
#define MAX_COMMAND_LENGTH 128
#define TEMP_PLS_PATH "RAM:amigaamp.pls"

void ClearMsgPort(struct MsgPort *port) {
  struct Message *msg;
  while ((msg = GetMsg(port))) {
    ReplyMsg(msg);
  }
}
static void CleanupRexxMessage(struct MsgPort *replyPort, struct RexxMsg *rexxMsg) {
    if (rexxMsg) {
        DeleteRexxMsg(rexxMsg);
    }
    if (replyPort) {
        DeleteMsgPort(replyPort);
    }
}

BOOL IsAmigaAMPRunning(void) {
    struct MsgPort *amigaampPort;
    BOOL isRunning = FALSE;
    
    Forbid();
    
    amigaampPort = FindPort(AMIGAAMP_PORT_NAME);
    if (amigaampPort) {
        struct MsgPort *testPort = CreateMsgPort();
        if (testPort) {
            ULONG signals = 1L << testPort->mp_SigBit;
            struct RexxMsg *testMsg = CreateRexxMsg(testPort, NULL, AMIGAAMP_PORT_NAME);
            if (testMsg) {
                testMsg->rm_Args[0] = (STRPTR)"STATUS";
                testMsg->rm_Action = RXCOMM;
                
                PutMsg(amigaampPort, (struct Message *)testMsg);
                
                // Wait for up to 1 second
                if (Wait(signals | SIGBREAKF_CTRL_C) & signals) {
                    GetMsg(testPort);
                    if (testMsg->rm_Result1 == 0) {
                        isRunning = TRUE;
                    }
                } else {
                    DEBUG("Timeout waiting for AmigaAMP response");
                }
                
                DeleteRexxMsg(testMsg);
            }
            DeleteMsgPort(testPort);
        }
    }
    
    Permit();
    
    DEBUG("AmigaAMP running status: %s", isRunning ? "YES" : "NO");
    return isRunning;
}

BOOL SendCommandToAmigaAMP(const char *command) {
  struct MsgPort *replyPort = NULL;
  struct MsgPort *amigaampPort = NULL;
  struct RexxMsg *rexxMsg = NULL;
  struct Message *reply = NULL;
  BOOL success = FALSE;
  ULONG waitSignal;

  // Validate input
  if (!command || !*command) {
    DEBUG("Invalid command (NULL or empty)");
    return FALSE;
  }

  DEBUG("Sending command to AmigaAMP: %s", command);

  // Create reply port
  replyPort = CreateMsgPort();
  if (!replyPort) {
    DEBUG("Failed to create reply port");
    return FALSE;
  }
  waitSignal = 1L << replyPort->mp_SigBit;

  Forbid();  // Prevent task switching

  // Find AmigaAMP port
  amigaampPort = FindPort(AMIGAAMP_PORT_NAME);
  if (!amigaampPort) {
    DEBUG("AmigaAMP port not found");
    Permit();  // Don't forget to Permit() before early return
    DeleteMsgPort(replyPort);
    return FALSE;
  }

  // Create Rexx message
  rexxMsg = CreateRexxMsg(replyPort, NULL, AMIGAAMP_PORT_NAME);
  if (!rexxMsg) {
    DEBUG("Failed to create RexxMsg");
    Permit();  // Don't forget to Permit() before early return
    DeleteMsgPort(replyPort);
    return FALSE;
  }

  // Setup message
  rexxMsg->rm_Args[0] = (STRPTR)command;
  rexxMsg->rm_Action = RXCOMM;

  // Send message while still Forbid()
  PutMsg(amigaampPort, (struct Message *)rexxMsg);

  Permit();  // Allow task switching again after message is sent

  // Wait for response with timeout
  if (Wait(waitSignal | SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) {
    DEBUG("Command canceled by user");
    goto cleanup;
  }

  // Get response
  Forbid();  // Protect message retrieval
  reply = GetMsg(replyPort);
  Permit();

  if (!reply) {
    DEBUG("No response received");
    goto cleanup;
  }

  if (reply != (struct Message *)rexxMsg) {
    DEBUG("Received unexpected message");
    goto cleanup;
  }

  // Check result
  if (rexxMsg->rm_Result1 == 0) {
    if (rexxMsg->rm_Result2) {
      // If there's a response string
      STRPTR resultString = (STRPTR)rexxMsg->rm_Result2;
      DEBUG("Command executed successfully with response: %s", resultString);
    } else {
      DEBUG("Command executed successfully");
    }
    success = TRUE;
  } else {
    // Detailed error reporting
    switch (rexxMsg->rm_Result1) {
      case 1:
        DEBUG("Program not found");
        break;
      case 5:
        DEBUG("Command string error");
        break;
      case 10:
        DEBUG("Command failed");
        break;
      case 20:
        DEBUG("Port not found");
        break;
      case 30:
        DEBUG("No memory available");
        break;
      default:
        DEBUG("Command failed with error: %ld", rexxMsg->rm_Result1);
        break;
    }
  }
  if (reply) {
    if (reply != (struct Message *)rexxMsg) {
      ReplyMsg(reply);
    }
  }
  ClearMsgPort(replyPort);
cleanup:
  // If we got a reply with a result string, free it
  if (success && rexxMsg->rm_Result2) {
    DeleteArgstring((UBYTE *)rexxMsg->rm_Result2);
  }

  // Cleanup
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
    BOOL result;
    
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
    
    result = SendCommandToAmigaAMP(command);
    
    // Clean up temporary file
    DeleteFile(TEMP_PLS_PATH);
    
    return result;
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