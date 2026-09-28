# 18 · LOST LINKS

*Internal design document. Not shown in the product.*

## Tribute to

**Golfaria** (UFO 50 game #18, Mossmouth). The rules were researched from
text only: the community wiki, Steam guides and threads, an FAQ page and
written reviews (listed under Sources; the full notes are in the research
folder, `18-golfaria.md`). No UFO 50 images, video, maps, sprites, music or
text were used, and nothing was read from an installed copy of the game.

**Map type: fixed.** Golfaria is one hand-made open world, so ours is too:
two hand-drawn layers in `tools/lostlinks/over.txt` and `under.txt` (turned
into `lostlinks_world.c` by `mkworld.sh`). Only the structure follows the
original, as the text sources describe it: an open overworld over an
underground reached through holes, the same number of each thing, the same
gating by abilities, the four corners the abilities sit in and the order the
FAQ gives for them, the temple-like ruin in the north-west with the gift, the
altar and the final fight under it, the desert in the east, the islands in
the south-west and the water to the north-east. Where each place sits, how
the places join, every tile and every name are ours.

| Full scale | Golfaria | LOST LINKS |
|---|---|---|
| The world | an open overworld; holes are doors to the underground, a "backlayer" under the main world [TOS], [LIZ] | two layers of 160 × 72 tiles (48 screens each), the underground right under the links; 30 holes, each a door both ways at the same spot |
| Places | 43 named places on the wiki [MH] | 53 named zones: 27 up top, 26 below |
| Flagpoles (checkpoints) | 10; three need the Block Buster [MH] | 10 pins; three need the Hammerhead (the Clubhouse Hall, the Under-Bunker, the Drainway) |
| Golf clubs | 20, +3 strokes each, 20 to 80 [MH], [QB] | 20 irons, +3 each, 20 to 80 |
| Abilities | 4: Brakes, Block Buster, Sand Roll, Water Roll [MH] | 4: Backspin, the Hammerhead, the Dune Tread, the Skipper |
| Holy Tee pieces | 4: one needs the Block Buster, one nothing, two the Water Roll [MH] | 4 pieces of the Star Pin, gated the same way |
| Parbots | 10, mostly above ground, one in Underputt [MH] | 10 scorecrows, nine above ground and one below (in Underputt) |
| Balls to rescue | 8 [MH] | 8 strays |
| Static folk | 11, one or two lines each [MH] | 11 |
| Stroke refills | flagpoles, safe zones, hawks, birdies, bogeys pushed into holes or water, parbots, items [MH], [QB] | pins, safe holes, larks (hidden in bushes) and albatrosses, slicers knocked into water or holes, scorecrows, everything found |
| Hazards | sand, water, orange blocks, bushes, divots, slopes, bogeys [MH], [ST-GF1], [LIZ] | the same, plus low rails the Tread hops |
| The finale | a puzzle, then the boss; no refill; friends refill low strokes [IGGY] | the sanctum's four plates, then the Brass Badger; the strays help |
| The boss | a giant cyborg gopher, 5-6 hits to the face [SEARCH], [IGGY] | the Brass Badger, 6 hits |
| Secrets | a crashed ship with a meditating ball; a discoloured tree [MH] | a crashed saucer with a ball trying to look like a rock; the odd tree |
| Goals | 3 [MH] | the same 3 |
| Length | a long adventure; the first hours are the hardest [LIZ], [POPCAR] | the demo player's perfect game is 300 strokes (10 minutes of play); people take far longer |

## The world

Eight zones across and six down on each layer; north at the top. The
underground lies directly under the links, so a hole always comes out
below (or above) where it was.

| The links | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| 1 | Clubhouse Moat | Clubhouse Hall | Crag West | Mashie Peak | Mashie Peak | Crag East | Heron Fen | Heron Fen |
| 2 | Clubhouse Gardens | Clubhouse Gate | the Old Range | North Green | North Green | Crag Slopes | Heron Fen | Heron Fen |
| 3 | Rootwood | Rootwood Trail | West Green | Middle Greens | Middle Greens | East Green | Fen Shore | Heron Fen |
| 4 | Rootwood | Rootwood Edge | West Green | Teeton | South Greens | East Green | the Dunes | the Dunes |
| 5 | Salt Isles | Salt Isles | South Rough | South Links | South Links | the Dunes | Gimme Manor | the Dunes |
| 6 | Salt Isles | Salt Isles | the Beach | South Shore | South Shore | the Dunes | the Dunes | the Dunes |

| Below | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| 1 | Badger's Den | the Sanctum | Mashie Depths | Mashie Depths | Mashie Depths | rock | Water-Wedge | Pearl Grotto |
| 2 | Clubhouse Cellars | Clubhouse Cellars | rock | Hush Cavern | rock | rock | Gurgle Grotto | the Drainway |
| 3 | Rootwood Roots | the Wormway | rock | Underputt | Underputt | the Loopway | Soggy Sump | the Drainway |
| 4 | rock | Rootwood Roots | Sage's Burrow | Burrowton | rock | rock | rock | rock |
| 5 | the Cove | rock | rock | rock | Waking Hollow | Under-Bunker | Sandy Subterra | Gritty Grotto |
| 6 | Stillwater Sink | Stillwater Sink | the Ocean Pit | Tumble Dip | Tumble Dip | rock | Manor Cellars | the Sinkhole |

- **The start:** Dimple wakes in the Waking Hollow, underground in the
  south, with a pin beside it and a hole up to the South Links (as
  Golfaria starts in the Waking Cave).
- **Most of the world is open from the start** (as in Golfaria, where the
  FAQ sends you to "the 4 corners"): with no ability at all, 15 of the 20
  irons, all 10 scorecrows, 7 of the 8 strays and 6 of the 10 pins can be
  reached (`ln_11_reach`).
- **The four corners** hold the four abilities, in the FAQ's order [QB]:
  the **Hammerhead** at the end of the Clubhouse Cellars in the north-west
  (the gift, like the Block Buster at the end of the Tee Temple path);
  **Backspin** in the Stillwater Sink in the far south-west, reached by
  chipping island to island (as Brakes sit early in the south-west and are
  "very difficult to get without any of the stroke upgrades" [ST-AGON]);
  the **Dune Tread** in the Manor Cellars in the south-east, past the
  Under-Bunker's cracked stone (as Sand Roll is near Mulligan Manse); the
  **Skipper** at the end of the eastern chain (Soggy Sump, Gurgle Grotto,
  the Drainway, Pearl Grotto), which needs the Tread and the Hammerhead (as
  Water Roll is at the end of a far-east underground chain).
- **The Star Pin:** in the Sandy Subterra (no ability), behind the Mashie
  Depths' cracked stone (the Hammerhead), in the Water-Wedge and on the isle
  in the Clubhouse moat in the north-west corner (both the Skipper).
- **The finale** sits under the Clubhouse: the altar in the hall, the
  sanctum under the gate, the Brass Badger's den beyond it.

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
| A golf ball's adventure | top-down; every move is a stroke | [MH], [LIZ] | ln_02 |
| Aiming | the d-pad aims (64 steps); a line of dots shows the way the ball will go, bending with the slopes | [MANUALS], [MH], [ST-GF1] | ln_02 |
| Charging | hold A: the dots light up along the line to show the power (twelve of them), up and down, more slowly the longer A is held; let go to hit | [MANUALS], [MH] | ln_02 |
| Cancel | B cancels a swing | [MH] | ln_02 |
| Check mode | B held: the d-pad looks round the map; the middle of the view shows the ground there: an arrow down a slope, a red square for a wall, a yellow circle for sand, a white circle for flat ground or a pit | [MANUALS], [MH], [LIZ] | ln_02 |
| Only along the ground | a stroke rolls (a putt); only from sand, a divot or a hole does the ball leave the ground (a chip), shown as an arc of dots with a circle where it lands | [LIZ], [MANUALS], [QB] | ln_03, ln_04 |
| Strokes | the counter top left, now / most; every stroke costs one, chips out of sand and divots too; leaving a hole is free; the upgrades found show below it, and a mystery on the right (the Star Pin's pieces) | [MANUALS], [MH], [QB] | ln_02, ln_03, ln_04 |
| 20 to start | +3 for each of 20 irons, 80 at most; an iron refills too | [QB], [MH] | ln_05 |
| Out of strokes | the run ends and Dimple wakes at the last pin touched with full strokes; everything found is kept (saved at once) | [QB], [IGGY], [STATIC] | ln_05 |
| Pins | touching one makes it the checkpoint and refills the strokes | [MH], [QB] | ln_05 |
| Safe zones | a hole with a pin by it tops the strokes up to 15 | [MH] | ln_05 |
| Found things refill | an ability, a piece, a stray or an iron refills the strokes | [QB] | ln_05, ln_13 |
| Slopes | a ball runs down them; one never rests on a fairway slope; long ones can't be climbed | [MH], [STATIC], [LIZ] | ln_03 |
| Sand | stops a ball dead; only a chip gets out | [MH], [ST-GF1], [QB] | ln_03 |
| Water | a ball in it goes back to where the stroke was hit from | [ST-GF1], [SEARCH] | ln_04 |
| Holes | a slow ball drops in and comes out on the other layer, in the cup; underground each hole is a shining circle that takes the ball back up; a fast ball rattles round the rim and rolls on | [MANUALS], [TOS], [LIZ], [MH] | ln_04 |
| Divots | a slow ball settles in one; out is a chip | [QB] | ln_03 |
| Bushes | the ball crashes through them; some hide holes | [MH] | ln_08 |
| Birds | larks (hidden in bushes) +4, albatrosses +8 when rolled into; back next run | [MH], [LIZ] | ln_07 |
| Bogeys (our slicers) | fly at a ball standing still, sting it (a stroke) and shove it; knocked into water or a hole they are gone, worth 5 strokes | [MH], [STATIC], [ST-GF1] | ln_07 |
| Parbots (our scorecrows) | seeing Dimple, one counts down five strokes and flies off (back next run); hit first, it is smashed for 3 strokes plus one for each stroke it had left, and plays a tape of the story | [MH], [QB], [IGGY] | ln_06 |
| The Block Buster (our Hammerhead) | a hard roll smashes cracked orange blocks | [MH] | ln_08 |
| Brakes (our Backspin) | stops the ball dead; a slope wears it down | [MH] | ln_08 |
| Sand Roll (our Dune Tread) | sand rolls like fairway; A again during a roll hops | [MH] | ln_03, ln_08 |
| Water Roll (our Skipper) | rolling onto water skims it; no brakes on it; landing in it still sinks | [MH] | ln_04 |
| Talking | everything is done by rolling into it; other golf balls give advice; one or two lines that repeat | [MANUALS], [MH] | ln_13 |
| Strays | roll into one and it joins; in the finale each helps once when strokes run low | [MH], [SEARCH], [IGGY] | ln_13, ln_10 |
| The Holy Tee (our Star Pin) | four pieces, set in the altar in the ruined clubhouse, open the way to the finale | [MH], [QB] | ln_10 |
| The finale | a puzzle first (four plates, one for each ability's kind of ground), then the boss; no pin, no refill | [IGGY] | ln_10 |
| The boss | starts in the middle; hit it in the face and it stands (can't be hit) and fires at a crosshair on the ball; then it moves to a different burrow, and the ball has six strokes to hit it again; 6 hits; free birds in the arena (two easy, one behind a rail) | [IGGY], [SEARCH] | ln_10 |
| No map | no in-game map; looking round is the only help | [ST-GF4] | |
| Goals | Beacon: find the Hammerhead; Saucer: beat the Badger; Alien: win with everything (20 irons, 8 strays, 4 abilities, 4 pieces, 10 scorecrows) | [MH], [QB] | ln_09, ln_10, ln_12 |
| Secrets | the crashed saucer and its still ball; the odd tree and its carving | [MH] | ln_13 |
| Saving | progress saved the moment anything is found; a new session starts from the last pin | [IGGY], [STATIC] | ln_09 |

### Readings we had to choose

- **Numbers no source gives.** A full stroke runs 18 tiles on the fairway
  and the lightest one tile, in twelve steps. Rough grips about twice as hard
  as the fairway and the green a little over half as hard; a slope pulls a
  little harder than the fairway grips (so a ball rests on a slope only in
  the rough), and more than seven tiles of fairway slope can't be climbed. A
  chip flies 17 to 94 pixels (one to six tiles); a hop is in the air for 22
  frames. The power meter takes about three quarters of a second to fill,
  and each swing slows it until it takes 100 frames. A hole takes a ball
  slower than 3.2 pixels a frame. A block needs a knock of 1.2 pixels a
  frame.
- **Starting strokes:** 20, as the FAQ, two reviews and a Steam thread say;
  the wiki says 15, but its own 80 at most only adds up from 20.
- **Safe zones** top up to 15 as the wiki says (the FAQ says to the most).
- **The aim:** the wiki says the d-pad "moves the directional aim" and the
  manual guide that it lets you "aim where you will move". Ours swings the
  aim towards the way pressed, a notch a tap, faster when held.
- **The power dots:** twelve, 7 pixels apart along a full stroke's path.
- **The hop and the brake:** Sand Roll's jump is "press again during a
  stroke" (A). No source says which button brakes; ours is B while
  rolling, which is free then.
- **What the hop is for:** low rails that only an airborne ball clears
  (the Dune Tread's gate), and short hops over water.
- **Scorecrows:** they see Dimple from seven tiles; the reward is 3 plus
  the strokes left on the count (4 to 8).
- **Refills:** a lark +4, an albatross +8, a slicer put in water or a hole
  +5; a stray in the finale +8 when the strokes are down to two.
- **Slicers** fly at a still ball within six tiles at a slow pace, sting
  it for one stroke and shove it; they only fall in water or holes when
  knocked.
- **The Badger:** 6 hits; it stands for two thirds of a second, then fires
  three volleys, each a crosshair on where the ball is that lands 2.5 s
  later (3 strokes if the ball is still inside); then it digs to one of
  four other burrows. If six strokes pass without a hit it fires again.
- **The sanctum:** four plates, each lit by rolling over it: one behind
  cracked blocks, one across water, one behind rails, one at the end of a
  slope.
- **Water:** a ball that stops on the water with the Skipper sinks too.
- **Bushes** grow back each run; a ball flying over one leaves it be.

## What is ours

- **Name:** LOST LINKS (1985, Beamdown Softworks).
- **Hero:** Dimple, a golf ball with a spark in it, the last Warden to wake.
- **Story:** the Keepers who mowed the links went underground when their
  mowers turned into slicers; they made the Wardens, rolling balls with a
  spark; Hazel, the last Keeper up top, gave her spark to Dimple; the Star
  Pin was broken in four to keep the Brass Badger's door shut. The ten
  scorecrow tapes tell it.
- **Characters:** the strays Sprocket, Clover, Drift, Rivet, Barnacle, Grit,
  Pebble and Ace; the Tired Ball, the Old Ranger, the Mossy Ball, the
  Porter, the Prospector, the Crosser, the Beachcomber, the Oasis Keeper,
  the Thinker, the Sage and the Sandy Ball; the moss folk.
- **Creatures:** slicers, larks, albatrosses, scorecrows, the Brass Badger.
- **Things:** irons, the Hammerhead, Backspin, the Dune Tread, the Skipper,
  the Star Pin, pins, the altar and the sanctum plates.
- **The world:** both layers, all 53 places and their names, every tile.
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
| Hold B (ball at rest) | check mode: the d-pad looks round | [MANUALS], [MH], [ST-GF1] |
| A (while rolling) | hop, with the Dune Tread | [MH] |
| B (while rolling) | brake, with Backspin | **not sourced:** our choice |
| D-pad (ball that can't settle) | nudge it | [IGGY] |
| START | pause | UFO 40 |
| Title | UP/DOWN and A: CONTINUE or NEW GAME; B back to the library | UFO 40 |

## Not confirmed

- Every number above: distances, grip, slopes, chips, hops, the meter,
  the rewards, the ranges, the Badger's timings.
- Which button brakes.
- How the aim is swung with the d-pad.
- What the finale's puzzle is (a source says only that there is one); our
  plates are ours.
- What the boss does when the six strokes run out (we have it fire again),
  and how its shots hurt.
- How bogeys behave (we read "shot-sucking" and "knock you around").
- Whether the Holy Tee has 4 pieces (the wiki and the FAQ) or 3 (English
  Wikipedia); we use 4.

## Tests

`tests/ln_01` … `ln_13`, driven by button presses (cheats only put the
ball somewhere or set up a save):

- `ln_01_world`: the counts above, and every hole has a hole on the other
  layer.
- `ln_02_aim_power`, `ln_03_ground`, `ln_04_water_holes`: the swing, the
  meter, the ground, sand, water, the Skipper, holes and the free chip.
- `ln_05_pins_strokes`: pins, running out, the safe zone, irons.
- `ln_06_scorecrows`, `ln_07_birds_slicers`, `ln_08_abilities`,
  `ln_13_folk`: everything in the world.
- `ln_09_goals_save`, `ln_10_finale`: the goals, the save, the altar, the
  plates, the Badger, the strays' help, winning.
- `ln_11_reach`: what each ability opens, checked on the whole world with
  the flow the demo player steers by (`lostlinks_probe.c`): the Hammerhead
  and Backspin need nothing, the Tread needs the Hammerhead, the Skipper
  both, and so on for every pin and piece.
- `ln_12_route`: the route proof. From a new game the demo player (the
  `bot` query) plays the whole game by real presses (d-pad to aim, A held
  and let go at its power, A to hop, B to brake), choosing each stroke by
  trying every swing with the game's own rolling rules, and gets every
  iron, stray, scorecrow, ability and piece, the altar, the plates and the
  Badger: all three goals in about 300 strokes.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Golfaria" (raw page): controls, strokes,
  flagpoles and safe zones, the four abilities and their wording, 20 clubs,
  the Holy Tee, parbots, rescued and static folk, birds and bogeys, bushes,
  hidden holes, the goals, the crashed ship and the discoloured tree, the
  cheat. https://ufo50.miraheze.org/wiki/Golfaria
- [QB] The Question Block, "Golfaria FAQ and Map" (the FAQ text only): 20
  strokes, what refills them, parbots in 5 strokes, out of strokes back to
  the flag, chips out of divots and sand cost a stroke but leaving holes
  doesn't, the four corners counter-clockwise from the north-west.
  https://thequestionblock.com/golfaria-map/
- [IGGY] Steam guide "Iggy's compiled notes while cherrying every game in
  UFO50", Golfaria: progress saved at once, clubs +3 and a refill, bird bots
  in five turns, popping a ball that can't settle, the two-part finale, no
  refill, friends giving strokes back, the gopher's stand, crosshair, six
  shots and 5-6 hits, two free eagles.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3345726476
- [MANUALS] Steam guide "The missing manuals - How to play UFO 50 games",
  "18. Golfaria": "a golf-based metroidvania"; the d-pad aims; holding A
  charges a putt, a line of dots whose lighting shows the strength, lighting
  up slower and slower the longer A is held; a chip shows an arc of dots and
  a circle where it lands; holding B is "check" mode, with the indicator for
  the ground under the middle of the view; the putts counter top left, the
  upgrades below it, a mystery on the right; everything is done by rolling
  into it; holes to the underground and the shining circles back up;
  flagpoles; other golf balls with advice.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [ST-GF1] Steam thread "Golfaria": hold X to look, 20 strokes, clubs in
  holes, the aiming line and slopes, water sends the ball back, bogeys
  knocking balls into water, FORE-EVER.
  https://steamcommunity.com/app/1147860/discussions/0/4849904176638182477/
- [ST-AGON] Steam thread "Golfaria is AGONIZING!": the brakes early in the
  south-west, hard without stroke upgrades.
  https://steamcommunity.com/app/1147860/discussions/0/4625854155553010736/
- [ST-GF4] Steam thread on Golfaria's missing map.
  https://steamcommunity.com/app/1147860/discussions/0/4852155320350210340/
- [LIZ] Lizstar's Trashcan, UFO 50 #18: only chips leave the ground, 20
  strokes, healing, the brakes, slopes and the look view.
  https://lizstar64.github.io/reviews/2024/10/13/UFO50-18.html
- [TOS] Thoughts on Series, "Thoughts on Golfaria": holes join the
  overworld and the underworld, the backlayer, the boss plays differently.
  https://thoughtsonseries.substack.com/p/thoughts-on-golfaria-ufo-50
- [STATIC] Static Canvas, "The UFO 50 Diaries: Golfaria": aim, power,
  commit; shot-sucking mosquitos; runs start from the checkpoint even
  after switching off. https://staticcanvas.substack.com/p/the-ufo-50-diaries-golfaria
- [POPCAR] Popcar's Blog, "Reviewing Every Single UFO 50 Game".
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [SEARCH] search-engine summaries of the pages above and of the TV Tropes
  recap (blocked): the cyborg gopher, ghost balls giving strokes in the
  final fight, water sending the ball back.
