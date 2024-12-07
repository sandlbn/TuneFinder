#include <clib/alib_protos.h>
#include <clib/exec_protos.h>
#include <clib/gadtools_protos.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <clib/icon_protos.h>
#include <clib/dos_protos.h>
#include <dos/dosextens.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <intuition/gadgetclass.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>
#include <proto/alib.h>
#include <proto/asl.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/wb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/hooks.h>
#include <workbench/workbench.h>
#include "../../include/amigaamp.h"
#include "../../include/gui.h"
#include "../../include/config.h"
#include "../../include/country_config.h"
#include "../../include/data.h"
#include "../../include/favorites.h"
#include "../../include/locale.h"
#include "../../include/network.h"
#include "../../include/utils.h"
#include "../../include/version.h"

#ifdef __GNUC__
extern void geta4(void);
#endif
// Menu Constants
#define MENU_PROJECT 0   // Menu number for Project menu
#define ITEM_SETTINGS 0  // for Settings
#define ITEM_FAVORITES 1 // For Favortes
#define ITEM_ABOUT 2     // for About
#define ITEM_ICONIFY 4      // for iconify  (after separator)
#define ITEM_QUIT 5      // for Quit  

// Library Handles
//struct Library *IntuitionBase = NULL;
struct Library *GadToolsBase = NULL;
struct Library *SocketBase = NULL;
struct GfxBase *GfxBase = NULL;
struct Library *AslBase = NULL;
struct Library *WorkbenchBase = NULL;

// Core GUI Structures
struct Window *window = NULL;
struct Menu *menuStrip = NULL;
void *visualInfo = NULL;
struct CountryConfig countryConfig;
struct RastPort *RastPort;
struct MsgPort *WindowPort;
struct AppIcon *appIcon = NULL;
struct MsgPort *appPort = NULL;
char *programName = NULL;
WORD savedLeftEdge = 0;
WORD savedTopEdge = 0;

// Data Structures
struct List *browserList = NULL;
struct ExtNode *currentStation;  // Track currently selected station
struct APISettings currentSettings;
struct List *savedBrowserList = NULL; 

// State Variables
BOOL running = FALSE;
BOOL isIconified = FALSE;  // Initialize state
// Gadget Elements
struct Gadget *nameStrGad;
struct Gadget *countryCodeCycle;
struct Gadget *codecCycle;
struct Gadget *tagsStrGad;
struct Gadget *hideBrokenCheckBox;
struct Gadget *httpsCheckBox;
struct Gadget *searchButton;
struct Gadget *saveButton;
struct Gadget *listView;
struct Gadget *statusMsgGad;
struct Gadget *playButton;
struct Gadget *stopButton;
struct Gadget *saveSingleButton;
struct Gadget *stationDetailGad;
struct Gadget *stationNameGad;
struct Gadget *favoriteButton;
struct Gadget *unfavoriteButton;

// Data Arrays and Choices
const char *codecChoices[] = {"", "MP3", "AAC", "AAC+", "OGG", "FLAC", NULL};

// Hooks and Menu Structures
static struct Hook renderHook;

static struct Menu *CreateAppMenus(void) {
  struct NewMenu newMenu[] = {
      {NM_TITLE, GetTFString(MSG_PROJECT), NULL, 0, 0L, NULL},
      {NM_ITEM, GetTFString(MSG_SETTINGS), "S", 0, 0L, NULL},
      {NM_ITEM, GetTFString(MSG_FAVORITES), "F", 0, 0L, NULL},
      {NM_ITEM, GetTFString(MSG_ABOUT), "?", 0, 0L, NULL},
      {NM_ITEM, NM_BARLABEL, NULL, 0, 0L, NULL},
      {NM_ITEM, GetTFString(MSG_ICONIFY), "I", 0, 0L, NULL},
      {NM_ITEM, GetTFString(MSG_QUIT), "Q", 0, 0L, NULL},
      {NM_END, NULL, NULL, 0, 0L, NULL}};

  struct Menu *menuStrip = CreateMenusA(newMenu, NULL);
  if (!menuStrip) {
    DEBUG("Failed to create menus");
    return NULL;
  }

  return menuStrip;
}

static UWORD GhostPattern[2] = {0x4444, 0x1111};

static VOID Ghost(struct RastPort *rp, UWORD pen, UWORD x0, UWORD y0, UWORD x1,
                  UWORD y1) {
  SetABPenDrMd(rp, pen, 0, JAM1);
  SetAfPt(rp, GhostPattern, 1);
  RectFill(rp, x0, y0, x1, y1);
  SetAfPt(rp, NULL, 0);
}

BOOL InitLibraries(void) {
  IntuitionBase = OpenLibrary("intuition.library", 37);
  if (!IntuitionBase) return FALSE;

  GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
  if (!GfxBase) {
    CloseLibrary(IntuitionBase);
    return FALSE;
  }

  GadToolsBase = OpenLibrary("gadtools.library", 37);
  if (!GadToolsBase) {
    CloseLibrary(IntuitionBase);
    CloseLibrary((struct Library *)GfxBase);
    return FALSE;
  }

  AslBase = OpenLibrary("asl.library", 37);
  if (!AslBase) {
    CloseLibrary(GadToolsBase);
    CloseLibrary(IntuitionBase);
    CloseLibrary((struct Library *)GfxBase);
    return FALSE;
  }

  DOSBase = OpenLibrary("dos.library", 37);
  if (!DOSBase) {
    CloseLibrary(AslBase);
    CloseLibrary(GadToolsBase);
    CloseLibrary(IntuitionBase);
    CloseLibrary((struct Library *)GfxBase);
    return FALSE;
  }

  SocketBase = OpenLibrary("bsdsocket.library", 0);
  if (!SocketBase) {
    CloseLibrary(AslBase);
    CloseLibrary(GadToolsBase);
    CloseLibrary(IntuitionBase);
    CloseLibrary((struct Library *)GfxBase);
    CloseLibrary(DOSBase);
    return FALSE;
  }
    WorkbenchBase = OpenLibrary("workbench.library", 37);
    if (!WorkbenchBase) {
        DEBUG("Failed to open workbench.library");
    CloseLibrary(AslBase);
    CloseLibrary(GadToolsBase);
    CloseLibrary(IntuitionBase);
    CloseLibrary((struct Library *)GfxBase);
    CloseLibrary(DOSBase);
            return FALSE;
    }
  return TRUE;
}

