#include "briefingpanes.h"

namespace briefingpanes {

int paneForButton(unsigned long buttonID)
{
    if (buttonID < kFirstButton || buttonID >= kFirstButton + kPanes)
        return -1;
    return static_cast<int>(buttonID - kFirstButton);
}

unsigned long buttonForPane(int pane)
{
    return kFirstButton + static_cast<unsigned long>(pane);
}

bool hasText(const char* text)
{
    if (!text)
        return false;
    for (; *text; text++) {
        if (*text != ' ' && *text != '\t' && *text != '\r' && *text != '\n')
            return true;
    }
    return false;
}

int selectPane(const bool paneHasText[kPanes], int current, unsigned long buttonID)
{
    const int pane = paneForButton(buttonID);
    if (pane < 0 || !paneHasText[pane])
        return current;
    return pane;
}

} // namespace briefingpanes
