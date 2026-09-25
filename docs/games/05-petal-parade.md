# 05 · PETAL PARADE

*Internal design document. Not shown in the product.*

## Tribute to

**Magic Garden** (UFO 50 game #5, Mossmouth). The rules were researched from
text only: the community wiki, written reviews and forum threads (listed
under Sources). No UFO 50 images, video, sprites, music or text were used as
references.

**Map type: random.** Magic Garden is one fixed field whose contents spawn at
random, and so is ours. Generator rules:

| Rule | Value | Source |
|---|---|---|
| Field | 12 × 12 tiles, walled | [W] |
| Friendly oppies on the field | 3 at a time (ours); each gold flask adds one for good | [W], [SG] |
| Where they appear | a random empty tile, away from the gardener's path | ours |
| Star pads | 3 on the field at random empty tiles; one that's been used moves somewhere else (ours) | [MM] "star tiles appear on the ground" |
| Angry oppies | one crawls out every ~9 seconds, from a random empty tile (rate ours) | [W] "spawn slowly, independent of player action" |
| Mushrooms | the witch plants one if no oppie has been saved for 15 seconds (ours); 2 at a time from 100 saved, 3 from 150 | [W] |
| Flasks | spawn on a random empty tile when the flask counter reaches 6 | [W] |

## Mechanics checklist

| Mechanic | How PETAL PARADE does it | Source |
|---|---|---|
| Continuous movement | Lina never stops; the D-pad turns her; she can't turn straight back | [W], [SC] |
| A trail | petalpups she walks over follow in a line behind her (like Snake) | [W] |
| Obstacles | the trail, angry pups, mushrooms and the walls all end the game on contact | [W], [SC] |
| No lives | one mistake is game over | [W], [SC] |
| Dropping the trail | B lets the line go | [MM] |
| Saving | dropped while Lina stands on a star pad, the whole line is saved | [W], [MM] |
| Dropping elsewhere | the pups turn angry where they stand and become obstacles | [W] |
| Save score | each saved pup scores 10 × its place in the line (N pups: 10 × N(N+1)/2) | [W] |
| Jump | A hops over the next tile: the trail, an angry pup, a loose pup, a flask or a mushroom; each has its own timing window | [W] |
| Stunning | hopping over an angry pup stuns it for a while | [W], [SG] |
| Angry pups move | slowly, one tile at a time; they won't step onto the trail or a loose pup | [W], [SG] |
| Flask counter | every saved pup adds one; at 6 it resets and a flask appears | [W] |
| Overcooking | each pup saved beyond the 6 needed raises that flask one level: red, green, blue, gold | [W], [SG] |
| Flasks ripen | a flask left on the ground slowly rises a level | [SG] |
| Powered state | walking onto a flask starts a countdown from 48; while it runs, Lina smashes angry pups she runs into | [W] |
| More flasks | another flask while powered resets the countdown | [W] |
| Multiplier | green and higher flasks raise the multiplier by one | [W], [SG] |
| Kill score | kills in one powered run score 10, 20, 30… × the multiplier | [W] |
| Reset | when the countdown runs out, the kill chain and the multiplier reset | [W] |
| Mushrooms | only blue and gold flasks let Lina smash them | [W], [SG] |
| Gold flasks | also add one friendly pup to the field for good | [W], [SG] |
| The witch | plants mushrooms when too long passes without a save; later 2 or 3 at once | [W] |
| Palettes | the garden changes colour every 50 pups saved, four palettes in all | [W] |
| The goal | save 200 pups; then Lina confronts the witch (the same ending either way) | [W] |

### Readings we had to choose

The sources give the rules but not every number:

- **Speed:** Lina moves one tile every 8 frames (7.5 tiles a second), the
  same all game.
- **Countdown:** 48 steps of 0.6 s.
- **Stun:** 4 seconds. **Flask ripening:** one level every 12 seconds.
- **Jump:** a hop lasts one step (8 frames). Low things (the line, pups,
  brambles, jars) are checked when Lina is half-way into a tile, so any hop
  clears exactly one of them. Toadstools are tall and are checked the moment
  she steps in as well, so the hop has to start in the 4 frames before she
  reaches one: the kindest window is for low things, the tightest for
  mushrooms.
- **Brambles** (angry pups) creep one tile every ¾ s; a stunned one stays put
  but is still in the way.
- **The line** passes over anything Lina hopped: it follows her path exactly.

## What is ours

- **Name:** PETAL PARADE (1984, Beamdown Softworks).
- **Hero:** Lina, the palace gardener, in a straw hat.
- **The pups:** petalpups, fluffy flower puppies that got loose in the palace
  garden. Angry ones turn into brambles with little red eyes.
- **Star pads:** sun circles, rings of marigolds.
- **The witch:** Madame Nettle, the jealous hedge-witch next door, who grows
  toadstools out of spite.
- **Flasks:** nectar jars (red, green, blue, gold).
- **Palettes:** spring, summer, autumn and a moonlit night garden.
- **Music:** the "Garden Waltz", a quicker "Nectar Rush" for the powered
  state, and jingles.

## Additions: none

Only the platform needs every UFO 40 cartridge has:

- the START pause menu;
- saving: a game in progress is suspended when you quit to the library and
  resumed next time (the field, the line and the score); the best score is
  kept;
- the three UFO 40 goals, which replicate Magic Garden's own three:

| UFO 40 goal | Condition | Magic Garden's goal |
|---|---|---|
| Beacon | save 10 or more pups at once | gift: save at least 10 oppies at once |
| Saucer | save 200 pups | gold: save 200 oppies |
| Alien | save 200 with 20,000 points or more | cherry: 200 saved and 20,000 points |

## Controls

| Input | Action |
|---|---|
| D-pad | turn Lina (never straight back) |
| A | hop over the next tile |
| B | let the line go (save it on a sun circle) |
| START | pause |

## Sources

- [W] UFO 50 Wiki (Miraheze), "Magic Garden": the 12×12 grid, controls, the
  trail, dropping and saving, scoring, flask levels and the counter, the
  powered state and multiplier, mushrooms and the witch, palettes, goals.
  https://ufo50.miraheze.org/wiki/Magic_Garden
- [SC] Static Canvas, "The UFO 50 Diaries: Magic Garden": continuous movement,
  one hit ends the run, a potion for every six saved.
- [SG] Steam thread "Magic Garden rules?": a flask tier per extra pup (9+ =
  gold), flasks ripen on the ground, green+ adds to the multiplier, gold adds
  a pup, jumping stuns, angry pups avoid the line.
  https://steamcommunity.com/app/1147860/discussions/0/4638240774905961302/
- [MM] Steam guide "The missing manuals - How to play UFO 50 games": A jumps
  a tile, B releases the line, star tiles on the ground.

## Progress

- Built: `src/games/petalparade/` (game, art, audio), slot 05.
- Tests: `tests/pp_01` … `pp_19` cover turning, the hedge, the line and
  running into it, hopping, saving and its score, dropping off a circle, jar
  levels and ripening, nectar and the multiplier, the countdown running out,
  toadstools (green can't, blue/gold can), stunning, the witch, the random
  field, the three goals and suspend/continue.
- Media: `docs/shots/petalparade.gif`, `petalparade_title.png`,
  `petalparade_power.png`, `petalparade_night.png`
  (`tools/shots/05_petalparade.ufs`).
