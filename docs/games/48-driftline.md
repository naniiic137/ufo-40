# 48 · DRIFTLINE

*Internal design document. Not shown in the product.*

## Tribute to

**Seaside Drive** (UFO 50 game #48, Mossmouth, "May 1989"). The rules were
researched from text only: the community wiki's raw and rendered pages,
Steam guides and threads, written reviews, TV Tropes and speedrun.com (as
search-engine summaries where the pages refused us) and the soundtrack's
listing (all under Sources; the full notes are in the research folder,
`48-seaside-drive.md`). No UFO 50 images, video, sprites, music, stage
layouts or text were used as references, and nothing was taken from a UFO
50 install. A last pass of searches before the build (the wiki's raw page,
the missing-manuals guide, two searches on co-op and the gun's tilt) added
one detail: "moving forward and back tilts your shots". The independent
review then read the missing-manuals guide's Seaside Drive section in full
[MANUAL] and found two accounts of how co-op is played [HANS], [STEAM-COOP];
this version carries its fixes.

**Map type: fixed.** Seaside Drive's waves are hand-placed and "exactly the
same everytime" [IGGY], so ours are too (`driftline_stages.c`, all our own
waves, swells and block layouts). Only the structure follows the original:

| Full scale | Seaside Drive | DRIFTLINE |
|---|---|---|
| Players | 1P, or 2P co-op in one car: one player drives, the other shoots [MH-MP], [HANS], [STEAM-COOP] | the same: P1 drives, P2 aims and fires |
| Stages | 4, fixed order: day, purple sunset, night, the sea [MH], [SEARCH-TVT] | 4: Harbour Road (morning), Sundown Strip (sunset), Moonlit Mile (midnight), Open Water (daybreak) |
| Bosses | one a stage, worth 10,000, 20,000, 30,000 and 40,000 [MH] | the Zephyr, the Orrery, the Man in the Moon and Old Crab, worth the same |
| Foes | 4 to 5 kinds a stage, each with a value [MH] | 4 or 5 kinds a stage at the same values (391 foes in all: 129, 91, 72 and 99) |
| Bonus stages | 3, after stages 1 to 3 when no car was lost, each with its own blocks [MH] | 3, of 18, 20 and 22 blocks |
| Run length | about 15 minutes; records 12:41 (1P, bonus stages skipped), 13:55 (cherry), 14:02 (2P) [SEARCH-SRC], [SEARCH-POPCAR] | the demo player's runs: about 13:30 in 1P and 12:30 in co-op, with all three bonus stages; 145 s of waves a stage before each boss |
| Lives | 3 in reserve (the gold route loses one in each of stages 1 to 3 and ends "with 0 lives remaining"); +2 a bonus coin, no score extends [SEARCH-SRC], [MANUAL], [MH] | 3 in reserve; +2 a coin |
| HUD | at the bottom: the score on the left, the lives on the right, a large power bar between [MANUAL] | the same, in the strip under the road; no boss health bar |
| Stat | Most Lives At Once [MH] | MOST CARS AT ONCE, on the title |
| Goals | 3 [MH], [SEARCH-GGC] | the same 3 |
| Saving | none: a run is one sitting [STEAM-SAVES] | the same; only the records are kept |

What each stage brings (our layouts): 1 kites and buzzers over the bay, road
hogs both ways, rotors, the Zephyr hanging far off all stage; 2 shards in
showers, prisms in pairs, hoops, tumblers on the road, the Orrery up in the
purple sky; 3 skulls, sheets, snappers, three slabs and the Beamdown saucer
passing over, the moon watching from its corner; 4 jellies, dartfish,
squirts and two sharks, whales sending swells in all the way through, and a
pair of claws at the horizon.

## Mechanics checklist

| Mechanic | How DRIFTLINE does it | Source | Test |
|---|---|---|---|
| One axis | the car only drives left and right along the road at the bottom; the world scrolls by | [MH], [ANI], [THEGAMER] | dfl_02 |
| Steering aims | the gun swings the way you steer, within 45 degrees of straight up (a 90-degree cone); let go and it stays at that angle; there is no aim lock | [SEARCH-MANUALS], [MH], [LIZ] | dfl_02 |
| UP | brings the gun back to straight up; it doesn't steer the car | [IGGY] | dfl_02 |
| Main gun | B held: a rapid stream into the cone | [SEARCH-MANUALS], [STATIC] | dfl_02, dfl_04 |
| Side guns | A held: rapid pairs, one along the road and one diagonally up (a V), ahead or behind | [SEARCH-MANUALS], [MH] | dfl_05 |
| Road foes | only the side guns reach them | [IGGY], [SEARCH-TVT] | dfl_05, dfl_06 |
| Charge meter | a bar at the bottom in three sections, grey, green, red, between the score and the cars | [MH], [MANUAL] | dfl_03 |
| The power drift | only driving left fills it, with sparks; everything else drains it slowly, standing still too | [MH], [STATIC], [IGGY] | dfl_03 |
| The section sets the guns | grey, green, red: 1, 2 and 4 damage; a main-gun shot every 8, 6 and 5 frames, side pairs every 12, 10 and 8; shots at 4.5, 5.25 and 6 px a frame ("in the red for the fastest, most powerful shots"); nothing more within a section | [MANUAL], [IGGY] | dfl_04 |
| One hit | any shot or foe loses the car | [MH] | dfl_07 |
| Respawn | the next car drives in from the left edge fully charged, and every foe but the boss is swept off | [MH], [MANUAL] | dfl_07 |
| Lives | 3 in reserve; more only from bonus coins (+2 each); no continue | [SEARCH-SRC], [MH], [SEARCH-TVT] | dfl_07, dfl_17 |
| Fliers that punish | a buzzer left hovering bursts into shots; a kite left loitering starts shooting | [IGGY] | dfl_08 |
| Two-stage kills | a road hog shot becomes a wreck; only the wreck pays | [IGGY] | dfl_06 |
| Stage 1 | kites 100, buzzers 50, road hogs (their wrecks) 500, rotors 500 | [MH], [IGGY] | dfl_06, dfl_08, dfl_23 |
| Stage 2 | shards 75 (come at you, down to the road and along it; shrapnel only when shot), prisms 1,000 (a beam to the road that walks inward; hits bounce them back), hoops 500 (bounce), tumblers 1,000 (a side shot pops one up; in the air every hit hurts it and throws it higher) | [MH], [IGGY] | dfl_09 |
| Stage 3 | skulls 250 (float at you), sheets 500 (fire at 45 degrees down; hard to pin down), snappers 500 (blue, then purple, then they burst), slabs 1,500, the saucer 3,000 (fires down) | [MH], [IGGY] | dfl_10 |
| Stage 4 | jellies 100, dartfish 200 (zigzag down, then chase on the road), squirts 1,000 (scoot, fire three behind and below), sharks 5,000; the whales' swells break over the road all stage | [MH], [IGGY] | dfl_11 |
| Boss 1 | an airship seen far off all stage; three turrets and nothing else, taken one at a time, firing faster as fewer are left | [IGGY], [SEARCH-TVT2] | dfl_12 |
| Boss 2 | a sphere with four orbiters; one shot off goes dark and bounces round the arena, and shots juggle it; the sphere turns yellow, then red as it is hurt | [IGGY] | dfl_13 |
| Boss 3 | the moon in the corner all stage is the eye of a face; a five-way spread aimed at you, a fist that sweeps low along a wave (drive under it), and the hand crossing the screen to drop four chasers | [IGGY], [SEARCH-TVT2] | dfl_14 |
| Boss 4 | only the mark on its head takes damage; six legs that change colour before they slam the road and hurt until fully raised; fans of 3, 4 and 5 bubbles with a hitbox generous to you; straight up is the best angle | [MH], [IGGY] | dfl_15 |
| Bosses foreshadowed | each boss is in the background during its stage | [STATIC], [SEARCH-TVT2], [IGGY] | (drawn) |
| Bonus stage | after stages 1 to 3, only when no car was lost; none after stage 4 | [MH], [IGGY] | dfl_16 |
| Breakout | the car is the paddle, a coin in a bubble the ball; the coin breaks a block in one hit, gunfire with enough shots; every block gone: the bubble bursts and the coin is worth two cars | [MH], [SEARCH-TVT] | dfl_17 |
| Bonus scoring | the first block 1,000, each one after 200 more; +800 for a block shot down | [MH] | dfl_17 |
| No drain | the meter doesn't drain in a bonus stage | [MH] | dfl_17 |
| Co-op | one car: player 1 drives (and drifts), player 2 aims and fires; one meter, one stock of cars, one paddle in the bonus stage | [HANS], [STEAM-COOP], [MH-MP] | dfl_20, dfl_22 |
| Damage shown on the bosses | no health bar: the Orrery changes colour, turrets past half smoke, every weak spot flashes when hit | [IGGY], [MANUAL] | (drawn) |
| Goals | Beacon: obtain a coin; Saucer: beat all four stages; Alien: beat the game with 300,000 points or more | [MH], [SEARCH-GGC] | dfl_17, dfl_19 |
| The cherry's maths | 300,000 needs at least two bonus stages and nearly every foe | [STEAM-CHERRY], [SEARCH-SRC] | dfl_23 |
| Fixed waves | the same every time | [IGGY], [STEAM-CHERRY], [LIZ] | dfl_23 |
| No save | a run isn't saved; the records are | [STEAM-SAVES], [CONV] | dfl_01, dfl_18 |
| Music | a tune for each stage | [BANDCAMP], [LIZ] | (audio) |

### Readings we had to choose

- **The gun's tilt** follows the way you drive (right tilts it right), two
  degrees a frame, to 45 degrees; UP brings it back at three degrees a
  frame, but only with the pad not steering (the steering wins, so there is
  still no aim lock). The side pair fires the way the pad points, or the way
  the car last drove with the pad let go. Both guns can fire at once.
- **The meter:** 720 points, grey below 240, green below 480, red above. A
  frame of driving left (moving left faster than 0.3 px a frame) adds 12, any
  other frame takes 1 away: about 40 frames (two-thirds of a second, some
  80 pixels of road) of drifting from empty to red, and twelve seconds from
  full to empty. Holding left against the end of
  the road is not a drift. Damage, rate of fire and shot speed by section as
  in the checklist ("fastest" could mean either, so both rise a little).
- **A new car** from the reserve is fully charged [MANUAL], with the gun
  straight up and two seconds of blinking safety. The first car of a run
  starts with the meter empty (no source says either way).
- **Lives:** 3 in reserve, settled by the gold route and the manual's "Lose
  your last life, and it's game over". Game over goes back to the title.
- **Foe hit points** (no source gives them): kite 2, buzzer 1, hog 4 and its
  wreck 4, rotor 10, shard 2, prism 24, hoop 8, tumbler 8, skull 3, sheet 8,
  snapper 10, slab 30, saucer 80, jelly 2, dartfish 3, squirt 14, shark 50.
  Contact with any foe loses the car ("hit by an enemy").
- **Which sprite is which in stage 3:** the 500 chaser is our snapper; the
  1,500 one (named after a falling stone face in its sprite) is our slab,
  which follows along the top and drops onto the road.
- **Bosses:** turrets 80 each, the Orrery 600 (orbiters 10), the eye 800,
  the mark 800. A boss's own chasers and drones are worth nothing, so the
  cherry can't be farmed. The fist rides a wave that touches the road at
  some points of the screen and not others.
- **The swells:** a whale breaches, and 100 frames later its wave breaks over
  44 pixels of road for 24 frames, marked as it nears. One every 255 frames,
  at fixed places, through the boss fight too.
- **Bonus blocks** take 12 points of gunfire (three red shots). A coin that
  falls ends the bonus stage with no coin; no car is lost. The coin drops
  into the car by itself once the bubble bursts.
- **Co-op:** one car [HANS], [STEAM-COOP]. Player 1's LEFT/RIGHT drive it
  (so the drift and the one meter are the driver's); player 2's LEFT/RIGHT
  swing the gun and UP straightens it, A fires the roof gun and B the side
  guns, which point the way player 2 holds (or last swung). Where the
  sources are silent, our reading: the driver doesn't fire, and the
  driver's steering doesn't move the gun; the gunner owns the aim and both
  triggers. On the Vita (one pad) the 2 PLAYERS line is greyed out, as in
  our other co-op cartridges.
- **Stage 4's time of day:** daybreak, closing the day-long drive.
- **High scores:** no table is documented, so there is none; the best score
  and the other records are kept.

## What is ours

- **Name:** DRIFTLINE (1989, Beamdown Softworks).
- **The crew:** Lou, who drives the Gull (a red convertible), and in 2P Dee,
  riding in the back on the gun.
- **The road:** Harbour Road, the Sundown Strip, the Moonlit Mile and Open
  Water, with their skies, sea, palms, lamps and graveyard hill; all four
  stages' waves, the swells and the three bonus layouts (a sun, a sunset and
  a full moon in blocks).
- **The foes:** kites, buzzers, road hogs, rotors; shards, prisms, hoops,
  tumblers; skulls, sheets, snappers, slabs; jellies, dartfish, squirts,
  sharks; and the whales.
- **Cameos:** the saucer that crosses the Moonlit Mile is the Beamdown
  saucer from UFO 40's boot screen (where the original has a Campanella
  ship), and the last boss is Old Crab from ROOFCAT's harbour, in its
  bandana (where the original borrows Ninpek's octopus).
- **The bosses:** the Zephyr, the Orrery, the Man in the Moon and Old Crab.
- **Music:** "Driftline" (title), "Harbour Road", "Sundown Strip", "Moonlit
  Mile", "Open Water", "Trouble on the Coast" (bosses), "High Tide" (the
  last boss), "Bubble and Bounce" (bonus stages), "The Last Mile" (ending),
  "Tail Lights" (credits) and three jingles.
