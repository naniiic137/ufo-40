# 19 · CHIME CIRCUIT

*Internal design document. Not shown in the product.*

## Tribute to

**The Big Bell Race** (UFO 50 game #19, Mossmouth, "October 1985"), a spin-off
of Campanella (#17) "built in just two weeks" and sold with the console. The
rules were researched from text only: the community wiki's raw pages, the
Steam guides "The missing manuals" and "The Big Bell Race - Guide and Tips",
the Gift, Gold & Cherry guide, Steam threads and written reviews (listed
under Sources; the full notes are in the research folder,
`19-big-bell-race.md`). No UFO 50 images, video, sprites, music, course
layouts or text were used as references (the wiki's course pictures were not
opened), and nothing was taken from a UFO 50 install.

**Map type: fixed.** The original's eight courses are hand-made and always
run in the same order [W], [G], so ours are too (`chime_tracks.c`, all eight
layouts our own). Only what each course is for follows the original, from
the guide's descriptions [G]:

| # | The original's course [G] | CHIME CIRCUIT |
|---|---|---|
| 1 | a simple loop with two boost pads; the tutorial | PRELUDE RING: a wide loop over a meadow, a boost arrow on each straight |
| 2 | a loop with a dip and a big drop that decide where to build momentum, and a narrow passage that has its uses but is often a trap | THE DIPPER: the main line dives under a hanging rock (the dip), climbs out and then drops the whole height of the screen; a slot through the top of the rock, three tiles high with a two-tile step in the middle, is a short, risky bypass with a pickup station inside it |
| 3 | a very narrow loop; easy to stay ahead, hard to catch up, pile-ups | NEEDLE'S EYE: three tiles wide over the top and the bottom and four down the sides, with a hump over the top and a dip under the bottom |
| 4 | the first figure-8, probably the toughest; rising and falling | HOURGLASS: two round lobes that cross in an X in the middle, where every lap climbs through and drops through |
| 5 | a forked loop: the top half longer with a boost plate, the bottom plain; they rejoin before another boost plate down a straightaway | FORKED REED: after the climb the way splits into a long high branch with a boost and a plain low one; they meet on the right and drop to a straightaway with a boost |
| 6 | a figure-8 the AI runs badly; it turns to wrecking ships instead | TWIN FLUE: a figure-8 of two lobes in opposite corners of the screen, crossing square in the middle; the CPUs hunt |
| 7 | a bendy loop whose narrow passage is easy and is the key to beating the AI | CROOKED MILE: a square-wave mile of bends along the top, and a plain three-tile tunnel under them that cuts the whole mile; the CPUs never take it |
| 8 | the last figure-8, with a long bottom half; the most balanced | GRAND OCTAVE: a figure-8 with a wide loop on top (lanes seven tiles across), an open crossing and a long low straight with a boost; the CPUs run it closest to your pace |

| Full scale | The Big Bell Race | CHIME CIRCUIT |
|---|---|---|
| Ships in a race | 6 (six places score) [G], [TVT] | 6 |
| Players | 1P, or 2P at once on one screen [W], [ST-1P], [STATIC] | the same; in 2P, 4 CPU ships |
| Laps in a race | 8 [W], [G] | 8 |
| Races | 8, one per course, fixed order [W], [G] | 8 |
| Points | 9, 7, 5, 3, 2, 1; 72 at most [G] | the same |
| Hit points | 3 [MM], [W] | 3 |
| Pickups | 5 [W] | 5 |
| Pickup stations | stations that refresh [W-CH] | 2 or 3 on each track (18 in all) |
| Pilots | 6, looks only [W], [LIZ] | 6 |
| Boost arrows | on some courses [MM] | on 3 tracks: PRELUDE RING, FORKED REED and GRAND OCTAVE |
| Goals | 3 [W], [GGC] | the same 3 |
| Stats | P1 and P2 average lap and average race, in seconds [W] | the same four |
| Codes | one: pickup stations refresh faster [W], [W-CH] | one of ours, LOOT-GALE |
| Meta message | a terminal page that says "I'M SO TIRED" [W] | LAST-LAMP, a tired note in our own words |

**How long it takes.** A whole tournament is about 12–20 minutes for a new
player and 7:51 for the best speedrun, and a race about a minute
[ST-BEATEN], [STATIC], [SR]. The demo pilot's clean runs (the CPUs taken off
the track) add up to 6:43 of racing; against the CPUs, the scrum and the
pickups it averages 63 s a race, about 8:30 of racing in all, and it wins
146 of 160 races (20 seeds on each track, with its lone wrecks at 0.7 a
race). The CPUs' pace against it is the track's own (see Readings):

| Track | Demo pilot alone | A CPU alone, against it | Demo pilot in a race (mean) | Races it wins |
|---|---|---|---|---|
| PRELUDE RING | 41 s | 65 % | 54 s | 20 of 20 |
| THE DIPPER | 53 s | 70 % | 67 s | 19 of 20 |
| NEEDLE'S EYE | 51 s | 62 % | 73 s | 19 of 20 |
| HOURGLASS | 48 s | 75 % | 64 s | 14 of 20 |
| FORKED REED | 37 s | 74 % | 43 s | 20 of 20 |
| TWIN FLUE | 48 s | 57 % | 65 s | 20 of 20 |
| CROOKED MILE | 51 s | 47 % | 53 s | 20 of 20 |
| GRAND OCTAVE | 73 s | 79 % | 89 s | 14 of 20 |

On PRELUDE RING the CPUs are still on their third to fifth lap when it
finishes, so it laps them more than once, as in the original's tutorial
race [G].

## Structure

- **Title**: 1 PLAYER, 2 PLAYERS or CODE, over a race of six CPU pilots;
  the stats (each player's average lap and race) and the cups and races
  won along the bottom. B goes back to the library.
- **Pilots**: each player picks one of the six with their own pad; the
  unpicked pilots fly the CPU ships.
- **The tournament**: eight races, one on each track in order. Before
  each, a card shows the track, what it is for and the standings. Each
  race: a 3-2-1 countdown on the grid, eight laps, then the results
  (places, times, points) and the standings.
- **The end**: the podium. If a player won the cup, their pilot's ending,
  then the credits and THE END; if a CPU won, back to the title.
- **Not kept**: a tournament is played in one sitting (START pauses it);
  the stats and records are saved.

## The three goals

| UFO 40 goal | Condition | The Big Bell Race's goal [W], [GGC] |
|---|---|---|
| Beacon | take 1st place in a race | gift: win first place in at least one race |
| Saucer | win the tournament (the Chime Cup) | gold: win the tournament |
| Alien | take 1st place in all eight races | cherry: win first place in every race |

A tournament begun with the LOOT-GALE code on earns none of them.

## Mechanics checklist

| Mechanic | How CHIME CIRCUIT does it | Source | Test |
|---|---|---|---|
| Side view, gravity | the ship falls unless A is held; tapping A hovers | [MM], [W] | chm_02 |
| Thrust | hold A to push up; no fuel; held with a steer, it helps the ship pick up speed that way | [MM], [W], [TVT], [G] | chm_02 |
| Steering with inertia | left and right accelerate the ship, which drifts on when let go; the same model as the chime-ship games (`chime_flight.c`) | [W], [STATIC-CAMP], [SEARCH-CAMP] | chm_02 |
| Slash | B: a short swipe to the side the ship faces; no damage; the ship it reaches is knocked away and can't steer for a moment (its thrust still works); the swipe slows the slasher's fall a little | [MM], [MM-CAMP], [W], [W-CAMP] | chm_04 |
| Ramming | ships bump apart and swap speed; no damage | [G] | chm_05 |
| Hit points | 3; walls, floors, ceilings and weapons each take one | [MM], [W], [G] | chm_03, chm_09 |
| Damage shown | a meter pops up over the ship for a second whenever its hit points change; a ship on its last hit point smokes | [MM], [G], [STATIC] | chm_03, chm_06, chm_07 |
| Wrecked | at 0 hit points; a replacement launches from under the start line a moment later and flies that lap again; laps are kept | [MM], [G], [W] | chm_06 |
| Mercy after a hit | a short time, "not super generous" | [G] | chm_03, chm_06 |
| Laps | a lap counts at the line after the track's checkpoints in order, so no short cut skips half a lap | (reading) | chm_07 |
| Lap heal | every finished lap mends one hit point, never above three | [MM], [W] | chm_07 |
| Boost arrows | flying over one throws the ship the way it points | [MM], [G] | chm_t1, chm_t5, chm_t8 |
| Pickup stations | nothing shows at an empty station; a yellow "!!" flashes where a pickup is about to appear; flying into it uses it at once | [W], [G], [W-CH] | chm_08 |
| Bullets | fired by themselves in the four directions for a short time; they hurt other ships | [W] | chm_09 |
| Mines | laid behind by themselves; they hurt on contact, the layer too if it loops back into them | [W], [G] | chm_09 |
| Fireballs | two circle the ship for a while and hurt any ship they touch | [W], [G] | chm_09 |
| Big slash | knocks ships much further | [W], [G] | chm_09 |
| Payload | a ball on a chain for a few seconds, then it blows up, throws the other ships near it back and leaves three fires in a spread triangle; fires hurt and slow; the payload never turns on its own ship | [W], [G] | chm_09, chm_20 |
| Six ships, one screen | the whole track on one screen, all six ships on it | [STATIC], [TVT] | chm_01 |
| HUD | top left: the running order and each ship's laps left | [MM] | (drawn) |
| Points | 9, 7, 5, 3, 2, 1 by place | [G] | chm_10 |
| The grid | bunched just past the line; the winner starts the next race at the back | [POPCAR], [ST-BEATEN] | chm_10 |
| The tournament | 8 races on the 8 tracks in order; most points wins | [W], [G] | chm_11 |
| CPU pilots | slower than a good player, by a pace set for each track; their aggression is drawn at random for each race: sometimes they let you by, sometimes one turns back to hunt you; once you lap them only a hunter still bothers you | [LIZ], [G], [ST-BEATEN], [POPCAR] | chm_16, chm_21 |
| The opening scrum | the first seconds are a scramble of slashes | [POPCAR], [LIZ] | chm_16 |
| TWIN FLUE | the CPUs run it badly and go after ships instead | [G] | chm_16, chm_21, chm_t6 |
| THE DIPPER | the dip is everyone's line; the slot is a short, risky bypass | [G] | chm_22 |
| GRAND OCTAVE | the most even race; a ship on its own flies it clean | [G] | chm_t8 |
| The side route | CROOKED MILE's tunnel beats the CPUs, who never take it | [G] | chm_t7 |
| 2P | two players race at once on one screen with their own pads | [W], [ST-1P], [STATIC] | chm_13 |
| Pilots | six, the same ship in six colours, each with an ending | [W], [LIZ] | chm_01, chm_18 |
| Ending | the result, the winner's own ending, the credits and THE END; a CPU's cup goes back to the title | [LIZ], [W] | chm_11, chm_18, chm_19 |
| Goals | Beacon: 1st place in a race; Saucer: win the tournament; Alien: 1st place in all 8 races | [W], [GGC] | chm_11, chm_12 |
| Stats | P1 and P2 average lap and average race, on the title | [W] | chm_12, chm_13, chm_18 |
| No save mid-run | a tournament is played in one sitting; START pauses | [ST-SAVES], [G] | chm_12, chm_17 |
| The code | LOOT-GALE: stations stock and restock four times as fast | [W], [W-CH] | chm_14 |
| The meta page | LAST-LAMP shows a tired note | [W], [W-META] | chm_14 |
| No out-of-bounds bug | see Readings | [ST-OOB] | chm_15 |

### Readings we had to choose

- **Flight** (1/256 px and frames): gravity 20, thrust 50 (30 up net),
  steering 16, or 24 while thrust is held too (thrust "can help accelerate
  the saucer if applied directionally" [G]), a drift that slows by 3 a
  frame, top speeds 560 across (2.2 px a frame), 512 up and 704 down. A
  ship pushed faster (a boost, a knock) slows back by 14 a frame; nothing
  goes over 7 px a frame. The hit box is 8 × 8. Only "the same as
  Campanella" is known [STATIC]; these give the heavy, floaty handling the
  reviews describe. With thrust and a steer held together the ship leans
  that way.
- **Walls** bounce a ship back at 43 % of its speed (at least 0.63 px a
  frame). After any hit nothing hurts the ship for 45 frames; a relaunched
  ship is safe for 60.
- **Destroyed**: the replacement launches 80 frames later from the hatch one
  tile past the start line, rising at 1.5 px a frame; the lap clock keeps
  running.
- **The slash** reaches 19 px to the facing side and 20 px up and down (the
  big slash 24 and 26), hits in the first 8 of its 14 frames and can be
  used again after 24. It knocks at 3.6 px a frame (the big slash 6.4) with
  a little lift, and the ship it hits can't steer for 16 frames (26); it
  can still thrust. While the blade is out (8 frames) the slasher's
  downward speed loses an eighth a frame, so it falls about a third
  slower over the swipe: Campanella's slash "slightly slows down your
  falling rate" [MM-CAMP], and the two games share their controls [W-CAMP].
- **Ramming**: ships push apart and trade their speed along the push (90 %).
- **Laps**: each track has two to seven checkpoint lines, passed in order.
  On a fork a checkpoint is a line in each branch, so either branch counts.
- **Stations**: fixed spots on each track, drawn only once the "!!" shows;
  the first pickup comes 4 to 7 s after the start, the next 8 to 14 s after
  one is taken (with LOOT-GALE, a quarter of both); the "!!" shows for 1 s.
  The pickup is one of the five at random. A new pickup replaces one still
  working.
- **How long pickups last**: bullets 4 s (four shots every 16 frames at
  3.5 px a frame); mines 4 s (one every 20 frames, armed after half a
  second, gone after 3 s, before even the fastest lap (5 s) comes round
  again, so a layer only meets its own mines by turning back); fireballs 5 s
  (two, 14 px out, a turn every 32 frames); the big slash 6 s; the payload
  3 s, then the blast throws every other ship within 36 px clear at 4 px a
  frame (no damage; its own ship is left alone) and leaves three fires
  16 px out for 3 s ("a short time" [G]): the point of the triangle faces
  away from the ship that towed it, and its own fires spare it for their
  first second. A ship in a fire is hurt and can go no faster than 40 % of
  its top speeds while it is in it, so it flies on out with one hit.
- **Boost arrows** set a ship's speed to 4.2 px a frame their way.
- **The grid**: two rows of three past the line. The first race's grid is
  drawn by lot; after that the last race's finish runs the other way (one
  player's report says a winner always starts last [ST-BEATEN]).
- **The start**: a 3-2-1 countdown of two seconds.
- **The finish**: a ship that finishes leaves the track. The race ends two
  seconds after every player has finished; ships still racing are placed by
  the running order.
- **Ties** on points go to more wins, then more seconds and so on, then the
  better place in the last race.
- **Goals in 2P**: either player earns them.
- **2P pilots**: the two players can't fly the same pilot (six ships, six
  colours).
- **CPU pilots**: each has a top speed for the race of 370–440 (66–79 % of
  a ship's), times the track's pace: PRELUDE RING 75 %, TWIN FLUE 88 %,
  GRAND OCTAVE 102 %, the rest 100 %. Against the demo pilot's clean runs
  that makes them 65 % as fast on PRELUDE RING (it laps them more than
  once [G]), 79 % on GRAND OCTAVE (the most even race [G]), 75 % on
  HOURGLASS (where "the AI keeps pace" [G]) and 47 % on CROOKED MILE (whose
  tunnel "chumps" them [G]). Their temper is drawn once for each race
  ("sometimes they just let you run by … sometimes they literally turn
  around" [ST-BEATEN]): calm half the time (nobody slashes after the
  scrum), mixed 35 % (each CPU jostles or not, even odds), mean 15 % (one
  hunter, the rest jostle); TWIN FLUE is always mean, with two hunters.
  For the first 5 s everyone jostles, a slash one frame in ten for a ship
  in reach. A jostler slashes a player in reach one frame in twenty and a
  CPU one in forty, and never a ship that is a lap or more ahead of it:
  once you are clear you are left alone [POPCAR]. A hunter goes for a
  player within 110 px (on TWIN FLUE any ship), turning back if it must,
  and slashes whenever it can, lapping or not; once a knock lands it lets
  its prey go for 4 s, and it gives up past 165 px. They fly by following
  the track's distance field (`chime_ai.c`) and mind only the walls.
- **The demo pilot** in the tests is the same pilot at full speed on each
  track's fastest line, standing in for a player: it also watches for what
  a player sees coming (mines, fires, fireballs and the ships just ahead)
  and edges round them or eases off, and when there is no clear way it
  just flies on.
- **The damage meter**: the manual says the damage is shown by "an
  indicator that appears above your ship whenever this changes" [MM], and
  the guide says a ship smokes on its last hit point [G]. Ours pops three
  pips up over the ship for a second when its hit points change (a hit, a
  lap's mend, a relaunch), and a ship on its last hit point smokes.
- **LOOT-GALE** is our own name for the more-pickups code, typed on the
  title's CODE screen. As with the collection's cheats [CONV], a tournament
  begun with it on earns no goals or stats. The credits mention it.
- **LAST-LAMP** shows a note taped inside the cartridge, in our words: a
  tired pilot-tester's note with no signature. The credits mention it too.
- **Saving**: only the stats and records (cups and races won, the best
  score, the last pilots picked). The stats are the average of every lap
  and every finished race each player has flown.
- **The out-of-bounds bug**: in the original a big slash can push a ship
  through a wall into the HUD area, where it stays stuck [ST-OOB]. Ours
  moves ships in half-pixel steps, so a ship can't pass through a wall; and
  a ship that ever ends up outside the track or inside a wall is wrecked
  and relaunched like any other. This is a bug fix, not an addition.

## Review fixes

An independent review (2026-09-30) found the scale and the rules faithful
but the feel off: the original is "definitely the easiest" [ST-CHERRY], and
once you are ahead you are "not bothered again" [POPCAR], while ours made
demolition derbies of three tracks. What changed:

| Finding | Fix |
|---|---|
| F1 the payload hurt its own ship | the blast skips its own ship; the fire triangle points away from it; its own fires spare it for their first second; a fire now slows a ship (40 % of its top speeds while in it) instead of all but stopping it, so a ship flies on out with one hit |
| F2 PRELUDE RING was the hardest race to win | a CPU pace for each track; PRELUDE RING's is 75 %, so a clean player laps the field more than once |
| F3 every race a scrum, the leader never left alone | the CPUs' temper is drawn once a race (calm 50 %, mixed 35 %, mean 15 %; TWIN FLUE always mean); a jostler slashes one frame in twenty and never at a ship lapping it; one hunter at most (two on TWIN FLUE), and a hunter that lands a knock leaves its prey for 4 s |
| F4 mines outlived a lap | mines last 3 s; fires too |
| F5 GRAND OCTAVE was the most chaotic track | its top loop's lanes and the crossing are seven tiles or more across; a ship alone flies it clean; its CPU pace is the closest of all |
| F6 NEEDLE'S EYE never let a leader be | the side shafts are four tiles wide (F3 and F4 did the rest) |
| F7 THE DIPPER's slot was the only line | the dip is the main line for everyone; the slot is three tiles high with a two-tile step: rushed, a ship hits the step; flown well it saves about half a second |
| F8 thrust didn't help sideways | steering is 24 instead of 16 while thrust is held; the ship leans that way |
| F9 the slash didn't slow the fall | a slash takes an eighth off the fall each of its 8 active frames |
| F10 an always-on damage lamp | a three-pip meter pops up over the ship for a second when its hit points change |
| F11 stations shown when empty | nothing is drawn until the "!!" |
| F12 a knocked ship lost its thrust | a knock locks the steering only |
| F13 CROOKED MILE's card hinted at the tunnel | "A MILE OF BENDS." |
| Nits | LOOT-GALE's first pickup comes in a quarter of the time, as written; the LAST-LAMP note no longer mentions how long the game took to make, and has no signature. TWIN FLUE still opens with a climb (optional; kept) |

The review's probes (`p8_stats` and `p3_all_tracks`, the full-pace demo
pilot against five CPUs, 56 races over the eight tracks), before and
after:

| | Before | Game fixes only | With the demo pilot's eyes |
|---|---|---|---|
| Races won by the demo pilot | 40 of 56 | 49 of 56 | 52 of 56 |
| Its wrecks a race (mean) | 4.34 | 1.18 | 0.73 |
| Its race (mean) | 90.6 s | 65.0 s | 63.3 s |
| The longest track's race | 130.6 s (GRAND OCTAVE) | 91.0 s (GRAND OCTAVE) | 86.7 s (GRAND OCTAVE) |
| The CPUs' wrecks a race (mean, each) | 5.17 | 2.33 | 1.80 |

("The demo pilot's eyes": it now also watches for mines, fires and the
ships ahead, as a player would; the CPU pilots don't.)

## What is ours

- **Name:** CHIME CIRCUIT (1985, Beamdown Softworks), the race for the Chime
  Cup.
- **The ship:** the chime ship, a little bell-shaped craft with a porthole,
  a trim band and a thruster under its skirt; our own drawing.
- **The pilots:** Ansel in the *Tinkler* and his sister Clary in the
  *Clarion* (the original has its hero and his sister), and four visitors
  from other UFO 40 cartridges (the original's other four are cameos from
  other UFO 50 games): Wick and Tanger from HOMESPUN, Kip from SKYWELL and
  Zorp from WOBBLE DERBY. Each has an ending.
- **The tracks:** PRELUDE RING, THE DIPPER, NEEDLE'S EYE, HOURGLASS, FORKED
  REED, TWIN FLUE, CROOKED MILE and GRAND OCTAVE: all eight layouts and
  their meadow, canyon, crystal cave, desert, marsh, foundry, garden and
  night skies.
- **Music:** "Chime Circuit" (title), "Pit Lane" (menus and cards), "Full
  Thrust", "Updraft", "Pinball Pack" (races), "Grand Octave" (the last
  race), "The Cup", "Homeward" (endings and credits), and two jingles,
  "Chequered Flag" and "Also Ran".
- **Words:** every label, the track cards, the endings, the credits, the
  code and the page.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
UFO 40 goals, which are The Big Bell Race's own (gift, gold, cherry). The
code and the meta page are the original's, in our own words (UFO 40 has no
terminal, so the code is typed on the cartridge's own CODE screen). The
out-of-bounds respawn is a bug fix (see Readings).

## Controls

| Input | Action | Source |
|---|---|---|
| D-pad left / right | steer (the ship faces the way it last steered) | [MM] |
| A (hold) | thrust up; tap to hover | [MM], [W-CAMP] |
| B | slash to the side the ship faces | [MM] (NOT CONFIRMED: which side; "in front of you" [MM], "to the left or right" [W-CAMP]) |
| D-pad up / down | nothing in a race | NOT CONFIRMED: no source gives them a role |
| START | pause | [CONV], [G] |
| 2P | the second pad (or the other half of the keyboard) flies ship two | [W] (the pad split is UFO 40's) |
| Menus | d-pad moves, A picks, B goes back; on the CODE screen up and down change a letter | UFO 40's |

## Not confirmed

| Reading | Why it is flagged |
|---|---|
| The slash slows the fall (about a third over the swipe) | Campanella's does [MM-CAMP], and the two share controls [W-CAMP]; nobody says so for this game |
| Thrust with a steer: 24 instead of 16 | "can help accelerate the saucer if applied directionally" [G]; no number |
| Walls bounce a ship back | sources only say walls hurt; the pinball scrum suggests bouncing |
| How laps are checked (checkpoints) | not described anywhere |
| The grid runs backwards after each race | one player's report [ST-BEATEN] |
| A countdown at the start | not described |
| Every number in Readings (speeds, times, reaches, durations) | only "the same as Campanella", "a short time", "a few seconds" |
| Stations' places and timing | the code's wording says "stations" [W-CH]; nothing else is known |
| A new pickup replaces the one in use | not described |
| How the damage meter looks (three pips, a second) | "an indicator that appears above your ship whenever this changes" [MM] |
| A knocked ship can't steer (but can thrust) for a moment | only "can send them flying into obstacles" [MM] |
| The CPUs' tempers (half calm, a third mixed, the rest mean) and each track's CPU pace | only "sometimes … sometimes" [ST-BEATEN] and the guide's remarks on each track [G] |
| A hunter lets its prey go for 4 s after a knock | not described |
| A fire slows a ship to 40 % of its top speeds while it is in it | "damage your saucer and slow it down" [G] |
| The tie-break on points | unknown |
| The race ends after the players finish; the rest are placed by order | unknown |
| Goals count in 2P for either player | unknown |
| The two players can't pick the same pilot | unknown |
| The code turns goals off | a collection-wide rule for cheats [CONV], not confirmed for this code |

## Tests

`tests/chm_01` … `chm_22` drive the rules with button presses, or set up a
moment with cheats (a ship placed, a pickup given, a finishing order) and
then play it: flight (`chm_02`), walls and mercy (`chm_03`), the slash
(`chm_04`), ramming (`chm_05`), wrecks and relaunching (`chm_06`), laps,
checkpoints and the lap heal (`chm_07`), stations (`chm_08`), the five
pickups and fires (`chm_09`), points and the reversed grid (`chm_10`), a
whole tournament with a tie broken on wins (`chm_11`), goals and saving
(`chm_12`), two players (`chm_13`), the code and the page (`chm_14`), the
out-of-bounds fix (`chm_15`), the CPUs and the scrum (`chm_16`), pausing
(`chm_17`), losing the cup (`chm_19`), the payload sparing its own ship
(`chm_20`), the CPUs' temper for a race and who they slash (`chm_21`) and
THE DIPPER's dip and slot (`chm_22`). `chm_t1` … `chm_t8` have the demo
pilot fly all eight laps of each track against the five CPUs with real
button presses (`chm_t8` also flies GRAND OCTAVE alone, clean), and
`chm_18` plays a whole tournament from the title (every menu and every
lap) and wins the Chime Cup. The demo pilot is `chm_ai` with `bot = true`, read through
the cartridge's `bot` query.

## Sources

- [W] UFO 50 Wiki (Miraheze), "The Big Bell Race" (raw page): the gameplay,
  the five power-ups, the six characters, the stats, the goals, the cheat
  and the meta message. https://ufo50.miraheze.org/wiki/The_Big_Bell_Race
- [W-CAMP] Miraheze, "Campanella": thrust and steer, the slash to the left
  or right, hovering by tapping. https://ufo50.miraheze.org/wiki/Campanella
- [W-CH] Miraheze, "Cheats": KIWI-AURA, "power up stations have a quicker
  refresh rate". https://ufo50.miraheze.org/wiki/Cheats
- [W-META] Miraheze, "Meta Messages". https://ufo50.miraheze.org/wiki/Meta_Messages
- [MM], [MM-CAMP] Steam guide "The missing manuals - How to play UFO 50
  games": falling by default, A to thrust, B to slash, 3 hits and an
  indicator, a hit restored each lap, relaunching at the start, the icons,
  the red arrows, the HUD; Campanella's slash slows the fall.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [G] Steam guide "The Big Bell Race - Guide and Tips": three pips, walls,
  knocks don't hurt, smoke on the last pip, the launcher under the start,
  invulnerability, 8 races of 8 laps, the points, the five collectibles,
  and what each of the eight tracks is like.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3339239159
- [GGC] Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [ST-BEATEN] Steam thread "To those who have beaten one or more of the
  games…": a winner starts last; the opponents' aggression is random; 12
  minutes for a first clean sweep.
  https://steamcommunity.com/app/1147860/discussions/0/4852154959746797715/?ctp=5
- [ST-CHERRY] Steam thread "Easiest and hardest game to Cherry?": "definitely
  the easiest … managed to win every race".
  https://steamcommunity.com/app/1147860/discussions/0/604150365670110300/
- [ST-OOB] Steam thread "Big Bell Race out of bounds glitch".
  https://steamcommunity.com/app/1147860/discussions/1/595136072544997971/
- [ST-1P] Steam thread on the 1P/2P choice.
  https://steamcommunity.com/app/1147860/discussions/0/508449585248645390/
- [ST-SAVES] Steam thread "Which games save progress?".
  https://steamcommunity.com/app/1147860/discussions/0/4849904176633630339/
- [STATIC], [STATIC-CAMP] The UFO 50 Diaries (Static Canvas), "The Big Bell
  Race" and "Campanella": one screen, knockback, the same physics, heft
  and momentum.
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-the-big-bell-race
- [LIZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 19": the AI is
  slower, the pilots are cosmetic, the ending.
  https://lizstar64.github.io/reviews/2024/10/14/UFO50-19.html
- [POPCAR] Popcar's Blog, "Reviewing Every Single UFO 50 Game": the scrum,
  no traps. https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [SR] speedrun.com, UFO 50, The Big Bell Race: 7:51.
  https://www.speedrun.com/UFO_50/runs/y2wlk3wy
- [TVT], [SEARCH-CAMP] search-engine summaries of the TV Tropes recap ("six
  UFOs", no fuel) and of Campanella's controls (inertia).
- [CONV] the UFO 40 research notes on the collection's conventions
  (`00-ufo50-conventions.md`): cheats disable achievements.
