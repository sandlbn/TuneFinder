#ifndef GUI_LAYOUT_H
#define GUI_LAYOUT_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <graphics/rastport.h>
#include <graphics/text.h>

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

struct GUIMetrics {
    WORD windowWidth;
    WORD windowHeight;
    WORD leftMargin;
    WORD rightMargin;
    WORD topMargin;
    WORD bottomMargin;
    WORD gadgetHeight;
    WORD gadgetSpacing;
    WORD labelSpacing;
    struct TextFont *font;
};

void CalculateGUIMetrics(struct Screen *screen, struct GUIMetrics *metrics);
WORD CalculateTextWidth(struct RastPort *rp, const char *text);
WORD CalculateGadgetWidth(struct RastPort *rp, const char *text, WORD minWidth);

#endif /* GUI_LAYOUT_H */
