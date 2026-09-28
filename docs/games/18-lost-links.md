# 18 · LOST LINKS

*Internal design document. Not shown in the product.*

## Tribute to

**Golfaria** (UFO 50 game #18, Mossmouth). The rules were researched from
text only: the community wiki, Steam guides and threads, an FAQ page, the
TV Tropes recap and written reviews (listed under Sources; the full notes
are in the research folder, `18-golfaria.md`, and the independent review in
`ufo40-review/18-review.md`). No UFO 50 images, video, maps, sprites, music
or text were used, and nothing was read from an installed copy of the game.
The 100% checklist guide [LLAMA] was used for rules and counts only (and its
quotes of the in-game pickup texts); its maps were not.

**Map type: fixed.** Golfaria is one hand-made open world, so ours is too:
two hand-drawn layers in `tools/lostlinks/over.txt` and `under.txt` (turned
into `lostlinks_world.c` by `mkworld.sh`; `setseg.pl` and `setmap.pl` edit
them). Only the structure follows the original, as the text sources describe
it: an open overworld over an underground reached through holes, the same
number of each thing, every flagpole, club and tee piece underground, the
start in a cave right under the town, the same gating by abilities, the four
corners the abilities sit in and the order the FAQ gives for them, the
temple-like ruin in the north-west with the gift and the final fight under
it, a desert in the east, islands in the south-west and water to the
north-east, a hidden village of humans. Where each place sits, how the places
join, every tile and every name are ours.

| Full scale | Golfaria | LOST LINKS |
|---|---|---|
| The world | an open overworld; holes are doors to the underground, a "backlayer" under the main world [TOS], [LIZ] | two layers of 160 × 72 tiles (48 screens each), the underground right under the links; 31 holes, each a door both ways at the same spot |
| Places | 43 named places on the wiki [MH] | 53 named zones: 27 up top, 26 below |
| The start | a cave directly under Putterton, the town [QB] | the Cradle, right under Lanternby |
| Flagpoles | 10, all underground, by holes; three need the Block Buster [MH] | 10 pins, all underground by holes; three need the Hammerhead (the Crypt, the Sand Cellar, the Sluice) |
| Golf clubs | 20, only underground, +3 strokes each, 20 to 80 [LLAMA], [MH], [QB] | 20 irons, all underground, +3 each, 20 to 80 |
| Abilities | 4: Brakes, Block Buster, Sand Roll, Water Roll [MH] | 4: Backspin, the Hammerhead, the Dune Tread, the Skipper |
| Holy Tee | 4 pieces, all underground (one needs the Block Buster, one nothing, two the Water Roll); the slot is underground in the temple area [LLAMA], [MH], [GF4] | 4 pieces of the Star Pin, gated the same way; the altar is in the Crypt under the hall |
| Parbots | 10, mostly above ground, one below [MH] | 10 scorecrows, nine above, one below |
| Balls to rescue | 8; three need the Water Roll, one of them the Block Buster too [MH] | 8 strays; three need the Skipper, one of them the Hammerhead too |
| Folk | about 20 golf balls, a signpost that counts, and a hidden village of about 8 humans [MH] | 20 golf balls, the signpost in Lanternby, the Keepers' Refuge (8 Keepers) behind a secret way |
| Birds | birdies +5, eagles +10 [LLAMA], [IGGY] | larks (in bushes) +5, albatrosses +10 |
| Enemies | bogeys that knock you about, mosquitoes that suck strokes [LIZ], [STATIC], [GF1], [QB] | slicers that burst out of the ground and shove, sippers that suck a stroke |
| Terrain | sand, water, orange blocks, bushes, pits and chasms, jumping flowers, bridges, divots, slopes [MH], [LIZ] | the same |
| The finale | a puzzle, then the boss; no refill; friends refill low strokes [IGGY] | the sanctum's four plates, then the Brass Badger; the strays help |
| The boss | a giant cyborg gopher, 5-6 hits to the face [TVT], [IGGY] | the Brass Badger, 6 hits |
| The ending | one screen: the bogeys stop, the surface is safe; a completion % [TVT], [GF4] | one screen, the same news, and a completion % |
| Secrets | a crashed ship with a meditating ball; a discoloured tree [MH] | a crashed saucer with a ball trying to look like a rock; the odd tree |
| Goals | 3 [MH] | the same 3 |
| Length | a long adventure; the first hours are the hardest [LIZ], [POPCAR], [TVT] | the demo player's perfect game is about 330 strokes (12 minutes of play); people take far longer |

## The world

Eight zones across and six down on each layer; north at the top. The
underground lies directly under the links, so a hole always comes out
below (or above) where it was.

| The links | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| 1 | Clubhouse Moat | Clubhouse Hall | Crag West | Highcrown | Highcrown | Crag East | Heron Fen | Heron Fen |
| 2 | Clubhouse Gardens | Clubhouse Gate | the Stone Ring | Upper Lawn | Upper Lawn | Crag Slopes | Heron Fen | Heron Fen |
| 3 | Brackenwood | Brackenwood Path | West Meadow | the Commons | the Commons | East Lawn | Fen Shore | Heron Fen |
| 4 | Brackenwood | Brackenwood Edge | West Meadow | Cottage Row | Lower Lawn | East Lawn | the Sandsea | the Sandsea |
| 5 | Salt Isles | Salt Isles | the Brambles | Mill Lawn | Lanternby | the Sandsea | Hawthorn Hall | the Sandsea |
| 6 | Salt Isles | Salt Isles | the Beach | South Shore | South Shore | the Sandsea | the Sandsea | the Sandsea |

| Below | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| 1 | Badger's Den | the Sanctum (and the Crypt) | the Crypt | Crown Vaults | Crown Vaults | rock | Tide Notch | Pearl Grotto |
| 2 | Clubhouse Cellars | Clubhouse Cellars | Range Well | Echo Hall | rock | rock | Spring Hollow | the Sluice |
| 3 | Bracken Roots | Rootcrawl | rock | the Undercroft | the Undercroft | the Loopway | the Bog Drain | the Sluice |
| 4 | rock | Bracken Roots | Knobble's Nook | Burrowton | Keepers' Refuge | rock | rock | rock |
| 5 | the Crab Hole | rock | rock | rock | the Cradle | the Sand Cellar | the Grit Vault | Shale Pocket |
| 6 | the Still Pool | the Still Pool | Brine Well | the Scrape | the Scrape | rock | Hawthorn Cellars | the Funnel |

- **The start:** Dimple wakes in the Cradle, a safe zone with a pin and a
  hole straight up into Lanternby, the town, where the first folk and the
  signpost are (as Golfaria starts in a cave under Putterton).
- **Most of the world is open from the start** (the FAQ sends you to "the 4
  corners"): with no ability at all, 15 of the 20 irons, all 10 scorecrows,
  5 of the 8 strays, 6 of the 10 pins, all the folk and the Keepers' Refuge
  can be reached (`ln_11_reach`).
- **The four corners** hold the four abilities, in the FAQ's order [QB]:
  the **Hammerhead** at the end of the Clubhouse Cellars in the north-west
  (the gift, like the Block Buster at the end of the Tee Temple path);
  **Backspin** in the Still Pool in the far south-west, reached by chipping
  island to island (as Brakes sit early in the south-west and are "very
  difficult to get without any of the stroke upgrades" [AGON]); the **Dune
  Tread** in the Hawthorn Cellars in the south-east, past the Sand Cellar's
  cracked stone (as Sand Roll is near Mulligan Manse); the **Skipper** at the
  end of the eastern chain: the Bog Drain, six rows of pits only a jump off
  sand clears, Spring Hollow, the Sluice's bridge of cracked stone, and Pearl
  Grotto (as Water Roll ends a far-east underground chain).
- **The Star Pin:** in the Grit Vault (no ability), behind the Crown Vaults'
  cracked stone (the Hammerhead), in the Tide Notch and in a pocket of the
  Badger's Den reached through a hole on the Clubhouse moat's isle (both the
  Skipper).
- **The strays:** the Sluice's, the Brine Well's and the Crab Hole's each sit
  across water (the Skipper); the Crab Hole's behind cracked stone too.
- **The finale** sits under the Clubhouse: a hole in the hall (behind the
  cracked stone) drops into the Crypt with its pin, an iron and the altar;
  the altar opens the gate west into the sanctum; the plates open the door to
  the Badger's Den.
- **Secrets:** the Keepers' Refuge under the Undercroft, behind a stretch of
  rock the ball rolls right through (check mode shows it as flat ground);
  pitfalls hidden under bushes in Brackenwood; holes hidden in bushes; the
  hill by the Stone Ring, reached only by a jumping flower.

## The three goals

| UFO 40 goal | LOST LINKS | Golfaria's goal [MH], [QB] |
|---|---|---|
| Beacon | find the Hammerhead | gift: obtain the Block Buster |
| Saucer | beat the Brass Badger | gold: beat the game |
| Alien | beat it with everything found: 20 irons, 8 strays, 4 abilities, 4 pieces, 10 scorecrows | cherry: win with 100 % completion (all clubs, balls, abilities, tee pieces and parbots) |

Like most cherries, the Alien is checked at the ending.

## Mechanics checklist

| Mechanic | How LOST LINKS does it | Source | Test |
|---|---|---|---|
| A golf ball's adventure | top-down; every move is a stroke | [MH], [LIZ], [MANUALS] | ln_02 |
| Aiming | the d-pad aims (64 steps); a line of dots shows the way the ball will go, bending with the slopes | [MANUALS], [MH], [GF1] | ln_02 |
| Charging | hold A: the dots light up along the line to show the power (twelve), full in about 1.2 s, then back down, slower the longer A is held; let go to hit | [MANUALS], [MH], [QB] | ln_02 |
| Cancel | B cancels a swing | [MH] | ln_02 |
| Check mode | B held: the d-pad looks round, further up top than underground; the middle of the view shows the ground there: an arrow down a slope, a red square for a wall, a yellow circle for sand, a white circle for flat ground or a pit | [MANUALS], [LLAMA], [MH] | ln_02 |
| Only along the ground | a stroke rolls (a putt); only from sand, a divot or a hole does the ball leave the ground (a chip), shown as an arc with a circle where it lands | [LIZ], [MANUALS], [QB] | ln_03, ln_04 |
| Strokes | the counter top left, now / most; every stroke costs one, chips out of sand and divots too; leaving a hole is free; the upgrades show below it, and a mystery on the right (the Star Pin's pieces) | [MANUALS], [MH], [QB] | ln_02, ln_03, ln_04 |
| 20 to start | +3 for each of 20 irons, 80 at most; an iron refills too | [QB], [MH], [LLAMA] | ln_05 |
| Out of strokes | the run ends and Dimple wakes at the last pin touched with full strokes; everything found is kept (saved at once) | [QB], [IGGY], [STATIC] | ln_05 |
| Pins | underground by holes; touching one makes it the checkpoint and refills the strokes | [MH], [QB] | ln_05 |
| Safe zones | the cave round each pin: dropping in, and every stroke taken there, keeps the strokes at 15 or more; the starting cave is one | [MH], [LLAMA], [QB] | ln_05 |
| Found things refill | an ability, a piece, a stray or an iron refills the strokes | [QB], [LLAMA] | ln_05, ln_13 |
| Slopes | a ball runs down them; one never rests on a fairway slope; long ones can't be climbed | [MH], [STATIC], [LIZ] | ln_03 |
| Sand | stops a ball dead; only a chip gets out | [MH], [GF1], [QB] | ln_03 |
| Water | a ball in it goes back to where the stroke was hit from, no extra cost | [QB], [GF1] | ln_04 |
| Pits | a rolling ball falls in and goes back to where it was hit from; a ball in the air crosses; some hide under bushes | [MH], [QB], [IGGY], [MANUALS] | ln_08 |
| Jumping flowers | throw a rolling ball into the air, over pits | [MH] | ln_08 |
| Bridges | planks over water; one is blocked by cracked stone (the Sluice's) | [MH] | ln_11, ln_12 |
| Holes | a slow ball drops in and comes out on the other layer, in the cup; underground each hole is a shining circle back up; a fast ball rattles round the rim and rolls on | [MANUALS], [TOS], [LIZ], [MH] | ln_04 |
| Divots | a slow ball settles in one; out is a chip | [QB] | ln_03 |
| Bushes | the ball crashes through them; some hide holes, some pits | [MH] | ln_08 |
| Birds | larks (hidden in bushes) +5, albatrosses +10 when rolled into | [LLAMA], [IGGY], [GF5] | ln_07 |
| Respawns | birds, bugs and scorecrows that flew off are back whenever the ball changes layer (and each run) | [LLAMA], [IGGY], [MH] | ln_07 |
| Bogeys (our slicers) | burst out of the ground by a ball at rest and shove it; knocked into water or a hole they are gone, worth 5 strokes | [LIZ], [GF1], [MH] | ln_07 |
| Mosquitoes (our sippers) | drift at the ball and suck a stroke out of it, mostly underground; knocked into water or a hole, gone | [STATIC], [QB], [GF1] | ln_07 |
| Parbots (our scorecrows) | seeing Dimple, one counts down five strokes and flies off; hit first, it is smashed for 3 strokes plus one for each stroke it had left, and plays the next tape of the story (in the order they fall, not by crow) | [MH], [QB], [IGGY] | ln_06 |
| The Block Buster (our Hammerhead) | the ball crashes through cracked orange blocks, as through bushes | [MH], [LLAMA] | ln_08 |
| Brakes (our Backspin) | hold A while rolling to slow the ball "for a little while"; a slope wears it out three times as fast | [LLAMA], [MH] | ln_08 |
| Sand Roll (our Dune Tread) | sand rolls like grass; tap B while rolling over sand to jump | [LLAMA], [MH], [TVT] | ln_03, ln_08 |
| Water Roll (our Skipper) | rolling onto water skims it; no brakes on it; landing in it still sinks | [MH] | ln_04 |
| Talking | everything is done by rolling into it; golf balls give advice, one or two lines that repeat | [MANUALS], [MH] | ln_13 |
| The signpost | in the town, counts the strays found and the scorecrows smashed | [MH] | ln_13 |
| The hidden village | eight Keepers, one of them Hazel's son, behind a secret way | [MH], [TVT] | ln_11, ln_13 |
| Strays | roll into one and it joins; in the finale each helps once when strokes run low | [MH], [LLAMA], [IGGY] | ln_13, ln_10 |
| The Holy Tee (our Star Pin) | four pieces, set in the altar underground under the ruined clubhouse, open the way to the finale | [MH], [QB], [GF4] | ln_10 |
| The finale | a puzzle first (four plates: behind cracked stone, across water, across pits, at the end of a slope), then the boss; no pin, no refill | [IGGY] | ln_10 |
| The boss | starts in the middle; hit it in the face and it stands (can't be hit) and fires at a crosshair on the ball; then it moves to a different burrow, and the ball has six strokes to hit it again; 6 hits; birds in the arena (two easy, one across a pit) | [IGGY], [TVT] | ln_10 |
| The ending | one screen: the Badger falls silent, every bug stops, the links are safe; the completion % | [TVT], [GF4] | ln_10 |
| No map | no in-game map; looking round and the signpost are the only help | [GF4], [MH] | |
| Goals | Beacon: find the Hammerhead; Saucer: beat the Badger; Alien: win with everything | [MH], [QB] | ln_09, ln_10, ln_12 |
| Secrets | the crashed saucer and its still ball; the odd tree and its carving | [MH] | ln_13 |
| Saving | progress saved the moment anything is found; a new session starts from the last pin | [IGGY], [STATIC] | ln_09 |

### Readings we had to choose

- **Numbers no source gives.** A full stroke runs 18 tiles on the fairway
  and the lightest one tile, in twelve steps. Rough grips about twice as hard
  as the fairway and the green a little over half as hard; a slope pulls a
  little harder than the fairway grips (so a ball rests on a slope only in
  the rough), and more than seven tiles of fairway slope can't be climbed. A
  chip flies 17 to 94 pixels (one to six tiles); a jump off sand is in the
  air 26 frames, and at speed clears the six rows of pits a chip can't; a
  jumping flower throws the ball up for 24 frames. The power meter fills in
  70 frames at first and slows to 120 (the FAQ's "count to two"). A hole
  takes a ball slower than 3.2 pixels a frame.
- **Backspin:** held, it takes 0.25 pixels a frame off the ball's speed for
  50 frames a stroke; on a slope each frame costs three.
- **Starting strokes:** 20, as the FAQ, two reviews and a Steam thread say;
  the wiki says 15, but its own 80 at most only adds up from 20.
- **Safe zones** keep the strokes at 15 (the wiki's figure; the FAQ says to
  the most), in the cave within six tiles of a pin.
- **The aim:** the wiki says the d-pad "moves the directional aim" and the
  manual guide that it lets you "aim where you will move". Ours swings the
  aim towards the way pressed, a notch a tap, faster when held.
- **The power dots:** twelve, 7 pixels apart along a full stroke's path.
- **Scorecrows:** they see Dimple from seven tiles; the reward is 3 plus
  the strokes left on the count (4 to 8).
- **Refills:** a bug put in water or a hole +5; a stray in the finale +8
  when the strokes are down to two.
- **Bugs:** a slicer bursts out within five tiles of a ball at rest (not one
  in a cup), runs at it and shoves it; a sipper drifts at a ball within four
  tiles and sucks one stroke, then goes home for a while.
- **The Badger:** 6 hits; it stands for two thirds of a second, then fires
  three volleys, each a crosshair on where the ball is that lands 2.5 s
  later (3 strokes if the ball is still inside); then it digs to one of
  four other burrows. If six strokes pass without a hit it fires again.
- **The sanctum:** four plates, each lit by rolling over it.
- **Completion %:** irons, strays, scorecrows, abilities, pieces and the
  Badger, 47 things.
- **Water:** a ball that stops on the water with the Skipper sinks too.
- **Bushes and blocks** grow back each run; a ball flying over a bush leaves
  it be.

## What is ours

- **Name:** LOST LINKS (1985, Beamdown Softworks).
- **Hero:** Dimple, a golf ball with a spark in it, the last Warden to wake.
- **Story:** the Keepers who mowed the links went underground when their
  mowers turned into slicers and sippers; they made the Wardens, rolling
  balls with a spark; Hazel, the last Keeper up top, gave her spark to
  Dimple; the Star Pin was broken in four to keep the Brass Badger's door
  shut. The ten scorecrow tapes tell it, and the Keepers tell their side.
- **Characters:** the strays Sprocket, Clover, Drift, Rivet, Barnacle, Grit,
  Pebble and Ace, each with its own situation and line; twenty golf balls
  (the Lookout, the Stargazer, the Lamplighter, the Mayor, Old Knobble, the
  twins and the rest); the eight Keepers (Elder Rowan, Maple, Ash, Willow,
  Birch, Oak, Holly and Elm).
- **Creatures:** slicers, sippers, larks, albatrosses, scorecrows, the Brass
  Badger.
- **Things:** irons, the Hammerhead, Backspin, the Dune Tread, the Skipper,
  the Star Pin, pins, the altar, the sanctum plates and the signpost.
- **The world:** both layers, all 53 places, every tile.
- **Names:** our own. An earlier draft had place names that matched or
  nearly matched Golfaria's (the four greens, the rough, Underputt, Sandy
  Subterra, the Ocean Pit and others) and stray lines that followed
  Golfaria's rescue lines one by one; after the review all of them were
  renamed and rewritten.
- **Music:** "Lost Links", "Fairway Breeze", "Under the Links", "Bunker
  Heat", "Heron Fen", "The Old Clubhouse", "Brass Badger" and "Green
  Again", and three jingles.
- **Words:** every line, the tapes, the intro and the ending.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu, the save (a
title screen with CONTINUE and NEW GAME, which asks first) and the three
UFO 40 goals, which are Golfaria's own (gift, gold, cherry). Golfaria's
terminal cheat FORE-EVER (endless strokes, nothing saved) is a UFO 50
collection feature and UFO 40 has no terminal, so it is left out.

## Controls

| Input | Action | Source |
|---|---|---|
| D-pad | aim (it swings towards the way pressed) | [MANUALS], [MH] (how it swings: **ours**) |
| Hold A | charge a putt or a chip; let go to hit | [MANUALS], [MH] |
| B (while charging) | cancel the swing | [MH] |
| Hold B (ball at rest) | check mode: the d-pad looks round | [MANUALS], [MH], [GF1] |
| Hold A (while rolling) | brake, with Backspin | [LLAMA] (the in-game text) |
| Tap B (rolling over sand) | jump, with the Dune Tread | [LLAMA] (the in-game text) |
| D-pad (ball that can't settle) | nudge it | [IGGY] |
| START | pause | UFO 40 |
| Title | UP/DOWN and A: CONTINUE or NEW GAME; B back to the library | UFO 40 |

## Not confirmed

- Every number above: distances, grip, slopes, chips, jumps, the meter,
  the brake's budget, the rewards, the ranges, the Badger's timings.
- How the aim is swung with the d-pad.
- What the finale's puzzle is (a source says only that there is one); our
  plates are ours.
- What the boss does when the six strokes run out (we have it fire again),
  and how its shots hurt.
- Exactly how the two kinds of enemy behave (we read "shot-sucking
  mosquitos", "knocks your ball into the water" and "pops up out of
  nowhere").
- Whether the Holy Tee has 4 pieces (the wiki, the FAQ and the checklist) or
  3 (English Wikipedia); we use 4.

## Tests

`tests/ln_01` … `ln_13`, driven by button presses (cheats only put the
ball somewhere or set up a save):

- `ln_01_world`: the counts above, everything that should be underground is,
  the start is under the town, and every hole has a hole on the other layer.
- `ln_02_aim_power`, `ln_03_ground`, `ln_04_water_holes`: the swing, the
  meter, check mode's reach, the ground, sand, water, the Skipper, holes and
  the free chip.
- `ln_05_pins_strokes`: pins, safe zones, running out, irons.
- `ln_06_scorecrows`, `ln_07_birds_slicers`, `ln_08_abilities`,
  `ln_13_folk`: everything in the world: the tapes' order, birds, both bugs,
  respawns on a layer change, the abilities, pits, flowers, the folk, the
  signpost and the Keepers.
- `ln_09_goals_save`, `ln_10_finale`: the goals, the save, the altar, the
  plates, the Badger, the strays' help, winning, the completion %.
- `ln_11_reach`: what each ability opens, checked on the whole world with
  the flow the demo player steers by (`lostlinks_probe.c`): the Hammerhead
  and Backspin need nothing, the Tread needs the Hammerhead, the Skipper
  both, and so on for every pin, piece and stray.
- `ln_12_route`: the route proof. From a new game the demo player (the
  `bot` query) plays the whole game by real presses (d-pad to aim, A held
  and let go at its power, A held again to brake, B on sand to jump),
  choosing each stroke by trying every swing with the game's own rolling
  rules, and gets every iron, stray, scorecrow, ability and piece, the
  altar, the plates and the Badger: all three goals in about 330 strokes.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Golfaria" (raw page): controls, strokes,
  flagpoles underground and safe zones, the four abilities and their
  wording, 20 clubs, the Holy Tee, parbots and the tapes' order, the folk
  and the signpost, the hidden human village, birds and bogeys, bushes,
  pits, jumping flowers, bridges, hidden holes, the goals, the crashed ship
  and the discoloured tree, the cheat. https://ufo50.miraheze.org/wiki/Golfaria
- [LLAMA] Steam guide "Golfaria Guide - Complete Maps & 100% Checklist" (rules
  and counts only, not its maps): the in-game texts for Brakes ("Hold primary
  button to slow your roll for a little while") and Sand Roll ("Tap secondary
  button to jump while rolling over sand!"), birdies +5 and eagles +10,
  respawns on a layer change, clubs only underground, safe zones not
  reducing strokes, free look reaching further up top.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3370346609
- [QB] The Question Block, "Golfaria FAQ and Map" (the FAQ text only): 20
  strokes, what refills them, parbots in 5 strokes, out of strokes back to
  the flag, water and holes resetting the shot, chips out of divots and sand
  cost a stroke but leaving holes doesn't, the start under Putterton,
  counting to two for the meter, the four corners counter-clockwise from the
  north-west. https://thequestionblock.com/golfaria-map/
- [IGGY] Steam guide "Iggy's compiled notes while cherrying every game in
  UFO50", Golfaria: progress saved at once, clubs +3 and a refill, bird bots
  in five turns, respawns after a hole, popping a ball that can't settle,
  the two-part finale, no refill, friends giving strokes back, the gopher's
  stand, crosshair, six shots and 5-6 hits, two free eagles.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3345726476
- [MANUALS] Steam guide "The missing manuals - How to play UFO 50 games",
  "18. Golfaria": the d-pad aims; holding A charges a putt, a line of dots
  whose lighting shows the strength, lighting up slower and slower; a chip
  shows an arc and a landing circle; holding B is "check" mode with its
  ground indicator; the counter top left, the upgrades below it, a mystery
  on the right; everything done by rolling into it; holes and the shining
  circles back up; flagpoles; golf balls with advice.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [TVT] TV Tropes recap: the jump out of bunkers, the cyborg gopher, the
  human village, the one-screen ending.
  https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game18Golfaria
- [GF1] Steam thread "Golfaria": hold X to look, 20 strokes, the aiming
  line and slopes, water sends the ball back, things that knock balls into
  water, mosquitoes, FORE-EVER.
  https://steamcommunity.com/app/1147860/discussions/0/4849904176638182477/
- [AGON] Steam thread "Golfaria is AGONIZING!": the brakes early in the
  south-west, hard without stroke upgrades.
  https://steamcommunity.com/app/1147860/discussions/0/4625854155553010736/
- [GF4] Steam thread on Golfaria's missing map and completion %.
  https://steamcommunity.com/app/1147860/discussions/0/4852155320350210340/
- [GF5] Steam thread: eagles +10, most holes with a +3 upgrade.
  https://steamcommunity.com/app/1147860/discussions/0/4849904176634467320/
- [LIZ] Lizstar's Trashcan, UFO 50 #18: only chips leave the ground, 20
  strokes, "horrible bugs called bogies", the brakes, slopes and the look view.
  https://lizstar64.github.io/reviews/2024/10/13/UFO50-18.html
- [TOS] Thoughts on Series, "Thoughts on Golfaria": holes join the
  overworld and the underworld, the backlayer, the boss plays differently.
  https://thoughtsonseries.substack.com/p/thoughts-on-golfaria-ufo-50
- [STATIC] Static Canvas, "The UFO 50 Diaries: Golfaria": aim, power,
  commit; shot-sucking mosquitos underground; runs start from the checkpoint.
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-golfaria
- [POPCAR] Popcar's Blog, "Reviewing Every Single UFO 50 Game".
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
