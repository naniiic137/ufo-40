# 03 · ROOFCAT

*Internal design document. Not shown in the product.*

## Tribute to

**Ninpek** (UFO 50 game #3, Mossmouth). The mechanics were researched from
text only: the community wiki, written reviews and Steam guides and threads
(listed under Sources). No UFO 50 images, video, sprites, music, stage
layouts or text were used as references.

**Map type: fixed.** Ninpek is one hand-made world, so ours is too
(`roofcat_levels.c`, all our own screens):

| Structure | Ninpek | ROOFCAT |
|---|---|---|
| World | one long continuous world in several stages [MG] | one town in four areas that run straight into each other |
| Length | about nine to ten minutes a loop [WP], [PC], [SH] | 52 screens: about nine minutes of scrolling, then the fight |
| Busy | "enemies come at you fast, in great numbers" [TE]; learn what spawns on each screen [SH] | 126 placed foes plus the ones that fly in |
| Bonus areas | two, a solid mass of cherries [W], [MG] | the roof garden (screens 14-15) and the fish market (43-44) |
| Mushrooms | about halfway [SH] | the firecrackers, screens 24-27 of 52 |
| Hidden sandwiches | several hidden spots [W], [MG] | five hidden letter spots |
| Balloons at places | some 1-up balloons at set spots [W], [MG] | four placed lanterns |
| Boss | one, at the end [W] | Old Crab, in the harbour |
| Second loop | the same world, harder [W] | the Night Route |

## Mechanics checklist

| Mechanic | ROOFCAT | Source |
|---|---|---|
| Auto-scroll | steady, to the right; stops at the boss | [W], [MG] |
| No input | she keeps her place on the screen and drifts off platforms | [MG], [SA] |
| Left | holds her ground (stands still in the town) | [SA], [SE] |
| Right | runs ahead of the scroll | [W] |
| Falling off the bottom | costs a life | [MM] |
| Jump | A; held longer, higher | [MM] |
| Double jump | A again in the air, once; also after walking off an edge | [W], [MM], [SC] |
| Drop through | down + A through a one-way ledge | [W], [SH] |
| Stars | B; hold to keep throwing; short range | [W], [MM], [SC] |
| One star at first | a new star only when the last is gone | [W] |
| Power-ups | every third defeated foe drops catnip while she holds fewer than two; each allows one more star on screen | [W], [MG] |
| Losing power-ups | back to one star on a lost life | [W], [SC] |
| One hit | any foe, shot, spike or cloud costs a life | [W], [ST] |
| Lives | three, as heads in the HUD | [ST], [MM] |
| Spirit | floats down from the top, flies anywhere through everything, can't be hurt, twin shots much slower than stars, can't pick anything up | [W], [MG], [MM] |
| Coming back | when its short time is up or with A, where the spirit is, with no invincibility | [W], [MM] |
| Game over | with no lives left; the score goes on the high-score board | [W] |
| Extra lives | lanterns at 3,000, 7,000, then every 5,000, plus four at set places; shoot one for a life | [W], [MG] |
| Points only on pickup | foes score nothing; they drop an egg worth 100, 200 or 300 by kind | [W], [MG] |
| On-the-spot points | spike lamps and firecrackers pay 200 when destroyed | [W] |
| Sandwiches | standing on a hidden spot calls down a letter worth 500 | [W], [MG] |
| Crowns | a jumping jar left alone for 16 hops becomes a crown worth 300 | [W] |
| Bonus snacks | 5 each; 100 eaten in a stretch turn the rest into 20s, 100 more into 100s | [W] |
| Boss | 35 hit points; only the eye takes damage; stand on the legs as they rise; bursts of green shots; flying fish and falling orbs | [W], [SC] |
| Loop 2 | the same town after dark, keeping score and lives, with the swaps below | [W], [SH] |
| Endings | the parcel comes home; after the second loop, a whole stack | [W] |
| Goals | Beacon: 3 letters in a run; Saucer: beat loop 1; Alien: beat loop 2 | [W] |
| High scores and records | a board of five; most foes defeated, most letters found | [W] |
| No mid-run save | every run starts from the first rooftop | [SV] |
| "He is no ninja" | a game over in a bonus stretch says "SHE IS NO ROOFCAT." | [W] |

### The foes

Every Ninpek foe [W] has a counterpart with the same hit points, egg and
behaviour. Names and art are ours.

| Ours | HP | Egg | Behaviour | Ninpek |
|---|---|---|---|---|
| Pigeon | 2 | 100 | walks its platform; now and then rolls, and while rolling only a star from behind hurts it | Chops [W], [SC] |
| Racing pigeon | 2 | 200 | a much faster pigeon | Skips |
| Crow | 4 | 200 | pops out of a roof chimney, faces her, fires two evenly spaced pebbles shortly before it ducks back | Snake |
| Gecko | 1 | 100 | spits a seed that bounces along the roofs | Slime |
| Gull | 2 | 100 | floats slowly in from the right edge, bobbing up and down | Floats |
| Spike lamp | 10 | 200 on the spot | still or swinging, with two spikes circling | Mine |
| Flying fish | 1 | 200 | waits until its spot is on screen and she is near, leaps in a fixed arc, hangs at the top | Fish (yellow) [W], [SC] |
| Wasp | 1 | 100 | comes in from the left edge, heads right, eases to her height while behind her | Bee |
| Snail | 1 | 100 | crawls along a platform, or along its underside | Snail (red) |
| Toad | 4 | 300 | jumps now and then (onto other platforms too), lobs stones in an arc | Bog |
| Pelican | 2 | 200 | flies fast from the right; drops a bomb when she is below | Manta |
| Bomb | – | – | bursts into eight shards | Bomb |
| Jumping jar | 2 | 200 | hops toward her (from the roofs or out of the water); dives when she is right under it; 16 hops and it is a crown; four at most; hops away while she is a spirit | Karakasa [W], [SC] |
| Firecracker | 8 | 200 on the spot | lights a short fuse when she is near, then leaves a choking cloud | Shroom |
| Spider | 1 | 100 | hangs under a ledge, going up and down at random | Octospider |
| Magpie | 1 | 200 | appears out of nowhere, flies straight at her, bursts into two shards at the edge | Ghost |
| Flasher | 2 | 300 | phases in, fires one fast beam across the screen toward her, phases out and turns up again | Warps [W], [SC] |
| Orb (boss) | – | – | falls slowly until it hits something, then splits down-left and down-right | Orb |
| Pufferfish | 1 | 200 | a flying fish that fires up to three bubbles at where she is; stars pop them | White fish |
| Laundry (loop 2) | 2 | 100 | rises out of a crow's chimney to her height, then sails at her, bobbing | Rag |
| Green snail (loop 2) | 2 | 200 | a tougher snail | Green snail |
| Dust moth (loop 2) | 2 | 200 | flies about its spot, shedding spore clouds | Noil |

Loop 2 [W]: crow chimneys hold purple crows or laundry, geckos spit a second
seed, spike lamps get three spikes, snails turn green, most flying fish
become pufferfish (not in the boss fight), and firecrackers are either
livelier or dust moths.

### Readings we had to choose

- Numbers: scroll, run and jump speeds, the spirit's four seconds and when
  foes fire are ours; no source gives them.
- Holding LEFT stands her exactly still ("hold left", "tap left" [SA], [SE]).
- The screen edge squeezing her against a wall costs a life.
- The Beacon's three letters must come in one run.
- Loop 2 crows: the wiki swaps snakes for both purple lizards and rags, so
  the chimneys take turns.
- The board keeps five scores, without initials. A run ends after loop 2.

Not included: two-player co-op (the Vita has one controller).

## What is ours

- **Name:** ROOFCAT (1984, Beamdown Softworks).
- **Hero:** Pepper, a quick courier cat who throws tin stars.
- **Story:** the Magpie Mob snatched Grandma Rosa's birthday parcel, and
  Pepper chases it across a whitewashed seaside town to Old Crab's water.
- **Areas:** Whitewash Rooftops, the Spice Market (with the roof garden),
  the Fort Walls at dusk and the Harbour at Night (with the fish market).
- **Foes, pickups and the boss:** listed above. Eggs are fish tokens; the
  snacks are dates, then figs, then glasses of tea; letters stand in for
  sandwiches, paper lanterns for balloons, catnip for the glowing star.
- **Levels:** 52 screens plus the arena, all drawn for this game.
- **Music:** one original track per area, a boss theme and jingles.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu, the high-score
save, and the three UFO 40 goals, which replicate Ninpek's own (gift, gold,
cherry). The previous version's checkpoint/continue, its 5,000-point boss
bonus and its loose snacks outside the bonus areas were removed.

