# 06 · TIN TROOP

*Internal design document. Not shown in the product.*

## Tribute to

**Mortol** (UFO 50 game #6, Mossmouth). The rules were researched from text
only: the community wiki, written reviews, a player guide and forum threads
(listed under Sources). No UFO 50 images, video, maps, sprites, music or text
were used as references.

**Map type: fixed.** Mortol's levels are hand-made, so ours are too
(`tintroop_levels.c`). We copy the structure where the text describes it,
never the layouts:

| Structure | Mortol (text sources) | TIN TROOP |
|---|---|---|
| Level count | 10 levels [SC] | 10 levels |
| Names | 1-A … 1-C, 2-A … 2-C, 3-A, 3-B, 4-A, 4-B (all named in [SG], [YT]; no 3-C or 4-C appears anywhere) | the same numbering |
| Order of ideas | each level introduces something new [PB]; 4-A combines everything from before [SR] | one new idea per level, 4-A is the mixed exam |
| World 4 | new lives do not come from the ship [W] | lives march out of the last toy-box door passed |
| Final level | a puzzle boss that only fires a slow spread shot; statue heads around it shoot things and spawn foes; a burning soldier thrown into its open mouth kills it at once [SR], [SG] | the Jack of the Chest between three carved nutcracker heads |
| Gift | beat 2-C [W] | Beacon: beat 2-C |

The level layouts, the world themes and their order of hazards are ours.

## Mechanics checklist

| Mechanic | How TIN TROOP does it | Source |
|---|---|---|
| Lives as a resource | the troop starts 1-A with 20 soldiers; every sacrifice or death spends one | [W], [MM] |
| Lives carry over | a level's finishing count is the next level's starting count | [W], [PB] |
| Replays | any cleared level can be replayed; only its best result is kept, and improving it raises the start of every later level | [W], [SG] |
| Level result | shown as the life change (e.g. +12) | [W] ("Best Life Change in a Single Level"), [YT] |
| Out of soldiers | the attempt fails and the level restarts with its starting count | reading |
| Extra lives | floating numbers give that many soldiers (+3, +5…) | [W], [MM], [SG] |
| Kill bonus | every third foe defeated gives one more soldier | [W], [MM] |
| The ship | new soldiers parachute from the ship, which moves along as the troop gets further | [W], [LZ] |
| Parachuting | slow and invincible; the D-pad steers the descent | [W], [MM] |
| Movement | run left and right; A jumps, higher the longer it's held | [MM] |
| Ritual of the Arrow | B: the soldier flies straight ahead, kills foes in his path and lodges in the first wall; his body is a ledge for later soldiers | [W], [MM] |
| Ritual of the Bomb | B + up: he explodes, destroying foes and breakable blocks in the blast | [W], [MM] |
| Ritual of Stone | B + down: he becomes a stone block where he is; dropped from the air it falls, crushes foes, and smashes down through any breakable blocks below it | [W], [MM] |
| Placement | stones and bodies sit wherever the soldier was, not on a grid (half off a ledge is fine), and stack | [SG2] |
| Combos | rituals chain: an arrow in flight can blow up or turn to stone; a falling stone can blow up | [W] |
| No undo | every ritual ends that soldier's life | [MM] |
| Bodies weigh | fallen soldiers press switches and scales like stones do, without blocking the way | [W] |
| Water | hold A to swim; a breath meter runs down underwater; a drowned soldier floats up and becomes a platform | [W] |
| Fire | a burning soldier can't be hurt by foes and burns the ordinary ones he touches; a meter fills and he burns away with no body; water puts him out | [W] |
| Plants | pods spit seeds; a seeded soldier glows green, and when the meter fills he dies and a climbable vine grows where he stood | [W] |
| Foes | patrol one platform, fly in a straight line, or come after you | [W] |
| Rammer | charges along its level; can be baited off ledges, jumped over, and hit from behind after a dash | [SG3] |
| Give up | hold B while parachuting to abandon the level and start it again | [SG4] ("Hold X to give up" at the start) |
| Saving | progress is saved between levels | [MM] |

### Foes

| Ours | Mortol | Behaviour |
|---|---|---|
| Wind-up mouse | Patroller | walks to and fro on its platform |
| Paper dart | Flyer | flies in a straight line, turning back at walls |
| Tin ram | Rammer | charges when a soldier is on its level; only hurt from behind |
| Bath fish | Fish | swims after soldiers in the water |
| Pill bug | Roller | rolls along floors and off ledges; seeds make it sprout a vine |
| Toy dragon | Dragon | stands still and breathes fire that sets soldiers alight |

### Readings we had to choose

The sources give the rules but not the numbers or every edge case:

- 10-px tiles, 16 rows per level. Run speed 1.1 px/frame; a full jump rises
  26 px (2.6 tiles) and carries about 34 px.
- The arrow flies 4 px/frame. A lodged soldier is a one-way ledge (you can
  jump up through him and land on top). The bomb blast reaches 2 tiles in
  each direction (a diamond). A stone is one tile and solid all round.
- Spikes only hurt at their points (the lower part of the tile).
- The rammer's front is armoured: a lance hitting its face stops there and
  the soldier falls.
- The camera follows the soldier; the ship follows the furthest point
  reached and never turns back, and it can't drop soldiers through a roof,
  so it waits at the edge of one. Destroyed blocks stay destroyed.
- Breath lasts 4 s, burning 3 s, the seed glow 2.5 s. A vine grows up to
  12 tiles from where its soldier (or pill bug) died.
- Scales swing 20 px: the heavier pan sinks, the other rises, and equal
  weights bring both back level.
- Creeper pots lob seeds to either side in turn, long and short.
- Holding B while parachuting (or while stepping out of a door in world 4)
  gives the level up.
- In world 4 the soldiers come out of the last toy-box door passed.
- The boss: three Pops also defeat him; his head is only open to a lance
  while he is sprung out of the chest.

## What is ours

- **Name:** TIN TROOP (1984, Beamdown Softworks).
- **Heroes:** the tin soldiers of Captain Pip's troop, parachuting from the
  toy blimp *Dauntless*.
- **Setting:** a toymaker's house at night. The Jack of the Chest has
  sprung out of the old toy chest and the troop must retake the house:
  the Nursery (1), the Bathroom (2), the Kitchen (3) and the Toy Chest (4).
- **Rituals:** the Lance (arrow), the Pop (bomb) and the Lead (stone): a tin
  soldier can turn himself to a lead weight.
- **Foes, hazards and pickups:** see the table above; candle flames, the
  kitchen stove, creeper pods in flowerpots, and brass number tags for lives.
- **Levels:** 10 original layouts.
- **Music:** original UFO-MML marches.

## Additions: none

Only the platform needs every UFO 40 cartridge has:

- the START pause menu;
- saving: the troop's record is saved after every level (as in Mortol);
- the three UFO 40 goals, which replicate Mortol's own three:

| UFO 40 goal | Condition | Mortol's goal |
|---|---|---|
| Beacon | clear 2-C | gift: beat level 2-C |
| Saucer | defeat the Jack of the Chest | gold: beat the game |
| Alien | clear 4-B with 75 soldiers or more | cherry: finish with at least 75 lives |

## Controls

| Input | Action |
|---|---|
| D-pad | run; steer the parachute; climb vines |
| A | jump (hold for height); swim |
| B | the Lance (arrow) |
| B + up | the Pop (bomb) |
| B + down | the Lead (stone) |
| hold B while parachuting | give up the level |
| START | pause |

## Sources

- [W] UFO 50 Wiki (Miraheze), "Mortol": 20 lives, carry-over and replays,
  extra-life blocks, the third-kill bonus, the three rituals and combos, the
  ship and parachute, world 4, water, fire, plants, bodies as weights,
  stones breaking blocks below, foe kinds, goals.
  https://ufo50.miraheze.org/wiki/Mortol
- [MM] Steam guide "The missing manuals - How to play UFO 50 games": the
  controls (A jump with height control, B, B + up, B + down), parachute
  steering, extra lives, saving between levels.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [SC] Static Canvas, "The UFO 50 Diaries: Mortol": 10 levels, 20 lives.
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-mortol
- [LZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 6 - Mortol": the ship
  moves along as you progress.
  https://lizstar64.github.io/reviews/2024/10/08/UFO50-6.html
- [PB] Popcar's blog, "Reviewing Every Single UFO 50 Game": lives carry
  between levels, replays, each level introduces something new.
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [SG] Steam thread "Mortol Ruby is 75 lives?!": the best count carries into
  later levels, +3s and +5s, every three kills, the final boss.
- [SG2] Steam thread "Mortol 2-C Help": stacking stones, blocks placed half
  off an edge.
- [SG3] Steam thread about rammers (search summary): baiting them off
  ledges, jumping over them, hitting them from behind after the dash.
- [SG4] Steam thread "Is there a quick way to reset the level for Mortol?":
  "Hold X to give up" at the start of a level.
- [SR] Search summaries: 4-A combines all earlier elements; the final boss's
  spread shot, statue heads, and the burning soldier in its mouth.
- [YT] Level names 1-A, 1-C, 2-C, 3-A, 3-B, 4-B as they appear in the titles
  of players' record videos (titles only; no video was watched).

## Progress

- Built: `src/games/tintroop/` (game, levels, art, audio), slot 06. The
  levels are painted by a small script (kept outside the repo) into
  `tintroop_levels.c`; `level_errors` checks every level's shape.
- Tests: `tests/tt_01` … `tt_21`: the rules (rituals, combos, parachute,
  kills, tags, rams, stone smashing, water, fire, seeds, the record and
  carry-over, saving, failing) and one route test per level that plays its
  obstacles the intended way, ending with the Jack of the Chest.
- Media: `docs/shots/tintroop.gif` (the boss), `tintroop_title.png`,
  `tintroop_shelf.png`, `tintroop_bath.png` (`tools/shots/06_tintroop.ufs`).
