# 16 · TINTAIL

*Internal design document. Not shown in the product.*

## Tribute to

**Camouflage** (UFO 50 game #16, Mossmouth). The rules were researched from
text only: the community wiki, the Steam guide "The missing manuals",
written reviews and guides (listed under Sources). No UFO 50 images, video,
sprites, music, level maps or text were used; the wiki's level images and
island map were not opened.

**Map type: fixed, hand-made.** Fifteen hand-made single screens on a
branching island map, every tile ours:

| Structure | Camouflage | TINTAIL |
|---|---|---|
| Levels | 15 single screens | 15 single screens (20 × 10 tiles) |
| Map | an island with branching routes | Salt Island; each level opens the next two |
| Collectibles | two fruit and a baby, all but the last level | two prickly pears and a hatchling on levels 1–14 |
| Finale | level 15, the temple, no collectibles | level 15, the Sun Gate, no collectibles |
| Order of ideas | frogs, then gators (5), logs, switches late | toads, storks from 5, logs at 3, switches from 10 |

## Mechanics checklist

Every line was compared with the code; the tests named prove it.

| Mechanic | How TINTAIL does it | Source | Test |
|---|---|---|---|
| Moving | one tile per press, in real time | [MM], [L] | tn_01, tn_13 |
| Terrain | grass, sand, swamp, rock; dry and wet grass | [W] | tn_03, tn_08 |
| White by default | never hidden until she changes | [W] | tn_01 |
| Changing colour | its own button (A); takes a moment; exposed while changing | [W], [MM] | tn_01, tn_14 |
| One colour at a time | a tile of another colour exposes you | [W] | tn_03 |
| Not in danger | no change while Twig or the hatchling is in danger | [W] | tn_02 |
| Danger view | hold B: every watched tile turns pink; no moving or changing while held | [MM] | tn_14 |
| Frogs (toads) | sit still, fixed sight | [W], [L] | tn_02 |
| Gators (storks) | patrol fixed rectangles (or lines); eat on contact even when hidden; block toads' sight | [W] | tn_04, tn_05 |
| Birds (falcon) | come when the predator that saw you is too far away | [W] | tn_02 |
| Hollow logs | always hidden inside | [W] | tn_06 |
| Hatchling | follows one step behind with its own colour: white when it joins, then its own tile's colour whenever Twig changes; both must stay hidden | [W] "both must always be simultaneously safe", "usually required that both share camouflage" | tn_07, tn_14 |
| Collectibles | two fruit and a hatchling per level; the best single escape counts; 3/3 puts a white star on the map | [W] | tn_11, tn_12 |
| Switches | rain wets dry grass, sun dries it | [W] | tn_08 |
| Eaten | undo (a tile or so back), start over, or the map | [W], [MM] | tn_09 |
| Branching map | beating a level opens the next two | [MM], [WP] | tn_15 |
| Goals | 30%; reach the Sun Gate; 100% | [W] | tn_11 |

### Readings we had to choose

- **Time:** beats of 8 frames; a step takes a beat, a change two; storks step
  every 2 or 3 beats.
- **Sight:** toads see 4 tiles ahead in a widening cone, storks 3.
- **The falcon** comes when the spotter is more than three tiles away.
- **Undo** goes back to just before the fatal move.
- **Completion:** a point per level beaten and per collectible in its best run.
- **The branches** are ours: each level opens the next two; 14 and 15 only
  from the one before.
- **SELECT** opens the level menu (restart, map): the original's way to
  reach them mid-level isn't described.

## What is ours

- **Name:** TINTAIL (1985, Beamdown Softworks).
- **Characters:** Twig the chameleon, the hatchlings, toads, storks and the
  falcon; Salt Island and the Sun Gate; all fifteen levels and their names;
  art and music.

## Additions: none

Only the platform needs every UFO 40 cartridge has: the START pause menu,
saving and the three goals, which are Camouflage's own.

## Controls

| Input | Action |
|---|---|
| D-pad | step one tile |
| A | change colour to the ground you stand on |
| hold B | show every predator's danger area (you can't move) |
| SELECT | level menu: restart, or back to the island |
| START | pause menu |

## Not confirmed

- The exact walk speed, change time and sight shapes.
- How the original branches its map.

## Sources

- [W] UFO 50 Wiki (Miraheze), "Camouflage": terrain, camouflage and its
  limits, predators, logs, birds, fruit and the baby, switches, undo,
  goals. https://ufo50.miraheze.org/wiki/Camouflage
- [MM] Steam guide "The missing manuals" (via the research notes): A
  changes colour and exposes you while it does, hold B for pink danger
  tiles and no moving, undo or start over, the map, the HUD.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [L] Lizstar's Trashcan, Part 16: one button to shift, one to see, two
  oranges and a baby, frogs still, gators always moving.
  https://lizstar64.github.io/reviews/2024/10/13/UFO50-16.html
- [WP] Wikipedia, "UFO 50": plan a route to the end, fruit and a baby.
  https://en.wikipedia.org/wiki/UFO_50
- [P] Popcar's Blog: undo to your last move, 15 levels, the baby.
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
