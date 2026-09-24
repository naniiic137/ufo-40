# 01 · UNDERDELVE

*Internal design document. Not shown in the product.*

## Modelled on

**Barbuta** (UFO 50 game #1, Mossmouth). Mechanics were researched from text
sources only (the community wiki, reviews and walkthrough articles). No UFO 50
images, video, maps, music or text were used as references.

## Core mechanics we keep

These are the rules that make it play like its counterpart:

- **One-hit death.** Any enemy, projectile, spike, ooze or ceiling drip kills you.
- **Lives as tokens.** You carry a stock of lives (ours: six lanterns). Lose
  them all and the run starts again from the very beginning, with no continues.
- **Committed jumps.** The jump has a fixed arc and no air control. You pick a
  direction when you take off and then you're committed. Walking off a ledge
  keeps your walking momentum.
- **Short melee weapon** with no feedback until the enemy dies. Some enemies
  have a hard front and can only be hit from behind.
- **Interconnected flip-screen map** on a square grid of rooms, with a minimap
  in the HUD. Enemies respawn when you re-enter a room. Collected treasure
  stays collected.
- **Money** from treasure (small pickups and big pickups) and sometimes from
  enemies. You spend it at **shops** on items.
- **Items open new paths**, metroidvania style:
  - protection from ceiling drips
  - breaking a barrier material
  - powering moving platforms
  - faster climbing (ours also lets you grip ropes)
  - a key for the final door
  - a companion that deals with the boss's minions
  - a ranged weapon you get by trading in your melee weapon
  - a cursed double-damage weapon that costs a life to take
- **A life vendor** near the boss, so you can buy lives before the final fight.
- **An unkillable roaming hazard** that sometimes shows up when you enter a
  room. The only escape is to leave the room.
- **Secrets:** fake walls, hidden blocks and treasure sacks that turn out to
  be enemies.
- **Final boss** that drifts overhead dropping paired projectiles and
  periodically comes down to the floor. It can only be hurt while it's down.

## What is ours

- **Name:** UNDERDELVE (released 1983 by the fictional Beamdown Softworks).
- **Hero:** Mo, a mole miner with a yellow hard hat and a pickaxe.
- **Setting:** the Underdelve, an abandoned mine gone dark. It has four
  depth zones, each with its own palette: Upper Workings (timber and torches),
  Glowcap Hollows (glowing mushrooms), Crystal Veins (blue crystal) and the
  Ember Deep (lava and ooze).
- **Lives:** lanterns.
- **Items:**
  - Copper Pot (drips bounce off it)
  - Tuning Fork (shatters crystal walls)
  - Gear Crank (powers the mine lifts)
  - Climbing Gloves (grip ropes, climb twice as fast)
  - Brass Key
  - the Canary (it pecks the boss's wisps out of the air)
  - Spark Rod (fires sparks; you trade your pick for it)
  - Hungry Pick (double damage; feeding it costs a lantern)
- **Map:** 36 original hand-built screens in a 6×6 grid (`underdelve_rooms.c`).
- **Characters:**
  - Burlap the toad trader
  - the Owl exchange
  - the Forge lizard
  - the Lantern Keeper
- **Enemies:**
  - dust moths
  - hop toads
  - grubs
  - rock spitters
  - ceiling swoopers
  - mimic ore sacks
  - shellback beetles
  - wisps
- **Roaming hazard:** the Gloom.
- **Boss:** the Old Lode, a living geode that guards the Sunstone.
- **Music:** four original UFO-MML tracks (the mine theme, the Ember Deep
  theme, the boss theme and the victory theme) plus jingles.

## Controls

| Input | Action |
|---|---|
| D-pad left/right | walk |
| D-pad up/down | climb ladders and ropes, buy at a shop, take the altar pick |
| A | jump (committed arc) |
| B | swing pick / fire rod |
| START | pause menu |

## Goals (UFO 40 three-tier system)

| Tier | Goal |
|---|---|
| Beacon | Free the canary (it's hidden behind a fake wall) |
| Saucer | Defeat the Old Lode and reach the Sunstone |
| Alien | Win without losing a single lantern |

## Critical path

1. Collect ore in the Upper Workings and buy the **Copper Pot** from Burlap (300).
2. Climb down the drip shaft into the Hollows and find the **Tuning Fork** in the
   Fork Vault.
3. Shatter the crystal walls to reach the north-east. Buy the **Gear Crank** at
   the Owl exchange (500).
4. Ride the lifts in the Lift Shaft to the Glove Ledge and take the **Climbing
   Gloves**.
5. Climb down the Rope Drop to the Key Vault and take the **Brass Key**.
6. Buy lanterns from the Lantern Keeper, open the Deep Gate and defeat the Old Lode.
7. Touch the Sunstone.
