#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <proto/asl.h>
#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/gadtools_protos.h>
#include <clib/alib_protos.h>
#include <clib/graphics_protos.h>
#include <proto/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <exec/memory.h>
#include <proto/graphics.h>
#include <intuition/screens.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>
#include <proto/gadtools.h>
#include <libraries/asl.h>
#include <proto/asl.h>
#include "../../include/config.h"
#include "../../include/network.h"
#include "../../include/utils.h"
#include "../../include/data.h"
#include "../../include/country_config.h"

static struct TextAttr topaz8 = { "topaz.font", 8, 0, 0 };
struct CountryConfig countryConfig;

// Global variables
BOOL running = FALSE;
struct Window *window = NULL;
struct List *browserList = NULL;
struct Library *IntuitionBase = NULL;
struct Library *GadToolsBase = NULL;
struct Library *SocketBase = NULL;
//struct Library *DOSBase = NULL;
struct GfxBase *GfxBase = NULL;
struct Library *AslBase = NULL;
void *visualInfo = NULL;
#define MENU_PROJECT     0   // Menu number for Project menu
#define ITEM_SETTINGS    0   // for Settings
#define ITEM_ABOUT       1   // for About
#define ITEM_QUIT        3   // for Quit (after separator)


struct Menu *menuStrip = NULL;
struct APISettings currentSettings;
// GUI Elements
struct Gadget *nameStrGad;
struct Gadget *countryCodeCycle;
struct Gadget *stateStrGad;
struct Gadget *codecCycle;
struct Gadget *tagsStrGad;
struct Gadget *hideBrokenCheckBox;
struct Gadget *httpsCheckBox;
struct Gadget *searchButton;
struct Gadget *saveButton;
struct Gadget *listView;
struct Gadget *statusMsgGad;

// Choices for dropdowns
const char *codecChoices[] = {"","MP3","AAC", "FLAC", NULL};
const char *countryChoices[] = {"","PL", "US", "GB", "DE", "FR", "ES", "AT", "CZ", "JP", "RU", NULL};
static struct Menu *CreateAppMenus(void) {
    struct NewMenu newMenu[] = {
        { NM_TITLE,  "Project",     NULL,     0, 0L, NULL },
        { NM_ITEM,   "Settings...", "S",      0, 0L, NULL },
        { NM_ITEM,   "About...",    "?",      0, 0L, NULL },
        { NM_ITEM,   NM_BARLABEL,   NULL,     0, 0L, NULL },
        { NM_ITEM,   "Quit",        "Q",      0, 0L, NULL },
        { NM_END,    NULL,          NULL,     0, 0L, NULL }
    };
    
    return CreateMenusA(newMenu, NULL);
}

static void ShowAboutWindow(void) {
    struct EasyStruct es = {
        sizeof(struct EasyStruct),
        0,
        "About TuneFinder",
        "TuneFinder v1.0\n\n"
        "Created by sandlbn\n"
        "An Internet radio browser for AmigaOS 3.x\n"
        "\n"
        "Current API: %s:%ld",
        "OK"
    };
    
    char message[512];
    snprintf(message, sizeof(message), es.es_TextFormat, currentSettings.host, currentSettings.port);
    es.es_TextFormat = message;
    
    EasyRequest(window, &es, NULL, NULL);
}

BOOL InitLibraries(void) {
    IntuitionBase = OpenLibrary("intuition.library", 40);
    if (!IntuitionBase) return FALSE;
    
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 40);
    if (!GfxBase) {
        CloseLibrary(IntuitionBase);
        return FALSE;
    }
    
    GadToolsBase = OpenLibrary("gadtools.library", 40);
    if (!GadToolsBase) {
        CloseLibrary(IntuitionBase);
        CloseLibrary((struct Library *)GfxBase);
        return FALSE;
    }
    
    AslBase = OpenLibrary("asl.library", 40);
    if (!AslBase) {
        CloseLibrary(GadToolsBase);
        CloseLibrary(IntuitionBase);
        CloseLibrary((struct Library *)GfxBase);
        return FALSE;
    }
    
    DOSBase = OpenLibrary("dos.library", 40);
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
    
    return TRUE;
}

