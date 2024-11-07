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
#include <proto/gadtools.h>
#include "../../include/config.h"
#include "../../include/network.h"
#include "../../include/gui_layout.h"
#include "../../include/utils.h"
#include "../../include/data.h"
#include "../../include/country_config.h"
#include "../../include/amigaamp.h"

struct CountryConfig countryConfig;
struct RastPort *RastPort;
struct MsgPort *WindowPort;

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
struct Gadget *codecCycle;
struct Gadget *tagsStrGad;
struct Gadget *hideBrokenCheckBox;
struct Gadget *httpsCheckBox;
struct Gadget *searchButton;
struct Gadget *saveButton;
struct Gadget *listView;
struct Gadget *statusMsgGad;
struct GadgetMetrics {
	WORD leftMargin;
	WORD topMargin;
	WORD gadgetHeight;
	WORD gadgetSpacing;
	WORD columnWidth;
	WORD labelWidth;
	WORD labelSpacing;
	WORD checkboxSpacing;
	WORD buttonWidth;
	WORD checkboxWidth;
	WORD cycleWidth;
};

// Choices for dropdowns
const char *codecChoices[] = {"","MP3","AAC", "AAC+", "FLAC", NULL};
static void SetupGadgetPosition(struct NewGadget *ng, const struct GadgetMetrics *metrics,
                                const char *text, BOOL isFirstInRow) {
	if (isFirstInRow) {
		ng->ng_LeftEdge = metrics->leftMargin;
	} else {
		ng->ng_LeftEdge = metrics->leftMargin * 2 + metrics->columnWidth;
	}

	if (text && ng->ng_Flags & PLACETEXT_ABOVE) {
		ng->ng_TopEdge += metrics->labelSpacing;
	}
}

static struct Menu *CreateAppMenus(void) {
	struct NewMenu newMenu[] = {
		{ NM_TITLE,  "Project",     NULL,     0, 0L, NULL },
		{ NM_ITEM,   "Settings...", "S",      0, 0L, NULL },
		{ NM_ITEM,   "About...",    "?",      0, 0L, NULL },
		{ NM_ITEM,   NM_BARLABEL,   NULL,     0, 0L, NULL },
		{ NM_ITEM,   "Quit",        "Q",      0, 0L, NULL },
		{ NM_END,    NULL,          NULL,     0, 0L, NULL }
	};

	struct Menu *menuStrip = CreateMenusA(newMenu, NULL);
	if (!menuStrip) {
		DEBUG("Failed to create menus");
		return NULL;
	}

	return menuStrip;
}
void DrawWrappedText(struct RastPort *rp, STRPTR text, WORD xStart, WORD *yPos, WORD maxWidth) {
	WORD textLen = strlen(text);
	WORD charWidth;
	WORD totalWidth = 0;
	WORD lastSpace = -1;
	WORD lineStart = 0;
	WORD i;

	for (i = 0; i < textLen; i++) {
		charWidth = TextLength(rp, &text[i], 1);

		// Check if adding this character would exceed maxWidth
		if (totalWidth + charWidth > maxWidth || text[i] == '\n') {
			// If we found a space in this line, break there
			if (lastSpace != -1) {
				Move(rp, xStart, *yPos);
				Text(rp, &text[lineStart], lastSpace - lineStart);
				lineStart = lastSpace + 1;
				*yPos += 14; // Line height
				totalWidth = 0;
				i = lastSpace;  // Resume from after the last space
				lastSpace = -1;
			} else {
				// No space found, force break
				Move(rp, xStart, *yPos);
				Text(rp, &text[lineStart], i - lineStart);
				lineStart = i;
				*yPos += 14;
				totalWidth = 0;
			}
		} else {
			if (text[i] == ' ') {
				lastSpace = i;
			}
			totalWidth += charWidth;
		}
	}

	// Draw remaining text
	if (lineStart < textLen) {
		Move(rp, xStart, *yPos);
		Text(rp, &text[lineStart], textLen - lineStart);
		*yPos += 14;
	}
}