void CleanupLibraries(void) {
  if (DOSBase) CloseLibrary(DOSBase);
  if (AslBase) CloseLibrary(AslBase);
  if (GadToolsBase) CloseLibrary(GadToolsBase);
  if (IntuitionBase) CloseLibrary(IntuitionBase);
  if (SocketBase) CloseLibrary(SocketBase);
  if (GfxBase) CloseLibrary((struct Library *)GfxBase);
  if (WorkbenchBase) CloseLibrary(WorkbenchBase);
}

void CleanupGUI(void) {
  DEBUG("Starting GUI cleanup...");

  if (window) {
    if (menuStrip) {
      ClearMenuStrip(window);
      FreeMenus(menuStrip);
      menuStrip = NULL;
    }

    struct Gadget *glist = (struct Gadget *)window->UserData;
    CloseWindow(window);
    window = NULL;

    if (glist) {
      FreeGadgets(glist);
    }
  }
    if (appIcon) {
        RemoveAppIcon(appIcon);
        appIcon = NULL;
    }
    if (appPort) {
        DeleteMsgPort(appPort);
        appPort = NULL;
    }
  if (browserList) {
    free_labels(browserList);
    browserList = NULL;
    currentStation = NULL;
  }

  if (visualInfo) {
    FreeVisualInfo(visualInfo);
    visualInfo = NULL;
  }
  if (&countryConfig) {
    FreeCountryConfig(&countryConfig);
  }
  DEBUG("GUI cleanup completed");
}

struct List *CreateInitialList(void) {
  struct List *l = (struct List *)allocate(sizeof(struct List), V_List);
  if (l) NewList(l);
  return l;
}

void SaveSingleStation(struct ExtNode *station) {
  struct FileRequester *fileReq;
  char filepath[256];
  char sanitized_name[20];  // Max 20 chars for AmigaOS
  // Sanitize the station name for use as filename
  SanitizeAmigaFilename(station->displayText, sanitized_name,
                        15);  // 15 chars + ".pls"
  strcat(sanitized_name, ".pls");

  fileReq = AllocAslRequest(ASL_FileRequest, NULL);
  if (!fileReq) return;

  if (AslRequestTags(fileReq, ASLFR_DrawersOnly, FALSE, ASLFR_InitialFile,
                     sanitized_name, ASLFR_DoPatterns, TRUE,
                     ASLFR_InitialPattern, "#?.pls", TAG_DONE)) {
    strcpy(filepath, fileReq->rf_Dir);
    AddPart(filepath, fileReq->rf_File, sizeof(filepath));

    if (!strstr(filepath, ".pls")) {
      strcat(filepath, ".pls");
    }

    FILE *fp = fopen(filepath, "w");
    if (fp) {
      fprintf(fp, "[playlist]\n");
      fprintf(fp, "NumberOfEntries=1\n");
      fprintf(fp, "File1=%s\n", station->url);
      fprintf(fp, "Title1=%s\n", station->displayText);
      fprintf(fp, "Length1=-1\n");
      fclose(fp);
      DisplayBeep(NULL);
    }
  }
  FreeAslRequest(fileReq);
}

void RefreshFavoritesList(void) {
  struct List *favorites;

  // First clear current selection
  currentStation = NULL;

  // Clear station details first
  GT_SetGadgetAttrs(stationNameGad, window, NULL, GTTX_Text, "", TAG_DONE);
  GT_SetGadgetAttrs(stationDetailGad, window, NULL, GTTX_Text, "", TAG_DONE);

  // Disable all relevant buttons
  GT_SetGadgetAttrs(playButton, window, NULL, GA_Disabled, TRUE, TAG_DONE);
  GT_SetGadgetAttrs(stopButton, window, NULL, GA_Disabled, TRUE, TAG_DONE);
  GT_SetGadgetAttrs(favoriteButton, window, NULL, GA_Disabled, TRUE, TAG_DONE);
  GT_SetGadgetAttrs(unfavoriteButton, window, NULL, GA_Disabled, TRUE,
                    TAG_DONE);

  // Load new favorites list
  favorites = LoadFavorites();
  if (favorites) {
    // Clear old list
    if (browserList) {
      // First detach list from listview to prevent invalid access
      GT_SetGadgetAttrs(listView, window, NULL, GTLV_Labels, NULL, TAG_DONE);
      free_labels(browserList);
    }

    browserList = favorites;

    // Update the listview with new list
    GT_SetGadgetAttrs(listView, window, NULL, GTLV_Labels, browserList,
                      GTLV_Selected, ~0,  // Clear selection
                      TAG_DONE);

    // Force a refresh of the listview
    RefreshGList(listView, window, NULL, 1);
  }

  // Make sure window is updated
  RefreshWindowFrame(window);
}


