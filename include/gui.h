#ifndef GUI_H
#define GUI_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/exec.h>
#include "data.h"
#include "settings.h"
#include "country_config.h"

extern struct Window *window;
extern struct List *browserList;
extern struct Gadget *statusMsgGad;
extern struct Menu *menuStrip;
extern struct APISettings currentSettings;
extern struct CountryConfig countryConfig;
extern BOOL running;  // Add this line
extern void *visualInfo;

BOOL InitLibraries(void);
void CleanupLibraries(void);
BOOL OpenGUI(void);
void CleanupGUI(void);
struct List* CreateInitialList(void);
static struct Menu *CreateAppMenus(void);
static VOID Ghost(struct RastPort *rp, UWORD pen, UWORD x0, UWORD y0, UWORD x1, UWORD y1);
static ULONG RenderFunc(struct Hook *hook, struct Node *node, struct LVDrawMsg *msg);
void HandleMenuPick(UWORD menuNumber);
void HandleGadgetUp(struct IntuiMessage *imsg);
void HandleListSelect(struct IntuiMessage *imsg);
void HandleSearch(void);
void HandleSave(void);
void SaveSingleStation(struct ExtNode *station);

#endif /* GUI_H */
