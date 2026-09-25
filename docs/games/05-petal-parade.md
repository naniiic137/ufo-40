# 05 · PETAL PARADE

*Internal design document. Not shown in the product.*

## Tribute to

**Magic Garden** (UFO 50 game #5, Mossmouth). The rules were researched from
text only: the community wiki, Steam guides and threads, and written reviews
(listed under Sources). No UFO 50 images, video, sprites, music or text were
used as references.

**Map type: random.** Magic Garden is one fixed 12 × 12 field whose contents
appear at random, and so is ours. What appears, and where:

| Thing | Rule | Source |
|---|---|---|
| Loose pups | three at a time (ours), topped up on a random free tile at least 3 tiles from Posy; each gold jar adds one for good | [W], [G] |
| Sun circle | one at a time, off Posy's tile: a whole row or column, a square ring (side 4 to 6) or a cross (arms of 2 or 3) | [MM] |
| Brambles | one crawls out on its own every 9 s on a random free tile | [W] |
| Toadstools | planted by the witch on random free tiles | [W], [G] |
| Jars | on a random free tile when the counter reaches 6 | [W] |

## Mechanics checklist

Every line was compared with the code; the tests named prove it with button
presses.

| Mechanic | How PETAL PARADE does it | Source | Test |
|---|---|---|---|
| The field | 12 × 12 tiles inside a hedge | [W] | pp_02 |
| Never stops | Posy walks one tile every 8 frames and can't stop | [W], [MM], [G] | pp_01 |
| Turning | 90° only, never straight back, even with no line | [W], [G] | pp_01 |
| Turns lock to the grid | a turn pressed part-way through a tile waits for the next tile; a held direction steers too | [SC] | pp_01 |
| Her tile | the one more than half of her is on: things are met half-way into a tile | [SC] | pp_03 |
| The line | pups she walks over follow her path exactly, like Snake, and are an obstacle | [W], [MM] | pp_03 |
| No lives | the hedge, the line, a bramble or a toadstool ends the run at once | [W], [G] | pp_02, pp_03, pp_07, pp_12 |
| Hop (A) | instant, clears exactly one tile: the line, a bramble, a pup, a jar or a toadstool; tall toadstools have the tightest timing | [W], [SC], [MM] | pp_04, pp_14 |
| Hopping a bramble | leaves it dizzy for a while | [W], [SR] | pp_14 |
| Letting go (B) | Posy twirls and lets the whole line go | [W], [MM] | pp_05 |
| Saving | each pup standing on the sun circle is saved and scores 10 × its place in the line, wherever it is in the line | [W], [MM], [G] | pp_05, pp_06, pp_16 |
| Off the circle | every other pup turns into a bramble where it stands | [W], [MM] | pp_06, pp_07 |
| The sun circle | flashes, then moves somewhere else, about every 10 s | [MM], [L] | pp_20 |
| The witch | when a sun circle moves on and nobody was saved on it, she plants a toadstool; later 2 at a time, then 3 | [W], [G] | pp_15 |
| Brambles | look the way they will hop, then hop one tile, slowly; if the line, a loose pup or anything else is there, they hop another way | [W], [SR] | pp_21 |
| Jar counter | goes up by the number saved; at 6 it starts again and a nectar jar appears | [W] | pp_05, pp_08 |
| Jar level | each pup saved past the six needed makes it a level riper: red, green, blue, gold (9 or more: gold) | [W], [SR], [G] | pp_08 |
| Ripening | a jar left on the ground rises a level every 8 s, up to gold | [W], [G] | pp_09 |
| Nectar | drinking one starts a countdown from 48, faster than one a second; while it runs, walking into a bramble smashes it | [W] | pp_10 |
| Smash chain | 10, 20, 30… × the multiplier; the garden holds still a moment on each smash | [W], [SC] | pp_10 |
| More jars | any jar starts the countdown again; green and up add one to the multiplier | [W], [SR] | pp_11 |
| Running out | the chain and the multiplier reset | [W] | pp_11 |
| Toadstools | only blue or gold nectar smashes them (they count in the chain) | [W], [SR] | pp_12, pp_13 |
| Gold | also adds one loose pup to the field for good | [W], [SR] | pp_13 |
| Pace | never speeds up | [SE] | pp_22 |
| Palettes | the garden changes every 50 saved, four in all | [W] | shots |
| The end | as soon as 200 or more are saved | [W], [G] | pp_17 |
| Goals | Beacon: 10 or more at once; Saucer: 200; Alien: 200 with 20,000 points | [W] | pp_16, pp_17, pp_18 |
| Scores | a board of five; a run is never saved half-way | [W], [SV] | pp_02, pp_19 |

### Readings we had to choose

- **Numbers no source gives:** a tile every 8 frames; a count every 0.4 s
  (the wiki's "faster than one a second", not one guide's 45 s); dizzy for
  4 s; a bramble hop every ¾ s; a new bramble every 9 s; a 6-frame hitch.
- **The witch** plants 2 from 100 saved and 3 from 150.
- **A used sun circle** stays until its time is up.
- **Brambles** won't hop onto Posy's own tile, but nothing keeps one off the
  tile in front of her; their eyes show where they will go.

## What is ours

- **Name:** PETAL PARADE (1984, Beamdown Softworks).
- **Hero:** Posy, the palace gardener, in a straw hat.
- **The pups:** petalpups, fluffy flower puppies loose in the palace garden;
  angry ones turn into brambles with yellow eyes.
- **Sun circles:** tiles ringed with marigolds.
- **The witch:** Madame Nettle, the jealous hedge-witch next door, who grows
  toadstools out of spite.
- **Jars:** nectar jars (red, green, blue, gold); the counter is six little
  jars filling up.
- **Palettes:** spring, summer, autumn and a moonlit night garden.
- **Music:** the "Garden Waltz", the quicker "Nectar Rush" while the nectar
  runs, the "Petal Lullaby" title and "Two Hundred Home"; the ending's words.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu, the score board
save, and the three UFO 40 goals, which are Magic Garden's own (gift, gold,
cherry). Removed as invented: the suspended game and CONTINUE, the best score,
line count, season name, multiplier and "breaks toadstools" labels in the HUD,
the witch's patience bar, the control hints on the HUD and title, the rule
text in the pause menu, three single-tile circles that moved when used, and
brambles keeping off the tile in front of Posy.