static void CalculateMetrics(struct Screen *s, struct GadgetMetrics *metrics) {
	struct TextFont *font = s->RastPort.Font;  // Use RastPort's font instead
	WORD baseUnit;

	if (!font) {
		// Fallback if no font is set
		baseUnit = 8;  // Default minimum size
		DEBUG("No font found, using default size");
	} else {
		baseUnit = font->tf_YSize;
		DEBUG("Font height: %d", baseUnit);
	}
	// Basic spacing
	metrics->leftMargin = baseUnit * 2;
	metrics->topMargin = baseUnit * 2;
	metrics->gadgetHeight = baseUnit + 4;
	metrics->gadgetSpacing = baseUnit;

	// Important: Make label width wider
	metrics->labelWidth = baseUnit * 8;  // More space for label text
	metrics->checkboxSpacing = baseUnit * 8;
	// Calculate effective widths
	WORD availableWidth = WINDOW_WIDTH - (metrics->leftMargin * 3);  // Three margins: left, between columns, right
	metrics->columnWidth = (availableWidth - metrics->labelWidth * 2) / 2;  // Account for two label spaces
	metrics->buttonWidth = baseUnit * 10;
	metrics->checkboxWidth = baseUnit * 12;  // Width including text

	DEBUG("Metrics: text height=%d, label width=%d, column width=%d",
	      baseUnit, metrics->labelWidth, metrics->columnWidth);
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
    WORD windowHeight = 250;  // Increased height for wrapped text
    
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
    
    // Save button
    ng.ng_LeftEdge = windowWidth/2 - 90;
    ng.ng_TopEdge = windowHeight - 30;  // Adjusted position
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
        // AmigaAMP button
    ng.ng_LeftEdge = windowWidth/2 + 10;  
    ng.ng_GadgetText = "Play";
    ng.ng_GadgetID = 2;  
    
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
    WORD labelX = 20;
    WORD contentX = 100;
    WORD maxWidth = windowWidth - contentX - 20;  // Maximum width for wrapped text
    
    SetAPen(rp, 1);
    SetBPen(rp, 0);
    
    // Draw Name field
    Move(rp, labelX, textY);
    Text(rp, "Name:", 5);
    DrawWrappedText(rp, station->displayText, contentX, &textY, maxWidth);
    
    // Add some spacing between fields
    textY += 10;
    
    // Draw URL field
    Move(rp, labelX, textY);
    Text(rp, "URL:", 4);
    DrawWrappedText(rp, station->url, contentX, &textY, maxWidth);
    
    textY += 10;
    
    // Draw Codec field
    Move(rp, labelX, textY);
    Text(rp, "Codec:", 6);
    Move(rp, contentX, textY);
    Text(rp, station->codec, strlen(station->codec));
    textY += 20;
    
    // Draw Bitrate field
    Move(rp, labelX, textY);
    Text(rp, "Bitrate:", 8);
    char bitrateBuf[20];
    sprintf(bitrateBuf, "%ld kbps", station->bitrate);
    Move(rp, contentX, textY);
    Text(rp, bitrateBuf, strlen(bitrateBuf));
    textY += 20;
    
    // Draw Country field
    Move(rp, labelX, textY);
    Text(rp, "Country:", 8);
    Move(rp, contentX, textY);
    Text(rp, station->country, strlen(station->country));

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
                UWORD class = msg->Class;
                UWORD code = msg->Code;
                struct Gadget *gad = (struct Gadget *)msg->IAddress;

				switch(msg->Class) {
				case IDCMP_CLOSEWINDOW:
					done = TRUE;
					break;

				case IDCMP_GADGETUP:
                        switch(gad->GadgetID) {
                            case 1: // Save button
                                SaveSingleStation(ext);
                                break;
                                
                            case 2: // Play button
                                if (IsAmigaAMPRunning()) {
                                    if (OpenStreamInAmigaAMP(ext->url)) {
                                        // Success - show status in main window
                                        char msg[MAX_STATUS_MSG_LEN];
                                        snprintf(msg, MAX_STATUS_MSG_LEN, 
                                                "Playing: %s", ext->displayText);
                                        UpdateStatusMessage(msg);
                                    } else {
                                        UpdateStatusMessage("Failed to start playback");
                                    }
                                } else {
                                    UpdateStatusMessage("AmigaAMP is not running");
                                }
                                break;
                        }
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
                          stations[i].country,
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
        ext->country = strdup(stations[i].country ? stations[i].country : "??");
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
	struct GadgetMetrics metrics;

	DEBUG("Starting GUI initialization...");

	s = LockPubScreen(NULL);
	if (!s) {
		DEBUG("Failed to lock public screen");
		return FALSE;
	}

	// Calculate layout metrics
	CalculateMetrics(s, &metrics);
	if (!LoadCountryConfig(COUNTRY_CONFIG_FILE, &countryConfig)) {
		DEBUG("Failed to load country configuration");
		goto cleanup;
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

	// Create gadget context
	gad = CreateContext(&glist);
	if (!gad) {
		DEBUG("Failed to create gadget context");
		free_labels(site_labels);
		FreeVisualInfo(vi);
		UnlockPubScreen(NULL, s);
		return FALSE;
	}


	// Initialize base gadget properties
	ng.ng_TextAttr = s->Font;
	ng.ng_VisualInfo = vi;
	ng.ng_Height = metrics.gadgetHeight;

	// First Row - Name input
	// Name field
	ng.ng_LeftEdge = metrics.leftMargin + metrics.labelWidth;  // Start after label space
	ng.ng_TopEdge = metrics.topMargin;
	ng.ng_Width = metrics.columnWidth;
	ng.ng_Height = metrics.gadgetHeight;
	ng.ng_GadgetText = "Name";
	ng.ng_TextAttr = s->Font;
	ng.ng_GadgetID = 1;
	ng.ng_Flags = PLACETEXT_LEFT;
	ng.ng_VisualInfo = vi;

	nameStrGad = CreateGadget(STRING_KIND, gad, &ng,
	                          GTST_MaxChars, 40,
	                          GTST_String, "",
	                          TAG_DONE);

	// Tags
	ng.ng_LeftEdge = metrics.leftMargin * 2 + metrics.columnWidth + metrics.labelWidth * 2;  // Account for two label spaces
	ng.ng_GadgetText = "Tags";
	ng.ng_GadgetID = 5;

	tagsStrGad = CreateGadget(STRING_KIND, nameStrGad, &ng,
	                          GTST_MaxChars, 100,
	                          GTTX_Text, "",
	                          TAG_DONE);

	// Country dropdown
	ng.ng_LeftEdge = metrics.leftMargin + metrics.labelWidth;  // Same as Name
	ng.ng_TopEdge += metrics.gadgetHeight + metrics.gadgetSpacing;
	ng.ng_GadgetText = "Country";
	ng.ng_GadgetID = 2;

	countryCodeCycle = CreateGadget(CYCLE_KIND, tagsStrGad, &ng,
	                                GTCY_Labels, countryConfig.choices,
	                                GTCY_Active, 0,
	                                TAG_DONE);

	// Codec dropdown
	ng.ng_LeftEdge = metrics.leftMargin * 2 + metrics.columnWidth + metrics.labelWidth * 2;
	ng.ng_GadgetText = "Codec";
	ng.ng_GadgetID = 4;

	codecCycle = CreateGadget(CYCLE_KIND, countryCodeCycle, &ng,
	                          GTCY_Labels, (STRPTR *)codecChoices,
	                          GTCY_Active, 0,
	                          TAG_DONE);

	// Checkboxes - adjust for better text visibility
	ng.ng_LeftEdge = metrics.leftMargin;
	ng.ng_TopEdge += metrics.gadgetHeight + metrics.gadgetSpacing;
	ng.ng_Width = metrics.labelWidth;
	ng.ng_GadgetText = "HTTPS Only";
	ng.ng_GadgetID = 6;
	ng.ng_Flags = PLACETEXT_RIGHT;

	httpsCheckBox = CreateGadget(CHECKBOX_KIND, codecCycle, &ng,
	                             GTCB_Checked, FALSE,
	                             TAG_DONE);

	// Hide Broken checkbox with proper spacing
	ng.ng_LeftEdge += metrics.labelWidth + metrics.checkboxSpacing;  // Use dedicated spacing
	ng.ng_GadgetText = "Hide Broken";
	ng.ng_GadgetID = 7;

	hideBrokenCheckBox = CreateGadget(CHECKBOX_KIND, httpsCheckBox, &ng,
	                                  GTCB_Checked, TRUE,
	                                  TAG_DONE);


	// Search button - right aligned
	ng.ng_LeftEdge = WINDOW_WIDTH - metrics.leftMargin - metrics.buttonWidth;
	ng.ng_Width = metrics.buttonWidth;
	ng.ng_GadgetText = "Search";
	ng.ng_Flags = PLACETEXT_IN;  // Text inside button
	ng.ng_GadgetID = 8;

	searchButton = CreateGadget(BUTTON_KIND, hideBrokenCheckBox, &ng, TAG_DONE);



	// List view remains full width
	ng.ng_LeftEdge = metrics.leftMargin;
	ng.ng_TopEdge += metrics.gadgetHeight + metrics.gadgetSpacing;
	ng.ng_Width = WINDOW_WIDTH - (metrics.leftMargin * 2);
	ng.ng_Height = WINDOW_HEIGHT - ng.ng_TopEdge - metrics.gadgetHeight * 3;
	ng.ng_GadgetID = 9;

	listView = CreateGadget(LISTVIEW_KIND, searchButton, &ng,
	                        GTLV_ReadOnly, FALSE,
	                        GTLV_Labels, site_labels,
	                        TAG_DONE);

	// Save button - right aligned, bottom
	ng.ng_LeftEdge = WINDOW_WIDTH - metrics.leftMargin - metrics.buttonWidth;
	ng.ng_TopEdge += ng.ng_Height + metrics.gadgetSpacing;
	ng.ng_Width = metrics.buttonWidth;
	ng.ng_Height = metrics.gadgetHeight;
	ng.ng_GadgetText = "Save";

	saveButton = CreateGadget(BUTTON_KIND, listView, &ng, TAG_DONE);

	// Status Message (bottom left)
	ng.ng_LeftEdge = metrics.leftMargin;
	ng.ng_Width = WINDOW_WIDTH - metrics.leftMargin * 3 - metrics.columnWidth;
	ng.ng_GadgetText = NULL;
	ng.ng_Flags = 0;
	ng.ng_GadgetID = 11;

	statusMsgGad = CreateGadget(TEXT_KIND, saveButton, &ng,
	                            GTTX_Text, "Ready",
	                            TAG_DONE);
	if (!statusMsgGad) {
		DEBUG("Failed to create Status gadget");
		goto cleanup;
	}
	// Create window
	window = OpenWindowTags(NULL,
	                        WA_Left, 100,
	                        WA_Top, 50,
	                        WA_InnerWidth, WINDOW_WIDTH,
	                        WA_InnerHeight, WINDOW_HEIGHT,
	                        WA_ReportMouse, TRUE,
	                        WA_MouseQueue, 3,
	                        WA_IDCMP, BUTTONIDCMP |
	                        IDCMP_CLOSEWINDOW |
	                        IDCMP_REFRESHWINDOW |
	                        IDCMP_GADGETUP |
	                        IDCMP_MENUPICK |
	                        CYCLEIDCMP,
	                        WA_Flags, WFLG_DRAGBAR |
	                        WFLG_DEPTHGADGET |
	                        WFLG_CLOSEGADGET |
	                        WFLG_ACTIVATE |
	                        WFLG_MENUSTATE |
	                        WFLG_NOCAREREFRESH |
	                        WFLG_NEWLOOKMENUS |
	                        WFLG_SMART_REFRESH,
	                        WA_Gadgets, glist,
	                        WA_ScreenTitle, "TuneFinder 1.0",
	                        WA_Title, "TuneFinder by sandlbn",
	                        WA_PubScreen, s,
	                        TAG_DONE);

	if (!window) {
		DEBUG("Failed to create window");
		goto cleanup;
	}

	// Create and setup menus
	menuStrip = CreateAppMenus();
	if (menuStrip) {
		DEBUG("Menu created successfully");

		if (!LayoutMenus(menuStrip, vi,
		                 GTMN_NewLookMenus, TRUE,  // Add this for new look
		                 TAG_DONE)) {
			DEBUG("Failed to layout menus");
			FreeMenus(menuStrip);
			menuStrip = NULL;
		} else if (!SetMenuStrip(window, menuStrip)) {
			DEBUG("Failed to set menu strip");
			FreeMenus(menuStrip);
			menuStrip = NULL;
		} else {
			DEBUG("Menu setup complete");
		}
	} else {
		DEBUG("Menu creation failed");
	}

	RefreshGList(glist, window, NULL, -1);
	// Store window ports
	RastPort = window->RPort;
	WindowPort = window->UserPort;

	// Add gadgets to window
	//AddGList(window, glist, (UWORD)~0, (UWORD)~0, NULL);
	//RefreshGList(glist, window, NULL, 0);
	GT_RefreshWindow(window, NULL);


	// Store important pointers
	browserList = site_labels;
	window->UserData = (void *)glist;
	visualInfo = vi;

	// Load settings
	LoadSettings(&currentSettings);

	UnlockPubScreen(NULL, s);
	DEBUG("GUI initialization completed successfully");
	return TRUE;

cleanup:
	if (menuStrip) {
		FreeMenus(menuStrip);
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