- **Words:** the title line, the ending, the credits and every label.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three UFO
40 goals, which are Seaside Drive's own (gift, gold, cherry). The terminal
codes (NEED-PIES, WEAK-SHOT) are UFO 50 collection features; UFO 40 has no
terminal, so they are left out.

## Controls

| Input | Action |
|---|---|
| LEFT / RIGHT | drive; the gun swings the same way |
| LEFT (moving) | the power drift: fills the meter |
| UP | gun back to straight up |
| Hold A | roof gun, up into the cone (from a turret in the middle of the roof) |
| Hold B | side guns, along the road and diagonally up |
| START | pause |
| 2 PLAYERS | player 1: LEFT/RIGHT drive. Player 2: LEFT/RIGHT swing the gun, UP straightens it, B and A fire |
| Title | UP / DOWN pick 1 or 2 players, A starts, B to the library |

### Not confirmed (flagged)

| Control | What we chose | Why it is open |
|---|---|---|
| Which way the gun tilts | toward the way you drive | sources say steering tilts it, not which way [SEARCH-MANUALS], [MH] |
| How fast it swings | 2 degrees a frame, 45 at most | not documented |
| UP | re-centres at 3 degrees a frame, only when not steering | "holding up to shoot upward" [IGGY] doesn't say whether it beats steering |
| Side-gun direction | the pad's way, or the last way driven | supported, not spelled out: "aimed with the directional buttons" [MH], "to the left or right" [MANUAL] |
| Both buttons at once | both guns fire | not documented |
| The co-op split | P1 only drives; P2 alone aims and fires | the sources say "P1 moves, P2 shoots", not whether P1 can fire too [HANS], [STEAM-COOP] |

