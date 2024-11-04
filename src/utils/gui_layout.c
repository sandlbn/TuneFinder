#include <string.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include "../../include/gui_layout.h"
#include "../../include/config.h"

void CalculateGUIMetrics(struct Screen *screen, struct GUIMetrics *metrics) {
    // Get screen font
    metrics->font = screen->RastPort.Font;
    
    // Calculate base metrics from font height
    WORD baseUnit = metrics->font->tf_YSize;
    
    metrics->gadgetHeight = baseUnit + 4;
    metrics->gadgetSpacing = baseUnit;
    metrics->labelSpacing = baseUnit / 2;
    
    // Set margins based on font size
    metrics->leftMargin = baseUnit * 2;
    metrics->rightMargin = baseUnit * 2;
    metrics->topMargin = baseUnit * 2;
    metrics->bottomMargin = baseUnit * 2;
    
    // Calculate minimum window dimensions
    metrics->windowWidth = screen->Width / 2;  // 50% of screen width
    metrics->windowHeight = screen->Height / 2; // 50% of screen height
}

WORD CalculateTextWidth(struct RastPort *rp, const char *text) {
    return TextLength(rp, text, strlen(text));
}

WORD CalculateGadgetWidth(struct RastPort *rp, const char *text, WORD minWidth) {
    WORD textWidth = CalculateTextWidth(rp, text);
    return MAX(textWidth + 20, minWidth); // 20 pixels padding
}