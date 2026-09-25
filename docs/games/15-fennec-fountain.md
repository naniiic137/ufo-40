# 15 · FENNEC FOUNTAIN

*Internal design document. Not shown in the product.*

## Tribute to

**Block Koala** (UFO 50 game #15, Mossmouth). The rules were researched from
text only: the community wiki, written reviews and forum threads (listed under
Sources). No UFO 50 images, video, sprites, music, level thumbnails or text
were used as references, and no original room was reconstructed.

**Map type: fixed, hand-made.** Block Koala's fifty rooms are hand-made, so
ours are too, and every tile of them is our own design. We match the
structure the sources describe:

| Structure | Block Koala | FENNEC FOUNTAIN |
|---|---|---|
| Rooms | 50 puzzles | 50 rooms |
| Hub | a walled park you walk around, one door per puzzle | the dried-up oasis garden, one door per room |
| Reward | a star per room | a water drop per room |
| Gates | 5, 10, 20, 30 and 40 stars | 5, 10, 20, 30 and 40 drops |
| Finale | level 50, in the villain's bathing area behind the last gate | room 50, in the Vizier's bath behind the last gate |
| Editor | ten custom levels, "levels 51–60" | ten custom rooms, 51–60 |
| Order of ideas | numbered blocks first, other blocks and the toads "later" | sandstone and merging, then lapis, basalt and geckos (the exact order is ours) |

How the rooms are spread between the gates is ours (rooms 1–8 before the
first gate, 9–16, 17–28, 29–38, 39–49, then 50).

## Mechanics checklist

| Mechanic | How FENNEC FOUNTAIN does it | Source |
|---|---|---|
| Sokoban moves | the fennec walks one tile per press and pushes blocks | [W], [P], [L] |
| The goal of a room | push the water stone onto the dry spring | [W] "deliver a star block to the star space" |
| Weights | a block can push another only if its number is equal or higher; a chain pushes along if every link can | [L] "only push other blocks if they're of a greater or equal weight"; [P] |
| The fennec | pushes sandstone (red) blocks of any number | [TT] "red blocks can be freely moved by the player" |
| Merging with 1s | a block pushed onto a 1, or a 1 pushed onto a block, adds up: 3 + 1 = 4 | [P] "you can merge any number with 1's"; [L] |
| Grey at 5 | a block that reaches 5 turns to marble and never changes again | [TT] |
| Blue blocks | lapis (blue) blocks can only be pushed by other blocks of equal or greater number, never by the fennec | [TT], [L] |
| Black blocks | basalt (black) blocks are N × N tiles; each time you stop pushing one it shrinks by one, unless another block did the pushing or the push ended in a merge | [TT], [SR] "numbers decrease only when you stop pushing in a direction, and not every step. They don't decrease if you merge them"; [SE] the editor keeps black 2s out of the last row and column and black 3s out of the last two, so we read them as 2 × 2 and 3 × 3 |
| Black 1s | a basalt 1 can be added to sandstone or lapis blocks | [SE] |
| Mimics | geckos copy every step the fennec takes | [P] "mimicking toads"; [L] "a clone movement guy" |
| Undo | Button I (the X key, our B) takes back one step, as often as you like | [W] "If you press [Button 1] you can go back one step"; [SC] |
| Hub and gates | a hub world of 50 doors; drops open gates at 5/10/20/30/40 | [W] |
| Editor | ten custom rooms you can build, test and play | [W], [SE], [L] |

### Readings we had to choose

- **Which tiles a shrinking basalt keeps:** it keeps its top-left corner.
- **Mimics** move only when the fennec actually moves, in the same direction,
  pushing like the fennec; they don't count as "stopping" a basalt push.
- **Merges** only happen between single-tile blocks, and two basalt blocks
  never merge; the merged block keeps the colour of the non-basalt side.
- **The water stone** weighs 1: the fennec and any block can push it, it can
  push a 1, and it never merges. Marble (5) is pushed like sandstone.
- **Leaving a room:** walk back out through the door you came in; the room
  resets. The pause menu's RESTART starts the cartridge from the hub.
- **Room size:** up to 16 × 9 tiles.
- **Chains ending in a 1** always merge: a 2 pushed into a 1 becomes a 3 rather
  than pushing the 1 along.

## What is ours

- **Name:** FENNEC FOUNTAIN (1985, Beamdown Softworks).
- **Story:** the fennec siblings Fen and Zizi find their oasis garden dry: the
  camel Grand Vizier Humph has piped every spring into his palace bath. Each
  room hides a water stone that unplugs a spring.
- **Blocks:** sandstone (red), lapis (blue), basalt (black), marble (grey),
  the water stone; the mimicking **geckos**; drops instead of stars.
- **All 50 rooms**, every tile, and the hub garden layout.
- **Music:** an oasis theme for the hub, a thinking tune for the rooms, the
  palace theme and jingles.

## Additions: none

Only the platform needs every UFO 40 cartridge has:

- the START pause menu;
- saving: the drops, every room's state of completion and the ten custom
  rooms;
- the three UFO 40 goals, which replicate Block Koala's own three:

| UFO 40 goal | Condition | Block Koala's goal |
|---|---|---|
| Beacon | complete a custom room | gift: complete a custom level |
| Saucer | beat room 50 | gold: beat level 50 |
| Alien | beat room 50 holding all 50 drops | cherry: earn 50 stars (checked when level 50 is beaten) |

## Controls

| Input | Action |
|---|---|
| D-pad | walk / push |
| B | undo one step |
| A | enter a door, talk (hub) |
| START | pause menu |

Editor: D-pad moves the cursor, A places the chosen piece, B erases, SELECT
opens the piece menu (and PLAY TEST, SAVE, CLEAR).
Custom rooms may have no door, so SELECT walks out of them.

## Testing hooks

The rules are pure functions of a `FnState` (`fennec_logic.c`). Every room
ships with its shortest solution, and a headless test replays all fifty.

How the rooms were made: the wall plans are drawn by hand, one per room. A
small offline tool built on the same rule code scattered the chapter's
pieces over each plan thousands of times, solved every try with a
breadth-first search, dropped pieces that didn't matter, and kept the tries
closest to a difficulty target for that room; the picks were then checked by
eye and ordered easiest-first inside each chapter.

## Sources

- [W] UFO 50 Wiki (Miraheze), "Block Koala": the goal, lower numbers only
  pushed by higher numbers, fifty puzzles in a hub world, a star per level,
  gates at 5/10/20/30/40 stars, the undo line, goals.
  https://ufo50.miraheze.org/wiki/Block_Koala
- [P] Popcar's Blog, "Reviewing Every Single UFO 50 Game": higher numbers push
  lower numbers, merging with ones, 50 levels, self-degrading black blocks,
  mimicking toads. https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [L] Lizstar's Trashcan, "UFO 50 Retrospective Part 15 - Block Koala": blocks
  push blocks of greater or equal weight, pushing onto ones adds, a clone
  movement guy, blue blocks only pushed by other blocks, fifty levels, the
  level editor. https://lizstar64.github.io/reviews/2024/10/12/UFO50-15.html
- [TT] Search summary of the TV Tropes recap: red blocks move freely, blue only
  by blocks of equal or greater size, black blocks shrink when they stop being
  moved (not when pushed by another block or when merging), grey at five,
  yellow star blocks.
- [SR] Steam thread "Block Koala room 06" (rules only, no solution read):
  numbers drop when you stop pushing in a direction, not per step, and not
  when merging. https://steamcommunity.com/app/1147860/discussions/0/4852154959749613430/
- [SE] Steam threads on custom levels: ten custom levels (51–60), black 2 and
  black 3 blocks take 2 × 2 and 3 × 3 tiles, black 1s add to red or blue.
  https://steamcommunity.com/app/1147860/discussions/0/4700161534026757755/
- [SC] Static Canvas, "The UFO 50 Diaries: Block Koala": fifty levels, generous
  undo, you don't need every puzzle to reach the end.
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-block-koala
- [SK] Steam thread "Block Koala Cherry": the cherry is checked when level 50
  is completed.
