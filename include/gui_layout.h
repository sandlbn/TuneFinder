#ifndef GUI_LAYOUT_H
#define GUI_LAYOUT_H

struct GUIMetrics {
    WORD baseUnit;
    WORD leftMargin;
    WORD topMargin;
    WORD rowHeight;
    WORD elementSpacing;
    WORD labelWidth;
    WORD controlWidth;
    WORD buttonHeight;
    WORD windowPadding;
    WORD listViewHeight;
    WORD columnWidth;
    WORD checkboxWidth;
};
void CalculateGUIMetrics(struct Screen *screen, struct GUIMetrics *metrics);
#endif
