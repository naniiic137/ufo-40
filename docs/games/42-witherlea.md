# 42 · WITHERLEA

*Internal design document. Not shown in the product.*

## Tribute to

**Night Manor** (UFO 50 game #42, Mossmouth, "September 1988"). The rules
were researched from text only: the community wiki's page, the Steam guides
"Night Manor - 100% with Full Map" and "Gift, Gold & Cherry", Steam threads,
the search-result text of a speedrun guide and written reviews (listed under
Sources; the full notes are in the research folder, `42-night-manor.md`).
No UFO 50 images, video, sprites, music or text were used, and nothing was
taken from a UFO 50 install.

**What was read, and what we made.** The sources include the original's
item table and walkthrough. We used them for the structure only: the counts,
the order in which the house opens up and the shape of the gates (the cell
first, the car route open early, a light for upstairs, four tokens for the
hedge maze, the bolt at its heart for the crossbow, the crossbow-only kill
and the key it drops, a four-part repair in the basement, a three-part poison
for the fungus). Every puzzle inside that shape is ours: the containers, the
clues, the combinations, which room holds what, the set pieces and the item
list and its order. An earlier version of this cartridge followed the
original's walkthrough with the nouns changed; the review caught it, and the
puzzles were redesigned (see "Our puzzle graph").

**Map type: fixed.** Night Manor's house, items and answers are the same
every game; only the killer's timing reacts to what you do [MH]. Ours is
fixed too (`wth_spots.h`, `wth_logic.c`).

| Full scale | Night Manor | WITHERLEA |
|---|---|---|
| Players | 1, no 2-player mode of any kind [MH] | 1 |
| View | first person, one still picture a room, exits change the cursor [MOE], [STEAM-WAY] | the same: 31 pictures, an arrow cursor that turns into a direction arrow over a way out |
| Places | 31 named areas on the guide's map: 11 ground-floor rooms, 10 outdoor areas and annexes, 8 upstairs, 2 in the basement [100PCT] | 31: 12 ground-floor rooms; the garage, the pond and the potting shed; the back garden, the side yard, the front drive and the hedge court; the hedges and their heart; 8 upstairs; the cellar and the white room |
| Things | 54 kinds on the wiki's item table [MH] | 54 kinds of our own (`WTH_ITEM`), each with its job, a few with two |
| Journal | 17 pages, out of date order [MH] | 17 pages of Ada Morrow's diary, April to October 1987, found out of order |
| Hiding places | 4 destroyed after one use, 2 that last [MH] | 4 one-use: the kitchen pantry, under the dining table, the big bedroom's wardrobe, the bathroom's airing cupboard; 2 that last: the garage's inspection pit and the side yard's coal bunker |
| Maze tokens | 4, placed in order, open the gate to the hedge maze [MH] | 4 metal bees for the stone hive by the hedge gate, in any order |
| Endings | escape by car (Gold); kill the killer, go down and destroy the fungus, the house burns (Cherry); never caught: the best ending; ever caught: a post-credits face in the rear-view mirror [MH], [STEAM-HIDE] | the same three, with our own words and pictures |
| Goals | 3 [MH], [GGC] | the same 3 |

## Our puzzle graph

- **The cold store.** The door is locked outside, but its hinges are on this
  side: a nail from the salt barrel under a hinge pin, a loose stone from the
  wall to drive it out.
- **The car (open from the start).** The key is sealed into a dead beehive
  (cut the wax with the kitchen knife). The rotor arm Ada hid is in a
  tobacco tin in the side yard's water butt (the scullery drawer's tap key
  drains it; the knife opens the rusted tin). The garage door is chained
  down: bolt cutters from the potting shed, whose number lock is the wedding
  date on the back of the hall photograph. Rotor in, chain cut, door heaved
  up (loud), key turned.
- **Upstairs.** The glasshouse's hurricane lamp, filled with paraffin from
  the garage and lit with matches from the smoking room's pipe rack, held up
  the stairwell.
- **The big bedroom** door is swollen shut: lever it with the poker or lift
  it off its hinges with the screwdriver (the two-way puzzle).
