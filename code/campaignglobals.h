// code/campaignglobals.h
//
// wolfman-MC2X's campaign variables: five integers its scripts keep across a
// campaign's missions (ABL getcampaignglobalvar / setcampaignglobalvar),
// saved with the campaign in their own block, written only when one is not
// 0, so other campaigns' saves are unchanged. Uses only the FIT reader, so it
// can be unit tested.

#ifndef CAMPAIGNGLOBALS_H
#define CAMPAIGNGLOBALS_H

class FitIniFile;

namespace campaignglobals
{

const long kCount = 5;
extern const char* const kBlockName;

// Index 0 to kCount - 1; any other index reads 0 and is not stored.
int get(long index);
void set(long index, int value);

// All back to 0 (a campaign started, or loaded from a save without them).
void reset();

// Writes the block if any value is not 0.
void save(FitIniFile& file);

// Resets, then reads the block if the file has one.
void load(FitIniFile& file);

} // namespace campaignglobals

#endif
