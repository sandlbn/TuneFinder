#include <clib/alib_protos.h>
#include <clib/exec_protos.h>
#include <clib/gadtools_protos.h>
#include <clib/intuition_protos.h>
#include <dos/dostags.h>
#include <exec/ports.h>
#include <intuition/intuition.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>
#include <proto/asl.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/wb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <workbench/workbench.h>

#include "../include/config.h"
#include "../include/data.h"
#include "../include/gui.h"
#include "../include/locale.h"
#include "../include/network.h"
#include "../include/utils.h"
#include "../include/amigaamp.h"

int main(int argc, char *argv[]) {
  struct WBStartup *WBenchMsg = NULL;
  struct IntuiMessage *imsg;
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
    return 20;
  }

  if (!InitLocaleSystem()) {
    DEBUG("Warning: Could not initialize locale system");
    return 20;
  }

  if (!OpenGUI()) {
    DEBUG("Failed to open GUI");
    CleanupLibraries();
    return 20;
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

  // Create message port for AppIcon
  appPort = CreateMsgPort();
  if (!appPort) {
    DEBUG("Failed to create AppIcon port");
    CleanupGUI();
    CleanupLibraries();
    return 20;
  }

  while (running) {
    ULONG waitSignals = 0;
    struct MsgPort *windowPort = window ? window->UserPort : NULL;

    // Set up wait signals
    if (windowPort) {
      waitSignals |= 1L << windowPort->mp_SigBit;
    }
    if (appPort) {
      waitSignals |= 1L << appPort->mp_SigBit;
    }

    // Wait for any signal
    signals = Wait(waitSignals);

    // Process window messages first if window exists
    if (windowPort && (signals & (1L << windowPort->mp_SigBit))) {
      while ((imsg = (struct IntuiMessage *)GT_GetIMsg(windowPort))) {
        ULONG class = imsg->Class;
        UWORD code = imsg->Code;

        // Cache any important data before replying
        struct Gadget *gad = (struct Gadget *)imsg->IAddress;

        // Reply to message immediately
        GT_ReplyIMsg(imsg);

        // Handle message after reply
        switch (class) {
          case IDCMP_CLOSEWINDOW:
            SavePreferencesOnExit();
            if (currentSettings.autostart[0] != '\0') {
              if (IsAmigaAMPRunning()) {
                QuitAmigaAMP();
              }
            }
            running = FALSE;
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
            while (menuNumber != MENUNULL) {
              HandleMenuPick(menuNumber);
              menuNumber = ItemAddress(menuStrip, menuNumber)->NextSelect;
            }
            break;
          }

          case IDCMP_GADGETUP:
            if (gad) {
              HandleGadgetUp((struct IntuiMessage *)imsg);
            }
            break;
        }
      }
    }

    // Process AppIcon messages
    if (appPort && (signals & (1L << appPort->mp_SigBit))) {
      struct AppMessage *appMsg;

      while ((appMsg = (struct AppMessage *)GetMsg(appPort))) {
        BOOL shouldUnIconify = (appMsg->am_NumArgs == 0);
        ReplyMsg((struct Message *)appMsg);

        if (shouldUnIconify) {
          UnIconifyWindow();
        }
      }
    }
  }

  // Cleanup sequence
  if (window) {
    struct Gadget *glist = (struct Gadget *)window->UserData;
    if (glist) {
      RemoveGList(window, glist, -1);
      FreeGadgets(glist);
    }
    CloseWindow(window);
    window = NULL;
  }

  if (appIcon) {
    RemoveAppIcon(appIcon);
    appIcon = NULL;
  }

  if (appPort) {
    // Flush any remaining messages
    struct Message *msg;
    while ((msg = GetMsg(appPort))) {
      ReplyMsg(msg);
    }
    DeleteMsgPort(appPort);
    appPort = NULL;
  }

  if (browserList) free_labels(browserList);
  if (visualInfo) FreeVisualInfo(visualInfo);

  CleanupLocaleSystem();
  CleanupLibraries();
  return 0;
}