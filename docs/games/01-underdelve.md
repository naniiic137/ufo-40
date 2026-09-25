# 01 · UNDERDELVE

*Internal design document. Not shown in the product.*

## Tribute to

**Barbuta** (UFO 50 game #1, Mossmouth). The mechanics were researched from
text only: the community wiki, written guides and walkthroughs, reviews and
forum threads (listed under Sources). No UFO 50 images, video, maps, music or
text were used as references.

**Map type: fixed.** Barbuta's castle is one hand-made map, so ours is too
(`underdelve_rooms.c`). It copies the original's structure where the text
describes it, never its layouts:

| Structure | Barbuta (text sources) | UNDERDELVE |
|---|---|---|
| Map size | 8×8 flip screens [W] | 8×8 rooms (64 screens) |
| Wrap-around | leaving one side brings you in on the other [MG], [SD] | rows open at the edge wrap east ↔ west |
| Start | the left side of the map [GR] | Mo's camp, west side (A6) |
| Final boss | top right, in a tower [W], [GR] | the Old Lode at G1, the Sunstone at H1 |
| Life vendor | next to the boss [W], [GR] | the Lantern Keeper at F1 |
| Shops | B4 and B7 [W] | Burlap's Post at B4, the Owl's Exchange at B7 |
| Weapon trader | G8 [W] | the Salamander Forge at G8 |
| Blood sword | push the blocks at C4, enter C5 and die on its spikes [W] | Block Room C4, Hungry Spikes C5 |
| Key puzzle | ladders climbed in an order carved elsewhere, mirrored [W] (D5, hint H7) | Tally Ladders D5, Carved Tablet H7 |
| Bat altar | A4 [W] | Canary Altar A4 |
| Platform item | reached through a pool that looks deadly [W], [GR] (D7/E7) | Poison Pool D7 → Crank Vault E7 |
| Three ways to the boss | a switch, a locked door, a 500-cash hammer [GR], [SB] | the lever, the Deep Gate, the smith |

The zones by depth are ours: the Headframe and Upper Workings (rows 1-2),
the Glowcap Hollows (rows 3-4), the Crystal Veins (rows 5-6) and the Ember
Deep (rows 7-8).

## Mechanics checklist

| Mechanic | How UNDERDELVE does it | Source |
|---|---|---|
| No title screen | the cartridge boots straight into the camp | [MG] |
| Controls | D-pad walks and climbs; A jumps; B thrusts the weapon | [W], [MM] |
| One fixed jump | the same arc every time (tap or hold); no air control at all: every jump and every fall is committed | [W], [MM] |
| Jump speed | a jump carries Mo's walking speed; a standing jump goes straight up | [W], [MM] (reading) |
| Slow, stiff walker | full speed at once, no run; slower than every other UFO 40 hero | [W], [MG], [LN] |
| Low jump | a little over two tiles; some drops can't be climbed back up | [TH] |
| Short weapon | the pick reaches barely past Mo; most foes take 2-3 hits | [W], [LZ] |
| No hit feedback | nothing shows until a foe dies | [W] |
| One-hit death | any foe, shot, spike, ooze or drip | [W], [MM], [LZ] |
| Lives | six spare lanterns and the one Mo is on: seven lives | [MM], [GR] |
| Respawn | where Mo came into the room, keeping his items and ore (holding the ladder if he came in on one) | [MM], [GR] |
| Game over | with no lantern left, a death ends the delve; no continues, the next delve starts from the camp | [MM], [W] |
| Buying lives | the Lantern Keeper, just before the boss: 100 a lantern, up to six | [W], [GR] |
| Roaming Death | the Gloom moves one map cell, in a random direction, every time Mo changes room; the HUD map shows it | [W], [GR] |
| Roaming Death in a room | rises in the middle, then chases Mo at twice his speed; can't be hurt; its touch ends the delve (every lantern at once); leaving the room escapes it | [W], [MG] |
| Flip-screen map | 8×8 screens; a HUD map shows Mo's room and the Gloom's | [W], [MM] |
| Foes come back | every foe returns when Mo re-enters a room; ore and items stay taken | [W] |
| Money | chests struck open (50 or 100), loose gems (100), miner's shrines that pay 100 once their room is clear | [W] |
| Crumbly walls | the pick chips unstable walls; ore can hide inside | [W] |
| Hidden things | walk-through rock, invisible blocks, ladders that appear once climbed | [W], [GR], [SD] |
| Deaths on purpose | dying on one room's spikes brings Mo back where only the dead can go (the hungry pick) | [W], [SC] |
| Push blocks | holding against crates slides them and opens a way down | [W], [GR] |
| Ceiling drips | deadly without the umbrella (our copper pot) | [W] |
| Magic bubbles | crystal the pick pops once Mo has the pin (our tuning fork) | [W] |
| Red platforms | the mine lifts only run with the necklace (our gear crank) | [W] |
| Candy | climbing gloves: twice as fast on ladders | [W] |
| Key | opens the tower door (our brass tally and the Deep Gate) | [W] |
| Trash | an old boot, 50 ore, useless | [W] |
| Bat | the canary, from the altar; in the final fight it picks off the wisps | [W] |
| Altar | stand on the altar stone and wait | [W], [SB] |
| Rod | the sparker, traded for the pick; fires shots | [W] |
| Blood sword | the hungry pick hits twice as hard | [W] |
| Shop prices | pot 100 (Burlap) or 50 (Owl), boot 50, fork 200, gloves 100, lanterns 100, the smith 500 | [W], [GR] |
| Items aren't explained | nothing tells Mo what an item does | [MG] |
| Boss start | the Old Lode sleeps in its nest until Mo strikes it | [SB] (reading) |
| Final boss | 8 hits; drifts overhead dropping two shots at once, now and then drops to the floor, where it can be hurt; wisps cross the arena on a wave | [W] |
| Goals | see Additions | [W], [GR] |

### Foes

| Ours | Hits | Behaviour | Barbuta |
|---|---|---|---|
| Dustmoth | 1 | flies through walls, changing direction at random | Bat |
| Hop toad | 3 | hops four tiles left or right at random, bouncing off walls | Hopper |
| Grub | 2 | crawls to and fro, turning at walls and ledges | Slime |
| Axe newt | 2 | stays put facing one way and throws an axe four tiles ahead on a beat | Lizard |
| Swooper | 1 | hovers, then swoops at Mo | Fly |
| Grabsack | 10 | a chest until Mo is within three tiles, then hops a tile at a time | Mimic |
| Shellback | 3 | walks to and fro; only hurt from behind | Knight |
| Wisp | 1 | drifts across the arena on a wave | Ghost |
| The Old Lode | 8 | see above | The Malignance |
| The Gloom | — | see above | The Roaming Death |

### Readings we had to choose

- Numbers (walk 1 px/frame, jump 2.2 tiles, Gloom 2 px/frame): no source
  gives any.
- The jump keeps walking speed ("single fixed jump", "no mid-air adjustment").
- "Strike the blue egg" (a player's guess [SB]): any weapon wakes the Old Lode.
- Buying: UP at a shop, as up enters Barbuta's doors [MM].
- No foe is placed within 40 px of Mo when he comes into a room.

## What is ours

- **Name:** UNDERDELVE (released 1983 by the fictional Beamdown Softworks).
- **Hero:** Mo, a mole miner with a yellow hard hat and a pickaxe.
- **Setting:** the Underdelve, an abandoned mine gone dark. The Old Lode, a
  living geode, nests in the headframe beside the Sunstone.
- **Lives:** lanterns.
- **Items:** Copper Pot, Tuning Fork, Gear Crank, Climbing Gloves, Brass
  Tally, the Canary, Old Boot, the Sparker, Hungry Pick.
- **Characters:** Burlap the toad, the Owl, the Forge salamander, the Lantern
  Keeper, and the Smith with his sledgehammer.
- **Foes:** dustmoths, hop toads, grubs, axe newts, swoopers, grabsacks,
  shellbacks, wisps, the Gloom and the Old Lode.
- **Map:** 64 original hand-built screens (`underdelve_rooms.c`).
- **Music:** four original UFO-MML tracks (the mine theme, the Ember Deep
  theme, the boss theme, the victory theme) plus jingles. Barbuta has no
  music; ours stays by choice.

## Additions: none

Only the platform needs every UFO 40 cartridge has:

- the START pause menu;
- saving: the delve is saved as it goes (each death at once), and a
  cartridge with a delve in progress asks CONTINUE or NEW DELVE; a game over
  erases it;
- the three UFO 40 goals, which replicate Barbuta's own three:

| UFO 40 goal | Condition | Barbuta's goal |
|---|---|---|
| Beacon | explore 25 rooms | gift: visit 25 rooms |
| Saucer | bring the Sunstone home (after beating the Old Lode) | gold: liberate the castle |
| Alien | win with all six lanterns lit | cherry: win with max eggs |

## Controls

| Input | Action |
|---|---|
| D-pad left/right | walk |
| D-pad up/down | climb (hold UP mid-jump to catch a ladder); UP also buys, pulls the lever, pays the smith |
| A | jump (one fixed arc) |
| B | thrust the pick / fire the sparker |
| START | pause menu |

## Testing hooks

Headless tests drive Mo with real button presses (`ud_*.ufs`); cheats only
set up a room. Probes built on the game's own physics check the level
design: every spot reachable on foot in every room (`reach_lost`), the rooms
reachable from the camp with given items and gates (`reach_rooms_I_F`),
things that can't be walked up to (`reach_missing_I_F`) and rooms whose way
in would drop a returning Mo onto spikes (`respawn_traps`).

## Sources

- [W] UFO 50 Wiki (Miraheze), "Barbuta": controls, the committed jump, the
  8×8 castle, items and where they are, prices, money, foes, the Roaming
  Death, the Malignance, goals. https://ufo50.miraheze.org/wiki/Barbuta
- [MM] Steam guide "The missing manuals - How to play UFO 50 games": one
  jump height whether tapped or held, the thrust, up into doors, six eggs,
  "no continues, nor a save system".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [GR] Game Rant, "UFO 50: Barbuta Walkthrough (All Items, Secrets, and
  Cherry)": respawning in the room, the egg vendor, the three paths and the
  hammer's 500, the cherry.
  https://gamerant.com/barbuta-walkthrough-items-secrets-cherry-ufo-50/
- [MG] MoeGamer, "UFO 50: Kicking things off with Barbuta": no title screen,
  sedate walking, the skull takes every life, the wrap, unexplained items.
  https://moegamer.net/2024/09/20/ufo-50-kicking-things-off-with-barbuta/
- [SD] Steam thread "Barbuta Discussion": the loop, invisible ladders.
  https://steamcommunity.com/app/1147860/discussions/0/4852154959746326166
- [SB] Steam thread "Barbuta: Can't seem to find a way to beat this game":
  the blue egg, the invisible block, the lever, the altar.
  https://steamcommunity.com/app/1147860/discussions/0/592887778739535799/
- [TH] Thoughts on Series, "Thoughts on Barbuta": one-way drops.
  https://thoughtsonseries.substack.com/p/thoughts-on-barbuta-ufo-50
- [LZ] Lizstar, UFO 50 #1: one-hit deaths, the short sword.
  https://lizstar64.github.io/reviews/2024/10/03/UFO50-1.html
- [LN] Lost Nostalgia, UFO 50 games 1-25: "painfully sluggish".
  https://lostnostalgia.com/ufo-50s-every-game-played-and-graded-1-25
- [SC] Search summaries of the TV Tropes recap: dying on purpose to come back
  somewhere out of reach.
