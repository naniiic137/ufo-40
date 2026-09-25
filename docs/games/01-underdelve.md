# 01 · UNDERDELVE

*Internal design document. Not shown in the product.*

## Tribute to

**Barbuta** (UFO 50 game #1, Mossmouth). The mechanics were researched from
text only: the community wiki, written walkthroughs, reviews and forum threads
(listed under Sources). No UFO 50 images, video, maps, music or text were used
as references.

**Map type: fixed.** Barbuta's castle is one hand-made map, so ours is too
(`underdelve_rooms.c`). It copies the original's structure where the text
describes it, never its layouts:

| Structure | Barbuta (text sources) | UNDERDELVE |
|---|---|---|
| Map size | 8×8 rooms [W] | 8×8 rooms (64 screens) |
| Wrap-around | leaving one side brings you in on the other [MG], [SD] | rows open at the edge wrap east ↔ west |
| Start | the left side of the map [GR] | Mo's camp, west side (A6) |
| Final boss / tower | top right [GR] | the Old Lode at G1, the Sunstone at H1 |
| Life vendor | next to the boss [W], [GR] | the Lantern Keeper at F1 |
| Shops | one mid-left, one lower-left [W] (B4, B7) | Burlap's Post at B4, the Owl's Exchange at B7 |
| Weapon trader | bottom right [W] (G8) | the Lizard Forge at G8 |
| Blood sword area | middle left, with a push-block room [W], [GR] (C4/C5/B5) | Block Room C4 and Hungry Spikes C5 |
| Key puzzle | a room of ladders with the order carved elsewhere, mirrored [W] (D5, hint H7) | Key Ladders D5, Carved Tablet H7 |
| Companion altar | left side [W] (A4) | Canary Altar A4 |
| Platform activator | reached through a pool that looks deadly [W], [GR] (D7/E7) | Poison Pool D7 → Crank Vault E7 |
| Money | coins 50 and jewels 100, in chests (struck open), one in a wall; skull boxes pay a jewel when their room is clear [W] | 5 coin spots (50), 3 gem spots (100), 2 shrines (100): 750 ore in all |
| Three paths to the boss | a switch, a locked door, a 500-cash hammer [GR], [SD] | the lever, the Deep Gate, the smith |

The zones by depth are ours: the Headframe and Upper Workings (rows 1-2),
the Glowcap Hollows (rows 3-4), the Crystal Veins (rows 5-6) and the Ember
Deep (rows 7-8).

## Mechanics checklist

| Mechanic | How UNDERDELVE does it | Source |
|---|---|---|
| One-hit death | any foe, shot, spike, ooze or drip costs a lantern | [W], [MG] |
| Lives | six lanterns; lose them all and the delve restarts from the camp, no continue | [GR], [W] |
| Respawn | after a death Mo comes back where he entered the room | [GR] |
| Committed jumps | a fixed arc, no air control; walking off a ledge keeps momentum | [W] |
| Slow walker | Mo moves slowly | [W] |
| Short weapon | the pick barely reaches past Mo; most foes take 2-3 hits | [W] |
| No hit feedback | nothing shows until a foe dies (no flash, no sound, no health bar) | [W] |
| Knight | the shellback can only be hurt from behind | [W] |
| Flip-screen map | a map in the HUD shows where Mo is and where the Gloom is | [W], [MG] |
| Foes come back | every foe returns when Mo re-enters a room; ore and items stay taken | [W] |
| Chests | opened by striking them | [GR] |
| Crumbly walls | the pick chips unstable walls; ore can hide inside | [W] |
| Fake walls, hidden blocks, hidden ladders | walk-through rock, invisible ledges that show when touched, ladders that appear once climbed | [W], [GR], [SD] |
| Push blocks | holding against crates slides them and opens a way down | [GR] |
| Roaming Death | the Gloom moves one room in a random direction every time Mo changes room; if it's in his room it rises in the middle, is faster than he is and can't be hurt; its touch puts out every lantern at once | [W], [MG], [SR] |
| Ceiling drips | deadly without the umbrella (our copper pot) | [W] |
| Magic bubbles | crystal bubbles that the pick pops once Mo has the pin (our tuning fork) | [W], [GR] |
| Red platforms | the mine lifts only run with the necklace (our gear crank) | [W] |
| Candy | climbing gloves: faster climbing | [W], [GR] |
| Key | opens the tower door (our Deep Gate) | [W] |
| Trash | an old boot, 50 ore, useless | [W] |
| Bat | the canary; in the final fight it deals with the wisps | [W] |
| Stand-on-a-block secret | standing on the altar stone long enough lowers a ladder to the canary | [GR] |
| Rod | traded for the pick; fires shots | [W] |
| Blood sword | the hungry pick hits twice as hard; getting it means dying on its spikes, which drop Mo where only the dead can go | [W], [GR] |
| Shop prices | pot 100 (Burlap) or 50 (Owl), boot 50, fork 200, gloves 100, lanterns 100 | [W] |
| Items aren't explained | nothing tells Mo what an item does; he just gets it | [MG] |
| Final boss | 8 hits; drifts overhead dropping two shots at once and now and then drops to the floor, where it can be hurt; wisps cross the arena on a wave | [W] |
| Win | beat the boss and reach the Sunstone | [W] |

### Foes

| Ours | Hits | Behaviour | Barbuta |
|---|---|---|---|
| Bat-moth | 1 | flies through walls, changing direction at random | Bat |
| Hop toad | 3 | hops at random up to four tiles, bouncing off walls | Hopper |
| Grub | 2 | crawls to and fro, turning at walls and ledges | Slime |
| Axe newt | 2 | stays put and throws an axe four tiles ahead on a beat | Lizard |
| Swooper | 1 | hovers, then swoops at Mo | Fly |
| Mimic sack | 10 | a chest until Mo is within three tiles, then hops a tile at a time | Mimic |
| Shellback | 3 | walks to and fro; only hurt from behind | Knight |
| Wisp | 1 | drifts across the arena on a wave; the canary picks them off | Ghost |
| The Old Lode | 8 | see above | The Malignance |
| The Gloom | — | see above | The Roaming Death |

### Readings we had to choose

- The Gloom's speed (a little faster than Mo) and the moment it takes to rise
  are ours; the sources only say it is much faster than the player.
- The Gloom never enters the boss arena or the Sunstone room.
- The switch path ("a gap in the floor"): our lever drops the slab that seals
  the trapdoor shaft up to the Lantern Keeper.
- The bat altar is behind crystal, as the sources imply the pin comes first.

## What is ours

- **Name:** UNDERDELVE (released 1983 by the fictional Beamdown Softworks).
- **Hero:** Mo, a mole miner with a yellow hard hat and a pickaxe.
- **Setting:** the Underdelve, an abandoned mine gone dark. The Old Lode, a
  living geode, sits in the headframe with the Sunstone.
- **Lives:** lanterns.
- **Items:** Copper Pot, Tuning Fork, Gear Crank, Climbing Gloves, Brass Key,
  the Canary, Old Boot, Spark Rod, Hungry Pick.
- **Characters:** Burlap the toad, the Owl, the Forge lizard, the Lantern
  Keeper, and the Smith with his sledgehammer.
- **Map:** 64 original hand-built screens (`underdelve_rooms.c`). Every
  room's edges are checked by the `map_errors` test.
- **Music:** four original UFO-MML tracks (the mine theme, the Ember Deep
  theme, the boss theme, the victory theme) plus jingles.

## Additions: none

Only the platform needs every UFO 40 cartridge has:

- the START pause menu;
- saving: the delve is saved whenever Mo changes room or finds something,
  and CONTINUE resumes it (a game over erases it, as Barbuta has no continues);
- the three UFO 40 goals, which replicate Barbuta's own three:

| UFO 40 goal | Condition | Barbuta's goal |
|---|---|---|
| Beacon | explore 25 rooms | gift: visit 25 rooms |
| Saucer | bring the Sunstone home | gold: liberate the castle |
| Alien | win with all six lanterns lit | cherry: win with max eggs |

Removed in the faithfulness audit: the old 6×6 map; loose ore everywhere and
ore dropped by foes; one-hit foes; hit flashes and clinks; item-explaining
messages; the Gloom's random chance to appear; ropes that needed gloves; the
Crank and the Rod being bought; the Hungry Pick's "pay a lantern" altar; the
canary following Mo everywhere; the boss's health bar and landing sparks; and
the "free the canary" and "no deaths" goals.

## Controls

| Input | Action |
|---|---|
| D-pad left/right | walk |
| D-pad up/down | climb ladders; up also buys, pulls the lever, pays the smith |
| A | jump (committed arc) |
| B | swing the pick / fire the rod |
| START | pause menu |

## Sources

- [W] UFO 50 Wiki (Miraheze), "Barbuta": the 8×8 castle, items and where they
  are, prices, money, every foe's hit points and behaviour, the Roaming Death,
  the Malignance, goals. https://ufo50.miraheze.org/wiki/Barbuta
- [GR] Game Rant, "UFO 50: Barbuta Walkthrough (All Items, Secrets, and
  Cherry)": six eggs, respawning in the room, the three paths, the hammer
  path's 500, striking chests and bubbles, the block push, the purple block,
  the invisible platform, the blood sword's spike respawn, where things are in
  broad strokes. https://gamerant.com/barbuta-walkthrough-items-secrets-cherry-ufo-50/
- [MG] MoeGamer, "UFO 50: Kicking things off with Barbuta": the skull takes
  every life, the map wraps horizontally, items aren't explained, the minimap.
  https://moegamer.net/2024/09/20/ufo-50-kicking-things-off-with-barbuta/
- [SD] Steam threads "Barbuta Discussion" and "Barbuta: Can't seem to find a
  way to beat this game": the map loops, invisible ladders, block pushing, the
  three routes.
- [SR] Search summaries of the fandom wiki: "The Roaming Death ... will
  instantly defeat the Knight and cause a game over upon contact".