## Controls

| Input | Action |
|---|---|
| D-pad | turn at the next tile (never straight back) |
| A | hop over the next tile |
| B | let the line go |
| START | pause |

## Not confirmed

- How many loose oppies wait at once, how often angry ones appear on their
  own, and how fast the gardener walks.
- Whether a used star area moves on at once.
- The potion's length (the wiki's count of 48 or a guide's 45 s).
- The witch's numbers (when 2 and 3 mushrooms start).

## Tests

`tests/pp_01` … `pp_23`, all driven by button presses. `pp_23_demo_run` is
a demo player (`petalparade.c`, the `bot` query) that wins a whole run: it
plans a way through the grid that never turns straight back, fetches pups,
walks them along the sun circle and lets go when they all stand on it,
drinks jars and smashes brambles while the nectar runs, keeps off the tile
each bramble is looking at, and hops what it can't avoid.

## Sources

- [W] UFO 50 Wiki (Miraheze), "Magic Garden": the grid, controls, the trail,
  dropping and saving, scoring by place, the flask counter and levels, the
  countdown from 48, the multiplier, mushrooms and the witch, angry oppies
  looking before they move, palettes, the end at 200, goals, the high score
  board. https://ufo50.miraheze.org/wiki/Magic_Garden
- [MM] Steam guide "The missing manuals - How to play UFO 50 games": B
  releases the line; oppies on star tiles are saved, the rest turn angry; one
  trail of star tiles at a time (a line across the level, a square or a
  cross) that flashes and reappears elsewhere; A jumps a tile.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [G] Steam guide "Magic Garden - Normal and Cherry Clear Tips": what kills
  you, walk the line along the star tiles and drop before the first slime
  leaves them, potion tiers by points (9+ gold), potions tier up every 8 s,
  gold adds a slime spawn, mushrooms only appear if you let the star area
  expire, you win at 200 or more.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3333877649
- [SR] Steam thread "Magic Garden rules?": a tier per oppie past 6, potions
  improve on the ground, green and up add to the multiplier, gold adds an
  oppie, jumping stuns, angry oppies look where they hop and go elsewhere if
  blocked. https://steamcommunity.com/app/1147860/discussions/0/4638240774905961302/
- [SC] Steam thread on Magic Garden's controls: turning is locked to the
  grid and waits for the next tile, the jump is instant, your tile is the
  one more than half of you is on, a small lag on each kill, steer by
  holding. https://steamcommunity.com/app/1147860/discussions/0/4849904427675941661/
- [SE] Steam thread "Magic Garden endless mode?": the same pace the whole way
  through. https://steamcommunity.com/app/1147860/discussions/0/4700161534027864570/
- [SV] Steam thread "Which games save progress?": Magic Garden doesn't save
  mid-run. https://steamcommunity.com/app/1147860/discussions/0/4849904176633630339/
- [L] Lizstar's Trashcan, "UFO 50 Retrospective Part 5": star squares move
  every 10 seconds or so. https://lizstar64.github.io/reviews/2024/10/07/UFO50-5.html
- Static Canvas, "The UFO 50 Diaries: Magic Garden" (you can't stop; pick a
  direction slightly early) and Popcar's Blog, "Reviewing Every Single UFO 50
  Game", for the feel.
