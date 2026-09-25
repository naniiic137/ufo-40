# 03 · ROOFCAT

*Internal design document. Not shown in the product.*

## Tribute to

**Ninpek** (UFO 50 game #3, Mossmouth). The mechanics were researched from
text only: the community wiki, written reviews and Steam guides (listed under
Sources). No UFO 50 images, video, sprites, music, stage layouts or text were
used as references.

**Map type: fixed.** Ninpek is one hand-made world, so ours is too
(`roofcat_levels.c`). It matches the original's structure where the text
sources describe it:

| Structure | Ninpek | ROOFCAT |
|---|---|---|
| World | one long continuous world, split into several distinct stages [MG] | one continuous town in four areas that flow into each other without stopping |
| Bonus areas | "a couple of bonus areas" with cherries [MG] | two bonus stretches (the roof garden, the fish market) full of snacks and no foes |
| Bosses | one, at the end of the world [W], [MG] | one: Old Crab, at the end of the harbour |
| Length | under nine minutes to finish [WP] | 48 screens at the scroll speed ≈ 8½ minutes plus the fight |
| Second loop | a harder second loop [W] | the Night Route |
| Hidden sandwiches | several hidden spots [W] | five hidden letter spots |
| Balloons at fixed places | some 1-up balloons sit at predetermined spots [W], [MG] | four placed lanterns |

The screens are our own designs. The order of what each area teaches is
ours too: rooftops teach gaps, jumps and walkers; the souk adds shooters and
the first bonus stretch; the fort brings divers, beams and spike lamps; the
harbour has water and fish, then the second bonus stretch and the fight.

## Mechanics checklist

| Mechanic | How ROOFCAT does it | Source |
|---|---|---|
| Auto-scroll | the screen and Harissa keep moving right; standing still carries her along | [W], [MG] |
| Scroll carries the rooftops | idle, she drifts off the end of whatever she stands on | [MG] |
| Death by the scroll | squeezed between the left edge and a wall costs a life | our reading of the auto-scroll |
| Double jump | a second press in mid-air | [W], [MM] |
| Drop through | down + jump drops through a one-way ledge | [W] |
| Throwing | B throws a short-range star; holding B keeps throwing | [W], [MM] |
| One star at first | only one star on screen at a time | [W] |
| Power-ups | every third defeated foe drops catnip while she holds fewer than two; each allows one more star on screen (up to three) | [W], [MG] |
| Losing power-ups | all catnip is lost with a life | [W] |
| Ghost mode | losing a life turns her into a spirit that floats down from the top of the screen, flies freely, fires twin shots much slower than stars, passes through everything and can't collect anything | [W], [MG] |
| Coming back | the spirit ends on a timer or when A is pressed; she returns where the spirit is, with no invincibility | [W], [MG] |
| Game over | when the last life is lost | [W] |
| No points for kills | foes drop a token instead; the token scores when picked up | [MG] |
| Token values | green 100, silver 200, violet 300 by foe | [W] egg colours |
| Direct points | spike lamps and firecrackers pay 200 when destroyed | [W] mines and shrooms |
| Crowns | a jumping jar left alone for 16 hops turns into a crown worth 300 | [W] (the wiki says 300; MoeGamer says 500) |
| Snacks | 5 points each; after 100 collected they become 20-point snacks, after 100 more 100-point snacks | [W] cherry → onigiri → coffee |
| Lost letters | standing on a hidden spot calls down a letter worth 500 | [W], [MG] sandwiches |
| Extra lives at scores | at 3,000, then 7,000, then every 5,000 a lantern drifts up; pop it with a star | [W], [MG] |
| Extra lives at places | lanterns wait at four spots in the town | [W], [MG] |
| Boss | 35 hit points; only a hit in the eye counts; bursts of shots; the fight is over water and the legs rising out of it are the only footing; flying fish and falling orbs join in | [W] |
| Orbs | fall slowly and split into a left half and a right half | [W] |
| Loop 2 | the same world again after the ending, keeping score and lives, with the changes below | [W] |
| Ending | the parcel comes home; the second loop brings home a whole stack | [W] (sandwich / tower of sandwiches) |

### The foes

Every Ninpek foe type [W] has a counterpart with the same hit points, points
and behaviour. The designs, names and art are ours.

| Ours | Hit points | Token | Behaviour | Ninpek |
|---|---|---|---|---|
| Pigeon | 2 | 100 | walks its platform back and forth; now and then tucks in and rolls | Chops |
| Crow | 4 | 200 | pops out of a chimney, turns to face her, fires two evenly spaced pebbles | Snake |
| Gecko | 1 | 100 | spits a seed that bounces along the roofs | Slime |
| Gull | 2 | 100 | floats slowly in from the right edge to the left | Floats |
| Spike lamp | 10 | 200 on the spot | stays put while spikes circle it | Mine |
| Flying fish | 1 | 200 | waits in the water, then leaps in a fixed arc | Yellow fish |
| Wasp | 1 | 100 | homes in on her, then speeds up | Bee |
| Snail | 1 | 100 | crawls slowly back and forth | Red snail |
| Toad | 4 | 300 | sits, now and then hops, lobs stones | Bog |
| Pelican | 2 | 200 | flies fast from right to left and drops a bomb when she's below | Manta |
| Bomb | — | — | bursts into eight shards | Bomb |
| Jumping jar | 2 | 200 | small hops toward her; 16 hops and it becomes a crown | Karakasa |
| Firecracker | 8 | 200 on the spot | lights a short fuse when she's near, then explodes | Shroom |
| Pufferfish | 1 | 200 | surfaces and puffs up to three bubbles that drift after her | White fish |
| Racing pigeon | 2 | 200 | a pigeon, much faster | Skips |
| Spider | 1 | 100 | hangs under a ledge, bobbing up and down at random | Octospider |
| Magpie | 1 | 200 | locks onto her and dives in a straight line | Ghost |
| Flasher | 2 | 300 | shoots one fast beam across the whole screen | Warps |
| Laundry sheet (loop 2) | 2 | 100 | rises to her height, then sails across | Rag |
| Green snail (loop 2) | 2 | 200 | a tougher snail | Green snail |
| Dust moth (loop 2) | 2 | 200 | flutters about, shedding clouds of dust | Noil |

Loop 2 changes, as in [W]: crows turn purple, geckos spit two seeds, most
flying fish become pufferfish (not in the boss fight), spike lamps get three
spikes instead of two, and firecrackers light up from farther away with a
shorter fuse. The "purple lizard" and "more active shroom" variants aren't
described beyond that, so our versions are a purple crow and a jumpier
firecracker.

### Readings we had to choose

- **Starting lives:** 3. The sources don't give the number.
- **Spirit length:** 4 seconds ("a short delay").
- **Variable jump height** (letting go of A early cuts the jump short) is our
  reading of the controls.

## What is ours

- **Name:** ROOFCAT (1984, Beamdown Softworks).
- **Hero:** Harissa, a spicy courier cat. She throws jasmine stars.
- **Story:** the Magpie Mob snatched Grandma Zohra's birthday parcel, and
  Harissa chases it across a sunny Mediterranean seaside town to Old Crab's
  water.
- **Areas:** Whitewash Rooftops, the Spice Souk (with the roof garden), the
  Fort Walls at dusk, and the Harbour at Night (with the fish market).
- **Night Route:** the same town after dark.
- **Foes, pickups and the boss:** all our own designs, listed above. Snacks
  are dates, then figs, then glasses of mint tea. Lost letters stand in for
  sandwiches, paper lanterns for balloons and catnip for the glowing star.
- **Levels:** 48 hand-built screens plus the arena (`roofcat_levels.c`).
- **Music:** one original track per area (bright major, Hijaz-flavoured souk,
  driving fort, breezy harbour), a boss theme and jingles.

## Additions: none

Only the platform needs every UFO 40 cartridge has:

- the START pause menu;
- saving: each area reached is a checkpoint that CONTINUE resumes from (a
  game over erases it);
- the three UFO 40 goals, which replicate Ninpek's own three:

| UFO 40 goal | Condition | Ninpek's goal |
|---|---|---|
| Beacon | find 3 lost letters in a run | gift: eat three sandwiches |
| Saucer | deliver the parcel (beat the first loop) | gold: beat the game |
| Alien | clear the Night Route (beat the second loop) | cherry: beat the second loop |

Removed in the faithfulness audit: a boss at the end of every stage (Ninpek
has one), the stage-clear screens, the 15,000-point Beacon, foe types with no
Ninpek counterpart (the phasing mirage, the rat-spawning chimney), fruit worth
200-500 points, the faster second loop, and power-ups speeding up the throw.
The score for extra lives now follows 3,000 / 7,000 / +5,000.

## Also fixed in this pass

- **Vita shimmer.** The carried cat used to alternate between two screen
  columns every frame. Carried things now move by the camera's whole-pixel
  step, and the SDL loop runs exactly one update per 60 Hz vsync. Test:
  `rc_15_no_jitter`.
- Stars now really expire at their range (a timer bug kept them flying).
- Pigeons and magpies no longer face backwards.

## Controls

| Input | Action |
|---|---|
| D-pad left/right | hold ground / run ahead |
| A | jump, then press again in the air to double jump |
| Down + A | drop through a ledge |
| B | throw a jasmine star (hold to keep throwing) |
| Spirit mode: D-pad | float |
| Spirit mode: B | twin shots |
| Spirit mode: A | come back to life |
| START | pause |

## Sources

- [W] UFO 50 Wiki (Miraheze), "Ninpek": controls, power-ups, lives at 3,000
  and 7,000 then every 5,000, ghost mode, the full enemy table with hit points
  and egg colours, scoring items, the boss, loop 2 changes, goals.
  https://ufo50.miraheze.org/wiki/Ninpek
- [MG] MoeGamer, "UFO 50: Ninpek's epic quest for a sandwich": one continuous
  world with stages and bonus areas, eggs instead of points, sandwiches on
  hidden spots, the ghost floating down from the top, the glowing star twice
  per life. https://moegamer.net/2024/09/22/ufo-50-ninpeks-epic-quest-for-a-sandwich/
- [MM] Steam guide "The missing manuals - How to play UFO 50 games": B attacks
  continuously when held; double jump.
- [WP] Wikipedia, "UFO 50": "takes under nine minutes to complete".
