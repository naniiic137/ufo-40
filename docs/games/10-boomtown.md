# 10 · BOOMTOWN

*Internal design document. Not shown in the product.*

## Tribute to

**Devilition** (UFO 50 game #10, Mossmouth). The rules were researched from
text only: the community wiki, a player's guide linked from it, Steam guides
and threads, and written reviews (listed under Sources). No UFO 50 images,
video, sprites, music or code were used as references. The research notes
are in `ufo-40-notes/ufo40-research/10-devilition.md`.

**Map type: random, on one fixed square.** Devilition plays every night on
the same 10 × 8 board; what stands on it is dealt at random (the two first
townies, each night's hole and demons, the new townie after a clear, the
last night's extra holes), and so is the order of the pieces, from bags. So
is ours, with our own generator. There are no hand-made levels to draw.

### Full scale

| Structure | Devilition | BOOMTOWN |
|---|---|---|
| Board | 10 × 8 | 10 × 8 |
| Rounds | 10: nine of demons, then the boss | 10: nine nights of bogles, then the Bog King |
| Pieces | 8 kinds + the rocket pad | 8 kinds + the rocket perch |
| Demons | 3 kinds + the boss | 3 kinds + the Bog King |
| Round table | pieces 15 (start), 15, 10, 20, 15, 10, 20, 15, 15, 13; minor 8, 9 ×8; major 0, 0, 1, 2, 3, 3, 3, 3, 3; elder from round 6: 1, 2, 3, 4 | the same numbers |
| Boss | 10 hit points, the middle 2 × 2 | 10, the middle 2 × 2 |
| Endings | overrun (any round), boss survives, win with a score | the same three |
| Goals | survive 5 rounds; beat the game; 30,000 points | the same three |
| Secret | a message when a piece is blown at 5:55 on the clock | a message (our own words) at 5:55 |
| Modes | 1 player only, no saving mid-run | the same |

## Mechanics checklist

Every line was compared with the code; the tests named prove it with button
presses (cheats only set the square up).

| Mechanic | How BOOMTOWN does it | Source | Test |
|---|---|---|---|
| The square | 10 columns × 8 rows | [W] | bm_01 |
| Start | 2 folk on random tiles, 15 fireworks | [W], [SQ] | bm_01 |
| A night begins | a hole at random, then the night's bogles on random free tiles, then its fireworks | [SQ] | bm_08 |
| Holes | dead tiles: nothing is placed or spawns there; blasts pass over them | [W], [SQ] | bm_08 |
| The round table | pieces and demons per round exactly as the wiki's table | [W], [SQ] | bm_08 |
| Carry-over | unused fireworks stay in the cart; fireworks left on the square stay there | [W], [MM] | bm_14 |
| Three on offer | three fireworks at a time; a used one is replaced while more are left than are showing | [W], [SQ], [SC] | bm_02 |
| Bags | groups of 6: one tier-1, three tier-2, two tier-3, shuffled; each rolled within its tier, so repeats happen | [W], [SQ] | bm_07 |
| Choosing | ← → along the three and the LIGHT button; A takes one to the square | [MM] | bm_01, bm_02 |
| Placing | d-pad moves, B turns (only pieces that turn), A sets it down, hold B puts it back | [MM] | bm_02 |
| Where | only on a free tile: not a bogle, folk, hole or firework | [SQ] | bm_02 |
| Preview | the tiles it will hit show only while it is being placed | [SC], [SRCH] | shots |
| Starburst | the 8 tiles round it (bomb, tier 1) | [W], [SQ] | bm_03 |
| Roman candle | every tile in a line in front, to the edge; turns (cannon, tier 1) | [W], [SQ] | bm_03, bm_09 |
| Skyrocket | hits nothing itself: lit, it flies up and comes down on its perch (rocket, tier 1) | [W], [SQ] | bm_06 |
| Perch | placed straight after its rocket, costs a firework of its own, the 8 tiles round it; set off by its rocket or by any blast | [W], [SQ], [BUG] | bm_06 |
| Two rockets | a red and a green: never more than two on the square | [W] | bm_06 |
| Perch first | if the perch has gone off, the rocket flies off and hits nothing | [SQ] | bm_06 |
| Rocket last | a rocket that was the last firework owes its perch to the next night | [BUG] | bm_06 |
| Banger | the 4 tiles beside it (plus, tier 2) | [W], [SQ] | bm_03 |
| Pinwheel | the 4 corner tiles (cross, tier 2) | [W], [SQ] | bm_03 |
| Jumping jack | the tiles 2 away, 4 ways (toad, tier 2) | [W], [SQ] | bm_03 |
| Twin tube | in front and behind; turns (snake, tier 3) | [W], [SQ] | bm_03 |
| Fountain | the 3 tiles in front, straight and both corners; turns (strawman, tier 3) | [W], [SQ], [IHZ] | bm_03 |
| One match a night | LIGHT, then pick one firework on the square and press A; B goes back | [MM], [IHZ], [PC] | bm_03 |
| The chain | every firework a blast reaches goes off in turn, until nothing new is lit | [W], [SQ] | bm_03, bm_09 |
| Folk | never move; one hit blows them away, with no penalty but the head count | [W], [SQ] | bm_05 |
| Small bogle | 1 hit (minor) | [W] | bm_03 |
| Big bogle | 2 hits, from one chain or over several nights; turns red when hurt (major) | [W] | bm_04 |
| Old bogle | 2 hits; a wounded one heals at the night's end (elder) | [W], [SQ] | bm_04 |
| End of a night | more bogles than folk: overrun, game over at once; otherwise the town holds | [W], [SQ], [MM] | bm_05 |
| A clear | no bogles left: a new neighbour moves in on a random tile | [W], [SQ] | bm_05 |
| Out of fireworks | with nothing in the cart, LIGHT is the only choice; with nothing anywhere the night ends | [SQ] | bm_14 |
| The last night | leftover bogles become holes; the Bog King fills the middle 2 × 2; fireworks in the 12 tiles round him go; 2 to 4 more holes; 13 fireworks | [W], [SQ] | bm_08 |
| The Bog King | 10 hits; each blast tile on him is a hit, so a blast over two of his tiles is two | [W], [SRCH-D] | bm_09 |
| Win or lose | he must go down in that one chain: win, or the town is lost | [SQ], [SRCH-D] | bm_09, bm_10 |
| Score | on a win only: 10,000 + 1,000 × (folk + fireworks on the square + in the cart) + the clock bonus | [W], [SQ] | bm_09 |
| Clock bonus | 15,000 − 250 a minute, rounded to the nearest minute, never below 0 | [W], [SQ] | bm_12 |
| The clock | on screen, runs through the run, stops while paused | [W] (meta message) | bm_12 |
| Secret | light a fuse at 5:55 and a message shows | [W] | bm_13 |
| Goals | Beacon: hold out 5 nights; Saucer: beat the Bog King; Alien: win with 30,000 | [W], [SQ] | bm_11, bm_09 |
| Saving | a run is never saved half-way; the records are | [SV] | bm_11 |
| Records | most folk and most fireworks left, as Devilition's stats; best score | [W] | bm_09, bm_11 |
| Playable | a demo player wins a whole run with button presses | — | bm_15 |

### Readings we had to choose

- **One detonation ends the night.** The wiki says the player "may detonate
  one piece"; reviews say "you can only detonate one thing" and "ignite a
  single unit". The night ends when the chain is over.
- **Carry-over of placed pieces.** One guide says pieces on the board stay;
  the wiki only says leftover pieces carry. Both are true here.
- **Pieces per round.** The missing-manuals guide says ten new pieces each
  round; we follow the wiki's and the Squirrel guide's table.
- **The perch** replaces the rocket's place in the row, is placed at once
  and can't be put back. If there is no piece left to pay for it, it waits
  for the next night and comes first. If the perch took the last piece, the
  rightmost firework on offer goes.
- **Rocket colours** are taken while that rocket or its perch is on the
  square, on offer or owed; a third rocket is rolled again as a starburst or
  a roman candle.
- **Big bogles** take one hit per blast that reaches them; two fireworks in
  one chain can finish one.
- **Candle lines** don't stop at anything; blasts pass over holes.
- **Nothing is lit with an empty square**, and there is no skipping: with
  fireworks in the cart, a night ends only when a fuse is lit.
- **Folk in the Bog King's four tiles** are lost when he rises; folk round
  him stay, only fireworks go. His holes and the extra ones are random
  (2 to 4, as the guide says "usually").
- **Timing:** a blast lights the next firework 14 frames later; a rocket
  takes 34 frames to come down.
- **Turning pieces** start facing up; B turns them clockwise. The twin tube
  has two looks (up-down, left-right).
- **The clock** starts with night 1 (not on the title or the story) and runs
  through the animations; it stops while paused.
- **Records** are shown on the title: best score, most folk, most left.

## What is ours

- **Name:** BOOMTOWN (1984, Beamdown Softworks).
- **Story:** every night bogles crawl out of the marsh into Mossbury.
  Hazel the fireworks maker has a cart of fireworks and one match a night.
- **Characters:** Hazel, Mossbury's folk (the lamplighter, the baker, old
  Mr Pell, a child in a red cap), small, big and old bogles, the Bog King.
- **The fireworks:** starburst, roman candle, skyrocket and perch, banger,
  pinwheel, jumping jack, twin tube and fountain, their names and looks.
- **Setting:** a cobbled market square at night, sinkholes of bog water,
  the marsh with the King's eyes drawing nearer each night.
- **Text:** the story, the night banners, the results, the ending tally and
  the secret ("HAIL THE ORDER OF THE TEA COSY").
- **Music:** "Lantern Lane" (title), "Midnight Market" (the nights), "The
  Bog King", "Sky Full of Sparks" (ending), and the "Mossbury Holds" and
  "Overrun" jingles.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu, the records
save, and the three UFO 40 goals, which are Devilition's own (gift, gold,
cherry). The story card and title records stand in for the original's
story text and stats page. Devilition's terminal codes (BOMB-HELP,
FULL-RAND, EVIL-EYES, WEAK-TOWN) belong to the UFO 50 collection's
Terminal, which UFO 40 does not have, so they are not included.