void HandleSave(void) {
  struct FileRequester *fileReq;
  char filepath[256];
  char msg[MAX_STATUS_MSG_LEN];

  fileReq = AllocAslRequest(ASL_FileRequest, NULL);
  if (!fileReq) {
    DEBUG("Failed to create file requester");
    return;
  }

  if (AslRequestTags(fileReq, ASLFR_DrawersOnly, FALSE, ASLFR_InitialFile,
                     DEFAULT_PLS_FILENAME, ASLFR_DoPatterns, TRUE,
                     ASLFR_InitialPattern, "#?.pls", TAG_DONE)) {
    strcpy(filepath, fileReq->rf_Dir);
    AddPart(filepath, fileReq->rf_File, sizeof(filepath));

    if (!strstr(filepath, ".pls")) {
      strcat(filepath, ".pls");
    }

    if (SaveToPLS(filepath)) {
      char msg[MAX_STATUS_MSG_LEN];
      GetTFFormattedString(msg, sizeof(msg), MSG_FILE_SAVED, filepath);
      UpdateStatusMessage(msg);
    } else {
      UpdateStatusMessage(GetTFString(MSG_FAILED_FILE_SAVE));
    }
  }

  FreeAslRequest(fileReq);
}

void HandleListSelect(struct IntuiMessage *imsg) {
    char nameText[62];
    UWORD selection = imsg->Code;
    struct Node *node = browserList->lh_Head;
    while (selection-- && node->ln_Succ) {
        node = node->ln_Succ;
    }
    if (node->ln_Succ) {
        struct ExtNode *ext = (struct ExtNode *)node;
        currentStation = ext;

        // Enable play and stop buttons
        GT_SetGadgetAttrs(playButton, window, NULL, GA_Disabled, FALSE, TAG_DONE);
        GT_SetGadgetAttrs(stopButton, window, NULL, GA_Disabled, FALSE, TAG_DONE);
        
        // Set up favorite buttons based on whether station is in favorites
        BOOL inFavorites = IsStationInFavorites(ext);
        GT_SetGadgetAttrs(favoriteButton, window, NULL,
                         GA_Disabled, inFavorites,
                         TAG_DONE);
        GT_SetGadgetAttrs(unfavoriteButton, window, NULL,
                         GA_Disabled, !inFavorites,
                         TAG_DONE);
        
        // Update station info display
        snprintf(nameText, 62, "%.61s", ext->name);
        GT_SetGadgetAttrs(stationNameGad, window, NULL,
                         GTTX_Text, nameText,
                         TAG_DONE);
        
        char detailText[100];
        snprintf(detailText, sizeof(detailText), "%s: %s %s: %ld %s: %s",
                GetTFString(MSG_CODEC), ext->codec,
                GetTFString(MSG_BITRATE), ext->bitrate,
                GetTFString(MSG_COUNTRY), ext->country);
        
        GT_SetGadgetAttrs(stationDetailGad, window, NULL,
                         GTTX_Text, detailText,
                         TAG_DONE);
        
        RefreshGList(stationNameGad, window, NULL, 2);  // Refresh both gadgets
    }
}
void HandleSearch(void) {
  STRPTR nameValue = NULL;
  STRPTR stateValue = NULL;
  STRPTR tagsValue = NULL;
  LONG codec = 0;
  LONG country = 0;
  LONG is_http = 0;
  LONG broken = 0;
  char msg[MAX_STATUS_MSG_LEN];
  struct SearchParams params = {0};
  char *response = NULL;
  struct RadioStation *stations = NULL;
  int station_count = 0;
  char *urlPath = NULL;

  DEBUG("Starting search...");

  GT_GetGadgetAttrs(nameStrGad, window, NULL, GTST_String, &nameValue,
                    TAG_DONE);
  if (nameValue && *nameValue) {
    params.name = nameValue;
  }

  GT_GetGadgetAttrs(countryCodeCycle, window, NULL, GTCY_Active, &country,
                    TAG_DONE);
  if (country > 0 && country < countryConfig.count) {
    params.country_code = countryConfig.entries[country].code;
  }

  GT_GetGadgetAttrs(codecCycle, window, NULL, GTCY_Active, &codec, TAG_DONE);
  if (codec >= 0 && codecChoices[codec]) {
    params.codec = codecChoices[codec];
  }

  GT_GetGadgetAttrs(tagsStrGad, window, NULL, GTST_String, &tagsValue,
                    TAG_DONE);
  if (tagsValue && *tagsValue) {
    params.tag_list = tagsValue;
  }

  GT_GetGadgetAttrs(httpsCheckBox, window, NULL, GTCB_Checked, &is_http,
                    TAG_DONE);
  params.is_https = is_http ? 1 : 0;

  GT_GetGadgetAttrs(hideBrokenCheckBox, window, NULL, GTCB_Checked, &broken,
                    TAG_DONE);
  params.hidebroken = broken ? 1 : 0;

  params.limit = currentSettings.limit;

  urlPath = build_search_url(&currentSettings, &params);
  if (!urlPath) {
    DEBUG("%s", "Failed to build URL");
    return;
  }

  response = make_http_request(&currentSettings, urlPath);
  free(urlPath);

  if (!response) {
    UpdateStatusMessage(GetTFString(MSG_HTTP_REQ_FAILED));
    return;
  }

  stations = parse_stations_json(response, &station_count);
  free(response);

  if (!stations || station_count == 0) {
    UpdateStatusMessage(GetTFString(MSG_NO_STATION_FOUND));
    return;
  }

  if (browserList) {
    struct Node *node;
    while ((node = RemHead(browserList))) {
      struct ExtNode *ext = (struct ExtNode *)node;

      // Free all allocated strings
      if (ext->name) free(ext->name);
      if (ext->url) free(ext->url);
      if (ext->codec) free(ext->codec);
      if (ext->country) free(ext->country);

      // Free the displayText/ln_Name
      deallocate(node->ln_Name, V_cstr);

      // Free the node itself
      deallocate(node, V_Node);
    }
  }
  for (int i = 0; i < station_count; i++) {
    struct ExtNode *ext;

    char *displayText =
        FormatStationEntry(stations[i].name, stations[i].url, stations[i].codec,
                           stations[i].country, stations[i].bitrate);
    if (!displayText) continue;

    ext = AllocVec(sizeof(struct ExtNode), MEMF_CLEAR);
    if (!ext) {
      FreeVec(displayText);
      continue;
    }

    ext->displayText = displayText;
    ext->name = strdup(stations[i].name ? stations[i].name : "");
    ext->url = strdup(stations[i].url ? stations[i].url : "");
    ext->codec = strdup(stations[i].codec ? stations[i].codec
                                          : GetTFString(MSG_UNKNOWN));
    ext->country = strdup(stations[i].country ? stations[i].country : "??");
    ext->bitrate = stations[i].bitrate;
    ext->node.ln_Name = ext->displayText;
    ext->node.ln_Type = 0;
    ext->node.ln_Pri = 0;

    AddTail(browserList, (struct Node *)ext);
  }

  free_stations(stations, station_count);

  if (window && listView) {
    GT_SetGadgetAttrs(listView, window, NULL, GTLV_Labels, browserList,
                      TAG_DONE);

    char msg[MAX_STATUS_MSG_LEN];
    GetTFFormattedString(msg, sizeof(msg), MSG_FOUND_STATIONS, station_count);
    // snprintf(msg, MAX_STATUS_MSG_LEN, "Search completed. Found %d stations.",
    // station_count);
    UpdateStatusMessage(msg);
  }
}
void HandleGadgetUp(struct IntuiMessage *imsg) {
  UWORD gadgetID = ((struct Gadget *)imsg->IAddress)->GadgetID;
  char msg[MAX_STATUS_MSG_LEN];

  switch (gadgetID) {
    case 8:  // Search button
      HandleSearch();
      break;

    case 9:  // ListView
      HandleListSelect(imsg);
      break;

    case 10:  // Save all button
      HandleSave();
      break;

    case 11:  // Save Single button
      if (currentStation) {
        SaveSingleStation(currentStation);
      }
      break;

    case 12:  // Stop button
      if (IsAmigaAMPRunning()) {
        if (StopAmigaAMP()) {
          GetTFFormattedString(msg, sizeof(msg), MSG_STOPPING_PLAYBACK);
          UpdateStatusMessage(msg);
        }
      } else {
        GetTFFormattedString(msg, sizeof(msg), MSG_AMIGAAMP_NOT_RUNNING);
        UpdateStatusMessage(msg);
      }
      break;

    case 13:  // Play button
      if (currentStation) {
        if (IsAmigaAMPRunning()) {
          if (!OpenStreamInAmigaAMP(currentStation->url)) {
            GetTFFormattedString(msg, sizeof(msg), MSG_FAILED_START_PLAYBACK);
            UpdateStatusMessage(msg);
          } else {
            char nameBuffer[MAX_STATION_NAME + 1];
            cleanNonAscii(nameBuffer, currentStation->name,
                          MAX_STATION_NAME + 1);
            GetTFFormattedString(msg, sizeof(msg), MSG_PLAYING_STATION,
                                 nameBuffer);
            UpdateStatusMessage(msg);
          }
        } else {
          GetTFFormattedString(msg, sizeof(msg), MSG_AMIGAAMP_NOT_RUNNING);
          UpdateStatusMessage(msg);
        }
      }
      break;

    case 17:  // Favorite button
      if (currentStation) {
        if (SaveFavorite(currentStation)) {
          // Enable unfavorite button after successful save
          GT_SetGadgetAttrs(unfavoriteButton, window, NULL, GA_Disabled, FALSE,
                            TAG_DONE);
        }
      }
      break;

    case 18:  // Unfavorite button
      if (currentStation) {
        struct MenuItem *item = ItemAddress(
            menuStrip, FULLMENUNUM(MENU_PROJECT, ITEM_FAVORITES, NOSUB));
        if (RemoveFavorite(currentStation)) {
          // Disable unfavorite button after successful removal
          GT_SetGadgetAttrs(unfavoriteButton, window, NULL, GA_Disabled, TRUE,
                            TAG_DONE);

          // Check if we're viewing favorites and refresh if needed
          if (item && (item->Flags & CHECKED)) {
            RefreshFavoritesList();
          }
        }
      }
      break;
  }
}
void SavePreferencesOnExit(void) {
    LONG country = 0, codec = 0;
    // Get current selections
    GT_GetGadgetAttrs(countryCodeCycle, window, NULL,
                     GTCY_Active, &country,
                     TAG_DONE);
    GT_GetGadgetAttrs(codecCycle, window, NULL,
                     GTCY_Active, &codec,
                     TAG_DONE);
    SaveCyclePreferences(country, codec);
}

