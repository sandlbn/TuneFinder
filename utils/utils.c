#include <stdio.h>
#include <string.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/types.h>
#include <libraries/gadtools.h>
#include <proto/gadtools.h>
#include <proto/intuition.h>
#include <intuition/gadgetclass.h>
#include <clib/alib_protos.h>
#include "../include/utils.h"
#include "../include/config.h"
#include "../include/gui.h"

void UpdateStatusMessage(const char *message) {
    if (window && statusMsgGad) {
        GT_SetGadgetAttrs(statusMsgGad, window, NULL,
            GTTX_Text, message,
            TAG_DONE);
    }
    
    #ifdef DEBUG_BUILD
        DEBUG("Status: %s", message);
    #endif
}

void free_labels(struct List* l) {
    struct Node* n;
    
    while ((n = RemHead(l))) {
        deallocate(n->ln_Name, V_cstr);
        deallocate(n, V_Node);
    }
    
    deallocate(l, V_List);
}

BOOL SaveToPLS(const char *filename) {
    FILE *fp;
    struct Node *node;
    int count = 0;
    
    if (!browserList) return FALSE;
    for (node = browserList->lh_Head; node->ln_Succ; node = node->ln_Succ) {
        count++;
    }
    
    if (count == 0) {
        DEBUG("No stations to save");
        return FALSE;
    }
    
    fp = fopen(filename, "w");
    if (!fp) {
        DEBUG("Failed to open file: %s", filename);
        return FALSE;
    }
    
    fprintf(fp, PLS_HEADER);
    fprintf(fp, PLS_NUMBER_OF_ENTRIES, count);
    
    count = 1;
    for (node = browserList->lh_Head; node->ln_Succ; node = node->ln_Succ) {
        struct ExtNode *ext = (struct ExtNode *)node;
        fprintf(fp, PLS_FILE_ENTRY, count, ext->url);
        fprintf(fp, PLS_TITLE_ENTRY, count, ext->displayText);
        fprintf(fp, PLS_LENGTH_ENTRY, count);
        count++;
    }
    
    fclose(fp);
    return TRUE;
}