## Controls

| Input | Action |
|---|---|
| ← → | choose a firework on offer, or LIGHT |
| A | take it to the square / set it down / light the chosen fuse |
| B | turn it (candle, twin tube, fountain) |
| hold B | put it back |
| LIGHT, d-pad, A | pick the one fuse to light; B goes back |
| START | pause menu |

## Not confirmed

- Whether the original allows ending a round without a detonation.
- Whether a cannon's line stops at anything.
- The boss round's handling of townies in and around the centre.
- The original's title screen, story text, music and ending: no text source
  describes them, so ours are our own.

## Tests

`tests/bm_01` … `bm_15`, all driven by button presses. `bm_15_demo_run`
is a demo player (`boomtown.c`, the `bot` query): for every firework on
offer, every free tile and every facing, it plays out the best chain on a
copy of the square and keeps the move that leaves the town safest, breaking
ties by how near its blasts come to the bogles still standing; it lights the
best fuse once the night is safe and clear (or when it is down to four
fireworks), and on the last night once the chain would finish the Bog King.
With seed 5 it holds all nine nights and wins, well over 30,000.

## Sources

- [W] UFO 50 Wiki (Miraheze), "Devilition": the board, holes, townies, the
  three demons and the boss, the nine pieces, tiers and bags, the round
  table, the round procedure, scoring, goals, stats, codes and the 5:55
  message. https://ufo50.miraheze.org/wiki/Devilition
