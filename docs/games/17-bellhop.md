# 17 · BELLHOP

*Internal design document. Not shown in the product.*

## Tribute to

**Campanella** (UFO 50 game #17, Mossmouth, "August 1985"), the arcade game
whose bell-shaped ship became the fictional company's mascot. The rules
were researched from text only: the community wiki's raw page, the Steam
guides "The missing manuals", "Tips & Tricks for Every Game", "Gift, Gold &
Cherry" and the coffee-trigger guide (read for its element names and
comments only, not its locations), Steam threads and written reviews
(listed under Sources; the full notes are in the research folder,
`17-campanella.md`). No UFO 50 images, video, sprites, music, stage
layouts or text were used as references, and nothing was taken from a
UFO 50 install.

**Map type: fixed.** Campanella's fifty stages are hand-made and always
come in the same order [W], so ours are too (`bellhop_stages.c`), all fifty
of them our own layouts. Only the structure follows the original: five
worlds of ten, a crystal room at X-5, a boss at X-10, one hidden cup on
every regular stage, four warps (A-1, the stage it lands on, C-7 and
D-3), and stages that get tighter and busier world by world: the
Clockworks, the Sugarworks and Hush Citadel run their main line through
two-tile corridors with turns, past their moving parts (about 71–75 % of
each screen inside the walls is open air there, against 82–83 % in
Millbrook and the Orchard).

**The ship flies on CHIME CIRCUIT's flight model** (`chime_flight.c`, the
chime ship of cartridge 19, our tribute to The Big Bell Race, which shares
Campanella's physics [STATIC-19], [LIZ-35]). BELLHOP gives it its own
numbers (`BHP_TUNE`), a fuel tank, and no bounce: here a touch is a crash.
The flight model itself is unchanged and CHIME CIRCUIT's tests still pass.
The Tinkler, Ansel's chime ship from CHIME CIRCUIT, is the hero ship, drawn
by the same sprite.

| Full scale | Campanella | BELLHOP |
|---|---|---|
| Players | 1 [W], [MP] | 1 |
| Stages | 50 single screens: 5 worlds × 10 [W], [MM] | 50 single screens: 5 worlds × 10 |
| Regular stages | 40 (X-1 to X-4, X-6 to X-9) [W] | 40 |
| Bonus stages | 5, always X-5 [W] | 5 crystal rooms |
| Bosses | 5, always X-10 [W] | 5 |
| Coffee | 40, one per regular stage, each behind an unmarked one-tile trigger [W], [ST-COFFEE] | 40 cups of tea, the same |
| Warps | 4 hidden sparkles: A-1, the stage A-1 lands on ("a sparkle on that level that warps you even further ahead, so you only have to play around 3 levels before the first boss"), C-7 and D-3 [ST-WARPS] | 4: A-1 to A-6, A-6 to A-9, C-7 to C-10, D-3 to D-8 |
| Lives | 3 spare ships, +1 every 1,000 points, always [W], [TIPS] | the same |
| Score display | 5 digits [MM] | 5 digits |
| Points | circler 500; coins 100, 100, 100, 200, 500; coffee 500; crystal 200 [W] | the same |
| Goals | 3 [W], [GGC] | the same 3 |
| Stats | Total Fuel Use, Total Crashes, Highest Ship Count [W] | the same three, with the high score, the best tea count and the wins |
| Code | BEAN-DRIP: the bonus stages alone, crystals 100 [W], [CHEATS] | BREW-ROOM, the same |
| Secret | wait a minute on the first stage: a cat face [W] | wait a minute on A-1: an owl |
| Saving | none: one sitting, game over means A-1 again [ST-SAVES] | the same; only the records are kept |

**How long it takes.** The speedrun records are 8:49 (gold, with warps)
and 12:44 (cherry, every coffee, no warps) [W]. The demo pilot, which
reads the whole stage and plans 40 frames ahead on the real rules, plays
the whole game with every cup in about 14 minutes of game time
(`bhp_s6`: 52,584 frames, 30 crashes, 80,350 points; the best known score
is 69,100 [W]), about 15 seconds a regular stage, the original's expert
pace.

