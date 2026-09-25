# 16 · TINTAIL

*Internal design document. Not shown in the product.*

## Tribute to

**Camouflage** (UFO 50 game #16, Mossmouth). The rules were researched from
text only: the community wiki, written reviews and guides (listed under
Sources). No UFO 50 images, video, sprites, music, level maps or text were
used as references. The wiki's level-layout images and island map were **not**
opened; no original level was reconstructed.

**Map type: fixed, hand-made.** Camouflage's fifteen levels are hand-made
single screens on an island map, so ours are too, and every tile is our own
design. We match the structure the sources describe:

| Structure | Camouflage | TINTAIL |
|---|---|---|
| Levels | 15 single-screen levels | 15 single-screen levels (20 × 10 tiles) |
| Map | an island map, played in order toward the far side | Salt Island, played west to east in order |
| Collectibles | two fruit and a baby on every level but the last | two prickly pears and a hatchling on levels 1–14 |
| Finale | level 15, the Sky Temple, no collectibles | level 15, the Sun Gate on the eastern cape, no collectibles |
| Order of ideas | frogs first; gators from "Crocodile Isle" (5); logs by level 3 ("River Lands"); switches late ("Sunny Banks", "Dry Gulch", "Rain or Shine") | toads first; storks from level 5; logs at level 3; the sun switch at 10, dry grass and the rain switch at 12, both switches at 13 |
| Difficulty | rising, with a hard last stretch ("Devil's Pass", "The Cliffs") | rising; the level names and exact placement are ours |

## Mechanics checklist

| Mechanic | How TINTAIL does it | Source |
|---|---|---|
| Top-down stealth puzzle | a chameleon crosses each single-screen level to the exit burrow | [W], [MM] "single-screen levels" |
| Terrain | grass (green), sand (yellow), swamp (dark blue), rock (brown) | [W] |
| Default colour | the chameleon starts white (never hidden) | [W] |
| Camouflage | a button turns the chameleon the colour of the tile it stands on; while that colour matches the tile under it, predators can't see it and it can pass through danger | [W], [L] |
| One colour at a time | stepping onto a different colour exposes it again, so you can never step between two danger tiles of different colours | [W] |
| Changing takes time | the chameleon is exposed while it changes colour, and it can't start changing while it (or the hatchling) is inside danger | [W], [TG] "cannot change its colour and move at the same time" |
| Predators and danger | predators watch areas ("danger"); an exposed chameleon in danger is eaten | [W] |
| Showing danger | a button shows every active danger area at any time | [W], [L], [P] |
| Frogs (our toads) | sit still; their danger areas are static | [W] |
| Gators (our storks) | walk fixed paths; their danger areas move with them | [W], [L] "move constantly" |
| Blocking sight | a walker between a watcher and the chameleon blocks the watcher's sight; walking close behind a walker is safe | [W] |
| Walked into | a walker that walks into the chameleon or the hatchling eats it, camouflage or not | [W] |
| Hollow logs | the chameleon can crawl through; inside, it is always hidden | [W] |
| Birds (our falcon) | if the predator that spotted you would take too long to reach you, a bird swoops in instead | [W] |
| Goal of a level | reach the exit hole | [W] |
| Fruit | two per level, collected on touch, gone from the map | [W], [SC] "two oranges" |
| Baby (our hatchling) | once picked up it follows exactly one step behind, shares the camouflage, and both must stay safe; it can't be dropped, only taken out or the level restarted | [W], [SC], [L] |
| Best run | only the best single run per level counts; 3/3 needs all three in one escape; 3/3 levels get a white star on the map | [W] |
| Rain / sun switches | rain turns dry grass wet, sun turns wet grass dry; some levels have both, so you can toggle | [W] |
| Undo | after being eaten you can undo, rewinding a few steps, or restart the level (losing its collectibles) | [W], [P], [MM] |
| Unwinnable states | you can paint yourself into a corner (e.g. taking the baby too early) and must restart | [SY], [W] |
| Stats | deaths are counted | [W] infobox "Deaths" |
| 15 levels, in order | the last is the temple on the far side of the island | [W], [P] |

### Readings we had to choose

The sources describe the rules but not the numbers or every edge case:

- **Time.** The world runs in real time on a grid, in beats of 8 frames
  (0.13 s). A step takes one beat; changing colour takes two beats, and the
  chameleon is exposed during both. Input is buffered to the next beat.
  Storks step every 2 beats (brisk) or 3 beats (slow). Everything is
  deterministic, which lets the tests solve every level.
- **Sight.** Every predator looks the way it faces, in a cone: a toad sees 4
  tiles ahead (widths 1, 3, 3, 5), a stork 3 tiles ahead (1, 3, 3). A tile is
  seen only if the line from the predator to it passes no palm, boulder, bush
  or other predator. Water and logs don't block sight.
- **Storks** face the way they will step next, so they turn the moment they
  reach the end of a leg. They never stop for anything; walking into one's
  tile is blocked, but a stork stepping onto you eats you.
- **The hatchling** takes the chameleon's colour, so they change together.
  It joins by hopping onto the tile the chameleon just left. It never picks up
  fruit or presses switches.
- **Dry grass** is its own straw colour. Wet grass is ordinary green grass.
  Switches are pads you step on; each one can be pressed again.
- **Logs** are entered from their ends only; you can't change colour inside.
- **The exit.** Stepping into the burrow escapes at once, whatever is watching.
- **Undo** rewinds to just before the move that got you eaten; if you were
  standing still, it rewinds one second.
- **The falcon** swoops in when the predator that saw you is more than three
  tiles away; otherwise that predator lunges.
- **Completion %.** One point per level beaten plus one per collectible in each
  level's best run: 15 + 42 = 57 points. The Beacon needs 30% (18 points).
- **Level menu.** SELECT pauses the level to restart it or leave for the map.

## What is ours

- **Name:** TINTAIL (1985, Beamdown Softworks).
- **Story:** every spring the chameleons of Salt Island climb to the Sun Gate
  on the eastern cape to greet the first sunrise. Kama, the smallest, is
  late, and picks up the island's lost hatchlings on the way.
- **Characters:** Kama the chameleon, the hatchlings, the watching **toads**,
  the wading **storks** and the **falcon**.
- **All 15 levels** (every tile), their names and the island map.
- **Pixel art, music and sounds**: a prickly-pear island, a tiptoe level tune,
  the Sun Gate theme and jingles.

## Additions: none

Only the platform needs every UFO 40 cartridge has:

- the START pause menu;
- saving: levels beaten, each level's best collectibles and the deaths;
- the three UFO 40 goals, which replicate Camouflage's own three:

| UFO 40 goal | Condition | Camouflage's goal |
|---|---|---|
| Beacon | reach 30% completion | gift: earn 30% completion |
| Saucer | reach the Sun Gate on the far side of the island | gold: reach the sky temple on the far side of the island |
| Alien | reach 100% completion | cherry: earn 100% completion |

## Controls

| Input | Action |
|---|---|
| D-pad | step one tile |
| A | change colour to the ground you stand on |
| hold B | show every predator's danger area |
| SELECT | level menu: restart the level, or back to the island |
| START | pause menu |

UFO 50's Button II (the Z key) and Button I (the X key) are our A and B.
Lizstar calls them "B to shift" and "A to see", which matches their naming of
Bushido Ball's charge button (Button I) as "A".

## Testing hooks

The rules are a pure function of a `TtState` and the beat number
(`tintail_logic.c`). A breadth-first solver over (tile, colour, change timer,
hatchling, fruit, switch state, beat phase) proves every level can be escaped
with all three collectibles, and plays its route back for the GIF.

## Sources

- [W] UFO 50 Wiki (Miraheze), "Camouflage": terrain colours, camouflage and
  its limits, frogs and gators, blocking sight, logs, birds, fruit and the
  baby, best-run collectibles, white stars, rain and sun switches, undo and
  restart, the fifteen level names, goals.
  https://ufo50.miraheze.org/wiki/Camouflage
- [L] Lizstar's Trashcan, "UFO 50 Retrospective Part 16 - Camouflage": one
  button changes colour, another shows where enemies can see; two oranges and
  a baby per level; the baby makes you twice as long; frogs stand still,
  alligators move constantly. https://lizstar64.github.io/reviews/2024/10/13/UFO50-16.html
- [P] Popcar's Blog, "Reviewing Every Single UFO 50 Game": a button shows enemy
  sight; undo to your last move; the baby follows you; 15 levels.
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [SC] Static Canvas, "The UFO 50 Diaries: Camouflage": reach a tile unseen,
  two oranges and a baby, the baby moves one step behind.
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-camouflage
- [MM] Search summary of the Steam guide "The missing manuals": single-screen
  levels; undo rewinds to a little before the fatal move.
- [TG] Search summary of Wikipedia and TheGamer: frogs, gators and birds; the
  lizard can't change colour and move at the same time.
- [SY] syltefar.com, "Camouflage (UFO 50)": you can get into an unwinnable
  situation and have to restart. https://syltefar.com/game?id=4276
