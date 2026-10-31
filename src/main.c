#include <clib/alib_protos.h>
#include <dos/dos.h>
#include <devices/timer.h>
#include <dos/dostags.h>
#include <exec/ports.h>
#include <intuition/intuition.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>
#include <proto/asl.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/intuition.h>
#include <proto/wb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <workbench/workbench.h>

#include "../include/amigaamp.h"
#include "../include/arexx.h"
#include "../include/config.h"
#include "../include/data.h"
#include "../include/gui.h"
#include "../include/locale.h"
#include "../include/network.h"
#include "../include/utils.h"

// GadTools, json-c and bsdsocket run nested in our code; the 4-8 KB default
// stack of a CLI or icon start is not enough. libnix swaps to a stack of
// this size at startup.
unsigned long __stack = 32768;

// Ticks while running to poll AmigaAMP for the current track
#define TRACK_POLL_SECS 5

static struct MsgPort *timerPort = NULL;
static struct timerequest *timerReq = NULL;
static BOOL timerOpen = FALSE;
static BOOL timerPending = FALSE;

static void StartTimer(ULONG seconds) {
  if (!timerOpen || timerPending) return;
  timerReq->tr_node.io_Command = TR_ADDREQUEST;
  timerReq->tr_time.tv_secs = seconds;
  timerReq->tr_time.tv_micro = 0;
  SendIO((struct IORequest *)timerReq);
  timerPending = TRUE;
}

static BOOL InitTimer(void) {
  timerPort = CreateMsgPort();
  if (!timerPort) return FALSE;
  timerReq = (struct timerequest *)CreateIORequest(timerPort, sizeof(struct timerequest));
  if (!timerReq) return FALSE;
  timerOpen = OpenDevice(TIMERNAME, UNIT_VBLANK, (struct IORequest *)timerReq, 0) == 0;
  return timerOpen;
}

static void CleanupTimer(void) {
  if (timerOpen) {
    if (timerPending) {
      AbortIO((struct IORequest *)timerReq);
      WaitIO((struct IORequest *)timerReq);
    }
    CloseDevice((struct IORequest *)timerReq);
    timerOpen = FALSE;
  }
  if (timerReq) DeleteIORequest((struct IORequest *)timerReq);
  if (timerPort) DeleteMsgPort(timerPort);
  timerReq = NULL;
  timerPort = NULL;
}

static void HandleWindowMessages(struct MsgPort *windowPort) {
  struct IntuiMessage *imsg;

  // GT_GetIMsg() may return NULL after the window was closed (iconify)
  while (window && (imsg = GT_GetIMsg(windowPort))) {
    // Copy what is needed: the message is gone after the reply
    ULONG class = imsg->Class;
    UWORD code = imsg->Code;
    UWORD gadgetID = 0;

    if (class == IDCMP_GADGETUP || class == IDCMP_GADGETDOWN) {
      gadgetID = ((struct Gadget *)imsg->IAddress)->GadgetID;
    }
    GT_ReplyIMsg(imsg);

    switch (class) {
      case IDCMP_CLOSEWINDOW:
        RequestQuit();
        break;

      case IDCMP_REFRESHWINDOW:
        GT_BeginRefresh(window);
        GT_EndRefresh(window, TRUE);
        break;

      case IDCMP_VANILLAKEY:
        if (code == 'i' || code == 'I') {
          IconifyWindow();
        }
        break;

      case IDCMP_MENUPICK: {
        UWORD menuNumber = code;
        while (menuNumber != MENUNULL && menuStrip && running) {
          struct MenuItem *item = ItemAddress(menuStrip, menuNumber);
          UWORD next = item ? item->NextSelect : MENUNULL;
          HandleMenuPick(menuNumber);
          // Iconify frees the menu strip
          if (!window) break;
          menuNumber = next;
        }
        break;
      }

      case IDCMP_GADGETUP:
        HandleGadgetUp(gadgetID, code);
        break;
    }

    if (!running) break;
  }
}

int main(int argc, char *argv[]) {
  struct WBStartup *WBenchMsg = NULL;
  ULONG signals;

  if (argc == 0) {
    WBenchMsg = (struct WBStartup *)argv;
    programName = WBenchMsg->sm_ArgList[0].wa_Name;
    DEBUG("Started from Workbench, program name: %s", programName);
  } else {
    programName = argv[0];
    DEBUG("Started from CLI, program name: %s", programName);
  }

  if (!InitLibraries()) {
    DEBUG("Failed to open libraries");
    return RETURN_FAIL;
  }

  // Without locale.library the built-in English strings are used
  if (!InitLocaleSystem()) {
    DEBUG("Warning: Could not initialize locale system");
  }

  if (!OpenGUI()) {
    DEBUG("Failed to open GUI");
    CleanupLocaleSystem();
    CleanupLibraries();
    return RETURN_FAIL;
  }

  running = TRUE;

  if (currentSettings.autostart[0] != '\0') {
    DEBUG("Launching autostart program: %s", currentSettings.autostart);
    SystemTags(currentSettings.autostart, SYS_Input, NULL, SYS_Output, NULL,
               SYS_Asynch, TRUE, TAG_DONE);
    if (currentSettings.iconifyAmigaAMP) {
      WaitAndIconifyAmigaAMP();
    }
  }

  // Message port for the AppIcon
  appPort = CreateMsgPort();
  if (!appPort) {
    DEBUG("Failed to create AppIcon port");
    CleanupGUI();
    CleanupLocaleSystem();
    CleanupLibraries();
    return RETURN_FAIL;
  }

  // Optional: scripting does not work without it
  if (!InitRexxPort()) {
    DEBUG("Failed to create ARexx port");
  }

  // Without the timer only the live track info is missing
  if (InitTimer()) {
    StartTimer(TRACK_POLL_SECS);
  }

  while (running) {
    struct MsgPort *windowPort = window ? window->UserPort : NULL;
    ULONG windowSignal = windowPort ? 1UL << windowPort->mp_SigBit : 0;
    ULONG appSignal = 1UL << appPort->mp_SigBit;
    ULONG ampSignal = GetAmigaAMPSignal();
    ULONG rexxSignal = GetRexxSignal();
    ULONG timerSignal = timerOpen ? 1UL << timerPort->mp_SigBit : 0;

    signals = Wait(windowSignal | appSignal | ampSignal | rexxSignal |
                   timerSignal | SIGBREAKF_CTRL_C);

    if (signals & SIGBREAKF_CTRL_C) {
      RequestQuit();
      break;
    }

    if (windowPort && (signals & windowSignal)) {
      HandleWindowMessages(windowPort);
    }

    if (signals & appSignal) {
      struct AppMessage *appMsg;

      while ((appMsg = (struct AppMessage *)GetMsg(appPort))) {
        BOOL shouldUnIconify = (appMsg->am_NumArgs == 0);
        ReplyMsg((struct Message *)appMsg);

        if (shouldUnIconify) {
          UnIconifyWindow();
        }
      }
    }

    if (signals & ampSignal) {
      HandleAmigaAMPReply();
    }

    if (signals & rexxSignal) {
      HandleRexxMessages();
    }

    if ((signals & timerSignal) && timerPending &&
        CheckIO((struct IORequest *)timerReq)) {
      WaitIO((struct IORequest *)timerReq);
      timerPending = FALSE;
      if (IsStationPlaying()) RequestTrackInfo();
      StartTimer(TRACK_POLL_SECS);
    }
  }

  CleanupTimer();
  CleanupRexxPort();
  CleanupAmigaAMP();
  CleanupGUI();
  CleanupLocaleSystem();
  CleanupLibraries();
  return RETURN_OK;
}
