# 28 · DUNE EXPRESS

*Internal design document. Not shown in the product.*

## Tribute to

**Rail Heist** (UFO 50 game #28, Mossmouth). The rules were researched from
text only: the community wiki, written reviews, Steam guides and forum
threads (listed under Sources). No UFO 50 images, video, sprites, music,
level maps or code were used as references. The wiki's and the guides'
train maps were **not** opened; no original train was reconstructed.

**Map type: fixed, hand-made (1P); generated (2P).** Rail Heist's twenty
missions are hand-made trains that never change, so ours are too, and every
tile of every train is our own design. We match the structure the sources
describe:

| Structure | Rail Heist | DUNE EXPRESS |
|---|---|---|
| Missions | 20, in order | 20, in order |
| The outlaws | three, one or two per mission, in the wiki's order | Wade, Hush and Pearl in the same slots |
| Objectives | loot on most; rescue (13), max ammo (15), the sheriff (19), 4 loot (20) | the same, per mission |
| New ideas by mission | patrols (4), gatling (6), switches (7), double jump (9), two outlaws (10), unmarked loot (11), a cow (12), longest train (14), exploding bullets (16) | the same missions introduce the same things |
| Time goals | one per mission, 24 s to 94 s | the same twenty numbers |
| Escape | Mister Blue at the far end of the train | Biscuit the camel, beside the last car |
| 2P | robbers vs lawmen on a longer, generated train | the same, with our generator |

## Mechanics checklist

Every line was compared with the code; the tests named prove it with button
presses.

| Mechanic | How DUNE EXPRESS does it | Source | Test |
|---|---|---|---|
| The heist | complete the objective, then reach the camel at the far end | [W], [MM] | dx_09 |
| Master timer | a limit per mission; at zero the train reaches town and the heist fails | [W], [MM] | dx_27 |
| Failure | any outlaw dying fails the heist; instant retry | [W], [SC] | dx_22 |
| Mission start | the camera pans along the whole train; any button takes control and starts the clock | [MM] | dx_21 |
| Two modes of time | real time while no lawman is active; turns once one patrols or is alert; back to real time when none is | [W], [MM], [SC] | dx_03 |
| Player turn | 10 s; the master timer runs | [W], [MM] | dx_03 |
| Lawmen turn | outlaws locked in place (a jump in the air carries on); lawmen move 3.5 s; the master timer stops | [W] | dx_03, dx_20 |
| Lawman states | guard (still, one sightline, no turn), patrol (fixed beat), alert (hunts the last sighting or noise) | [W] | dx_03, dx_19 |
| Seen | an armed lawman shoots an outlaw in his sight at once, at head or knee height | [W], [MM] | dx_02 |
| One bullet | then he is disarmed: he can only punch or throw | [W], [R] | dx_02, dx_17 |
| Friendly fire | objects block sight, other lawmen don't, and a bullet hits the first body in its way: bait one lawman into shooting another | [W], [G60] | dx_17 |
| Punching a guard | stuns him and he starts to patrol; seen, he turns alert | [W], [MM] | dx_04 |
| Gunshots | every lawman nearby turns alert | [W], [MM] | dx_06, dx_19 |
| Silent kills | a cinder block or a fall off the train alerts nobody | [W] | dx_27 |
| Stun | about 3 s, stars over the head; it counts down only in the owner's time, so a stunned lawman loses the outlaw's whole turn and wakes near the end of his own | [W] | dx_04 |
| Auto punch / auto shoot | unarmed characters who touch trade a punch (the mover hits); a drawn gun fires by itself at a lawman stepping into its sight in the lawmen turn | [W] | dx_27 |
| Moves | walk, jump (hold A for full height), climb ladders, duck, roll (↓ + A: walking speed, as long as ↓ is held, through one-tile gaps) | [W], [MM] | dx_01 |
| Hiding | ducked or rolling you are one tile tall, hidden behind anything that size | [W] | dx_02, dx_19 |
| Punch | stuns; breaks floors, walls and ceilings; down when ducking, up when jumping; a ducking punch sends people flying in an arc | [W], [MM] | dx_04 |
| Materials | planks take 3 punches, armor plate 30, cracking as they go | [MM], [R] | dx_04, dx_26 |
| Standing on lawmen | an outlaw can stand on a lawman's head, out of his sight | [GM] | dx_19 |
| Carrying | ↓ + B picks up or puts down; B throws; jump and roll while carrying; no ladders with full hands | [W], [MM] | dx_05 |
| Pushing | walk against a thing for a moment to push it along | [W] | dx_15 |
| Held cover | jump into a lawman's sight holding a box and he shoots the box | [W] | dx_17 |
| Barrels, crates | stun when thrown; cover; may hide power-ups; ↓ while holding a barrel climbs in; roll it; a barrel seen falling or rolling at him gets shot, and breaks, leaving you unharmed | [W], [R] | dx_05, dx_16 |
| Ladders | lawmen can't use a ladder with a thing on it | [W] | dx_27 |
| Cinder blocks (our anvils) | heavy: no full jump; lethal and silent when thrown (missions 3 and 14, and 2P) | [W], [R] | dx_27 |
| Dynamite (our powder sticks) | thrown: blows on a wall, or slides then blows; placed or dropped: stays put; breaks armor, not gears | [W] | dx_08 |
| Chickens (our geese) | a thrown one in a lawman's sight gets shot, wasting his bullet; they stun; carried, you fall slowly and glide | [W] | dx_08, dx_24 |
| Gatling (our crank gun) | shoots an outlaw in front of it after a green flash; punched or picked up from behind or above it turns on the lawmen; runs dry | [W], [R] | dx_07, dx_18 |
| Switches (our levers) | punched or shot, flip every gear wall; can be carried; cover | [W] | dx_07 |
| Cow (our ram) | runs people over | [W] | dx_25 |
| Kills | gunshot, cinder block, dynamite, cow, gatling, off the train, super punch | [W], [GM] | dx_22, dx_27 |
| Loot | carry the box to the camel, or break it and pick up the coins | [W], [MM] | dx_09 |
| Power-ups | extra bullets (+2; +1 in mission 1), exploding bullets (3×3 blast where the bullet stops), double jump (higher than the first), protection (one lethal hit), super punch (kills, breaks every wall and thing), quick draw (1 s) | [W], [GM] | dx_06, dx_23, dx_26, dx_27 |
| Gun | hold ↑ to load and draw (just over 2 s), B fires; limited ammo | [W], [MM] | dx_06 |
| Stars | three per mission: MERCY (Rail Heist's angel: no lawman killed), RUTHLESS (devil: every lawman killed), SWIFT (time: at or under the goal) | [W], [MM], [G60] | dx_10, dx_m01–m20 |
| Time goals | 24, 30, 49, 31, 54, 33, 36, 44, 36, 47, 31, 27, 36, 54, 40, 76, 45, 50, 53, 94 s | [G60], [GM] | dx_m01–m20 |
| The sheriff (our Governor) | killing him doesn't cost the mercy star | [W] | dx_m19 |
| Two outlaws | one moves per turn, alternating; SELECT swaps while no turns run | [W], [P] | dx_11 |
| 2P Versus | robbers vs lawmen on a longer generated train; each side moves one member a turn; 10 coins win for the outlaws, 10 turns for the lawmen | [W] | dx_13 |
| Goals | beat mission 10; beat all 20; beat the game with 40+ stars | [W], [GM] | dx_10 |

### The missions

| # | Ours | Rail Heist | Outlaws | Objective | What's new | Time goal |
|---|---|---|---|---|---|---|
| 1 | THE FIRST JOB | A Simple Heist | Wade | 1 loot | guards | 24 s |
| 2 | OVER THE ROOFS | Roof Assault | Hush | 1 loot | lawmen on the roofs | 30 s |
| 3 | FREIGHT FOR TWO | Cargo Ambush | Pearl | 2 loot | | 49 s |
| 4 | EYES OPEN | High Alert | Wade | 1 loot | patrols | 31 s |
| 5 | GOOSE CHASE | Fowl Business | Hush | 1 loot | geese | 54 s |
| 6 | THE RATTLER | Guarded By Gat | Pearl | 1 loot | the crank gun | 33 s |
| 7 | IRON GATES | Shifting Gears | Wade | 1 loot | levers and gates | 36 s |
| 8 | THE STRONGROOM | Vault Robbery | Hush | 1 loot | armor | 44 s |
| 9 | LEAP OF FAITH | Winging It | Pearl | 1 loot | the only spring boots | 36 s |
| 10 | TWO BY TWO | Daring Duo | Wade and Hush | 2 loot | two outlaws | 47 s |
| 11 | THE HAYSTACK | Root Around | Pearl | 2 unmarked loot | loot hidden in plain crates | 31 s |
| 12 | THE RAM CAR | Cow Poke | Wade | 1 loot | rams | 27 s |
| 13 | JAILBREAK | Rescue Mission | Pearl and Hush | free Hush | control passes to Hush | 36 s |
| 14 | THE LONG LINE | Long Haul | Wade | 2 loot | the longest train | 54 s |
| 15 | FULL BELT | Resupply | Hush | leave with a full belt | ammo | 40 s |
| 16 | PLATED | Armored Up | Pearl and Wade | 1 loot | the only blast rounds | 76 s |
| 17 | POWDER TRAIN | Powder Keg | Hush | 1 loot | powder everywhere | 45 s |
| 18 | EASY PICKINGS | Sitting Ducks | Pearl | 2 loot | | 50 s |
| 19 | THE GOVERNOR | Vengeance! | Wade and Hush | take down the Governor | he doesn't cost the mercy star | 53 s |
| 20 | ONE LAST RIDE | The Final Score | Pearl and Wade | 4 loot | | 94 s |

Every train is played through in `tests/dx_mNN_route.ufs` with button
presses alone, with no kill and under its time goal, so the mercy and swift
stars are in reach on every mission.

### Readings we had to choose

- **Speeds** (no source gives them): walking 1.25 px a frame, lawmen 0.9,
  climbing 1.0; a jump clears about two and a half tiles. Tuned so our routes
  beat the sourced time goals by a human margin.
- **Sight** is nine tiles; **nearby** (gunshots) is ten.
- **Throws** fly about ten tiles, a little further than a lawman sees.
- **The crank gun** fires two bursts of three.
- **Ammo:** at most 6 bullets.
- **The lawmen turn** is 3.5 s as the wiki says; one guide says 10 s.
- **Time limits** per mission are ours (180 to 420 s).
- **The 2P train:** 9 to 11 of our car layouts, 7 strongboxes of 3 coins,
  up to 4 guards and a crate or barrel or two per car.

## What is ours

- **Name:** DUNE EXPRESS (1987, Beamdown Softworks).
- **Story:** the Governor's tax trains carry the frontier towns' money
  across the salt flats. Old Hettie's band takes it back, until the Governor
  blows up their hideout.
- **Characters:** Wade, Hush, Pearl, Old Hettie, Biscuit the camel, the rail
  guards and the Governor.
- **All twenty trains**, every tile, the mission names and text.
- **Things:** oil barrels, crates, anvils, powder sticks, geese, rams, the
  crank gun, levers and iron gates, strongboxes and coins; spare rounds,
  blast rounds, spring boots, the lucky horseshoe, the iron fist, the quick
  holster.
- **Pixel art and music:** a desert railway at dusk and night, and our tunes.

## Additions: none

Only the platform needs every UFO 40 cartridge has: the START pause menu,
saving (missions beaten and stars), and the three UFO 40 goals, which are
Rail Heist's own three (beat mission 10; beat all 20; 40+ stars).

## Controls

| Input | Action |
|---|---|
| D-pad | walk, climb; ↓ ducks |
| A | jump (hold for full height) |
| ↓ + A | roll (as long as ↓ is held) |
| B | punch; throw what you hold |
| ↓ + B | pick up / put down |
| walk into a box | push it |
| holding a barrel, ↓ | climb in; ← → roll it; ↑ or A out; B out holding it |
| hold ↑ (still) | load and draw the gun; then B fires |
| SELECT | swap outlaws (two-outlaw missions, real time only) |
| START | pause menu |

## Not confirmed

- **Punch boosting** (a jump, a punch and a turn for extra distance) is
  mentioned in one Reddit thread without detail; not reproduced.
- **Plank strength** (3 punches) is our reading of "some materials take more
  hits"; the 30 for armor comes from one player's account.

## Sources

- [W] UFO 50 Wiki (Miraheze), "Rail Heist": objective, master timer, turn
  timer (10 s), lawmen turn (about 3.5 s), states, one bullet, stun and its
  timing, auto punch and auto shoot, kills, punching and rolling, every
  object and power-up, stars, the missions, 2P Versus.
  https://ufo50.miraheze.org/wiki/Rail_Heist
- [MM] Steam guide "The missing manuals - How to play UFO 50 games":
  buttons, the opening pan, punching makes a guard aware, turns until no
  lawman is aware, materials taking different numbers of hits.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [G60] Steam guide "Rail Heist - Gold, Cherry and 60 Stars Guide": the
  twenty time goals, star rules, making lawmen shoot each other.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3476548620
- [GM] Steam guide "Rail Heist - Maps and Star Requirements" (text only):
  the time goals again, power-up effects (3×3 exploding bullets), walking on
  top of a lawman.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3345935952
- [R] r/UFO50 threads (read through the Pullpush archive): one bullet per
  lawman, rolling a barrel off a ledge in front of a guard, lethal cinder
  blocks, a metal block taking 30 punches, the gatling firing more than
  three shots, punch boosting.
- [L] Lizstar's Trashcan, "UFO 50 Retrospective Part 28 - Rail Heist".
  https://lizstar64.github.io/reviews/2024/10/16/UFO50-28.html
- [P] Popcar's Blog, "Reviewing Every Single UFO 50 Game".
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [SC] Static Canvas, "The UFO 50 Diaries: Rail Heist".
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-rail-heist
- TV Tropes recap and Indie Hell Zone's review were read and added nothing
  new.
