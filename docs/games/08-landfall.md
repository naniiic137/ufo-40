# 08 · LANDFALL

*Internal design document. Not shown in the product.*

## Tribute to

**Planet Zoldath** (UFO 50 game #8, Mossmouth, "May 1984"). The rules were
researched from text only: the community wiki's raw pages, a community
guide to the game's generation rules linked from the wiki, Steam guides and
threads, and written reviews (listed under Sources; the full notes are in
the research folder, `08-planet-zoldath.md`). No UFO 50 images, video,
sprites, music, room layouts or text were used as references, and nothing
was taken from a UFO 50 install.

**Map type: random.** Planet Zoldath builds a new planet for every run, so
LANDFALL does too (`lfl_gen.c`). The generator follows the community's
written generation rules for the original [GDOC]; the room layouts it
places are all ours (14 field rooms, 4 hidden-wall rooms, 3 villages, the
ship's clearing, 6 dungeon rooms, each of them mirrored at random).

| Full scale | Planet Zoldath | LANDFALL |
|---|---|---|
| Planet | a grid of flip-screen rooms, 4 × 7 or 5 × 6, about 5 cells of solid stone [GDOC] | 7 × 4 or 6 × 5 (about half each), 4 to 6 cells of stone, the rest all joined |
| Side rooms | outside the grid, reached only through gates on its edge, never next to each other [GDOC] | 4, one per gate, never side by side |
| Real gates | exactly 4: to a map part, the dungeon, an item, 2 of the rare resource [GDOC] | the same 4, four different kinds; extra blue blocks and big shells as dressing |
| Dungeon | a separate line of dense rooms, entrance on the overworld (usually gated), may hold more gates, a map part at the end [GDOC], [MH] | 6 rooms in a line, 0 to 2 gates inside, the third chart piece at the end |
| Villages | 6 or 7; 3 need the Translator (with 6); the 7th gives the secret's hint [GDOC] | 6 (three in glyphs) or 7 (the seventh, plain, keeps the secret) |
| Trades | 6: 3 resource for resource, 2 resource for an item, 1 resource for a map part [GDOC] | the same 6 |
| Tools | 8, carry 2: 5 lying about, 1 behind a gate, 2 sold [GDOC], [MANUALS] | the same 8 jobs, 5 / 1 / 2 |
| Energy cubes | 4 per planet, rarely 5; start 3, max 7 [MH], [GDOC] | 4 (5 about one planet in eight), start 3, max 7 |
| Map parts | 3: one gated, one sold, one at the dungeon's end [GDOC] | 3 chart pieces, the same |
| Resources | 4, a rarity tier each per run, 63 at most [MANUALS], [GDOC] | beads, burrs, ferns and spores, tiers rolled per planet, 63 at most |
| Creatures | 2 friendly species (one speaks glyphs), 4 hostile, plus the snail; a mosquito; morphed chicks [GDOC], [MH] | the same: 10 looks to draw from, the big shell, the gnat, hatchlings |
| Secret | one hinted hidden wall on certain room looks [GDOC] | one per planet, on one of 4 looks, 5 possible keys, 4 of the rare resource behind it |
| Goals | 3 [MH], [GGC] | the same 3 |
| Stats | Most Items Found, Most Energy Found [MH] | most tools found, most energy found (on the title) |
| A run | 15–20 minutes informed, about an hour the first time [G-TIPS], [STEAM-PS] | the demo player takes 6–10 minutes on the planets it wins |

## Mechanics checklist

| Mechanic | How LANDFALL does it | Source | Test |
|---|---|---|---|
| A new planet | the title lands Wick on a new planet every run; a death ends the run and the next one is a new planet; no saving mid-run, no continues | [MH] | lfl_01 |
| The goal | no goal is told: the HUD's three empty chart slots and the clock are the only hints; touch the ship with all three to lift off | [MH], [MANUALS] | lfl_20 |
| Walking | four ways only, the newest press wins; 1.25 px a frame | [MANUALS] | lfl_02 |
| Corner slide | walking into the side of a block eases her round its corner | [GDOC] | lfl_02 |
| Flip-screen rooms | off an open edge the screen scrolls to the next room (Zelda 1 style); the edge tiles of the rooms beside show; she can't be hurt during the scroll | [LIZ], [GDOC] | lfl_02 |
| Use the tool in hand | the main action; held, it keeps her facing (strafe) | [MANUALS], [GDOC] | lfl_02, lfl_05 |
| Talk, else swap | a tap beside a local talks or trades; anywhere else it swaps hands; held, the tool in hand is set down | [MANUALS], [GDOC] | lfl_04, lfl_17 |
| Two hands | walking onto a third tool puts the one in hand down where she stands; a tool just set down waits until she steps off it (and a moment more) | [MANUALS], [GDOC] | lfl_04 |
| Short of a resource | a buzz, and that resource flashes on the HUD | [MANUALS] | lfl_08, lfl_09, lfl_13 |
| Energy | start 3; a cell adds one and fills her up, to 7; a blast destroys a cell for good | [MH], [MANUALS], [GDOC] | lfl_08, lfl_11 |
| Hurt | contact with hostile creatures (each species its own damage); the big shell 3, a creature's death blast 3, her own pod 2, crystal shards 5; a second of safety after a hit | [GDOC], [G-CHERRY] | lfl_08, lfl_11, lfl_12, lfl_13 |
| Healing | a chop heals 3 (not eaten when she is well) | [GDOC] | lfl_11 |
| Bog | 1 a step, then the second of safety, so short stretches can be crossed; the bog boots carried in either hand make it harmless | [MH], [GDOC], [STATIC] | lfl_10 |
| Respawn | creatures and what was taken come back three rooms away (steps across the grid) or on going down or up the dungeon stairs; walking in on a creature's spot means it isn't there; what is opened stays open | [MH], [GDOC] | lfl_16 |
| The clock | runs in play and during the scroll; stops while words are up and while paused | [MANUALS], [GDOC] | lfl_10, lfl_17, lfl_21 |
| Disc (Boomerang) | free; out half the screen and back, 2 a hit both ways, one in the air; grabs a pickup (even across pits) and brings it home; flies through a hollow log; something grabbed is delivered at once if she leaves the room | [MH], [G-TIPS], [GDOC] | lfl_06 |
| Rivet gun (Nail Gun) | free; rapid (every 8 frames, 3 in the air), short (72 px), 2 a hit; breaks crystal safely; a crystal breaking beside a local turns it on her | [G-TIPS], [GDOC] | lfl_05 |
| Hatcher (Metamorpher) | free; a ricocheting shot for 1; the last hit makes a harmless hatchling for 6 s; a hatchling knocked out drops the second-rarest resource; a ricochet into Wick shrinks her for 6 s (through holes; a second of safety when she shrinks and grows); crystal turns to flesh for a while | [MH], [G-TIPS], [GDOC] | lfl_07, lfl_08 |
| Blast pods (Bombs) | one resource a pod (rolled per planet); off after 1.5 s; foliage and flesh go, crystal goes with shards, blue blocks only in the dungeon, never stone, brick or rock; 2 to Wick, 4 to creatures | [MH], [G-TIPS], [GDOC] | lfl_08 |
| Thumper (Pulse Generator) | one resource a thump; every hostile creature in the room knocked out, the big shell left an empty shell, blue blocks sunk | [MH], [GDOC] | lfl_09 |
| Bog boots (Hazard Boots) | the bog can't hurt her; they stomp hatchlings | [MH], [G-TIPS] | lfl_07, lfl_10 |
| Chop press (Meat Printer) | one resource a chop, laid in front of her; a chop draws the big shell (and any creature that moves) | [MH], [GDOC] | lfl_13 |
| Talk box (Translator) | carried, the glyph speakers read plainly | [MH], [MANUALS] | lfl_17 |
| Talk words | A on the boots or the talk box only shows what they do | [MANUALS], [GDOC] | lfl_10 |
| Three tools burn resources | pods, thumper and chop press each a different resource, never the common one, rolled per planet | [MH], [GDOC] | lfl_03 |
| Gates | bog: boots; a pit with the prize across it: the disc; a hole in a wall or log: small; crystal: rivets (or a blast and its shards); foliage: a blast; blue blocks: a thump; the big shell: a chop (or push past it for 3, or a thump and a small Wick past the shell) | [MH], [GDOC], [G-TIPS] | lfl_05 to lfl_10, lfl_13, lfl_16 |
| The big shell (snail) | stands in the mouth of the way into a side room; can't be hurt; 3 to touch; drawn off by a chop, then the way stays open; a thumped shell is a wall a small Wick slips past only when it lies along the way, and comes back with its room | [MH], [GDOC], [STEAM-Q] | lfl_13 |
| Locals | stand still, always face down; hurt one and it turns on her; knocked out they may drop the rare resource (only they do), never a chop; they come back with the room | [MH], [MANUALS], [GDOC], [G-TIPS] | lfl_12, lfl_17 |
| Hostile species | each rolls how it moves (stands, wanders, walks at her), shoots (not, four ways, straight at her), HP (1–5), damage (1–3), death (nothing, a blast of 3, bog left behind) and drop; three drop the three plain resources, one drops chops; they face her | [MH], [GDOC], [G-CHERRY], [G-TIPS] | lfl_03, lfl_12 |
| Gnats (mosquito) | a chop on bog cooks; 5 s later a gnat flies in for it, one at a time; knocked out it drops 2 of the rare resource | [GDOC], [G-TIPS] | lfl_14 |
| Bog warps | a blast on bog links a bog tile in it with another anywhere on the planet (even the dungeon); one pair at a time | [GDOC] | lfl_15 |
| Trades | the giver stands on a fixed tile of each village layout; a tool costs 32 / 16 / 8 / 4 by rarity, a chart piece 32 / 16 / 8 (never the common one), the three ask different resources; resource trades give 32 >> tier for 16 >> tier; a tool or chart piece is sold once | [GDOC] | lfl_03, lfl_17 |
| Glyphs | half the villages speak in glyphs (a simple letter cipher, readable by hand); one glyph speaker stays unreadable even with the talk box | [MH], [GDOC] | lfl_17 |
| The hidden wall | the seventh village's giver says where (by the room's look) and how; its key is the thumper, the disc (from the side named), a pod, rivets or being small; 4 of the rare resource behind it | [GDOC] | lfl_18 |
| The dungeon | stairs in a gated side room; six dense rooms in a line full of foes, with their own gates; the third chart piece at the end; stairs work once stepped off and on | [GDOC], [MH] | lfl_19 |
| Every planet winnable | a progression check: every tool can be had (none sold only in glyphs out of reach), both chart gates and the dungeon's gates can be opened; a planet that fails is built again | [GDOC], [STEAM-IMP] | lfl_03 |
| Goals | Beacon: find one chart piece; Saucer: leave with all three; Alien: leave in under 20 minutes on the clock | [MH], [GGC] | lfl_20 |
| Records | most tools and most energy found; runs, wins, deaths and the best time are saved | [MH] | lfl_11, lfl_20 |
| The Skywell joke | with 31 of every resource, the ship has a line about SKYWELL (the original's is about Velgress, our slot 07) | [MH], [GDOC] | lfl_20 |
| One player | no 2-player mode | [MH], [MH-MP] | — |

### Readings we had to choose

- **Snail gates stand in the passage mouth.** The big shell that guards a
  side room stands in the grid room, two tiles square, in the mouth of the
  way through the planet's edge wall, where a chop can draw it aside (the
  side room's own passage is too narrow to draw it out of).
- **The live big shell is no wall.** The sources list damage-boosting (3)
  and the Metamorpher's moment of safety as ways past it, so Wick can push
  through it at the cost of a hit. The empty shell the thumper leaves is a
  wall; only a small Wick passes it, and only when it lies along the way
  (it keeps the facing it had when thumped, so the side she thumps it from
  matters). It comes back when its room does.
- **Speeds and times:** Wick walks 1.25 px a frame; a second of safety after
  a hit and on shrinking or growing; small for 6 s; a hatchling for 6 s; a
  pod's fuse 1.5 s; shards 0.4 s after the blast; a gnat 5 s after a chop
  cooks; the screen scroll 2/3 s; holding B half a second sets a tool down.
- **Weapons:** the rivet reaches 72 px (about a quarter of the room), the
  disc 144 px (half the room's width), the hatcher's shot bounces four times.
  Creatures take 4 from a blast.
- **Creature rolls:** HP 1–5, damage 1–3, death: nothing (half), a blast or
  bog (a quarter each); shooters fire every 1.5–2.5 s, a shot slower than
  her walk. Each field room holds 0–3 creatures of one or two species.
  Locals have 3 HP and drop the rare resource half the time.
- **Resources on the ground:** lying about mostly the common one (6 : 3 : 1
  common, less, more uncommon), in nooks the uncommon ones (1 : 3 : 3), in
  the dungeon (1 : 2 : 3). The rare one only from gates, the hidden wall,
  locals and gnats.
- **Resource trades** give 32 >> tier of what you get for 16 >> tier of what
  you pay (so 2 rare buy 32 common).
- **The dungeon:** six rooms; no gate inside one time in five, one in two in
  five, two in two in five, of any kind but a pit. The dungeon's own gate on
  the overworld is never a pit (nothing can fetch a staircase).
- **Hazard boots** work from either hand ("equipped/held" in the sources).
- **Blasts** destroy energy cells, not tools or chart pieces.
- **The cooldown on a dropped tool** counts down only while Wick is off it,
  so standing still on it never swaps her hands back and forth.
- **Crystals breaking beside a local** (within two tiles), by rivet or
  blast, turn it hostile.
- **Glyphs** are our own mirrored-letter cipher, readable by hand like the
  original's.
- **The snail's lure works every time** (the original's buggy lure isn't
  copied).
- **Title:** the original has no title menu described; ours is a title
  screen with the records, A to land.

## What is ours

- **Name:** LANDFALL (1984, Beamdown Softworks).
- **Hero:** Wick, in the small round ship (the same pilot as HOMESPUN, our
  slot 44, as Pilot is in the original's Pilot Quest).
- **The planet:** Yombra, its colours rolled per run, and every room layout:
  the field rooms, the hidden-wall rooms, the villages and their huts, the
  ship's clearing and the dungeon rooms.
- **Creatures:** ten looks (puffwort, shellback, stalkeye, drifter,
  squatter, pincher, spiner, bobber, burrower, clanker), the big shell, the
  gnat and the hatchlings.
- **Tools and resources:** disc, rivet gun, hatcher, blast pods, thumper,
  bog boots, chop press, talk box; beads, burrs, ferns and spores; chops.
- **Music:** "Landfall" (title), "Touchdown" (landing), "Strange Ground"
  (the planet), "Under Yombra" (the dungeon), "A Piece of the Sky" (a chart
  piece), "Liftoff", "The Way Home" (ending), "Stranded" (game over).
- **Words:** every local's line, the offers, the hints, the glyphs, the
  ending, the goal lines and every label.

## Owner's changes

- **The main action on A.** The original uses the item with Button 1 (B)
  and talks or swaps with Button 2 (A) [MANUALS]. Here the tool in hand is
  on A (keyboard Z / Space, Vita Cross) and talk / swap / set down is on B;
  the cartridge card's CONTROLS page can swap them back.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
UFO 40 goals, which are the original's own (gift, gold, cherry). The
original's Terminal codes (less harm from hazards, choosing the seed) and
its debug map are UFO 50 collection features; UFO 40 has no terminal, so
they are left out.

## Controls

| Input | Action |
|---|---|
| D-pad | walk four ways |
| A | use the tool in hand |
| Hold A | keep facing (strafe) |
| B (tap) | talk or trade beside a local; else swap hands |
| Hold B | set the tool in hand down |
| B / A while talking | take the trade / no thanks |
| START | pause (the clock stops) |
| Title | A lands on a new planet, B back to the library |

## Not confirmed

- Every speed, time and range above, the creature rolls' exact ranges, and
  how many resources lie about.
- Whether a live snail can really be walked through for a hit (we read the
  sources' "damage-boost (3)" that way).
- Whether the boots work from the inactive hand.
- What exactly makes a local turn hostile when crystals break.
- How the dungeon's inner gates are chosen and how long the original's is.
- Whether a blast destroys tools or chart pieces (ours: only cells).

## Tests

`tests/lfl_01` … `lfl_26` drive the rules with button presses, or set up a
moment with cheats and then play it: the title and a lost run (01),
walking (02), the planet rules checked on 800 planets (03), the hands (04),
each tool (05–10), energy (11), creatures (12), the big shell (13), gnats
(14), bog warps (15), respawn (16), trades and glyphs (17), the hidden wall
with each of its keys (18), the dungeon (19), the ship, goals and save
(20), the clock and pause (21), and the art and words (22).

The demo player (`lfl_bot.c`) plays with real button presses, knowing the
planet the way a player with a hand-drawn map does: it walks by a shortest
path search, juggles its two hands, fights what gets in line, trades,
opens gates and goes down the dungeon. From the title it wins three
planets in 405–495 s, losing a run or two on two of them first (`lfl_23`
… `lfl_25`), and four more planets start to finish when it can't be hurt
(`lfl_26`). It doesn't win every planet (it can still get its tools in a
tangle); every planet is winnable by the generator's progression check
(`lfl_03`).

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Planet Zoldath" (raw page): the infobox,
  gameplay, items, the strategy rules, cheats, meta messages and goals.
  https://ufo50.miraheze.org/wiki/Planet_Zoldath
- [MH-DEBUG] Miraheze, "Debug mode": the debug map.
  https://ufo50.miraheze.org/wiki/Debug_mode
- [MH-MP] Miraheze, "Multiplayer": not listed.
  https://ufo50.miraheze.org/wiki/Multiplayer
- [GDOC] Azurillkirby's comprehensive guide to Zoldath, linked from the
  wiki: the generation rules, trade prices, rooms and damage values.
  https://docs.google.com/document/d/1Zz6JpWmEUdlEvWlvodG8kJfwCD0mVCiCxr0T3bsnTYs
- [MANUALS] Steam guide "The missing manuals - How to play UFO 50 games",
  section 8: the HUD and the controls.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [GGC] Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [G-CHERRY] Steam guide "How To Get Cherry Clear For Planet Zoldath" and
  its comments. https://steamcommunity.com/sharedfiles/filedetails/?id=3334148433
- [G-TIPS] Steam guide "Planet Zoldath - Tips & Secret Item Uses".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3352757016
- [STEAM-PS] Steam thread "Pleasantly surprised by Planet Zoldath".
  https://steamcommunity.com/app/1147860/discussions/0/4849903998516710300/
- [STEAM-Q] Steam thread on the snail and meat.
  https://steamcommunity.com/app/1147860/discussions/0/4852154959746565110/
- [STEAM-IMP] Steam thread "impossible run".
  https://steamcommunity.com/app/1147860/discussions/0/604149013211542155/
- [STATIC] Static Canvas, "The UFO 50 Diaries: Planet Zoldath".
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-planet-zoldath
- [LIZ] Lizstar's Trashcan, UFO 50 retrospective, part 8.
  https://lizstar64.github.io/reviews/2024/10/09/UFO50-8.html
- [POPCAR] Popcar's Blog, "Reviewing Every Single UFO 50 Game".
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [KAYIN] Kayin, UFO 50 notes. https://kayin.moe/ufo-50
- [CONV] the research folder's `00-ufo50-conventions.md`: two buttons and
  START, the pause overlay, goals.
