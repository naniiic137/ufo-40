# 29 · ANTIPODE

*Internal design document. Not shown in the product.*

## Tribute to

**Vainger** (UFO 50 game #29, Mossmouth, "March 1987"). The rules were
researched from text only: the community wiki's raw pages (Vainger, Debug
mode, Cheats, Multiplayer), the Steam guides "The missing manuals" (section
29), "Gift, Gold & Cherry" and "Vainger - 100% with Full Map" (its text and
comments only), Steam threads, written reviews and TV Tropes (listed under
Sources; the full notes are in the research folder, `29-vainger.md`). No UFO
50 images, video, sprites, music, maps or text were used as references (the
100% guide's maps were not opened), and nothing was taken from a UFO 50
install.

**Map type: fixed.** Vainger's station is hand-made and the same every run:
"everything is fixed (map, items, bosses)" [MH]. Ours is fixed too, and all
our own: four sector maps drawn as grids of one-screen cells, each room
built from our own cell templates (`apd_world.c`). Only the structure
follows the original:

| Full scale | Vainger | ANTIPODE |
|---|---|---|
| Players | 1, no 2-player mode [MH], [MH-MP] | 1 |
| Sectors | Theta (start, hub, the locked final teleporter in the north), Verde, Latom, and Central Control behind four keycodes [MH], [MH-DEBUG] | the Gantry (start, hub, the locked uplink up north), the Brine, the Coil, and the Crown behind four ciphers |
| Teleporters | Theta to Verde lower left and lower right, Theta to Latom, the final one [MH], [STEAM-HELP] | the same four links |
| Rooms | not counted in any source; the 100% guide's grid reaches at least 21 rooms a sector (A21) and the start is C15 [G-MAP]; "cover the entire station in only a few minutes" [LIZ] | 98: 35 in the Gantry, 30 in the Brine, 29 in the Coil, 4 in the Crown; 1 to 4 screens each |
| Upgrades | 25 shield upgrades (+5 each, 75 to 200), 3 clones (2 to 5), 2 stabilizers, 4 mods [MH], [G-MAP] | 25 armor cells (75 to 200), 3 spare bodies (2 to 5), 2 dampers, 4 mods: 34 |
| Where they are | Heat Mod a few rooms east of the start; Force Mod west Verde; Multi Mod several rooms east of Latom's teleporter; Pulse Mod north-east Latom; clones central Theta, north-west Theta and central Latom; stabilizers far north-east of Verde and of Latom [MH] | EMBER four rooms east of the start; MASS in the west Brine; TWIN four rooms east of the Coil's teleporter; VOLT in the north-east Coil; spares in the middle of the Gantry, in the hidden room by the west teleporter, and in the middle of the Coil; dampers in the far north-east of the Brine and of the Coil |
| Hidden upgrades | about six, "obscured by objects, terrain, or enemies", one of them in the start room, one in Central Control; behind a big red eye in a west shaft, in slime under a save, behind a blue spawner in a shaft [G-MAP], [STEAM-CHERRY], [STEAM-ITEMS] | six: under the desk in the start room, behind a star-eye in the west shaft, in the acid under the north-west Brine cradle, behind a hivebox in a Coil shaft, in the acid of the Brine's south corridor, in the acid before the Crown |
| Clearance | 3 levels, terminals in south Verde (1), central Verde (2), central Latom (3) [MH] | 3 pass levels, terminals in the south Brine, the middle of the Brine, the middle of the Coil |
| Bosses | a mini-boss below the armory by the west teleporter ("the first miniboss"), a "big robot guy" in the centre that moves the terrain, Ramses and Y-Vainger, the final boss in 3 phases; four keycodes, "boss codes" [STEAM-HELP], [STEAM-BUG], [MH-DEBUG], [STEAM-SECTOR], [TVT] | 7 fights: the Scrapper (below the armory), the Warden (the Gantry's middle; cipher 1), Brackish (cipher 2), the Lamprey (a second minor one, in the Coil), the Coilheart (cipher 3), Obverse (cipher 4), Halloran in three phases |
| Saves | Gene Pods: walking by saves and refills shields and clones; inside, the map and the mods [MH], [MANUALS] | 10 cradles, the same |
| Goals | 3 [MH], [GGC] | the same 3 |
| Stats | Upgrades, Exploration, Times Died [MH] | the same three, on the title |
| Length | 100 % in about an hour and change; speedrun about 29:38 [LIZ], [SR] | the demo player takes 100 % in about 30 minutes of game time |

## The structure

The suit wakes in the cradle bay in the middle of the **Gantry**. Doors
lead west to a cradle and the maintenance rooms, and east, four rooms on,
to the **EMBER** mod (the gift). With SCORCH the orange barrels burn: the
west shaft down to the armory, the Scrapper below it and the west
teleporter; the flue east of EMBER (a heat shaft) and the east teleporter;
the way to the Warden's hall. The Coil's teleporter is under a pass-2 hatch;
the uplink to the Crown is open from the start but takes four ciphers.

The **Brine** (both Gantry teleporters land there, west and east of its
south corridor) holds pass 1 in the south and pass 2 in the middle, the
**MASS** mod in the west behind a pass-1 door, Brackish in the settling
tank, and, behind electric fields, Obverse's mirror room and a damper in
the far north-east. Acid lies along its floors.

The **Coil** (pass 2 from the Gantry): the Lamprey guards the **TWIN** mod;
pass 3 is in the middle; a nano-wall of three sentry orbs closes the spare
body; the Coilheart sits in the dynamo; the **VOLT** mod is behind a pass-3
door north-east, and the damper beyond a shutter whose target sits inside
the wall (PHASER). A spike hall and a kiln (heat) need IRONSOLE and
KILNSKIN.

With all four ciphers the uplink carries the suit to **the Crown**: a
cradle, an acid room with a hidden cell, and Halloran.

## Mechanics checklist

| Mechanic | How ANTIPODE does it | Source | Test |
|---|---|---|---|
| Walking | one pace, 1.5 px a frame, steering in the air too | [MANUALS] | apd_02 |
| The jump | B jumps; held, higher (about 58 px); let go early, cut short | [MANUALS] | apd_02 |
| The flip | B again in the air flips gravity: falling now toward the ceiling, which is a real floor; as often as you like; only in the air | [MH], [MANUALS] | apd_03 |
| Upside down | walk, crouch, jump (now downward) on the ceiling | [MANUALS] | apd_03 |
| The gun | hold A: automatic fire, a shot every 8 frames; the facing is locked while firing | [MH], [MANUALS] | apd_04 |
| Aiming | UP + A over the head; DOWN + A under the feet only in the air; DOWN on the ground crouches and fires low ahead; no diagonals | [MH], [MANUALS] | apd_04 |
| Interacting | UP at a cradle, a terminal or a teleporter | [MANUALS] | apd_12, apd_13 |
| Armor | 75, +5 a cell, 200 with all 25 | [MH] | apd_15 |
| Harm | machines 5-15 on contact, their shots 6-12; spikes 100 (they kill at the start); 60 frames safe after a hit | [TVT], [TVT-YMMV], [POPCAR] | apd_09 |
| Acid and heat | acid takes 5 every 20 frames; a heat room 1 every 6 frames without KILNSKIN | [MH], [TVT] | apd_09 |
| Electric fields | 30 and a push back; STORMBALL passes | [MH], [TVT] | apd_09, apd_06 |
| Gravity balls | red ones are live while gravity points down, purple while it points up | [TVT] | apd_10 |
| One-way plates | solid from the side your feet are on, red arrows | [TVT] | apd_10 |
| Spare bodies | 2, +1 a pick-up, 5 at most; at 0 armor one takes over at the door you came in by, full armor, the room as you left it (kills and broken walls stay) | [MH], [MANUALS], [TVT] | apd_11 |
| Game over | no spare left: back to the last cradle, everything since lost | [MH] | apd_11 |
| Cradles | walking by: saved, armor and spares full; UP: the MODS and MAP pages, the only place to see the map or move mods; B leaves | [MH], [MANUALS] | apd_12 |
| The map | rooms seen, the current one flashing, yellow doors, thick pass doors (flashing once the pass is held), S cradles, E rooms with something left, T teleporters, dots for terminals; up and down change sector | [MH], [MANUALS] | apd_12 |
| The cradle's numbers | MAP % and ITEMS % beside the suit | [MANUALS] (undocumented numbers), [STEAM-CHERRY] (two percentages) | apd_12 |
| Mods | 4 mods, each in FLIP, FRAME or GUN (or not fitted); several can share a system and add up | [MH], [TVT] | apd_05-07 |
| EMBER | FLARE (a heat wave both ways on a flip), KILNSKIN (half harm, heat-proof, spikes 50), SCORCH (double damage, burns barrels) | [MH], [TVT] | apd_05-07 |
| TWIN | AFTERIMAGE (a copy where you flipped fires when you fire), SATELLITE (a drone that fires with you and latches on to foes), FANSHOT (two more shots at 45°) | [MH], [TVT] | apd_05-07 |
| VOLT | STORMBALL (each flip, up to 3 a jump, wraps you in a ball that does 50 and passes fields), SIPHON (harm done gives armor back), PHASER (shots through walls) | [MH], [TVT] | apd_05-07 |
| MASS | ANVIL (the flip becomes a heavy drop: 50, breaks cracked concrete), IRONSOLE (spikes 10; 5 with KILNSKIN), REACH (double range) | [MH], [TVT-YMMV] | apd_05-07 |
| Overload | one mod a system is safe, +1 a damper; beyond: FLIP costs 2 a flip, GUN 1 a shot, FRAME takes more harm | [MH], [TVT] | apd_08 |
| Doors | yellow ones open at a touch; pass doors with the pass; boss doors shut until the fight ends | [MH] | apd_13, apd_16 |
| Gates | barrels (SCORCH; the blast hurts foes, not you), cracked concrete (ANVIL), fields (STORMBALL), targets in walls (PHASER), nano-walls (the sentry orbs), heat (KILNSKIN), spike halls (IRONSOLE) | [MH], [TVT] | apd_14 |
| Bosses | a bar in the top corner; no leaving once it starts; a spare body finds it with only 250 back; red when low; a chain of blasts | [MH], [TVT] | apd_16, apd_17 |
| Ciphers | from the Warden, Brackish, the Coilheart and Obverse; all four open the uplink | [MH], [STEAM-SECTOR] | apd_13, apd_16 |
| Obverse | the other suit: flips to your side, sword beams, up to five in a row once she is low | [TVT] | apd_17 |
| Halloran | 1: a walker whose legs can always be shot, its hand fires three volleys then it rams; now and then it turns shots back for a while. 2: the heart flies about dropping orbs and pink rings. 3: the head; shoot the neck's joints to bring it down; it shuts itself away for 20 s at a time calling lightning that deletes shots, with pink rings and orbs | [TVT], [STEAM-BOSS] | apd_17 |
| Completion | rooms seen and upgrades found; the Crown's hall counts once Halloran is beaten, so everything before the last fight is 98 % | [G-MAP] | apd_24 |
| Endings | below 100 %: the visor stays dark; at 100 %: the pilot is shown | [MH], [G-MAP] | apd_19 |
| Goals | gift: get a mod; gold: beat the game; cherry: beat it with 100 % rooms and upgrades, and see the credits through | [MH], [GGC], [G-MAP] | apd_19 |
| The code | a code (LOAD-KITS) gives every upgrade, with no saving and no goals; scratched under the first terminal, seen coming at it from the left | [MH-CHEATS], [LIZ] | apd_21 |
| The hidden room | by the west teleporter, its walls burn with SCORCH (from either side); a grey wall falls when two buttons are hit at once (AFTERIMAGE, or PHASER with FANSHOT and REACH) | [STEAM-HELP] | apd_14 |
| Stats | Upgrades, Exploration, Times Died | [MH] | apd_20 |
| Playable start to end | the demo player, with real presses from the title, takes every room and upgrade and beats Halloran: cherry | | apd_23 |

### Readings we had to choose

- **Gravity and the d-pad:** "up" and "down" go with the suit: DOWN always
  crouches (toward the feet), UP aims over the head, whichever way up it is.
- **A flip** stops the fall where it is and starts the new one from rest.
- **Numbers no source gives:** walk 1.5 px a frame; jump about 58 px high;
  gravity 0.25 px a frame per frame, falls up to 4.5; a shot every 8 frames,
  5 damage, 5 px a frame for 22 frames (REACH: 44); ANVIL drops at 8 px a
  frame; STORMBALL lasts 40 frames a flip (it doesn't end on landing; the
  three-flip count does); AFTERIMAGE 4 s; FLARE 15 a wave; SATELLITE shots
  3, latched 1 every 8 frames; SIPHON 1 armor for every 10 harm; an
  overloaded FRAME takes half again; acid 5 every 20 frames; heat 1 every 6;
  electric fields 30; mercy 60 frames.
- **Unfitted mods:** a mod can also sit in no system (the OFF row), so
  four mods and no dampers need not overload. A new mod goes into the first
  system with room (GUN, then FRAME, then FLIP), else OFF.
- **Foe and boss hit points:** treadles 15, 40 and 80; spitter 25;
  star-eye 60; hivebox 80; mite 5; skipjack 25; skimmer 15; sentry orb 30;
  the Scrapper 300, the Warden 600, Brackish 700, the Lamprey 400, the
  Coilheart 700 (its nodes 60), Obverse 800, Halloran 700, 600 and 800.
- **The boss list:** the sources name only Ramses, Y-Vainger, "the first
  miniboss", a big robot in the centre and the three-phase final boss. We
  made two minor fights (one a sector), four cipher holders and the final;
  Brackish stands where Ramses stands (its behaviour is ours); the Warden's
  moving floor follows the "move that makes the terrain move" [STEAM-BUG].
  Halloran's middle phase is ours (sources describe only the first and the
  last).
- **The two clones of the Gantry:** the wiki puts them in the "central and
  north-western" Gantry; the help thread puts the hidden one by the west
  teleporter. Ours: one in the middle, one in the hidden room by the west
  teleporter (which is in the west of the map).
- **Enemies drop nothing:** no source mentions health drops, and SIPHON
  exists, so armor comes back only at cradles, from SIPHON and with a new
  cell.
- **Gates stay open** once burnt, broken or shot (they are saved).
- **The map's E** marks a room seen that still holds an upgrade.
- **Where a boss starts:** the walkers start on the far side from the suit.

## What is ours

- **Name:** ANTIPODE (1987, Beamdown Softworks).
- **Characters:** the AP-2 suit ANTIPODE and its pilot, Dr. Sela Ambry (shown
  only at 100 %); Director Halloran; Obverse, the second suit; Tallow
  Station on the moon Mira-6, built by Corvid Arms.
- **The machines:** treadles (grey, red, gold), spitters, star-eyes,
  hiveboxes and mites, skipjacks, skimmers, sentry orbs; the Scrapper, the
  Warden, Brackish, the Lamprey, the Coilheart, Obverse, Halloran.
- **The world:** the Gantry, the Brine, the Coil and the Crown, 98 rooms on
  our own maps, from 24 cell templates of our own.
- **The mods' names:** EMBER (FLARE, KILNSKIN, SCORCH), TWIN (AFTERIMAGE,
  SATELLITE, FANSHOT), VOLT (STORMBALL, SIPHON, PHASER), MASS (ANVIL,
  IRONSOLE, REACH); systems FLIP, FRAME, GUN; armor cells, spare bodies,
  dampers, pass levels, ciphers, cradles.