## Structure

| World | Theme | What it brings | Stages (X-1 … X-9) | Boss (X-10) |
|---|---|---|---|---|
| A | MILLBROOK, mill country | the flight itself; moths, mites, bubbles, blocks, a big coin, circlers, a fuel can, the millpond | FIRST FLIGHT, THE MILLRACE, BUBBLE BANKS, COIN MEADOW, (CRYSTAL ROOM), THE WEIR, BUBBLE LIFT, GRAIN STORE, MILL DAM | THE MILLWHEEL |
| B | THE ORCHARD | wasps that come for you, crawlers along ledges, bombs and cracked rock, brooks | WINDFALL, WORM ROW, BLASTING PIT, THE BROOK, (CRYSTAL ORCHARD), HIVE HOLLOW, ROOT CELLAR, THE ORCHARD WALL, BRANCH LADDER | THE CIDER PRESS |
| C | THE CLOCKWORKS | turrets, glass, levers and gates, fire wheels; crawlers and blocks to refuel on | MAINSPRING, ESCAPEMENT, THE GATEHOUSE, GLASSWORKS, (CRYSTAL WORKS), TWO SWITCHES, PENDULUM HALL, BELFRY CLOCK, OVERWOUND | THE TWIN COGS |
| D | THE SUGARWORKS | ghosts through the walls, sliding plates, syrup | SPUN SUGAR, CONVEYOR, SYRUP WELLS, JELLY ROOM, (CRYSTAL KITCHEN), TAFFY PULL, CANDY FLOSS, MOULDS, THE BOILING ROOM | THE GUMBALL MACHINE |
| E | HUSH CITADEL | drones, cannons, ink, crawlers on some stages, and everything before | OUTER WALL, SENTRY WALK, THE GREAT LOCK, INK MOAT, (CRYSTAL HALL), BELL STORE, GALLERY OF GUNS, MUFFLED HALL, HUSH STAIR | LADY HUSH |

- **Title**: START or CODE over Millbrook, with the records along the
  bottom (high score, most ships, best tea, fuel burned, crashes, wins).
  B goes back to the library.
- **The run**: the story, then World A's card, then A-1 to E-10 in order.
  Each stage starts with the ship in its bubble for as long as you like;
  touching the exit ring goes straight on to the next stage. After each
  boss a TEA BREAK card shows the world's eight cups, then the next
  world's card.
- **The end**: beating Lady Hush brings the ending (a second one if every
  cup was taken), the credits and THE END. Losing the last ship is
  GROUNDED, then the title.
- **Not kept**: a run is played in one sitting (START pauses it); the
  records are saved.

## The three goals

| UFO 40 goal | Condition | Campanella's goal [W], [GGC] |
|---|---|---|
| Beacon | "KEEP 15 SHIPS IN THE HANGAR": hold 15 spare ships at once | gift: 15 spare ships |
| Saucer | "BRING THE BELLS HOME": clear all fifty stages (Lady Hush last) | gold: all 50 stages |
| Alien | "BRING THEM HOME AFTER ALL 40 CUPS OF TEA": the same, with all 40 cups taken in that run | cherry: the win with every coffee |

The goal lines and the blurb are in our own words, not the original's.

A run begun with the BREW-ROOM code on earns none of them and leaves the
records alone.

## Mechanics checklist

