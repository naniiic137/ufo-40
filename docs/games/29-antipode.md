# 29 · ANTIPODE

*Internal design document. Not shown in the product.*

## Tribute to

**Vainger** (UFO 50 game #29, Mossmouth, "March 1987"). The rules were
researched from text only: the community wiki's raw pages (Vainger, Debug
mode, Cheats, Multiplayer), the Steam guides "The missing manuals" (section
29), "Gift, Gold & Cherry" and "Vainger - 100% with Full Map" (its text and
comments only), Steam threads, written reviews, TV Tropes and speedrun.com's
run times (listed under Sources; the full notes are in the research folder,
`29-vainger.md`, and the independent review is `ufo40-review/29-review.md`).
No UFO 50 images, video, sprites, music, maps or text were used as
references (the 100% guide's maps were not opened), and nothing was taken
from a UFO 50 install.

**Map type: fixed.** Vainger's station is hand-made and the same every run:
"everything is fixed (map, items, bosses)" [MH]. Ours is fixed too, and all
our own: four sector maps and a hand-drawn tile layout for every one of the
98 rooms, kept in `tools/antipode/station.txt` and written to
`apd_rooms.c` by `tools/antipode/make_station.py` (which also checks that
every doorway and every thing in a room can be reached). The rooms read both
ways, floor and ceiling: ceiling-only routes, gaps that need a flip back in
mid-air, spike-ball corridors, mazes of one-way plates, spike runs, switchback
shafts and doorways at every height. Only the structure follows the
original:

| Full scale | Vainger | ANTIPODE |
|---|---|---|
| Players | 1, no 2-player mode [MH], [MH-MP] | 1 |
| Sectors | Theta (start, hub, the locked final teleporter in the north), Verde, Latom, and Central Control behind four keycodes [MH], [MH-DEBUG] | the Gantry (start, hub, the locked uplink up north), the Brine, the Coil, and the Crown behind four ciphers |
| Teleporters | Theta to Verde lower left and lower right, Theta to Latom, the final one [MH], [STEAM-HELP] | the same four links |
| Rooms | not counted in any source; the 100% guide's grid reaches at least 21 rooms a sector (A21) and the start is C15 [G-MAP]; "cover the entire station in only a few minutes" [LIZ] | 98: 33 in the Gantry, 29 in the Brine, 29 in the Coil, 7 in the Crown; 1 to 4 screens each |
| Upgrades | 25 shield upgrades (+5 each, 75 to 200), 3 clones (2 to 5), 2 stabilizers, 4 mods [MH], [G-MAP] | 25 armor cells (75 to 200), 3 spare bodies (2 to 5), 2 dampers, 4 mods: 34 |
| Where the mods and stabilizers are | Heat Mod a few rooms east of the start; Force Mod west Verde; Multi Mod several rooms east of Latom's teleporter; Pulse Mod north-east Latom; stabilizers far north-east of Verde and of Latom [MH] | EMBER four rooms east of the start; MASS in the west Brine; TWIN past the Lamprey in the south Coil; VOLT in the north-east Coil; dampers in the far north-east of the Brine and of the Coil |
| Spare bodies | central and north-west Theta, central Latom [MH] | under the cradle bay's cracked floor (the Gantry), in the quiet vault (the Brine), behind the sentry orbs' nano-wall (the Coil) |
| Hidden upgrades | about six, "obscured by objects, terrain, or enemies" [G-MAP], [STEAM-CHERRY], [STEAM-ITEMS] | six, each with its own kind of cover: behind pipes over a one-way plate (the Gantry's long east hall), behind pipes in the corner of a Brine hall, sealed in a block of cracked concrete (the Gantry), sealed in cracked concrete in the Crown's gallery, in a Brine acid pool, and inside the Coil's kiln (heat) |
| The hidden room | one, behind burnable walls, opened by two buttons hit at once [STEAM-HELP] | the quiet vault in the Brine, under a cracked floor; a grey wall slides away when the pad in its floor and the pad in its ceiling are pressed at once |
| Clearance | 3 levels, terminals in south Verde (1), central Verde (2), central Latom (3) [MH] | 3 pass levels, terminals in the south Brine, the middle of the Brine, the middle of the Coil |
| Bosses | "the first miniboss", Ramses and Y-Vainger, the final boss in 3 phases whose first is a big walker, "the first boss of the Central area" that moves the terrain; four keycodes, "boss codes" [STEAM-HELP], [STEAM-BUG], [MH-DEBUG], [STEAM-SECTOR], [TVT] | 7 fights: the Scrapper (below the armory), the Warden (cipher 1), Brackish (cipher 2), the Lamprey (a second minor one), the Coilheart (cipher 3), Obverse (cipher 4), Halloran in three phases, his walker raising columns of the floor |
| Saves | Gene Pods: walking by saves and refills shields and clones; inside, the map and the mods [MH], [MANUALS] | 10 cradles, the same |
| Goals | 3 [MH], [GGC] | the same 3 |
| Stats | Upgrades, Exploration, Times Died [MH] | the same three, on the title |

**How long a run takes.** After the review the rooms were drawn by hand one
by one, so the station takes about as long to cross as the original's. The
demo player looks ahead on copies of the game and knows the map, so it
should come in clearly under the best human runs:

| Run | Time |
|---|---|
| The demo player: every upgrade | 17:18 of play (62,304 frames) |
| The demo player: 100 %, Halloran beaten | 18:45 (67,481 frames) |
| The original's best Cherry run (speedrun.com) | 24:25; then 24:57, 30:46, 31:58, and a first run of 73:27 [SR-CHERRY] |
| The original at 100 %, by reviewers | "an hour and change" [LIZ], "a couple hours" [POPCAR] |

## The structure

The suit wakes in the cradle bay in the middle of the **Gantry**. The
Concourse to the north holds a cradle and, above it, the uplink to the
Crown, open from the start but needing all four ciphers. **EMBER** (the
gift) is four rooms east of the start. With SCORCH the orange barrels burn:
the way west to the armory, with the Scrapper below it and the west
teleporter beyond; the way to the Warden's hall. A spare body lies under the
cradle bay's cracked floor (ANVIL). The east teleporter is in the south-east;
the Coil's teleporter is under a pass-2 hatch.

The **Brine** (both Gantry teleporters land there, at the west and east ends
of its south corridor) holds pass 1 by the west landing and pass 2 in the
middle, the **MASS** mod in the west behind a pass-1 door, Brackish in the
settling tank, a spike hall, and along the north a row of rooms behind
electric fields: a cradle, a damper in the far north-east, and Obverse's
mirror room. The quiet vault, with a spare body, is under a cracked floor in
the south-west. Acid lies along its floors.

The **Coil** (pass 2 from the Gantry): the Lamprey guards the **TWIN** mod
in the south; pass 3 is in the middle; a nano-wall of three sentry orbs
closes the spare body; the Coilheart sits in the dynamo; the **VOLT** mod is
behind a pass-3 door north-east, and the damper beyond a shutter whose target
sits inside the wall (PHASER). A spike hall and a kiln (heat) need IRONSOLE
and KILNSKIN.

With all four ciphers the uplink carries the suit to **the Crown**: seven
rooms, a cradle, then an electric field, barrels, cracked concrete, acid and
a heat room on two ways down to Halloran, and a cell sealed in concrete in
the gallery.

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
| Harm | machines 5-15 on contact, their shots 6-12; spikes 100 (they kill at the start); 60 frames safe after a hit, enough to slip through spikes or a field | [TVT], [TVT-YMMV], [POPCAR] | apd_09 |
| Acid and heat | acid takes 5 every 20 frames; a heat room 1 every 6 frames without KILNSKIN | [MH], [TVT] | apd_09 |
| Electric fields | between two emitters, across a doorway; 30 and a push back; the STORMBALL carries the suit through | [MH], [TVT] | apd_09 |
| Gravity balls | red ones are live while gravity points down, purple while it points up | [TVT] | apd_10 |
| One-way plates | solid from the side your feet are on, red arrows | [TVT] | apd_10 |
| Spare bodies | 2, +1 a pick-up, 5 at most; at 0 armor one takes over at the door you came in by, full armor, the room as you left it (kills and broken walls stay) | [MH], [MANUALS], [TVT] | apd_11 |
| Game over | no spare left: back to the last cradle, everything since lost | [MH] | apd_11 |
| Cradles | walking by: saved, armor and spares full; UP: the MODS and MAP pages, the only place to see the map or move mods; B leaves | [MH], [MANUALS] | apd_12 |
| The map | rooms seen, the current one flashing, yellow doors, thick pass doors (flashing once the pass is held), S cradles, E rooms with something left, T teleporters, dots for terminals; UP and DOWN page through the three sectors (the Crown's page only in the Crown's own cradle) | [MH], [MANUALS] | apd_12 |
| The cradle's numbers | MAP % and ITEMS % beside the suit | [MANUALS] (undocumented numbers), [STEAM-CHERRY] (two percentages) | apd_12 |
| Mods | 4 mods, each always fitted in FLIP, FRAME or GUN; several can share a system and add up; a new one goes into the first empty system (GUN, FRAME, FLIP), else the GUN | [MH], [TVT], [MANUALS] | apd_05-07, apd_15 |
| EMBER | FLARE (a heat wave both ways on a flip), KILNSKIN (half harm, heat-proof, spikes 50), SCORCH (double damage, burns barrels) | [MH], [TVT] | apd_05-07 |
| TWIN | AFTERIMAGE (a copy where you flipped fires when you fire), SATELLITE (a drone that fires with you and latches on to foes), FANSHOT (two more shots at 45°) | [MH], [TVT] | apd_05-07 |
| VOLT | STORMBALL (the first flip of a jump wraps you in a ball that does 50 and passes fields; it holds through up to three flips until you land, a fourth flip puts it out), SIPHON (harm done gives armor back), PHASER (shots through walls) | [MH], [TVT] | apd_05-07, apd_09 |
| MASS | ANVIL (the flip becomes a heavy drop: 50, breaks cracked concrete), IRONSOLE (spikes 10; 5 with KILNSKIN), REACH (double range) | [MH], [TVT-YMMV] | apd_05-07 |
| Overload | one mod a system is safe, +1 a damper; beyond: FLIP costs 2 a flip, GUN 1 a shot, FRAME takes more harm; with four mods and no damper something always overloads | [MH], [TVT] | apd_08, apd_15 |
| Doors | yellow ones open at a touch; pass doors with the pass; boss doors shut until the fight ends | [MH] | apd_13, apd_16 |
| Gates | barrels (SCORCH; the blast hurts foes, not you), cracked concrete (ANVIL), fields (STORMBALL), targets in walls (PHASER), nano-walls (the sentry orbs), heat (KILNSKIN), spike halls (IRONSOLE) | [MH], [TVT] | apd_05, apd_06, apd_09, apd_14 |
| Bosses | a bar in the top corner; no leaving once it starts; a spare body finds it with only 250 back; red when low; a chain of blasts | [MH], [TVT] | apd_16, apd_17 |
| Ciphers | from the Warden, Brackish, the Coilheart and Obverse; all four open the uplink | [MH], [STEAM-SECTOR] | apd_13, apd_16 |
| Obverse | the other suit: flips to your side, sword beams, up to five in a row once she is low | [TVT] | apd_17 |
| Halloran | 1: a walker whose legs can always be shot, its hand fires three volleys then it rams; it stamps and columns of the floor rise for three seconds; now and then it turns shots back for a while. 2: the heart flies about dropping orbs and pink rings. 3: the head; shoot the neck's joints to bring it down; it shuts itself away calling lightning that deletes shots, 20 to 30 s at a time with calms of 6 to 14 s, each length drawn at random; pink rings and orbs | [TVT], [STEAM-BOSS], [STEAM-BUG] | apd_17 |
| Completion | rooms seen and upgrades found; the Crown's hall counts once Halloran is beaten, so everything before the last fight is 98 % | [G-MAP] | apd_24 |
| Endings | below 100 %: the visor stays dark; at 100 %: the pilot is shown | [MH], [G-MAP] | apd_19 |
| Goals | gift: get a mod; gold: beat the game; cherry: beat it with 100 % rooms and upgrades, and see the credits through | [MH], [GGC], [G-MAP] | apd_19 |
| The code | a code (LOAD-KITS) gives every upgrade, with no saving and no goals; scratched under the first terminal, seen coming at it from the left | [MH-CHEATS], [LIZ] | apd_21 |
| The hidden room | one hidden room with a spare body, opened by two switches pressed at once (ours: a pad in the floor and one in the ceiling over it, the suit on one and its AFTERIMAGE left on the other, in the other gravity) | [STEAM-HELP] | apd_14 |
| Stats | Upgrades, Exploration, Times Died | [MH] | apd_20 |
| Playable start to end | the demo player, with real presses from the title, takes every room and upgrade and beats Halloran: cherry, in 18:45 of play | [SR-CHERRY] | apd_23 |

### Readings we had to choose

- **Gravity and the d-pad:** "up" and "down" go with the suit: DOWN always
  crouches (toward the feet), UP aims over the head, whichever way up it is.
- **A flip** stops the fall where it is and starts the new one from rest.
- **STORMBALL** is counted in flips, not timed: "lasting up to three flips"
  [MH]. The ball comes with the first flip of a jump and holds until the
  suit lands, through the second and third flips; a fourth flip in the same
  jump puts it out.
- **Electric fields** stand across doorways between two emitters. One across
  a side doorway is four tiles tall (a plain doorway is two), so the suit can
  jump into it and flip and be carried across in the air by the ball; one in
  a floor or a ceiling is two tiles wide and is crossed by falling through
  it. On foot, a field throws the suit back, and the mercy frames after the
  hit can carry it through (as the original's players do with
  electricity [TVT-YMMV]).
- **Numbers no source gives:** walk 1.5 px a frame; jump about 58 px high;
  gravity 0.25 px a frame per frame, falls up to 4.5; a shot every 8 frames,
  5 damage, 5 px a frame for 22 frames (REACH: 44); ANVIL drops at 8 px a
  frame; AFTERIMAGE 4 s; FLARE 15 a wave; SATELLITE shots 3, latched 1 every
  8 frames; SIPHON 1 armor for every 10 harm; an overloaded FRAME takes half
  again; acid 5 every 20 frames; heat 1 every 6; electric fields 30; mercy 60
  frames.
- **Where a new mod goes:** the manual names only the three systems, and "a
  module can only be attached to one at a time" [MANUALS], so there is no
  unfitted row: a new mod goes into the first empty system (GUN, then FRAME,
  then FLIP), and with all three taken into the GUN, which then overloads
  until a damper is found.
- **An armor cell** fills the armor to its new top (no source says).
- **Foe and boss hit points:** treadles 15, 40 and 80; spitter 25;
  star-eye 60; hivebox 80; mite 5; skipjack 25; skimmer 15; sentry orb 30;
  the Scrapper 300, the Warden 600, Brackish 700, the Lamprey 400, the
  Coilheart 700 (its nodes 60), Obverse 800, Halloran 700, 600 and 800.
- **The boss list:** the sources name only Ramses, Y-Vainger, "the first
  miniboss", the "first boss of the Central area" (a big robot that moves the
  terrain) and the three-phase final boss. We made two minor fights, four
  cipher holders and the final; Brackish stands where Ramses stands (its
  behaviour is ours). The terrain move belongs to the first fight of the last
  sector, which we read as the final boss's walker, so Halloran's walker
  raises columns of the floor. The Warden's move is ours: it crosses between
  floor and ceiling, and its landing sends a shock both ways along that
  surface. Halloran's middle phase is ours (sources describe only the first
  and the last).