- **Music:** "Antipode" (title), "Wake in the Cradle" (story), "Gantry
  Shift", "Brine", "The Coil", "The Crown", "Cradle", "Hostile Unit"
  (fights), "Halloran" (the last fight), "Visor Up" (ending), "Tallow
  Station" (credits), and the jingles "Upgrade" and "Signal Lost".
- **Words:** the story, fifteen terminal logs, both endings, the credits, the
  code and every label.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three UFO
40 goals, which are Vainger's own (gift, gold, cherry). The code screen on
the title stands in for the UFO 50 Terminal, as on other UFO 40 cartridges;
the hint to it is in the game, as in the original.

## Controls

The owner's layout puts the main action (the gun) on **A** and jump/flip on
**B**; Vainger has jump/flip on A and the gun on B [MANUALS]. The cartridge
card's CONTROLS page can swap them back.

| Input | Action |
|---|---|
| LEFT / RIGHT | walk; steer in the air |
| DOWN | crouch (fire low ahead) |
| A (hold) | automatic fire; the facing is locked |
| UP + A | fire over the head |
| DOWN + A in the air | fire under the feet |
| B | jump (hold: higher) |
| B in the air | flip gravity |
| UP | step into a cradle, read a terminal, use a teleporter |
| In a cradle | LEFT / RIGHT pick a mod (RIGHT past the last: the map), UP / DOWN move it between FLIP, FRAME, GUN and OFF; on the map UP / DOWN change sector; B leaves |
| START | pause |

