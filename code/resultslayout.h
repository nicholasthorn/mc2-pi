//---------------------------------------------------------------------------
// The results screens' mode (salvage and pilot review, mcui_mr_layout.fit).
//
// Their layouts are fixed pixel positions. On stock data everything that must
// be seen fits 800x600; Omnitech moved both Done buttons beyond it (pilot
// review's to 741,712), so at 800x600 there was no way to leave. The layout's
// overall extent cannot decide the size: both files park animated parts far
// off screen (y 1326). What must be on screen is each Done button's clickable
// text rectangle, so the mode is the smallest of 800x600 and 1024x768 that
// holds those, and the front end's own size beyond that.
//---------------------------------------------------------------------------

#ifndef RESULTSLAYOUT_H
#define RESULTSLAYOUT_H

namespace results_layout {

// right, bottom: the furthest edge of any Done button's text rectangle (0 when
// none was read); frontEndWidth/Height: the largest size to fall back on
inline void screenSize(long right, long bottom, long frontEndWidth, long frontEndHeight,
                       long& width, long& height)
{
    if (right <= 800 && bottom <= 600) {
        width = 800;
        height = 600;
    } else if (right <= 1024 && bottom <= 768) {
        width = 1024;
        height = 768;
    } else {
        width = frontEndWidth;
        height = frontEndHeight;
    }
}

} // namespace results_layout

#endif
