# 31 · TILTSHOT

*Internal design document. Not shown in the product.*

## Tribute to

**Pingolf** (UFO 50 game #31, Mossmouth, "May 1987"). The rules were
researched from text only: the community wiki's raw and rendered pages,
Steam guides and threads, written reviews and forum posts (listed under
Sources; the full notes are in the research folder, `31-pingolf.md`). No UFO
50 images, video, sprites, music, hole layouts or text were used as
references, and nothing was taken from a UFO 50 install.

**Map type: fixed.** Pingolf's eighteen holes are hand-made and "the same
every time" [LZ], [MH], so ours are too (drawn in `tools/tiltshot/holes.txt`,
made into `tiltshot_maps.c`). Each hole keeps its original's par, its purpose
as the guides describe it and roughly its length; every layout is our own,
drawn from the purpose alone.

| Full scale | Pingolf | TILTSHOT |
|---|---|---|
| Holes | 18, fixed, always in the same order; no hole select, practice or retry [MH], [PC], [ST-PRACTICE] | 18, fixed, in order; no hole select, practice or retry |
| Pars | 3 3 3 4 4 4 3 4 1 · 3 2 4 3 4 4 3 3 6 = 61 [MH] | the same, hole by hole |
| The par-1 hole | hole 9, where an ace is par [MH] | hole 9, TOSS-UP |
| The par-6 hole | hole 18, the long finale [MH] | hole 18, LAST ORBIT (six screens) |
| Hole length | longer than a screen, scrolling sideways [MH], [LZ] | 1.25 to 6 screens (50 to 240 tiles of 8 pixels) |
| Field | 8 golfers: you (and a second player) and CPU golfers [MH] | 8: 1 or 2 players and 7 or 6 CPU rivals |
| Golfers | 5 to choose, 2 colourways each, cosmetic; 2 more by a code [MH], [MH-CHEATS] | 5 (NOVA, DIGBY, PEACHES, TUCK, MOSS), 2 colourways each; WICK and KIP by the code |
| CPU names | not documented [MH] | 12 regulars, 7 (or 6) drawn each tournament |
| Course things | bumpers (large, small), trampolines, ramps, orange movers, purple trash, pegs, tunnels, sand, water, pits [MH], [CG] | the same kinds: big and small bumpers, spring pads and spring lines, slopes, 5 kinds of mover, 4 kinds of junk, pegs, tunnels, sand, water, pits |
| Power-ups | none [MH] | none |
| Goals | 3 [MH], [GG] | the same 3 |
| Stats | Golfers Used, Most Hole-in-Ones; records Speedrun and Lowest Total Score [MH] | RECORDS: lowest total, quickest win, most holes in one, golfers used, classics won, holes in one |
| Length of a tournament | about 10 minutes; speedrun record about 4 [LZ], [MH] | the demo player's clean run takes 3:35 of play |

### The eighteen holes

Purposes are summarised from the guides [CG], [RS-35], [SR-RAMEN], [IGGY];
layouts are ours.