void HandleMenuPick(UWORD menuNumber) {
  struct MenuItem *item;
  UWORD menuNum, itemNum;

  item = ItemAddress(menuStrip, menuNumber);
  if (!item) return;

  menuNum = MENUNUM(menuNumber);
  itemNum = ITEMNUM(menuNumber);

  DEBUG("Menu selected: menu=%d, item=%d", menuNum, itemNum);

  if (menuNum == MENU_PROJECT) {
    switch (itemNum) {
      case ITEM_SETTINGS:
        DEBUG("Settings selected");
        if (CreateSettingsWindow(&currentSettings, window)) {
          UpdateStatusMessage(GetTFString(MSG_SETTINGS_SAVED));
        }
        break;

      case ITEM_ABOUT:
        DEBUG("About selected");
        {
          struct EasyStruct es = {
              sizeof(struct EasyStruct), 0, "About " VERS,
              VERS
              "\n\n"
              "Created by " AUTHOR
              "\n"
              "An Internet radio browser for AmigaOS 3.x\n\n"
              "Translations:\n" TRANSLATION,
              "OK"};
          EasyRequest(window, &es, NULL, NULL);
        }
        break;
      case ITEM_ICONIFY: // Iconify menu item
        IconifyWindow();
        break;
      case ITEM_QUIT:
        DEBUG("Quit selected");
        SavePreferencesOnExit(); 
        // Signal the main loop to quit
        if (window) {
          // Post a CLOSEWINDOW message to the window
          struct IntuiMessage *msg = (struct IntuiMessage *)AllocMem(
              sizeof(struct IntuiMessage), MEMF_CLEAR);
          if (msg) {
            msg->Class = IDCMP_CLOSEWINDOW;
            msg->IDCMPWindow = window;
            PutMsg(window->UserPort, (struct Message *)msg);
          } else {
            // If we can't allocate memory, close window directly
            CloseWindow(window);
          }
        }
        break;
        case ITEM_FAVORITES: {
          // Toggle checkmark for favorites menu item
          item->Flags ^= CHECKED;
          // If we're switching to favorites view
          
          if (item->Flags & CHECKED) {
            RefreshFavoritesList();
          }
          break;
        }
    }
  }
}

