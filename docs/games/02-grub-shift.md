# 02 · GRUB SHIFT

*Internal design document. Not shown in the product.*

## Modelled on

**Bug Hunter** (UFO 50 game #2, Mossmouth). Rules were researched from text
sources only (the community wiki's rules and module list, the MoeGamer review
and other written reviews). No UFO 50 images, video, sprites, music or text
were used as references.

## Core rules we keep

- **Turn-based grid tactics** on a small board with raised platforms and holes.
- **Seven one-use tool slots.** Every slot can be used once per day. Using a
  tool spends it until the next day. **Rest** ends the day and is always
  available.
- **Starting kit:** four continuous moves (1–2 tiles), one discrete hop
  (1–2 tiles), one continuous shot (1–2 tiles) and one single-tile lob
  (up to 2 tiles away).
- **Continuous vs discrete movement:**
  - Continuous moves collect everything they pass over. They push bugs one
    tile, and a bug pushed into a hole dies. They can step down off a
    platform, but they can't climb up onto one.
  - Discrete hops ignore elevation and don't push.
  - Stepping down off a platform onto a bug **stomps** it.
- **Elevation and shots:**
  - A continuous shot hits every tile along its path.
  - It's blocked by a higher tile. Firing downhill works, firing uphill doesn't.
  - Lobs ignore elevation.
- **Energy:**
  - Explosive energy pods drop every night. Picking them up gives you
    currency to buy new tools.
  - A tile can hold two pods. A third pod makes that tile explode, which hits
    the 8 tiles around it, blows a hole in the tile and chains into
    neighbouring pods.
  - Hitting a pod tile with an attack detonates it too.
- **The shop.** Eight offers every day: five cost 2, one costs 3 and two cost 4.
  Buying replaces a slot of your choice, and the new tool can be used
  straight away.
- **Bug life cycle:**
  - Larvae spawn every night.
  - Each night one species evolves (larva → adult → queen). Only two species
    per contract ever evolve.
  - A queen left alive becomes an **egg** the next night, except on the final
    day.
  - If an egg survives to the end of a day it hatches and you **lose**.
  - Breaking eggs doesn't count towards the quota.
- **Species abilities:** adults and queens have abilities, larvae don't.
  - One species fires sparks in four directions when it's killed by an
    attack or explosion.
  - One species is armoured and needs two hits, but a stomp or a hole kills it
    instantly.
  - One species leaves a hole behind when it dies.
  - One species raises its own tile into a platform when it evolves.
  - Kills by stomp or hole never trigger an ability.
- **Winning and losing:**
  - Meet the kill quota before the last day ends.
  - You lose if you're caught in a blast or sparks, if an egg hatches, or if
    you miss the quota.
- **Streaks.** Contracts get harder as your winning streak grows.

## What is ours

- **Name:** GRUB SHIFT (1983, Beamdown Softworks).
- **Hero:** Tilly, a pint-sized tiller robot on the night shift.
- **Setting:** the glass domes of Orchard Station, a farm on a comet. The grubs
  come out at night. Days are *shifts*, jobs are *contracts* and energy cubes
  are **fizz pods**, pods of volatile fertiliser.
- **Tools** (our names and our own selection):
  - moves: ROLL, DASH, STREAK, SKIP, LEAP, VAULT, WARP
  - attacks: ZAP, ARC, BEAM, TOSS, LOB, MORTAR, PULSE, DETONATE
  - specials: SEED, RAISE, RECHARGE, REBOOT, DEVOLVE
- **Species** (our designs):
  - Sparkmites: yellow, fire sparks when they die.
  - Shellbugs: blue, armoured.
  - Burrowers: brown, leave a hole when they die.
  - Moundmakers: green, raise their tile when they evolve.
- **Board:** 8×6 tiles, with our own procedural layouts per contract.
- **Contracts:** three contracts make a full run, ending with the Employee of
  the Month certificate. After that, Overtime contracts go on forever.
- **Music:** original "Night Shift" groove, briefing theme and jingles.

## Controls

| Input | Action |
|---|---|
| D-pad | move the cursor between tool slots, the shop and the board |
| A | pick a tool / confirm a target / buy |
| B | cancel |
| START | pause menu |

## Goals

| Tier | Goal |
|---|---|
| Beacon | Squash 4 or more grubs with a single action |
| Saucer | Complete 3 contracts in a row (Employee of the Month) |
| Alien | Complete 5 contracts in a row |

## Testing hooks

The game logic lives in one plain `Board` struct, and every action is a pure
function of it. That lets the headless tests run a greedy **autoplay bot** that
tries every tool and target on a copy of the board. We use it as a balance
smoke test and to prove a full contract can be won.
