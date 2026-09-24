<p align="center">
  <img src="docs/shots/boot.gif" width="640" alt="UFO 40 boot animation: a flying saucer beams the logo down">
</p>

<h1 align="center">UFO 40</h1>

<p align="center">
  <b>A pretend 1980s console with forty cartridges (well, three so far), built from scratch<br>
  for the PlayStation Vita, Windows and the web.</b><br><br>
  <a href="https://naniiic137.github.io/ufo-40/"><b>▶ Play it in your browser</b></a> ·
  <a href="https://github.com/naniiic137/ufo-40/releases">Download for Vita / Windows</a>
</p>

> **Disclaimer.** UFO 40 is a fan-made parody tribute. It contains no code, art,
> music or levels from UFO 50 and is not affiliated with or endorsed by Mossmouth.

---

## What is this?

UFO 40 is inspired by **UFO 50** by Mossmouth, a collection of fifty "lost" games
from a console that never existed. This is a joke port with a shorter name
and fewer games: forty cartridge slots on the equally fictional
**Beamdown Softworks** console. Each cartridge plays like its counterpart, but
everything you see and hear is new: the names, characters, pixel art, levels
and chiptune music.

Everything is written in plain C with no dependencies in the core:

- a 320×180 indexed-colour software renderer
- a four-channel chiptune synthesiser with its own music format
- a scene system and a save system

A thin SDL2 layer runs that same core on a modded **PS Vita** (HENkaku/Ensō),
on **Windows/Linux** and in the **browser** through Emscripten. A headless build
drives the whole console from scripted input for testing and for every
screenshot and GIF on this page.

## The library (3 of 40 loaded)

<p align="center"><img src="docs/shots/library.png" width="640" alt="The UFO 40 game library with 40 cartridge slots"></p>

| # | Cartridge | Genre | What it is |
|---|---|---|---|
| 01 | **UNDERDELVE** | exploration | Mo the mole relights a mine that's gone dark. You die in one hit, your jumps are locked in once you take off, and there are 36 hand-built screens with shops, secrets and item-gated paths. |
| 02 | **GRUB SHIFT** | tactics | Tilly the farm robot works the night shift. It's turn-based grub control: seven one-use tools, explosive fizz pods, and grubs that grow into egg-laying queens. |
| 03 | **ROOFCAT** | action | Harissa the courier cat chases the Magpie Mob across a sunny seaside town. It's an auto-scroller with double jumps, a spirit form when you die, and four stages plus a night route. |
| 04–40 | *coming soon* | | still in the saucer's cargo hold |

Every cartridge has three goals: a **Beacon** (a side challenge), the
**Saucer** (beating the game) and the **Alien** (a secret, harder challenge).
Earned goals light up on the cartridge in the library.

### 01 · UNDERDELVE

<p align="center">
  <img src="docs/shots/underdelve.gif" width="640" alt="Underdelve gameplay: Mo the mole jumps a spike pit">
</p>
<p align="center">
  <img src="docs/shots/underdelve_title.png" width="320" alt="Underdelve title">
  <img src="docs/shots/underdelve_grotto.png" width="320" alt="The Glowcap Grotto">
</p>

A 6×6 mine in four depth zones:

- the Upper Workings
- the Glowcap Hollows
- the Crystal Veins
- the Ember Deep

You collect ore, buy a copper pot to keep ceiling drips off your head, and
shatter crystal walls with a tuning fork. After that come the mine lifts, ropes
and the Deep Gate. Six lanterns are your lives. Watch out for the Gloom.

<details>
<summary>The whole mine: all 36 screens, stitched from headless screenshots (spoilers)</summary>
<p align="center"><img src="docs/shots/underdelve_map.png" alt="All 36 screens of the Underdelve map"></p>
</details>

### 02 · GRUB SHIFT

<p align="center">
  <img src="docs/shots/grubshift.gif" width="640" alt="Grub Shift: a toss sets off a chain of fizz-pod explosions">
</p>
<p align="center">
  <img src="docs/shots/grubshift_title.png" width="320" alt="Grub Shift title">
  <img src="docs/shots/grubshift_target.png" width="320" alt="Targeting a tool">
</p>

Meet the quota before the last shift ends:

- **Tools:** every one of your seven works once per shift. Continuous moves push
  grubs into pits, and rolling off a planter stomps them.
