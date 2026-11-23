# First run on a Mac: brief for an agent

A brief to hand to a coding agent (Claude Code or similar) running on an
M-series Mac, for step 2 of `MACOS.md`: the first mission on real Apple
hardware. Paste everything from "Brief" down. Nothing here has run on a Mac yet;
where a step is a guess, the brief says so.

## Brief

You are on an Apple Silicon Mac. The goal is to build MechCommander 2 from
`nicholasthorn/mc2-pi` and run one mission, then report exactly what happened.
CI already builds this tree on `macos-15` arm64 and passes the unit tests, but
no one has run the game on a Mac. Expect problems at run time, not at compile
time.

Read `CLAUDE.md` and `planning/future-features/MACOS.md` first. Rules that
matter here:

- Push only to the `pi` remote (`nicholasthorn/mc2-pi`). Never open issues or
  PRs on `alariq/mc2`.
- The game opens a window and takes the mouse. **Ask the person before each
  launch.** A Mac has no Xvfb, so `scripts/smoke.sh` will not work as is.
- Do not use em dashes or emoji in anything you write.

### 1. Tools and source

```bash
xcode-select --install            # if the compilers are missing
brew install cmake ninja sdl2 sdl2_mixer sdl2_ttf glew git-lfs
git clone https://github.com/nicholasthorn/mc2-pi.git mc2 && cd mc2
git checkout macos-port           # or pi-port once PR #180 is merged
```

### 2. Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_PREFIX_PATH="$(brew --prefix)"
cmake --build build
ctest --test-dir build --output-on-failure
```

`-DMC2_NET=OFF` skips downloading and building the network library, which
saves several minutes; missions do not need it.

### 3. Game data (pick one)

The data is platform independent. A game directory holds `data/`, `assets/`,
`*.fst` and `*.cfg`.

**A. From a release package (quickest).** The Linux x64 "full" package already
holds built data. Release `v0.5.0-rc1` is a draft, so downloading it needs
`gh auth login` with an account that can see the repo's drafts (ask the person).

```bash
gh release download v0.5.0-rc1 -R nicholasthorn/mc2-pi \
   -p 'mc2-v0.5.0-rc1-linux-x86_64-full.tar.gz' -D ..
tar -xzf ../mc2-v0.5.0-rc1-linux-x86_64-full.tar.gz -C ..
mv ../mc2-v0.5.0-rc1-linux-x86_64 ../mc2game
rm ../mc2game/mc2 ../mc2game/libmc2res_64.so   # Linux binaries; replaced below
```

It already has `options.cfg`.

**B. Build it from `alariq/mc2srcdata` (no access needed, slower, untried on
macOS).** Only movies are in Git LFS, so skip them:

```bash
GIT_LFS_SKIP_SMUDGE=1 git clone https://github.com/alariq/mc2srcdata.git ../mc2srcdata
scripts/build_gamedata.sh --src-data ../mc2srcdata --build-dir build \
   --out ../mc2game --no-movies -j 8
cp ../mc2game/minprefs.cfg ../mc2game/options.cfg
```

Not yet tried on macOS. The upstream makefile uses only `cp`, `mkdir` and
`rm` and nothing newer than Apple's GNU make 3.81; the data tools (`makefst`,
`pak`, `aseconv`, `makersp`, `text_tool`) build on macOS in CI but have never
run there. Report anything that breaks; fix this repo's script or tools, not
the upstream data.

Without `options.cfg` the first launch only runs the hardware sniffer and
exits.

**C. Copy it from the person's PC** if they have a built game directory there.

### 4. Put the build into the game directory

```bash
cp build/mc2 build/out/res/libmc2res_64.so ../mc2game/
rm -rf ../mc2game/shaders && cp -r shaders ../mc2game/
```

### 5. First run (ask first: it opens a window)

```bash
cd ../mc2game
./mc2 -mission mc2_01 -autoexit 3000 2>&1 | tee mac-run1.log
```

`-autoexit` counts gameplay frames and quits cleanly; it also ignores input.
Check the log for:

- `Version    : 4.1 ...` with Metal in the name: the core context (macOS
  refuses a core profile without the forward-compatible flag, which the build
  sets).
- `[MISSION] mc2_01 started`, then `[AUTOEXIT]`, and exit code 0.
- No `OpenGL Error`, `STOP: `, `FATAL` or shader compile/link errors. A shader
  error means the GLSL 4.10 path (`MC2_NO_LAYOUT_BINDING`) needs work; the
  shader text is in the log.

Then ask the person to look at the screen during a second run: are the terrain
fog and the mechs' lighting as on Linux? (They come from the `SceneData`
uniform block, which macOS binds by name instead of `layout(binding=)`.)

Frame rate, if wanted:
`./mc2 -mission mc2_01 -autoexit 3000 -bench frames.csv` prints a `[BENCH]`
summary.

### 6. Play by hand (the person, not you)

`./mc2` to the main menu, then a few minutes of a mission. Things likely to be
wrong on a Mac, to check one by one:

- **Retina:** the window may be small or blurry; mouse clicks may land in the
  wrong place. The plan is to leave SDL's high-DPI flag off, so macOS scales.
- **Right click** (two-finger click on a trackpad), **keyboard** shortcuts,
  **sound**.
- **Where files go:** options and saves are written relative to the game
  directory and to `~/.config`; note anything that lands somewhere odd.

### 7. Report and record

- Write down: macOS version, chip (M1 / M2 ...), the GL version line,
  pass/fail for steps 2 to 6, and the relevant log lines.
- Tick "A mission on an M-series Mac" in `planning/HARDWARE-TESTS.md` with the
  date, machine and result, and add a line to the log in `planning/PROGRESS.md`.
- Fix what you can in a branch off `macos-port` (or `pi-port`), following
  `.claude/rules/`: platform differences behind `__APPLE__` in `GameOS/`,
  small diffs, both renderers kept building. Open a draft PR on
  `nicholasthorn/mc2-pi` and say in it what was run on the Mac and what was not.
