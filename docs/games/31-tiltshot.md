# 31 · TILTSHOT

*Internal design document. Not shown in the product.*

## Tribute to

**Pingolf** (UFO 50 game #31, Mossmouth, "May 1987"). The rules were
researched from text only: the community wiki's raw and rendered pages,
Steam guides and threads, written reviews and forum posts (listed under
Sources; the full notes are in the research folder, `31-pingolf.md`, and the
independent review in `31-review.md`). No UFO 50 images, video, sprites,
music, hole layouts or text were used as references, and nothing was taken
from a UFO 50 install.

**Map type: fixed.** Pingolf's eighteen holes are hand-made and "the same
every time" [LZ], [MH], so ours are too (drawn in `tools/tiltshot/design.py`,
written out to `tools/tiltshot/holes.txt` and made into `tiltshot_maps.c`;
the movers and spring lines are in `tiltshot_holes.c`). Each hole keeps its
original's par, its purpose and the pieces the guides name on it, and
roughly its length; every layout is our own, drawn from those descriptions
alone.

| Full scale | Pingolf | TILTSHOT |
|---|---|---|
| Holes | 18, fixed, always in the same order; no hole select, practice or retry [MH], [PC], [ST-PRACTICE] | 18, fixed, in order; no hole select, practice or retry |
| Pars | 3 3 3 4 4 4 3 4 1 · 3 2 4 3 4 4 3 3 6 = 61 [MH] | the same, hole by hole |
| The par-1 hole | hole 9, where an ace is par [MH] | hole 9, TOSS-UP |
| The par-6 hole | hole 18, the long finale [MH] | hole 18, LAST ORBIT (six screens) |
| Hole length | longer than a screen, scrolling sideways [MH], [LZ] | 1.5 to 6 screens (60 to 240 tiles of 8 pixels) |
| Field | 8 golfers: you (and a second player) and CPU golfers [MH] | 8: 1 or 2 players and 7 or 6 CPU rivals |
| Golfers | 5 to choose, 2 colourways each, cosmetic; 2 more by a code [MH], [MH-CHEATS] | 5 (NOVA, DIGBY, PEACHES, TUCK, MOSS), 2 colourways each; WICK and KIP by the code |
| CPU names | not documented [MH] | 12 regulars, 7 (or 6) drawn each tournament |
| Course things | bumpers (large, small), trampolines, ramps, orange movers, purple trash, pegs, tunnels, ceilings, bridges, a hanging pillar, sand, water, pits [MH], [CG], [RAMEN] | the same kinds: big and small bumpers, spring pads and spring lines, slopes and kickers, 7 kinds of mover, 4 kinds of junk, pegs, tunnels, roofs and halls, a plank bridge on 12 and 18, a pillar hanging from the sky, corner blocks, sand, water, pits |
| Moving hazards | on most holes: drones (2, 15), birds (3), spinners (4, 5), suns (4, 6), unicycles (7, 13, 18), juggling balls (9), swinging monkeys (10, 16, 18), fish (12, 18) [RAMEN], [CG] | on the same 13 holes: blimps (2, 15), kites (3), sparks (4, 5, 6), trundlers on the ground (7, 13, 18), hop-bots (9), lanterns on chains (10, 16, 18), fish (12, 18) |
| Power-ups | none [MH] | none |
| Goals | 3 [MH], [GG] | the same 3 |
| Stats | Golfers Used, Most Hole-in-Ones [MH] (the wiki's speedrun and lowest total are community records) | RECORDS: golfers used, most holes in one in a round |
| Length of a tournament | about 10 minutes; speedrun record 3:56 [LZ], [MH] | the demo player's run takes 3:57 from the field to the final standings |

### The eighteen holes

Purposes and pieces are summarised from the guides [CG], [RAMEN], [RS-35],
[IGGY]; layouts are ours. "Route" is the route finder's strokes, "Yard" the
yardstick player's average over ten rounds (see How they play).

| # | TILTSHOT | Par | The original's hole is for | Ours | Tiles | Route | Yard |
|---|---|---|---|---|---|---|---|
| 1 | OPENING TEE | 3 | a tutorial: full speed into a hill, into sand past the bumper; the ace is a dunk on the mound with the box, off the bumper | a mound with a crate on top and a bumper past it; a full shot over the mound drops into the trap under the bumper, a pond between it and the green. Slam onto the mound's far side at the right moment and the ball races off on fire over the trap and the pond, off the backstop and in | 112 | 1 | 3.1 |
| 2 | STONE SKIP | 3 | skipping the water at full speed; drones; an upper pathway; a bumper | a lake under a sheer tee cliff, blimps over it; a rock with a sand top halfway; a high stone ledge with two bumpers over the lake; the green up a bank past a trap, the ground ending past the cup | 128 | 2 | 2.5 |
| 3 | BOUNCE HOUSE | 3 | a bouncy line with blockers, birds over it; a near-impossible, very precise ace | two spring lines across a pit with a stone post between them, two kites over them; a trap past the pit, then the green: a low cave under a roof that rises to the sky, sand in its mouth, a drop past the cup. Its one ace is a chain of bounces and a slam, not at full power | 100 | 1 | 2.1 |
| 4 | SKYLARK | 4 | lobbing over suns and spinners and dunking down; clutter as brakes | a bottomless canyon with sparks wheeling over it, a sheer mesa with junk at its far end, a second canyon with a bumper and a spark over it, a trap at the green's edge and a drop past it | 140 | 2 | 4.2 |
| 5 | THE CHUTE | 4 | a dunk onto a ramp for speed, the ball rolling back; a top tunnel with spinners and corner blocks | a drop into a hollow and a climb out of it that runs up inside the hill under a roof: slam onto the drop and the ball races up into the sand at the top; the tunnel goes on with sparks and hanging corner blocks; or loft it over the hill (junk on top); a trap at the foot of the far slope, a drop past the green | 140 | 2 | 4.0 |
| 6 | THREE STOREYS | 4 | stacked platform routes, dunking to push further; suns, corner blocks, junk; one of the "banes" | a floor broken by pits and two ice shelves over it; the green stands on a pillar past a wide pit, higher than the middle shelf: slam onto the ice near its end and the ball hops across. A wall stands on the top shelf's end; sparks drift round, a corner block on the floor | 130 | 2 | 5.1 |
| 7 | DUNE STEPS | 3 | sand pits; a precise water bounce skips the first; a unicycle on the approach; corner blocks | dune terraces with sand in the hollows, a pool to skim before the first sand, a corner block over the dunes, a trundler patrolling the approach, sand either side of the cup and a drop past it | 132 | 2 | 3.0 |
| 8 | FROST TUNNEL | 4 | an accurate opener into a small tunnel (dunk to drop in); the red block | an ice mountain with a one-tile shaft in its top that drops into a tunnel out to the green; a bumper on the approach; the red block on the peak; a drop past the green | 140 | 2 | 2.6 |
| 9 | TOSS-UP | 1 | the only par 1: juggling balls, "aim and pray", "full send" | a full shot high over a pit onto a slope that runs down against a stone wall, the cup at its foot; four hop-bots juggle across the ball's way | 60 | 1 | 1.1 |
| 10 | DOUBLE SPRING | 3 | hit the first bouncy line, dunk into the second; the ace slammed after the second bounce; one of the hardest to ace | a chasm with two spring lines and a lantern swinging between them; the green is a low hall under a ridged roof, sand at its door and an open end over a drop, a chimney through the ridge's peak over the cup. The ace: slam the ball back onto a spring line at the right moment and it flies over the ridge, in down the chimney or round through the hall's far end | 110 | 1 | 2.1 |
| 11 | PEBBLE | 2 | short; shooting above the platform and dunking on the first ramp; an ace | a hilltop tee over a steep ramp with a stone ledge over it; a flat runs into a low hall packed with junk, the cup past it and a drop at the far end. The ace drives in low and slams at the right moment to hop the last of the junk | 72 | 2 | 2.0 |
| 12 | ATOLL | 4 | islands and fish; stopping the ball (a barrel, a cone); a run-killer | four small islands in open water, a fish in every gap, a strip of sand and a piece of junk on each, a plank bridge standing alone in the last stretch | 150 | 2 | 5.0 |
| 13 | BOULDER | 3 | a tall wall; "the most difficult hole, it's not close": a precise dunk dodging two unicycles | a wall far taller than anything so far close to the tee, a pit behind it, sand past the pit; two trundlers patrol a yard either side of a two-row stone step with the cup on top, a bumper over the yard and a pit past it | 96 | 2 | 4.7 |
| 14 | BUMPER ALLEY | 4 | a bouncy line at near full power, then a dunk to dodge a bumper | a spring pad by the tee throws the ball over a pit full of bumpers onto a sand-edged shelf with more bumpers over it; a gap with a big bumper, then the green in a hall whose roof rises to the sky, a pit inside past the cup | 140 | 2 | 3.2 |
| 15 | THE HIGH SHELF | 4 | one of the hardest to perfect: a full shot over clutter into an up-ramp; a drone in the climb; a bottom path | a fairway strewn with junk to a kicker; the bottom path below it (a valley with a pit at its near end, then a long climb) with a blimp over the climb; the green high on the shelf, a trap short of the cup and a drop past it | 140 | 2 | 4.4 |
| 16 | NERVE | 3 | lob distance control; clutter helps the last shot; swinging monkeys | two pillars over pits, the first one all sand, a lantern swinging between them; junk on the green past the cup and a drop past that | 100 | 2 | 3.3 |
| 17 | THE PAGODA | 3 | one of the hardest to perfect: pegs and a sand pit, a pit past the sand | a lattice of pegs over a sand pit, a pit between the sand and the raised green's bank, a drop past the green | 100 | 2 | 3.0 |
| 18 | LAST ORBIT | 6 | the long finale: fish, unicycles, swinging monkeys, pegs, several sand pits, water, a bridge, a hanging pillar | a lake with two fish, a beach, a forest of pegs over a sand bed, a second trap, a second lake with a plank bridge and a pillar hanging down out of the sky with a lantern under it, a climb to a plateau where a trundler patrols, a wall to the sky across its end with a low gate, the green down a bank | 240 | 4 | 5.6 |

**How they play.** The **route finder** (`tsh_solve`, a frame-perfect
search with slams) gets round in **34 strokes (-27)**: 1 2 1 2 2 2 2 2 1 ·
1 2 2 2 2 2 2 2 4. The world record is -29 (32 strokes) [MH] and the guide's
planned route is 41 (-20) [RAMEN]. It needs a slam for every hole in one
(1, 3, 10; 9 is a timed full shot), at least 2 on every other par 3 and on
every par 4, and 4 on the par-6 finale.

The **yardstick player** (`tsh_steady_play`) knows the holes: from each lie
it weighs a grid of shots (and, where things move, when to swing) by trying
each with its strength 5 frames and 2 frames off either way and its slam 6
and 3 frames off, then plays the best with its own swing a little off: the
strength by up to 5 frames (full power too), the slam by up to 6, the moment
it swings by up to 3. Since the review it can also wait for movers (up to
two seconds), slam as late as 135 frames into a flight (it stopped at 81),
and it weighs a shot that loses the ball as about twenty tiles lost rather
than five, as a player who knows the holes would; its misses are the same.
Over ten rounds (`cheat yard 10 1 18`) it averages
**61.0 (level par)**, its best round is **57 (-4)** and its worst 68 (+7),
against practised players' best rounds of -4 to -7 [RE-34], [RAMEN] and first
rounds of +28 to +49. The front nine averages 27.7 (par 29), the back nine
33.3 (par 32). The holes that play hardest against par are **13 (+1.7)**,
**6 (+1.1)** and **12 (+1.0)**, then 15 and 16.

On 13 holes a tee shot can be lost (a pit, water or the lake), all but 5,
8, 11, 15 and 17. **Toss-Up**: the full shot at 45 degrees drops in at 21 %
of the moments in the jugglers' round, the aims either side at 11 % and
5 %, and the gaps a watcher can time are up to 28 frames wide.

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
| HUD | stroke, par and distance on an amber dot-matrix display, with the meter and a SLAM lamp at its right end, lit while the slam is there to use in the air | [MM], [TG], [HANS], [RE-35] | tsh_03 |
| Events on the display | SPLASH!, DOWN THE PIT!, KA-BOOM!, ON FIRE!, BUMPER!, BOING!, SMASH!, CRUNCH!, SKIP!, SLAM! and the result (BIRDIE!, HOLE IN ONE! ...) flash up on the display, pinball style; the course stays clear | [HANS] | tsh_04, tsh_05, tsh_06, tsh_13 |
| Sand | stops nearly everything; the ball rests where it lands | [MH] | tsh_07 |
| Water | the ball sinks and comes back to where it was hit from; the stroke counts, no penalty on top | [MH], [IGGY] | tsh_05 |
| Skipping | a fast ball nearly flat to the water skips across | [MH], [CG], [RAMEN] | tsh_05 |
| Pits | back to where it was hit from; the stroke counts, no extra penalty | [MM], [MH] | tsh_05 |
| The cup | the hole counts the moment the ball is in, even if it would bounce out | [MM] | tsh_06 |
| No stroke limit | a hole takes as many strokes as it takes | [ST-PG], [RS-35] | tsh_06 |
| Bumpers | round, fixed in the air, kick the ball away | [MH] | tsh_07 |
| Trampolines | spring pads and spring lines throw the ball far harder than bumpers do | [MH], [CG] | tsh_07 |
| Orange movers | move about (in the air, along the ground, on a chain), break at a touch, and slow the ball drastically | [MH], [CG], [RAMEN] | tsh_07 |
| Purple trash | still clutter that breaks at a touch and slows the ball a little; usable as a brake | [MH], [CG], [ST-JUNK] | tsh_07 |
| Pegs | small fixed posts to thread between | [CG] | (holes 17, 18) |
| Holes in one | only with a slam, on every hole but the par 1 | [ST-JUNK], [CG], [RAMEN] | tsh_17 |
| The par-1 hole | aim and pray: timed through the jugglers | [CG], [RAMEN] | tsh_18 |
| The red block | hit on hole 8, it shows a strange message | [MH-META] | tsh_16 |
| Stroke play | eight on the board; each golfer's total against par; lowest after 18 wins | [MH], [ST-RNG] | tsh_06, tsh_09 |
| Weak, random CPU | the best CPU finishes between 4 and 13 over par, never at par or under | [ST-RNG], [LZ] | tsh_08 |
| 2P versus | two players in the same tournament; they don't touch each other's ball | [MH], [MH-MP], [ST-2P] | tsh_11 |
| Golfers | cosmetic; two colourways each | [MH] | tsh_12 |
| The code | brings two cameo golfers; printed in the credits; while it is on nothing is saved and no goals are given, and it lasts until you leave | [MH-CHEATS], [LZ] | tsh_12 |
| No saving mid-run | a tournament is played from hole 1 to 18; the stats and goals are kept | [ST-SAVES], [CONV] | tsh_13 |
| Stats | golfers used, most holes in one in a round | [MH] | tsh_09, tsh_13 |
| Goals | Beacon: a hole in one; Saucer: win the Comet Classic; Alien: win at par or under | [MH], [GG] | tsh_09, tsh_10, tsh_13 |
| Always finishable | however badly it goes, all 18 holes are played to the end | [STATIC] | tsh_10, tsh_14, tsh_15 |

### Readings we had to choose

- **The blow-up spends a stroke.** The manual says the shot is cancelled,
  the wiki that it is "forfeited entirely"; we count it as a stroke taken
  with the ball left where it was.
- **The meter:** 60 frames to fill; full for 24 frames; then 48 frames of
  warning (EASY NOW!, the golfer flashing red) before the blow-up. A
  release at any point before that swings. A tap is the softest putt.
  **Full power is the same shot all the time the meter sits full**: the
  meter "rises to max" and holds there [MH], [MM], and the guides use full
  power as their fixed reference shot ("full speed", "max power", "full
  send") [CG], [IGGY], [RAMEN]. What made identical full shots a problem was
  holes that gave them a free hole in one; no hole does now (tsh_17).
- **Aim:** 37 settings five degrees apart, from flat right to flat left
  (guides give aims like "45 degrees"); every hole starts at 45 degrees;
  held, LEFT and RIGHT keep turning after a moment.
- **Power and gravity:** 0.7 to 5.8 pixels a frame, gravity 0.1: a full
  shot at 45 degrees carries about a screen and lands running.
- **The slam:** it sets the fall to 5 pixels a frame (plus a little of any
  fall it already had) and leaves the speed across alone; it only works in
  the air (a ball rolling along the ground can't be slammed), and the slam
  isn't spent by a press on the ground.
- **The slope boost:** a slam's first touch on the face of a slope of 1 in
  2 or steeper that falls away sends the ball down it at 1.45 times its
  speed along the slope plus 2.2, and it burns for a second and a quarter.
  The corner of a ledge isn't a slope ("dunking onto a downward ramp" [MH]).
  Fire is only a look; it doesn't change what the ball hits.
- **Skipping:** the ball must be going at least 3 pixels a frame across and
  coming down at under about 27 degrees; each skip keeps 85 % of its speed
  across and 70 % of its fall, up to six skips.
- **Bumpers** send the ball off at three quarters of its speed into them
  plus 2.2 (at least 2.8); **spring pads and lines** at 1.3 times plus 2.4
  (up to 10), from anything faster than 0.8 a frame, so they give back more
  than they get ("a much more volatile force" [MH]); a ball that just rests
  on them stays put. After 15 seconds in play the kicks stop, so nothing
  bounces forever.
- **Junk** keeps 60 % of the ball's speed and stays broken for the rest of
  the golfer's hole; a **mover** keeps 20 % and is back for the next stroke.
  Trundlers patrol a floor at a steady speed; lanterns swing on a chain.
- **The cup** counts once the whole ball is below the rim; the check comes
  before the ball can bounce back out.
- **Out of bounds:** each hole is walled at both ends; the sky is open, and
  a ball above the screen shows as an arrow at the top. Stone that reaches
  the top of the screen carries on up out of sight, like the end walls, so
  a roof or a wall that touches the top can't be flown over.
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
  terminal). Like a terminal code in UFO 50 it only lasts while it is on,
  here until you leave the cartridge, and while it is on the cartridge saves
  nothing and gives no goals [MH-CHEATS]; the title says so.
- **The display:** the SLAM lamp is the dot-matrix "dunk button in the
  bottom right corner" [RE-35]: dark on the ground, lit in the air while the
  slam can be used, dark again once it is spent. Events and the result of
  the hole take the display over for a moment and flash.
- **The hole card** before each hole shows its name, par and outline.

## What is ours

- **Name:** TILTSHOT (1987, Beamdown Softworks), and its tournament, the
  **Comet Classic**, played every time the comet swings by; the title's
  line: "THE COMET IS BACK. THE BUMPERS ARE WAITING."
- **Golfers:** NOVA (a girl in a visor), DIGBY (a robot), PEACHES (a
  three-eyed alien), TUCK (a penguin) and MOSS (a frog), two colourways each.
- **The cameos and the code:** WICK (from HOMESPUN, our Pilot Quest) and KIP
  (from SKYWELL), brought by **BEAM-DOWN**, which the credits print.
- **The regulars:** ORBO, GLIMMA, GEARBOX, MAVIS, BIG NED, ZIBBO, LADY FEN,
  SPROUT, DR QUILL, BLIX, HONK and MARGO.
- **Courses:** all eighteen holes, their names and seven looks (meadow,
  dusk, night, ice, beach, temple, funfair).
- **Things:** blimps, hop-bots, kites, fish, sparks, trundlers (two-wheeled
  patrol robots) and lanterns on chains (the orange movers); crates, cones,
  churns and popcorn buckets (the purple junk); bumpers, spring pads, spring
  lines, pegs, corner blocks, the chimney on Double Spring and the step on
  Boulder.
- **Words:** "slam" for the dunk, EASY NOW!, KA-BOOM!, BOING!, SMASH!,
  CRUNCH! and the other display calls, the red block's line ("THEY BUILT IT
  FOR FORTY. FIFTY CAME DOWN.", a UFO 40 joke), the titles, the credits and
  every label.
- **Music:** "Comet Classic" (title), "Front Nine", "Back Nine",
  "Eighteenth Hole", "The Leaderboard", "Champion of the Comet" and four
  jingles ("In the Cup", "Hole in One", "Over Par", "Runner-Up").

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu, the three UFO 40
goals (Pingolf's own gift, gold and cherry), and saving the original's two
stats and the goals. The hole card is a reading, above.

## Controls

| Input | Action | Confirmed? |
|---|---|---|
| LEFT / RIGHT | turn the aim | yes [MM] |
| Hold A | fill the meter | yes [MM], [MH] |
| Let go of A | swing | yes [MM] |
| A in the air | slam, once a stroke | yes [MM], [MH], [CG] |
| B | nothing in play | the manual gives play only the d-pad and A [MM] |
| UP / DOWN | nothing in play (colourway on the golfer screen) | **no**: no source mentions them |
| START | pause | collection convention [CONV] |
| Menus | UP / DOWN choose, A picks, B goes back | collection convention [CONV] |
| 2P on one keyboard | WASD F G and ARROWS K L | UFO 40's own versus keys |

## Not confirmed

- Whether the blow-up costs a stroke (we say yes).
- The meter's speed, how long it stays full, how long the warning lasts.
- The aim's step and range.
- What UP/DOWN do in play.
- How 2P takes turns and how many CPUs play then.
- How the CPU rounds are made, and the CPU names.
- What fire does beyond speed (we say nothing).
- Out of bounds at the top and sides.
- Whether broken junk and movers come back.
- Ties at the top.

## Tests

`tests/tsh_01` … `tsh_18` drive the rules with button presses; cheats only
put the ball somewhere, jump to a hole or run a design check. `tsh_01` plays
from the title through the meter, the warning and the blow-up; `tsh_03`
checks the slam and its lamp; `tsh_07` the course things, springs against
bumpers and the new movers; `tsh_09` plays a whole tournament from the title
with the demo player pressing real buttons and wins it under par (all three
goals, the stats and the ending); `tsh_10` loses one; `tsh_11` plays 2P
with both pads; `tsh_12` enters the code (no goals or saving while it is on,
gone after leaving); `tsh_14` and `tsh_15` replay a route round every one of
the eighteen holes with real presses, each at or under par, and check the
route finder's counts (33 to 38 in all, at least 2 on each par 4 and on the
par 3s with no slammed ace, at least 4 on 18).

**`tsh_17` is the design test for holes in one:** on every hole but 9, all
37 aims at all 61 strengths with no slam, from the tee, on the course with
nothing moving and, from every point where the ball could meet a mover, as
if it had struck it; on holes with movers the same sweep again at four
start times through their round. None drops in. It then plays the slammed
holes in one on 1, 3, 10 and 11: in with the slam, out without it or with
the slam a few frames off. `tsh_18` checks Toss-Up's odds and plays its ace.

The routes come from the route finder (`sh tools/tiltshot/mkroutes.sh`,
after `make headless`). Design checks: `cheat sweep N M` (the free-ace
sweep), `cheat slamaces N P S` (every slammed tee shot that drops in),
`cheat timing N AIM POWER` (one shot at every moment of the movers' round),
`cheat yard R A B` (the yardstick over R rounds of holes A to B),
`cheat yardlog R A B` (its shots), `cheat play N a p s ...` (shots from the
tee), `cheat landscape N AIM SLAM [X Y CLOCK]` and `cheat trace N AIM POWER
SLAM [X Y CLOCK]` (where shots go).

## Review fixes

The independent review (`31-review.md`, 2026-09-30) found 15 things. All
are dealt with:

1. **The first shot aced hole 1.** Opening Tee is redrawn: a full shot over
   the mound drops into the trap under the bumper; the ace needs a slam onto
   the mound's far side within a few frames. No tee shot without a slam
   drops in (tsh_17).
2. **Full-power aces with no slam on 1, 3, 10, 11 and 14.** Every hole but 9
   is checked by tsh_17. The aces the guides describe need a slam: 1, 10 and
   11 within a few frames, 3 a chain of bounces not at full power; 14 and
   every other par 4 have none (the route finder needs at least 2). The full
   meter stays a plateau, as the sources describe it (see Readings).
3. **Holes too short; the par 6 in 2.** All 18 are redrawn: the route finder
   now needs 34 (-27), against 27 before; hole 18 needs 4.
4. **Much easier than the original.** The yardstick player now averages
   level par over ten rounds with a best of -4 (it averaged -11 before); the
   front nine is no longer a stroll (27.7 for par 29).
5. **The hardest holes weren't hard.** 13, 6 and 12 now play hardest:
   trundlers either side of a step on 13, a slam-hop off the ice on 6, small
   islands with junk and a lone bridge on 12; a pit past the sand on 17.
6. **Too few movers.** Movers on the same 13 holes as the original, with two
   new kinds: trundlers patrolling the ground (7, 13, 18) and lanterns
   swinging on chains (10, 16, 18).
7. **Plain holes.** Roofs and halls (3, 5, 10, 11, 14, 18), tunnels (5, 8),
   a bottom path (15), an upper path with bumpers (2), a ledge over the ramp
   (11), corner blocks (5, 6, 7), bridges (12, 18), a pillar hanging from
   the sky (18), bumpers on 1, 2, 4, 8, 13 and 14, junk where the guides
   brake on it.
8. **Hole 9 was a coin flip.** Four jugglers over a pit: about one moment
   in five for the full shot, with clear gaps for a watcher (tsh_18).
9. **Springs barely beat bumpers.** Springs now give back 1.3 times plus
   2.4 (up to 10); bumpers 0.75 times plus 2.2 (tsh_07).
10. **No dunk lamp.** A SLAM lamp at the right end of the display (tsh_03).
11. **Callouts over the course.** Every event and the hole's result go up
    on the display instead.
12. **Hold B to look along the hole.** Removed: B does nothing in play.
13. **Extra stats.** RECORDS keeps only golfers used and most holes in one.
14. **The code unlocked for good with goals on.** It now lasts until you
    leave the cartridge, and while it is on nothing is saved and no goals
    are given, like UFO 50's terminal codes (tsh_12).
15. **The tagline followed UFO 50's sentence.** Replaced by our own line.

Also found while redrawing: a slam onto the corner of a ledge counted as a
slope and set the ball on fire; the boost now needs the face of a slope.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Pingolf" (rendered and raw): the meter, the
  dunk and the slopes, the golfers, the hazards, the 18 holes and pars, the
  goals, stats and records. https://ufo50.miraheze.org/wiki/Pingolf
- [MH-CHEATS] Miraheze, "Cheats": STAR-BALL (not used); terminal codes stop
  saving and achievements while they are on.
  https://ufo50.miraheze.org/wiki/Cheats
- [MH-META] Miraheze, "Meta Messages": the red block on hole 8.
  https://ufo50.miraheze.org/wiki/Meta_Messages
- [MH-MP] Miraheze, "Multiplayer". https://ufo50.miraheze.org/wiki/Multiplayer
- [MM] Steam guide "The missing manuals - How to play UFO 50 games",
  Pingolf: aim with the dotted line, hold to charge, the warning and the
  explosion, the dunk, the HUD, the cup rule, pits; no use for B.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [CG] Steam guide "How To Get Cherry Clear For Pingolf": hole purposes,
  the slammed aces on 1, 10 and 11, hole 9's "aim and pray", skipping,
  bouncy lines, purple brakes.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3333659823
- [RAMEN] Steam guide "Ramen's Guide to PINGOLF": a stroke-by-stroke route
  (41, -20), the pieces on every hole (drones, birds, spinners, unicycles,
  swinging monkeys, fish, corner blocks, tunnels, a bridge, a hanging
  pillar), 13 the most difficult hole, 6 and 12 the banes, best finished
  round -4. https://steamcommunity.com/sharedfiles/filedetails/?id=3365963634
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
  speedbumps; the hole 1 ace by dunking on the mound with the box.
  https://steamcommunity.com/app/1147860/discussions/0/4849904631721716602/
- [LZ] Lizstar's review of UFO 50 #31: angle and power, the dunk slams the
  ball down at an angle, the same holes every time, the weak AI.
  https://lizstar64.github.io/reviews/2024/10/17/UFO50-31.html
- [STATIC] Static Canvas, "The UFO 50 Diaries: Pingolf": you always see
  every hole. https://staticcanvas.substack.com/p/the-ufo-50-diaries-pingolf
- [PC] Popcar's Blog, "Reviewing every UFO 50 game": dunking on slopes sets
  the ball on fire. https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [TG] TheGamer, "The Hardest Games in UFO 50": "so elaborate".
  https://www.thegamer.com/ufo-50-hardest-games/
- [HANS] thehans255, "One unique thing about every UFO 50 game": the pinball
  display that accentuates game events.
  https://www.thehans255.com/blog/2024/10/one-unique-thing-about-every-ufo-50-game/
- [RE-34] ResetEra UFO 50 thread, page 34: +32 at first, -7 after playing
  the holes a bunch.
  https://www.resetera.com/threads/ufo-50-ot-50-games-for-the-price-of-1.981834/page-34
- [RS-35], [RE-35] ResetEra UFO 50 thread, page 35: +13 on one hole, the
  dunk not taught, "the dunk button in the bottom right corner".
  https://www.resetera.com/threads/ufo-50-ot-50-games-for-the-price-of-1.981834/page-35
- [SR-RAMEN] Search summaries of Ramen's guide, used before the guide itself
  could be read (now [RAMEN]).
- [BQ] Bug Quest, "UFO 50 (pt. 2)": only two players.
  https://bugquest.substack.com/p/ufo-50-pt-2
- [CONV] the UFO 50 conventions notes (`00-ufo50-conventions.md`).
