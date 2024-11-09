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
    metrics->leftMargin = 10; 
    metrics->topMargin = font_height + 12;  
    
    // Text heights and spacing
    metrics->rowHeight = font_height;
    metrics->elementSpacing = font_height + 2;  
    
    // Control sizes based on font width
    metrics->labelWidth = font_width * 17;  
    metrics->controlWidth = font_width * 22; 
    metrics->buttonHeight = font_height + 10; 
    
    // List view sizing
    metrics->listViewHeight = (font_height + 8) + (font_height * 7); 
    
    // Additional metrics
    metrics->windowPadding = 15;  
    metrics->checkboxWidth = font_width * 14; 
    metrics->columnWidth = metrics->controlWidth + metrics->leftMargin; 
}