| # | TILTSHOT | Par | The original's hole is for | Ours | Tiles |
|---|---|---|---|---|---|
| 1 | OPENING TEE | 3 | a tutorial: ramps and a bumper; a full shot into a ramp and a dunk can ace | a rise with a bumper above it, a long slope down to a flat green and a stone backstop; slam on the slope and the ball runs on, on fire | 104 |
| 2 | STONE SKIP | 3 | skipping the water at full speed; things flying over it | a lake with two blimps patrolling over it; skip across flat and hard, or stop on the sand of the rock in the middle | 120 |
| 3 | BOUNCE HOUSE | 3 | a bouncy line with blockers; a near-impossible ace | a bed of spring pads between stone posts; the cup on a little peak past a third post, and a pit past the green | 100 |
| 4 | SKYLARK | 4 | lobbing over moving hazards and dunking down; clutter as brakes | a bottomless canyon with two sparks wheeling over it, a mesa to land on (junk on it to stop against), then down to the green | 140 |
| 5 | THE CHUTE | 4 | a dunk onto a ramp for speed; the ball can roll back | a steep drop into a hollow and a long climb out: rolled in, the ball rolls back; slammed onto the drop, it runs up and over | 140 |
| 6 | THREE STOREYS | 4 | several stacked platform routes; dunk to push on | a floor with two pits in it and a middle and a top stone shelf over it; the shelves end short of the green, a pit between: run off the top one fast (a slam along it helps) or chip over from below | 130 |
| 7 | DUNE STEPS | 3 | sand pits; a water bounce can skip the first | dune terraces with sand in the hollows, a pool before the first sand to skim off, sand either side of the cup | 132 |
| 8 | FROST TUNNEL | 4 | an accurate opener into a small tunnel (dunk to drop in); the red block | an ice mountain with a shaft one tile wide in its top that drops into a tunnel out to the green; over the top the ice is quick; the red block sits on the peak | 140 |
| 9 | TOSS-UP | 1 | the only par 1: moving jugglers, "aim and pray" | over a pit into a funnel down to the cup, two hop-bots bobbing across the way | 50 |
| 10 | DOUBLE SPRING | 3 | hit the first bouncy line, dunk into the second | a chasm with two spring lines over it | 110 |
| 11 | PEBBLE | 2 | short; dunk on the first down-ramp; an ace is possible | a hilltop tee over a steep slope, a raised green, a pit past it | 70 |
| 12 | ATOLL | 4 | water islands and fish; stopping the ball; a run-killer | islands with sand patches in open water, three fish leaping between them | 150 |
| 13 | BOULDER | 3 | a tall wall: lob it or dunk over | a stone wall nearly the height of the screen close to the tee; the cup on the flat behind it | 90 |
| 14 | BUMPER ALLEY | 4 | a bouncy line at near full power, then a dunk to dodge a bumper | a spring pad by the tee throws the ball over a pit full of bumpers | 140 |
| 15 | THE HIGH SHELF | 4 | one of the two hardest: full power over clutter into an up-ramp | a fairway strewn with junk, a kicker at its end, the green on a high shelf past a pit | 140 |
| 16 | NERVE | 3 | distance control on lobs; clutter helps the last shot | two turf pillars over pits and a kite between them; junk by the cup stops a ball that would run off | 100 |
| 17 | THE PAGODA | 3 | one of the two hardest: pegs and a sand pit, precise timing | a lattice of pegs over a sand pit before a raised green | 100 |
| 18 | LAST ORBIT | 6 | the long finale: fish, pegs, several sand pits and water | two lakes with fish, sand on the far shore, a forest of pegs over a sand bed, a spring pad, a climb, sand by the green | 240 |

**How they play.** The route finder (`tsh_solve`, a frame-perfect search)
gets round in 27 strokes (-34), a little under the original's record of -29
[MH]; it aces the ace holes (1, 9, 10, 11) and a few others with one exact
shot. A yardstick player that knows every hole but misses its strength by
up to five frames and its slam by up to six (`tsh_steady_play`) averages
about 50 strokes (-11). Ace cells in a full sweep of tee shots are rare on
every hole but the ace chances (`cheat survey N`).

## Mechanics checklist