| Mechanic | How BELLHOP does it | Source | Test |
|---|---|---|---|
| Side view, gravity | the ship falls unless A is held | [MM], [W] | bhp_02 |
| Thrust | hold A to push up; momentum carries a climb on after you let go | [MM], [W], [POPCAR] | bhp_02 |
| Hover | tapping A about two frames in five holds the ship near one height; steering plus a tap-and-release rhythm flies a straight line | [W] | bhp_02 |
| Steering with inertia | left and right build speed slowly and the ship drifts on when let go | [POPCAR], [STATIC], [MM] | bhp_02 |
| The slash | B: a short swipe directly to the left or right, the side last steered toward; it kills enemies, breaks blocks, glass and bubbles, and swings levers | [MM], [W] | bhp_05 |
| The slash slows the fall | slightly: while the swipe lasts gravity pulls half as hard and the fall loses 1/32 a frame; slashing again and again is no hover | [MM] ("slightly slows down your falling rate") | bhp_05 |
| One touch | any wall, floor, ceiling, enemy, shot or hazard wrecks the ship at once; no bounce, no hit points | [W], [MM] | bhp_04 |
| Ramming | moths, mites, wasps, crawlers and ghosts the ship crashes into are wrecked too, for no points and no fuel; turrets, drones and the rest stay | [W] ("Some enemies will also be destroyed by colliding with them, but this awards no points") | bhp_04, bhp_23 |
| The start bubble | the ship waits safe in its bubble with no time limit; thrust or a direction pops it | [W] | bhp_01 |
| After a crash | a spare ship waits in the bubble; enemies, blocks, glass and bombs are back, but not the cup once shown, circlers and big coins had, the gates as the levers left them, or the damage done to a boss | [W], [MM], [POPCAR], [COFFEE-G-C], [ST-HOOP] | bhp_04, bhp_07, bhp_18, bhp_11-15 |
| Fuel | a vertical-segment red bar, top left; every frame of thrust burns fuel; dry, the ship can't thrust and falls | [MM], [W] | bhp_03 |
| Fuel per stage | full at every stage and every new life | [POPCAR] | bhp_03, bhp_09 |
| Refuelling | enemies, bubbles and blocks slashed give some fuel back (not every kind: turrets and glass give none); every regular stage has something that does; fuel cans (they fill the tank) sit on two stages of each world | [MM], [W] ("some stages have fuel refills"), [TIPS], [LIZ] ("orbs") | bhp_03, bhp_05, bhp_24 |
| Score is life | 3 spare ships; one more every 1,000 points, always; a green bar under the score fills toward the next | [W], [MM], [TIPS] | bhp_06 |
| Game over | the last ship lost ends the run; no continues | [MM], [ST-SAVES] | bhp_06, bhp_20 |
| The exit | a blue circle round a flashing red cross; a touch ends the stage at once, cup or no cup | [W], [MM], [COFFEE-G] | bhp_09 |
| Coffee (tea) | one per regular stage behind an unmarked one-tile spot; the cup then appears elsewhere on the screen; 500 points; no hints | [W], [ST-COFFEE], [TIPS] | bhp_07 |
| A cup stays out | once shown, it survives a crash | [W] | bhp_07 |
| Locked out | on TWO SWITCHES, flipping both levers before finding the spot loses that cup for the visit; a crash doesn't undo it (a hard lock, as players found) | [COFFEE-G-C] | bhp_18 |
| The cups shown | nothing in the HUD gives the tea away; the TEA BREAK card after each boss tallies the world's eight by stage, and GROUNDED and the ending give the count | [ST-COFFEE] ("tallied as having zero"), [ST-COFFEEBUG] | bhp_s1-s5 |
| Circler | fly close past all four sides to light them, 500; its body is a crash until then | [W] | bhp_08 |
| Big coin | five coins for five seconds: 100, 100, 100, 200, 500 in the order taken | [W] | bhp_08 |
| Crystal rooms | X-5: one slow crystal ricochets round the room; each clear brings one more and faster; 200 each; when a round runs too long the exit opens; the walls still kill | [W] | bhp_10 |
| Enemies | each world adds its own; one slash each; they bash or shoot; crawlers (eye-snake stand-ins) in B, C and E | [MM], [STATIC], [W], [COFFEE-G] | bhp_23 |
| Stage parts | bubbles, blocks, glass, levers and gates, moving plates, fire wheels, bombs, cannons, water and other liquids | [W], [COFFEE-G] | bhp_19, bhp_18, bhp_23 |
| Boss A | THE MILLWHEEL: six buckets on a wheel, two hits each, one bucket at a time spits pellets, the wheel speeds up as buckets go | [W] (Ferris 1) | bhp_11 |
| Boss B | THE CIDER PRESS: knock the apple into the chute three times; thorns from the second phase; a sprinkler whose jet stops the apple dead in the third | [W], [ST-HOOP] (Hoop) | bhp_12 |
| Boss C | THE TWIN COGS: two wheels of lamps, only one wheel's lamps open at a time, dead lamps grey, the lamps shoot | [W] (Ferris 2) | bhp_13 |
| Boss D | THE GUMBALL MACHINE: rounds of gumballs, each one slash; slashed pieces fly and burst into shrapnel where they land (the lid too); a giant last that splits | [W] (Vegetable Pot; the lid spray in [ST-POT] is a reported bug, so it isn't copied) | bhp_14 |
| Boss E | LADY HUSH: sprays of shots and lobbed spike balls; red can't be harmed, blue slashed flies back at her (the only thing that hurts her); slashing her ship refuels you, no damage | [W] (Queen Zu) | bhp_15 |
| Warps | a sparkle for 5 s from the stage loading (the bubble and a crash don't stop the clock); taken in time it skips ahead, and those stages' cups; A-1's lands on A-6, which has the next one | [ST-WARPS] | bhp_16 |
| The secret | about a minute on A-1: an owl in the hills | [W] | bhp_17 |
| The code | BREW-ROOM: the five crystal rooms alone, crystals 100 | [W], [CHEATS] | bhp_22 |
| Goals | Beacon 15 spare ships, Saucer all 50 stages, Alien win with all 40 cups | [W], [GGC] | bhp_06, bhp_21, bhp_s6 |
| Stats | fuel burned, crashes, most ships, on the title | [W] | bhp_20 |
| No save | one sitting; START pauses | [ST-SAVES] | bhp_20 |

### Controls

| Input | Action | Confirmed? |
|---|---|---|
| ◀ ▶ | steer (with inertia) | yes [W], [MM] |
| A (hold) | thrust; tap to hover | yes [MM], [TILDES] |
| B | slash to the side last steered toward | yes [MM], [W]; one side only is backed by Campanella 2's "Mirror Slash" upgrade, which adds the other side [W-C2]; facing by the last steer is our reading of "in front of you" |
| A, ◀ or ▶ in the bubble | leave the bubble | **our reading** (sources only say you leave it by starting to fly) |
| ▲ ▼ | nothing | **our reading** (no role documented in #17; down speeds the fall only in Campanella 2) |
| START | pause | yes [CONV] |

Players found thrust and slash together tiring on the default buttons
[TILDES]; UFO 40's cartridge card lets A, B and SELECT swap jobs.

### Readings we had to choose

- **Flight numbers** (no source gives any): gravity 18/256 px a frame
  each frame, thrust 44 against it (net 0.10 up), steering 12 (14 while
  thrusting), drift loss 2, top speeds 1.5 across, 1.75 up and 2.4 down;
  an 8 × 8 hit box in a 12 × 10 sprite.
- **Fuel**: a full tank is 600 frames of thrust (10 s), drawn as 20
  segments. Back: moth or mite 120, wasp, crawler, ghost or drone 150,
  bubble 150, block 90, crystal 90, bucket or lamp 120, gumball 90, an
  apple hit 90; turrets, glass and spike balls give none.
- **Points no source gives**: block 50, glass 30, bubble 50, moth and mite
  100, wasp and turret 150, crawler, ghost and drone 200; a bucket 100 a
  hit and 300 when it breaks, a lamp 200, a gumball 100, a dunk 500, a hit
  on Lady Hush 500; a boss beaten 1,000 (Lady Hush 2,000).
- **The slash**: 12 frames, striking in the first 8, reaching 18 px past
  the ship's middle, 18 frames from one to the next; while it lasts
  gravity is halved and each frame keeps 248/256 of the fall, so a slash
  at top speed still leaves about 85 % of it ("slightly").
- **After a crash** enemies, blocks, glass and bombs come back. What
  stays: the cup once shown [W]; circlers and big coins already had (so
  they can't be farmed); the gates as the levers left them (the lock-out
  is a hard lock [COFFEE-G-C]); a finished crystal room; and the damage
  done to a boss: buckets and lamps broken, the press's phase with its
  thorns and sprinkler, the gumball round (it starts again), Lady Hush's
  hits. The boss's parts go back to their places and its shots are gone.
  Keeping a boss's damage is our reading: a player losing "a lot of lives
  when in phase 3" of the Hoop never mentions redoing phases 1 and 2
  [ST-HOOP]. A crash lasts 70 frames.
- **Crystal rooms**: round n has n crystals at 128 + 128 × (n − 1) /256 px a
  frame (at most 5 px), and 8 s; they start when the bubble pops and are
  harmless to touch.
- **Bombs**: a slash lights a 1 s fuse; the blast (26 px) breaks cracked
  rock, blocks and glass, takes enemies and lights other bombs, and wrecks
  the ship within 22 px. Touching a bomb is a crash.
- **Cannons** fire a ball every 2.5 s; **turrets** an aimed shot every
  2.5 s at a ship within 150 px; **drones** drop a shot every 2 s; **fire
  wheels** turn once every 8.5 s; **plates** slide at 0.75 px a frame.
- **Water, syrup and ink** are walls to touch.
- **The warps' destinations**: A-1 goes "about 4 levels" on to a stage
  with its own sparkle, and then "around 3 levels before the first boss"
  [ST-WARPS], so A-1 to A-6 to A-9; C-7 to C-10 and D-3 to D-8 are ours.
- **Bosses**: the Millwheel's buckets spit three-shot fans three times
  every 2.5 s and the wheel goes 1.5 times faster per bucket lost; the
  press needs one dunk a phase (the wiki's three phases "each requiring"
  one) and its
  sprinkler fires every 2.5 s; the cogs' lamps shoot every 1.3 s; the
  gumball rounds are 3, 4 and 5 and then the giant; Lady Hush takes 8
  hits, alternates red and blue spike balls and attacks faster below
  half; the apple is harmless to touch.
- **The tea record**: a cup counts for the run the moment it is taken.
- **Lives**: the counter stops at 99.
- **Between stages**: a short story before A-1, a card for each world and
  a TEA BREAK card after each boss (sources describe none of them); the
  ending's last lines change when every cup was taken.

## What is ours

- **Name**: BELLHOP (1985, Beamdown Softworks), a bell that hops.
- **Hero**: Ansel and the Tinkler, the chime ship from CHIME CIRCUIT; he
  never flies past a cup of tea.
- **Villain**: Lady Hush, who stole every bell in the valley, in her ship.
- **The worlds**: Millbrook, the Orchard, the Clockworks, the Sugarworks
  and Hush Citadel, their skies and walls; all fifty layouts and their
  names.
- **The things**: moths, mites, wasps, crawlers, turrets, ghosts and
  drones; bubbles, fuel cans, circlers, coins, fire wheels, plates,
  bombs, levers, gates and cannons in our own drawings.
- **The bosses**: the Millwheel, the Cider Press, the Twin Cogs, the
  Gumball Machine and Lady Hush.
- **Tea** instead of coffee; an owl instead of a cat; the BREW-ROOM code.
- **Music**: "Bellhop" (title), "Millbrook", "Orchard Hop", "Tick-Tock
  Works", "Sugarworks", "Hush Citadel" (the worlds), "Crystal Room", "Big
  Machinery" (bosses), "Lady Hush", "The Bells Ring" (ending), and three
  jingles: "Stage Clear", "Grounded" and "Tea Break".
- **Words**: the story, the endings, the credits and every label.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu, the three UFO
40 goals (Campanella's own gift, gold and cherry) and the cartridge card's
button swap. UFO 40 has no terminal, so its code goes on the title's CODE
screen.

## Not confirmed

- Every number under Readings.
- How the bubble is left, and up and down doing nothing (the Controls
  table).
- What water, bombs, cannons, plates and levers do in detail; the regular
  enemy roster (no source lists one).
- Where the C-7 and D-3 warps lead.
- The Queen's hit count.
- Whether a boss keeps its damage through a crash (see Readings).

## Tests

`tests/bhp_01` … `bhp_24` drive the rules with button presses, or set up a
moment with cheats and then play it. The demo pilot (`bellhop_bot.c`)
follows a route written for each stage (the hidden spot, the cup, the
levers and bombs in order, the exit) over a distance field, and every
four frames plans: it copies the stage and flies 36 candidate control
plans 40 frames ahead on the real rules, keeping the one that ends
furthest along without a crash. It slashes whatever is in reach. With real
presses it clears every world from a fresh run with all eight cups
(`bhp_s1` … `bhp_s5`), and plays the whole game from the title to the
ending with all forty cups and all three goals (`bhp_s6`). It takes all
four warps (`bhp_16`) and the lever puzzle the right way round
(`bhp_18`). `bhp_24` checks all fifty layouts.

## Sources

- [W] UFO 50 Wiki (Miraheze), "Campanella" (raw and rendered): rules,
  points table (with "Some enemies will also be destroyed by colliding
  with them, but this awards no points"), coffee, bonus stages, bosses
  ("three phases, each requiring" a dunk), fuel refills on some stages,
  the cat, records, BEAN-DRIP.
  https://ufo50.miraheze.org/wiki/Campanella
- [MP] Miraheze, "Multiplayer" (not listed). https://ufo50.miraheze.org/wiki/Multiplayer
- [CHEATS] Miraheze, "Cheats". https://ufo50.miraheze.org/wiki/Cheats
- [MM] Steam guide "The missing manuals - How to play UFO 50 games",
  Campanella section. https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [TIPS] Steam guide "Tips & Tricks for Every Game".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3340576461
- [COFFEE-G] Steam guide "Campanella Cherry - coffee trigger locations"
  (element names and comments only; no locations used).
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335876087
- [COFFEE-G-C] its comments: "you can hard-lock yourself out of the
  coffee if you flip both of the switches before you trigger the coffee
  and screw up your run"; "softlocking on C3".
- [W-C2] Miraheze, "Campanella 2": the Mirror Slash upgrade reaches both
  sides. https://ufo50.miraheze.org/wiki/Campanella_2
- [GGC] Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [ST-SAVES] Steam thread "Which games save progress?".
  https://steamcommunity.com/app/1147860/discussions/0/4849904176633630339/
- [ST-WARPS] Steam thread "Campanella Warps": the opening post (A-1's
  sparkle, "about 4 levels", a sparkle on that level too, "around 3 levels
  before the first boss") and a later post (C-7 and D-3).
  https://steamcommunity.com/app/1147860/discussions/0/4849904176633447397/
- [ST-COFFEE] Steam thread "Campanella Coffee": "I didn't even KNOW there
  were hidden coffees until I beat the first boss and got tallied as
  having zero".
  https://steamcommunity.com/app/1147860/discussions/0/4849904828211003305/
- [ST-COFFEEBUG] Steam thread on the coffee display.
  https://steamcommunity.com/app/1147860/discussions/1/4700162167909946800/
- [ST-HOOP] Steam thread "Campanella 1 - Basketball Boss".
  https://steamcommunity.com/app/1147860/discussions/0/4699034745345476321/
- [ST-POT] Steam Bug Reports threads on the Vegetable Pot (the lid that
  spews bullets is "a benign bug").
  https://steamcommunity.com/app/1147860/discussions/1/4849904631721483646/
- [TILDES] Tildes, "UFO 50 discussion topic".
  https://tildes.net/~games/1iyv/ufo_50_discussion_topic
- [STATIC], [STATIC-19] Static Canvas, "The UFO 50 Diaries": Campanella
  and The Big Bell Race. https://staticcanvas.substack.com/p/the-ufo-50-diaries-campanella
- [LIZ], [LIZ-35] Lizstar's reviews of #17 ("collect orbs, which
  increase your fuel") and #35.
  https://lizstar64.github.io/reviews/2024/10/13/UFO50-17.html
- [POPCAR] Popcar's Blog, "Reviewing Every Single UFO 50 Game".
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [CONV] the UFO 50 conventions notes, `00-ufo50-conventions.md`.
