# 30 · FLINTHOLD

*Internal design document. Not shown in the product.*

## Tribute to

**Rock On! Island** (UFO 50 game #30, Mossmouth, "April 1987"). The rules
were researched from text only: the community wiki's raw page, Steam guides
and threads, written reviews, TV Tropes, speedrun.com's levels and run times
and YouTube descriptions (listed under Sources; the full notes are in the
research folder, `30-rock-on-island.md`, and the independent review is
`ufo40-review/30-review.md`). No UFO 50 images, video, sprites, music, stage
layouts or text were used as references, and nothing was taken from a UFO 50
install.

**Map type: fixed.** Rock On! Island's stages and waves are hand-made and
"predetermined and predictable" [W], so ours are too (`flinthold_levels.c`,
all our own layouts and waves). Only the structure follows the original:

| Full scale | Rock On! Island | FLINTHOLD |
|---|---|---|
| Stages | 10 [LZ], [GC], [SR] | 10, in the same kinds of places (a first stage, a spiral, underbrush, a wasteland, a crossroads, an oasis, a jungle, a stage of fliers, a maze, the lords' last stand) |
| Villages | the Village of Peace (with the in-town tutorial), the Dinosaur Camp (the second village, next to The Oasis) and the Remote Island (east, right of Maze of Death); two of them hidden until you walk to them [W], [GG], [TVT], [SR], [TIPS-C], [CHERRY] | Hearthome (its folk explain hens and fire pits), the Scale Camp (hidden, off Palm Spring) and Far Isle (hidden, off The Tangle) |
| Waves | a set number per stage; the last stage has 20 [W], [ST-20] | 8, 8, 8, 9, 9, 10, 10, 10, 14 and 20 (106 in all), 3 or 4 in a village (11) |
| Beasts per stage | stages take the best players 3–4 min, Terror Overhead 5 min, Maze of Death 8.5 min and the last stage 15.5 min [SR-RUNS], [WR] | 91, 99, 76, 84, 74, 162, 99, 82, 204 and 426 (1,397 in all); 29, 23 and 19 in the villages |
| Spawn points | one or more per stage; "most levels with 1 enemy cave", some with 3+ [W], [HARD-C] | 1 to 3 |
| Foes | 6 kinds [W] | 6: nipper, redback, fangcat, clubtail, glider, gnat |
| Bosses | 5: the woolly mammoth and the Four Emperors [W] | 5: the shagtusk and the Four Lords (Snap, Jaw, Plate and Gale) |
| Hunters | caveman and 9 upgrades in three trees [W] | thrower and 9 upgrades in three trees |
| Pim's upgrades | 2 tracks of 3 levels [W] | the same |
| Other things to place | chickens and campfires [W] | hens and fire pits |
| Obstacles | rocks and bushes to dig out [W] | boulders and bushes |
| Goals | 3 [W] | the same 3 |
| Stats | Cave Damage, Most Cavemen, Most Chickens, Most Upgrade Spending [W] | the same four, on the title screen |

What each stage brings: 1 nippers and redbacks, a shagtusk at the end;
2 fangcats, on a long spiral; 3 clubtails, two roads and bushes on the best
spots; 4 two long straight roads and Lord Snap; 5 three roads that cross;
6 a pond road that stays quiet for six waves, then gnats in swarms from the
jungle road; 7 a jungle rush of fangcats and gnats, a shagtusk among them,
and Lord Jaw; 8 gliders, and Lord Gale; 9 a 53-tile maze over 14 waves, and
Lord Plate; 10 three roads, 20 waves and all Four Lords at once.

**How long a stage takes.** The waves were scaled up after the review so the
stages take about as long as the original's. The demo player (which builds
slowly, walking to every tile) takes these times; "in waves" leaves out its
build time, which is closest to a clean run's time:

| Stage | Demo player | In waves | Original's record [SR-RUNS] |
|---|---|---|---|
| First Tracks | 266 s | 166 s | 188 s |
| The Coil | 342 s | 271 s | 232 s |
| Fernbrake | 312 s | 253 s | 235 s |
| Ashflats | 342 s | 213 s | 209 s |
| Four Ways | 439 s | 231 s | 245 s |
| Palm Spring | 530 s | 336 s | 229 s |
| Vine Run | 361 s | 247 s | 217 s |
| Sky Scare | 541 s | 489 s | 313 s |
| The Tangle | 838 s | 604 s | 512 s |
| The Four Lords | 919 s | 600 s | 931 s |
| Hearthome / Scale Camp / Far Isle | 56 / 101 / 46 s | 48 / 91 / 42 s | 81 / 50 / 50 s |

## Mechanics checklist

| Mechanic | How FLINTHOLD does it | Source | Test |
|---|---|---|---|
| Waves on roads | beasts come from each spawn point down a fixed road to the cave; each stage's waves are always the same | [W], [MM] | fh_03, fh_18 |
| Next wave shown | the HUD shows which kinds come next, not in what order | [W] | (drawn) |
| Build phase | lasts as long as you like | [W] | fh_03 |
| The horn | A facing the road, the edge of the field or anything else that offers nothing (and at the cave, and on open ground) sounds the horn right there; LEAVE STAGE is always on offer | [MM-30] | fh_22 |
| Building in a wave | hunters, hens, fire pits, digging, upgrades and selling all work during a wave | [W] | fh_03 |
| The pay-out | when a wave is over its meat ticks in over 2.5 s; spending meanwhile is fine; hens put down in the first half still lay, and the cooking comes at the end | [HARD-C], [YMMV] | fh_21 |
| The cave | 30 hearts; each beast that reaches it takes its own number; at 0 the stage is lost (try again or leave) | [W], [TIPS-C] | fh_12 |
| Boss wave | every stage's last wave brings a boss; later on shagtusks also come as ordinary beasts | [W], [TVT] | fh_18 |
| Meat | earned at once for each kill, and 20 for every spawn point after a wave; 99 at most, the rest lost | [W], [PC] | fh_03, fh_04 |
| Hens | cost 10, sell 10; 5 meat after each wave while not cooked | [W] | fh_05 |
| Cooking | one fire pit beside a hen cooks it over two waves, two pits in one; a cooked hen gives no more meat but sells for 30 | [W] | fh_05, fh_21 |
| Fire pits and hunters | each pit beside a hunter adds 50 % damage, up to +200 %; slows last 50 % longer per pit | [W] | fh_06 |
| Where to build | not on the road, not next to it, not on boulders or bushes | [W] | fh_02 |
| Digging | bushes and boulders cost meat to dig out, then the ground is free | [W] | fh_13 |
| Pim walks | eight ways, fairly slowly (about twice a walker's pace); not through thickets | [W], [MM], [PC], [TO] | fh_01 |
| Pim throws | hold B: she throws the way the pad last pointed, diagonals too, again and again, and can't walk or turn while she does | [W], [MM], [HARD-C] | fh_01, fh_23 |
| A | works on the tile Pim faces: build, dig, upgrade, sell, talk, the horn; at the cave also her upgrades | [MM], [MM-30] | fh_02, fh_10, fh_22 |
| Her arm | 2, 3 and 6 throws a second, reaching 2, 3 and 4 tiles; the second and third cost 30 and 50 | [W] | fh_10 |
| Her weapon | bones 10, stone axe 20 (30 meat), fire axe 30 through armour (50 meat) | [W], [TVT] | fh_10 |
| Only between waves | her upgrades are sold only in the build phase | [W] | fh_03 |
| Pim knocked down | any beast that touches her, gliders too, stuns her; she comes back at the cave a few seconds later and the cave loses a heart | [W], [LZ], [TVT] | fh_11 |
| Shots | hunters throw straight at where a beast is when they throw; quick beasts are often missed | [FIRE-C], [YMMV] | fh_24 |
| Thrower | 10 a bone, every 5/6 s, 2 tiles | [W] | fh_06 |
| Spear / Barb / Bow | 30; 50 and slows; 100 every 2 s at 6 tiles, flies over thickets and carries on into the next beast | [W], [YMMV] | fh_07, fh_08, fh_09 |
| Sling / Hurler / Boulder | 8 twice as often at 3 tiles; 10 four times a second at 4 tiles; a rolling stone (reach 2) that crushes every walker it passes, every frame, for three tiles of road, and never touches a flier | [W], [ST-TIPS], [FIRE-C] | fh_09 |
| Torch / Blaze / Pitch | 20 fire through armour; 5 fire twelve times a second at half speed; 10 twice a second that slows and makes fire hurt 20 % more, once per beast | [W] | fh_08 |
| Selling | a hunter sells for half of what went into it | [MM], [MM-30] | fh_07 |
| The beasts | nipper 50, redback 80 (10 % armour), fangcat 40 (twice as fast), clubtail 120 (80 % armour), glider 80 (60 % armour, 30 % against fire, none against arrows; flies straight, half speed), gnat 60 (flies; goes for Pim when she is near, then on to the cave) | [W], [TVT] | fh_08, fh_11 |
| The bosses | shagtusk 2000, Lord Jaw 3000 (30 to the cave), Lord Plate 4000 (all armour, none against fire), Lord Gale 2000 (flies), Lord Snap 2000 (quick), with the original's armour, speed, meat and cave damage | [W] | fh_12, fh_18 |
| The Four Lords together | in the last wave of the last stage | [TVT] | fh_18 |
| The map | an island of stops; the road forks after the first stage (The Coil or Fernbrake), and again after Fernbrake; stages can be played again | [W], [TIPS-C], [WR], [YT-T], [BUG] | fh_14 |
| Hidden villages | two villages don't show until you walk to them from the stage beside them | [W], [TVT] | fh_14 |
| Villages | a short defence with the villagers standing about to talk to | [SR], [FIRE], [TIPS-C], [LZ], [TERM] | fh_17, fh_v1-3 |
| Saving | after every cleared stage; each stage's best result is kept | [MM], [BUG] | fh_15 |
| Goals | Beacon: find the Scale Camp (the second village); Saucer: clear the last stage; Alien: every stage's best clear without the cave hurt, checked only when the last stage is cleared | [W], [GG], [CHERRY] | fh_14, fh_15, fh_16 |
| The better ending | clearing the last stage with every stage perfect | [LZ], [TVT] | fh_15, fh_16 |
| Stats | cave damage, most hunters, most hens, most spent on upgrades | [W] | fh_20 |
| No music while building | the tunes start with the horn | [LZ] | (audio) |
| The demo | left alone on the title, the cartridge plays one of the first stages; any button ends it | [TIPS-C] | fh_25 |

### Readings we had to choose

- **Costs no source gives:** a thrower 10; spear, sling and torch 20; the
  second rung 40; a fire pit 10; digging a bush 5 and a boulder 10; a fire
  pit sells for 5, a half-cooked hen for 10.
- **Speeds:** a walker goes about 1.2 tiles a second, and the others by the
  wiki's multipliers. Clubtails walk at a walker's pace: the wiki gives them
  no speed note, though TV Tropes calls the armoured stegosaurs slow; we
  kept the wiki. Pim walks 11/16 of a pixel a frame, about twice a walker.
- **Waves:** our own contents, scaled so each stage takes about as long as
  the original's (the table above). The first wave of a stage is about the
  size of a small wave; the waves grow to full size by 70 % of the way
  through, and big groups come as several sub-waves of up to six. Beasts in
  a group leave 0.8 s apart (fangcats 0.5 s, bosses 2.5 s), groups 1.5 s
  apart.
- **The pay-out:** 2.5 s; the spawn points' meat ticks in a point a frame,
  the hens lay half way through, and the cooking comes at the end.
- **Shots:** thrown at the beast's position at that moment; they fly their
  reach and a tile more, and hit the first beast within 6 pixels (8 for
  Pim's, 6 more for a boss). Every shot but an arrow is stopped by thickets,
  bushes, boulders and huts. Reach is measured from the middle of the
  hunter's tile to the beast, plus half a tile.
- **The boulder hunter** aims at the beast furthest along within 2 tiles and
  rolls its stone that way; the stone rolls on for 3 tiles once it reaches
  the road, crushing walkers for 5 a frame.
- **Pitch** hurts as a plain hit (armour counts), lasts 2 s (longer by fire
  pits) and never lands twice on one beast ("an enemy cannot be tarred
  twice"). A barb slows for 1 s. Slowed beasts go half speed.
- **The fire axe** ignores all armour; its bones and axe are plain hits.
- **Pim:** knocked down for 1 s, then gone for 2 s before she comes back
  beside the cave. Beasts touch her within 10 pixels (16 for bosses); a
  gnat goes for her within 3 tiles. A fresh press of a new direction turns
  her on the spot, so she can face the tile next to her.
- **Starting meat:** 30 on every stage.
- **The map:** our own island and roads, forking where the evidence says the
  original's does (Underbrush playable after only the first stage;
  Crossroads and Jungle Rush both before Wasteland is possible). Palm Spring
  opens from Ashflats only, so the second village still waits behind the
  wasteland.
- **Villages:** speedrun.com lists all three villages as levels timed to a
  "You Win!", a player tested fire pits there "without consequences", and
  the Village of Peace holds the in-town tutorial, so each village is a
  short defence (3 or 4 waves) with talking villagers. Village results
  don't count towards the goals.
- **Which lord leads which stage** and every wave's contents are ours; only
  "the final wave brings bosses", "mammoths later become normal enemies"
  and "all four at the end" are the original's.
- **The demo** starts after 12 s on the title and runs up to 90 s.

## What is ours

- **Name:** FLINTHOLD (1987, Beamdown Softworks).
- **Heroine:** Pim, youngest chief of the Hearth Clan, with a bone in her
  hair and a spotted fur.
- **The island:** Flint Isle, its smoking peak, and the ten stages: First
  Tracks, The Coil, Fernbrake, Ashflats, Four Ways, Palm Spring, Vine Run,
  Sky Scare, The Tangle and The Four Lords; the villages Hearthome, the
  Scale Camp and Far Isle. All 13 layouts and all 117 waves.
- **The beasts:** nippers, redbacks, fangcats, clubtails, gliders, gnats,
  the shagtusk, and the Four Lords of the Scale: Snap, Jaw, Plate and Gale.
- **The hunters:** thrower, spear, barb, bow, sling, hurler, boulder, torch,
  blaze and pitch; hens and fire pits; the horn.
- **The villagers:** Old Bram, the Little One and Aunt Ola; Elder Skink,
  Bask, Gecka and Newt of the Scale Camp; the four tusklings of Far Isle,
  who wait for Beamdown's saucer (a UFO 40 joke: built for forty boxes, it
  carries fifty).
- **Music:** "Flinthold" (title), "The Island" (map), "Hearth Song"
  (villages), "Horns at Dawn", "Ash and Fern", "Vines and Wings" (battles),
  "The Four Lords", "Embers Home" (ending) and three jingles.
- **Words:** the story, the villagers' lines, the endings, the goal lines
  and every label.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu (with RESTART
STAGE and BACK TO THE MAP under RESUME) and the three UFO 40 goals, which
are Rock On! Island's own (gift, gold, cherry). The terminal code that opens
every stage and the debug keys are UFO 50 collection features; UFO 40 has
no terminal, so they are left out.

## Controls

| Input | Action |
|---|---|
| D-pad | walk eight ways (a fresh press of a new way turns Pim first) |
| Hold B | throw the way the pad last pointed, diagonals too |
| A | on the faced tile: build, dig, upgrade, sell, talk; facing the road or nothing: sound the horn or leave; at the cave: her arm and weapon |
| A / B in a menu | choose / close; UP and DOWN move |
| START | pause |
| Map | d-pad walks the roads, A plays the stage, B to the title |

## Not confirmed

- Every cost the wiki doesn't give (hunters, fire pits, digging).
- Every speed, gap and time above, and how reach is measured; whether
  clubtails walk slower.
- How the boulder's stone rolls, what pitch's own hit is, and whether the
  fire axe ignores all armour.
- What a village holds beyond its folk (we made each a short defence).
- The map's exact shape.

## Tests

`tests/fh_01` … `fh_25` drive the rules with button presses, or set up a
moment with cheats and then play it. The demo player (`fh_bot_buttons` in
`flinthold_logic.c`) follows a build plan for each stage: it walks to each
tile, faces it, opens the menu with A and picks the line, farms hens
between fire pits (selling each cooked hen and putting a new one down, in
the pay-out too), buys Pim's upgrades, sounds the horn from its post and
throws from there, diagonally where fliers cross. Once its plan runs out it
keeps upgrading hunters and adds throwers where they cover the most road.
With real presses it wins all ten stages (`fh_s01` … `fh_s10`; the last one
takes it over 900 s) and the three villages (`fh_v1` … `fh_v3`), and
`fh_20` plays from the title through the story, the map and the first three
stages. `fh_18` checks that all 13 layouts are sound.

## Sources

- [W] UFO 50 Wiki (Miraheze), "Rock On! Island" (raw page): the waves and
  the display, the cave, build and battle phases, meat, chickens and
  campfires, Zola's upgrades, every caveman's numbers, every enemy's and
  boss's numbers, the hidden villages, the goals and stats.
  https://ufo50.miraheze.org/wiki/Rock_On!_Island
- [TERM] Miraheze, "Terminal": speak to the baby in the Village of Peace.
  https://ufo50.miraheze.org/wiki/Terminal
- [MM], [MM-30] Steam guide "The missing manuals - How to play UFO 50
  games": the d-pad, B held to throw, A for menus; at the cave upgrade or
  quit, facing anything else start the next wave or quit; selling at a
  loss; saving after each level.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [GG] Steam guide "Gift, Gold & Cherry": beat The Oasis and enter the
  Dinosaur Camp; beat the game; perfect on all stages.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [GC] Steam guide "Rock On! Island All Levels Perfect" and its comments.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335176805
- [CHERRY] Steam thread "Rock On! Island Cherry achievement condition": the
  cherry is only checked in the end-game cutscene; the best record counts.
  https://steamcommunity.com/app/1147860/discussions/0/4852155320349606043/
- [BUG] Steam thread "#30 Rock On! Island Cherry".
  https://steamcommunity.com/app/1147860/discussions/1/4849903793439219803/
- [ST-20] Steam thread "Rock On! Island last level: How many waves?": 20.
  https://steamcommunity.com/app/1147860/discussions/0/4700161534027207242/
- [ST-TIPS], [TIPS-C] Steam thread "Any tips for Rock On! Island?" and its
  comments: 30 hearts on every level, the in-town tutorial, the demo, only
  the first level and Underbrush cleared, rollers down a lane.
  https://steamcommunity.com/app/1147860/discussions/0/4852155152091897814/
- [HARD-C] Steam thread "Rock On difficulty is crazy", comments: meat
  accumulating after a round with a window to spend it, the chicken window,
  holding attack diagonally, levels with 3+ spawns.
  https://steamcommunity.com/app/1147860/discussions/0/595145468747494509/
- [FIRE], [FIRE-C] Steam thread "Do fire bonuses stack in Rock on! Island?"
  and comments: stacking, the Village of Peace as a test bed, units missing,
  the wheel's reach. https://steamcommunity.com/app/1147860/discussions/0/4849904631717049045/
- [SR], [SR-RUNS] speedrun.com, UFO 50 level "30 - Rock On! Island": the ten
  stages and three villages, timed to "You Win!", and the record times.
  https://www.speedrun.com/api/v1/levels/w6qnx8nd/variables and
  https://www.speedrun.com/api/v1/runs?level=w6qnx8nd
- [WR] YouTube chapter lists of two cherry runs: the stage order, the last
  stage about 19.5 min. https://www.youtube.com/watch?v=gkBggtdIkVY
- [OASIS] YouTube description, "The Oasis Perfected": mosquitoes sprung late
  on a diagonal from the jungle spawn. https://www.youtube.com/watch?v=X9gECMbRxS4
- [TO] YouTube description, Terror Overhead: how slow the hero moves.
  https://www.youtube.com/watch?v=jjZy0fKejMc
- [LZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 30": ten levels, the
  Four Emperors, getting hit and respawning after a few seconds, the good
  ending for the cherry, no music in the planning stage.
  https://lizstar64.github.io/reviews/2024/10/16/UFO50-30.html
- [PC] Popcar's Blog, "Reviewing Every Single UFO 50 Game": meat over 99 is
  wasted; the hero is fairly slow.
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [TVT] TV Tropes recap: mammoths become normal enemies later, contact with
  a dinosaur kills, two endings, the Four Emperors attack together in the
  last wave, mosquitoes hit the hero on their way.
  https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game30RockOnIsland
- [YMMV] TV Tropes YMMV: the few seconds after a wave to buy chickens and
  cook them at once; sabertooths too quick to hit.
  https://tvtropes.org/pmwiki/pmwiki.php/YMMV/UFO50Game30RockOnIsland
- [YT-T] YouTube video titles seen in search results: the order players
  took the stages in.
