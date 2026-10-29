#ifndef AUDIO_PAN_H
#define AUDIO_PAN_H

#include <cstdint>

// Per-side levels for Mix_SetPanning (0 = silent, 255 = full).
struct StereoLevels {
	uint8_t left;
	uint8_t right;
};

// Maps a pan of -1 (hard left) .. 0 (centre) .. +1 (hard right) to stereo
// levels. Constant-power curve scaled so the centre is full level on both
// sides (Mix_SetPanning(255, 255) is a no-op), and the near side never goes
// above full. Out-of-range values are clamped; NaN is treated as centre.
// Volume is applied separately (Mix_Volume), so this only sets the balance.
// SDL-free so it can be unit tested.
StereoLevels panToStereoLevels(float pan);

#endif // AUDIO_PAN_H
