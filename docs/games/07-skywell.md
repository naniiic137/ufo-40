# 07 · SKYWELL

*Internal design document. Not shown in the product.*

## Tribute to

**Velgress** (UFO 50 game #7, Mossmouth). The rules were researched from
text only: the community wiki, Steam guides and threads, and written
reviews (listed under Sources). No UFO 50 images, video, sprites, music or
text were used as references.

**Map type: random.** Velgress builds its pit anew every run, so SKYWELL
does too. Structure copied from the text; generator rules ours:

| Structure | Velgress (text sources) | SKYWELL |
|---|---|---|
| Levels | 3, plus a locked 4th [W] | 3, plus a locked 4th |
| Floors | 30 per level ("sublevels") [W], numbered on the left of the play area [MM] | 30 per level, numbered 1-120 on the left wall |
| Shop | after each of levels 1-3, a choice of 3 items [W], [CH] | the Tinker's cart, three items |
| Key bird | on the 12th floor of levels 1-3, for 15 seconds [W] | the Keeper Owl, floor 12 (42, 72) |
| Final boss | appears on floor 104 and climbs with you [W] | the Well Eye, floor 104 |
| Random | levels are randomly generated [W], [PB], [MM] | a fresh seed every run |

### What each level is made of

| Level | Velgress (text sources) | SKYWELL |
|---|---|---|
| 1 | clouds, wooden boxes and bricks, each lasting longer than the last [IG]; coin blocks [W]; bats, the level's only creature [W] | the Roots: clouds, crates, bricks, coin blocks, a star block, bats hanging under platforms |
| 2 | no creatures [W]; TNT blocks [IG], [CH]; cloud mines [CH]; zappers (level 2 only) [W] | the Sparkworks |
| 3 | bubbles (level 3 only) [W]; fewer platforms, so fish are the platforms [W]; jellies, squids [W] | the Sky Reef |
| 4 | blocks, clouds, "smoke bombs" (cloud mines) and bounce platforms [BV]; metal (level 4 only) [W]; the mines return [IG]; coin blocks and star blocks only in levels 1-3 [IG]; one line of clouds left at the top to finish the boss [IG] | the Eye: no coin or star blocks; the top is a row of clouds until the eye is shut |

### Generator rules (ours)

- The shaft is 13 tiles (16 px) wide between two walls. A floor is 32 px
  tall; each floor gets one to three platforms (one or two in the reef),
  2-4 tiles wide, and the first is always within a jump of one on the
  floor below and never a trap.
- Coins float above about one platform in three.
- A star block turns up once in each of levels 1-3.
- In the Roots about one floor in four has a bat hanging under a platform.

## Mechanics checklist

| Mechanic | How SKYWELL does it | Source |
|---|---|---|
| Climb out | reach the top of each level's 30th floor | [W] |
| Controls | D-pad runs; A jumps, higher the longer it's held, steerable in the air; B shoots | [MM] |
| Double jump | A again in the air; out of jumps, Kip's suit turns red | [MM] |
| Jumps reset | on landing, and when bouncing off a creature | [MM], [W] |
| Hop on landing | holding A jumps again the instant she lands | [W] |
| The gun | rapid-fire, short range; fires the way she faces, or straight up or down; same rate held or tapped | [MM], [W] |
| The Grinder | always at the foot of the screen; it only rises when Kip climbs above the middle of the screen, and never goes down; touching it is the only way to die | [W], [CH], [SC], [BW], [LZ] |
| One life | a death ends the run; A goes straight back into a new pit | [W], [LZ] |
| Knocks | creatures and hazards never hurt, but the knockback is huge: she is flung hard across the shaft, out of control, bouncing off the walls; unless a platform catches her she drops floors, often into the Grinder. Recovery shortens it | [W], [MM], [BW], [RC], [TVT], [LZ], [PB] |
| After a knock | the knock costs her standing jump but not her jump in the air: she can jump as soon as she comes round | [IG], [CH], [SR] |
| Bouncing | every creature is safe to land on: she bounces off it with her jumps back, higher if A is held | [W], [IG], [CH] |
| Crumbling | every platform crumbles after she lands: clouds at once, then boxes, then bricks | [MM], [BW], [IG] |
| Clouds | break at once; shots break them and fly on through | [W], [IG] |
| Crates | break 0.2 s after a shot, 0.3 s after landing | [W] |
| Bricks (rocks) | break about a second after landing or a shot | [W] |
| Coin blocks | durable; every hit knocks out a coin, and the first hit starts its crumbling | [W], [IG] |
| Star blocks | solid to stand on; jumping off one rockets her up about six and a half floors, untouchable, whatever A does; with A still held at the end she floats down slowly, otherwise she just drops | [W], [CH], [IG] |
| Cloud mines | shot, or bumped, they burst into a row of clouds; bumping one also knocks her | [W], [CH] |
| TNT blocks | her weight pushes the plunger down; a moment later it fires three shots, up and to both diagonals; the Grinder sets them off too | [W], [IG], [CH] |
| Zappers (level 2) | a middle block with two electrified bars, a five-tile wall across or upright; a shot on the middle block turns it round | [W], [IG] |
| Bubbles (level 3) | flimsy like clouds; Lightning doesn't protect them | [W], [IG] |
| Metal (level 4) | glides side to side; landing on it bounces her up with her jumps back; from below it knocks her; shots bounce off, and it flashes | [W], [BV], [CH] |
| Bats (level 1) | asleep until she comes close or passes underneath, then they chase her; one hit pops them; landing on one pops it; she can outclimb them and leave them to the Grinder | [W], [CH], [BW], [SR] |
| Level 2 | no creatures | [W] |
| Fish (level 3) | swim side to side; good to stand on | [W], [CH] |
| Jellies (level 3) | fast; split into two small ones when shot or landed on | [W], [CH], [IG] |
| Squids (level 3) | one at a time; a mark shows where it will drop; it lands on the Grinder and sits there a moment, a last thing to bounce on | [W], [IG] |
| Coins | scattered through the pit, counted top right; spent in the shop | [W], [MM] |
| Shop | after levels 1-3, three of the six items | [W], [CH] |
| Items | Recovery 20 (shorter knocks), Power 15 (breaks terrain and hurts bosses faster), Double Jump 30 (one more jump in the air), Lightfoot 10 (platforms she stands on break more slowly), Magnet 5 (shots collect coins), Lightning 20 (shots don't break clouds, still break bubbles); the first four stack | [W] |
| Keeper bird | on floor 12 of levels 1-3 it flies near the top of the screen for 15 s without attacking, holding the column it came in on; shot enough, it drops a key and flees; if it gets away it stops coming | [W], [IG] |
| Level 4 | opens only with all three keys | [W] |
| Final boss | on floor 104 she climbs with Kip: the eye fires small shots at her, each hand drops lightning straight down; the hands can be shot off; the eye must be shot to win; she must be beaten before the exit counts | [W] |
| Boss attacks (medium confidence) | each hand also fires spreads of five; the lightning breaks the platforms under it; in turn a fist sweeps low across the shaft in a wave, and the eye swoops across letting out four lunging minions | [IG], [BV] |
| HUD | floor numbers and the dotted best line on the left, cogs top right; the red suit is the only jump counter | [MM], [TVT] |
| Best height | a dotted line marks the highest floor ever reached | [MM], [SC] |
| Records | items bought, most cogs, most keys, and the best floor | [W] |

### Readings we had to choose

The sources give no numbers for these, so they are ours:

- Kip runs 1.4 px/frame; a full jump rises 44 px (a floor and a bit), a jump
  in the air 32.
- A shot every 10 frames (so a coin block gives up to five coins before it
  goes), 5 px/frame, reaching 110 px.
- A knock lasts 56 frames, 10 fewer per Recovery (never under 20), then 40
  frames of blinking safety. It throws her at 2.8 px/frame across and
  1.8 up; unchecked, it drops her about four floors.
- A star ride lasts 52 frames at 4 px/frame; the float after it falls at
  most 0.5 px/frame.
- Bats fly 0.75 px/frame, slower than Kip runs or climbs. TNT goes off
  60 frames after the plunger goes down.
- The Keeper Owl takes 14 hits. The Well Eye has 60, each hand 14. The
  Eye watches for 8 s (shots every second, a hand spread every 2.5 s,
  lightning every 1.8 s), then sweeps a fist (4 s), watches again, then
  swoops (4.3 s), and so on.

## What is ours

- **Name:** SKYWELL (1984, Beamdown Softworks).
- **Hero:** Kip, a scrap-diver whose balloon popped over the Skywell, an
  ancient shaft that goes down forever.
- **The roller:** the Grinder, a drum of teeth that follows her up.
- **Levels:** the Roots (1), the Sparkworks (2), the Sky Reef (3) and the
  Eye (4).
- **Characters:** the Tinker (shopkeeper), the Keeper Owl, the Well Eye.
- **Currency:** cogs.
- **Music:** original UFO-MML tracks.

## Additions: none

Only the platform needs every UFO 40 cartridge has:

- the START pause menu, and a how-to-play page on SELECT at the title;
- saving: the best floor, items bought, most cogs and most keys are kept
  (as Velgress tracks); a run in progress is not saved, as in the original;
- the three UFO 40 goals, which replicate Velgress's own three:

| UFO 40 goal | Condition | Velgress's goal |
|---|---|---|
| Beacon | hold 2 keys | gift: collect 2 keys |
| Saucer | climb out of the Skywell (clear level 3) | gold: escape Velgress |
| Alien | beat the Well Eye | cherry: unlock the final level and defeat Charkas |

## Controls

| Input | Action |
|---|---|
| D-pad | run; hold up or down to aim |
| A | jump (hold for height; hold to hop again on landing) |
| A in the air | jump again |
| B | shoot (down only in the air) |
| START | pause |
| SELECT (title) | how to play |

## Sources

- [W] UFO 50 Wiki (Miraheze), "Velgress": controls, hop on landing, the
  gun, the Spike Roller, knocks, bouncing on enemies, every platform type
  and its timings, enemies per level, Cavian and the keys, the shop items
  and prices, Charkas, levels and floors, goals.
  https://ufo50.miraheze.org/wiki/Velgress
- [MM] Steam guide "The missing manuals - How to play UFO 50 games":
  controls, the double jump turning her red, jumps resetting on landing,
  the gun's aim, fragile blocks, being flung, floor numbers, the dotted
  line, the coin counter, the shop.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [CH] Steam guide "Velgress How To Cherry": the roller moves when you go
  above the middle of the screen; recovering after a hit with the double
  jump; star blocks safer holding jump; level-1 monsters only trigger
  under or close to them and die in one hit; red TNT blocks and blinking
  cloud mines in level 2; fish, jellies; grey level-4 blocks that flash;
  the birds and keys; a choice of 3 upgrades.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3343451670
- [IG] Steam guide "Iggy's compiled notes while cherrying every game in
  UFO50": landing on enemies bounces you, a hit steals your first jump,
  level-1 clouds / boxes / bricks, the 5-tile electric wall, jellies
  splitting when jumped on, squids dropping from a mark onto the spikes,
  coin blocks' timer, TNT plungers and the three-shot spread, mines
  returning in level 4, "you have far more time than you think".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3345726476
- [BV] Steam thread "Beat Velgress!": level 4's blocks, clouds, smoke bombs
  and bounce platforms; the boss.
  https://steamcommunity.com/app/1147860/discussions/0/4852155152088633016/
- [BW] Bloomed Wings, "UFO 50's Velgress is simple perfection": the screen
  keeps Alpha in the centre; enemies send her flying, stunned for a moment;
  bouncing refreshes the air jump; fast climbing leaves enemies behind;
  clouds vanish almost at once.
  https://bloomedwings.com/2025/08/21/ufo-50s-velgress-is-simple-perfection-and-perfect-simplicity/
- [SC] Static Canvas, "The UFO 50 Diaries: Velgress": the roller only moves
  when you do, bats, the dotted line.
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-velgress
- [LZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 7": only the pit
  kills, everything else stuns and throws you; press A to go straight back
  in; four levels and a shop.
  https://lizstar64.github.io/reviews/2024/10/09/UFO50-7.html
- [PB] Popcar's blog, "Reviewing Every Single UFO 50 Game": random levels,
  coins and a shop between stages.
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [RC] Record Crash, "Game Review: UFO 50": a hit takes control away and
  you hope to land on a cloud.
  https://recordcrash.substack.com/p/game-review-ufo-50
- [SR] Search-engine summaries (text only) of guides: bats knock you
  sideways and steal your first jump, so jump as the stun runs out; landing
  on top of bats removes them.
- [TVT] TV Tropes, Velgress recap (search snippets only): "a tremendous
  amount of knockback" that can throw her into the spikes; the suit's
  colour shows the jumps left.
  https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game7Velgress

## Progress

- Built: `src/games/skywell/` (game, art, audio), slot 07.
- Tests: `tests/sw_01` … `sw_18`: the start, the level-1 mix and a quick
  restart, jumps, crumbling and Lightfoot, shooting every platform type,
  bats waking and chasing, knocks (flung far, deadly over the Grinder,
  caught by a platform, shortened by Recovery, the jump in the air left),
  the one-way camera and the Grinder, star blocks (launch, fixed ride,
  float), cloud mines and TNT, the Keeper Owl and its key, the shop and
  items bought, all three goals, saving, the reef's creatures, metal and
  zappers, stomping and shooting bats and leaving them to the Grinder,
  and the Eye's level (`sw_18`: its contents, the cloud top, every attack,
  the ledge out). `sw_16` and `sw_17` climb all of level 1 on two
  different pits with plain button presses only.
- A demo climber (`cheat autoplay`, used only by scripts) looks ahead and
  picks its landings; `cheat botlog` prints its presses as script lines,
  which is how `sw_16` and `sw_17` were recorded.
- Media: `docs/shots/skywell.gif`, `skywell_title.png`, `skywell_climb.png`,
  `skywell_reef.png`, `skywell_eye.png`, `skywell_shop.png`
  (`tools/shots/07_skywell.ufs`).
