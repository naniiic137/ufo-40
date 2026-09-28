# 04 · WET PAINT

*Internal design document. Not shown in the product.*

## Tribute to

**Paint Chase** (UFO 50 game #4, Mossmouth). The rules were researched from
text only: the community wiki, Steam guides and threads, and written reviews
(listed under Sources; the full notes are in the research folder,
`04-paint-chase.md`). No UFO 50 images, video, sprites, music, course
layouts or text were used as references.

**Map type: fixed.** Paint Chase is 25 hand-made one-screen courses and a
final showdown, so ours are too (`wetpaint_courses.c`, all our own layouts).
Only the structure follows the original:

| Full scale | Paint Chase | WET PAINT |
|---|---|---|
| Courses | 25, then the final showdown [W] | 25, then the final showdown (26 in all) |
| Foe types | 9 plus the rival racer [W] | 9 plus Foxy Fuchsia |
| Course elements | 7: spawners, boost tiles, toggle blocks and switch, conveyors, bumpers, spike walls, timed blocks [W] | the same 7 |
| Power-ups | 4: paint gun, spike gun, drone, clock [W] | the same 4 |
| Modes | 1 player, 2P versus [W] | 1 player, 2P versus |
| Cutscenes | one introduces each group of courses [MG], [LZ], [EXP] | 6: before courses 1, 6, 11, 16, 21 and the final, plus the ending |
| New ideas | "you can barely go a level without them introducing a new concept" [EXP] | 16 of the 25 courses bring something new (below) |
| Difficulty | easy first 15, a spike in the last 5 [PC], [MG] | the demo driver passes courses 1-4 nine times in ten, the last five about half the time |
| Length | about 20+ minutes a full game [PC] | 45 s a course (60 s the final), about 22 minutes with the screens between |
| Goals | 3 [W] | the same 3 |
| Secret | a message for parking in course 1's corner [MMSG] | our own message, same trigger |

What each course brings: 1 smudgers; 2 boost arrows; 3 the sprinkler; 4
dusters; 5 swing barriers and their lever; 6 tankers; 7 gloops; 8 bumpers
and the tack shooter; 9 poppers; 10 conveyor belts and the helper; 11
hedgehogs; 12 thorn hedges; 13 the freeze pop; 14 bollards; 15 everything so
far; 16 conkers; 17-20 mixes; 21 jellies; 22-25 the hardest mixes; 26
Foxy.

## Mechanics checklist

| Mechanic | How WET PAINT does it | Source | Test |
|---|---|---|---|
| Controls | the d-pad only; A and B do nothing in play | [MM], [SC] | wp_01, wp_02 |
| Always moving | the car drives on by itself from tile to tile and speeds up from a standstill | [W] | wp_01 |
| Painting | every tile the car crosses turns blue | [W] | wp_01 |
| No turning round | the pad the other way brakes (down to a crawl), the pad ahead speeds you back up | [MM], [LZ] | wp_02 |
| Walls | driving into one halts you dead until you pick a way (then any open way, even back) | [MM] | wp_01 |
| Turning | a turn waits for the next tile centre, and is forgotten there if that way is shut and the pad is let go | ours, as Magic Garden's grid | wp_02 |
| The clock and the goal | when the clock runs out the blue share of the floor must reach the course's goal | [W], [MM] | wp_03 |
| Points | every per cent over the goal is a point | [W], [LZ] | wp_03 |
| Lives | three; missing the goal costs one and the course starts again; none left is game over, with no continue | [MM], [PC], [MG] | wp_04 |
| Extra lives | one for every 120 points | [TVT] | wp_04 |
| Crashes | never cost a life: the car crumples for a moment, then drives on | [MG] | wp_06 |
| Foes paint pink | over blue too | [W], [MG] | wp_05 |
| Ramming | running into a foe wrecks it | [W], [MM] | wp_05 |
| Garages | flash first, then send foes out the arrow's way in loads of 2 or 3 | [W], [MG] | wp_15 |
| Smudger | the plain one | [W] | wp_05 |
| Duster | flies over walls; left too long it flashes, sprays paint four ways and flies off | [W], [MG] | wp_09 |
| Tanker | twice a smudger's speed, still under yours | [W] | wp_05 |
| Gloop | the little one; comes out with the mother and is laid by her | [W] | wp_10 |
| Mother gloop | twice as fast; now and then slows, lays a gloop and speeds up again | [W], [ST-TIPS] | wp_10 |
| Popper | slower than the rest; left too long it flashes and bursts, painting a big circle pink | [W], [MG] | wp_09 |
| Hedgehog | wrecked only from the side or behind; head-on it stuns you unless you are boosted; two lights show its next turn | [W], [EXP] | wp_06 |
| Conker | lets its spines go four ways when you line up with it; they fly to a wall, stun you and wreck foes they meet; bare, it is harmless | [W] | wp_07 |
| Jelly | wrecked at normal speed or braking; hit boosted, you spin straight to the wall leaving pink, then sit stunned | [W], [EXP] | wp_08 |
| Boost arrows | a boost driven over the arrow's way; boosts on top of each other go faster and faster; foes use them too | [W], [MM], [ST-TIPS] | wp_11 |
| Boosted rams | wreck anything but a jelly | [MG] | wp_06 |
| Bumpers | hit from any side, by you or a foe: turned round and boosted; they stack like arrows | [W], [ST-TIPS] | wp_11 |
| Thorn hedges | hit at normal speed: broken, and you're stunned; boosted: broken, and you drive on | [W] | wp_12 |
| Swing barriers | the lever swings them all (shut ones open, open ones shut); the floor under them counts | [W] | wp_13 |
| Bollards | rise and sink on a steady beat; one rising under you stuns you | [W] | wp_13 |
| Conveyor belts | faster along them, slower against | [W] | wp_13 |
| Power-ups | yellow icons on the floor; used by driving on while the tile isn't blue, so a foe painting it makes it ready again | [W], [ST-PU], [ST-TIPS] | wp_14 |
| Sprinkler | for a while shoots paint ahead, left and right; a shot colours the first tile that isn't blue | [W] | wp_14 |
| Tack shooter | for a while shoots tacks ahead, left and right; they wreck foes | [W] | wp_14 |
| Helper | a little car that drives round painting blue until a foe knocks it out; safe for a moment when it appears | [W] | wp_14 |
| Freeze pop | stops the clock and every foe for 7.5 s | [W] | wp_14 |
| The final | one course against Foxy, who drives, boosts and uses power-ups by your rules; a ram only stuns her for a moment | [W], [search summary] | wp_16 |
| The end | a medal: gold, or platinum with 500 points | [W] | wp_16 |
| Goals | Beacon: pass the first 12 courses; Saucer: pass all 25 and beat Foxy; Alien: win with 500 points | [W] | wp_16, wp_17 |
| Score board | five best scores; a run is never saved half-way; the save also keeps the highest course reached and the lives left after a win (the original's two stats) | [W], [AR] | wp_17 |
| 2P versus | blue against pink, most paint wins | [W], [MP] | wp_18 |
| The corner | park in course 1's bottom-right corner while the pink takes 80 % and the results screen has a message | [MMSG] | wp_19 |
| Cutscenes | a scene opens each group of five courses and the final, and shows the new foes with a word on each | [MG], [LZ], [EXP] | wp_21 |

### Readings we had to choose

- **Numbers no source gives:** your car cruises at a pixel a frame (five
  tiles a second), brakes to 5/16 of that, and takes 16 frames to get back
  up (8 with the pad pushed ahead). A boost lasts a second at 2, 2.5 and
  3 pixels a frame for one, two and three in a row. Foes go 7/16 of your
  speed (tankers and mother gloops twice that, poppers 5/16, conkers 3/8). A stun lasts
  54 frames, then 40 frames when nothing can hit you.
- **Clocks and goals:** 45 s a course and 60 s for the final. Each course's
  goal (42 % to 56 %) was set from the demo driver's results so that the
  early courses are generous and the last five are a spike; the final asks
  for 50 %.
- **Lives:** three at the start, read from "fail 3 times and you start over".
- **Garages:** the first load leaves after a second (the next garage ¾ s
  later), a garage flashes for a second, foes leave 0.4 s apart, and an
  empty garage rests 3 s before the next load. A mother gloop and her
  little one count as two of a load.
- **How foes steer:** never straight back unless they must, and three times
  in four towards a tile that isn't pink yet.
- **Timers:** a duster flies 10 s, then sprays four volleys over 2 s; a
  popper bursts 13 s after it leaves its garage, over every tile within
  about 4½ tiles; a mother gloop slows every 4 s.
- **Power-ups:** the sprinkler and tack shooter last 5 s at five volleys a
  second; the helper drives at ¾ your speed and lasts until knocked out or
  the course ends. Foxy's freeze pop freezes you.
- **The lever** swings the barriers for whichever car drives over it; a
  barrier swinging shut on a car doesn't stun it (only bollards do).
- **Thorn hedge tiles** count in the floor total, since they become floor.
- **Conkers** that are still armed when you touch them (only possible round
  a corner) stun you like a thorn hedge does, unless you are boosted.
- **Rams between racers:** a boosted car stuns an unboosted one; otherwise
  the one hit from behind or the side is stunned, and head-on both are.
- **2P versus:** any of the 26 courses, picked on a map; garages stay shut,
  power-ups stay; the round goes to whoever has more paint; wins are
  counted until you leave.

## What is ours

- **Name:** WET PAINT (1983, Beamdown Softworks), the Town Square Wet Paint
  Rally.
- **Hero:** Bo, in a blue kart with a yellow helmet and goggles.
- **The rival:** Foxy Fuchsia, a fox in a pink hot rod.
- **The Marshal**, who runs the rally and explains the new foes.
- **The foes:** smudgers (paint roller karts), dusters (toy biplanes),
  tankers, gloops and the mother gloop, poppers (paint-bomb carts),
  hedgehogs (spiked ploughs), conkers and jellies.
- **The power-ups:** the sprinkler, the tack shooter, the helper and the
  freeze pop.
- **The course pieces:** garages, chevron arrows, swing barriers and their
  lever, bollards, conveyor belts, red bumpers and thorn hedges.
- **Places:** the town square, the park, the docks, the works, the town by
  night and the final, each with its own walls; all 26 layouts.
- **Music:** "Wet Paint" (title), "First Coat", "Second Coat", "Top Coat",
  "Showdown", "Intermission" and "Platinum", and the jingles.
- **Words:** the cutscenes (with their #@!% swearing), the corner message
  ("THE NIGHT PAINTER SAYS: NICE PARKING.") and the ending.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu, the score board
save and the three UFO 40 goals, which are Paint Chase's own (gift, gold,
cherry). Paint Chase's terminal cheats (all courses open, no foes or clock)
are a UFO 50 collection feature and UFO 40 has no terminal, so they are left
out.

## Controls

| Input | Action |
|---|---|
| D-pad | turn at the next tile; the other way brakes; ahead speeds back up |
| START | pause |
| Title | UP/DOWN and A: 1 player or 2 players; B back to the library |
| 2 players | the second pad (or the other half of the keyboard) drives the pink car |

## Not confirmed

- Every number above: speeds, clocks, goals, how long things last, how many
  foes a load and how often.
- The number of lives at the start (read from a review).
- How the final is won: we read it as the usual goal with Foxy painting
  against you.
- What 2P versus plays on and whether foes appear there.
- Whether an armed spike ball can be rammed, and what bumpers and conveyors
  do to foes exactly.

## Tests

`tests/wp_01` … `wp_21`, all driven by button presses or set up with
cheats and then played. `wp_20_demo_run` is a demo driver (the `bot`
query in `wetpaint_logic.c`): from the title screen, with real presses,
it skips the cutscene and passes the first three courses. It searches the
lanes for the nearest tile that isn't blue, a power-up it can use or a foe
it can safely ram, never plans straight back, and keeps off hedgehog
ploughs, conker lines, thorn hedges and (boosted) jellies. The same driver,
with a little randomness, is Foxy. `wp_21_structure` checks that all 26
courses are sound.

## Sources

- [W] UFO 50 Wiki (Miraheze), "Paint Chase" (raw page): the racer, painting,
  the timer and goal, points over the goal, 25 courses and the Pink Racer,
  every foe, course element and power-up, the goals, the medal, the meta
  message. https://ufo50.miraheze.org/wiki/Paint_Chase
- [MMSG] Miraheze, "Meta Messages": park in course 1's bottom-right corner
  until the enemy reaches 80 %. https://ufo50.miraheze.org/wiki/Meta_Messages
- [MP] Miraheze, "Multiplayer"; [AR] "Arcade" (a high-score sheet).
- [MM] Steam guide "The missing manuals - How to play UFO 50 games": d-pad
  only, brakes the other way, speed up ahead, halt at walls, arrows,
  power-up blocks, a life lost and the stage retried, game over.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [ST-PU] Steam thread "Paint Racer: How do the powerups work?": power-ups
  work when the tile isn't blue.
  https://steamcommunity.com/app/1147860/discussions/0/4849903793443028153/
- [ST-TIPS] Steam thread "any tips for paint chase cherry?": power-ups used
  again after a foe repaints them, white slugs, boosts and bounce orbs stack.
  https://steamcommunity.com/app/1147860/discussions/0/4846527362888785809/
- [MG] MoeGamer, "Paint the town blue with Paint Chase": spawner blocks
  flash, crashing, no way to die, boosted rams, no continue, the last five.
  https://moegamer.net/2024/09/28/ufo-50-paint-the-town-blue-with-paint-chase/
- [LZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 4 - Paint Chase": no
  turning round, points over the goal, cutscenes.
  https://lizstar64.github.io/reviews/2024/10/06/UFO50-4.html
- [PC] Popcar's Blog, "Reviewing Every Single UFO 50 Game": fail 3 times
  and start over, 20+ minutes, the last 10 hard.
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [SC] Static Canvas, "The UFO 50 Diaries: Paint Chase": arrow keys only.
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-paint-chase
- [EXP] exp., "UFO 50 #4: Paint Chase": 25 stages in one go, a new concept
  almost every level, interstitials that explain the foes.
  https://expzine.com/2026/05/07/ufo-50-4-paint-chase-perry-2024/
- [TVT] TV Tropes recap (blocked; a search engine's summary only): an extra
  life every 120 points.
  https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game4PaintChase
- [search summary] a search engine's summary of the pages above: ramming the
  final racer only stuns it.
