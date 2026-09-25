<p align="center">
  <img src="docs/shots/boot.gif" width="640" alt="UFO 40 boot animation: a flying saucer beams the logo down">
</p>

<h1 align="center">UFO 40</h1>

<p align="center">
  <b>A pretend 1980s console with forty cartridges (well, twelve so far), built from scratch<br>
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
**Beamdown Softworks** console.

Each cartridge is a tribute to one UFO 50 game and sits in the slot with that
game's number. It plays by the same rules: the controls, systems, foes, items,
scoring, goals and structure follow the original as closely as written sources
describe it. Every cartridge has a design document in `docs/games/` that lists
each mechanic and where it comes from. Everything you see and hear is new:
the names, characters, setting, pixel art, levels and chiptune music. Where the
original builds its levels at random, so does the tribute, with its own
generator. Where the original's levels are hand-made, the tribute's are too,
drawn from scratch.

Everything is written in plain C with no dependencies in the core:

- a 320×180 indexed-colour software renderer
- a four-channel chiptune synthesiser with its own music format
- a scene system and a save system

A thin SDL2 layer runs that same core on a modded **PS Vita** (HENkaku/Ensō),
on **Windows/Linux** and in the **browser** through Emscripten. A headless build
drives the whole console from scripted input for testing and for every
screenshot and GIF on this page.

## The library (12 of 40 loaded)

<p align="center"><img src="docs/shots/library.png" width="640" alt="The UFO 40 game library with 40 cartridge slots"></p>

| # | Cartridge | Tribute to | What plays the same | What's ours |
|---|---|---|---|---|
| 01 | **UNDERDELVE** | Barbuta | an 8×8 wrapping map, one-hit deaths, six spare lives and no continue, one fixed jump with no air control, a roaming death that moves a room whenever you do, traps, hidden walls and ladders, hint-givers, a death taken on purpose, items that open the way, three paths to the final boss | Mo the mole, a dark mine, the Gloom, the glow-worms, 64 new screens |
| 02 | **GRUB SHIFT** | Bug Hunter | a 6×5 field dealt afresh each morning, seven tools that each work once a shift, a shop open any time, pods that blow up in threes, grubs that grow by colour into adults, queens and eggs, 30 kills in 10, 9 or 8 shifts | Tilly the farm robot, the grub species, 41 tools |
| 03 | **ROOFCAT** | Ninpek | one long auto-scrolling world, double jumps, thrown stars, a spirit that floats back after a death, token pickups, one final boss, a harder second loop | Harissa the courier cat, a Tunisian seaside town, Old Crab |
| 05 | **PETAL PARADE** | Magic Garden | a 12×12 field, a trail of followers you must never run into, saving them on star pads for rising points, potions by strength, a witch who plants mushrooms | Lina the gardener, petalpups, sun circles, Madame Nettle |
| 06 | **TIN TROOP** | Mortol | 20 lives that carry through ten levels, the arrow, bomb and stone sacrifices, bodies as ledges and weights, water, fire and plants, a ship that drops the next life | a toy army in a toymaker's house, the Jack of the Chest, 10 new levels |
| 07 | **SKYWELL** | Velgress | a random shaft of crumbling platforms, a roller that only follows you up, stun instead of damage, four-way shooting, a shop between levels, a key bird, a locked fourth level | Kip the scrap-diver, the Grinder, the Tinker, the Well Eye |
| 09 | **BANNERFALL** | Attactics | a 6×8 field between two keeps, a countdown turn where you drag troops between lanes, the same end-of-turn order, eight unit types with promotions and heroes, a 24-battle campaign, ranked, survival and 2P versus | the Marigold Guard and the Thistle Host, all units and battles |
| 14 | **CUTLASS CUP** | Bushido Ball | first to 8 points, aimed and curved strikes, lobs, secondary weapons, charged super shots, the fouls and penalty points, six fighters, a tournament, 2P versus and co-op doubles | six corsairs, the galley *Sabra*, judge Rais Mabrouk |
| 15 | **FENNEC FOUNTAIN** | Block Koala | pushing numbered blocks (big pushes small, 1s add up), blocks that turn to marble, blue and black blocks, mimics, unlimited undo, fifty rooms behind gates, a room editor | Fen the fennec, an oasis, 50 new rooms |
| 16 | **TINTAIL** | Camouflage | sneaking on a grid past watchers by taking the colour of the ground you stand on, hollow logs, sun and rain pads, hatchlings to carry home, a danger view, undo | Kama the chameleon, Salt Island, toads and storks, 15 new levels |
| 25 | **OPEN HOUSE** | Party House | drawing guests from a rolodex into a house with limited space, the original's 46 guest abilities, costs and trouble, police, fire marshal and bans, set guest lists and a random list, win within 25 nights, 2P hot-seat | a whitewashed house by the sea, all 46 guests' names and faces |
| 28 | **DUNE EXPRESS** | Rail Heist | real time until a guard is alert, then short turns, one-bullet lawmen, punches through walls and floors, carrying and throwing, hiding in barrels, levers, a crank gun, a ram, three outlaws, angel/devil/time stars, 2P versus | desert outlaws, the Governor's tax trains, Baraka the camel, 20 new missions |
| 04, 08, 10–13, 17–24, 26–27, 29–40 | *coming soon* | | | still in the saucer's cargo hold |

