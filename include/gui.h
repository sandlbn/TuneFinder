#ifndef GUI_H
#define GUI_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include "data.h"

extern struct Window *window;
extern struct List *browserList;
extern struct Gadget *statusMsgGad;
extern void *visualInfo; 

BOOL InitLibraries(void);
void CleanupLibraries(void);
BOOL OpenGUI(void);
void HandleSearch(void);
void HandleSave(void);
void HandleGadgetUp(struct IntuiMessage *imsg);
void HandleListSelect(struct IntuiMessage *imsg);
struct Window* OpenDetailsWindow(struct ExtNode *station);

#endif /* GUI_H */