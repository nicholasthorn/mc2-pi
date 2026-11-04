# macOS on Apple silicon (M-series)

Status: proposed, nothing built, not tried on a Mac. Everything about how the
game runs there is a prediction until a first build runs.

## Problem

The game has no Mac build: no `__APPLE__` code in the engine, no CI job, no
package. Apple's graphics are the main reason it is not simply another
recompile.

## Why it is close

- **64-bit Unix with Clang:** macOS `long` is 64-bit as on Linux, `char` is signed
  on Apple's arm64, and the tree already builds with Clang on Linux and for arm64
  on Android.
- **SDL2 covers the window, input and sound.**
- **The renderer needs little past GL 4.1.** It asks for a GL 4.0 core context and
  already runs on WSLg's GL 4.1 driver, given `GL_ARB_shading_language_420pack`
  (`buildShaderHeader()`). The one thing it takes from that extension that matters
  here is `layout(binding=)`: two uniform blocks (`scene.hglsl`,
  `gos_tex_vertex_lighted.vert`) and any such samplers.

## Graphics: which route

| Route | Work | Risk |
|---|---|---|
| **Apple's OpenGL 4.1** (recommended first) | Forward-compatible core 4.1 context; `#version 410`; set the bindings in C++ (`glUniformBlockBinding`, `glUniform1i`) where 420pack is missing | Deprecated since macOS 10.14, with no announced removal; 4.1 is the ceiling. Apple does not fix it, so a driver bug stays |
| **ANGLE on Metal, using our GLES renderer** | Bundle ANGLE; point SDL at it; no renderer fork | Our GLES path asks for ES 3.1. ANGLE's Metal status for 3.1 is unclear from its docs, so a spike must find out, and whether 3.0 would do |
| **ANGLE on Vulkan on MoltenVK** | As above, plus MoltenVK | Two translation layers, for ES 3.1 |
| **A native Metal renderer** | A third backend beside desktop GL and GLES | Large, and against the small-diff policy. Not proposed |

Start with OpenGL 4.1: it is the smallest change, and the binding fix also helps
any other 4.1 driver without 420pack. It keeps the rule of two renderers, since
`shaders/` only gains a third compile target. `scripts/validate_shaders.py` must
then check GLSL 4.10 as well. Keep ANGLE as the way out if Apple removes OpenGL.

## Packaging

`mc2-<ver>-macos-arm64-full.dmg` holding `MechCommander 2.app`, arm64 only: Intel
Macs are not a target.

- **Inside the app:** `Contents/MacOS/mc2`, the resource library, SDL2, SDL2_mixer
  and GLEW as bundled dylibs with `@rpath` names (`dylibbundler` or CMake's
  `fixup_bundle`), and game data, shaders and licences in `Contents/Resources`.
- **Writable files must leave the bundle.** The game reads and writes relative to
  its working directory (`options.cfg`, logs, saves), while a bundle should stay
  unchanged: writing there breaks its signature. At start, a small `__APPLE__`
  step in GameOS, the Mac's `android_prepare`, makes
  `~/Library/Application Support/MechCommander2`, moves there, and symlinks `data`,
  `assets`, `shaders` and `*.fst` from the bundle. No second copy of the data, and
  an update takes effect by itself. This needs checking against the game's own
  file handling.
- **HiDPI:** leave SDL's high-DPI flag off at first, so macOS scales the window and
  mouse coordinates stay as they are.

### Signing

Apple silicon requires every binary to be signed, but an ad-hoc signature
(`codesign -s -`) meets that and costs nothing. Gatekeeper still blocks a
downloaded app that is not notarized. Since macOS 15 the Control-click way past
this is gone: a player tries to open it once, then chooses System Settings,
Privacy & Security, Open Anyway. That is two extra steps and an alarming dialog,
to be spelled out in `INSTALL.md`.

The alternative is a Developer ID with notarization, through the paid Apple
Developer Program (about $99 a year, to confirm), with the certificate as a CI
secret. The owner decides, as with Windows code signing in `RELEASES.md`. The
Apple account would be the owner's, and its name appears on the app.

Homebrew casks are a later option, and do not avoid this.

## CI

- `ci.yml`: a `macos-15` arm64 job building `mc2` and `mc2_tests` with Homebrew's
  SDL2, and running the unit tests.
- `release.yml`: a `macos` job as the other platforms, then the DMG in job `full`.
- **Smoke is the gap.** There is no llvmpipe route on macOS as on Linux, and hosted
  runners may have no usable GPU, so a mission may not run in CI. Try it once; until
  then Mac support is "Built" in `RELEASES.md`'s terms, with a hand check per release.

## Order of work

| Step | What | Done when |
|---|---|---|
| 1 | Build on macOS: fix what the compiler finds, behind `__APPLE__` or in GameOS | CI job green, unit tests pass |
| 2 | OpenGL 4.1: context, `#version 410`, bindings in C++, validator | A mission runs on an M-series Mac, and GL and GLES are unchanged on Linux |
| 3 | `.app`: bundled libraries, data, Application Support step | Runs from `/Applications` on a Mac with no Homebrew; saves survive an update |
| 4 | DMG in `release.yml`, ad-hoc signed; `INSTALL.md` | A downloaded DMG opens through Open Anyway |
| 5 | Optional: notarization | No Gatekeeper dialog |

**Steps 2 to 5 need an M-series Mac by hand.** Without one, only step 1 can be done
with confidence.

## Open questions

- Is there an M-series Mac to test on, and which macOS?
- Notarize, or ask players to use Open Anyway?
- Do any legacy Linux assumptions in `GameOS/src` (`platform_*.cpp`) fail on macOS?
  Step 1 finds out.
- Frame rate through Apple's GL-on-Metal layer: unknown, though an M-series GPU has
  far more headroom than the Pi this game already runs on.
