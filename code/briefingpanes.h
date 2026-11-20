#ifndef BRIEFINGPANES_H
#define BRIEFINGPANES_H

// MechCommander Omnitech's mission briefing screen shows four panes in one
// text area, picked with buttons 83 to 86 of its art/mcl_mn.fit (as its
// MCORel.exe does in MissionBriefingScreen):
//   83 briefing: the objectives and Blurb, continued in Blurb3
//   84 tactical information: Blurb4
//   85 additional information: Blurb5
//   86 mission description: Blurb2
// A pane with no text has its button disabled. Game-free so it can be unit
// tested; MissionBriefingScreen fills and draws the panes.

namespace briefingpanes {

enum Pane { kBriefing = 0, kTactical, kAdditional, kDescription, kPanes };

// The briefing pane's button; the others follow in Pane order.
const unsigned long kFirstButton = 83;

// The pane a button selects, or -1 when it is not a pane button.
int paneForButton(unsigned long buttonID);
unsigned long buttonForPane(int pane);

// True when the text has a character other than whitespace.
bool hasText(const char* text);

// The pane shown after a click on buttonID: its pane when that pane has text,
// otherwise current.
int selectPane(const bool paneHasText[kPanes], int current, unsigned long buttonID);

} // namespace briefingpanes

#endif // BRIEFINGPANES_H
