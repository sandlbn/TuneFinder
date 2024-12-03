#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <proto/asl.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <clib/asl_protos.h>
#include <dos/dos.h>
#include "../../include/settings.h"
#include "../../include/config.h"
#include "../../include/gui.h"
#include "../../include/utils.h"
#include "../../include/locale.h"
BOOL SaveSettings(const struct APISettings *settings) {
    char filepath[256];
    char portStr[MAX_PORT_LEN];
    char limitStr[MAX_PORT_LEN];
    BPTR file = NULL;
    BOOL success = FALSE;
    char msg[MAX_STATUS_MSG_LEN];
    
    if (!EnsureSettingsPath()) {
        return FALSE;
    }
    
    // Save host
    sprintf(filepath, TUNEFINDER_DIR ENV_HOST);
    file = Open(filepath, MODE_NEWFILE);
    if (!file) {
        char msg[MAX_STATUS_MSG_LEN];
        GetTFFormattedString(msg, sizeof(msg), MSG_FAILED_CREAT_HOST_SET_FILE, filepath);
        UpdateStatusMessage(msg);
        return FALSE;
    }
    
    LONG len = strlen(settings->host);
    if (Write(file, settings->host, len) != len) {
        UpdateStatusMessage(GetTFString(MSG_FAILED_CREAT_HOST_SET_FILE));
        Close(file);
        file = NULL;
        return FALSE;
    }
    Close(file);
    
    // Save port
    sprintf(filepath, TUNEFINDER_DIR ENV_PORT);
    file = Open(filepath, MODE_NEWFILE);
    if (!file) {
        char msg[MAX_STATUS_MSG_LEN];
        GetTFFormattedString(msg, sizeof(msg), MSG_FAILED_CREAT_PORT_FILE, filepath);
        UpdateStatusMessage(msg);
        return FALSE;
    }
    
    len = snprintf(portStr, sizeof(portStr), "%u", (unsigned int)settings->port);
    if (Write(file, portStr, len) != len) {
        UpdateStatusMessage(GetTFString(MSG_FAILED_WRITE_PORT_SET));
        Close(file);
        return FALSE;
    }
    Close(file);
    // Save limit
    sprintf(filepath, TUNEFINDER_DIR ENV_LIMIT);
    file = Open(filepath, MODE_NEWFILE);
    if (!file) {
        char msg[MAX_STATUS_MSG_LEN];
        GetTFFormattedString(msg, sizeof(msg), MSG_FAILED_CREAT_LIMIT_FILE, filepath);
        UpdateStatusMessage(msg);
        return FALSE;
    }
    
    len = snprintf(limitStr, sizeof(limitStr), "%u", (unsigned int)settings->limit);
    if (Write(file, limitStr, len) != len) {
        UpdateStatusMessage(GetTFString(MSG_FAILED_WRITE_LIMIT_SET));
        Close(file);
        return FALSE;
    }
    Close(file);

    // Save autostart program
    sprintf(filepath, TUNEFINDER_DIR ENV_AUTOSTART);
    file = Open(filepath, MODE_NEWFILE);
    if (!file) {
        char msg[MAX_STATUS_MSG_LEN];
        GetTFFormattedString(msg, sizeof(msg), MSG_FAILED_CREAT_AUTO_FILE, filepath);
        UpdateStatusMessage(msg);
        return FALSE;
    }
    len = strlen(settings->autostart);
    if (Write(file, settings->autostart, len) != len) {
        UpdateStatusMessage(GetTFString(MSG_FAILED_WRITE_AUTO_SET));
        Close(file);
        return FALSE;
    }
    Close(file);

    GetTFFormattedString(msg, sizeof(msg), MSG_SET_SAVED,  settings->host, settings->port, settings->limit);
    UpdateStatusMessage(msg);
    return TRUE;
}

