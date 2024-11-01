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
extern void *visualInfo;
extern struct Menu *menuStrip;
extern struct APISettings currentSettings;
extern BOOL running;  // Add this line
extern struct CountryConfig countryConfig;

BOOL InitLibraries(void);
void CleanupLibraries(void);
BOOL OpenGUI(void);
void HandleSearch(void);
void HandleSave(void);
void HandleGadgetUp(struct IntuiMessage *imsg);
void HandleListSelect(struct IntuiMessage *imsg);
void HandleMenuPick(UWORD menuNumber);

#endif /* GUI_H */
