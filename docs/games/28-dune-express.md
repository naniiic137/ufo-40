# 28 · DUNE EXPRESS

*Internal design document. Not shown in the product.*

## Tribute to

**Rail Heist** (UFO 50 game #28, Mossmouth). The rules were researched from
text only: the community wiki, written reviews and a guide (listed under
Sources). No UFO 50 images, video, sprites, music, level maps or text were
used as references. The wiki's and guides' level maps were **not** opened;
no original train was reconstructed.

**Map type: fixed, hand-made (1P); generated (2P).** Rail Heist's twenty
missions are hand-made trains that never change, so ours are too, and every
tile of every train is our own design. We match the structure the sources
describe:

| Structure | Rail Heist | DUNE EXPRESS |
|---|---|---|
| Missions | 20, in order | 20, in order |
| The outlaws | three, one or two per mission, in the wiki's order | Khaled, the Veil and Sahar in the same slots |
| Objectives | loot on most; rescue (13), max ammo (15), the sheriff (19), 4 loot (20) | the same, per mission |
| New ideas by mission | patrols (4), gatling (6), switches (7), double jump (9), two outlaws (10), unmarked loot (11), a cow (12), longest train (14), exploding bullets (16) | the same missions introduce the same things |
| Escape | Mister Blue at the far end of the train | Baraka the camel, trotting beside the last car |
| 2P | robbers vs lawmen on a longer, generated train | the same, with our generator |

## Mechanics checklist

| Mechanic | How DUNE EXPRESS does it | Source |
|---|---|---|
| The heist | complete the objective and escape to the horse (our camel) at the far end of the train | [W] |
| Master timer | a time limit per mission; at zero the train reaches town and the heist fails | [W] |
| Failure | any outlaw dying fails the heist; instant retry | [W], [SC] |
| Two modes of time | while no lawman is active it's real time; once one patrols or is alerted, turns begin | [W], [SC] "both real time AND turn based", [L] |
| Player turn | a 10-second turn timer; the master timer runs | [W] |
| Lawmen turn | the outlaws are locked in place; lawmen move for about 3.5 s; the master timer stops | [W] |
| Lawman states | guard (still, watches one sightline, no lawmen turn), patrol (walks a fixed route), alert (hunts the outlaws) | [W] |
| Seen | an armed lawman shoots an outlaw in his sight at once; one bullet, then he is disarmed and can only punch or throw | [W], [L] |
| Gunshots | every lawman nearby turns alert | [W] |
| Stunned guards | a stunned guard becomes a patrol | [W] |
| Hiding | ducking or rolling makes you one tile tall: hidden behind anything that size | [W] |
| Held cover | jump into a lawman's sight holding a box and he shoots the box (ours: whatever an outlaw holds up takes the bullet) | [W] |
| Moves | walk, jump, climb ladders, duck and roll (as fast as walking, as long as you like, through one-tile gaps) | [W] |
| Punch | stuns; breaks standard floors, walls and ceilings; down when ducking, up when jumping; ducking punches hit harder and send people flying | [W] |
| Auto punch | two unarmed characters meeting trade an automatic punch | [W] |
| Stun | about 3 s of not moving or reacting; stars over the head | [W] |
| Gun | must be loaded and drawn (just over 2 s) to shoot; limited ammo; a drawn gun fires by itself at a lawman who steps into its sight during their turn | [W], [L] |
| Carrying | pick up, throw, put down; you can jump and roll while carrying | [W] |
| Barrels, crates | stun when thrown; cover; can hide power-ups; hide inside a barrel; block ladders | [W] |
| Cinder blocks (our anvils) | heavy: no full jump; lethal when thrown | [W] |
| Dynamite | thrown: blows up on a wall, or slides then blows up; placed or dropped: stays put; breaks armored walls, not gears | [W] |
| Chickens (our geese) | a lawman shoots a thrown one; they stun; carried, you fall slowly and glide over gaps | [W] |
| Gatling | shoots an outlaw in front of it (green flash); punched or picked up from behind or above it turns on the lawmen; limited ammo | [W] |
| Switches (our levers) | punched or shot, flip every gear wall; can be carried; cover | [W] |
| Cow (our ram) | runs people over | [W] |
| Kills | silent: thrown cinder block, knocked off the train, punched out through an opening; loud: gunshot, dynamite, cow, gatling, super punch | [W] |
| Loot | carry the box to the horse, or break it and pick up the coins | [W] |
| Power-ups | extra bullets (+2, +1 in mission 1), exploding bullets, double jump, protection (one lethal hit, once a mission), super punch, quick draw (1 s) | [W] |
| Stars | angel (kill no lawmen), devil (kill every lawman), time (at or under the goal time) | [W], [L], [P] |
| Two outlaws | in two-outlaw missions you control one outlaw each turn, alternating | [P], [W] |
| 2P Versus | robbers vs lawmen on a longer train generated each time; each side controls one member a turn; outlaws win at 10 coins, lawmen by holding out 10 turns | [W] |

### The missions

| # | Ours | Rail Heist | Outlaws | Objective | What's new |
|---|---|---|---|---|---|
| 1 | THE FIRST JOB | A Simple Heist | Khaled | 1 loot | guards |
| 2 | OVER THE ROOFS | Roof Assault | the Veil | 1 loot | lawmen on the roofs |
| 3 | FREIGHT FOR TWO | Cargo Ambush | Sahar | 2 loot | |
| 4 | EYES OPEN | High Alert | Khaled | 1 loot | patrols |
| 5 | GOOSE CHASE | Fowl Business | the Veil | 1 loot | geese |
| 6 | THE RATTLER | Guarded By Gat | Sahar | 1 loot | the crank gun |
| 7 | IRON GATES | Shifting Gears | Khaled | 1 loot | levers and gates |
| 8 | THE STRONGROOM | Vault Robbery | the Veil | 1 loot | armored walls |
| 9 | LEAP OF FAITH | Winging It | Sahar | 1 loot | the only spring boots |
| 10 | TWO BY TWO | Daring Duo | Khaled and the Veil | 2 loot | two outlaws |
| 11 | THE HAYSTACK | Root Around | Sahar | 2 unmarked loot | loot hidden in plain crates |
| 12 | THE RAM CAR | Cow Poke | Khaled | 1 loot | the ram |
| 13 | JAILBREAK | Rescue Mission | the Veil and Sahar | free the Veil | control passes to the Veil |
| 14 | THE LONG LINE | Long Haul | Khaled | 2 loot | the longest train |
| 15 | RESUPPLY | Resupply | the Veil | leave with a full belt | ammo |
| 16 | PLATED | Armored Up | Sahar and Khaled | 1 loot | the only blast rounds |
| 17 | POWDER TRAIN | Powder Keg | the Veil | 1 loot | dynamite everywhere |
| 18 | SITTING DUCKS | Sitting Ducks | Sahar | 2 loot | |
| 19 | THE GOVERNOR | Vengeance! | Khaled and the Veil | take down the Governor | he doesn't count against the angel star |
| 20 | THE LAST SCORE | The Final Score | Sahar and Khaled | 4 loot | |

Every one of the twenty trains is played through in the test suite
(`tests/dx_mNN_route.ufs`) with button presses alone, without a kill and under
its time star, so every mission is known to be winnable with the angel and
time stars in reach (the devil star asks for every lawman down instead).

### The 2P train generator

The sources say only that the 2P train is longer than any 1P train and
generated fresh each time. Ours strings together 9 to 11 cars chosen at
random from a set of our car layouts (passenger, freight, flat, tank,
two-deck), deals 7 strongboxes (3 coins each) across them, places 4 guards
and a crate or barrel or two per car, and starts the outlaws on the first car
and the camel at the last.

### Readings we had to choose

- **Buttons.** A jumps. B punches; ducking + B picks up; B throws what you
  hold; ↓ + B puts it down (a barrel: climbs inside it). Holding ↑
  (standing still) loads and draws the gun; B then fires; moving holsters it.
  SELECT swaps outlaws in two-outlaw missions while no turns are running.
- **Sizes.** Tiles are 16 pixels; people are two tiles tall, one ducking. A
  jump clears about two and a half tiles.
- **Sight** runs straight ahead from a lawman at both of his heights (head and
  knees), up to nine tiles, until a wall, floor or anything solid; a box
  someone holds blocks it too. The Governor sees the same way.
- **Held cover:** a lawman who sees an outlaw holding something shoots the
  thing instead (a box or strongbox bursts, a goose falls, a powder stick goes
  off in your hands). A box held up out of sight only blocks his view.
- **Nearby**, for gunshots and explosions, means within ten tiles.
- **Throws** leave from the chest in a long low arc of about ten tiles, a
  little further than a lawman sees, so a crate thrown from the dark can stun
  him. Barrels, crates and strongboxes burst on whoever they hit.
- **Things stand where they stand:** walking into a box doesn't push it; jump
  it outside, or lift or break it. Nobody climbs a ladder with full hands.
- **Doors:** an open car end is open from floor to roof.
- **Explosions** blow out walls, roofs and plate about a tile and a half
  around, but not the floor, and kill anyone within about two tiles. Blast
  rounds break the tile they hit and the one below it.
- **Stun** only counts down while its owner could otherwise act (a lawman's
  during lawmen turns or real time), which is why stunned lawmen lose the
  player's whole turn and wake near the end of theirs.
- **Alert lawmen** walk toward the nearest outlaw they can reach, climbing
  ladders; unarmed ones pick up anything throwable next to them and throw it
  when the outlaw is on their row.
- **Auto punch:** when an unarmed lawman and an outlaw touch, the one who is
  moving punches; the other is stunned and pushed back.
- **The ram** stands still until punched, shot at or it hears a gunshot, then
  charges the way it faces until it hits a wall, trampling anyone in its way.
- **Ammo:** every outlaw carries at most 6 bullets. Mission 15 needs all 6.
- **Mission 13:** once the Veil's cell opens, Sahar slips away to the camel
  and you play the Veil.
- **The master timer and time star goals** are ours, one pair per mission.
- **Fog:** the inside of a car stays dark until an outlaw enters it.

## What is ours

- **Name:** DUNE EXPRESS (1987, Beamdown Softworks).
- **Story:** the Governor's tax trains carry the oasis villages' money across
  the salt flats. Old Zohra's band of desert outlaws takes it back, until the
  Governor blows up their hideout. Everything that follows is ours.
- **Characters:** Khaled, the Veil, Sahar, Old Zohra, Baraka the camel, the
  rail guards and the Governor.
- **All twenty trains**, every tile, the mission names and text.
- **Things:** oil barrels, crates, anvils, powder sticks, geese, the ram, the
  crank gun, levers and iron gates, strongboxes and dinars; spare rounds,
  blast rounds, spring boots, the hamsa charm, the iron fist, the quick
  holster.
- **Pixel art and music:** a desert railway at dusk and night, and our tunes.

## Additions: none

Only the platform needs every UFO 40 cartridge has:

- the START pause menu;
- saving: missions beaten and every mission's stars;
- the three UFO 40 goals, which replicate Rail Heist's own three:

| UFO 40 goal | Condition | Rail Heist's goal |
|---|---|---|
| Beacon | beat mission 10 | gift: beat level 10 |
| Saucer | beat all 20 missions | gold: beat all 20 levels |
| Alien | beat the game with at least 40 stars | cherry: beat the game with at least 40 stars |

## Controls

| Input | Action |
|---|---|
| D-pad | walk, climb; ↓ ducks, ↓ + ← → rolls |
| A | jump |
| B | punch / pick up (ducking) / throw / put down (↓) |
| hold ↑ | load and draw the gun; then B fires |
| SELECT | swap outlaws (two-outlaw missions, between turns) |
| START | pause menu |

## Sources

- [W] UFO 50 Wiki (Miraheze), "Rail Heist": the objective and master timer,
  the turn timer (10 s) and lawmen turn (about 3.5 s), lawmen states, one
  shot then disarmed, gunshots alerting, stunned guards patrolling, stun
  length, auto punch, ducking and rolling, punching floors, walls and
  ceilings, the gun's draw time and automatic fire, every object, every
  power-up, silent and loud kills, stars, the twenty missions with outlaws,
  objectives and firsts, 2P Versus, goals.
  https://ufo50.miraheze.org/wiki/Rail_Heist
- [L] Lizstar's Trashcan, "UFO 50 Retrospective Part 28 - Rail Heist": turns
  begin once the lawmen are wary, they shoot on sight, hiding in barrels,
  throwing boxes, punching lawmen off the train, three medals.
  https://lizstar64.github.io/reviews/2024/10/16/UFO50-28.html
- [P] Popcar's Blog, "Reviewing Every Single UFO 50 Game": turn structure,
  punching, shoving off the train, breakable walls, the alert timer, the
  three stars, alternating two-character levels.
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [SC] Static Canvas, "The UFO 50 Diaries: Rail Heist": real time and turn
  based, twenty bite-sized missions, instant retry, medals for speed,
  lethality and pacifism, meagre ammunition, a fog of war.
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-rail-heist