BOOL LoadSettings(struct APISettings *settings) {
    BOOL hostLoaded = FALSE;
    BOOL portLoaded = FALSE;
    BOOL limitLoaded = FALSE;
    BOOL defaultProgram = FALSE;
    char filepath[256];
    char portStr[MAX_PORT_LEN];
    char limitStr[MAX_PORT_LEN];
    BPTR file;
    char msg[MAX_STATUS_MSG_LEN];
    
    // Set defaults first
    strncpy(settings->host, API_HOST, MAX_HOST_LEN-1);
    settings->host[MAX_HOST_LEN-1] = '\0';
    settings->port = API_PORT; 
    settings->limit = DEFAULT_LIMIT;
    
    // Load host
    sprintf(filepath, TUNEFINDER_DIR ENV_HOST);
    file = Open(filepath, MODE_OLDFILE);
    if (file) {
        LONG len = Read(file, settings->host, MAX_HOST_LEN-1);
        if (len > 0) {
            settings->host[len] = '\0';
            hostLoaded = TRUE;
        }
        Close(file);
    } 

    // Load port
    sprintf(filepath, TUNEFINDER_DIR ENV_PORT);
    file = Open(filepath, MODE_OLDFILE);
    if (file) {
        memset(portStr, 0, sizeof(portStr));
        LONG len = Read(file, portStr, sizeof(portStr) - 1);
        if (len > 0) {
            portStr[len] = '\0';
            unsigned int tempPort = 0;
            if (sscanf(portStr, "%u", &tempPort) == 1) {
                settings->port = (UWORD)tempPort;
                portLoaded = TRUE;
            } else {
                GetTFFormattedString(msg, sizeof(msg), MSG_INVALID_PORT, API_PORT);
                UpdateStatusMessage(msg);
            }
        }
        Close(file);
    }

    // Load limit
    sprintf(filepath, TUNEFINDER_DIR ENV_LIMIT);
    file = Open(filepath, MODE_OLDFILE);
    if (file) {
        memset(limitStr, 0, sizeof(limitStr));
        LONG len = Read(file, limitStr, sizeof(limitStr) - 1);
        if (len > 0) {
            limitStr[len] = '\0';
            int tempLimit = 0;
            if (sscanf(limitStr, "%d", &tempLimit) == 1 && tempLimit >= 0) {
                settings->limit = tempLimit;
                limitLoaded = TRUE;
            } else {
                GetTFFormattedString(msg, sizeof(msg), MSG_INVALID_PORT, DEFAULT_LIMIT);
                UpdateStatusMessage(msg);
            }
        }
        Close(file);
    }
    sprintf(filepath, TUNEFINDER_DIR ENV_AUTOSTART);
    file = Open(filepath, MODE_OLDFILE);
    if (file) {
        LONG len = Read(file, settings->autostart, MAX_PATH_LEN-1);
        if (len > 0) {
            settings->autostart[len] = '\0';
            DEBUG("Loaded autostart: %s", settings->autostart);
        }
        Close(file);
        defaultProgram = TRUE;
    } else {
        settings->autostart[0] = '\0';
    }
    
    UpdateStatusMessage(GetTFString(MSG_SETTINGS_LOADED));
    
    return (hostLoaded || portLoaded || limitLoaded || defaultProgram);
}

