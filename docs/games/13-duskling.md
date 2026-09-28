# 13 · DUSKLING

*Internal design document. Not shown in the product.*

## Tribute to

**Mooncat** (UFO 50 game #13, Mossmouth). The mechanics were researched
from text only: the community wiki, Steam guides and threads, written
reviews, TV Tropes and speedrun guide snippets and speedrun.com's record
tables (listed under Sources). No UFO 50 images, video, maps, sprites,
music or text were used as references, and nothing was taken from a local
install. An independent review against Mooncat (twenty findings) was worked
through; the changes are folded into this document.

**Map type: fixed.** Mooncat's rooms are hand-made, so ours are too
(`duskling_rooms.c`, painted from a room sheet by a small script kept with
the UFO 40 notes, `tools13/`). We keep the structure the text describes,
never a layout or a solution: a linear main way through a forest, water,
ruins and a high-tech area to a boss and an egg; invisible warps (marked by
flowers) that skip ahead, one that sends you back to the start; and warps
within warps that open two other ways, each with its own boss and egg.
Every secret's solution is ours (see "Secrets are ours").

## Full-scale counts

| | Mooncat (text sources) | DUSKLING |
|---|---|---|
| Rooms | 42 [SN] | 42 |
| Main way | forest → underwater → ruins → high-tech area with a boss → egg [SN] | the dayling's walk, then 6 wood, 5 mere, 5 steps, 3 works rooms, the Brass Warden, the white egg (22 rooms) |
| Other ways | two, each ending in a boss and an egg; eggs sit in "portals within portals" [SEC], [SG] | the amber way (5 rooms, the Ember Hermit) and the rose way (5 rooms, the Old Badger) |
| Rooms off every egg's way | a player with all three eggs was "missing 10 levels" [SEC] | 10 warp pockets |
| Eggs / endings | 3 [W] | 3: white, amber, rose |
| Bosses | 3 [IHZ], [SEC] | the Brass Warden, the Ember Hermit, the Old Badger |
| Warps | most skip areas, one returns to the start area, two lead to an alternative boss [SEC], [SG2] | 14: ten into warp pockets (nine skip ahead, one goes back to the start of the wood), two that open the other ways, and the two warps within them |
| Pace (speedrun.com world records) | warpless 325 s, green egg 256 s, yellow egg 219 s, red egg 197 s [SRC] | recorded routes from the waking: main way 288 s, amber way 185 s, rose way 218 s |
| Length | ~50 min first, ~20 min later, "a checkpoint every minute or so" [SN], [SC] | rooms of 32 to 256 tiles; the busiest take a minute or more |

The route tests are recorded by a route finder that plays close to
perfectly, so their totals are compared with the world records.

## The map

Rooms by index (the route tests are named after them):

| # | Room | Area | Way on | Warp |
|---|---|---|---|---|
| 0 | THE LAST LIGHT | the dusk wood | the pit (the waking) | |
| 1 | WAKING GLADE | Hush Wood | 2 | |
| 2 | FERN STEPS | Hush Wood | 3 | blue bells → 37 MOSSY BURROW |
| 3 | HOLLOW LOG | Hush Wood | 4 | poppies → 34 MUSHROOM HOLLOW |
| 4 | BRAMBLE HOLLOW | Hush Wood | 5 | |
| 5 | THORN RUN | Hush Wood | 6 | |
| 6 | MERE EDGE | Hush Wood (the Ember Hermit runs off) | 7 | |
| 7 | FIRST DIVE | Sunken Mere | 8 | blue bells → 22 GLOWWORM GROTTO (the amber way) |
| 8 | WEED MAZE | Sunken Mere | 9 | blue bells → 41 CINDER POCKET |
| 9 | BUBBLE SHAFT | Sunken Mere | 10 | blue bells → 38 BUBBLE POCKET |
| 10 | DROWNED BRIDGE | Sunken Mere | 11 | blue bells → 26 NEWT DEN |
| 11 | MERE GATE | Sunken Mere | 12 | poppies → 29 SEED VAULT (the rose way) |
| 12 | BROKEN STAIR | Old Steps (the mole's tent) | 13 | |
| 13 | EYE HALL | Old Steps | 14 | |
| 14 | PILLAR RUN | Old Steps | 15 | poppies, a stone face → 39 DUSTY CRAWL |
| 15 | COLLAPSED NAVE | Old Steps | 16 | |
| 16 | TEMPLE ROOF | Old Steps | 17 | blue bells → 40 BACKWATER |
| 17 | COG WALK | Humming Works | 18 | blue bells → 32 GALE BRIDGE |
| 18 | PISTON HALL | Humming Works | 19 | |
| 19 | LAST CORRIDOR | Humming Works | 20 | |
| 20 | THE WARDEN'S FLOOR | Humming Works | 21 (the door opens when the Warden falls) | |
| 21 | THE WHITE NEST | Humming Works | the white egg | |
| 22 | GLOWWORM GROTTO | amber way | 9 (back to the main way) | blue bells → 23 (the warp within the warp) |
| 23 | EMBER MOUTH | Ember Caves | 24 | blue bells → 25 LAVA STEPS |
| 24 | CINDER HALL | Ember Caves (bones, rolling eyes) | 27 | |
| 27 | THE HERMIT'S HEARTH | Ember Caves | 28 | |
| 28 | THE AMBER NEST | Ember Caves | the amber egg | |
| 29 | SEED VAULT | rose way | 12 (back to the main way) | poppies → 30 (the warp within the warp) |
| 30 | WINDY FOOT | Windy Heights | 33 | poppies → 31 CLOUD LEDGES |
| 33 | STONE FACES | Windy Heights | 35 | |
| 35 | THE BADGER'S DEN | Windy Heights | 36 | |
| 36 | THE ROSE NEST | Windy Heights | the rose egg | |
| 25 | LAVA STEPS | pocket | 27 (skips the cinder hall) | |
| 26 | NEWT DEN | pocket | 11 (skips the rest of the bridge) | |
| 31 | CLOUD LEDGES | pocket | 35 (skips the stone faces) | |
| 32 | GALE BRIDGE | pocket | 19 (skips the piston hall) | |
| 34 | MUSHROOM HOLLOW | pocket (mushroom caps) | 8 (skips from the wood into the mere) | |
| 37 | MOSSY BURROW | pocket | 7 (skips the rest of the wood) | |
| 38 | BUBBLE POCKET | pocket | 12 (skips the rest of the mere) | |
| 39 | DUSTY CRAWL | pocket (the mole's tent again, in ruins) | 17 (skips the rest of the steps) | |
| 40 | BACKWATER | pocket | 1 (the one warp back: to the start of the wood) | |
| 41 | CINDER POCKET | pocket | 10 (skips the bubble shaft) | |

- **The amber way** starts where you first go into the water (Mooncat's
  middle egg is found from its first water screen [SG]). Its first pocket
  leads back to the main way; the warp within it needs a slam through a
  pink ledge onto a hidden spring. In the Ember Caves the Hermit, who ran
  off on the main way, is fought.
- **The rose way** leaves from the top of the mere gate: a slam onto the
  frog between the poppies is the only thing that throws you high enough.
  The warp within it lies at the bottom of a pool under a lid of gulpers.
- **The difficulty curve** follows Mooncat's: the first rooms are simple
  ("less than Mario 1" [LZ]) because the controls are the challenge; the
  warps are hard to find rather than hard to cross [IHZ]; the pace and
  density rise through the ruins and the works, and the other ways ask for
  everything.

## Mechanics checklist

| Mechanic | How DUSKLING does it | Source |
|---|---|---|
| Two sides | every D-pad direction is left, A and B are both right | [W], [LZ], [MM] |
| Input A / B | the title offers P1 PAD and P2 PAD, A or B; B turns the pad round (the D-pad walks right, the buttons left); saved | [CG] ("P1 INPUT A for normal controls, type B reverses left/right"), [DBG] |
| Walk | hold a side | [W] |
| Jump | hold one side, then press the other: a jump toward the side held | [W], [LZ], [SC] |
| Higher jump | keep the second press down: the jump keeps rising for up to 16 frames | [W], [CG] ("hold for longer jump") |
| Low jump | tap a side, then press both at once: a low hop toward the side tapped | [W] |
| Roll | double tap a side on the ground | [W], [LZ] |
| Sprint | double tap and keep the second tap held; a sprint gives more time to jump after running off an edge | [W] ("sprint with extended coyote time") |
| Somersault | double tap a side in the air: adds speed that way or takes it off; once a jump | [W], [SG] |
| Slam | in the air, hold one side and press the other again: the duskling stops for a few frames with a "!" over its head, then drops straight down | [W] ("pause in mid-air for a couple frames with an exclamation mark above it before going straight down") |
| Slam bounce | the bounce off the ground, a foe, a boss, a frog, a spring or a mushroom goes the way the side held at contact points, or straight up with nothing held | [W], [SL] ("the direction you're holding when you hit an enemy is the direction you'll bounce") |
| Slam through ledges | a slam drops through pink ledges and clouds | [W], [LZ] |
| Flipping | a slam that lands near a walker on the same floor flips it onto its back for three seconds; walking into it then kicks it off the screen | [CG] ("slam nearby and they'll flip, then just walk into them"), [TR] |
| Spiked walkers | a prickle's spikes kill even a slam: it is beaten only by flipping it | [TR] ("spiked cone enemies need to be flipped over by using a Ground Pound near them") |
| Hits | a slam counts on any contact while it is coming down, from above or not | [SPD] ("it only checks if you're moving down when you hit an enemy") |
| Momentum | a jump keeps sprint speed; landing fast with the side held keeps the sprint; a walk-through exit keeps your speed into the next room | [SC] |
| One touch | any deadly foe, thorn, shot or fall | [IHZ], [SC] |
| Lives | unlimited; a death puts you back where you came into the room, and the room starts over | [MM], [SC] |
| No mid-run save | quitting loses the run; only eggs, warps taken, rooms seen and the pad setting are kept | [MM], [SN] |
| Near misses | thistledown drifts along just over a standing duskling's head: a jump into it is the end | [SC] ("some will scoot over you, barely missing by a fraction of a pixel") |
| Landmark | the crow on the bramble hollow's plateau is harmless | [SC] |
| Creatures as ledges | gulpers (while shut), fallen eyes and curled beetles are blocks; walking beetles carry you | [SC] |
| Knocked about | a slam on a pebble beetle knocks it flying the way the side held points; it curls up where it lands, a block somewhere new | [LZ] |
| Clockwork | every foe moves on a fixed timer from the moment you enter | [SC] |
| Frogs | landing on one springs you high (a slam on it higher) | [SG] |
| Mouth | the gulper, a two-tile mouth: a ledge while shut, it bites when it opens | [SG] |
| Robot eyes | ceiling eyes drop when a slam lands near them (and crush what's under them); a slam onto a fallen eye makes it leap up, carrying you | [SG], [CG] |
| Rolling eyes in a bone cave | fallen eyes roll slowly along the floor; the cinder hall has bones in the background | [TM] ("Eyes roll in the skeleton caves") |
| Spear throwers | spear newts, in the amber way and a warp pocket, throw spears on a timer | [SEC] |
| Invisible bouncy blocks | hidden springs: invisible until touched | [SEC] |
| Bounce mushrooms | a warp pocket full of mushroom caps that spring you only when slammed | [SG2] ("jumping mushroom area"), [SL] |
| Hint flowers | blue bells or orange poppies in the background: a warp is on this screen | [W] |
| Stone faces | jumping over one twice wakes it and shows the hidden ledges near it | [W], [LZ], [SG] ("jump or walk around them a bunch") |
| Warps | never drawn until touched; they skip ahead, one goes back to the start, two lead to the other ways and bosses; eggs are behind a warp within a warp | [W], [SEC], [SG], [SG2] |
| The wizard on the main way | before the water the Ember Hermit throws sparks from three perches and vanishes to the next as you come close; he can't be hurt there, and is fought on the amber way | [CG] ("make your way past the wizard and jump into the water"), [SEC] ("fighting the fireball wizard boss instead of just having them run away") |
| Main boss | the Brass Warden walks at you, fades out and glides across (passing harmlessly through you), and sends sparks along the floor; six slams, faster after each | [SN], [SPD] |
| Fireball wizard | the Ember Hermit appears on one of four perches, throws three sparks at you and vanishes; five slams | [SEC] |
| Giant rabbit | the Old Badger: slow, five ledges to keep out of its reach; it jumps up onto yours and blasts at you from where it lands; eight slams; the easiest of the three | [TY], [CG] ("it'll take 8 solid hits") |
| Eggs | each egg is an ending: the egg opens and shows what is inside, and "N OF 3" | [W] |
| No credits | there is no credits or stats page: the egg's page is the end | [W] ("all three games lack credits") |
| Opening | an orange creature drifts down from the clouds with a little light, walks an empty wood, and falls into a pit whose far side isn't there; a pink one wakes and plays the rest | [W], [SI], [TG] |
| Two players | co-op on one screen; landing on the other's head bounces you and stuns them; three head bounces in a row, with no other landing between, show a line at the end | [W], [MSG] ("bounces on another player's head three times in a row") |
| Nod to game 1 | the mole's tent from our cartridge 01 stands in the broken stair, and again, with the same shapes, in the ruins of the dusty crawl | [W] ("Barbuta's starting room is seen twice, but in ruins"), [SN] |
| Stats | rooms seen out of 42 on the title | [SN] |

### Secrets are ours

The Steam egg thread [SG] describes Mooncat's own solutions (a green bird
holding a rock by the ledge top left of a flower, a jump as far right as you
can, crevices on the left of a climb, hidden platforms left of a mouth,
bouncing on both frogs). None of those is rebuilt here; the mechanics are
Mooncat's, the solutions are ours:

- **Hollow log** (to the mushroom pocket): a pebble beetle is a step. From
  its back, walking or knocked against the trunk by a slam and curled up,
  a jump reaches the warp; the log alone is a tile short.
- **Mere gate** (the rose way): a slam onto the frog on the top step; a
  plain bounce falls short.
- **First dive** (the amber way): far out over the water from a rock by
  the blue bells, with a long jump and a somersault, or by slamming off the
  wasp that patrols there.
- **Seed vault** (the warp within the rose way): at the bottom of a pool
  under a lid of opening and closing gulpers.
- **Glowworm grotto** (the warp within the amber way): a slam through a pink
  ledge onto a hidden spring.
- **Amber nest**: a slam onto a hidden spring at the foot of the egg's
  ledge (a plain bounce falls short).
- **Pillar run**: two jumps over a stone face show the steps to the warp.

### Readings we had to choose

Numbers not in any source are ours, and so are these readings:

- **Rooms scroll.** The text speaks of "rooms" and "levels" that are each a
  checkpoint "every minute or so" [SC], [MM], and guides describe "down to
  up sections" within one level [SG]; Mooncat's precursor is "a simple
  sidescroller" [MC]. Ours are 1 to 8 screens wide (four are taller than a screen), with
  the camera following. 10-px tiles, a view of 32 × 18.
- **The feel numbers.** Walk 1.2 px/frame, sprint 2.2, roll 3.0 falling to
  sprint speed over 18 frames. A tapped jump rises 2 tiles, a held one 4
  and carries 5 across; a sprint jump carries 9, and a sprint jump with a
  somersault 13. A low hop rises 1. "At once" means within 3 frames; a
  double tap is two presses within 14 frames with a release between.
  Coyote time is 5 frames walking and 12 sprinting.
- **The slam.** A new press of one side while the other is held, in the
  air. It hangs 7 frames, falls at 5 px/frame, bounces 2.4 off the ground
  and 3.8 off a foe. A flip reaches 4 tiles each way on the same floor and
  lasts 3 s.
- **Only the slam beats foes.** Plain landings on a deadly foe kill you;
  whether Mooncat's plain landings stomp anything is not settled by the
  sources (reviews say "enemy stomping", the wiki ties hits to the slam).
- **What each creature does** is ours beyond what the text says (some
  harmless, some deadly, some ledges, some knocked about or flipped, all on
  clockwork).
- **Water.** Mooncat has an underwater area [SN]; how swimming works isn't
  written anywhere. Ours: slow sinking, any jump is a stroke, and a stroke
  that breaks the surface leaps out.
- **Lives.** Unlimited, as the missing-manuals guide says [MM]; the "three
  per checkpoint" line seen in a search summary most likely describes the
  jam game that came before Mooncat.
- **Eggs across runs.** Each egg is an ending, so the cherry has to count
  eggs over several runs: found eggs are saved.
- **Which way is which.** On the wiki the plain way gives the green egg,
  red flowers lead to the yellow egg and green flowers with the Moai to the
  red one [W]; the knight's sword is in the yellow egg [TR], [CG]. Our
  eggs are white (the main way: a cold fog and grey mud), amber (a dented
  helmet and a bent sword) and rose (an old moth once called Tallow).
- **Where the rose way starts** isn't in the text beyond a landmark; ours
  leaves from the mere gate, which puts its pace near the red egg's.
- **Bosses.** The Warden's and Hermit's hit counts and all three patterns
  are ours; the Badger's eight hits are Mooncat's rabbit's [CG]. A death
  restarts the boss room with the boss whole.
- **Two players.** How co-op handles deaths and room changes isn't in the
  sources: a fallen player comes back at the room's way in while the other
  plays on, either one reaching the way out takes both, and one left far
  off screen is brought to the other. Our end line is "COMFY SOCKS,
  ALWAYS".

### Checked by the route finder

`cheat solve ROOM TARGET` runs a best-first search over short button
patterns (walks, jumps held for 1, 7 or 16 frames, low hops, rolls,
sprints, somersaults and slams, each with a few ways to steer) on the real
rules, and prints the presses as script lines; `cheat solvevia` chains it
through waypoints painted in the room sheet. Every route test was recorded
from it. With test switches (`cheat nerf`) that put the faces to sleep,
flatten the frogs, turn springs into plain blocks or bolt the eyes down, it
finds, from beside the hidden springs under the amber egg and under the
glowworm grotto's warp within the warp, no way on without them (checked in
`dk_20`; the one under the egg is an exhaustive search).

## What is ours

- **Name:** DUSKLING (1985, Beamdown Softworks).
- **Characters:** the pink duskling, the orange dayling and its little
  light, and a blue second duskling for player 2.
- **Places:** the dusk wood, the Hush Wood, the Sunken Mere, the Old Steps,
  the Humming Works, the Ember Caves, the Windy Heights and the warp
  pockets.
- **Creatures:** prickles, wasps and drones, thistledown, frogs, gulpers,
  ceiling eyes, pebble beetles, spear newts, a crow and fish.
- **Bosses:** the Brass Warden, the Ember Hermit, the Old Badger.
- **Rooms:** all 42 layouts and every secret's solution.
- **Text:** the egg lines, the help pages, the title's pad wording and
  "COMFY SOCKS, ALWAYS".
- **Music:** twelve original UFO-MML tunes (DUSK BELL, LAST LIGHT, WAKING,
  HUSH WOOD, SUNKEN MERE, OLD STEPS, HUMMING WORKS, EMBER CAVES, WINDY
  HEIGHTS, HIDDEN PLACES, THREE KEEPERS, THE EGG).

## Additions: none

Only what every UFO 40 cartridge has:

- the START pause menu, with RETRY ROOM under RESUME (the same as a fall);
- three how-to-play pages on SELECT at the title (Mooncat has no tutorial;
  the pages only list what the controls page shows);
- two players locked on the Vita, which has one controller;
- saving: the eggs found, the warps taken, the rooms seen and the pad
  setting;
- the three goals, which replicate Mooncat's own, in our words:

| UFO 40 goal | Condition | Mooncat's goal |
|---|---|---|
| Beacon: TAKE A HIDDEN WAY | take a warp | gift: find a warp [W] |
| Saucer: OPEN AN EGG | reach an egg (any egg) | gold: beat the game; speedrun categories give gold for each egg [W], [SRC] |
| Alien: OPEN EVERY EGG | have found all three eggs | cherry: find all 3 eggs [W] |

## Controls

| Input | Action (pad A; pad B swaps the sides) |
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
  somersault, the slam's pause and "!", bounce direction), flowers and
  Moai, which flowers lead where, the egg colours and contents, the
  opening, the goals, two players, no credits, Barbuta's room seen twice.
  https://ufo50.miraheze.org/wiki/Mooncat
- [MSG] UFO 50 Wiki, "Meta Messages": three head bounces in a row.
  https://ufo50.miraheze.org/wiki/Meta_Messages
- [DBG] UFO 50 Wiki, "Debug mode": input style A and B for Mooncat.
  https://ufo50.miraheze.org/wiki/Debug_mode
- [TM] UFO 50 Wiki, "Terminal": "Eyes roll in the skeleton caves".
  https://ufo50.miraheze.org/wiki/Terminal
- [MM] Steam guide "The missing manuals" (Mooncat section, read in full):
  two sides of the pad, unlimited lives, back to the room's start.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [CG] Steam guide "Mooncat Cherry clear guide + basics", via search
  summaries (the page returned 429): input A/B, hold for a longer jump, the
  slam near a foe to flip it, the wizard before the water, the rabbit's 8
  hits, the knight's sword in the yellow egg.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3334076070
- [SL] Steam thread on the slam: the bounce follows the direction held.
  https://steamcommunity.com/app/1147860/discussions/0/4849904828212277540/
- [SG] Steam thread "Where are the other eggs in Mooncat?": the first-dive
  egg and Mooncat's own egg solutions (which we do not rebuild), "down to
  up" sections.
  https://steamcommunity.com/app/1147860/discussions/0/4852154959749605174/
- [SG2] Steam thread "Mooncat": eggs that make platforms appear, warps back
  to the main way or to eggs, one portal back to the starting area, a
  jumping-mushroom area, the third egg needs a certain order.
  https://steamcommunity.com/app/1147860/discussions/0/4849903998512463415/
- [SEC] Steam thread "Mooncat secrets": most warps skip areas, one goes
  back, eggs in portals within portals, a fireball wizard boss who runs
  away on the main way, two warps to an alternative boss, invisible bouncy
  blocks, spear-throwing lizards in warp areas, "missing 10 levels".
  https://steamcommunity.com/app/1147860/discussions/0/4852155320351525504/
- [SN] Steam thread on Mooncat (first impressions): 42 rooms, "21/42", the
  forest, underwater, high-tech area and boss, run times, no mid-run save,
  a starting area seen again halfway.
  https://steamcommunity.com/app/1147860/discussions/0/4852154959747742751/
- [LZ] Lizstar, "UFO 50 Retrospective", Mooncat.
  https://lizstar64.github.io/reviews/2024/10/12/UFO50-13.html
- [SC] Static Canvas, "The UFO 50 Diaries: Mooncat": every room a
  checkpoint, near misses, foes as platforms, clockwork, momentum.
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-mooncat
- [IHZ] Indie Hell Zone, Devilition/Mooncat/Rail Heist/Quibble Race.
  https://indiehellzone.com/2024/10/30/ufo-50-devilition-mooncat-rail-heist-quibble-race/
- [TR] TV Tropes, Mooncat recap (search snippets only): spiked cones
  flipped by a slam and kicked; the yellow egg's sword.
- [TY] TV Tropes, "YMMV / UFO 50 Game 13 Mooncat" (search snippet only):
  the giant rabbit, the easiest boss, five platforms, a ranged fire blast.
- [SPD] speedrun.com guide "Mooncat Gold Boss Quick Kill" (title and a
  search summary): hits need only downward movement.
- [SRC] speedrun.com UFO 50 records for Mooncat's categories (warpless,
  green, yellow and red egg, cherry).
  https://www.speedrun.com/api/v1/leaderboards/v1pl7876/category/jdzzxwvd
- [MC] itch.io, "...and the mooncats" (the jam game Mooncat grew from): a
  simple sidescroller. https://aarkipel.itch.io/and-the-mooncats
- [SI] Steam thread "Mooncat intro" and [TG] TheGamer's article: the
  orange creature, the pit, the pink one (via the research bible).
- The research bible `13-mooncat.md` and the review `13-review.md` (UFO 40
  notes) collect these.

## Progress

- Built: `src/games/duskling/` (game, world, rooms, art, audio), slot 13.
  The room sheet, its painter and the route-test writer are in the UFO 40
  notes (`tools13/`).
- Tests: `dk_01`-`dk_05` the controls (two sides, jumps, the low hop, the
  roll and sprint with its longer coyote time, the somersault, the slam's
  pause and bounce, pink ledges); `dk_06` one touch and unlimited lives;
  `dk_07` slams on foes, spiked prickles, flipping and kicking; `dk_08`
  near-miss thistledown, the crow, frogs, gulpers and beetles; `dk_09`
  eyes; `dk_10` faces; `dk_11` springs; `dk_12` water; `dk_13` warps and
  the Beacon; `dk_14` eggs, the Saucer and the Alien; `dk_15` no mid-run
  save; `dk_16` the dayling's walk; `dk_17` the bosses; `dk_18` two
  players; `dk_19` RETRY ROOM; `dk_20` the room checks and secrets needed;
  `dk_21` the opening from the title; `dk_22` pad A and B; `dk_23` the
  Hermit running off; `dk_24` mushrooms; `dk_25` rolling eyes; `dk_26`
  head bounces in a row. `dk_r01`-`dk_r41` cross every room from its way in
  with plain button presses, and take every warp.
- Media: `docs/shots/duskling.gif` and stills, from
  `tools/shots/13_duskling.ufs` (it replays the route tests).
