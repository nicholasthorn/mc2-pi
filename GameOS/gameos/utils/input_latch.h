#ifndef INPUT_LATCH_H
#define INPUT_LATCH_H

// Buttons set between input samples (the VR controllers' virtual mouse and
// keys, gos_input). In VR the game reads input only on frames it steps, 30
// times a second against the headset's 72, so a press that starts and ends
// between two samples must still show as down at the next one, once.

#include <cstdint>

struct InputLatch {
    uint32_t held = 0;      // down now
    uint32_t latched = 0;   // went down since the last sample

    void set(uint32_t buttons)
    {
        latched |= buttons & ~held;
        held = buttons;
    }

    // what is down for this sample: held now, or pressed since the last one
    uint32_t sample()
    {
        const uint32_t down = held | latched;
        latched = 0;
        return down;
    }
};

#endif // INPUT_LATCH_H
