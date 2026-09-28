# 13 · DUSKLING

*Internal design document. Not shown in the product.*

## Tribute to

**Mooncat** (UFO 50 game #13, Mossmouth). The mechanics were researched
from text only: the community wiki, Steam guides and threads, written
reviews and speedrun category names (listed under Sources). No UFO 50
images, video, maps, sprites, music or text were used as references, and
nothing was taken from a local install.

**Map type: fixed.** Mooncat's rooms are hand-made, so ours are too
(`duskling_rooms.c`, painted from a room sheet by a small script kept
outside the repo). We keep the structure the text describes, never a
layout: a linear main way through a forest, water, ruins and a high-tech
area to a boss and an egg, with invisible warps (marked by flowers) that
skip ahead, one that sends you back, and warps within warps that open two
other ways, each with its own boss and egg.

## Full-scale counts

| | Mooncat (text sources) | DUSKLING |
|---|---|---|
| Rooms | 42 [SN] | 42 |
| Main way | forest → underwater → ruins → high-tech area with a boss → egg [SN] | the dayling's walk, then 6 wood, 5 mere, 5 steps, 3 works rooms, the Brass Warden, the white egg (22 rooms) |
| Eggs / endings | 3 [W] | 3: white, amber, rose |
| Bosses | 3, one at the end of each egg route [IHZ], [SEC] | the Brass Warden, the Ember Hermit, the Old Badger |
| Warps | most skip areas, one sends you back, two lead to an alternative boss, eggs sit in "portals within portals" [SEC], [SG] | 9: five skips into warp pockets, one back, two warps within warps that open the other ways, and the amber way's first warp |
| Rooms seen on one egg's run | "21/42 rooms" after a first egg [SN] | the main way alone is 22 |
| Rooms not on any egg's way | a player "missing 10 levels" with all three eggs [SEC] | the five warp pockets, plus main rooms a warp skips |
| Length | ~50 min first, ~20 min later [SN]; speedruns 3-7 min [SRC] | the recorded routes add up to about 3 min of play for the main way |

## The map

Rooms by index (the route tests are named after them):

| # | Room | Area | Way on | Warp |
|---|---|---|---|---|
| 0 | THE LAST LIGHT | the dusk wood | the pit (the waking) | |
| 1 | WAKING GLADE | Hush Wood | 2 | |
| 2 | FERN STEPS | Hush Wood | 3 | blue bells → 37 MOSSY BURROW |
| 3 | HOLLOW LOG | Hush Wood | 4 | |
| 4 | CROW'S PERCH | Hush Wood | 5 | poppies, a stone face → 29 CROW'S WARP |
| 5 | THORN RUN | Hush Wood | 6 | |
| 6 | MERE EDGE | Hush Wood | 7 | |
| 7 | FIRST DIVE | Sunken Mere | 8 | blue bells → 22 GLOWWORM GROTTO |
| 8 | WEED MAZE | Sunken Mere | 9 | |
| 9 | BUBBLE SHAFT | Sunken Mere | 10 | blue bells → 38 BUBBLE POCKET |
| 10 | DROWNED BRIDGE | Sunken Mere | 11 | |
| 11 | MERE GATE | Sunken Mere | 12 | |
| 12 | BROKEN STAIR | Old Steps | 13 | |
| 13 | EYE HALL | Old Steps | 14 | |
| 14 | PILLAR RUN | Old Steps | 15 | poppies, a stone face → 39 DUSTY CRAWL |
| 15 | COLLAPSED NAVE | Old Steps | 16 | |
| 16 | TEMPLE ROOF | Old Steps | 17 | blue bells → 40 BACKWATER |
| 17 | COG WALK | Humming Works | 18 | |
| 18 | PISTON HALL | Humming Works | 19 | |
| 19 | LAST CORRIDOR | Humming Works | 20 | |
| 20 | THE WARDEN'S FLOOR | Humming Works | 21 (the door opens when the Warden falls) | |
| 21 | THE WHITE NEST | Humming Works | the white egg | |
| 22 | GLOWWORM GROTTO | warp pocket | 9 (back to the main way) | blue bells → 23 (the warp within the warp) |
| 23-26 | EMBER MOUTH, CINDER HALL, LAVA STEPS, NEWT DEN | Ember Caves | on | 24 has blue bells → 41 CINDER POCKET |
| 27 | THE HERMIT'S HEARTH | Ember Caves | 28 | |
| 28 | THE AMBER NEST | Ember Caves | the amber egg | |
| 29 | CROW'S WARP | warp pocket (tall) | 6 (back to the main way, past the thorn run) | poppies in a crevice → 30 (the warp within the warp) |
| 30-34 | WINDY FOOT, CLOUD LEDGES, GALE BRIDGE, STONE FACES, SUMMIT PATH | Windy Heights | on | |
| 35 | THE BADGER'S DEN | Windy Heights | 36 | |
| 36 | THE ROSE NEST | Windy Heights | the rose egg | |
| 37 | MOSSY BURROW | warp pocket | 7 (skips the rest of the wood) | |
| 38 | BUBBLE POCKET | warp pocket | 12 (skips the rest of the mere) | |
| 39 | DUSTY CRAWL | warp pocket | 17 (skips the rest of the steps) | |
| 40 | BACKWATER | warp pocket | 8 (the one warp that goes back) | |
| 41 | CINDER POCKET | warp pocket | 26 (skips the lava steps) | |

- **The amber way** starts where you first go into the water: the warp in
  FIRST DIVE is reached from the invisible block left of the gulper, or by
  bouncing off both frogs [SG]. Its pocket leads back to the main way; the
  warp within it needs a slam through a pink ledge onto a hidden spring.
- **The rose way** is the trickiest [TY]. In CROW'S PERCH, by the crow
  holding a pebble: jump over the stone face to show the hidden steps,
  climb to the ledge top left with the poppies, then sprint, jump and
  somersault as far right as you can [SG]. In the tall pocket that
  follows, the warp within the warp is in a crevice in the left wall of
  the climb [SG].
- **The difficulty curve** follows Mooncat's: the first rooms are simple
  ("less than Mario 1" [LZ]) because the controls are the challenge; the
  warps are hard to find rather than hard to cross [IHZ]; the rose way
  asks for everything (one-tile clouds, sprint-somersault gaps, faces in
  order).

## Mechanics checklist

| Mechanic | How DUSKLING does it | Source |
|---|---|---|
| Two sides | every D-pad direction is left, A and B are both right | [W], [LZ], [MM] |
| Walk | hold a side | [W] |
| Jump | hold one side, then press the other: a jump toward the side held | [W], [LZ], [SC] |
| Higher jump | keep the second press down: the jump keeps rising for up to 16 frames | [W] ("holding longer = longer jump", search summary) |
| Low jump | tap a side, then press both at once: a low hop toward the side tapped | [W] |
| Roll | double tap a side on the ground | [W], [LZ] |
| Sprint | double tap and keep the second tap held; a sprint gives more time to jump after running off an edge | [W] ("sprint with extended coyote time") |
| Somersault | double tap a side in the air: adds speed that way or takes it off; once a jump | [W], [SG] |
| Slam | in the air, hold one side and press the other again: a fast drop that beats foes, bounces you back up (steer the bounce with the side held), and drops through pink ledges | [W], [LZ], [IHZ] |
| Pink ledges and clouds | one-way: jump up through, stand on top | [W], [LZ] |
| Momentum | a jump keeps sprint speed; landing fast with the side held keeps the sprint; a walk-through exit keeps your speed into the next room | [SC] |
| One touch | any deadly foe, thorn, shot or fall | [IHZ], [SC] |
| Lives | unlimited; a death puts you back where you came into the room, and the room starts over | [MM], [SC] |
| No mid-run save | quitting loses the run; only eggs, warps taken and rooms seen are kept | [MM], [SN] |
| Harmless creatures | thistledown and the crow drift past; frogs are springs | [SC], [SG] |
| Creatures as ledges | gulpers (while shut), fallen eyes and curled beetles are blocks; walking beetles carry you | [SC] |
| Knocked about | a slam on a pebble beetle knocks it flying; it curls up where it lands, a block somewhere new | [LZ] |
| Clockwork | every foe moves on a fixed timer from the moment you enter | [SC] |
| Frogs | landing on one springs you high (a slam on it higher) | [SG] |
| Mouth | the gulper, a two-tile mouth: a ledge while shut, it bites when it opens; the amber warp's hidden block is left of one | [SG] |
| Robot eyes | ceiling eyes drop when a slam lands near them (and crush what's under them); a slam onto a fallen eye makes it leap up, carrying you | [SG], search summary of the cherry guide |
| Spear throwers | spear newts, in the amber way (a warp area), throw spears on a timer | [SEC] |
| Invisible bouncy blocks | hidden springs: invisible until touched | [SEC] |
| Hint flowers | blue bells or orange poppies in the background: a warp is on this screen | [W] |
| Stone faces | jumping over one shows the hidden ledges near it | [W], [LZ], [SG] |
| Warps | never drawn until touched; they skip ahead, one goes back, two lead to the other ways and bosses; eggs are behind a warp within a warp | [W], [SEC], [SG] |
| Bosses | hurt only by a slam that is falling onto them while they are solid | search summary of [SPD] ("you only need to be moving downward during the ground pound when the boss walks into you or becomes tangible again") |
| Main boss | the Brass Warden walks at you, fades out and glides across (passing harmlessly through you), and sends sparks along the floor; six slams, faster after each | [SN], [SPD] |
| Fireball wizard | the Ember Hermit appears on one of four perches, throws three fire sparks at you and vanishes | [SEC] |
| Giant rabbit | the Old Badger: slow, five ledges to keep out of its reach; it jumps up onto yours and blasts at you from where it lands; the easiest of the three | [TY] |
| Eggs | each egg is an ending: the egg opens and shows what is inside | [W] |
| Opening | an orange creature drifts down from the clouds with a little light, walks an empty wood, and falls into a pit whose far side isn't there; a pink one wakes and plays the rest | [W], [SI], [TG] |
| Two players | co-op on one screen; landing on the other's head bounces you and stuns them; a secret line shows at the end after three head bounces | [W], [MH-raw] |
| Nod to game 1 | the mole's tent from our cartridge 01 stands in the broken stair, as Barbuta's first room turns up in Mooncat's ruins | [W], [SN] |
| Stats | rooms seen out of 42 on the title | [SN] |

### Readings we had to choose

Numbers not in any source are ours, and so are these readings:

- **Rooms scroll.** The text speaks of "rooms" and "levels" that are each a
  checkpoint "every minute or so" [SC], [MM]. Ours are 1 to 3.5 screens
  wide (four are tall), with the camera following. 10-px tiles, a view of
  32 × 18.
- **The feel numbers.** Walk 1.2 px/frame, sprint 2.2, roll 3.0 falling to
  sprint speed over 18 frames. A tapped jump rises 2 tiles, a held one 4
  and carries 5 across; a sprint jump carries 9, and a sprint jump with a
  somersault 13. A low hop rises 1. "At once" means within 3 frames; a
  double tap is two presses within 14 frames with a release between.
  Coyote time is 5 frames walking and 12 sprinting.
- **Which press is the slam.** A new press of one side while the other is
  held, in the air (the same move that jumps on the ground). A slam falls
  at 5 px/frame, carries a little sideways speed, bounces 2.4 off the
  ground and 3.8 off a foe.
- **Only the slam beats foes.** Plain landings on a deadly foe kill you;
  the boss hint describes hits as downward slams only [SPD].
- **What each creature does** is ours: the text only says some are
  harmless, some deadly, some ledges, some knocked about, all on clockwork
  [SC], [LZ].
- **Water.** Mooncat has an underwater area [SN]; how swimming works isn't
  written anywhere. Ours: slow sinking, any jump is a stroke, and a stroke
  that breaks the surface leaps out.
- **Lives.** The missing-manuals guide says unlimited lives [MM]; a search
  summary spoke of three per checkpoint. We use unlimited: either way a
  death never costs more than the room.
- **Eggs across runs.** Each egg is an ending, so the cherry has to count
  eggs over several runs: found eggs are saved.
- **Which way is which.** On the wiki the plain way gives the green egg,
  red flowers lead to the yellow one and green flowers with the Moai to
  the red [W]; speedrun categories name all three [SRC]. Our eggs are
  white (the main way), amber (blue bells) and rose (orange poppies and
  stone faces).
- **Stone faces wake at one jump over them** (the wiki); a Steam post says
  eggs "spawn platforms if you jump (or walk) around them a bunch" [SG2];
  we did not add a second kind.
- **Bosses.** Hit points (Warden 6, Hermit 5, Badger 5), patterns and
  speeds are ours. A death restarts the boss room with the boss whole.
- **Two players.** How co-op handles deaths and room changes isn't in the
  sources: a fallen player comes back at the room's way in while the other
  plays on, either one reaching the way out takes both, and one left far
  off screen is brought to the other. Our end line is "COMFY SOCKS,
  ALWAYS".
- **What the eggs hold** is ours: a cold fog and grey mud, an old moth
  once called Tallow, a dented helmet and a small bent sword. (Mooncat's
  are slime and miasma, a mantis sage, and a knight's sword and shield
  [W]; which egg holds which isn't stated.)

### Checked by the route finder

`cheat solve ROOM TARGET` runs a best-first search over short button
patterns (walks, jumps held for 1, 7 or 16 frames, low hops, rolls,
sprints, somersaults and slams, each with a few ways to steer) on the real
rules, and prints the presses as script lines. Every route test was
recorded from it. With test switches (`cheat nerf`) that put the faces to
sleep, flatten the frogs, turn springs into plain blocks or bolt the eyes
down, it finds no way to the rose warp in CROW'S PERCH, the dusty-crawl
warp or through CLOUD LEDGES and STONE FACES without the faces; none
through PISTON HALL or to the glowworm grotto's inner warp without
springs; none through EYE HALL, the COLLAPSED NAVE or to the amber egg
without eyes; and none up the SUMMIT PATH without frogs. The HOLLOW LOG
and THORN RUN frogs can also be skipped with a jump and a somersault off a
pink cap, which we left in as a skill route. The quick exhaustive cases
(faces, eyes) run in `dk_20`.

## What is ours

- **Name:** DUSKLING (1985, Beamdown Softworks).
- **Characters:** the pink duskling, the orange dayling and its little
  light, and a blue second duskling for player 2.
- **Places:** the dusk wood, the Hush Wood, the Sunken Mere, the Old Steps,
  the Humming Works, the Ember Caves, the Windy Heights and the warp
  pockets.
- **Creatures:** prickles, wasps and drones, thistledown, frogs, gulpers,
  ceiling eyes, pebble beetles, spear newts, the crow with a pebble and
  fish.
- **Bosses:** the Brass Warden, the Ember Hermit, the Old Badger.
- **Rooms:** all 42 layouts.
- **Text:** the egg lines, the help pages and "COMFY SOCKS, ALWAYS".
- **Music:** twelve original UFO-MML tunes (DUSK BELL, LAST LIGHT, WAKING,
  HUSH WOOD, SUNKEN MERE, OLD STEPS, HUMMING WORKS, EMBER CAVES, WINDY
  HEIGHTS, HIDDEN PLACES, THREE KEEPERS, THE EGG).

## Additions: none

Only what every UFO 40 cartridge has:

- the START pause menu, with RETRY ROOM under RESUME (the same as a fall);
- three how-to-play pages on SELECT at the title (Mooncat has no tutorial;
  the pages only list what the controls page shows);
- two players locked on the Vita, which has one controller;
- saving: the eggs found, the warps taken and the rooms seen;
- the three goals, which replicate Mooncat's own:

| UFO 40 goal | Condition | Mooncat's goal |
|---|---|---|
| Beacon | take a warp | gift: find a warp [W] |
| Saucer | reach an egg (any egg) | gold: beat the game; speedrun categories give gold for each egg [W], [SRC] |
| Alien | have found all three eggs | cherry: find all 3 eggs [W] |

## Controls

| Input | Action |
|---|---|
| D-pad (any way) | walk left |
| A or B | walk right |
| hold one side, tap the other | jump toward the held side (hold the tap: higher) |
| tap a side, then both at once | a low hop |
| double tap | roll; keep it held: sprint |
| double tap in the air | somersault |
| in the air, hold one side and press the other | slam |
| START | pause (RETRY ROOM) |
| SELECT (title) | how to play |

## Sources

- [W] UFO 50 Wiki (Miraheze), "Mooncat" (page and raw text): the controls
  (the jump, the low jump, the roll, the sprint with more coyote time, the
  somersault, the slam with directional control), flowers and Moai, which
  flowers lead where, the egg colours and contents, the opening, the goals,
  two players. https://ufo50.miraheze.org/wiki/Mooncat
- [MM] Steam guide "The missing manuals": two sides of the pad, unlimited
  lives, back to the room's start, no saving.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [SG] Steam thread "Where are the other eggs in Mooncat?": the egg by the
  first dive (secret platforms left of a mouth, or both frogs; thump to
  make the eye jump up), the egg by the green bird holding a rock (the
  ledge top left of a flower, jump as far right as you can; the crevices on
  the left of the climb), "dash, jump, airspin".
  https://steamcommunity.com/app/1147860/discussions/0/4852154959749605174/
- [SG2] Steam thread "Mooncat": eggs that make platforms appear, warps back
  to the main way or to eggs, the third egg needs a certain order.
  https://steamcommunity.com/app/1147860/discussions/0/4849903998512463415/
- [SEC] Steam thread "Mooncat secrets": most warps skip areas, one goes
  back, eggs in portals within portals, a fireball wizard boss, two warps
  to an alternative boss, invisible bouncy blocks, spear-throwing lizards
  in warp areas, "missing 10 levels".
  https://steamcommunity.com/app/1147860/discussions/0/4852155320351525504/
- [SN] Steam thread on Mooncat (first impressions): 42 rooms, "21/42", the
  forest, underwater, high-tech area and boss, first and later run times,
  no mid-run save.
  https://steamcommunity.com/app/1147860/discussions/0/4852154959747742751/
- [LZ] Lizstar, "UFO 50 Retrospective", Mooncat: two sides, jumping toward
  the first side, slams that bounce and drop through platforms, knocking
  foes about, skulls that show platforms, level exits and portals.
  https://lizstar64.github.io/reviews/2024/10/12/UFO50-13.html
- [SC] Static Canvas, "The UFO 50 Diaries: Mooncat": every room a
  checkpoint, harmless and deadly foes, foes as platforms, clockwork,
  momentum. https://staticcanvas.substack.com/p/the-ufo-50-diaries-mooncat
- [IHZ] Indie Hell Zone, Devilition/Mooncat/Rail Heist/Quibble Race:
  one-hit deaths, three bosses, finding them is the challenge.
  https://indiehellzone.com/2024/10/30/ufo-50-devilition-mooncat-rail-heist-quibble-race/
- [TY] TV Tropes, "YMMV / UFO 50 Game 13 Mooncat" (search snippet only):
  the giant rabbit at the end of the red egg path, the easiest boss, slow,
  five platforms, jumps onto them, a ranged fire blast.
- [SPD] speedrun.com guide "Mooncat Gold Boss Quick Kill" (title and a
  search summary; the page itself would not load): the boss is hit while
  moving downward in a slam, when it walks into you or turns solid again.
- [SRC] speedrun.com UFO 50 category names: Gold (Red, Green, Yellow Egg),
  Cherry, Warpless, 1 and 2 players.
- [SI] Steam thread "Mooncat intro" and [TG] TheGamer's article: the
  orange creature, the pit, the pink one (via the research bible).
- [MH-raw] the wiki's raw text, via the research bible: bouncing on the
  other player's head three times shows a line at the end.
- The research bible `13-mooncat.md` (UFO 40 notes) collects these and the
  reviews by Popcar, PixelDie and Kayin used for feel only.

## Progress

- Built: `src/games/duskling/` (game, world, rooms, art, audio), slot 13.
- Tests: `dk_01`-`dk_05` the controls (two sides, jumps, the low hop, the
  roll and sprint with its longer coyote time, the somersault, the slam,
  pink ledges); `dk_06` one touch and unlimited lives; `dk_07` slams on
  foes; `dk_08` harmless creatures, frogs, gulpers and beetles; `dk_09`
  eyes; `dk_10` faces; `dk_11` springs; `dk_12` water; `dk_13` warps and
  the Beacon; `dk_14` eggs, the Saucer and the Alien; `dk_15` no mid-run
  save; `dk_16` the dayling's walk; `dk_17` the bosses; `dk_18` two
  players; `dk_19` RETRY ROOM; `dk_20` the room checks and secrets needed;
  `dk_21` the opening from the title. `dk_r01`-`dk_r41` cross every room
  from its way in with plain button presses, and take every warp.
- Media: `docs/shots/duskling.gif` and stills, from
  `tools/shots/13_duskling.ufs` (it replays the route tests).