## Controls

| Input | Action |
|---|---|
| D-pad right | run ahead |
| D-pad left | hold your ground |
| A | jump (hold for higher); again in the air to double jump |
| Down + A | drop through a ledge |
| B | throw a star (hold to keep throwing) |
| Spirit: D-pad / B / A | fly / twin shots / come back |
| START | pause |

## Tests

`tests/rc_*.ufs`, 23 scripts. `rc_23_first_section` runs the demo player
(a simple, sensible player in `roofcat.c` that chooses the buttons each
frame) through the whole first area with real button presses.

## Sources

- [W] UFO 50 Wiki (Miraheze), "Ninpek": controls, shurikens and power-ups,
  lives from score, ghost mode, the enemy table (hit points, eggs,
  behaviour, loop 2), scoring items, bonus areas, the boss, goals, stats,
  "HE IS NO NINJA". https://ufo50.miraheze.org/wiki/Ninpek
- [MG] MoeGamer, "UFO 50: Ninpek's epic quest for a sandwich": the fixed
  sprite and the scroll, one continuous world with stages and bonus areas,
  eggs, sandwiches on hidden spots, balloons, the ghost.
  https://moegamer.net/2024/09/22/ufo-50-ninpeks-epic-quest-for-a-sandwich/
- [MM] Steam guide "The missing manuals - How to play UFO 50 games": hold A
  to jump higher, two jumps, hold B, head icons for extra lives, the ghost.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [SC] Steam guide "Ninpek Cherry Clear Guide": double jump after falling
  off, power-ups reset on death, fish hang at the top, running under jars
  makes them dive, flashers fire once and teleport, the eye.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3336087646
- [SH] Steam thread "Is Ninpek too hard": learn each screen, drop through,
  the mushroom area about halfway, about ten minutes a run, loop 2.
  https://steamcommunity.com/app/1147860/discussions/0/4849904631719370097/
- [SA] Steam thread "Ninpek Auto-running": tapping and holding left.
  https://steamcommunity.com/app/1147860/discussions/0/595153277396484459/
- [SE] Search summaries quoting reviews: "holding position requires you to
  constantly tap left".
- [ST] Static Canvas, "The UFO 50 Diaries: Ninpek": three lives, hitboxes.
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-ninpek
- [TE] Torrential Equilibrium review (via the research notes): early foes
  come fast and in numbers.
- [WP] Wikipedia, "UFO 50": a nine-minute run. https://en.wikipedia.org/wiki/UFO_50
- [PC] popcar, "Reviewing every UFO 50 game": about ten minutes.
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [SV] Steam thread "Which games save progress?": Ninpek does not save mid-run.
  https://steamcommunity.com/app/1147860/discussions/0/4849904176633630339/