The original's mapping (B main gun, A side guns) is settled by the manual's own
text [MANUAL]. **Owner's change (06/10):** DRIFTLINE swaps them, so the button most
players reach for first (A: Z / Space / Cross) fires the roof gun, and its shots
leave from the barrel's tip on a turret in the middle of the roof. B fires the side
guns. The cartridge card's CONTROLS page can swap them back.
**Owner's addition (06/10):** the first road car of stage 1 shows a 4-second hint ("HOLD B AND STEER: SIDE GUNS HIT CARS"), since the roof gun can't reach the road and nothing else says so. The game-over text sits in a solid box so it stays readable.

## Tests

`tests/dfl_01` … `dfl_23` drive every rule with button presses, setting up a
moment with cheats where needed and then playing it. The demo player
(`dfl_bot_buttons` in `driftline_bot.c`) tries a few ways to steer for the
next half second, follows every shot, foe, beam, leg, fist and swell that
far ahead, and takes the safest one that lines its gun up on something, or
drifts left to charge when there is road for it; in the bonus stages it
plays the coin's path out, blocks and all, and gets under it. With real
presses it starts at the title and wins the whole game in 1P (`dfl_21`,
about 400,000 points with all three coins) and in co-op (`dfl_22`: the
driver on pad 1, and a gunner on pad 2 that swings the gun onto the
driver's target and turns the side guns on anything low). `dfl_23`
checks the waves as written and the cherry's arithmetic: every foe and boss
plus the best single bonus stage comes to 295,075, under 300,000; with the
two smallest bonus stages it is 315,875.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Seaside Drive" (raw and rendered): the car,
  the two firing modes, the meter, respawn and the sweep, the bonus stage
  and its scoring, enemy and boss values, co-op, the stat, the goals.
  https://ufo50.miraheze.org/wiki/Seaside_Drive
- [MH-MP] Miraheze, "Multiplayer" (co-op exists). https://ufo50.miraheze.org/wiki/Multiplayer
- [MANUAL] Steam guide "The missing manuals - How to play UFO 50 games",
  section 48, read in full by the review: the d-pad drives and aims; A the
  side pairs, B the main gun in a 90-degree cone; the score on the left,
  lives on the right and a large power bar at the bottom, in three
  sections, "keep it in the red for the fastest, most powerful shots"; a
  new car "will jump in, fully charged"; game over at the last life.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [HANS] thehans255, "One unique thing about every UFO 50 game": co-op makes
  "one player the driver and the other the gunner".
  https://www.thehans255.com/blog/2024/10/one-unique-thing-about-every-ufo-50-game/
- [STEAM-COOP] Steam thread "Top and Bottom 5 games?", a comment by Tim:
  co-oped "as the driver", a "P1 moves, P2 shoots" game.
  https://steamcommunity.com/app/1147860/discussions/0/4852155556170957174/
- [MH-CHEATS] Miraheze, "Cheats" (NEED-PIES, WEAK-SHOT).
  https://ufo50.miraheze.org/wiki/Cheats
- [IGGY] Steam guide "Iggy's compiled notes while cherrying every game in
  UFO50", section 48: drifting, only red matters, UP aims up, each enemy and
  boss, the route. https://steamcommunity.com/sharedfiles/filedetails/?id=3345726476
- [SEARCH-MANUALS] search summaries of the Steam guide "The missing manuals":
  B main gun in a 90-degree cone, A side pairs, the drift, the bar; "moving
  forward and back tilts your shots".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [SEARCH-GGC] search summary of the Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [STEAM-CHERRY] Steam thread "Seaside drive cherry?".
  https://steamcommunity.com/app/1147860/discussions/0/4700161643034793527/
- [STEAM-2P] Steam thread "Best 2-Player Games?".
  https://steamcommunity.com/app/1147860/discussions/0/4849904631717074845
- [STEAM-SAVES] Steam thread "Which games save progress?".
  https://steamcommunity.com/app/1147860/discussions/0/4849904176633630339/
- [LIZ] Lizstar's review. https://lizstar64.github.io/reviews/2024/10/21/UFO50-48.html
- [STATIC] Static Canvas, "The UFO 50 Diaries: Seaside Drive".
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-seaside-drive
- [ANI], [THEGAMER] mini reviews: movement locked to the bottom.
  https://anigamers.com/posts/ufo-50-mini-reviews-every-game/ ,
  https://www.thegamer.com/ufo-50-review/
- [SEARCH-TVT], [SEARCH-TVT2] search summaries of TV Tropes' recap.
  https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game48SeasideDrive
- [SEARCH-SRC] search summaries of speedrun.com's runs and the gold-route
  thread. https://www.speedrun.com/UFO_50/forums/2fb3y
- [SEARCH-POPCAR] search summary of Popcar's reviews.
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [BANDCAMP] "Seaside Drive - Stage 4". https://phlogiston.bandcamp.com/track/seaside-drive-stage-4
- [CONV] `00-ufo50-conventions.md` in the research folder.
