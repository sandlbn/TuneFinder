#include <string.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include "../../include/gui_layout.h"
#include "../../include/config.h"

void CalculateGUIMetrics(struct Screen *screen, struct GUIMetrics *metrics) {
    // Get font metrics directly from screen
    ULONG font_width = screen->RastPort.TxWidth;
    ULONG font_height = screen->RastPort.TxHeight;
    
    // Basic spacing based on font
    metrics->leftMargin = 10;  // Fixed margin like in example
    metrics->topMargin = font_height + 8;  // Following example pattern
    
    // Text heights and spacing
    metrics->rowHeight = font_height;
    metrics->elementSpacing = font_height + 2;  // Standard spacing between elements
    
    // Control sizes based on font width
    metrics->labelWidth = font_width * 17;  // Standard label width from example
    metrics->controlWidth = font_width * 22;  // Standard control width from example
    metrics->buttonHeight = font_height + 10;  // Button height with padding
    
    // List view sizing
    metrics->listViewHeight = (font_height + 8) + (font_height * 7);  // Matches example pattern
    
    // Additional metrics
    metrics->windowPadding = 15;  // Standard window padding from example
    metrics->checkboxWidth = font_width * 14;  // For checkbox gadgets
    metrics->columnWidth = metrics->controlWidth + metrics->leftMargin;  // For column layout
}