| Mechanic | How TILTSHOT does it | Source | Test |
|---|---|---|---|
| Side-on, scrolling | tee on the left, cup on the right; the view follows the ball across the hole | [MH], [LZ] | (drawn) |
| Aim | LEFT and RIGHT turn a short dotted guide from the ball | [MM], [MH] | tsh_02 |
| Hold to charge | A held fills the meter; a harder shot goes faster and further | [MM], [MH] | tsh_01, tsh_02 |
| Release to shoot | letting go of A swings at the meter's power | [MM] | tsh_01 |
| Not an oscillating meter | it rises to full and stays there | [MH], [MM] | tsh_01 |
| The warning | a brief time at full, then EASY NOW! and the golfer flashes red | [MM], [MH] | tsh_01 |
| Held too long | the golfer blows up and the shot is cancelled; the ball stays put | [MM], [MH] | tsh_01 |
| The dunk (our slam) | A while the ball is in the air fires it downward, keeping its speed across; once a stroke | [MM], [MH], [LZ], [CG] | tsh_03 |
| Slam onto a down-slope | a big jump in speed and a brief fire | [MH], [PC] | tsh_04 |
| Bouncy ball | the ball bounces a lot and rolls down slopes | [LZ], [MH] | tsh_02, tsh_04 |
| Each stroke from where it stopped | the golfer walks up to the ball | [MM] | tsh_02 |
| HUD | stroke, par and distance on an amber dot-matrix display, with the meter | [MM], [TG], [HANS] | (drawn) |
| Sand | stops nearly everything; the ball rests where it lands | [MH] | tsh_07 |
| Water | the ball sinks and comes back to where it was hit from; the stroke counts, no penalty on top | [MH], [IGGY] | tsh_05 |
| Skipping | a fast ball nearly flat to the water skips across | [MH], [CG] | tsh_05 |
| Pits | back to where it was hit from; the stroke counts, no extra penalty | [MM], [MH] | tsh_05 |
| The cup | the hole counts the moment the ball is in, even if it would bounce out | [MM] | tsh_06 |
| No stroke limit | a hole takes as many strokes as it takes | [ST-PG], [RS-35] | tsh_06 |
| Bumpers | round, fixed in the air, kick the ball away | [MH] | tsh_07 |
| Trampolines | spring pads and spring lines throw the ball harder than bumpers do | [MH], [CG] | tsh_07 |
| Orange movers | move about, break at a touch, and slow the ball drastically | [MH], [CG] | tsh_07 |
| Purple trash | still clutter that breaks at a touch and slows the ball a little; usable as a brake | [MH], [CG], [ST-JUNK] | tsh_07 |
| Pegs | small fixed posts to thread between | [CG] | (hole 17, 18) |
| The red block | hit on hole 8, it shows a strange message | [MH-META] | tsh_16 |
| Stroke play | eight on the board; each golfer's total against par; lowest after 18 wins | [MH], [ST-RNG] | tsh_06, tsh_09 |
| Weak, random CPU | the best CPU finishes between 4 and 13 over par, never at par or under | [ST-RNG], [LZ] | tsh_08 |
| 2P versus | two players in the same tournament; they don't touch each other's ball | [MH], [MH-MP], [ST-2P] | tsh_11 |
| Golfers | cosmetic; two colourways each | [MH] | tsh_12 |
| The code | unlocks two cameo golfers; printed in the credits | [MH-CHEATS], [LZ] | tsh_12 |
| No saving mid-run | a tournament is played from hole 1 to 18; records and goals are kept | [ST-SAVES], [CONV] | tsh_13 |
| Stats | golfers used, most holes in one; lowest total and a quickest time | [MH] | tsh_09, tsh_13 |
| Goals | Beacon: a hole in one; Saucer: win the Comet Classic; Alien: win at par or under | [MH], [GG] | tsh_09, tsh_10, tsh_13 |
| Always finishable | however badly it goes, all 18 holes are played to the end | [STATIC] | tsh_10 |

### Readings we had to choose

- **The blow-up spends a stroke.** The manual says the shot is cancelled,
  the wiki that it is "forfeited entirely"; we count it as a stroke taken
  with the ball left where it was.
- **The meter:** 60 frames to fill; full for 24 frames; then 48 frames of
  warning (EASY NOW!, the golfer flashing red) before the blow-up. A
  release at any point before that swings. A tap is the softest putt.
- **Aim:** 37 settings five degrees apart, from flat right to flat left
  (guides give aims like "45 degrees"); every hole starts at 45 degrees;
  held, LEFT and RIGHT keep turning after a moment.
- **Power and gravity:** 0.7 to 5.8 pixels a frame, gravity 0.1: a full
  shot at 45 degrees carries about a screen and lands running.
- **The slam:** it sets the fall to 5 pixels a frame (plus a little of any
  fall it already had) and leaves the speed across alone; it only works in
  the air (a ball rolling along the ground can't be slammed), and the slam
  isn't spent by a press on the ground.
- **The slope boost:** a slam's first touch on a slope of 1 in 2 or steeper
  that falls away sends the ball down it at 1.45 times its speed along the
  slope plus 2.2, and it burns for a second and a quarter. Fire is only a
  look; it doesn't change what the ball hits.
- **Skipping:** the ball must be going at least 3 pixels a frame across and
  coming down at under about 27 degrees; each skip keeps 85 % of its speed
  across and 70 % of its fall, up to six skips.
- **Bumpers** send the ball off at three quarters of its speed into them
  plus 2.2 (at least 2.8); **spring pads and lines** at 1.05 times plus 1.6
  (the stronger kick), and a ball that just rests on them stays put. After
  15 seconds in play the kicks stop, so nothing bounces forever.
- **Junk** keeps 60 % of the ball's speed and stays broken for the rest of
  the golfer's hole; a **mover** keeps 20 % and is back for the next stroke.
- **The cup** counts once the whole ball is below the rim; the check comes
  before the ball can bounce back out.
- **Out of bounds:** each hole is walled at both ends; the sky is open, and
  a ball above the screen shows as an arrow at the top.
- **CPU rounds** are made when the tournament starts: the leader's total is
  drawn from +4 to +13 and each rival after it 0 to 4 strokes further back;
  each takes 0 to 3 birdies, paid back elsewhere, and its bad holes fall more
  often on long holes, at most 4 over on one hole. The board reveals them a
  hole at a time.
- **Ties** share a place (T2 on the board); a share of first place counts as
  a win.
- **2P:** hole by hole, player 1 plays the hole out, then player 2 (the
  movers start again for each); six CPU rivals. Each player uses their own
  pad (on the Vita, which has one, the pad is passed across). Goals count for
  either player.
- **The code** is entered on the cartridge's own CODE screen (UFO 40 has no
  terminal) and unlocks the two golfers for good; in UFO 50 terminal cheats
  switch off achievements, but this one only adds looks, so the goals stay
  on.
