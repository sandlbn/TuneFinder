#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/gadtools_protos.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <exec/ports.h>
#include <libraries/asl.h>
#include <proto/asl.h>
#include "../include/gui.h"
#include "../include/network.h"
#include "../include/utils.h"
#include "../include/data.h"
#include "../include/config.h"

int main(void) {
    struct IntuiMessage *imsg;
    
    if (!InitLibraries()) {
        DEBUG("Failed to open libraries");
        return 20;
    }
    
    if (!OpenGUI()) {
        DEBUG("Failed to open GUI");
        CleanupLibraries();
        return 20;
    }
    
    running = TRUE;  // Initialize running flag
    
    while (running) {
        WaitPort(window->UserPort);
        
        while ((imsg = GT_GetIMsg(window->UserPort))) {
            switch (imsg->Class) {
                case IDCMP_CLOSEWINDOW:
                    GT_ReplyIMsg(imsg);
                    running = FALSE;  
                    break;
                    
                case IDCMP_REFRESHWINDOW:
                    GT_BeginRefresh(window);
                    GT_EndRefresh(window, TRUE);
                    GT_ReplyIMsg(imsg);
                    break;
                    
                case IDCMP_MENUPICK: {
                    UWORD menuNumber = imsg->Code;
                    while (menuNumber != MENUNULL) {
                        HandleMenuPick(menuNumber);
                        menuNumber = ItemAddress(menuStrip, menuNumber)->NextSelect;
                    }
                    GT_ReplyIMsg(imsg);
                    break;
                }
                
                case IDCMP_GADGETUP:
                    HandleGadgetUp(imsg);
                    GT_ReplyIMsg(imsg);
                    break;
                    
                default:
                    GT_ReplyIMsg(imsg);
                    break;
            }
        }
    }
    
    // Cleanup
    if (window) {
        struct Gadget *glist = (struct Gadget *)window->UserData;
        CloseWindow(window);
        if (glist) FreeGadgets(glist);
    }
    if (browserList) free_labels(browserList);
    if (visualInfo) FreeVisualInfo(visualInfo);
    
    CleanupLibraries();
    return 0;
}