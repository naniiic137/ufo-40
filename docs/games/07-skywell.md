# 07 · SKYWELL

*Internal design document. Not shown in the product.*

## Tribute to

**Velgress** (UFO 50 game #7, Mossmouth). The rules were researched from
text only: the community wiki, written reviews and forum/search summaries
(listed under Sources). No UFO 50 images, video, sprites, music or text were
used as references.

**Map type: random.** Velgress builds its pit anew every run, so SKYWELL
does too. Structure copied from the text; generator rules ours:

| Structure | Velgress (text sources) | SKYWELL |
|---|---|---|
| Levels | 3, plus a locked 4th [W] | 3, plus a locked 4th |
| Floors | 30 per level, marked on the left side of the screen [SS]; counted on (floor 104 is in level 4) [W] | 30 per level, numbered 1-120 on the left wall |
| Shop | after each level [W], [SS] | the Tinker's cart after each level |
| Key bird | on the 12th floor of levels 1-3, for 15 seconds [W] | the Keeper Owl, floor 12 (42, 72) |
| Final boss | appears on floor 104 and climbs with you [W] | the Well Eye, floor 104 |
| Random | levels are randomly generated [W], [PB] | a fresh seed every run |

### Generator rules (ours)

- The shaft is 13 tiles (16 px) wide between two walls. A floor is 32 px
  tall; each floor gets one to three platforms, 2-5 tiles wide, placed so
  that at least one is within a single jump of a platform on the floor
  below.
- Platform types are drawn per level from the table under "Platforms";
  harder types grow more common as the floor number rises.
- Coins float above about one platform in three; one coin block every few
  floors.
- Level 1 wakes a bat every few floors. Level 2 has no creatures [SS].
  Level 3 sends minnows, jellies and one squid at a time.
- A star block turns up about once per level.

## Mechanics checklist

| Mechanic | How SKYWELL does it | Source |
|---|---|---|
| Climb out | the goal of each level is its 30th floor | [W] |
| The roller | a spiked drum sits at the bottom of the screen and only moves up when Kip climbs above the middle of the screen; touching it is the only way to die | [W], [SS], [LZ] |
| One life | a death ends the run; the next run is a new pit | [W], [PB] |
| Stun | every foe and hazard stuns Kip and knocks her about instead of hurting her | [W], [LZ] |
| Crumbling | platforms break soon after she lands on them | [LZ], [W] |
| Jumping | a jump and one more in mid-air; each Double Jump bought adds another; holding jump hops again on landing | [W] |
| Shooting | a gun that fires in four directions at a fixed rate, held or mashed | [W] |
| Clouds | flimsy; shots pass through and break them | [W] |
| Crates | break 0.2 s after being shot or 0.3 s after being stood on | [W] |
| Rocks | tough; break about a second after being stood on or shot | [W] |
| Star blocks | touching one makes her invincible and shoots her up about six floors | [W] |
| Coin blocks | tough; shooting them knocks coins out | [W] |
| Cloud mines | stun on touch; shot, they spread into a row of safe clouds | [W] |
| Bomb platforms | standing on one arms a mine that later bursts into shots | [W] |
| Zappers (level 2) | two sparking pillars; the block between them can be stood on for good | [W] |
| Bubbles (level 3) | weak platforms like clouds | [W] |
| Metal (level 4) | slide side to side; safe from above, stun from below | [W] |
| Bats (level 1) | asleep until she comes near, then give chase; one hit | [W], [SC] |
| Level 2 | no creatures | [SS] |
| Minnows (level 3) | swim side to side; good to stand on | [W] |
| Jellies (level 3) | split into two small ones when shot | [W] |
| Squids (level 3) | drop from above one at a time down to the roller; she can bounce off them | [W] |
| Coins | scattered through the pit; spent in the shop | [W], [SS] |
| Shop | after each level; a few of the six items are on offer each time | [W], [PB] |
| Items | Recovery 20 (shorter stun), Power 15 (stronger shots), Double Jump 30 (one more mid-air jump), Lightfoot 10 (platforms crumble slower), Magnet 5 (shots collect coins), Lightning 20 (shots don't break clouds); the first four stack | [W] |
| Keeper bird | on floor 12 of levels 1-3 it flies near the top for 15 s; shot enough, it drops a key and flees; if it gets away, it stops coming | [W] |
| Level 4 | opens only with all three keys | [W] |
| Final boss | on floor 104 she climbs with Kip, the eye firing small shots and the two hands dropping lightning; the hands can be shot off; the eye must be shot to win; she must be beaten before the exit counts | [W] |
| Best height | a dotted line marks the highest point reached | [SC] |

### Readings we had to choose

- Kip runs 1.4 px/frame; a jump rises about 44 px (a floor and a bit);
  mid-air jumps rise about 32.
- Shots every 9 frames, 5 px/frame, reaching about 110 px. Power adds one
  point of damage per purchase.
- Stun lasts 36 frames, 8 fewer per Recovery (never under 12), then 30
  frames of blinking safety.
- Clouds last 12 frames once stood on; Lightfoot adds half again per
  purchase to every crumble timer.
- The shop shows three items, chosen at random; Magnet and Lightning can
  only be bought once.
- The Keeper Owl takes 14 hits. The Well Eye has 60, each hand 14.
- Coins: floating ones are worth 1, a coin block gives 1 per hit up to 6.

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

- the START pause menu;
- saving: the best floor, most cogs and most keys are kept (as Velgress
  tracks); a run in progress is not saved, as in the original;
- the three UFO 40 goals, which replicate Velgress's own three:

| UFO 40 goal | Condition | Velgress's goal |
|---|---|---|
| Beacon | hold 2 keys | gift: collect 2 keys |
| Saucer | climb out of the Skywell (clear level 3) | gold: escape Velgress |
| Alien | beat the Well Eye | cherry: unlock the final level and defeat Charkas |

## Controls

| Input | Action |
|---|---|
| D-pad | run; aim up or down |
| A | jump (again in mid-air; hold to hop on landing) |
| B | shoot (hold or tap; up/down + B shoots up/down) |
| START | pause |

## Sources

- [W] UFO 50 Wiki (Miraheze), "Velgress": controls, the roller, stun, the
  gun, jumps, every platform type and its timings, enemies per level,
  Cavian and the keys, the shop items and prices, Charkas, the levels and
  floors, goals. https://ufo50.miraheze.org/wiki/Velgress
- [SS] Search summaries of the Steam guides ("Velgress How To Cherry", "The
  missing manuals"): the roller only moves when you go above the middle of
  the screen, floor markers on the left, 30 floors a level, no enemies on
  level 2, a shop at the end of each stage.
- [LZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 7": floors crumble as
  you land, only the pit kills, three mini bosses that flee, four levels
  and a shop.
  https://lizstar64.github.io/reviews/2024/10/09/UFO50-7.html
- [SC] Static Canvas, "The UFO 50 Diaries: Velgress": the roller only moves
  when you do, bats, a dotted line at your highest point.
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-velgress
- [PB] Popcar's blog, "Reviewing Every Single UFO 50 Game": random levels,
  coins and a shop between stages, the shop's selection varies.
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/

## Progress

- Built: `src/games/skywell/` (game, art, audio), slot 07.
- Tests: `tests/sw_01` … `sw_14`: the start and the generator's
  reachability check, jumps and extra jumps, crumbling and Lightfoot,
  shooting each platform type, stun and Recovery, the one-way camera and
  the Grinder, star blocks, mines and bombs, the Keeper Owl and its key,
  the shop, all three goals and the fourth level's Well Eye, saving, the
  level-3 creatures and the level-2/4 hazards.
- A demo climber (`cheat autoplay`, used only by scripts) climbs generated
  pits for the README GIF; with seed 6 it reaches floor 28 of 30, which
  doubles as a sanity check that generated levels can be climbed.
- Media: `docs/shots/skywell.gif`, `skywell_title.png`, `skywell_climb.png`,
  `skywell_reef.png`, `skywell_eye.png`, `skywell_shop.png`
  (`tools/shots/07_skywell.ufs`).
