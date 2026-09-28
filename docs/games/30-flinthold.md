# 30 · FLINTHOLD

*Internal design document. Not shown in the product.*

## Tribute to

**Rock On! Island** (UFO 50 game #30, Mossmouth, "April 1987"). The rules
were researched from text only: the community wiki's raw page, Steam guides
and threads, written reviews and speedrun.com's level list (listed under
Sources; the full notes are in the research folder, `30-rock-on-island.md`).
No UFO 50 images, video, sprites, music, stage layouts or text were used as
references, and nothing was taken from a UFO 50 install.

**Map type: fixed.** Rock On! Island's stages and waves are hand-made and
"predetermined and predictable" [W], so ours are too (`flinthold_levels.c`,
all our own layouts and waves). Only the structure follows the original:

| Full scale | Rock On! Island | FLINTHOLD |
|---|---|---|
| Stages | 10 [LZ], [GC], [SR] | 10, in the same kinds of places (a first stage, a spiral, underbrush, a wasteland, a crossroads, an oasis, a jungle, a stage of fliers, a maze, the lords' last stand) |
| Villages | the Village of Peace, the Dinosaur Camp (the second village, next to The Oasis) and the Remote Island (right of Maze of Death); two of them hidden until you walk to them [W], [GG], [TVT-S], [SR] | Hearthome, the Scale Camp (hidden, off Palm Spring) and Far Isle (hidden, off The Tangle) |
| Waves | a set number per stage; the last stage has 20 [W], [ST-20] | 8, 8, 8, 9, 9, 10, 10, 10, 14 and 20 (106 in all), 3 or 4 in a village (11) |
| Spawn points | one or more per stage [W] | 1 to 3 |
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
6 gnats over a pond; 7 a jungle rush of fangcats and gnats, and Lord Jaw;
8 gliders, and Lord Gale; 9 a 53-tile maze over 14 waves, and Lord Plate;
10 three roads, 20 waves and all Four Lords at once.

## Mechanics checklist

| Mechanic | How FLINTHOLD does it | Source | Test |
|---|---|---|---|
| Waves on roads | beasts come from each spawn point down a fixed road to the cave; each stage's waves are always the same | [W], [MM] | fh_03, fh_18 |
| Next wave shown | the HUD shows which kinds come next, not in what order | [W] | (drawn) |
| Build phase | lasts as long as you like; A at the cave sounds the horn and the wave comes | [W] | fh_03 |
| Building in a wave | hunters, hens, fire pits, digging, upgrades and selling all work during a wave | [W] | fh_03 |
| The cave | 30 hearts; each beast that reaches it takes its own number; at 0 the stage is lost (try again or leave) | [W], [YT-T] | fh_12 |
| Boss wave | every stage's last wave brings a boss | [W] | fh_18 |
| Meat | earned at once for each kill, and 20 for every spawn point after a wave; 99 at most, the rest lost | [W], [PC] | fh_03, fh_04 |
| Hens | cost 10, sell 10; 5 meat after each wave while not cooked | [W] | fh_05 |
| Cooking | one fire pit beside a hen cooks it over two waves, two pits in one; a cooked hen gives no more meat but sells for 30 | [W] | fh_05 |
| Fire pits and hunters | each pit beside a hunter adds 50 % damage, up to +200 %; slows last 50 % longer per pit | [W] | fh_06 |
| Where to build | not on the road, not next to it, not on boulders or bushes | [W] | fh_02 |
| Digging | bushes and boulders cost meat to dig out, then the ground is free | [W] | fh_13 |
| Pim walks | freely, eight ways; not through thickets | [W], [MM] | fh_01 |
| Pim throws | hold B: she throws the way she faces again and again, and can't walk or turn while she does | [W], [MM] | fh_01 |
| A | works on the tile Pim faces: build, dig, upgrade, sell, talk; at the cave the horn and her upgrades | [MM] | fh_02, fh_10 |
| Her arm | 2, 3 and 6 throws a second, reaching 2, 3 and 4 tiles; the second and third cost 30 and 50 | [W] | fh_10 |
| Her weapon | bones 10, stone axe 20 (30 meat), fire axe 30 through armour (50 meat) | [W] | fh_10 |
| Only between waves | her upgrades are sold only in the build phase | [W] | fh_03 |
| Pim knocked down | a beast that runs into her stuns her; she comes back at the cave a moment later and the cave loses a heart | [W], [LZ] | fh_11 |
| Thrower | 10 a bone, every 5/6 s, 2 tiles | [W] | fh_06 |
| Spear / Barb / Bow | 30; 50 and slows; 100 every 2 s at 6 tiles, flies over thickets and carries on into the next beast | [W] | fh_07, fh_08, fh_09 |
| Sling / Hurler / Boulder | 8 twice as often at 3 tiles; 10 four times a second at 4 tiles; a rolling stone that crushes every walker it passes, every frame, and never touches a flier | [W], [ST-TIPS] | fh_09 |
| Torch / Blaze / Pitch | 20 fire through armour; 5 fire twelve times a second at half speed; 10 twice a second that slows and makes fire hurt 20 % more, once per beast | [W] | fh_08 |
| Selling | a hunter sells for half of what went into it | [MM] | fh_07 |
| The beasts | nipper 50, redback 80 (10 % armour), fangcat 40 (twice as fast), clubtail 120 (80 % armour), glider 80 (60 % armour, 30 % against fire, none against arrows; flies straight, half speed), gnat 60 (flies; goes for Pim when she is near) | [W] | fh_08, fh_11 |
| The bosses | shagtusk 2000, Lord Jaw 3000 (30 to the cave), Lord Plate 4000 (all armour, none against fire), Lord Gale 2000 (flies), Lord Snap 2000 (quick), with the original's armour, speed, meat and cave damage | [W] | fh_12, fh_18 |
| The Four Lords together | in the last wave of the last stage | [TVT-S] | fh_18 |
| The map | an island of stops; roads lead on from cleared stages, stages can be played again | [W], [YT-T], [BUG] | fh_14 |
| Hidden villages | two villages don't show until you walk to them from the stage beside them | [W], [TVT-S] | fh_14 |
| Villages | a short defence with the villagers standing about to talk to | [SR], [FIRE], [TVT-S], [LZ], [TERM] | fh_17, fh_v1-3 |
| Saving | after every cleared stage; each stage's best result is kept | [MM], [BUG] | fh_15 |
| Goals | Beacon: reach the second village; Saucer: clear the last stage; Alien: clear every stage without the cave being hurt | [W], [GG] | fh_14, fh_15, fh_16 |
| The better ending | clearing the last stage with every stage perfect | [LZ] | fh_15 |
| Stats | cave damage, most hunters, most hens, most spent on upgrades | [W] | fh_20 |
| No music while building | the tunes start with the horn | [LZ] | (audio) |

### Readings we had to choose

- **The cave's hearts:** 30 on every stage, from "All Levels Perfect 30
  Hearts" (a video title) and the wiki's cave damage of up to 30. So Lord
  Jaw reaching a cave ends the stage.
- **Costs no source gives:** a thrower 10; spear, sling and torch 20; the
  second rung 40; a fire pit 10; digging a bush 5 and a boulder 10; a fire
  pit sells for 5, a half-cooked hen for 10.
- **Speeds:** a walker goes about 1.2 tiles a second, and the others by the
  wiki's multipliers; Pim walks a pixel a frame. Beasts in a group leave a
  second apart (fangcats 0.6 s, bosses 2.5 s), groups 1.5 s apart.
- **Reach** is measured from the middle of the hunter's tile to the beast,
  plus half a tile. Shots home on their mark; every shot but an arrow is
  stopped by thickets, bushes, boulders and huts.
- **The boulder hunter** aims at the beast furthest along within 3 tiles and
  rolls its stone that way; the stone rolls on for 3 tiles once it reaches
  the road, crushing walkers for 5 a frame. We read "rolls over three tiles"
  and the tip about "throwing down a lane at oncoming enemies" this way.
- **Pitch** hurts as a plain hit (armour counts), lasts 2 s (longer by fire
  pits) and never lands twice on one beast ("an enemy cannot be tarred
  twice"). A barb slows for 1 s. Slowed beasts go half speed.
- **The fire axe** ignores all armour; its bones and axe are plain hits.
- **Pim:** knocked down for 2/3 s, then gone for 5/6 s before she comes
  back beside the cave. Beasts touch her within 10 pixels (16 for bosses);
  gliders fly over her; a gnat goes for her within 3 tiles. A fresh press
  of a new direction turns her on the spot, so she can face the tile next
  to her.
- **Starting meat:** 30 on every stage.
- **The map:** our own island and roads. The order evidence (Underbrush then
  Crossroads; Jungle Rush before Wasteland) says the map branches, so after
  Fernbrake it splits into two roads that meet again at Sky Scare.
- **Villages:** speedrun.com lists all three villages as levels timed to a
  "You Win!", and a player tested fire pits "in Village of Peace", so each
  village is a short defence (3 or 4 waves) with talking villagers. Village
  results don't count towards the goals.
- **Which lord leads which stage** and every wave's contents are ours; only
  "the final wave brings bosses" and "all four at the end" are the
  original's.
- **The Alien** is checked whenever a stage is cleared (the original checks
  it at the last stage, and a player found it didn't update otherwise; we
  read that as a bug, not a rule).

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
  who miss the fifth.
- **Music:** "Flinthold" (title), "The Island" (map), "Hearth Song"
  (villages), "Horns at Dawn", "Ash and Fern", "Vines and Wings" (battles),
  "The Four Lords", "Embers Home" (ending) and three jingles.
- **Words:** the story, the villagers' lines, the endings and every label.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu (with RESTART
STAGE and BACK TO THE MAP under RESUME) and the three UFO 40 goals, which
are Rock On! Island's own (gift, gold, cherry). The terminal code that opens
every stage and the debug keys are UFO 50 collection features; UFO 40 has
no terminal, so they are left out.

## Controls

| Input | Action |
|---|---|
| D-pad | walk (a fresh press of a new way turns Pim first) |
| Hold B | throw the way Pim faces |
| A | on the faced tile: build, dig, upgrade, sell, talk; at the cave: sound the horn, buy her arm and weapon |
| A / B in a menu | choose / close; UP and DOWN move |
| START | pause |
| Map | d-pad walks the roads, A plays the stage, B to the title |

## Not confirmed

- Every cost the wiki doesn't give (hunters, fire pits, digging) and the
  cave's 30 hearts.
- Every speed, gap and time above, and how reach is measured.
- How the boulder's stone rolls, what pitch's own hit is, and whether the
  fire axe ignores all armour.
- What a village holds beyond its folk (we made each a short defence).
- The map's shape and which stage leads where.

## Tests

`tests/fh_01` … `fh_20` drive the rules with button presses, or set up a
moment with cheats and then play it. The demo player (`fh_bot_buttons` in
`flinthold_logic.c`) follows a build plan for each stage: it walks to each
tile, faces it, opens the menu with A and picks the line, farms hens
between fire pits (selling each cooked hen and putting a new one down),
buys Pim's upgrades, sounds the horn and throws from its post. With real
presses it wins all ten stages (`fh_s01` … `fh_s10`) and the three villages
(`fh_v1` … `fh_v3`), and `fh_20` plays from the title through the story,
the map and the first three stages. `fh_18` checks that all 13 layouts are
sound.

## Sources

- [W] UFO 50 Wiki (Miraheze), "Rock On! Island" (raw page): the waves and
  the display, the cave, build and battle phases, meat, chickens and
  campfires, Zola's upgrades, every caveman's numbers, every enemy's and
  boss's numbers, the hidden villages, the goals and stats.
  https://ufo50.miraheze.org/wiki/Rock_On!_Island
- [TERM] Miraheze, "Terminal": speak to the baby in the Village of Peace.
  https://ufo50.miraheze.org/wiki/Terminal
- [MM] Steam guide "The missing manuals - How to play UFO 50 games": the
  d-pad, B held to throw, A for menus, selling, saving after each level.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [GG] Steam guide "Gift, Gold & Cherry": beat The Oasis and enter the
  Dinosaur Camp; beat the game; perfect on all stages.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [GC] Steam guide "Rock On! Island All Levels Perfect" and its comments: ten
  perfect levels, villages.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335176805
- [BUG] Steam thread "#30 Rock On! Island Cherry": the cherry checked at the
  last level. https://steamcommunity.com/app/1147860/discussions/1/4849903793439219803/
- [ST-20] Steam thread "Rock On! Island last level: How many waves?": 20.
  https://steamcommunity.com/app/1147860/discussions/0/4700161534027207242/
- [ST-TIPS] Steam thread "Any tips for Rock On! Island?": chicken farms,
  rollers down a lane, archers over terrain.
  https://steamcommunity.com/app/1147860/discussions/0/4852155152091897814/
- [FIRE] Steam thread "Do fire bonuses stack in Rock on! Island?": stacking,
  tested in the Village of Peace.
  https://steamcommunity.com/app/1147860/discussions/0/4849904631717049045/
- [SR] speedrun.com, UFO 50 level "30 - Rock On! Island": the ten stages and
  three villages by name, timed to "You Win!".
  https://www.speedrun.com/api/v1/levels/w6qnx8nd/variables
- [LZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 30": ten levels, the
  Four Emperors, Zola's axe, getting hit, the good ending for the cherry, no
  music in the planning stage, a side area's talking animals.
  https://lizstar64.github.io/reviews/2024/10/16/UFO50-30.html
- [PC] Popcar's Blog, "Reviewing Every Single UFO 50 Game": meat over 99 is
  wasted. https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [TVT-S] TV Tropes recap (blocked; a search engine's summaries only): the
  Four Emperors attack together in the last wave; the Remote Island right of
  Maze of Death and its four friendly mammoths.
  https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game30RockOnIsland
- [YT-T] YouTube video titles seen in search results only (no video
  watched): "30 Hearts", the order players took the stages in.