void CleanupLibraries(void) {
    if (DOSBase) CloseLibrary(DOSBase);
    if (AslBase) CloseLibrary(AslBase);
    if (GadToolsBase) CloseLibrary(GadToolsBase);
    if (IntuitionBase) CloseLibrary(IntuitionBase);
    if (SocketBase) CloseLibrary(SocketBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
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
    
    if (browserList) {
        free_labels(browserList);
        browserList = NULL;
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



struct List* CreateInitialList(void) {
    struct List* l = (struct List *)allocate(sizeof(struct List), V_List);
    if (l) NewList(l);
    return l;
}

void SaveSingleStation(struct ExtNode *station) {
    struct FileRequester *fileReq;
    char filepath[256];
    char sanitized_name[20];  // Max 20 chars for AmigaOS
    // Sanitize the station name for use as filename
    SanitizeAmigaFilename(station->displayText, sanitized_name, 15);  // 15 chars + ".pls"
    strcat(sanitized_name, ".pls");
    
    fileReq = AllocAslRequest(ASL_FileRequest, NULL);
    if (!fileReq) return;
    
    if (AslRequestTags(fileReq,
        ASLFR_DrawersOnly, FALSE,
        ASLFR_InitialFile, sanitized_name,
        ASLFR_DoPatterns, TRUE,
        ASLFR_InitialPattern, "#?.pls",
        TAG_DONE)) {

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

struct Window* OpenDetailsWindow(struct ExtNode *station) {
    struct Screen *screen = LockPubScreen(NULL);
    struct Window *detailWindow;
    struct Gadget *glist = NULL, *gad;
    struct NewGadget ng;
    void *vi;
    WORD windowWidth = 400;
    WORD windowHeight = 200;
    
    if (!screen) return NULL;
    
    WORD leftEdge = (screen->Width - windowWidth) / 2;
    WORD topEdge = (screen->Height - windowHeight) / 2;
    
    vi = GetVisualInfo(screen, TAG_DONE);
    if (!vi) {
        UnlockPubScreen(NULL, screen);
        return NULL;
    }
    
    gad = CreateContext(&glist);
    if (!gad) {
        FreeVisualInfo(vi);
        UnlockPubScreen(NULL, screen);
        return NULL;
    }
    
    ng.ng_LeftEdge = windowWidth/2 - 40;
    ng.ng_TopEdge = windowHeight - 40;
    ng.ng_Width = 80;
    ng.ng_Height = 15;
    ng.ng_GadgetText = "Save";
    ng.ng_TextAttr = screen->Font;
    ng.ng_GadgetID = 1;
    ng.ng_Flags = PLACETEXT_IN;
    ng.ng_VisualInfo = vi;
    
    gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_DONE);
    if (!gad) {
        FreeGadgets(glist);
        FreeVisualInfo(vi);
        UnlockPubScreen(NULL, screen);
        return NULL;
    }
    
    detailWindow = OpenWindowTags(NULL,
        WA_Title, "Station Details",
        WA_Left, leftEdge,
        WA_Top, topEdge,
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
        
    if (!detailWindow) {
        FreeGadgets(glist);
        FreeVisualInfo(vi);
        UnlockPubScreen(NULL, screen);
        return NULL;
    }
    
    detailWindow->UserData = (void*)station;
    
    struct RastPort *rp = detailWindow->RPort;
    WORD textY = detailWindow->BorderTop + 20;
    
    SetAPen(rp, 1);
    
    Move(rp, 20, textY);
    Text(rp, "Name:", 5);
    Move(rp, 100, textY);
    Text(rp, station->displayText, strlen(station->displayText));
    
    textY += 20;
    Move(rp, 20, textY);
    Text(rp, "URL:", 4);
    Move(rp, 100, textY);
    Text(rp, station->url, strlen(station->url));
    
    textY += 20;
    Move(rp, 20, textY);
    Text(rp, "Codec:", 6);
    Move(rp, 100, textY);
    Text(rp, station->codec, strlen(station->codec));
    
    textY += 20;
    Move(rp, 20, textY);
    Text(rp, "Bitrate:", 8);
    char bitrateBuf[20];
    sprintf(bitrateBuf, "%d kbps", station->bitrate);
    Move(rp, 100, textY);
    Text(rp, bitrateBuf, strlen(bitrateBuf));
    
    UnlockPubScreen(NULL, screen);
    return detailWindow;
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
    
    if (AslRequestTags(fileReq,
        ASLFR_DrawersOnly,    FALSE,
        ASLFR_InitialFile,    DEFAULT_PLS_FILENAME,
        ASLFR_DoPatterns,     TRUE,
        ASLFR_InitialPattern, "#?.pls",
        TAG_DONE)) {
        
        strcpy(filepath, fileReq->rf_Dir);
        AddPart(filepath, fileReq->rf_File, sizeof(filepath));
        
        if (!strstr(filepath, ".pls")) {
            strcat(filepath, ".pls");
        }
        
        if (SaveToPLS(filepath)) {
            snprintf(msg, MAX_STATUS_MSG_LEN, "File saved: %s", filepath);
            UpdateStatusMessage(msg);
        } else {
            UpdateStatusMessage("Failed to save file");
        }
    }
    
    FreeAslRequest(fileReq);
}

void HandleListSelect(struct IntuiMessage *imsg) {
    UWORD selection = imsg->Code;
    struct Node *node = browserList->lh_Head;
    struct Window *detailWindow;
    struct IntuiMessage *msg;
    BOOL done = FALSE;
    
    while (selection-- && node->ln_Succ) {
        node = node->ln_Succ;
    }
    
    if (node->ln_Succ) {
        struct ExtNode *ext = (struct ExtNode *)node;
        detailWindow = OpenDetailsWindow(ext);
        if (!detailWindow) return;
        
        while (!done) {
            WaitPort(detailWindow->UserPort);
            
            while ((msg = GT_GetIMsg(detailWindow->UserPort))) {
                switch(msg->Class) {
                    case IDCMP_CLOSEWINDOW:
                        done = TRUE;
                        break;
                        
                    case IDCMP_GADGETUP:
                        SaveSingleStation(ext);
                        break;
                        
                    case IDCMP_REFRESHWINDOW:
                        GT_BeginRefresh(detailWindow);
                        GT_EndRefresh(detailWindow, TRUE);
                        break;
                }
                GT_ReplyIMsg(msg);
            }
        }
        
        CloseWindow(detailWindow);
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
    
    GT_GetGadgetAttrs(nameStrGad, window, NULL,
        GTST_String, &nameValue,
        TAG_DONE);
    if (nameValue && *nameValue) {
        params.name = nameValue;
    }
    
    GT_GetGadgetAttrs(countryCodeCycle, window, NULL,
        GTCY_Active, &country,
        TAG_DONE);
    if (country > 0 && country < countryConfig.count) {
        params.country_code = countryConfig.entries[country].code;
    }
    
    GT_GetGadgetAttrs(codecCycle, window, NULL,
        GTCY_Active, &codec,
        TAG_DONE);
    if (codec >= 0 && codecChoices[codec]) {
        params.codec = codecChoices[codec];
    }
    
    GT_GetGadgetAttrs(tagsStrGad, window, NULL,
        GTST_String, &tagsValue,
        TAG_DONE);
    if (tagsValue && *tagsValue) {
        params.tag_list = tagsValue;
    }
    
    GT_GetGadgetAttrs(httpsCheckBox, window, NULL,
        GTCB_Checked, &is_http,
        TAG_DONE);
    params.is_https = is_http ? 1 : 0;
    
    GT_GetGadgetAttrs(hideBrokenCheckBox, window, NULL,
        GTCB_Checked, &broken,
        TAG_DONE);
    params.hidebroken = broken ? 1 : 0;
    
    params.limit = DEFAULT_LIMIT;
    
    urlPath = build_search_url(&params);
    if (!urlPath) {
        UpdateStatusMessage("Failed to build URL");
        return;
    }
    
    response = make_http_request(&currentSettings, urlPath);
    free(urlPath);
    
    if (!response) {
        UpdateStatusMessage("HTTP request failed");
        return;
    }
    
    stations = parse_stations_json(response, &station_count);
    free(response);
    
    if (!stations || station_count == 0) {
        UpdateStatusMessage("No stations found");
        return;
    }
    
    if (browserList) {
        struct Node *node;
        while ((node = RemHead(browserList))) {
            deallocate(node->ln_Name, V_cstr);
            deallocate(node, V_Node);
        }
    }
    
    for (int i = 0; i < station_count; i++) {
        struct ExtNode *ext;
        char *displayText;
        
        displayText = FormatStationEntry(
            stations[i].name,
            stations[i].url,
            stations[i].codec,
            stations[i].bitrate
        );
        
        if (!displayText) continue;
        
        ext = AllocVec(sizeof(struct ExtNode), MEMF_CLEAR);
        if (!ext) {
            FreeVec(displayText);
            continue;
        }
        
        ext->displayText = displayText;
        ext->url = strdup(stations[i].url ? stations[i].url : "");
        ext->codec = strdup(stations[i].codec ? stations[i].codec : "Unknown");
        ext->bitrate = stations[i].bitrate;
        ext->node.ln_Name = ext->displayText;
        ext->node.ln_Type = 0;
        ext->node.ln_Pri = 0;
        
        AddTail(browserList, (struct Node *)ext);
    }
    
    free_stations(stations, station_count);
    
    if (window && listView) {
        GT_SetGadgetAttrs(listView, window, NULL,
            GTLV_Labels, browserList,
            TAG_DONE);
        
        snprintf(msg, MAX_STATUS_MSG_LEN, "Search completed. Found %d stations.", station_count);
        UpdateStatusMessage(msg);
    }
}
void HandleGadgetUp(struct IntuiMessage *imsg) {
    UWORD gadgetID = ((struct Gadget *)imsg->IAddress)->GadgetID;
    
    switch(gadgetID) {
        case 8:
            HandleSearch();
            break;
            
        case 9:
            HandleListSelect(imsg);
            break;
            
        case 10:
            HandleSave();
            break;
    }
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
                    char msg[MAX_STATUS_MSG_LEN];
                    snprintf(msg, MAX_STATUS_MSG_LEN, "Settings saved: %s:%d", 
                        currentSettings.host, currentSettings.port);
                    UpdateStatusMessage(msg);
                }
                break;
                
            case ITEM_ABOUT:
                DEBUG("About selected");
                {
                    struct EasyStruct es = {
                        sizeof(struct EasyStruct),
                        0,
                        "About TuneFinder",
                        "TuneFinder v1.0\n\n"
                        "Created by sandlbn\n"
                        "An Internet radio browser for AmigaOS 3.x",
                        "OK"
                    };
                    EasyRequest(window, &es, NULL, NULL);
                }
                break;
                
            case ITEM_QUIT:
                DEBUG("Quit selected");
                // Signal the main loop to quit
                if (window) {
                    // Post a CLOSEWINDOW message to the window
                    struct IntuiMessage *msg = (struct IntuiMessage *)
                        AllocMem(sizeof(struct IntuiMessage), MEMF_CLEAR);
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
        }
    }
}
BOOL OpenGUI(void) {
   struct Gadget *glist = NULL, *gad;
    struct NewGadget ng;
    struct Screen *s;
    void *vi;
    struct List *site_labels;
    WORD leftEdge = 30;
    WORD topEdge = 30;
    
    DEBUG("Starting GUI initialization...");
    
    s = LockPubScreen(NULL);
    if (!s) {
        DEBUG("Failed to lock public screen");
        return FALSE;
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
    
     // Name input field
    ng.ng_LeftEdge = leftEdge;
    ng.ng_TopEdge = topEdge + 5;
    ng.ng_Width = (WINDOW_WIDTH - 80) / 2;
    ng.ng_Height = 15;
    ng.ng_GadgetText = "Name";
    ng.ng_TextAttr = s->Font;
    ng.ng_GadgetID = 1;
    ng.ng_Flags = PLACETEXT_ABOVE;
    ng.ng_VisualInfo = vi;
    
    gad = CreateGadget(STRING_KIND, gad, &ng,
        GTST_MaxChars, 40,
        GTST_String, "",
        TAG_DONE);
    if (!gad) {
        DEBUG("Failed to create Name gadget");
        goto cleanup;
    }
    nameStrGad = gad;
    
    // Tags input
    ng.ng_LeftEdge = leftEdge + ng.ng_Width + 40;
    ng.ng_GadgetText = "Tags";
    ng.ng_GadgetID = 5;
    ng.ng_Flags = PLACETEXT_ABOVE;
    
    gad = CreateGadget(STRING_KIND, gad, &ng,
        GTST_MaxChars, 100,
        GTTX_Text, "",
        TAG_DONE);
    if (!gad) {
        DEBUG("Failed to create Tags gadget");
    }
    if (!gad) goto cleanup;
    tagsStrGad = gad;
    // Load country configuration
    if (!LoadCountryConfig(COUNTRY_CONFIG_FILE, &countryConfig)) {
        DEBUG("Failed to load country configuration");
        goto cleanup;
    }
    // Country dropdown
    ng.ng_LeftEdge = leftEdge;
    ng.ng_TopEdge += 30;
    ng.ng_GadgetText = "Country";
    ng.ng_GadgetID = 2;
    ng.ng_Flags = PLACETEXT_ABOVE;
    
    gad = CreateGadget(CYCLE_KIND, gad, &ng,
        GTCY_Labels, countryConfig.choices,  
        GTCY_Active, 0,
        TAG_DONE);
    
    if (!gad) {
        DEBUG("Failed to create Country gadget");
    }
    if (!gad) goto cleanup;

    countryCodeCycle = gad;
    
    // Codec dropdown
    ng.ng_LeftEdge = leftEdge + ng.ng_Width + 40;
    ng.ng_GadgetText = "Codec";
    ng.ng_GadgetID = 4;
    ng.ng_Flags = PLACETEXT_ABOVE;
    
    gad = CreateGadget(CYCLE_KIND, gad, &ng,
        GTCY_Labels, (STRPTR *)codecChoices,
        GTCY_Active, 0,
        TAG_DONE);
    if (!gad) {
        DEBUG("Failed to create Codec gadget");
    }
    if (!gad) goto cleanup;

    codecCycle = gad;
    
    // HTTPS checkbox
    ng.ng_LeftEdge = leftEdge;
    ng.ng_TopEdge += 30;
    ng.ng_Width = 120;
    ng.ng_Height = 20;
    ng.ng_GadgetText = "HTTPS Only";
    ng.ng_GadgetID = 6;
    ng.ng_Flags = PLACETEXT_RIGHT;
    
    gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, FALSE,
        TAG_DONE);
    if (!gad) {
        DEBUG("Failed to create HTTPS gadget");
    }
    if (!gad) goto cleanup;
    httpsCheckBox = gad;
    
    // Hide Broken checkbox
    ng.ng_LeftEdge = leftEdge + 150;
    ng.ng_GadgetText = "Hide Broken";
    ng.ng_GadgetID = 7;
    
    gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, TRUE,
        TAG_DONE);
    if (!gad) {
        DEBUG("Failed to create Hide Broken gadget");
    }
    if (!gad) goto cleanup;
    hideBrokenCheckBox = gad;
    
    // Search Button
    ng.ng_LeftEdge = WINDOW_WIDTH - 120;
    ng.ng_TopEdge = ng.ng_TopEdge;
    ng.ng_Width = 100;
    ng.ng_Height = 15;
    ng.ng_GadgetText = "Search";
    ng.ng_GadgetID = 8;
    ng.ng_Flags = PLACETEXT_IN;
    
    gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_DONE);
    if (!gad) {
        DEBUG("Failed to create Search button gadget");
    }
    if (!gad) goto cleanup;
    searchButton = gad;
    
    // Results List
    ng.ng_LeftEdge = leftEdge;
    ng.ng_TopEdge += 30;
    ng.ng_Width = WINDOW_WIDTH - 40;
    ng.ng_Height = WINDOW_HEIGHT - ng.ng_TopEdge - 80;
    ng.ng_GadgetText = NULL;
    ng.ng_GadgetID = 9;
    
    gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
        GTLV_ReadOnly, FALSE,
        GTLV_Labels, site_labels,
        TAG_DONE);
    if (!gad) {
        DEBUG("Failed to create ListView gadget");
    }
    if (!gad) goto cleanup;
    listView = gad;
    
    // Save Button
    ng.ng_LeftEdge = WINDOW_WIDTH - 120;
    ng.ng_TopEdge += ng.ng_Height + 10;
    ng.ng_Width = 100;
    ng.ng_Height = 15;
    ng.ng_GadgetText = "Save";
    ng.ng_Flags = PLACETEXT_IN;
    ng.ng_GadgetID = 10;
    
    gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_DONE);
    if (!gad) {
        DEBUG("Failed to create Save gadget");
    }
    if (!gad) goto cleanup;
    saveButton = gad;
    
    // Status Message
    ng.ng_LeftEdge = leftEdge;
    ng.ng_TopEdge -= 5;
    ng.ng_Width = WINDOW_WIDTH - 180;
    ng.ng_Height = 15;
    ng.ng_GadgetText = NULL;
    ng.ng_Flags = 0;
    ng.ng_GadgetID = 11;
    
    gad = CreateGadget(TEXT_KIND, gad, &ng,
        GTTX_Text, "Ready",
        TAG_DONE);
    if (!gad) {
        DEBUG("Failed to create MSG gadget");
    }
    if (!gad) goto cleanup;
    statusMsgGad = gad;
    
    window = OpenWindowTags(NULL,
        WA_Left, 100,
        WA_Top, 10,
        WA_Width, WINDOW_WIDTH,
        WA_Height, WINDOW_HEIGHT,
        WA_IDCMP, IDCMP_CLOSEWINDOW | 
                  IDCMP_REFRESHWINDOW | 
                  IDCMP_GADGETUP | 
                  IDCMP_MENUPICK |  // Make sure this is included
                  CYCLEIDCMP | 
                  LISTVIEWIDCMP,
        WA_Flags, WFLG_DRAGBAR |
                  WFLG_DEPTHGADGET |
                  WFLG_CLOSEGADGET |
                  WFLG_ACTIVATE |
                  WFLG_NOCAREREFRESH |
                  WFLG_SMART_REFRESH |
                  WFLG_MENUSTATE,   // Add this flag
        WA_Gadgets, glist,
        WA_Title, "TuneFinder by sandlbn",
        WA_PubScreen, s,
        TAG_DONE);
        
    if (!window) {
        DEBUG("Failed to create Window");
        goto cleanup;
    }
    if (!window) {
        DEBUG("Failed to create Window");
        goto cleanup;
    }

    // Create and set up menus after window creation
    menuStrip = CreateAppMenus();
    if (menuStrip) {
        // Make sure to enable menus
        ModifyIDCMP(window, window->IDCMPFlags | IDCMP_MENUPICK);
        if (!SetMenuStrip(window, menuStrip)) {
            DEBUG("Failed to set menu strip");
            FreeMenus(menuStrip);
            menuStrip = NULL;
        }
    }
    
    browserList = site_labels;
    window->UserData = (void *)glist;
    visualInfo = vi;  
    
    // Initialize window
    RefreshGList(glist, window, NULL, -1);
    GT_RefreshWindow(window, NULL);
    
    // Add menu strip after window creation
    menuStrip = CreateAppMenus();
    if (menuStrip) {
        if (!LayoutMenus(menuStrip, vi, TAG_DONE)) {
            DEBUG("Failed to layout menus");
            FreeMenus(menuStrip);
            menuStrip = NULL;
        } else if (!SetMenuStrip(window, menuStrip)) {
            DEBUG("Failed to set menu strip");
            FreeMenus(menuStrip);
            menuStrip = NULL;
        }
    }

    
    // Load settings
    LoadSettings(&currentSettings);
    
    UnlockPubScreen(NULL, s);
    
    DEBUG("GUI initialization completed successfully");
    return TRUE;

cleanup:
    if (menuStrip) {
        FreeMenus(menuStrip);
        menuStrip = NULL;
    }
    if (glist) {
        FreeGadgets(glist);
    }
    if (site_labels) {
        free_labels(site_labels);
    }
    if (vi) {
        FreeVisualInfo(vi);
    }
    if (s) {
        UnlockPubScreen(NULL, s);
    }
    DEBUG("GUI initialization failed");
    return FALSE;
}
    