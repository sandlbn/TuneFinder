#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <libraries/gadtools.h>
#include <dos/dos.h>
#include "../../include/settings.h"
#include "../../include/config.h"
#include "../../include/gui.h"
#include "../../include/utils.h"

BOOL SaveSettings(const struct APISettings *settings) {
    char filepath[256];
    char portStr[MAX_PORT_LEN];
    BPTR file;
    BOOL success = FALSE;
    char msg[MAX_STATUS_MSG_LEN];
    
    if (!EnsureSettingsPath()) {
        return FALSE;
    }
    
    // Save host
    sprintf(filepath, TUNEFINDER_DIR ENV_HOST);
    file = Open(filepath, MODE_NEWFILE);
    if (!file) {
        snprintf(msg, MAX_STATUS_MSG_LEN, "Failed to create host settings file: %s", filepath);
        UpdateStatusMessage(msg);
        DEBUG("%s", msg);
        return FALSE;
    }
    
    LONG len = strlen(settings->host);
    if (Write(file, settings->host, len) != len) {
        snprintf(msg, MAX_STATUS_MSG_LEN, "Failed to write host setting");
        UpdateStatusMessage(msg);
        Close(file);
        return FALSE;
    }
    Close(file);
    
    // Save port
    sprintf(filepath, TUNEFINDER_DIR ENV_PORT);
    file = Open(filepath, MODE_NEWFILE);
    if (!file) {
        snprintf(msg, MAX_STATUS_MSG_LEN, "Failed to create port settings file: %s", filepath);
        UpdateStatusMessage(msg);
        return FALSE;
    }
    
    len = snprintf(portStr, sizeof(portStr), "%u", (unsigned int)settings->port);
    if (Write(file, portStr, len) != len) {
        snprintf(msg, MAX_STATUS_MSG_LEN, "Failed to write port setting");
        UpdateStatusMessage(msg);
        Close(file);
        return FALSE;
    }
    Close(file);
    
    snprintf(msg, MAX_STATUS_MSG_LEN, "Settings saved: %s:%u", settings->host, settings->port);
    UpdateStatusMessage(msg);
    return TRUE;
}

BOOL LoadSettings(struct APISettings *settings) {
    char filepath[256];
    char portStr[MAX_PORT_LEN];
    BPTR file;
    BOOL success = FALSE;
    char msg[MAX_STATUS_MSG_LEN];
    
    // Set defaults first
    strncpy(settings->host, API_HOST, MAX_HOST_LEN-1);
    settings->host[MAX_HOST_LEN-1] = '\0';
    settings->port = API_PORT; 
    
    // Try to load saved settings
    sprintf(filepath, TUNEFINDER_DIR ENV_HOST);
    file = Open(filepath, MODE_OLDFILE);
    if (file) {
        LONG len = Read(file, settings->host, MAX_HOST_LEN-1);
        if (len > 0) {
            settings->host[len] = '\0';
            success = TRUE;
        }
        Close(file);
    } 
    if (!file) {
        snprintf(msg, MAX_STATUS_MSG_LEN, "Can't load file: %s", filepath);
        DEBUG("%s", msg);
    }
    
    // Load port setting
    sprintf(filepath, TUNEFINDER_DIR ENV_PORT);
    file = Open(filepath, MODE_OLDFILE);
    if (file) {
        memset(portStr, 0, sizeof(portStr));  // Clear the buffer
        LONG len = Read(file, portStr, sizeof(portStr) - 1);
        if (len > 0) {
            portStr[len] = '\0';  // Ensure null termination 
            unsigned int tempPort = 0;
            if (sscanf(portStr, "%u", &tempPort) == 1) { 
                settings->port = (UWORD)tempPort;
                success = TRUE;
            } else {
                snprintf(msg, MAX_STATUS_MSG_LEN, "Invalid port number in settings, using default: %ld", API_PORT);
                UpdateStatusMessage(msg);
                DEBUG("%s", msg);
                settings->port = API_PORT;
            }
        }
        Close(file);
    }
    if (!file) {
        snprintf(msg, MAX_STATUS_MSG_LEN, "Can't load file: %s", filepath);
        DEBUG("%s", msg);
    }
    snprintf(msg, MAX_STATUS_MSG_LEN, "Settings loaded: %s:%u", settings->host, settings->port);
    DEBUG("%s", msg);
    
    return success;
}