static ULONG RenderFunc(struct Hook *hook, struct Node *node,
                        struct LVDrawMsg *msg) {
  geta4();

  struct RastPort *rp;
  struct DrawInfo *drawInfo;
  UWORD *pens;
  struct ExtNode *ext;
  char buffer[32];
  char nameBuffer[MAX_STATION_NAME + 1];
  WORD x, y;
  WORD totalWidth;
  WORD nameWidth, codecWidth, bitrateWidth, countryWidth;
  UWORD bgColor, fgColor;
  WORD charWidth;

  if (!msg || msg->lvdm_MethodID != LV_DRAW) return LVCB_UNKNOWN;

  rp = msg->lvdm_RastPort;
  drawInfo = msg->lvdm_DrawInfo;
  pens = drawInfo->dri_Pens;
  ext = (struct ExtNode *)node;

  if (!rp || !drawInfo || !ext) return LVCB_UNKNOWN;

  // Set colors based on selection state
  if (msg->lvdm_State == LVR_SELECTED ||
      msg->lvdm_State == LVR_SELECTEDDISABLED) {
    bgColor = pens[FILLPEN];
    fgColor = pens[FILLTEXTPEN];
  } else {
    bgColor = pens[BACKGROUNDPEN];
    fgColor = pens[TEXTPEN];
  }

  // Set up pens and drawing mode
  SetABPenDrMd(rp, fgColor, bgColor, JAM2);
  charWidth = rp->TxWidth;
  codecWidth = charWidth * 6;
  bitrateWidth = charWidth * 6;
  countryWidth = charWidth * 4;

  // Calculate column widths
  totalWidth = msg->lvdm_Bounds.MaxX - msg->lvdm_Bounds.MinX;

  nameWidth = totalWidth - codecWidth - bitrateWidth - countryWidth;
  if (nameWidth < charWidth * 40) {  // Minimum 40 characters
    nameWidth = charWidth * 40;
  }
  // Clear the background with selection color
  SetAPen(rp, bgColor);
  RectFill(rp, msg->lvdm_Bounds.MinX, msg->lvdm_Bounds.MinY,
           msg->lvdm_Bounds.MaxX, msg->lvdm_Bounds.MaxY);

  // Set text color
  SetAPen(rp, fgColor);

  // Draw vertical separators at new positions
  x = msg->lvdm_Bounds.MinX + nameWidth;
  Move(rp, x, msg->lvdm_Bounds.MinY);
  Draw(rp, x, msg->lvdm_Bounds.MaxY);

  x += codecWidth;
  Move(rp, x, msg->lvdm_Bounds.MinY);
  Draw(rp, x, msg->lvdm_Bounds.MaxY);

  x += bitrateWidth;
  Move(rp, x, msg->lvdm_Bounds.MinY);
  Draw(rp, x, msg->lvdm_Bounds.MaxY);

  // Calculate v. centering
  y = msg->lvdm_Bounds.MinY +
      ((msg->lvdm_Bounds.MaxY - msg->lvdm_Bounds.MinY - rp->TxHeight) / 2) +
      rp->TxBaseline;

  // Draw name
  if (ext->name) {
    cleanNonAscii(nameBuffer, ext->name, MAX_STATION_NAME + 1);
    ULONG nameLen = strlen(nameBuffer);
    if (nameLen > 0) {
      DEBUG("Drawing name: %s (len: %ld)", nameBuffer, nameLen);
      Move(rp, msg->lvdm_Bounds.MinX + 4, y);
      Text(rp, nameBuffer, nameLen);
    } else {
      DEBUG("Name buffer is empty after cleaning");
    }
  }

  // Draw codec
  if (ext->codec) {
    ULONG textLength = strlen(ext->codec);
    ULONG textWidth = TextLength(rp, ext->codec, textLength);
    Move(rp, msg->lvdm_Bounds.MinX + nameWidth + codecWidth - textWidth - 4, y);
    Text(rp, ext->codec, textLength);
  }

  // Draw bitrate
  sprintf(buffer, "%ld", ext->bitrate);
  ULONG textLength = strlen(buffer);
  ULONG textWidth = TextLength(rp, buffer, textLength);
  Move(rp,
       msg->lvdm_Bounds.MinX + nameWidth + codecWidth + bitrateWidth -
           textWidth - 4,
       y);
  Text(rp, buffer, textLength);
  // Draw country
  if (ext->country) {
    ULONG textLength = strlen(ext->country);
    ULONG textWidth = TextLength(rp, ext->country, textLength);
    Move(rp,
         msg->lvdm_Bounds.MinX + nameWidth + codecWidth + bitrateWidth +
             countryWidth - textWidth - 4,
         y);
    Text(rp, ext->country, textLength);
  }

  // Ghost if disabled
  if (msg->lvdm_State == LVR_NORMALDISABLED ||
      msg->lvdm_State == LVR_SELECTEDDISABLED) {
    Ghost(rp, pens[BLOCKPEN], msg->lvdm_Bounds.MinX, msg->lvdm_Bounds.MinY,
          msg->lvdm_Bounds.MaxX, msg->lvdm_Bounds.MaxY);
  }

  return LVCB_OK;
}