- [SQ] "Devilition Guide" by Squirrel (linked from the wiki): placement
  rules, the rocket and its pad, detonation, holes each round, the round
  10 setup (12 tiles cleared, 2-4 holes, 13 pieces), scoring and rounding,
  goals. https://docs.google.com/document/d/e/2PACX-1vT6Z0ouRa-rxZBHPq7aEV61gUAbk-cqArJcXA1QLItHY7W-oZZoh3znTvwvlJx-hrWo4YiSHCx50EgA/pub
- [MM] Steam guide "The missing manuals - How to play UFO 50 games": the
  controls, the Detonate button, pieces on the board persist.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [BUG] Steam thread "you can't put rocket target if you roll rocket on your
  last turn": the target costs a piece; it turns up the next round.
  https://steamcommunity.com/app/1147860/discussions/1/4849904176634649721/
- [SV] Steam thread "Which games save progress?": Devilition doesn't.
  https://steamcommunity.com/app/1147860/discussions/0/4849904176633630339/
- [SRCH-D] search summary of the Steam "Devilition Cherry Guide": the
  dragon boss, 10 hp, 2 × 2, one round or you lose.
- [SRCH] search summary: trigger the rocket, not the target; attacks can't
  be seen once placed.
- [PC] Popcar's Blog, "Reviewing Every Single UFO 50 Game": ignite a single
  unit; three choices, no re-roll.
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [IHZ] Indie Hell Zone, "UFO 50: Devilition, Mooncat, Rail Heist, Quibble
  Race": you only detonate one thing; the "trident guy" hits three in front.
  https://indiehellzone.com/2024/10/30/ufo-50-devilition-mooncat-rail-heist-quibble-race/
- [SC] Static Canvas, "The UFO 50 Diaries: Devilition": three at a time,
  attack zones seen only while placing.
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-devilition
- [PF], [RNG] Steam threads on perfect rounds and on RNG: spare pieces
  matter more than townies, unwinnable deals happen.
  https://steamcommunity.com/app/1147860/discussions/0/4849904427681207938/
  https://steamcommunity.com/app/1147860/discussions/0/4700161870965932923/
- Lizstar's Trashcan (the alchemist and the settlers) for the feel.
  https://lizstar64.github.io/reviews/2024/10/10/UFO50-10.html