- **The Crown's storm** comes "at complete random" [STEAM-BOSS]: each storm
  lasts 20 to 30 s and each calm 6 to 14 s, drawn from a seed taken when the
  last phase starts.
- **The map** pages through the three sectors, as the manual says ("the maps
  of the 3 sectors") [MANUALS]; the Crown's page shows only in the Crown's
  own cradle.
- **The Crown's size:** no source gives it; ours is seven rooms, a short last
  run that needs the mods.
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
- **The world:** the Gantry, the Brine, the Coil and the Crown: four maps and
  98 rooms, every one drawn by hand; where every upgrade lies, the six hidden
  spots and the quiet vault with its two pads.
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
| In a cradle | LEFT / RIGHT pick a mod (RIGHT past the last: the map), UP / DOWN move it between FLIP, FRAME and GUN; on the map UP / DOWN page through the sectors; B leaves |
| START | pause |

## Not confirmed

- The fire and jump buttons: B fires and A jumps in the original [MANUALS];
  A fires here (the owner's layout, swappable).
- Leaving a cradle: the original uses its fire button (B there, A here with
  the swap); here B, the menus' back button.
- Crouch and aim upside down: relative to the suit (DOWN crouches whichever
  way up).
- Where a new mod is fitted: the first empty system, else the GUN.
- The cradle's numbers: MAP % and ITEMS %.
- How tall an electric field stands, and how the STORMBALL counts its three
  flips.
- The Warden's move, the terrain move on Halloran's walker, and the storm's
  lengths.
- The Crown's size (seven rooms).
- Every number in "Readings we had to choose".

## Tests

`tests/apd_01` … `apd_22` and `apd_24` drive each rule with button presses,
or set up a moment with cheats and then play it (`apd_09` crosses the
Brine's tall field with the STORMBALL; `apd_14` opens the quiet vault with
the suit and its shadow; `apd_17` plays each fight, Halloran's pistons and
the random storm). `apd_22` checks the station: the room counts, that every
room's things find a place and that every room and upgrade can be reached.
The demo player (`apd_bot.c`) plans like a player who knows the map: from
what it holds (mods, pass level, ciphers, gates opened) it works out which
rooms each way of fitting its mods reaches, picks the nearest thing worth
doing (an upgrade, a pass terminal, a room not seen, a boss once nothing else
is near), and goes to a cradle to refit first when that needs it. Inside a
room it follows a distance field toward its goal, jumping and flipping to
steer up and down, and before each move runs the real game ahead on a copy to
see if that move gets hurt; if so, or if it makes no headway, it tries two
dozen button plans on copies and keeps the safest that gets furthest. Gates
get short scripted moves of their own (burning barrels, dropping on cracked
concrete, shooting a target through a wall, the vault's two pads with a
shadow). `apd_23` plays from the title to the end of the credits with every
room and upgrade, the cherry, inside 22 minutes of play (it takes 18:45).

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
- [STEAM-BUG] Steam thread "Vainger boss bug": "the first boss of the
  Central area", a big robot that moves the terrain. https://steamcommunity.com/app/1147860/discussions/1/523083364935237011/
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
- [SR-CHERRY] speedrun.com, UFO 50, category "29 - Vainger", Cherry runs
  (read through its API for the review): 24:25, 24:57, 30:46, 31:58, 73:27.
  https://www.speedrun.com/UFO_50