BOOL CreateSettingsWindow(struct APISettings *settings, struct Window *parent) {
    struct Window *window;
    struct Gadget *glist = NULL, *gad;
    struct Gadget *hostGad = NULL;     
    struct Gadget *portGad = NULL;  
    struct NewGadget ng;
    void *vi;
    struct Screen *screen;
    BOOL done = FALSE;
    struct IntuiMessage *msg;
    BOOL success = FALSE;
    char currentPortStr[MAX_PORT_LEN];
    WORD currentTop;
    
    // Get screen and visual info
    screen = parent->WScreen;
    vi = GetVisualInfo(screen, TAG_DONE);
    if (!vi) {
        DEBUG("Failed to get visual info");
        return FALSE;
    }
    
    // Get font metrics for consistent spacing
    ULONG font_width = screen->RastPort.TxWidth;
    ULONG font_height = screen->RastPort.TxHeight;
    
    // Calculate layout metrics
    WORD leftMargin = font_width * 2;            // Left margin
    WORD topMargin = font_height + 10;            // Top margin
    WORD rowHeight = font_height + 6;            // Height of each row
    WORD rowSpacing = font_height / 2;           // Space between rows
    WORD labelWidth = font_width * 10;           // Width for labels
    WORD controlWidth = font_width * 25;         // Width for input fields
    WORD buttonWidth = font_width * 10;          // Width for buttons
    WORD windowWidth = leftMargin * 3 + controlWidth + labelWidth;
    
    // Create gadget context
    gad = CreateContext(&glist);
    if (!gad) {
        FreeVisualInfo(vi);
        DEBUG("Failed to create gadget context");
        return FALSE;
    }
    
    // Initialize base gadget properties
    ng.ng_TextAttr = screen->Font;
    ng.ng_VisualInfo = vi;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    // Start positioning from top margin
    currentTop = topMargin;
    
    // Host input
    ng.ng_LeftEdge = leftMargin + labelWidth;
    ng.ng_TopEdge = currentTop;
    ng.ng_Width = controlWidth;
    ng.ng_Height = rowHeight;
    ng.ng_GadgetText = "API Host";
    ng.ng_GadgetID = 1;
    
    hostGad = CreateGadget(STRING_KIND, gad, &ng,
                          GTST_MaxChars, MAX_HOST_LEN-1,
                          GTST_String, settings->host,
                          TAG_DONE);
    if (!hostGad) {
        DEBUG("Failed to create host gadget");
        goto cleanup;
    }
    
    // Move to next row
    currentTop += rowHeight + rowSpacing;
    
    // Port input
    ng.ng_TopEdge = currentTop;
    ng.ng_Width = font_width * 10;  // Shorter width for port
    ng.ng_GadgetText = "API Port";
    ng.ng_GadgetID = 2;
    
    snprintf(currentPortStr, sizeof(currentPortStr), "%u", settings->port);
    
    portGad = CreateGadget(STRING_KIND, hostGad, &ng,
                          GTST_MaxChars, MAX_PORT_LEN-1,
                          GTST_String, currentPortStr,
                          TAG_DONE);
    if (!portGad) {
        DEBUG("Failed to create port gadget");
        goto cleanup;
    }
    
    // Move to button row
    currentTop += rowHeight + rowSpacing * 2;  // Extra spacing before buttons
    
    // Save button
    ng.ng_LeftEdge = windowWidth/2 - buttonWidth - leftMargin;
    ng.ng_TopEdge = currentTop;
    ng.ng_Width = buttonWidth;
    ng.ng_GadgetText = "Save";
    ng.ng_GadgetID = 3;
    ng.ng_Flags = PLACETEXT_IN;
    
    gad = CreateGadget(BUTTON_KIND, portGad, &ng, TAG_DONE);
    if (!gad) {
        DEBUG("Failed to create save button");
        goto cleanup;
    }
    
    // Cancel button
    ng.ng_LeftEdge = windowWidth/2 + leftMargin;
    ng.ng_GadgetText = "Cancel";
    ng.ng_GadgetID = 4;
    
    gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_DONE);
    if (!gad) {
        DEBUG("Failed to create cancel button");
        goto cleanup;
    }
    
    // Calculate window height based on last gadget position
    WORD windowHeight = currentTop + rowHeight + topMargin;
    
    // Center window relative to parent
    WORD windowLeft = parent->LeftEdge + (parent->Width - windowWidth) / 2;
    WORD windowTop = parent->TopEdge + (parent->Height - windowHeight) / 2;
    
    // Create window
    window = OpenWindowTags(NULL,
        WA_Title, "API Settings",
        WA_Left, windowLeft,
        WA_Top, windowTop,
        WA_Width, windowWidth,
        WA_Height, windowHeight,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_REFRESHWINDOW,
        WA_Gadgets, glist,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE,
        WA_Activate, TRUE,
        WA_SmartRefresh, TRUE,
        WA_PubScreen, screen,
        TAG_DONE);
        
    if (!window) {
        DEBUG("Failed to create window");
        goto cleanup;
    }
    
    // Event loop
    while (!done) {
        WaitPort(window->UserPort);
        
        while ((msg = GT_GetIMsg(window->UserPort))) {
            ULONG class = msg->Class;
            UWORD code = msg->Code;
            struct Gadget *gadget = (struct Gadget *)msg->IAddress;
            
            GT_ReplyIMsg(msg);
            
            switch (class) {
                case IDCMP_CLOSEWINDOW:
                    done = TRUE;
                    break;
                    
                case IDCMP_REFRESHWINDOW:
                    GT_BeginRefresh(window);
                    GT_EndRefresh(window, TRUE);
                    break;
                    
                case IDCMP_GADGETUP:
                    switch (gadget->GadgetID) {
                        case 3: // Save
                            {
                                STRPTR hostStr, portStr;
                                
                                GT_GetGadgetAttrs(hostGad, window, NULL,
                                    GTST_String, &hostStr,
                                    TAG_DONE);
                                
                                GT_GetGadgetAttrs(portGad, window, NULL,
                                    GTST_String, &portStr,
                                    TAG_DONE);
                                
                                if (hostStr && *hostStr) {
                                    strncpy(settings->host, hostStr, MAX_HOST_LEN-1);
                                    settings->host[MAX_HOST_LEN-1] = '\0';
                                }
                                
                                if (portStr && *portStr) {
                                    unsigned int tempPort;
                                    if (sscanf(portStr, "%u", &tempPort) == 1) {
                                        settings->port = (UWORD)tempPort;
                                        DEBUG("Port set to: %u", settings->port);
                                    } else {
                                        char msg[MAX_STATUS_MSG_LEN];
                                        snprintf(msg, MAX_STATUS_MSG_LEN, 
                                               "Invalid port number, keeping current: %u", 
                                               settings->port);
                                        UpdateStatusMessage(msg);
                                        DEBUG("%s", msg);
                                    }
                                }
                                
                                if (SaveSettings(settings)) {
                                    success = TRUE;
                                    done = TRUE;
                                }
                            }
                            break;
                            
                        case 4: // Cancel
                            done = TRUE;
                            break;
                    }
                    break;
            }
        }
    }
    
cleanup:
    if (window) CloseWindow(window);
    if (glist) FreeGadgets(glist);
    if (vi) FreeVisualInfo(vi);
    
    return success;
}