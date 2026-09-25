# 01 · UNDERDELVE

*Internal design document. Not shown in the product.*

## Tribute to

**Barbuta** (UFO 50 game #1, Mossmouth). The mechanics were researched from
text only: the community wiki, written guides and walkthroughs, reviews and
forum threads (listed under Sources). No UFO 50 images, video, maps, music or
text were used as references.

**Map type: fixed.** Barbuta's castle is one hand-made map, so ours is too
(`underdelve_rooms.c`). It follows the original's coarse structure where the
text describes it; the rooms, their layouts and which grid cell holds what
are ours (no feature stands in the cell the wiki gives it in Barbuta):

| Structure | Barbuta (text sources) | UNDERDELVE |
|---|---|---|
| Map size | 8×8 flip screens [W] | 8×8 rooms (64 screens) |
| Wrap-around | leaving one side brings you in on the other [MG], [SD] | rows open at the edge wrap east ↔ west |
| Start | the left side of the map [GR] | Mo's camp, west side |
| Shops | two, near the start [W] | Burlap's Post and the Owl's Exchange, just east of the camp, one above and one below |
| Final boss | a tower, top right [W], [GR] | the Old Lode's nest, top right; the Sunstone just east of it, round the wrap |
| Life vendor | next to the boss [W], [GR] | the Lantern Keeper, next door |
| Weapon trader | bottom right [W] | the Salamander Forge, bottom right |
| Blood sword | push blocks, then die on the spikes below [W] | Block Room over Hungry Spikes |
| Key puzzle | ladders climbed in an order carved elsewhere, mirrored [W] | Tally Ladders, and the Carved Tablet far away |
| Bat altar | the west side [W] | Canary Altar, west |
| Platform item | reached through a pool that looks deadly [W], [GR] | Poison Pool → Crank Vault |
| Three ways to the boss | a switch, a locked door, a 500-cash hammer [GR], [SB] | the lever, the Deep Gate, the smith |

The zones by depth are ours: the Headframe and Upper Workings (rows 1-2),
the Glowcap Hollows (rows 3-4), the Crystal Veins (rows 5-6) and the Ember
Deep (rows 7-8).

## Mechanics checklist

| Mechanic | How UNDERDELVE does it | Source |
|---|---|---|
| No title screen | the cartridge boots straight into the camp | [MG] |
| Controls | D-pad walks and climbs; A jumps; B thrusts the weapon | [W], [MM] |
| One fixed jump | the same arc every time (tap or hold); no air control: every jump and every fall is committed | [W], [MM] |
| Jump speed | a jump carries Mo's walking speed; a standing jump goes straight up | [W], [MM] (reading) |
| Slow, stiff walker | full speed at once, no run; slower than every other UFO 40 hero | [W], [MG], [LN] |
| Low jump | a little over two tiles; some drops can't be climbed back up | [TH] |
| Ladders | taken from the ground only (no catching one in mid-air) | [W] |
| Short weapon | the pick reaches barely past Mo; most foes take 2-3 hits | [W], [LZ] |
| No hit feedback | nothing shows until a foe dies | [W] |
| One-hit death | any foe, shot, spike, ooze, drip or falling rock | [W], [MM], [LZ] |
| Lives | six spare lanterns and the one Mo is on: seven lives | [MM], [GR] |
| Respawn | a spare lantern flickers alight where Mo came into the room; he keeps his items and ore | [MM], [GR], [LZ] |
| Game over | with no lantern left, a death ends the delve; no continues, the next delve starts from the camp | [MM], [W] |
| Buying lives | the Lantern Keeper, just before the boss: 100 a lantern, up to six | [W], [GR] |
| Roaming Death | the Gloom moves one map cell, in a random direction, every time Mo changes room; the HUD map shows it; nothing explains it | [W], [GR], [SM] |
| Roaming Death in a room | rises in the middle, then chases Mo at twice his speed; can't be hurt; its touch ends the delve (every lantern at once); leaving the room escapes it | [W], [MG] |
| HUD | ore, items, lanterns and the map; no room names | [MM], [MG] |
| Flip-screen map | 8×8 screens | [W] |
| Foes | always where the map puts them, and back whenever Mo re-enters a room; ore and items stay taken | [W] |
| Traps | a floor tile with a faint seam drops a loose rock from the ceiling; it kills and stays where it lands. One right beside the start | [GR], [SS], [MG] |
| Hint-givers | five glow-worms, each with one cryptic line (UP beside them) | [LZ] |
| Money | chests struck open (50 or 100), loose gems (100), miner's shrines that pay 100 once their room is clear, ore in cracked rock | [W] |
| Secrets | walk-through rock hiding gems, cracked rock that gives way to three blows (a gem behind it or ore inside), invisible blocks, ladders that appear once climbed | [W], [GR], [SD], [SC], [LZ] |
| Still pools | the harmless ooze lies still; only deadly ooze churns | [SD] |
| Deaths on purpose | dying on one room's spikes brings Mo back where only the dead can go (the hungry pick) | [W], [TT] |
| Push blocks | holding against crates slides them and opens a way down | [W], [GR] |
| Ceiling drips | deadly without the umbrella (our copper pot) | [W] |
| Magic bubbles | crystal the pick pops once Mo has the pin (our tuning fork) | [W] |
| Red platforms | the mine lifts only run with the necklace (our gear crank) | [W], [GR] |
| Candy | climbing gloves: twice as fast on ladders | [W] |
| Key | opens the tower door (our brass tally and the Deep Gate) | [W] |
| Trash | an old boot, 50 ore, useless | [W] |
| Bat | the canary, from the altar; in the final fight it picks off the wisps | [W] |
| Altar | stand on the altar stone and wait | [W], [SB] |
| Rod | the sparker, traded for the pick; fires shots | [W] |
| Blood sword | the hungry pick hits twice as hard | [W] |
| Shop prices | pot 100 (Burlap) or 50 (Owl), boot 50, fork 200, gloves 100, lanterns 100, the smith 500 | [W], [GR] |
| Items aren't explained | nothing tells Mo what an item does | [MG] |
| Boss start | the fight starts as Mo reaches the nest; the way back closes behind him | [TH] |
| Final boss | 8 hits; drifts overhead dropping two shots at once, now and then drops to the floor, where it can be hurt; wisps cross the arena on a wave | [W] |
| Goals | see Additions | [W], [GR] |

### Foes

| Ours | Hits | Behaviour | Barbuta |
|---|---|---|---|
| Dustmoth | 1 | flies through walls, changing direction at random | Bat |
| Hop toad | 3 | hops four tiles left or right at random, bouncing off walls | Hopper |
| Grub | 2 | crawls to and fro, turning at walls and ledges | Slime |
| Axe newt | 2 | stays put facing the way the map sets and throws an axe four tiles ahead on a beat | Lizard |
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
- Ladders: a hop off one (A with a direction) is kept; Mo can reach a ladder
  whose foot is just above his head.
- Buying: UP at a shop, as up enters Barbuta's doors [MM].

## What is ours

- **Name:** UNDERDELVE (released 1983 by the fictional Beamdown Softworks).
- **Hero:** Mo, a mole miner with a yellow hard hat and a pickaxe.
- **Setting:** the Underdelve, an abandoned mine gone dark. The Old Lode, a
  living geode, nests in the headframe beside the Sunstone.
- **Lives:** lanterns.
- **Items:** Copper Pot, Tuning Fork, Gear Crank, Climbing Gloves, Brass
  Tally, the Canary, Old Boot, the Sparker, Hungry Pick.
- **Characters:** Burlap the toad, the Owl, the Forge salamander, the Lantern
  Keeper, the Smith with his sledgehammer, and the glow-worms and their lines.
- **Foes:** dustmoths, hop toads, grubs, axe newts, swoopers, grabsacks,
  shellbacks, wisps, the Gloom and the Old Lode.
- **Map:** 64 original hand-built screens (`underdelve_rooms.c`) and where
  each one stands.
- **Music:** four original UFO-MML tracks (the mine theme, the Ember Deep
  theme, the boss theme, the victory theme) plus jingles. Barbuta has no
  music; Hamza keeps ours, and the death jingle, by choice.

## Additions: none

Only the platform needs every UFO 40 cartridge has:

- the START pause menu;
- saving: the delve is saved as it goes (each death at once), and a
  cartridge with a delve in progress asks CONTINUE or NEW DELVE; a game over
  erases it (Barbuta itself doesn't save);
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
| D-pad up/down | climb (a ladder is taken from the ground); UP also buys, pulls the lever, pays the smith, asks a glow-worm |
| A | jump (one fixed arc) |
| B | thrust the pick / fire the sparker |
| START | pause menu |

## Testing hooks

Headless tests drive Mo with real button presses (`ud_*.ufs`); cheats only
set up a room. Probes built on the game's own physics check the level
design: every spot reachable on foot in every room (`reach_lost`), the rooms
reachable from the camp with given items and gates (`reach_rooms_I_F`),
things that can't be walked up to (`reach_missing_I_F`), rooms whose way in
would drop a returning Mo onto spikes (`respawn_traps`) and foes placed on
a way in (`entry_foes`).

## Sources

- [W] UFO 50 Wiki (Miraheze), "Barbuta": controls, the committed jump, the
  8×8 castle, items, prices, money, foes, the Roaming Death, the Malignance,
  goals. https://ufo50.miraheze.org/wiki/Barbuta
- [MM] Steam guide "The missing manuals - How to play UFO 50 games": one
  jump height whether tapped or held, the thrust, up into doors, six eggs,
  the sidebar (cash, items, map), "no continues, nor a save system".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [GR] Game Rant, "UFO 50: Barbuta Walkthrough (All Items, Secrets, and
  Cherry)": the first tile to the right that drops the ceiling, respawning
  in the room, the egg vendor, the three paths and the hammer's 500.
  https://gamerant.com/barbuta-walkthrough-items-secrets-cherry-ufo-50/
- [SS] Set Side B, "A walkthrough of Barbuta": the first screen's trap.
  https://setsideb.com/a-walkthrough-of-barbuta-ufo-50-1/
- [MG] MoeGamer, "UFO 50: Kicking things off with Barbuta": no title screen,
  sedate walking, the mean first-screen trap, the skull, the wrap, items
  never explained. https://moegamer.net/2024/09/20/ufo-50-kicking-things-off-with-barbuta/
- [SD] Steam thread "Barbuta Discussion": the loop, invisible ladders, the
  still (non-animated) pools.
  https://steamcommunity.com/app/1147860/discussions/0/4852154959746326166
- [SB] Steam thread "Barbuta: Can't seem to find a way to beat this game":
  the invisible block, the lever, the altar.
  https://steamcommunity.com/app/1147860/discussions/0/592887778739535799/
- [SM] Steam thread on the map dot: learn what it is by meeting it.
  https://steamcommunity.com/app/1147860/discussions/0/4699034745340532462
- [TH] Thoughts on Series, "Thoughts on Barbuta": one-way drops, the boss
  room closing behind you.
  https://thoughtsonseries.substack.com/p/thoughts-on-barbuta-ufo-50
- [LZ] Lizstar, UFO 50 #1: one-hit deaths, the short sword, hint-giving
  blobs, cracked blocks, lives as eggs you hatch out of.
  https://lizstar64.github.io/reviews/2024/10/03/UFO50-1.html
- [SC] Static Canvas, "The UFO 50 Diaries: Barbuta": hidden walls covering
  secrets. https://staticcanvas.substack.com/p/the-ufo-50-diaries-barbuta
- [LN] Lost Nostalgia, UFO 50 games 1-25: "painfully sluggish".
  https://lostnostalgia.com/ufo-50s-every-game-played-and-graded-1-25
- [TT] Search summaries of the TV Tropes recap: dying on purpose to come back
  somewhere out of reach.