BOOL OpenGUI(void) {
  struct Gadget *glist = NULL, *gad;
  struct NewGadget ng;
  struct Screen *s;
  static struct Hook renderHook;
  void *vi;
  struct List *site_labels;
  ULONG font_width, font_height, border_height;

  DEBUG("Starting GUI initialization...");

  s = LockPubScreen(NULL);
  if (!s) {
    DEBUG("Failed to lock public screen");
    return FALSE;
  }

  // Get font metrics
  font_width = s->RastPort.TxWidth;
  font_height = s->RastPort.TxHeight;
  // Get border height
  border_height = s->WBorTop + s->Font->ta_YSize + 1;

  if (!LoadCountryConfig(FULL_COUNTRY_CONFIG_PATH, &countryConfig)) {
    DEBUG("Failed to load country configuration");
    UnlockPubScreen(NULL, s);
    return FALSE;
  }
  if (countryConfig.count > 0) {
    WORD maxChars = 20;  // Base number for safety
    if (font_width > 0) {
      // Calculate max chars based on actual font width
      maxChars =
          (font_width * 15) / font_width;  // Using same width as gadget - label
    }

    for (int i = 0; i < countryConfig.count; i++) {
      if (countryConfig.entries[i].name) {
        size_t len = strlen(countryConfig.entries[i].name);
        if (len > maxChars) {
          STRPTR truncated = AllocVec(maxChars + 1, MEMF_CLEAR);
          if (truncated) {
            strncpy(truncated, countryConfig.entries[i].name, maxChars);
            truncated[maxChars] = '\0';
            countryConfig.choices[i] = truncated;
          }
        }
      }
    }
  }

  vi = GetVisualInfo(s, TAG_END);
  if (!vi) {
    DEBUG("Failed to get visual info");
    UnlockPubScreen(NULL, s);
    return FALSE;
  }

  site_labels = CreateInitialList();
  if (!site_labels) {
    DEBUG("Failed to create initial list");
    FreeVisualInfo(vi);
    UnlockPubScreen(NULL, s);
    return FALSE;
  }

  gad = CreateContext(&glist);
  if (!gad) {
    DEBUG("Failed to create gadget context");
    free_labels(site_labels);
    FreeVisualInfo(vi);
    UnlockPubScreen(NULL, s);
    return FALSE;
  }

  // Base properties
  ng.ng_TextAttr = s->Font;
  ng.ng_VisualInfo = vi;

  // Name String gadget
  ng.ng_LeftEdge = font_width * 10;  // Move right to accommodate label
  ng.ng_TopEdge = border_height + font_height;
  ng.ng_Width = font_width * 22;
  ng.ng_Height = font_height + 4;
  ng.ng_GadgetText = GetTFString(MSG_NAME);
  ng.ng_GadgetID = 1;
  ng.ng_Flags =
      PLACETEXT_LEFT | NG_HIGHLABEL;  // Add NG_HIGHLABEL for better visibility

  nameStrGad = CreateGadget(STRING_KIND, gad, &ng, GTST_MaxChars, 40, TAG_DONE);
  if (!nameStrGad) DEBUG("Failed to create name gadget");

  // Tags String gadget
  ng.ng_LeftEdge = font_width * 40;  // Adjust for second column
  ng.ng_GadgetText = GetTFString(MSG_TAGS);
  ng.ng_GadgetID = 5;

  tagsStrGad =
      CreateGadget(STRING_KIND, nameStrGad, &ng, GTST_MaxChars, 100, TAG_DONE);
  if (!tagsStrGad) DEBUG("Failed to create tags gadget");

  // Country Cycle gadget
  ng.ng_LeftEdge = font_width * 10;  // Same as name gadget
  ng.ng_TopEdge += font_height + 12;
  ng.ng_GadgetText = GetTFString(MSG_COUNTRY);
  ng.ng_GadgetID = 2;

  countryCodeCycle =
      CreateGadget(CYCLE_KIND, tagsStrGad, &ng, GTCY_Labels,
                   countryConfig.choices, GTCY_Active, 0, TAG_DONE);
  if (!countryCodeCycle) DEBUG("Failed to create country gadget");

  // Codec Cycle gadget
  ng.ng_LeftEdge = font_width * 40;  // Same as tags gadget
  ng.ng_GadgetText = GetTFString(MSG_CODEC);
  ng.ng_GadgetID = 4;

  codecCycle = CreateGadget(CYCLE_KIND, countryCodeCycle, &ng, GTCY_Labels,
                            (STRPTR *)codecChoices, GTCY_Active, 0, TAG_DONE);
  if (!codecCycle) DEBUG("Failed to create codec gadget");
    LONG savedCountry = 0, savedCodec = 0;
    if (LoadCyclePreferences(&savedCountry, &savedCodec)) {
        // Set saved country if valid
        if (savedCountry >= 0 && savedCountry < countryConfig.count) {
            GT_SetGadgetAttrs(countryCodeCycle, window, NULL,
                            GTCY_Active, savedCountry,
                            TAG_DONE);
        }
        // Set saved codec if valid
        if (savedCodec >= 0 && savedCodec < 6) {  // 6 is the number of codec choices
            GT_SetGadgetAttrs(codecCycle, window, NULL,
                            GTCY_Active, savedCodec,
                            TAG_DONE);
        }
    }
  // Checkboxes row
  ng.ng_TopEdge += font_height + 12;
  ng.ng_LeftEdge = font_width * 2;  // Move left for checkbox
  ng.ng_Width = font_width * 12;
  ng.ng_Height = font_height + 4;
  ng.ng_GadgetText = GetTFString(MSG_HTTPS_ONLY);
  ng.ng_GadgetID = 6;
  ng.ng_Flags = PLACETEXT_RIGHT;  // Text to right of checkbox

  httpsCheckBox = CreateGadget(CHECKBOX_KIND, codecCycle, &ng, GTCB_Checked,
                               FALSE, TAG_DONE);
  if (!httpsCheckBox) DEBUG("Failed to create https checkbox");

  ng.ng_LeftEdge = font_width * 18;  // Position second checkbox
  ng.ng_GadgetText = GetTFString(MSG_HIDE_BROKEN);
  ng.ng_GadgetID = 7;

  hideBrokenCheckBox = CreateGadget(CHECKBOX_KIND, httpsCheckBox, &ng,
                                    GTCB_Checked, TRUE, TAG_DONE);
  if (!hideBrokenCheckBox) DEBUG("Failed to create broken checkbox");

  // Search button
  ng.ng_LeftEdge = font_width * 52;
  ng.ng_Width = font_width * 10;
  ng.ng_GadgetText = GetTFString(MSG_SEARCH);
  ng.ng_Flags = PLACETEXT_IN;
  ng.ng_GadgetID = 8;

  searchButton = CreateGadget(BUTTON_KIND, hideBrokenCheckBox, &ng, TAG_DONE);
  if (!searchButton) DEBUG("Failed to create search button");

  // ListView
  renderHook.h_Entry = (HOOKFUNC)HookEntry;  // Standard entry stub
  renderHook.h_SubEntry = (HOOKFUNC)RenderFunc;

  ng.ng_LeftEdge = font_width * 2;
  ng.ng_TopEdge += font_height + 12;
  ng.ng_Width = font_width * 60;
  ng.ng_Height = (font_height + 4) * 8;
  ng.ng_GadgetText = NULL;
  ng.ng_GadgetID = 9;
  ng.ng_Flags = PLACETEXT_LEFT;

  listView =
      CreateGadget(LISTVIEW_KIND, searchButton, &ng, GA_Immediate, TRUE,
                   GTLV_Labels, site_labels, GA_RelVerify, TRUE, GTLV_CallBack,
                   &renderHook, GTLV_ScrollWidth, 16, GTLV_ShowSelected, NULL,
                   GTLV_ReadOnly, FALSE, TAG_DONE);
  if (!listView) DEBUG("Failed to create listview");

  // Status text (right under ListView)
  ng.ng_TopEdge += ng.ng_Height + 4;
  ng.ng_Width = font_width * 60;
  ng.ng_Height = font_height + 4;
  ng.ng_GadgetText = NULL;
  ng.ng_Flags = 0;
  ng.ng_GadgetID = 14;
  ng.ng_Flags = PLACETEXT_LEFT | NG_HIGHLABEL;

  statusMsgGad =
      CreateGadget(TEXT_KIND, listView, &ng, GTTX_Text, GetTFString(MSG_READY),
                   GTST_MaxChars, 51, TAG_DONE);
  if (!statusMsgGad) DEBUG("Failed to create status gadget");

  // Size for buttons

  WORD regularWidth = (font_width * 60 - font_width * 10) / 5;
  WORD smallWidth = (regularWidth / 2);

  // Action buttons row
  ng.ng_TopEdge += font_height + 8;
  ng.ng_Height = font_height + 6;  // Slightly taller buttons
  ng.ng_Width = regularWidth;
  ng.ng_Flags = PLACETEXT_IN;

  // Save button (first)
  ng.ng_LeftEdge = font_width * 2;
  ng.ng_GadgetText = GetTFString(MSG_SAVE);
  ng.ng_GadgetID = 10;

  saveButton = CreateGadget(BUTTON_KIND, statusMsgGad, &ng, GT_Underscore, '_',
                            TAG_DONE);
  if (!saveButton) DEBUG("Failed to create save button");

  // Save Single button (second)
  ng.ng_LeftEdge += ng.ng_Width + font_width * 2;
  ng.ng_GadgetText = GetTFString(MSG_SAVE_SINGLE);
  ng.ng_GadgetID = 11;

  saveSingleButton =
      CreateGadget(BUTTON_KIND, saveButton, &ng, GT_Underscore, '_', TAG_DONE);
  if (!saveSingleButton) DEBUG("Failed to create save single button");

  // Favorite button (third)

  ng.ng_LeftEdge += ng.ng_Width + font_width * 2;
  ng.ng_Width = smallWidth;
  ng.ng_GadgetText = "Fav+";
  ng.ng_GadgetID = 17;
  favoriteButton =
      CreateGadget(BUTTON_KIND, saveSingleButton, &ng,GT_Underscore, '_', TAG_DONE);

  // Unfavorite button

  ng.ng_LeftEdge += ng.ng_Width + font_width *2;
  ng.ng_GadgetText = "Fav-";
  ng.ng_GadgetID = 18;
  unfavoriteButton =
      CreateGadget(BUTTON_KIND, favoriteButton, &ng, GT_Underscore, '_', GA_Disabled, TRUE, TAG_DONE);

  // Stop button (fourth)
  ng.ng_LeftEdge += ng.ng_Width + font_width * 2;
  ng.ng_Width = regularWidth;
  ng.ng_GadgetText = GetTFString(MSG_STOP);
  ng.ng_GadgetID = 12;

  stopButton = CreateGadget(BUTTON_KIND, unfavoriteButton, &ng, GT_Underscore,
                            '_', GA_Disabled, TRUE, TAG_DONE);
  if (!stopButton) DEBUG("Failed to create stop button");

  // Play button (fifth)
  ng.ng_LeftEdge += ng.ng_Width + font_width * 2;
  ng.ng_GadgetText = GetTFString(MSG_PLAY);
  ng.ng_GadgetID = 13;

  playButton = CreateGadget(BUTTON_KIND, stopButton, &ng, GT_Underscore, '_',
                            GA_Disabled, TRUE, TAG_DONE);
  if (!playButton) DEBUG("Failed to create play button");

  // Station name text (first line)
  ng.ng_LeftEdge = font_width * 2;
  ng.ng_TopEdge += ng.ng_Height + 4;
  ng.ng_Width = font_width * 60;
  ng.ng_Height = font_height + 4;
  ng.ng_GadgetText = NULL;
  ng.ng_Flags = 0;
  ng.ng_GadgetID = 15;

  stationNameGad = CreateGadget(TEXT_KIND, playButton, &ng, GTTX_Text, "",
                                GTST_MaxChars, 100, TAG_DONE);
  if (!stationNameGad) DEBUG("Failed to create station name gadget");

  // Station details text (second line)
  ng.ng_TopEdge += font_height + 4;
  ng.ng_GadgetID = 16;

  stationDetailGad = CreateGadget(TEXT_KIND, stationNameGad, &ng, GTTX_Text, "",
                                  GTST_MaxChars, 100, TAG_DONE);
  if (!stationDetailGad) DEBUG("Failed to create station detail gadget");

  // labels
  ULONG window_width = font_width * 64;  // Increased to accommodate labels
  ULONG window_height = ng.ng_TopEdge + font_height + 15;

  // Center window on screen
  ULONG window_pos_x = (s->Width - window_width) / 2;
  ULONG window_pos_y = (s->Height - window_height) / 2;

  window = OpenWindowTags(
      NULL, WA_Left, window_pos_x, WA_Top, window_pos_y, WA_Width, window_width,
      WA_Height, window_height, WA_Title, "TuneFinder", 
      WA_MinWidth, font_width * 40, WA_MinHeight, font_height * 20, 
      WA_MaxWidth, ~0, WA_MaxHeight, ~0, WA_Flags,
      WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET | WFLG_ACTIVATE |
          WFLG_MENUSTATE | WFLG_NOCAREREFRESH | WFLG_NEWLOOKMENUS |
          WFLG_SMART_REFRESH,
      WA_IDCMP,
      BUTTONIDCMP | IDCMP_CLOSEWINDOW | LISTVIEWIDCMP | IDCMP_REFRESHWINDOW |
          IDCMP_GADGETUP | IDCMP_VANILLAKEY | IDCMP_MENUPICK | CYCLEIDCMP,
      WA_Gadgets, glist, WA_PubScreen, s, TAG_DONE);
  if (!window) {
    DEBUG("Failed to create window");
    goto cleanup;
  }

  // Create and setup menus
  menuStrip = CreateAppMenus();
  if (menuStrip) {
    if (!LayoutMenus(menuStrip, vi, GTMN_NewLookMenus, TRUE, TAG_DONE)) {
      FreeMenus(menuStrip);
      menuStrip = NULL;
    } else if (!SetMenuStrip(window, menuStrip)) {
      FreeMenus(menuStrip);
      menuStrip = NULL;
    }
  }

  // Store important pointers
  RastPort = window->RPort;
  WindowPort = window->UserPort;
  browserList = site_labels;
  window->UserData = (void *)glist;
  visualInfo = vi;

  // Load settings
  LoadSettings(&currentSettings);

  // Refresh window
  RefreshGList(glist, window, NULL, -1);
  GT_RefreshWindow(window, NULL);

  UnlockPubScreen(NULL, s);
  return TRUE;

cleanup:
  if (menuStrip) FreeMenus(menuStrip);
  if (glist) FreeGadgets(glist);
  if (site_labels) free_labels(site_labels);
  if (vi) FreeVisualInfo(vi);
  if (s) UnlockPubScreen(NULL, s);
  return FALSE;
}
BOOL IconifyWindow(void) {
  if (!window) return FALSE;

  // Create message port for AppIcon if not already created
  if (!appPort) {
    appPort = CreateMsgPort();
    if (!appPort) return FALSE;
  }

  // Save window position for when we un-iconify
  savedLeftEdge = window->LeftEdge;
  savedTopEdge = window->TopEdge;

  if (browserList) {
    savedBrowserList = browserList;
    browserList = NULL;
  }

  // Try to load the original icon
  struct DiskObject *icon = NULL;
  if (programName) {
    DEBUG("Trying to load icon for: %s", programName);
    icon = GetDiskObject(programName);
  }
    
  if (!icon) {
    DEBUG("Could not get program icon, using default tool icon");
    icon = GetDefDiskObject(WBTOOL);
    if (!icon) {
      DEBUG("Could not get any icon");
      return FALSE;
    }
  } else {
    DEBUG("Successfully loaded program icon");
  }

  // Close window BEFORE creating AppIcon
  if (window) {
    struct Gadget *glist = (struct Gadget *)window->UserData;
    CloseWindow(window);
    if (glist) FreeGadgets(glist);
    window = NULL;
  }

  // Create AppIcon
  appIcon = AddAppIcon(0,                      // ID number
                       (ULONG)0,               // User data
                       (UBYTE *)"TuneFinder",  // Icon text
                       appPort,                // Message port
                       0,                      // Lock
                       icon,                   // Icon
                       TAG_DONE);

  if (!appIcon) {
    FreeDiskObject(icon);
    DEBUG("Could not create AppIcon");
    return FALSE;
  }

  // Free the icon - AddAppIcon makes its own copy
  FreeDiskObject(icon);
  isIconified = TRUE;
  DEBUG("Window iconified successfully");
  return TRUE;
}

BOOL UnIconifyWindow(void) {
  if (window) return FALSE;

  if (appIcon) {
    RemoveAppIcon(appIcon);
    appIcon = NULL;
  }

  // Try to reopen the window
  if (!OpenGUI()) {
    DEBUG("Failed to reopen window");
    return FALSE;
  }
  if (savedBrowserList) {
    browserList = savedBrowserList;
    savedBrowserList = NULL;
    if (window && listView) {
      GT_SetGadgetAttrs(listView, window, NULL,
      GTLV_Labels, browserList,
      TAG_DONE);
    }
  }
  isIconified = FALSE;

  DEBUG("Window restored successfully");
  return TRUE;
}
