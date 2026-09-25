# 06 · TIN TROOP

*Internal design document. Not shown in the product.*

## Tribute to

**Mortol** (UFO 50 game #6, Mossmouth). The rules were researched from text
only: the community wiki, the TASVideos game-resource page and a TAS
submission text, Steam guides and threads, and written reviews (listed under
Sources). No UFO 50 images, video, maps, sprites, music or text were used as
references.

**Map type: fixed.** Mortol's levels are hand-made, so ours are too
(`tintroop_levels.c`). We copy the structure where the text describes it,
never the layouts:

| Structure | Mortol (text sources) | TIN TROOP |
|---|---|---|
| Levels | 10: 1-A, 1-B, 1-C, 2-A, 2-B, 2-C, 3-A, 3-B, 4-A, 4-B [TAS], [SC] | the same ten |
| Lives | 20 at 1-A; the count carries on; replays keep the best [W] | the same |
| Worlds 1-3 | the view scrolls forward only; the plane sits at the top left and drops the next soldier [MM], [TR] | the same |
| World 4 | no plane; doorways are checkpoints and the troop may walk back [TT] | toy-box doors |
| Final level | a boss that fires a slow spread; four nests around it send things out on a timer; a burning lance into its open mouth kills it at once [TR], [SG] | the Jack of the Chest and four carved heads |

### What each level brings in

| Level | Mortol (text sources) | TIN TROOP |
|---|---|---|
| 1-A | the three rituals | THE PLAYROOM FLOOR: a lance ladder up a bookshelf, a Pop through toy blocks, a spike rug, a paper dart |
| 1-B | the rammer [TAS] | BUILDING BLOCKS: tin rams, a tower of toy blocks for a falling stone |
| 1-C | switches and gates, bodies on spikes [W] | THE TOY TRAIN: two switches (one down a shaft), a spike bed to bridge |
| 2-A | water, fish, pipes, launchers [W], [TAS] | BATH TIME: a tap to block, a plughole, a launcher in the tub wall |
| 2-B | burning creatures, shallow water over spikes [TAS] | CANDLELIGHT: a nest of mice to burn, a toy dragon |
| 2-C | a stone puts fire out, a lance catches fire, the bomb roller, stones stacked half off edges [TAS], [SG2] | THE SOAP DISH: candles under a hole, pill bugs, a high ledge |
| 3-A | plants and seeds [TAS] | THE HERB POTS: seed pots, and a flour bin only a vine can climb |
| 3-B | laser blocks, the rammer [TAS] | THE PANTRY: a laser eye down a tunnel, a guarded shelf |
| 4-A | everything, rolling and flying foes [TAS] | THE TOY CHEST: doors, a wick, a basin, lasers, rams |
| 4-B | the boss [TAS] | THE JACK OF THE CHEST |

## Mechanics checklist

| Mechanic | How TIN TROOP does it | Source |
|---|---|---|
| Lives as a resource | 20 soldiers at 1-A; every ritual or death spends one | [W], [MM] |
| Carry and replays | a level's final count starts the next; a replay only keeps a better result, which raises every later start | [W], [PB], [SG] |
| Extra lives | floating number tags give that many soldiers; every third foe beaten gives one; each beaten foe shows the running count | [W], [MM], [SG] |
| Movement | run with a short build-up; A jumps, higher the longer it's held, steerable in the air; a tap cuts the rise short; holding A while falling slows the fall; two frames of grace after stepping off a ledge | [MM], [TR] |
| The plane | waits at the top left of the view; with a roof under it, it creeps on until the drop is clear | [MM], [TR] |
| Parachute | slow, steered with the D-pad, can't be hurt; landing or A ends it; after A he can still jump for a moment | [MM], [TR], [W] |
| Scrolling | worlds 1-3 only scroll forward; the left edge of the view is a wall; broken blocks stay broken | [TR], [SG] |
| Give up | hold B on the parachute to give the level up | [SG4] |
| The Lance (B) | he flies straight ahead, kills foes on the way, collects tags, lodges in the first wall and stays there as a ledge | [W], [MM] |
| The Pop (up+B) | he blows up: foes and toy blocks round him go; the blast lights wicks | [W], [MM] |
| The Lead (down+B) | on the ground he stands still a moment, then turns to stone; in the air he is a stone at once, falling at full speed | [W], [TR] |
| Falling stone | smashes down through every toy block below, stopping and speeding up again after each; crushes spikes and foes; puts flames out; sinks slowly in water; blocks pipes | [W], [TR], [TAS] |
| Chains | while he flies or falls, B again changes ritual: lance, stone and Pop in any order, still one soldier | [W], [TR] |
| Bodies | corpses hold switches down like stones but don't block the way; foes walk through them; a soldier killed on spikes stays on the points as a bridge | [W], [TT], [TAS] |
| Water | hold A to swim; a breath meter runs down; a drowned soldier floats up as a platform; a ritual under water only drowns him | [W] |
| Pipes | a tap fills its pool while its mouth is clear; a plughole drains it; a stone on the tap stops it; drained water kills the fish, and launchers then send darts instead | [W], [TAS] |
| Fire | a flame or a fireball sets him alight: foes can't hurt him, he sets them alight, and a burning creature passes the fire on (not always); the meter fills and he burns away with no body; water puts him out; a lance through a flame catches fire | [W], [TAS] |
| Seeds | seeds make him glow green; when the meter fills he dies and a vine climbs as high as it can; killed by a foe first, the vine still grows; a lance loses the seed | [W], [TAS] |
| Lasers | fire the moment anything steps into their line; a soldier freezes and drops dead, foes die; walls, stones and bodies stop the beam | [TAS], [TT] |
| Patroller (mouse) | walks to and fro on its platform | [W] |
| Flyer (paper dart) | flies in straight lines; turns at random while it can't see a soldier | [W] |
| Rammer (tin ram) | charges a soldier on its ground and turns round after passing him; can be jumped over; a stone clips it | [W], [TAS] |
| Fish | swim to and fro; each picks a new direction at random every 40 frames | [TR] |
| Roller (pill bug) | a rolling bomb; it waits while off screen; seeded, it grows a vine | [SG2], [TAS], [SG] |
| Dragon | turns to face a soldier and breathes a slow fireball every 2.5 s (the wiki names it; what it does is our reading) | [W] |
| Boss | a slow spread shot; four nests send out fireballs, lasers, darts, rollers and seeds from a shuffled list of ten, one every 720 frames; lances hurt him through his open mouth, a burning one kills him at once; he flashes when hit | [TR], [SG], [TT] |
| Timing | kills and pickups hold the action for one frame | [TR] |
| Saving | the record is saved between levels | [MM] |

### Readings we had to choose

Numbers not in any source are ours, and so are these readings:

- 10-px tiles where Mortol has 16-px blocks, so its 1.6 px/frame walk is
  1.0 here. A full jump rises about 2.5 tiles.
- Breath lasts 4 s, burning 3 s, the seed glow 2.5 s; a pill bug's fuse 5 s.
- The Pop's blast is a diamond two tiles each way and lingers 16 frames.
  Standing, the Lead takes 20 frames.
- A laser beam lasts 24 frames, then rests 60. A launcher sends one creature
  every 4 s, two at most.
- The tin ram's brass front turns a plain lance. The boss takes three plain
  lances. A dart turns every 90 frames while it can't see a soldier; fire
  spreads 7 times in 8.
- Mortol also has scales [W]; the engine supports them, but none of our ten
  layouts uses them.

## What is ours

- **Name:** TIN TROOP (1984, Beamdown Softworks).
- **Heroes:** the tin soldiers of Captain Pip's troop, dropped from the toy
  blimp *Dauntless*.
- **Setting:** a toymaker's house at night: the nursery (1), the bathroom
  (2), the kitchen (3) and the toy chest (4), where the Jack of the Chest has
  sprung out.
- **Rituals:** the Lance, the Pop and the Lead.
- **Foes:** wind-up mice, paper darts, tin rams, bath fish, pill bugs, toy
  dragons, the Jack of the Chest and his carved heads.
- **Levels:** 10 original layouts.
- **Music:** original UFO-MML marches.

## Additions: none

Only the platform needs every UFO 40 cartridge has:

- the START pause menu, and a how-to-play book on SELECT at the title;
- saving: the troop's record is saved after every level (as in Mortol);
- the three UFO 40 goals, which replicate Mortol's own three:

| UFO 40 goal | Condition | Mortol's goal |
|---|---|---|
| Beacon | clear 2-C | gift: beat level 2-C |
| Saucer | defeat the Jack of the Chest | gold: beat the game |
| Alien | finish 4-B with 75 soldiers or more | cherry: finish with at least 75 lives |

## Controls

| Input | Action |
|---|---|
| D-pad | run; steer the parachute; climb vines |
| A | jump (hold for height, hold while falling to fall slower); swim |
| A on the parachute | cut loose |
| B | the Lance |
| up + B | the Pop |
| down + B | the Lead |
| B while flying or falling | change ritual |
| hold B on the parachute | give the level up |
| START | pause |
| SELECT (title) | how to play |

## Sources

- [W] UFO 50 Wiki (Miraheze), "Mortol": lives and carry-over, extra lives,
  the rituals and chains, bodies as weights, water, pipes and launchers,
  fire, plants, foe kinds, goals.
  https://ufo50.miraheze.org/wiki/Mortol
- [TR] TASVideos, "UFO 50: Mortol" game resources: walk speed, coyote
  frames, tapped and held jumps, the parachute's last frame, the plane
  creeping past a roof, the falling stone's speed through blocks, respawn
  delays, lag frames on kills, fish timers, the boss's nests and the fire
  lance. https://tasvideos.org/GameResources/PC/UFO50/Mortol
- [TAS] TASVideos submission 10466S (its text): the ten levels and what each
  brings in, a stone blocking the 2-A pipe, a stone putting fire out, a lance
  catching fire, lasers, rollers pausing off screen, foes passing through
  bodies, seeds kept when killed. https://tasvideos.org/10466S
- [MM] Steam guide "The missing manuals - How to play UFO 50 games":
  controls, the parachute, the plane at the top left, lives top right,
  numbers over beaten foes, floating number tags, saving.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [TT] TV Tropes (search snippets only): bodies over spikes, a stone
  destroying spikes, world 4's doorways, lasers, the boss flashing.
- [SC] Static Canvas, "The UFO 50 Diaries: Mortol": 10 levels, 20 lives.
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-mortol
- [LZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 6 - Mortol".
  https://lizstar64.github.io/reviews/2024/10/08/UFO50-6.html
- [PB] Popcar's blog, "Reviewing Every Single UFO 50 Game": carry-over and
  replays. https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [RC] Record Crash, "Game Review: UFO 50", and [BQ] BugQuest's UFO 50
  review: the general feel only.
- [SG] Steam thread "Mortol Ruby is 75 lives?!": +3s and +5s, every third
  kill, a vine from a roller, the boss.
- [SG2] Steam thread "Mortol 2-C Help": stones half off edges, the bomb
  roller.
- [SG4] Steam thread "Is there a quick way to reset the level for Mortol?":
  "Hold X to give up".
- [YT] Players' record-video titles only (no video watched): the level
  names and the best result per level, used to judge our routes.

## Progress

- Built: `src/games/tintroop/` (game, levels, art, audio), slot 06. The
  levels are painted by a small script (kept outside the repo) into
  `tintroop_levels.c`; `level_errors` checks every level's shape.
- Tests: `tt_01` rituals and chains; `tt_02` … `tt_11` play every level from
  start to finish with plain button presses, each through its intended
  sacrifices (recorded once from a route plan with `cheat plan_go`);
  `tt_12` the Jack; `tt_13` kills and tags; `tt_14` rams; `tt_15` falling
  stones; `tt_16` water and pipes; `tt_17` fire; `tt_18` seeds and vines;
  `tt_19` records; `tt_20` saving; `tt_21` failing; `tt_22` movement, the
  forward-only view and giving up; `tt_23` bodies on spikes, switches and
  lasers; `tt_24` world 4's doors.
- Media: `docs/shots/tintroop.gif` (1-A: a lance ladder, then a Pop),
  `tintroop_title.png`, `tintroop_shelf.png`, `tintroop_bath.png`,
  `tintroop_candles.png`, `tintroop_jack.png`
  (`tools/shots/06_tintroop.ufs` replays the route tests).
