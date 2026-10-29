# Positional sound: panning, and maybe doppler

Status: option A (stereo panning) implemented on branch `stereo-panning`
(`GameOS/gameos/utils/audio_pan.cpp`, unit-tested; not yet checked by ear).
Option B not started. Written 2026-10-29 from a read of `pi-port`. Upstream's
README lists "panning, doppler, etc. not supported yet".

## Problem

Sounds from units and explosions play centred: the stereo field does not tell
you where on the map something happened. Doppler (pitch shift for moving
sources) is also missing.

## What the code does today

- **Backend:** `GameOS/gameos/gameos_sound.cpp`, SDL_mixer, 32 channels
  (`SoundEngine::NUM_CHANNELS`), set up in `SoundEngine::init`.
- **The game already works out position.** `SoundSystem::playDigitalSample`
  (`mclib/soundsys.cpp`) takes a world position, computes a linear distance
  falloff to `FALLOFF_DISTANCE` (and skips sounds beyond it), rotates the
  offset into camera space, and computes a pan from -1 (left) to +1 (right). It
  passes these as `gosAudio_Volume` and `gosAudio_Panning`.
- **GameOS throws the pan away.** In `gosAudio_SetChannelSlider`, the
  `gosAudio_Panning` case stores the value but the `Mix_SetPanning` call is
  commented out: the old attempt folded volume into the pan levels, so changing
  volume afterwards went wrong.
- **Doppler is never requested.** The GameOS API has DirectSound3D-era sliders
  (`gosAudio_Position`, `Velocity`, `Doppler`, `Frequency`, `MinMaxDistance`,
  ...), but the game only ever allocates `gosAudio_Volume | gosAudio_Panning`
  (`soundsys.cpp`, `AllocateChannelSliders`). No game code sets a velocity,
  frequency or doppler factor. So the upstream "doppler" gap is unused API, not
  a feature the game is waiting on.
- **Small bug found:** `gosAudio_GetChannelSlider`'s `gosAudio_Panning` case has
  no `break`, so it falls into the `gosAudio_Frequency` case and logs a `PAUSE`.

## Option A: stereo panning (recommended, small)

Make `gosAudio_Panning` work with `Mix_SetPanning`, independent of volume:

- SDL_mixer applies `Mix_Volume` and the `Mix_SetPanning` effect separately and
  multiplies them, so the pan should only set the left/right balance (each
  side 0 to 255), never the overall level.
- Use an equal-power pan law (`left = cos(t * pi/2)`, `right = sin(t * pi/2)`,
  `t = (pan + 1) / 2`), scaled so centre is not quieter than today (or accept a
  small drop and compensate in the volume).
- Put the pan law in a GL- and SDL-free function so it can be unit-tested
  (like `pixel_repack.cpp`): centre, hard left, hard right, clamping.
- Fix the missing `break` above.
- The game sets pan to 0 for every non-positional sound (UI, Betty, music), so
  those stay centred. Mono output (`b_stereo` false) should skip panning.
- Pans are computed when a sound starts and not updated as the camera moves.
  That matches the original game; refreshing pans for long looping sounds is an
  optional follow-up.

Size: about a day including tests. Risk: low. Verify by ear on desktop and Pi
(the smoke runs use the dummy audio driver, so CI only covers the unit tests).

## Option B: full 3D audio with doppler (medium, optional)

SDL_mixer has no per-channel pitch control, so doppler needs a different mixer:

- **miniaudio** (single header, public domain / MIT-0): spatialisation, distance
  attenuation, doppler and pitch, works on Linux, Windows, Android. Easiest to
  add: no new system package.
- **OpenAL Soft** (LGPL): full 3D positional audio, doppler, and HRTF for
  headphones. A system package on Linux/Pi; good fit for VR.

Either way the GameOS sliders (`Position`, `Velocity`, `Doppler`,
`MinMaxDistance`) get real implementations, and `SoundSystem` passes positions
and velocities (it has the position; mover velocity and the camera's movement
would need adding). Music and streamed audio (`gosStreamedAudio`, movies) move
to the new mixer too.

Value: low for the normal game. The camera is far above the battlefield and
sounds are short, so doppler would rarely be audible (air strikes and VTOL
fly-overs are the exceptions). Real 3D audio becomes worthwhile with the VR
tabletop (`VR-TABLETOP.md`), where head-tracked positional sound helps a lot.

Size: one to two weeks. Risk: medium (replacing the mixer touches all sound,
music and movie audio; 8-bit unsigned WAV handling must carry over).

## Recommendation

Do option A now. Leave option B until the VR work, and choose OpenAL Soft (HRTF)
or miniaudio then.

## Open questions

- Pan law and range: should hard left/right be reachable, or should
  `FALLOFF_DISTANCE * 0.5` be widened so pans stay subtle?
- Should looping sounds (ambient, engines) re-pan every frame?