- **Button 1 (B)**, which no source gives a use in play: held while aiming,
  it looks along the hole (LEFT and RIGHT move the view; the aim stays put).
- **The hole card** before each hole shows its name, par and outline.

## What is ours

- **Name:** TILTSHOT (1987, Beamdown Softworks), and its tournament, the
  **Comet Classic**, played every time the comet swings by.
- **Golfers:** NOVA (a girl in a visor), DIGBY (a robot), PEACHES (a
  three-eyed alien), TUCK (a penguin) and MOSS (a frog), two colourways each.
- **The cameos and the code:** WICK (from HOMESPUN, our Pilot Quest) and KIP
  (from SKYWELL), unlocked by **BEAM-DOWN**, which the credits print.
- **The regulars:** ORBO, GLIMMA, GEARBOX, MAVIS, BIG NED, ZIBBO, LADY FEN,
  SPROUT, DR QUILL, BLIX, HONK and MARGO.
- **Courses:** all eighteen holes, their names and seven looks (meadow,
  dusk, night, ice, beach, temple, funfair).
- **Things:** blimps, hop-bots, kites, fish and sparks (the orange movers);
  crates, cones, churns and popcorn buckets (the purple junk); bumpers,
  spring pads, spring lines and pegs.
- **Words:** "slam" for the dunk, EASY NOW!, KA-BOOM!, the red block's line
  ("THEY BUILT IT FOR FORTY. FIFTY CAME DOWN.", a UFO 40 joke), the titles,
  the credits and every label.
