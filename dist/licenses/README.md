# Licences

MechCommander 2 (this engine) is under the GNU General Public License v3:
`license.txt` next to the game. The source of each release is the tagged
commit at https://github.com/nicholasthorn/mc2-pi.

No game data is included: it is built from
https://github.com/alariq/mc2srcdata under its own terms (see `INSTALL.md`).

## Compiled into the game (every package)

| Library | Use | Licence | File |
|---|---|---|---|
| pl_mpeg | Movie playback | MIT | `pl_mpeg.txt` |
| stb_image | Image loading | MIT or public domain | `stb.txt` |

## Bundled with the Windows package

The Linux packages use the system's own SDL2, GLEW and zlib, which come with
their licences through the system package manager. `mc2.exe` links zlib
statically, so its licence is listed although there is no zlib DLL.

| File | Library, version | Licence | File here |
|---|---|---|---|
| `SDL2.dll` | SDL 2.30.11 | zlib | `SDL2.txt` |
| `SDL2_mixer.dll` | SDL_mixer 2.8.0 | zlib | `SDL2_mixer.txt` |
| | Timidity (MIDI), built in | Artistic | `SDL2_mixer-timidity.txt` |
| | minimp3 (MP3), built in | CC0 | `SDL2_mixer-minimp3.txt` |
| | dr_flac (FLAC), built in | MIT-0 or public domain | `SDL2_mixer-dr_flac.txt` |
| | stb_vorbis (Ogg Vorbis), built in | MIT or public domain | `SDL2_mixer-stb_vorbis.txt` |
| `glew32.dll` | GLEW 2.2.0 | Modified BSD, MIT, Khronos | `GLEW.txt` |
| (in `mc2.exe`) | zlib 1.3.1 | zlib | `zlib.txt` |
| `vcruntime140.dll`, `vcruntime140_1.dll`, `msvcp140.dll` | Microsoft Visual C++ runtime | Distributable code under the Visual Studio licence terms | |

SDL_mixer's optional codecs (libxmp, Opus, WavPack, GME, FluidSynth) are
separate DLLs in upstream builds and are not shipped, so they are not listed.
