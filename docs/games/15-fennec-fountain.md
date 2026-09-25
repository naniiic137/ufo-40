# 15 · FENNEC FOUNTAIN

*Internal design document. Not shown in the product.*

## Tribute to

**Block Koala** (UFO 50 game #15, Mossmouth). The rules were researched from
text only: the community wiki, the Steam guide "The missing manuals",
written reviews and forum threads (listed under Sources). No UFO 50 images,
video, sprites, music, level thumbnails or text were used, and no original
room was reconstructed.

**Map type: fixed, hand-made.** Block Koala's fifty rooms are hand-made, so
ours are too, every tile our own design. We match the structure the sources
describe:

| Structure | Block Koala | FENNEC FOUNTAIN |
|---|---|---|
| Rooms | 50 puzzles, a star each | 50 rooms, a drop each |
| Hub | a walled park with a door per puzzle | the dry gardens, a hut per room |
| Gates | 5, 10, 20, 30 and 40 stars | 5, 10, 20, 30 and 40 drops |
| Finale | level 50 behind the last gate | room 50, Humph's bath, behind the last gate |
| Editor | custom levels 51–60 | custom rooms 51–60 |
| Order of ideas | black blocks by room 6 ([SR]); blue blocks, toads, arrows and doors later | basalt at room 6; lapis from 17, geckos from 29, arrows, stone patches and doors from 39 |

## Mechanics checklist

Every line was compared with the code; the tests named prove it with button
presses.

| Mechanic | How FENNEC FOUNTAIN does it | Source | Test |
|---|---|---|---|
| Goal of a room | push the water stone onto the dry spring | [W], [MM] | fn_09, fn_16 |
| Weights | a line of blocks moves only if no block is pushed into a heavier one; the water stone weighs 0 | [MM], [L], [P] | fn_02, fn_03 |
| Merging | a block pushed into a 1 that can't move takes it in and weighs one more (2 + 1 = 3; two 1s make a 2); a 1 that can move is pushed | [MM], [P] "a 3 can merge with a 1 and turn into a 4" | fn_03 |
| The fennec | pushes sandstone (red) of any number | [TT] | fn_02 |
| Lapis (blue) | only another block, of equal or more weight, can push it | [TT], [L] | fn_04 |
| Basalt (black) | shrinks by one when a push by the fennec ends, not when pushed by a block or merged; a 1 crumbles | [SR], [TT], [P] | fn_05 |
| Black 1s | added to red or blue blocks | [TT] | fn_05 |
| Five | a block reaching 5 turns to grey marble and never changes | [TT] | fn_03 |
| Geckos (toads) | copy every step the fennec takes | [P], [L] | fn_06 |
| Arrows | a block goes onto or off an arrow only its way; the second block in a line isn't held to them | [ST] "directional arrows not stopping you if you push 2 blocks" | fn_14 |
| Stone patches | the fennec walks on them, no block can | [ST] | fn_14 |
| Doors | see Readings | [G] | fn_15 |
| Undo | B takes back a step, without limit | [W], [MM] | fn_07, fn_13 |
| Room menu | A: start over, leave, or set / drop one undo mark | [MM] | fn_13 |
| No death, no timer | none | [MM] | |
| Pace | slow, a tile at a time | [ST] "how slow the Koala goes" | fn_12 |
| Hub and gates | 50 huts, gates at 5/10/20/30/40 | [W] | fn_08 |
| Editor | ten custom rooms, 51–60 | [MM], [SE] | fn_11 |
| Goals | complete a custom room; beat room 50; beat room 50 holding all 50 drops | [W], [SK] | fn_10, fn_11 |

### Readings we had to choose

- **Basalt size:** a black N is N × N ([SE]'s editor limits; unconfirmed).
  It shrinks keeping its top-left corner.
- **Doors:** no source says how they work. We read them as gates held open
  while every plate carries a block or a creature, and a door with
  something in it stays open until it is clear (the "priority" cheese in
  [G]).
- **Arrows** bind blocks only, not the fennec or geckos.
- **The mark:** B jumps straight back to it, once.
- **Merges:** a big basalt can't grow, marble and the water stone never
  merge, two basalt never meet.
- **Pace:** 12 frames a tile.

## What is ours

- **Name:** FENNEC FOUNTAIN (1985, Beamdown Softworks).
- **Story:** Fen and his sister Tuft find their gardens dry: Lord Humph the
  camel has piped every spring into his bath.
- **Characters:** Fen, Tuft, Old Moss, the tile-setter, the mason, Cousin
  Bramble, Lord Humph, the geckos.
- **All 50 rooms**, the hub garden, pixel art and music.

## Additions: none

Only the platform needs every UFO 40 cartridge has: the START pause menu,
saving (drops, custom rooms) and the three goals, which are Block Koala's
own.

## Controls

| Input | Action |
|---|---|
| D-pad | walk / push (hold to keep walking) |
| B | undo a step (or back to the mark) |
| A | room menu; in the garden, enter a hut or talk |
| START | pause menu |

Editor: A places the chosen piece (hold to paint), B opens the pieces and
PLAY TEST, SAVE AND EXIT, CLEAR ROOM.

## Not reproduced / unconfirmed

- Level codes for sharing custom rooms (no text entry on the console).
- The sister's "need a break?" offer ([W] dialogue): what it does is unknown.
- Whether toads push blocks, and door rules (see Readings).

## Sources

- [W] UFO 50 Wiki (Miraheze), "Block Koala": goal, weights, 50 puzzles,
  stars, gates, the undo line, goals. https://ufo50.miraheze.org/wiki/Block_Koala
- [MM] Steam guide "The missing manuals": weights (dots 1, star 0), lines of
  blocks, merging into an immovable dot, B rewinds, A's menu (reset, exit,
  one-off undo point), the editor.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [P] Popcar's Blog: merging 3 + 1, black blocks, mimicking toads.
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [L] Lizstar's Trashcan, Part 15: weights, blue blocks, the clone.
  https://lizstar64.github.io/reviews/2024/10/12/UFO50-15.html
- [TT] Search summary of the TV Tropes recap: red, blue, black, grey blocks.
- [SR] Steam thread "Block Koala room 06": black numbers drop when a push
  ends, not when merged; room 6 has them.
  https://steamcommunity.com/app/1147860/discussions/0/4852154959749613430/
- [ST] Steam thread on Block Koala: arrows, stone patches blocks can't cross,
  the slow koala. https://steamcommunity.com/app/1147860/discussions/0/4852155320352854747/
- [SE] Steam thread on the editor: black 2/3 placement limits.
  https://steamcommunity.com/app/1147860/discussions/0/4700161534026757755/
- [G] Steam guide "Block Koala full guide" (header only): doors in later
  levels, a door priority cheese. https://steamcommunity.com/sharedfiles/filedetails/?id=3337312682
- [SK] Steam thread "Block Koala Cherry": checked when level 50 is beaten.