Every cartridge has three goals, which follow the original's own three: a
**Beacon** (a side challenge), the **Saucer** (beating the game) and the
**Alien** (a secret, harder challenge). Earned goals light up on the
cartridge in the library.

### 01 · UNDERDELVE

*Tribute to Barbuta (UFO 50 #1).*

<p align="center">
  <img src="docs/shots/underdelve.gif" width="640" alt="Underdelve gameplay: Mo the mole swings his pick in the Glowcap Grotto">
</p>
<p align="center">
  <img src="docs/shots/underdelve_camp.png" width="320" alt="Mo's camp, where the cartridge starts">
  <img src="docs/shots/underdelve_grotto.png" width="320" alt="The Glowcap Grotto">
</p>

An 8×8 mine that wraps round at the sides, in four zones: the Ember Deep at
the bottom, then the Crystal Veins, the Glowcap Hollows and the headframe at
the top.

- **Plays like Barbuta:** it boots straight into the mine. Any touch kills;
  Mo has six spare lanterns, then the delve is over, with no continue. He
  walks slowly, his pick barely reaches, and his one fixed jump can't be
  steered once he leaves the ground. Foes come back when you re-enter a room
  and show no damage until they die. The Gloom moves one room whenever Mo
  does; if it finds him it rises in the middle and comes at twice his speed.
  Nothing is explained. The first step right from the camp drops a rock
  from the ceiling; walls hide gems, cracked rock gives way to the pick,
  ladders and ledges hide, still pools are safe and churning ones aren't,
  and one death must be taken on purpose. Glow-worms each give one
  cryptic hint. Items open the way: a copper pot for drips, a tuning
  fork for crystal, a gear crank for the lifts, gloves, a brass tally, a
  canary and a hungry pick. There are three ways up to the Old Lode: a
  lever, the Deep Gate and a 500-ore sledgehammer.
- **Ours:** Mo the mole, the mine and its 64 screens (and which cell holds
  what), the shopkeepers and glow-worms, every tile and the music.

<details>
<summary>The whole mine: all 64 screens, stitched from headless screenshots (spoilers)</summary>
<p align="center"><img src="docs/shots/underdelve_map.png" alt="All 64 screens of the Underdelve map"></p>
</details>

### 02 · GRUB SHIFT

*Tribute to Bug Hunter (UFO 50 #2).*

<p align="center">
  <img src="docs/shots/grubshift.gif" width="640" alt="Grub Shift: one toss sets off a chain of fizz-pod explosions">
</p>
<p align="center">
  <img src="docs/shots/grubshift_title.png" width="320" alt="Grub Shift title">
  <img src="docs/shots/grubshift_target.png" width="320" alt="Aiming a toss">
</p>

- **Plays like Bug Hunter:** a 6×5 field whose ground is dealt afresh each
  morning, and seven tool slots that each work once per shift. The shop is
  open any time: swap a tool in and use it at once. Rolls shove grubs and
  squash them from planters; shots can't go uphill. Energy pods blow up when
  three share a tile. Each night one colour whose grubs survived grows a
  level (larva, adult, queen), and its new grubs hatch at that level;
  queens left overnight turn into eggs, and an egg left at the end of a
  shift fails the contract. Clear 30 grubs before the shifts run out; stop
  after a win and the streak waits for you.
- **Ours:** Tilly the farm robot, the dome, the grub species and the names
  and looks of all 41 tools. Three contracts in a row make Employee of the
  Month.

### 03 · ROOFCAT

*Tribute to Ninpek (UFO 50 #3).*

<p align="center">
  <img src="docs/shots/roofcat.gif" width="640" alt="Roofcat: Harissa runs and throws jasmine stars">
</p>
<p align="center">
  <img src="docs/shots/roofcat_souk.png" width="320" alt="The Spice Souk">
  <img src="docs/shots/roofcat_harbour.png" width="320" alt="The harbour at night">
</p>
<p align="center">
  <img src="docs/shots/roofcat_boss.png" width="320" alt="Old Crab">
  <img src="docs/shots/roofcat_night.png" width="320" alt="The Night Route">
</p>

- **Plays like Ninpek:** one continuous auto-scrolling town, a double jump and
  stars thrown at a steady rate. Foes drop tokens instead of points, a spirit
  floats back down after a death, and extra lives come from score. There are
  hidden spots, one boss at the very end and a harder second loop.
- **Ours:** Harissa the courier cat and Grandma Zohra's parcel, a whitewashed
  seaside town (the rooftops, the Spice Souk, the fort walls and the harbour),
  the Magpie Mob, Old Crab and the Night Route.

### 05 · PETAL PARADE

*Tribute to Magic Garden (UFO 50 #5).*

<p align="center">
  <img src="docs/shots/petalparade.gif" width="640" alt="Petal Parade: Lina leads a line of petalpups onto a sun circle">
</p>
<p align="center">
  <img src="docs/shots/petalparade_title.png" width="320" alt="Petal Parade title">
  <img src="docs/shots/petalparade_night.png" width="320" alt="The moonlit garden">
</p>

- **Plays like Magic Garden:** you never stop walking. Pups you walk over
  follow in a line, and running into it ends the game, as does a wall, an
  angry pup or a mushroom. Let the line go on a star pad and each pup scores
  more than the last. Let it go anywhere else and they turn angry. Saved pups
  fill a counter that brews potions of four strengths; drink one to smash
  what's in your way for a while. A witch plants mushrooms if you dawdle.
  Save 200 to win.
- **Ours:** Lina the palace gardener, the petalpups and the brambles they turn
  into, the sun circles, the nectar jars, Madame Nettle and four seasons of
  garden.

### 06 · TIN TROOP

*Tribute to Mortol (UFO 50 #6).*

<p align="center">
  <img src="docs/shots/tintroop.gif" width="640" alt="Tin Troop: a burning soldier throws himself at the Jack of the Chest">
</p>
<p align="center">
  <img src="docs/shots/tintroop_shelf.png" width="320" alt="A soldier lodged in the bookshelf is a step for the next">
  <img src="docs/shots/tintroop_bath.png" width="320" alt="Bath time">
</p>

- **Plays like Mortol:** you have twenty lives, and spending them is how you
  get anywhere. A soldier can fly into a wall and stay there as a ledge, blow
  himself up to clear blocks and foes, or turn into a stone block. Bodies
  press switches and weigh down scales. Drowned soldiers float, burning ones
  can't be hurt, and seeded ones grow into vines. New soldiers parachute from
  a ship that follows your progress. Lives carry from level to level, and
  replaying a level to do better raises every level after it. There are ten
  levels in four worlds, then the final boss.
- **Ours:** Captain Pip's tin soldiers, the toy blimp, a toymaker's house at
  night (nursery, bathroom, kitchen and toy chest), wind-up mice, paper darts,
  tin rams and the Jack of the Chest. All ten levels are new layouts.

### 07 · SKYWELL

*Tribute to Velgress (UFO 50 #7).*

<p align="center">
  <img src="docs/shots/skywell.gif" width="640" alt="Skywell: Kip climbs a crumbling shaft">
</p>
<p align="center">
  <img src="docs/shots/skywell_reef.png" width="320" alt="The Sky Reef">
  <img src="docs/shots/skywell_eye.png" width="320" alt="The Well Eye">
</p>

- **Plays like Velgress:** every run is a new shaft. Platforms crumble soon
  after you land. A spiked roller follows you up and is the only thing that
  can kill you; every other hazard just stuns. You double jump, shoot in four
  directions and collect coins for the shop between levels. Shoot down the
  bird on each level's 12th floor for a key, and with all three keys a fourth
  level opens with a final boss.
- **Ours:** Kip the scrap-diver, the Grinder, the Roots, the Sparkworks, the
  Sky Reef, the Tinker's cart, the Keeper Owl and the Well Eye.

### 09 · BANNERFALL

<p align="center">
  <img src="docs/shots/bannerfall.gif" width="640" alt="Bannerfall: troops drag into lanes, then march, shoot and clash">
</p>
<p align="center">
  <img src="docs/shots/bannerfall_campaign.png" width="320" alt="The campaign map">
  <img src="docs/shots/bannerfall_volley.png" width="320" alt="Arrows fly as the turn ends">
</p>

*A tribute to **Attactics** (UFO 50 #9).*

- **Plays the same:** a 6 × 8 field between two keeps and one new unit a turn
  on a random row. While the timer counts down from 9 you drag units up, down
  or back (forward only to where you picked them up). Then everyone attacks
  and marches in Attactics' order.
- **The troops:** eight units with the original's rules. Footmen in a column
  of three shrug off melee, bowmen stand and shoot, wardens stop arrows until
  they're hurt, riders move twice, and powdermen blow up. Every fifth
  promotion calls a champion.
- **The CPU** never moves its troops, but it gets extra ones. The 24-battle
  campaign uses the original's banners, unit pools and percentages. Ranked,
  Survival and 2P Versus are there too.
- **Ours:** the Marigold Guard and the Thistle Host, every unit's name and
  look, the keeps, the battle names, the march and the battle music.

### 14 · CUTLASS CUP

<p align="center">
  <img src="docs/shots/cutlass.gif" width="640" alt="Cutlass Cup: Karim and Omar rally on the galley deck">
</p>
<p align="center">
  <img src="docs/shots/cutlass_select.png" width="320" alt="Choose your crew">
  <img src="docs/shots/cutlass_rally.png" width="320" alt="Karim charges a Super Shot">
</p>

*A tribute to **Bushido Ball** (UFO 50 #14).*

- **Plays the same:** knock the ball past your rival for a point, first to 8.
  Up and down aim, back lobs, and a rolling strike goes faster or curves.
- **Meter:** two strikes fill half a bar. A double tap throws your secondary
  weapon, and holding the button charges a Super Shot, which can be caught
  and mashed back.
- **Laws:** stalling, weapon fouls and serve interference; the third foul
  gives away a point.
- **Modes:** six fighters with the original's stats and kits, a five-match
  tournament with continues, 2P versus and 2P co-op doubles. Like the
  original, there's no music during play.
- **Ours:** the harbour and the galley *Sabra*, the judge Rais Mabrouk, and
  Hamdi, Leila, Nour, Karim, Zina and Omar with their coins, sea urchins,
  harpoon, sirocco, darts and powder pots. The tunes are ours too.

### 15 · FENNEC FOUNTAIN

<p align="center">
  <img src="docs/shots/fennec.gif" width="640" alt="Fennec Fountain: Fen pushes numbered stones to the dry spring">
</p>
<p align="center">
  <img src="docs/shots/fennec_hub.png" width="320" alt="The oasis hub">
  <img src="docs/shots/fennec_bath.png" width="320" alt="Room 50, the Vizier's bath">
</p>

*A tribute to **Block Koala** (UFO 50 #15).*

- **Plays the same:** Sokoban where numbers are weights. A block only pushes
  numbers as big as its own or smaller, pushing onto a 1 adds up, and five
  turns to marble.
- **Special blocks:** blue blocks only move for other blocks, black ones
  crumble when you stop pushing them, and geckos copy your steps. Undo is
  unlimited.
- **Structure:** fifty rooms around a hub, with gates at 5, 10, 20, 30 and 40
  drops, and a workshop for building ten rooms of your own.
- **Ours:** Fen and Zizi the fennecs, Grand Vizier Humph and his bath, the
  oasis garden, all fifty rooms (every wall and every block) and the music.

### 16 · TINTAIL

<p align="center">
  <img src="docs/shots/tintail.gif" width="640" alt="Tintail: Kama changes colour to slip past three watching toads">
</p>
<p align="center">
  <img src="docs/shots/tintail_danger.png" width="320" alt="Holding B shows every predator's danger area">
  <img src="docs/shots/tintail_map.png" width="320" alt="The Salt Island map">
</p>

*A tribute to **Camouflage** (UFO 50 #16).*

- **Plays the same:** a grid stealth puzzle in real time. Kama takes the
  colour of the tile she stands on; matching the ground, she walks through a
  predator's sight unseen. One colour at a time, and she's exposed while she
  changes, so every crossing has to be planned a step ahead.
- **The island:** toads watch fixed areas, storks walk fixed loops (they block
  the toads' view, and walk into you whatever colour you are), hollow logs
  hide you, and sun and rain pads dry or wet the grass. A button shows every
  danger area, and if you're eaten you can undo a few steps.
- **Structure:** fifteen single-screen levels in order across the island,
  each with two prickly pears and a hatchling that follows one step behind.
  Only the best single escape counts toward completion.
- **Ours:** Kama, the toads, storks and falcon, Salt Island and the Sun Gate,
  all fifteen levels (a solver in the tests proves each one), and the music.

### 25 · OPEN HOUSE

<p align="center">
  <img src="docs/shots/openhouse.gif" width="640" alt="Open House: guests arrive through the door and a paparazzo gets to work">
</p>
<p align="center">
  <img src="docs/shots/openhouse_party.png" width="320" alt="A busy night in the house">
  <img src="docs/shots/openhouse_shop.png" width="320" alt="The shop">
</p>

*A tribute to **Party House** (UFO 50 #25).*

- **Plays the same:** open the door and a random guest from your rolodex
  walks in. Three TROUBLE! guests bring the police, and guests brought along
  can overflow the house for the fire marshal; either way nobody pays and one
  guest misses the next party. End the party in time and everyone pays
  popularity (to buy guests) and cash (to add space).
- **The guests:** all 46 of Party House's guests with their costs, pay,
  talents and trouble, from fetchers, bouncers and peekers to drummers,
  upstarts and the nine star guests. Win by getting four stars into one
  party within 25 nights.
- **Structure:** five set guest lists with the original's pools, then a
  Random list and the five-win streak. 2P Versus takes alternate nights and
  shares the shop.
- **Ours:** the house by the sea, the nosy neighbour's lamp, every guest's
  name, portrait and line (the Saucer Pilot, the Wish Fish, the Rai Singer,
  the Goat...), the list names and the music.

### 28 · DUNE EXPRESS

<p align="center">
  <img src="docs/shots/dune.gif" width="640" alt="Dune Express: Khaled breaks a crate, crosses the roofs and drops in behind a guard for the strongbox">
</p>
<p align="center">
  <img src="docs/shots/dune_turns.png" width="320" alt="Sahar walks past a stunned guard during her turn">
  <img src="docs/shots/dune_roofs.png" width="320" alt="Dusk on the longest train, a guard on the next roof">
</p>

*A tribute to **Rail Heist** (UFO 50 #28).*

- **Plays the same:** rob a moving train and escape to your mount at the far
  end before the train reaches town. While every guard stands still it's real
  time; once one is up and about it goes in turns, ten seconds for you (the
  clock runs) and a few for the lawmen (it stops).
- **The rules:** a guard shoots an outlaw he sees straight ahead, once, and
  then can only punch and throw; a gunshot wakes everyone nearby. Duck behind
  anything a tile high, punch through walls, floors and ceilings, carry and
  throw barrels, crates, anvils, powder sticks and geese, hide in a barrel,
  and load the gun slowly. Levers flip every iron gate, the crank gun turns
  on the lawmen from behind, and a punched ram charges.
- **Structure:** twenty missions in order for the same three outlaws, one or
  two a mission, with loot, a rescue, a full belt of bullets and the Governor
  to finish; angel, devil and time stars; 2P Versus on a longer train dealt
  fresh each time.
- **Ours:** Khaled, the Veil, Sahar and Baraka, the Salt Line and all twenty
  trains (every one is won by a scripted run in the tests, without a kill and
  under its time star), the powder sticks, geese, rams and hamsa, and the
  music.

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
- **Two players:** the 2-player modes of Bannerfall and Cutlass Cup take two
  gamepads, or split the keyboard: player 1 on WASD + F/G, player 2 on the
  arrows + K/L. On the Vita (one controller) those modes are locked.
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
- **Timing.** The game logic always steps at a fixed 60 Hz. A frame that
  lasts a whole number of 1/60 s ticks (within a millisecond) runs exactly
  that many steps, so a 60 Hz vsync gives one step per refresh with no drift
  or judder; other refresh rates fall back to an accumulator. 120 Hz screens
  don't change the game's speed.
- **Audio.**
  - Output is 48 kHz stereo, rendered in the SDL audio callback (or on the
    main thread on the web).
  - Music is written in **UFO-MML**, a compact text notation. For example
    `@14 v12 o5 c8 e g ^4 [d e]2` sets instrument, volume and octave, then
    plays notes with lengths, ties and repeat blocks.
  - Sound effects use the same notation and take over one channel for a
    moment, just like old hardware.
  - Every track and jingle is original: 80 compositions across the console and
    the twelve cartridges.
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
  - Grub Shift keeps its rules in one pure struct. An autoplay bot that looks
    two moves ahead plays whole contracts from it as a balance smoke test.
  - Underdelve checks its mine with probes built on its own physics: every
    room walked and jumped from every way in, the rooms reachable from the
    camp with each route into the tower, and no way into a room that drops a
    returning Mo onto spikes. Its opening is also played by buttons alone.
  - Tin Troop has a route test for every level that plays its obstacles the
    intended way, and Skywell has a demo climber that checks generated pits
    can be climbed.
  - Fennec Fountain ships every room with its shortest solution, and a test
    replays all fifty on the real rules.

Example test (`tests/ud_02_hazard_death.ufs`):

```
# UNDERDELVE: spikes kill in one hit; a lantern goes out and Mo comes back
# where he entered the room.
game underdelve
wait 15
cheat no_gloom
cheat room 5 0 170 130
cheat clear_enemies
wait 5
expect lanterns == 6
hold RIGHT 60
expect deaths == 1
expect lanterns == 5
wait 90
expect state == 1
expect px < 180
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