- **Pods:** three on one tile blow it open.
- **Grubs:** larvae grow into queens, and queens lay eggs. If an egg survives
  a shift, you're fired.
- **Contracts:** win three in a row to become Employee of the Month.

### 03 · ROOFCAT

<p align="center">
  <img src="docs/shots/roofcat.gif" width="640" alt="Roofcat: Harissa runs and throws jasmine stars">
</p>
<p align="center">
  <img src="docs/shots/roofcat_souk.png" width="320" alt="The Spice Souk">
  <img src="docs/shots/roofcat_harbour.png" width="320" alt="The harbour at dusk">
</p>
<p align="center">
  <img src="docs/shots/roofcat_boss.png" width="320" alt="The Magpie King">
  <img src="docs/shots/roofcat_night.png" width="320" alt="The night route">
</p>

- **Stages:** whitewashed rooftops, a spice souk (with a Hijaz-flavoured tune),
  a harbour at dusk and the fort walls at night.
- **Fighting back:** throw jasmine stars. Catnip lets you throw more at once.
- **Extra lives:** hit the paper lanterns for 1-ups.
- **Dying:** you float around as a spirit for a few seconds before coming back.

## Install on PS Vita

You need a Vita running HENkaku / h-encore / Ensō with **VitaShell**.

1. Download `ufo40.vpk` from the [latest release](https://github.com/naniiic137/ufo-40/releases).
2. Copy it to the Vita:
   - **USB:** in VitaShell press **SELECT** to start USB mode, then copy the file to `ux0:`.
   - **FTP:** press **SELECT** for FTP, then upload the file with any FTP client.
3. In VitaShell, find `ufo40.vpk`, press **Cross** and choose **Install**.
4. Start **UFO 40** from the LiveArea bubble (title ID `UFOF00040`).

Saves live in `ux0:data/UFO40/`. The game runs at 960×540 (the 320×180
framebuffer at exactly ×3) with 2-pixel bars top and bottom.

## Windows

Download `UFO40-windows.zip` from the releases and run `ufo40.exe`.

- Saves go to `%APPDATA%\UFO40`. If you put an empty `portable.txt` next to the
  exe, it keeps them in a `saves` folder beside it instead.
- **F11** or **Alt+Enter** toggles fullscreen. The window scale is in Settings.

## Controls

| Console | Keyboard (PC / web) | PS Vita | Gamepad |
|---|---|---|---|
| D-pad | Arrows or WASD | D-pad or left stick | D-pad or left stick |
| A | Z, J or Space | Cross | A (bottom face button) |
| B | X or K | Circle | B (right face button) |
| START | Enter or Esc | START | Start |
| SELECT | Shift or Backspace | SELECT | Back / Select |

- **START** opens the pause menu in every game: Resume, Restart, Controls, Quit.
- **SELECT** opens Settings in the library.
- On phones the web page shows an on-screen D-pad with A, B, START and SELECT.

## Building

The core is portable C11. Local builds on Windows use the portable
[w64devkit](https://github.com/skeeto/w64devkit) toolchain (gcc + make), and
nothing needs installing.

```sh
make headless     # the headless runner
make test         # build it and run every scripted test in tests/
make shots        # regenerate the README screenshots and GIFs into docs/shots

# Windows desktop build (w64devkit + SDL2 mingw development libraries)
make sdl SDL2_DIR=C:/path/to/SDL2-2.32.10/x86_64-w64-mingw32

# Linux desktop build (uses sdl2-config)
make sdl
```

CMake does the same job for Vita, the web and CI. It reads the same file list
(`sources.mk`), so there's only one list to maintain:

```sh
# PS Vita (inside the vitasdk/vitasdk docker image)
cmake -S . -B build-vita -DUFO40_VITA=ON && cmake --build build-vita    # -> build-vita/ufo40.vpk

# Web (with emsdk activated)
emcmake cmake -S . -B build-web && cmake --build build-web              # -> build-web/index.html

# Desktop + CTest
cmake -S . -B build && cmake --build build && ctest --test-dir build
```

The Vita and web builds run in GitHub Actions (`.github/workflows/`):

| Workflow | What it does |
|---|---|
| `test` | builds the headless runner with gcc on Ubuntu and runs every test (Make and CTest) |
| `vita` | builds the `.vpk` inside `vitasdk/vitasdk:latest` |
| `web` | builds with emsdk and deploys to GitHub Pages from `main` |
| `windows` | cross-compiles with mingw-w64 and zips the exe with `SDL2.dll` |
| `release` | on `v*` tags, attaches the `.vpk` and the Windows zip to a GitHub Release |

## Architecture

```
src/engine/            platform-independent core (no dependencies)
  gfx.c                320x180 8-bit framebuffer, 32-colour palette, sprites with
                       flip / palette swap / outline, clipping, camera, dithering,
                       palette fades and flashes
  font.c               original 5x7 proportional font + 3x5 tiny font
  audio.c              4-channel synth (2 PolyBLEP pulses, 4-bit triangle, LFSR noise),
                       ADSR envelopes, vibrato, sweeps, arpeggios; UFO-MML sequencer
  input.c rng.c        pressed / held / released / auto-repeat; PCG32
  save.c scene.c       CRC32-checked saves; scenes with palette-fade transitions
src/shell/             the console: boot, 40-slot library, settings, pause menu, goal toasts
src/games/*/           one folder per cartridge: rules, art, audio, levels
src/platform/sdl2/     PC + PS Vita + Emscripten in one file
src/platform/headless/ scripted test runner, PNG / GIF / WAV writers, Vita LiveArea art
```

- **Rendering.** Games draw palette indices into the framebuffer. Once per
  frame, the platform layer runs the 57,600 pixels through a 32-entry colour
  table into one streaming SDL texture and scales it by a whole number. That
  keeps the Vita's 444 MHz ARM (which the game clocks up to) nearly idle.
  Palette fades and white flashes happen in that colour table, so they cost
  nothing.
- **Timing.** A fixed 60 Hz step runs with an accumulator. Vsync and 120 Hz
  screens don't change the game's speed.
- **Audio.**
  - Output is 48 kHz stereo, rendered in the SDL audio callback (or on the
    main thread on the web).
  - Music is written in **UFO-MML**, a compact text notation. For example
    `@14 v12 o5 c8 e g ^4 [d e]2` sets instrument, volume and octave, then
    plays notes with lengths, ties and repeat blocks.
  - Sound effects use the same notation and take over one channel for a
    moment, just like old hardware.
  - Every track and jingle is original: 19 compositions across the console and
    the three cartridges.
- **Art.** Sprites are written as strings of palette letters in the C source
  (`k` ink, `y` yellow, `C` cyan and so on), so there are no binary assets at
  all. The Vita LiveArea images are drawn by the engine itself
  (`ufo40_headless --vita-assets vita/sce_sys`) and saved as 8-bit indexed PNGs.
- **Saves.**
  - Global progress (goals and settings) goes in one CRC-checked file.
  - Each game saves its own run or checkpoint.
  - Where they're stored: the exe folder or `%APPDATA%\UFO40` on PC,
    `ux0:data/UFO40/` on Vita and `localStorage` in the browser.
- **Testing.**
  - Tests are plain text scripts (`tests/*.ufs`) that press buttons, wait,
    use cheats to set up situations and then `expect` game state.
  - Grub Shift keeps its rules in one pure struct. A greedy autoplay bot plays
    whole contracts from it as a balance smoke test.

Example test:

```
# UNDERDELVE: spikes kill in one hit, a lantern is lost and Mo respawns in the room.
game underdelve
wait 10
press A
wait 5
cheat room 0 0 150 130
cheat clear_enemies
wait 5
expect lanterns == 6
hold RIGHT 40
expect deaths == 1
expect lanterns == 5
wait 90
expect state == 1
```

## Credits and license

- Design, code, pixel art, music and levels were made for UFO 40 by
  **Hamza Ben Ismail** ([@naniiic137](https://github.com/naniiic137)),
  from Nabeul, Tunisia.
- Inspired by the idea and games of **UFO 50** by Mossmouth. Go play the real thing.
- [SDL2](https://www.libsdl.org/) (zlib license) handles windows, input and
  audio on every platform. [vitasdk](https://vitasdk.org/) and
  [Emscripten](https://emscripten.org/) handle the Vita and web builds.

Code is released under the **MIT License**. The pixel art, music, sound effects
and level designs made for this project are released under the same license.
See [LICENSE](LICENSE).
