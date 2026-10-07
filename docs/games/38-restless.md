# 38 · RESTLESS

*Internal design document. Not shown in the product.*

## Tribute to

**Rakshasa** (UFO 50 game #38, Mossmouth, "April 1988"). The rules were
researched from text only: the community wiki's raw page, the Steam guides
"The missing manuals" (section 38), "Rakshasa Guide" and "Gift, Gold &
Cherry", Steam threads, written reviews and TV Tropes (listed under
Sources; the full notes are in the research folder, `38-rakshasa.md`). No
UFO 50 images, video, sprites, music, stage layouts or text were used as
references, and nothing was taken from a UFO 50 install.

**Map type: fixed.** Rakshasa's stages, torches, secrets and bosses are
hand-made and the same every run; only the rank-and-file spawning, some
torch drops and the colour of each Fool are rolled [W], [GUIDE], [HB]. Ours
are fixed too (`rsl_levels.c`, all our own screens), with the same things
rolled. Only the structure follows the original:

| Full scale | Rakshasa | RESTLESS |
|---|---|---|
| Players | 1 [W] | 1 |
| Stages | 3, each in two halves with their own music [GUIDE], [STEAM-3LV] | 3, each in two halves: the Greenwood Gate and the Old Shrines; the Toad Warrens and the Sunken Court; the High Falls and the Hollow Crown |
| Length | a clear takes 10-20 minutes [HB] | 11, 11, 11, 10, 11 and 10 screens of 20 x 12 tiles (the falls climb to a second row of screens); a clean half takes the demo player about 70 s of walking |
| Fights | 6 bosses: a mid-boss at the end of each first half, a boss at the end of each second [W], [GUIDE] | the same 6: the Bridgekeeper, Old Rattle, Three-Tongue, Gust and Gash, the Rammer, the Hollow King |
| Weapons | 4: the default, homing, fire, spread [W], [TVT] | 4: the staff, the seeker ring, the ember, the scatter |
| Foes | 19 kinds with hit points [W] | the same 19 with the same hit points |
| Lives | none: every death is a soul round; 1, 3, 6, 9, 12 pieces [MM], [W] | the same |
| Clock | 150 s [TVT] | 150 s for each half |
| Items | treasure, clock, lotus, talisman, egg (familiar), bell, weapon wheels [W], [TVT] | treasure (pot, urn, gold jar, coin, gold hand, gold beetle), hourglass, moon lily, storm charm, egg (Blink the owlet), bell (Grandmother Ash), wheels |
| Hidden spots | 7, 8 and 7 rewards a stage, eggs in torches included [GUIDE] | 6 + 2 egg torches, 7 + 2, 5 + 2 and the hoard |
| Bells | in 1-2, 2-2, 3-1 (twice) [W]; a guaranteed one in the last pit [GUIDE], [TVT] | the same places, and the hoard |
| Pits | a few; the last one a room of six torches [TVT] | seven (two, two and three), the last one the hoard |
| Goals | 3 [W], [GGC] | the same 3 |
| Stats | Most Deaths; a high-score table (the only non-arcade one) [W] | the same: five scores with initials, and the most deaths |

What each stage brings: 1 a jungle of temple ruins, the first weapon torch
early, a familiar right after it, hives, wisps, tumblers, two lifts and a
gap under the second hive; 2 caves and ruins, toad holes, falling rocks,
spear traps with floor buttons, weepers, a familiar in the toads' cave and
another right after the first fight; 3 waterfalls with log bridges and
leaping fish, shellcrabs, gloomcrows, fire-breathing faces, a climb up
beside the falls, shades, shieldlings, firemaws, a hulk with a familiar
before it, two gloomcrows waiting together, and the hoard.

## Mechanics checklist