## Not confirmed

| Control or rule | Our reading |
|---|---|
| The fire and jump buttons | B fires and A jumps in the original [MANUALS]; A fires here (owner's layout, swappable) |
| Leaving a cradle | B in the original is the fire button; here B (the menus' back button) |
| Crouch and aim upside down | relative to the suit (DOWN crouches whichever way up) |
| Taking a mod out of every system | an OFF row in the cradle |
| Where a new mod is fitted | the first system with room, else OFF |
| The cradle's numbers | MAP % and ITEMS % |
| Every number in "Readings we had to choose" | as listed |

## Tests

`tests/apd_01` … `apd_22` drive each rule with button presses, or set up a
moment with cheats and then play it. The demo player (`apd_bot.c`) plans
like a player who knows the map: from what it holds (mods, pass level,
ciphers, gates opened) it works out which rooms each way of fitting its mods
reaches, picks the nearest thing worth doing (an upgrade, a pass terminal, a
room not seen, a boss once nothing else is near), and goes to a cradle to
refit first when that needs it. Inside a room it follows a distance field
toward its goal, jumping and flipping to steer up and down, and before each
move runs the real game ahead on a copy to see if that move gets hurt; if
so, or if it makes no headway, it tries two dozen button plans on copies
and keeps the safest that gets furthest. Gates get short scripted moves of
their own (burning barrels, dropping on cracked concrete, shooting a target
through a wall, the hidden room's two buttons with a shadow). `apd_23`
plays from the title to the end of the credits with every room and upgrade:
the cherry.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Vainger" (raw page): the premise, shields,
  clones, gene pods, the map legend, mods and stabilizers, clearance, the
  boss rule, endings. https://ufo50.miraheze.org/wiki/Vainger
- [MH-DEBUG] UFO 50 Wiki, "Debug mode": the sectors, Ramses, Y-Vainger, four
  key codes, the final boss's phases. https://ufo50.miraheze.org/wiki/Debug_mode
- [MH-CHEATS] UFO 50 Wiki, "Cheats": the all-upgrades code (no saving).
  https://ufo50.miraheze.org/wiki/Cheats
- [MH-MP] UFO 50 Wiki, "Multiplayer" (not listed).
  https://ufo50.miraheze.org/wiki/Multiplayer
- [MANUALS] Steam guide "The missing manuals - How to play UFO 50 games",
  section 29: controls, clones, pods, the map and mod screens.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [GGC] Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [G-MAP] Steam guide "Vainger - 100% with Full Map" (text and comments
  only): the start at C15, the counts, the six obscured upgrades, 98 % before
  the last fight, the credits rule.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3341323146
- [STEAM-BOSS] Steam thread "Vainger final boss is a bit annoying".
  https://steamcommunity.com/app/1147860/discussions/0/4638240577738098691/
- [STEAM-CHERRY] Steam thread "Vainger Cherry Disk": the hidden cells,
  map and item percentages. https://steamcommunity.com/app/1147860/discussions/0/4849903793441796774/
- [STEAM-HELP] Steam thread "Vainger help": the hidden room, its buttons,
  "the first miniboss, below the armory".
  https://steamcommunity.com/app/1147860/discussions/0/4849904345520088230/
- [STEAM-ITEMS] Steam thread "Vainger - What's the full set of items?".
  https://steamcommunity.com/app/1147860/discussions/0/4849904176637228175/
- [STEAM-SECTOR] Steam thread "Vainger sector question": the final entrance
  "requires the boss codes", "middle up of Theta".
  https://steamcommunity.com/app/1147860/discussions/0/4849903998513517178/
- [STEAM-BUG] Steam thread "Vainger boss bug": the first boss, a big robot
  that moves the terrain. https://steamcommunity.com/app/1147860/discussions/1/523083364935237011/
- [TVT] TV Tropes recap (from the research notes).
  https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game29Vainger
- [TVT-YMMV] TV Tropes YMMV page (from the research notes).
  https://tvtropes.org/pmwiki/pmwiki.php/YMMV/UFO50Game29Vainger
- [LIZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 29".
  https://lizstar64.github.io/reviews/2024/10/16/UFO50-29.html
- [POPCAR] Popcar's Blog, "Reviewing Every Single UFO 50 Game".
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [SR] speedrun.com, "29 - Vainger in 29:38.300".
  https://www.speedrun.com/UFO_50/runs/zgjxx8vz
