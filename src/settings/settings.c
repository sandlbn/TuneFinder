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
    sprintf(filepath, ENV_PATH ENV_HOST);
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
        DEBUG("%s", msg);
        Close(file);
        return FALSE;
    }
    Close(file);
    
    // Save port
    sprintf(filepath, ENV_PATH ENV_PORT);
    file = Open(filepath, MODE_NEWFILE);
    if (!file) {
        snprintf(msg, MAX_STATUS_MSG_LEN, "Failed to create port settings file: %s", filepath);
        UpdateStatusMessage(msg);
        DEBUG("%s", msg);
        return FALSE;
    }
    
    len = snprintf(portStr, sizeof(portStr), "%u", (unsigned int)settings->port);
    if (Write(file, portStr, len) != len) {
        snprintf(msg, MAX_STATUS_MSG_LEN, "Failed to write port setting");
        UpdateStatusMessage(msg);
        DEBUG("%s", msg);
        Close(file);
        return FALSE;
    }
    Close(file);
    
    snprintf(msg, MAX_STATUS_MSG_LEN, "Settings saved: %s:%u", settings->host, settings->port);
    UpdateStatusMessage(msg);
    DEBUG("%s", msg);
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
    sprintf(filepath, ENV_PATH ENV_HOST);
    file = Open(filepath, MODE_OLDFILE);
    if (file) {
        LONG len = Read(file, settings->host, MAX_HOST_LEN-1);
        if (len > 0) {
            settings->host[len] = '\0';
            success = TRUE;
        }
        Close(file);
    }
    
    // Load port setting
    sprintf(filepath, ENV_PATH ENV_PORT);
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
                snprintf(msg, MAX_STATUS_MSG_LEN, "Invalid port number in settings, using default: %d", API_PORT);
                UpdateStatusMessage(msg);
                DEBUG("%s", msg);
                settings->port = API_PORT;
            }
        }
        Close(file);
    }
    
    snprintf(msg, MAX_STATUS_MSG_LEN, "Settings loaded: %s:%u", settings->host, settings->port);
    UpdateStatusMessage(msg);
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
    
    screen = parent->WScreen;
    vi = GetVisualInfo(screen, TAG_DONE);
    if (!vi) return FALSE;
    
    gad = CreateContext(&glist);
    if (!gad) {
        FreeVisualInfo(vi);
        return FALSE;
    }
    
    // Host input
    ng.ng_LeftEdge = 20;
    ng.ng_TopEdge = 20;
    ng.ng_Width = 250;
    ng.ng_Height = 15;
    ng.ng_GadgetText = "API Host";
    ng.ng_TextAttr = screen->Font;
    ng.ng_GadgetID = 1;
    ng.ng_Flags = PLACETEXT_RIGHT;
    ng.ng_VisualInfo = vi;
    
    gad = CreateGadget(STRING_KIND, gad, &ng,
        GTST_MaxChars, MAX_HOST_LEN-1,
        GTST_String, settings->host,
        TAG_DONE);
    if (!gad) goto cleanup;
    hostGad = gad;   
    
    // Port input
    ng.ng_TopEdge += 30;
    ng.ng_Width = 100;
    ng.ng_GadgetText = "API Port";
    ng.ng_GadgetID = 2;
    
    snprintf(currentPortStr, sizeof(currentPortStr), "%u", settings->port);
    DEBUG("Current port string: %s", currentPortStr);
    
    gad = CreateGadget(STRING_KIND, gad, &ng,
        GTST_MaxChars, MAX_PORT_LEN-1,
        GTST_String, currentPortStr,
        TAG_DONE);
    if (!gad) goto cleanup;
    portGad = gad;  // Store port gadget pointer
    
    // Save button
    ng.ng_TopEdge += 40;
    ng.ng_Width = 80;
    ng.ng_GadgetText = "Save";
    ng.ng_GadgetID = 3;
    ng.ng_Flags = PLACETEXT_IN;
    
    gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_DONE);
    if (!gad) goto cleanup;
    
    // Cancel button
    ng.ng_LeftEdge += 100;
    ng.ng_GadgetText = "Cancel";
    ng.ng_GadgetID = 4;
    
    gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_DONE);
    if (!gad) goto cleanup;
    
    window = OpenWindowTags(NULL,
        WA_Title, "API Settings",
        WA_Left, parent->LeftEdge + 50,
        WA_Top, parent->TopEdge + 50,
        WA_Width, 350,
        WA_Height, 120,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP,
        WA_Gadgets, glist,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE,
        WA_Activate, TRUE,
        WA_SmartRefresh, TRUE,
        WA_PubScreen, screen,
        TAG_DONE);
        
    if (!window) goto cleanup;
    
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
                                        snprintf(msg, MAX_STATUS_MSG_LEN, "Invalid port number, keeping current: %u", settings->port);
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