| Mechanic | How RESTLESS does it | Source | Test |
|---|---|---|---|
| Walking | one pace, 7/8 px a frame, no run, no momentum; slower than most foes | [W], [MM], [HB] | rsl_02 |
| The jump | one fixed jump (40 px; a tap and a held button the same), no steering in the air; the pad only turns him | [MM], [W] | rsl_02 |
| Landing | frozen 14 frames on every landing | [MM], [POP] | rsl_02 |
| Crouch | DOWN crouches, ducking shots at head height; DOWN + jump drops through a plank | [MM], [W] | rsl_03 |
| Aiming | ahead, up the slant with UP, down the slant with DOWN in the air only; never straight up or down | [MM], [W], [LIZ] | rsl_04 |
| Charging | a tap shoots at once; hold and let go for the charged shot | [MM], [W] | rsl_05 |
| Staff | 8 a shot, short reach; charged faster and farther | [W], [GUIDE] | rsl_04, rsl_05 |
| Seeker ring | 6, turns toward foes; only two in the air track all the way; charged faster, farther, splits in two on a hit | [W], [TVT], [GUIDE] | rsl_06 |
| Ember | 10, farther; charged, two fireballs in a helix | [W], [TVT] | rsl_06 |
| Scatter | five pellets of 3, shortest reach; charged 4 each, hanging a moment and stopping foe shots | [W], [TVT] | rsl_06 |
| One hit | any touch of a foe or a foe's shot kills | [MM], [W] | rsl_03, rsl_08 |
| The opening | the run starts with Gaunt dead: the first soul round (1 piece, 1 guardian) | [TVT], [MM] | rsl_01 |
| The soul round | 1, 3, 6, 9, then 12 pieces, a guardian each; the wisp flies eight ways and can't attack; each piece taken keeps it safe a moment; a guardian's touch is game over; blue flames join from four deaths | [W], [MM], [LIZ] | rsl_07 |
| Coming back | on the spot, safe for a few seconds, with the plain staff; everything on screen (foes, a bell, a wheel) gone; with 20 s or less, the clock goes back to 20 | [W], [TVT], [HB] | rsl_08 |
| Pits | not deaths: a closed room of foes; all beaten (or a death and a return) and he is back at the start of the section | [TVT], [W] | rsl_09 |
| The last pit | no foes: six torches, a bell, a clock, a way out | [TVT], [GUIDE] | rsl_09 |
| The clock | 150 s a half; an hourglass +30; at nothing, clockfires (5 HP) keep coming from the edges | [TVT], [W], [MM] | rsl_10 |
| Time bonus | seconds left, to the nearest 5, times 50 | [W] | rsl_10 |
| Deaths make it harder | lackeys come more often and more at once; greens 1 in 8, 1 in 3 from 4 deaths; frenzied gnats from 3; torches spit fire from 4; the blue Bridgekeeper from 4; lackeys join Three-Tongue from 2; gloomcrows' three volleys from 5; wisps that don't come otherwise from 3; the clock at double speed from 6 | [W], [GUIDE], [TVT] | rsl_10, rsl_11, rsl_18 |
| ... and richer | treasure from torches and foes is likelier and worth more with each death | [W], [POP], [TVT] | rsl_11 (drops), play |
| Treasure | pot 100, urn 200, gold jar 500, coin 100, gold hand 1,000, gold beetle 1,000 | [W], [GUIDE] | rsl_12 |
| Moon lily | every foe on screen gone, no drops | [W] | rsl_12 |
| Storm charm | calls a stormcloud (42) that throws lightning; beaten, a gold hand | [W], [GUIDE] | rsl_12 |
| The owlet | from an egg: follows, takes one hit (a white flag, then gone), flies to hidden spots near by; a second egg is points | [W], [TVT] | rsl_12, rsl_15 |
| The bell | Grandmother Ash rises from the bottom of the screen; touched, one death fewer (none: 1,000) | [W], [TVT] | rsl_12 |
| Weapon wheels | a torch lets out three weapons; take one; the one held is a gold jar instead | [TVT], [W] | rsl_13 |
| Torches | lit by a shot: treasure, an hourglass, a lily, an egg or a wheel; fixed weapon and egg torches | [MM], [GUIDE] | rsl_13, rsl_20 |
| 5,000 gifts | every 5,000 points the next foe beaten drops a gift, fixed by the foe, the mark and the deaths | [GUIDE] | rsl_14 |
| Hidden spots | touched, they give coins, jars, urns, hands, beetles, eggs or bells | [MM], [GUIDE] | rsl_15 |
| The foes | lackey 6 (red blade, green spears), spitbloom 42 (three-shot volleys), starspitter 20 (stars that rain down), hive 24 (gnats), gnat 1, wisp 6 (fours on a wave; all four: a gold jar), tumbler 6 (drops in, can't be hit or hurt as it lands, rolls and hops), bog toad 10 (out of holes, a tongue), boulderkin 24 (bounds along), shellcrab 1 (stunned, gets up again), leaper 1 (out of the water), weeper 20 (only with its eye open; tears), shade 6 (quick, jumps up, drops through), gloomcrow 42 (volleys, pushed back by hits), firemaw 42 (shootable fireballs), shieldling 1 (shield in front), hulk 80 (walks, blocks), clockfire 5, stormcloud 42 | [W], [GUIDE], [TVT] | rsl_16 |
| Traps | floor buttons fire the spear launchers on their screen (foes set them off too, and die on them); rocks that drop; fire-breathing faces; lifts | [GUIDE], [W] | rsl_17 |
| Bridgekeeper | 80; jumps back and forth by set distances, crouching first; blue at 4+ deaths: 120 and bombs, beaten: 12 gold beetles scattered right | [W], [GUIDE] | rsl_18 |
| Old Rattle | 150; runs along the ledges, changes height at three fixed columns one or two ledges at a time, drops shootable bombs | [W], [GUIDE] | rsl_18 |
| Three-Tongue | three heads of 60; fire downward that leaves the floor burning; lackeys join from 2 deaths | [W], [GUIDE] | rsl_18 |
| Gust and Gash | 180 and 220; Gaunt starts between them; Gust hovers, swoops (crouch under it) and drops down after; Gash stalks, leaps and thrusts | [W], [GUIDE] | rsl_18 |
| The Rammer | 150; dashes at where he stood, bounces off the edges and resets; enough harm in one dash turns it back | [W], [GUIDE] | rsl_18 |
| The Hollow King | 300; minions from his mouth, a third eye that opens for volleys (duck them), a skull that crosses dropping tears; only the eyes take hits | [W], [GUIDE] | rsl_18 |
| Goals | Beacon: come back from the dead three times in one run; Saucer: lay the Hollow King to rest; Alien: do it with 50,000 or more | [W], [GGC] | rsl_19 |
| No saving | a run is never saved; the high-score table with initials, the most deaths, runs and wins are | [W], [STEAM-SAVES] | rsl_19 |
| Music | sparse: birdsong over the first half, a tune for each half, one for the first fights and one for the last, a stage-clear jingle, the hoard's tune | [HB], [GUIDE] | rsl_21 |
| Playable start to end | the demo player, with real button presses from the title, lays the King to rest with more than 50,000 | | rsl_22, rsl_23 |

### Readings we had to choose

- **The clock** runs 150 s for each half (sources say "each stage"; the
  game's halves are what the wiki calls stages 1-1, 1-2 ...), and the time
  bonus is paid at the end of each half.
- **Numbers no source gives:** walking 7/8 px a frame; the jump 40 px high
  and about 35 px long; the landing freeze 14 frames; a charge 40 frames;
  shot speeds and reaches (staff 4 px a frame for 18 frames, charged 6 for
  24; seeker 3, charged 4.5; ember 5 for 34, charged 5.5 for 40; scatter
  3.5 for 16 and a 60-frame hover); three staff or ember shots at a time.
- **The soul round:** the whole screen is the arena; the wisp moves 1.5 px
  a frame; guardians circle their piece 20 px out and lunge at a wisp that
  comes within 38-54 px (faster and more often with deaths); 40 safe frames
  after each piece, 60 at the start; no time limit.
- **Revival** keeps 150 safe frames; the spot is the last ground he stood
  on.
- **The wheel:** the three weapons circle the torch slowly; touching one
  takes it.
- **Gifts every 5,000:** a fixed table of eight (gold beetles three at a
  time, hourglass, egg, bell, jar, lily, beetles, charm) indexed by the
  foe's kind, the mark and the deaths.
- **Death scaling:** lackeys every 170 frames less 12 a death, 2 + deaths/2
  at once (5 at most); treasure tiers shift up at 2, 4 and 6 deaths; foe
  drops 6 % + 3 % a death; torches spit fire every 2.5 s; wisp flocks every
  7-16 s from 3 deaths.
- **Fairness choices of ours:** a toad can be shot as it peeks out of its
  hole but can't hurt until out; leapers jump from the water under a
  bridge, one at a time from each spot; the fire faces breathe on one shared
  clock; Three-Tongue's heads take turns; lackeys hop up a one-tile step
  toward him; a standing shot clears a one-tile step, so low foes need a
  crouched shot.
- **Pits:** the room's foes are fixed; toad holes in a pit give out after
  two. Falling where there is no room behind the pit (into a falls' pool)
  puts him back at the section start.
- **The writing** in the falls' first pit, for players with five deaths or
  more, is our own line ("NOTHING STAYS BURIED").
- **The hoard** keeps the hourglass and bell lying in it and has a way out
  to the section start.
- **Bells:** five, one more than the wiki lists, as the hoard has its own.

## What is ours

- **Name:** RESTLESS (1988, Beamdown Softworks).
- **Hero:** Old Gaunt, chief of Mossfold, with an antler crown, a white
  beard, a wine cloak and a crook; his wisp in the Low Glow; Blink the
  owlet; Grandmother Ash.
- **The foes:** the Hollow Host: lackeys, spitblooms, starspitters, hives
  and gnats, wisps, tumblers, bog toads, boulderkin, shellcrabs, leapers,
  weepers, shades, gloomcrows, firemaws, shieldlings, hulks, clockfires and
  stormclouds; the Bridgekeeper, Old Rattle, Three-Tongue, Gust and Gash,
  the Rammer and the Hollow King.
- **The world:** the Greenwood Gate, the Old Shrines, the Toad Warrens, the
  Sunken Court, the High Falls and the Hollow Crown: all 64 screens and
  six pit rooms.
- **Music:** "Restless" (title), "Mossfold Burns" (story), "Greenwood
  Gate", "The Old Shrines", "Toad Warrens", "The Sunken Court", "The High
  Falls", "The Hollow Crown", "Something in the Way" (first fights), "The
  Hollow Host" (last fights), "Hidden Hoard", "The Low Glow", "Laid to Rest"
  (ending), "Names in the Ash" (high scores) and three jingles.
- **Words:** the story, the ending, the goal lines and every label.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
UFO 40 goals, which are Rakshasa's own (gift, gold, cherry). The Terminal
code that stops the soul round from growing harder is a UFO 50 collection
feature; UFO 40 has no terminal, so it is left out.

## Controls

The owner's layout puts the main attack on **A**; Rakshasa has the attack
on B and the jump on A [MM]. The cartridge card's CONTROLS page can swap
them back.

| Input | Action |
|---|---|
| LEFT / RIGHT | walk (one pace); in the air, turn only |
| DOWN | crouch |
| A (tap) | shoot |
| A (hold, let go) | charged shot |
| UP + A | shoot up the slant |
| DOWN + A in the air | shoot down the slant |
| B | jump |
| DOWN + B | drop through a plank |
| D-pad in the Low Glow | fly the wisp eight ways |
| START | pause |

## Not confirmed

| Control or rule | Our reading |
|---|---|
| The attack button | B in the original [MM]; A here (owner's layout, swappable) |
| The wisp's moves | eight ways (the manual only says "control your spirit") |
| Taking a weapon from a wheel | touching its icon |
| Whether the opening revival counts toward "cheat death 3 times" | it does |
| The clock per half or per stage | per half |
| Every speed, time and distance in "Readings we had to choose" | as listed |

## Tests

`tests/rsl_01` … `rsl_21` drive each rule with button presses, or set up a
moment with cheats and then play it. The demo player (`rsl_bot.c`) reads
the game the way a player looks at the screen and presses real buttons: it
walks the stage by marks in the map ('!' jump, '^' jump straight up, ','
wait for the lift), and whenever trouble is near it tries a set of button
plans in a copy of the game (the real rules, run ahead 40 frames) and keeps
the one that survives and gets furthest or beats the most; in a fight it
scores harm done to the boss instead. In the Low Glow it looks ahead the
same way with the guardians' own moves. `rsl_22` plays from the title
through all three stages and six fights; `rsl_23` plays the cherry plan
(dying on purpose early for the richer world) to more than 50,000.

## Sources

- [W] UFO 50 Wiki (Miraheze), "Rakshasa" (raw page): movement, weapons and
  damage, the death table, death scaling, timer, items, familiar and bell
  spots, every foe's and boss's hit points.
  https://ufo50.miraheze.org/wiki/Rakshasa
- [MM] Steam guide "The missing manuals - How to play UFO 50 games",
  section 38: controls, the HUD, torches, starting dead, the soul round.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [GUIDE] Steam guide "Rakshasa Guide" (Llamakazi): the weapons, each
  stage's secret rewards, the 5,000-point drops and their pattern (with its
  comments), the blue Bridgekeeper's frogs, boss tips, the hidden room, the
  soundtrack's track list.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3354679204
- [GGC] Steam guide "Gift, Gold & Cherry": cheat death 3 times; beat the
  game; beat it with 50,000.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [TVT] TV Tropes recap: starting dead, pits and the last pit, the bat, the
  bell's figure, the wheel, the timer and the 20-second rule, harder with
  skulls.
  https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game38Rakshasa
- [STEAM-3LV] Steam thread "Rakshasa is only 3 levels??".
  https://steamcommunity.com/app/1147860/discussions/0/4700161192391483214/
- [STEAM-SAVES] Steam thread on which games save.
  https://steamcommunity.com/app/1147860/discussions/0/4849904176633630339/
- [LIZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 38".
  https://lizstar64.github.io/reviews/2024/10/18/UFO50-38.html
- [HB] Hunter Burton, "Rakshasa - UFO 50 Diaries": the slow walk, the
  committed jump, endless spawns, the jungle's birdsong.
  https://hunterburton.neocities.org/posts/2025-02-15-Rakshasa-UFO-50-Diaries
- [POP] Popcar's Blog, "Reviewing Every Single UFO 50 Game".
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