- **Music:** "Comet Classic" (title), "Front Nine", "Back Nine",
  "Eighteenth Hole", "The Leaderboard", "Champion of the Comet" and four
  jingles ("In the Cup", "Hole in One", "Over Par", "Runner-Up").

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu, the three UFO 40
goals (Pingolf's own gift, gold and cherry), and saving the records and
goals. The hole card and the look button are readings, above.

## Controls

| Input | Action | Confirmed? |
|---|---|---|
| LEFT / RIGHT | turn the aim | yes [MM] |
| Hold A | fill the meter | yes [MM], [MH] |
| Let go of A | swing | yes [MM] |
| A in the air | slam, once a stroke | yes [MM], [MH], [CG] |
| Hold B, LEFT / RIGHT | look along the hole | **no**: no source gives Button 1 a use in play |
| UP / DOWN | nothing in play (colourway on the golfer screen) | **no**: no source mentions them |
| START | pause | collection convention [CONV] |
| Menus | UP / DOWN choose, A picks, B goes back | collection convention [CONV] |
| 2P on one keyboard | WASD F G and ARROWS K L | UFO 40's own versus keys |

## Not confirmed

- Whether the blow-up costs a stroke (we say yes).
- The meter's speed, how long it stays full, how long the warning lasts.
- The aim's step and range.
- What Button 1 and UP/DOWN do in play.
- How 2P takes turns and how many CPUs play then.
- How the CPU rounds are made, and the CPU names.
- What fire does beyond speed (we say nothing).
- Out of bounds at the top and sides.
- Whether broken junk and movers come back.
- Ties at the top.

## Tests

`tests/tsh_01` … `tsh_16` drive the rules with button presses; cheats only
put the ball somewhere or jump to a hole. `tsh_01` plays from the title
through the meter, the warning and the blow-up; `tsh_09` plays a whole
tournament from the title with the demo player pressing real buttons and
wins it under par (all three goals, the records and the ending); `tsh_10`
loses one; `tsh_11` plays 2P with both pads; `tsh_12` enters the code;
`tsh_14` and `tsh_15` replay a route round every one of the eighteen holes
with real presses, each at or under par. The routes come from the route
finder (`sh tools/tiltshot/mkroutes.sh`, after `make headless`). Design
checks: `cheat survey N` sweeps every tee shot and plays the yardstick
player; `cheat landscape N AIM SLAM` shows where each power ends up.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Pingolf" (rendered and raw): the meter, the
  dunk and the slopes, the golfers, the hazards, the 18 holes and pars, the
  goals, stats and records. https://ufo50.miraheze.org/wiki/Pingolf
- [MH-CHEATS] Miraheze, "Cheats": STAR-BALL (not used).
  https://ufo50.miraheze.org/wiki/Cheats
- [MH-META] Miraheze, "Meta Messages": the red block on hole 8.
  https://ufo50.miraheze.org/wiki/Meta_Messages
- [MH-MP] Miraheze, "Multiplayer". https://ufo50.miraheze.org/wiki/Multiplayer
- [MM] Steam guide "The missing manuals - How to play UFO 50 games",
  Pingolf: aim with the dotted line, hold to charge, the warning and the
  explosion, the dunk, the HUD, the cup rule, pits.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [CG] Steam guide "How To Get Cherry Clear For Pingolf": hole purposes,
  skipping, bouncy lines, purple brakes.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3333659823
- [IGGY] Steam guide "Iggy's compiled notes while cherrying every game":
  no water penalty, angles, the timed ace on hole 9.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3345726476
- [GG] Steam guide "Gift, Gold & Cherry": hole in one, beat the game, win at
  0 or better. https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [ST-RNG] Steam thread "Getting gold for Pingolf might be a bit too RNG":
  the best CPU from +4 to +13.
  https://steamcommunity.com/app/1147860/discussions/0/4849903998514281511/
- [ST-PG] Steam thread "Pingolf": 35 strokes on one hole.
  https://steamcommunity.com/app/1147860/discussions/0/4638240281918966069/
- [ST-PRACTICE] Steam thread "Please add a practice mode!".
  https://steamcommunity.com/app/1147860/discussions/0/595136312034544769/
- [ST-2P] Steam thread "Best 2-Player Games?": "kinda solitaire".
  https://steamcommunity.com/app/1147860/discussions/0/4849904631717074845
- [ST-SAVES] Steam thread on which games save.
  https://steamcommunity.com/app/1147860/discussions/0/4849904176633630339/
- [ST-JUNK] Steam thread "Pingolf - purple barrels/traffic cones": they are
  speedbumps. https://steamcommunity.com/app/1147860/discussions/0/4849904631721716602/
- [LZ] Lizstar's review of UFO 50 #31: angle and power, the dunk slams the
  ball down at an angle, the same holes every time, the weak AI.
  https://lizstar64.github.io/reviews/2024/10/17/UFO50-31.html
- [STATIC] Static Canvas, "The UFO 50 Diaries: Pingolf": you always see
  every hole. https://staticcanvas.substack.com/p/the-ufo-50-diaries-pingolf
- [PC] Popcar's Blog, "Reviewing every UFO 50 game": dunking on slopes sets
  the ball on fire. https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [TG] TheGamer, "The Hardest Games in UFO 50".
  https://www.thegamer.com/ufo-50-hardest-games/
- [HANS] thehans255, "One unique thing about every UFO 50 game": the pinball
  display. https://www.thehans255.com/blog/2024/10/one-unique-thing-about-every-ufo-50-game/
- [RS-35] ResetEra UFO 50 thread, pages 34-35: +13 on one hole, the dunk
  not taught. https://www.resetera.com/threads/ufo-50-ot-50-games-for-the-price-of-1.981834/page-35
- [SR-RAMEN] Search summaries of "Ramen's Guide to PINGOLF" (the guide itself
  answered 429 every time). https://steamcommunity.com/sharedfiles/filedetails/?id=3365963634
- [BQ] Bug Quest, "UFO 50 (pt. 2)": only two players.
  https://bugquest.substack.com/p/ufo-50-pt-2
- [CONV] the UFO 50 conventions notes (`00-ufo50-conventions.md`).