BOOL CreateSettingsWindow(struct APISettings *settings, struct Window *parent) {
  struct Window *window = NULL;
  struct Gadget *glist = NULL, *gad;
  struct Gadget *hostGad = NULL;
  struct Gadget *portGad = NULL;
  struct Gadget *limitGad = NULL;
  struct Gadget *autostartGad = NULL;
  struct Gadget *browseGad = NULL;

  struct NewGadget ng;
  void *vi;
  struct Screen *screen;
  BOOL done = FALSE;
  struct IntuiMessage *imsg;
  BOOL success = FALSE;
  char currentPortStr[MAX_PORT_LEN];
  char currentLimitStr[MAX_PORT_LEN];
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
  ULONG border_height = screen->WBorTop + screen->Font->ta_YSize + 1;

  // Calculate layout metrics
  WORD leftMargin = font_width * 2;              // Left margin
  WORD topMargin = border_height + font_height;  // Top margin
  WORD rowHeight = font_height + 6;              // Height of each row
  WORD rowSpacing = font_height / 2;             // Space between rows
  WORD labelWidth = font_width * 10;             // Width for labels
  WORD controlWidth = font_width * 25;           // Width for input fields
  WORD buttonWidth = font_width * 10;            // Width for buttons
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
  ng.ng_GadgetText = GetTFString(MSG_HOST);
  ng.ng_GadgetID = 1;

  hostGad = CreateGadget(STRING_KIND, gad, &ng, GTST_MaxChars, MAX_HOST_LEN - 1,
                         GTST_String, settings->host, TAG_DONE);
  if (!hostGad) {
    DEBUG("Failed to create host gadget");
    goto cleanup;
  }

  // Move to next row
  currentTop += rowHeight + rowSpacing;

  // Port input
  ng.ng_TopEdge = currentTop;
  ng.ng_Width = font_width * 10;  // Shorter width for port
  ng.ng_GadgetText = GetTFString(MSG_PORT);
  ng.ng_GadgetID = 2;

  snprintf(currentPortStr, sizeof(currentPortStr), "%u", settings->port);

  portGad =
      CreateGadget(STRING_KIND, hostGad, &ng, GTST_MaxChars, MAX_PORT_LEN - 1,
                   GTST_String, currentPortStr, TAG_DONE);
  if (!portGad) {
    DEBUG("Failed to create port gadget");
    goto cleanup;
  }

  currentTop += rowHeight + rowSpacing;

  // Limit input
  ng.ng_TopEdge = currentTop;
  ng.ng_LeftEdge = leftMargin + labelWidth;
  ng.ng_Width = font_width * 10;  // Shorter width for port
  ng.ng_GadgetText = GetTFString(MSG_LIMIT);
  ng.ng_Flags = PLACETEXT_LEFT;

  ng.ng_GadgetID = 3;

  snprintf(currentLimitStr, sizeof(currentLimitStr), "%d", settings->limit);

  limitGad =
      CreateGadget(STRING_KIND, portGad, &ng, GTST_MaxChars, MAX_PORT_LEN - 1,
                   GTST_String, currentLimitStr, TAG_DONE);
  if (!limitGad) {
    DEBUG("Failed to create limit gadget");
    goto cleanup;
  }
  // Amigaamp text input
  ng.ng_TopEdge += rowHeight + rowSpacing;
  ng.ng_Width = controlWidth - font_width * 8 - 4;
  ng.ng_GadgetText = GetTFString(MSG_AUTOSTART);
  ng.ng_GadgetID = 20;

  autostartGad =
      CreateGadget(STRING_KIND, limitGad, &ng, GTST_MaxChars, MAX_PATH_LEN - 1,
                   GTST_String, settings->autostart, TAG_DONE);

  // Add browse button
  ng.ng_LeftEdge += ng.ng_Width + 10;
  ng.ng_Width = font_width * 8;
  ng.ng_GadgetText = GetTFString(MSG_BROWSE);
  ng.ng_Flags = PLACETEXT_IN;
  ng.ng_GadgetID = 21;

  browseGad = CreateGadget(BUTTON_KIND, autostartGad, &ng, TAG_DONE);

  // Move to next row
  currentTop += rowHeight + rowSpacing;

  // Move to button row
  currentTop += rowHeight + rowSpacing * 2;  // Extra spacing before buttons

  // Save button
  ng.ng_LeftEdge = windowWidth / 2 - buttonWidth - leftMargin;
  ng.ng_TopEdge = currentTop;
  ng.ng_Width = buttonWidth;
  ng.ng_GadgetText = GetTFString(MSG_SAVE);
  ng.ng_GadgetID = 4;
  ng.ng_Flags = PLACETEXT_IN;

  gad = CreateGadget(BUTTON_KIND, browseGad, &ng, TAG_DONE);
  if (!gad) {
    DEBUG("Failed to create save button");
    goto cleanup;
  }

  // Cancel button
  ng.ng_LeftEdge = windowWidth / 2 + leftMargin;
  ng.ng_GadgetText = GetTFString(MSG_CANCEL);
  ng.ng_GadgetID = 5;

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
  window =
      OpenWindowTags(NULL, WA_Title, GetTFString(MSG_SETTINGS), WA_Left,
                     windowLeft, WA_Top, windowTop, WA_Width, windowWidth,
                     WA_Height, windowHeight, WA_IDCMP,
                     IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_REFRESHWINDOW |
                         IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS,
                     WA_Gadgets, glist, WA_DragBar, TRUE, WA_DepthGadget, TRUE,
                     WA_CloseGadget, TRUE, WA_Activate, TRUE, WA_SmartRefresh,
                     TRUE, WA_PubScreen, screen, TAG_DONE);

  if (!window) {
    DEBUG("Failed to create window");
    goto cleanup;
  }

  // Event loop
  while (!done) {
    WaitPort(window->UserPort);

    while ((imsg = GT_GetIMsg(window->UserPort))) {
      ULONG class = imsg->Class;
      UWORD code = imsg->Code;
      struct Gadget *gadget = (struct Gadget *)imsg->IAddress;

      GT_ReplyIMsg(imsg);

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
            case 4:  // Save
            {
              STRPTR hostStr, portStr, limitStr, autoStartStr;
              BOOL inputValid = TRUE;
              char msg[MAX_STATUS_MSG_LEN];

              GT_GetGadgetAttrs(hostGad, window, NULL, GTST_String, &hostStr,
                                TAG_DONE);

              GT_GetGadgetAttrs(portGad, window, NULL, GTST_String, &portStr,
                                TAG_DONE);

              GT_GetGadgetAttrs(limitGad, window, NULL, GTST_String, &limitStr,
                                TAG_DONE);
              GT_GetGadgetAttrs(autostartGad, window, NULL, GTST_String,
                                &autoStartStr, TAG_DONE);

              if (autoStartStr) {
                strncpy(settings->autostart, autoStartStr, MAX_PATH_LEN - 1);
                settings->autostart[MAX_PATH_LEN - 1] = '\0';
              }
              // Validate host
              if (!hostStr || !*hostStr || strlen(hostStr) >= MAX_HOST_LEN) {
                UpdateStatusMessage(GetTFString(MSG_INVALID_HOST));
                inputValid = FALSE;
              }

              // Validate port
              unsigned int tempPort;
              if (!portStr || sscanf(portStr, "%u", &tempPort) != 1 ||
                  tempPort == 0 || tempPort > 65535) {
                GetTFFormattedString(msg, sizeof(msg), MSG_INVALID_PORT,
                                     settings->port);
                UpdateStatusMessage(msg);
                inputValid = FALSE;
              }

              // Validate limit
              int tempLimit;
              if (!limitStr || sscanf(limitStr, "%d", &tempLimit) != 1 ||
                  tempLimit < 0) {
                GetTFFormattedString(msg, sizeof(msg), MSG_INVALID_PORT,
                                     DEFAULT_LIMIT);
                UpdateStatusMessage(msg);
                inputValid = FALSE;
              }
              if (inputValid) {
                strncpy(settings->host, hostStr, MAX_HOST_LEN - 1);
                settings->host[MAX_HOST_LEN - 1] = '\0';
                settings->port = (UWORD)tempPort;
                settings->limit = tempLimit;

                if (SaveSettings(settings)) {
                  success = TRUE;
                  done = TRUE;
                }
              }
            } break;

            case 5:  // Cancel
              done = TRUE;
              break;
            case 21:  // Browse button
            {
              struct FileRequester *fileReq =
                  AllocAslRequest(ASL_FileRequest, NULL);
              if (fileReq) {
                if (AslRequestTags(
                        fileReq, ASLFR_Window, window, ASLFR_TitleText,
                        "Select Program", ASLFR_InitialDrawer,
                        "SYS:", ASLFR_DrawersOnly, FALSE, ASLFR_DoPatterns,
                        TRUE, ASLFR_InitialPattern, "#?", TAG_DONE)) {
                  char filepath[256];
                  strcpy(filepath, fileReq->rf_Dir);
                  AddPart(filepath, fileReq->rf_File, sizeof(filepath));

                  GT_SetGadgetAttrs(autostartGad, window, NULL, GTST_String,
                                    filepath, TAG_DONE);
                }
                FreeAslRequest(fileReq);
              }
            } break;
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
BOOL SaveCyclePreferences(LONG countryCode, LONG codec) {
  char filepath[256];
  char countryStr[8];
  char codecStr[8];
  BPTR file;
  BOOL success = FALSE;

  if (!EnsureSettingsPath()) {
    return FALSE;
  }

  // Save country
  sprintf(filepath, TUNEFINDER_DIR ENV_COUNTRY);
  file = Open(filepath, MODE_NEWFILE);
  if (!file) {
    return FALSE;
  }

  sprintf(countryStr, "%ld", countryCode);
  LONG len = strlen(countryStr);
  if (Write(file, countryStr, len) != len) {
    Close(file);
    return FALSE;
  }
  Close(file);

  // Save codec
  sprintf(filepath, TUNEFINDER_DIR ENV_CODEC);
  file = Open(filepath, MODE_NEWFILE);
  if (!file) {
    return FALSE;
  }

  sprintf(codecStr, "%ld", codec);
  len = strlen(codecStr);
  if (Write(file, codecStr, len) != len) {
    Close(file);
    return FALSE;
  }
  Close(file);

  return TRUE;
}

BOOL LoadCyclePreferences(LONG *countryCode, LONG *codec) {
  char filepath[256];
  char buffer[8];
  BPTR file;
  BOOL success = FALSE;

  // Set defaults
  *countryCode = 0;
  *codec = 0;

  // Load country
  sprintf(filepath, TUNEFINDER_DIR ENV_COUNTRY);
  file = Open(filepath, MODE_OLDFILE);
  if (file) {
    LONG len = Read(file, buffer, sizeof(buffer) - 1);
    if (len > 0) {
      buffer[len] = '\0';
      *countryCode = atoi(buffer);
      success = TRUE;
    }
    Close(file);
  }

  // Load codec
  sprintf(filepath, TUNEFINDER_DIR ENV_CODEC);
  file = Open(filepath, MODE_OLDFILE);
  if (file) {
    LONG len = Read(file, buffer, sizeof(buffer) - 1);
    if (len > 0) {
      buffer[len] = '\0';
      *codec = atoi(buffer);
      success = TRUE;
    }
    Close(file);
  }

  return success;
}