- **The four bees.** Ada's diary gives Tom's riddle: one in the ice (rock
  salt from the kitchen on the scullery's ice block), one down the well
  (cut the grate with the bolt cutters; fish with a loudspeaker magnet from
  the radiogram, taken out with the screwdriver, tied on fishing line from
  the sunken boat's tackle box), one in the horse (the nursery rocking
  horse's screwed belly hatch), one with the wasps (the attic nest: the
  bee smoker from the shed, packed with pine cones from the yard and lit;
  the attic hatch pulled down with the children's window pole). All four in
  the stone hive open the hedge gate.
- **The way in.** From the balcony, army binoculars (from the study bureau,
  whose key hangs on the empty kennel's collar) trace the path: left, left,
  right, straight on, left, right.
- **The heart.** A crossbow bolt is driven deep in an old oak: out with the
  shed's pliers.
- **The crossbow** is behind the smoking room's locked gun cabinet: the poker
  through the glass (loud). The bolt loads it; only it kills the Keeper, who
  drops the black key to the cellar doors on the drive.
- **The cellar.** Tom flooded the inner cellar. The hand pump beside it
  needs its handle (hidden among the hall's walking sticks), a leather
  washer (cut from the bedpost belt with the bathroom razor), a hose (cut
  from the garden reel with the knife) and axle grease (the garage).
- **What kills it.** Tom's tape (taped inside the washroom cistern) plays on
  the study's recorder once the house has power again (the sickroom's blown
  fuse mended with the guest room's fuse wire, back in the scullery fuse box)
  and the empty take-up spool is found in the attic's home movies. It gives
  the recipe: copper sulphate (glasshouse), quicklime (the garage sack, dug
  with the glasshouse trowel, wearing the shed's gloves) and water (the
  scullery pump, in the bucket under its sink), mixed in the shed's rose
  sprayer. Sprayed on the face in the white room: the threads dry to
  tinder, a spark, and the house burns.

## Mechanics checklist

| Mechanic | How WITHERLEA does it | Source | Test |
|---|---|---|---|
| The cursor | the pad moves it eight ways, slowly (1.25 px a frame) | [SR-SNIP], [MH] | wth_02 |
| Scared | while he is in the room, following, or down: about half the speed (0.69 px), and it trembles | [MOE], [100PCT], [STATIC] | wth_02 |
| No puzzles while scared | in that state, A on a thing or a floor item, and combining in the bag, only give a "too shaky" line; ways out, hiding places, the bag and a weapon on him still work | [MOE] | wth_26 |
| Looking, taking, using | A on a hotspot looks or takes; with a thing held, A uses it there; A on a way out walks through | [MOE], [100PCT] | wth_03 |
| Text | messages page on with A | [SR-SNIP] | wth_03 |
| The bag | A on a thing holds it as the cursor; held, A on another thing puts them together; a thing leaves when its job is done and the next one found fills the first hole | [100PCT], [STEAM-JOURNAL] | wth_04 |
| Dark rooms | upstairs and the cellar: a ring of dithered light round the hand once the lamp is lit; not up the stairs before | [MOE], [MH] | wth_14 |
| Noise | the garage door, the car started without getting away, the radiogram (once there's power), either toilet, breaking glass (the gun cabinet, the glasshouse roof), the pianola, the gramophone, a wrong turn in the hedges | [MH] | wth_05, wth_06, wth_15 |
| After a noise | he comes 2.5 s later (6.5 s after a wrong turn); the countdown stops while the rooms change and while you hold a thing; more noises meanwhile don't restart it | [SR-SNIP], [MH] | wth_05, wth_06 |
| Timed visits | he also comes when enough time has gone by, longer after each escape | [MH], [100PCT] | wth_07 |
| In your room | he walks up to you; 4 s and he has you | [100PCT] | wth_08, wth_13 |
| Running | he follows a room behind; four rooms in a row and he loses you; turn back into him and he is on you | [100PCT], [STEAM-MH-OPS] | wth_08 |
| Hiding | A on a hiding place with him near: a mark sweeps a bar, A while it is in the box; each of the eight goes faster with a smaller box; three misses and he finds you; misses still count, so two can be missed | [100PCT], [STEAM-HIDE] | wth_09, wth_10 |
| Hiding places | 4 wrecked after one use, 2 that last | [MH] | wth_09, wth_11 |
| Stunning | the knife or the poker on him knocks him down; leave the room and he gives up; each works twice, and the third blow with the same one he blocks and has you | [STEAM-MH-OPS], [100PCT] | wth_12 |
| Caught | the billhook, black, and you wake on the straw in the cold store with everything kept and every puzzle still solved | [MH], [MOE], [LIZ] | wth_13 |
| Killing him | only the loaded crossbow; he drops the key to the cellar; nothing comes after that | [MH], [100PCT] | wth_23 |
| A two-way puzzle | the big bedroom door: the poker or the screwdriver | [STEAM-WAY] | wth_22 |
| The maze | directions seen from the balcony; six junctions; wrong turn: back to the start, and the noise | [MH], [100PCT] | wth_06 |
| The maze tokens | four bees in the stone hive | [MH] | wth_21 |
| Saving | the house saves itself (every room, every change); NEW GAME / CONTINUE on the title | [STEAM-SAVES], [100PCT] | wth_17 |
| Goals | Beacon: escape the Keeper; Saucer: drive away (the car); Alien: kill what grows under the house (the cellar). Being caught doesn't stop the Alien | [MH], [GGC], [STEAM-HIDE] | wth_08, wth_15, wth_16 |
| Endings | car; cellar, the house burning; cellar never caught (a fourth page and its own tune); ever caught: the mirror after the credits | [MH] | wth_15, wth_16 |
| Playable start to end | the demo player, from the warning screen with real button presses: the car; the cellar with all 17 pages never caught; the cellar after being caught once | | wth_18, wth_19, wth_20 |
| Size | 31 places, 54 things, 17 pages; words fit; tunes loop together | [MH], [100PCT] | wth_24 |

### Readings we had to choose

- **Buttons.** Sources only say A presses take things and clear text. We put
  look / use / go and the hiding press on A, and the bag on B (B again closes
  it). The bag is a grid moved on with the pad, not the free cursor.
- **The first timed visit** is 90 s of play after leaving the cold store;
  each escape adds 30 s, up to 180 s; after a capture the wait starts again
  from what it was. The timed visit gives the same 2.5 s warning as a noise
  (and the same pausing).
- **The holding pause** stops only the countdown to his coming, not his
  walk up to you once he is in the room, nor the timed wait.
- **In your room:** 4 s (240 frames) to get out; he reaches the next room
  3.5 s after you (only frames outside a room change count). Doubling back
  into the room he is in leaves you 2 s.
- **Running:** "about 4 rooms" is read as four room changes in a row, each
  before he arrives, never back into him.
- **Stuns:** "a couple of times" with the knife is read as twice with each
  weapon; the third blow with the same weapon is blocked. Down, he gets up
  after 4 s. The counts are kept in the save.
- **Scared:** from his arrival until he is lost, hidden from or dead, the
  hand can't take, use or combine things; the bag still opens so a weapon can
  be held.
- **The breathing game:** 8 marks; the mark speeds from 1.0 to 2.9 px a
  frame and the box shrinks from 28 to 8 px on a 160 px bar; a mark that
  crosses the bar and back with no press is a miss. Hiding is possible from
  the moment he is coming; whatever was in hand goes back in the bag.
- **Stinger and best ending:** the mirror follows either ending if you were
  ever caught; the extra "never caught" page and tune come only with the
  cellar ending.
- **After an ending,** CONTINUE goes back to the moment before the final act,
  so the other ending can still be played.
- **The hedges' wrong turn** sends you back to the first junction.
- **The maze tokens** go in in any order (the original's go in a set order;
  our stone hive takes each bee in a cell of its own shape).

## What is ours

- **Name:** WITHERLEA (1988), a house on the fens.
- **The story:** you drove home angry from a harvest supper, hit something
  pale in the road and woke in a cold store. The Morrows (Ada, Tom, Pip and
  Barnaby) moved in in April 1987; the old man who planted the hedge garden
  locked it with four brass bees; Tom's bees fed on a white growth in the
  well, the pale honey made everyone calm, and then it made them something
  else. Tom hid the bees, flooded the cellar and taped how to kill it; Ada hid
  the car's rotor arm. The Keeper is Tom in his bee veil, with a billhook.
- **The house:** all 31 pictures, every hotspot and every puzzle in "Our
  puzzle graph", and the set pieces: the hinges on the wrong side, the
  frozen bee, the sealed hives, the white man behind the shower curtain, the
  wasps' nest, the old oak full of holes, the flooded cellar, the burning
  house.
- **The 54 things** and their words; Ada's 17 diary pages; the warning, the
  opening, the waking, both endings, the mirror and the credits.
- **Music:** "Witherlea" (title), "The Quiet House", "Lights Out" (upstairs),
  "The Grated Well" (outside), "The Keeper" (the chase), "Hold Your
  Breath" (hiding), "Under the House" (the cellar), "The Hedges", "The Lane
  Home" (ending), "Daybreak" (the best ending), and two jingles: "Bad Dream"
  (caught) and "In the Mirror".

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
UFO 40 goals, which are Night Manor's own (gift, gold, cherry). The OPEN-CAGE
Terminal code (the killer swapped for a giant hamster) is a UFO 50 collection
feature; UFO 40 has no terminal, so it is left out.

## Controls

The owner's layout puts the main action on **A**. Sources don't confirm
which of the original's two buttons opens the inventory; we read it as
Button 1 (B here). The cartridge card's CONTROLS page can swap A and B.

| Input | Action |
|---|---|
| D-pad | move the hand, eight ways (slower and shaking when he's near) |
| A | look / take / use the held thing / go through a way out; next page of text |
| A (hiding) | breathe: press when the mark is in the box |
| B | open the bag; in the bag: close it |
| A in the bag | hold a thing; held, A on another puts them together; A on the held thing puts it back; A on the diary reads it |
| D-pad in the bag / diary | choose a thing / turn pages |
| START | pause |

## Not confirmed

| Control or rule | Our reading |
|---|---|
| Which button opens the inventory and cancels | B opens and closes the bag; A on the held thing puts it back |
| Moving in the inventory | the pad steps a selection box, not the free cursor |
| The hiding press | A |
| Base timer and its growth | 90 s, +30 s an escape, 180 s at most |
| Number of breaths, speeds, box sizes | 8; 1.0 to 2.9 px a frame; 28 to 8 px |
| The stun rule | twice with each weapon, the third blocked |
| How "4 rooms away" is counted | four room changes in a row without turning back into him |
| Whether he follows visibly and how fast | not seen; he reaches your room 3.5 s after you |
| Save granularity | every room and every change |
| After he is dead | no more visits, noises answered by nothing |
| Every number in "Readings we had to choose" | as listed |

## Tests

`tests/wth_01` … `wth_26` drive each rule with button presses, setting up a
moment with cheats where that saves walking. The demo player (`wth_bot.c`)
plays from the warning screen with real presses: it moves the hand with the
pad, opens the bag with B, holds and combines things with A, and finds its
own way between rooms; when the Keeper comes it hides if the room has
somewhere left to hide (and times every breath), otherwise it runs, never
back into him and away from dead ends; with the loaded crossbow it makes a
noise and waits for him. `wth_18` drives away in the car; `wth_19` plays the
whole house (all 17 pages, the four bees, the hedges, the Keeper shot, the
cellar) to the best ending without being caught; `wth_20` lets him catch it
once and still ends it, mirror and all.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Night Manor": the item count (54), the 17
  journal entries, the goals, the noise list, the hiding places, the endings
  and the stinger. https://ufo50.miraheze.org/wiki/Night_Manor
- [GGC] Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [100PCT] Steam guide "Night Manor - 100% with Full Map": the 31 named
  areas, the cursor, the hiding game ("speeds up and shrinks the safe zone"),
  stunning ("give up when you move onto the next screen"), outrunning
  ("about 4"), idle time bringing him, the inventory filling holes.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3336774679
- [STEAM-HIDE] Steam thread "Night manor hidding minigame?": three misses;
  missed prompts count toward finishing; the stinger doesn't block the cherry.
  https://steamcommunity.com/app/1147860/discussions/0/508449060926009589/
- [STEAM-MH-OPS] Steam thread on the killer: stab a couple of times, outrun
  with a lap, a bush that lasts.
  https://steamcommunity.com/app/1147860/discussions/0/4699034745343599801
- [STEAM-WAY] Steam thread: the MacVenture and Clock Tower influences, a
  puzzle with two solutions.
  https://steamcommunity.com/app/1147860/discussions/0/4846527362887187269
- [STEAM-JOURNAL] Steam thread: the d-pad cursor, the inventory order.
  https://steamcommunity.com/app/1147860/discussions/0/4849904176638025723/
- [STEAM-SAVES] Steam thread on which games save.
  https://steamcommunity.com/app/1147860/discussions/0/4849904176633630339/
- [SR-SNIP] Search-result text of the speedrun.com "Night Manor Cherry
  Guide": 2.5 s and 6.5 s, the pause rules, eight directions, A presses (the
  page itself refused the fetch). https://www.speedrun.com/UFO_50/guides/tc6v4
- [MOE] MoeGamer, "UFO 50: Night Manor's horrific adventuring": the scared
  state stops puzzles and items.
  https://moegamer.net/2026/02/27/ufo-50-night-manors-horrific-adventuring-complimentary/
- [STATIC] Static Canvas, "The UFO 50 Diaries: Night Manor".
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-night-manor
- [LIZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 42".
  https://lizstar64.github.io/reviews/2024/10/19/UFO50-42.html
