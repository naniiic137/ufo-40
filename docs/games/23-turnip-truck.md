# 23 · TURNIP TRUCK

*Internal design document. Not shown in the product.*

## Tribute to

**Onion Delivery** (UFO 50 game #23, Mossmouth, "April 1986"). The rules
were researched from text only: the community wiki's raw and rendered
pages, the Steam "missing manuals", "Map and Tips" and "Gift, Gold &
Cherry" guides, Steam threads, written reviews and TV Tropes' recap (listed
under Sources; the full notes are in the research folder,
`23-onion-delivery.md`, and the independent review is
`ufo40-review/23-review.md`, whose 17 findings are fixed here). No UFO 50
images, video, sprites, music, maps or text were used as references, and
nothing was taken from a UFO 50 install.

**Map type: fixed.** Onion Delivery's city is one hand-made map, the same
every time; only the destinations, the order of the events and the rain's
puddles are random [MH]. So TURNIP TRUCK's city is hand-made too
(`tnp_city.c`), all our own streets, and the same rules are random.

| Full scale | Onion Delivery | TURNIP TRUCK |
|---|---|---|
| Players | 1 [MH], [MH-MP], [WIKI-EN] | 1 |
| Week | 7 workdays [MH] | 7 (Monday to Sunday) |
| Quota | 5 deliveries a day, then back to HQ [MH] | the same |
| Overtime | as many more as the clock allows, no time added [MH] | the same |
| Chaos events | 6, one a day on days 2 to 7, each once, random order [MH], [STATIC], [LIZ] | 6, the same way |
| City | one hand-made city that wraps in every direction [MH] | Blipton: 32 × 32 cells of 48 px, 1,536 px a side, wrapping both ways. That is 4.8 screens wide; tall, 8.5 of UFO 40's 180-px screens (7.1 at the 216-px height the review measured with) |
| Destinations | random named places [MH], [SEARCH-MANUALS] | 33 named places |
| Areas named on the dashboard | yes (Quartain Heights, Cheap Storage…) [MH-META], [SEARCH-MANUALS] | 9 districts |
| Time crates | "several", hanging from balloons, reached off ramps, often in dangerous spots [MH], [MAPTIPS] | 7, each over a hazard past a ramp |
| Drop zones ("+" circles) | some, near the slime [MH] | 9 |
| Hydrants, gas canisters, ramps, rocks, slime pools | yes; the hydrants at the road's edges [MH], [STEAM-FUN], [STEAM-BUG] | 10 hydrants (at the kerbs of crossings), 4 canister stacks, 7 ramps, 2 rocky yards, a canal, a channel, ponds and a harbour basin |
| Hearts | 3 [MH], [SEARCH-MANUALS] | 3 |
| Tries | a pool for the whole week: 3 [SEARCH-MANUALS], [MAPTIPS], "four failed attempts" [MH] | 3 spare (the 4th lost day ends the week) |
| Goals | 3 [MH], [GGC] | the same 3, in our own words |
| Stat | Daily Best [MH] | Daily Best (on the title and the newscast) |
| Codes | ROAD-RAGE (endless practice), GOOF-ZOOM (reverse steering flipped) [MH-CHEATS] | JOYRIDES and BACKWARD on the title's CODES page |
| Dashboard | the driver at the wheel, the clock, the day's counts, 3 hearts, a speedometer, the area name, the radar [SEARCH-MANUALS], [TVT] | the same, in our own art (below) |
| Music | Newscast, Day Job, Overtime; rain instead of music on the rainy day [BANDCAMP], [TVT] | 9 tunes of our own (below) |

## Structure

Title (START WORK, CODES; the demo after 12 s) → each morning the Channel 9
newscast, which names the day's event → the workday, starting at the depot
→ CLOCKED OUT (the day's count, the week so far, the daily best) or a lost
day (OUT OF TIME! / WRECKED!), which takes a try and starts the same day
again from its newscast → after Sunday the ending (a different one with 50
or more deliveries) and the credits. With no try left, YOU'RE FIRED! and
back to the title (Monday). Nothing of a week is saved; the cartridge keeps
its records.

## The three goals

| Goal | Onion Delivery [MH], [GGC] | TURNIP TRUCK | Test |
|---|---|---|---|
| Beacon (gift) | clear 2 days ("Beat 2 Rounds") | CLEAR TWO WORKDAYS: clocking out on Tuesday | tnp_14 |
| Saucer (gold) | clear the 7th day ("Beat 7 Rounds") | CLOCK OUT ON SUNDAY | tnp_14, tnp_28 |
| Alien (cherry) | 50+ deliveries in one week | DELIVER 50 TURNIPS IN ONE WEEK: checked at the ending, over the cleared days | tnp_14, tnp_28 |

## The dashboard

Down the left of the screen, as in the original [SEARCH-MANUALS], [TVT]:
Zib at the wheel (the wheel turns as you steer; he looks shocked for half a
second when the truck is hit, holds up a turnip for each delivery, and when
the truck is wrecked or runs out of time the cab burns and the picture cuts
to static), the day, the clock (red and ticking in the last 10 s), the
hearts, a speedometer beside them (lit green at healing speed, with a mark
there, and a bar under it filling towards the next heart), the tries, the
day's deliveries, the week so far, the radar (the delivery in red, the
depot in blue after the quota), where to go and the district's name.

## Mechanics checklist

| Mechanic | How TURNIP TRUCK does it | Source | Test |
|---|---|---|---|
| Gas | hold A (Button 2) | [MH], [SEARCH-MANUALS], [STEAM-CTRL] | tnp_04 |
| Brake, then reverse | hold B (Button 1): a weak brake, about a second from top speed to a stop; once stopped it reverses "at a much slower speed" (0.9 against 2.6) | [MH], [SEARCH-MANUALS], [STEAM-CTRL], [STEAM-FUN], [STEAM-ONION] | tnp_04 |
| Car-relative steering, fixed camera | LEFT/RIGHT turn the truck to its own left and right; the camera never turns (north is up); held, the truck keeps turning | [MH], [SEARCH-MANUALS], [STEAM-CTRL], [ANI] | tnp_03 |
| No turning from a stand | the turn grows with speed, up to full lock at 1.2 px a frame | [LIZ] | tnp_03 |
| Let go of the gas to turn sharper | with the gas down at speed the turn is 25 % wider | [LIZ], [MH], [MAPTIPS] | tnp_03 |
| Sidestep | a tapped turn (let go within 8 frames) puts the heading back and slides the truck to that side: about 10 px when moving, about 6 px from a stand | [MH], [SEARCH-MANUALS], [STEAM-FUN] | tnp_05 |
| Powerslide / U-turn | A+B with a turn at speed: the truck swings round twice as fast while it carries on the old way (a U-turn "without moving much") | [MH], [SEARCH-MANUALS], [LIZ], [MAPTIPS] | tnp_06 |
| Turning on the spot | A+B with a turn while (nearly) stopped turns it on the spot | [STEAM-FUN], [RESET-44] | tnp_06 |
| Spin-out and the spin attack | a slide held over 45 frames (about a full turn) spins out, sliding straight on; after a 40-frame wind-up, for 40 frames it smashes any car it touches and takes no harm; the pad nudges the spin (0.02 rad a frame) so you can choose where it ends | [MH], [STEAM-SPIN], [STEAM-FUN], [STEAM-ONION] | tnp_06 |
| Grip | low: sideways speed fades by 14 % a frame (1.5 % in a slide) | [LIZ], [MH] | tnp_03, tnp_05 |
| Walls | never hurt; a hit at speed bounces the truck and swings it round (a hard hit at top speed turns it 90° or more) | [MH], [STATIC] | tnp_07 |
| Hearts | 3, full at the start of every day (and every try) | [MH], [SEARCH-MANUALS] | tnp_02 |
| What costs a heart | crashing into a town car, a fall into the brine, gas canisters (they explode), the beet, bullets and bombs | [MH], [SEARCH-MANUALS] | tnp_11, tnp_16, tnp_18, tnp_20 |
| Brine and drop zones | a fall costs a heart and puts the truck back at the nearest drop zone (a "+" circle) | [MH] | tnp_11 |
| Speed heals | 2.5 s at 92 % of top speed or more, unhurt, brings back one heart, with a sound; the speedometer shows when you are fast enough; walls, rocks and slowing reset it | [MH], [SEARCH-MANUALS], [MAPTIPS], [LIZ], [POPCAR] | tnp_08 |
| A delivery heals | full hearts | [MH], [SEARCH-MANUALS] | tnp_08 |
| No hearts left | the truck is wrecked and the day is lost | [MH] | tnp_11 |
| The clock | one countdown, always on screen, red with a ticking tone at 10 s or less | [MH], [SEARCH-MANUALS] | tnp_09 |
| Time added | deliveries 1 to 4 +8 s, the 5th +30 s, overtime +0; a crate +12 s | [MH] | tnp_09 |
| 00 | the truck catches fire and explodes; the day is lost | [MH], [TVT] | tnp_10 |
| Time crates | hang from balloons over a pool, a canister stack, a rocky yard or the canal, just past a ramp; only a truck in the air breaks one; all back for every try | [MH], [MAPTIPS], [STEAM-SPIN] | tnp_09, tnp_31 |
| Deliveries | the dashboard names the place and the radar shows a red dot; a red arrow shows for a moment after each new one and again when it is close; drive into the red circle; the next appears at once | [MH], [SEARCH-MANUALS], [LIZ] | tnp_02, tnp_09 |
| Random destinations | any of the 33 but the last one, not within 160 px of the truck | [MH], [STEAM-ONION] | tnp_09 |
| The quota and the depot | after the 5th, a blue dot for the depot on the radar and the overtime tune; clock out in the depot's half circle (radius 11 against 16 for a delivery, only on the street side of the door) | [MH], [SEARCH-MANUALS], [STEAM-NITPICK] | tnp_12 |
| Back before 00 | the quota is not enough: the truck must reach the depot in time | [MH] | tnp_10, tnp_12 |
| Tries | a pool for the week; a lost day uses one and starts that day again; its deliveries don't count; losing with none left ends the week, back to Monday | [MH], [SEARCH-MANUALS], [MAPTIPS], [STEAM-ONION2] | tnp_13 |
| The newscast | every morning, naming the day's event | [SEARCH-MANUALS], [LIZ], [BANDCAMP] | tnp_02 |
| The dashboard | see above | [SEARCH-MANUALS], [TVT], [MAPTIPS] | tnp_27 (layout), (drawn) |
| The wrapping city | drive off any edge and you are on the other side | [MH] | tnp_24 |
| Dead ends, places off the main roads, areas with one way in | cul-de-sacs, alleys, the dead-end plaza, the fenced lot (one gate), streets that end in the canal | [MH], [STEAM-FUN] | tnp_24 |
| Bridge warnings | BRIDGE painted on the road before each bridge | [MH] | (drawn) |
| Ramps | at speed (1.6 px a frame or more, the ramp's way) the truck flies over brine, canisters, rocks and the canal; too slow, it rolls off the end | [MH] | tnp_22, tnp_31 |
| Hydrants | at the kerb, on a corner of their crossing: the kerb lane is not safe there; they knock the truck about like a wall | [MH], [STEAM-FUN], [STEAM-BUG] | tnp_15 |
| Gas canisters | a heart, and the stack explodes (gone for the rest of the try) | [MH] | tnp_11 |
| Rocks | harmless, very slow (arriving fast carries you further) | [MH] | tnp_22 |
| Traffic | right-hand lanes; cars never appear ahead of a moving truck going its way; they slow down or pull over for the truck but never leave the road; they turn round where a road ends; far-off cars are removed | [MH], [STEAM-ONION], [STEAM-FUN] | tnp_21 |
| Limited view | about 2.7 × 1.9 cells each way of the truck, shifted up to 36 × 24 px ahead of where it is going | [STEAM-FUN] | (drawn) |
| Day 1 | no event | [MH], [STATIC] | tnp_02 |
| Events | Tuesday to Sunday, each of the six once, in an order shuffled every week | [MH], [STATIC], [LIZ] | tnp_02 |
| Brine Burst (Sludge Overflow) | the hydrants spray brine across the road from the kerb, in turn, and push the truck; waves of brine sweep across three roads towards the brine beside them (the works' street into its channel, the dock road into the harbour basin, the riverside into the canal): a truck near the downstream edge is swept in, one by the upstream kerb only slides; no damage by itself | [MH] | tnp_15 |
| The Big Beet (Onion Monster) | charges at the truck when it sees it (faster than the truck, in bursts) and rolls along the streets after it when it doesn't; a touch is a heart; it gathers itself for about the truck's safety window and comes again, so a truck it corners is wrecked in about three seconds; a looping dash sound | [MH], [STEAM-AUDIO] | tnp_16 |
| Mush Mob (Zombies) | hordes of 3 to 12, with empty streets between; they never hurt; running one over slows the truck a lot and drags it onto the body (a jittery crawl through a crowd) | [MH], [STEAM-BUG], [STEAM-ZOMBIE] | tnp_17 |
| Radish Ring (Garlic Gang) | one or two chases at a time roam the whole town (gang cars fleeing police cars), a new one every 15 to 30 s and none for the first 5 to 10 s, so there are long spells with no gang about; a gang car that sees the truck within 100 px chases it for 3 s after losing sight, and shoots, badly; ramming their cars costs nothing | [MH], [LIZ], [TVT] | tnp_18 |
| Downpour (Severe Weather) | rain all day and only the rain to hear (the overtime tune still comes in); puddles appear in an instant (9 frames), a third of them on the road just ahead of a moving truck, and spin it out when hit fast; reversing through them is safe; the pad nudges the spin; a little harder to see | [MH], [STEAM-ONION], [STEAM-FUN], [TVT], [POPCAR] | tnp_19 |
| Moon Raid (Alien Invasion) | saucers of three sizes fly erratically and bomb where the truck is when they come near; the big one fires slowly | [MH] | tnp_20 |
| Saving | none for a week; records only | [STEAM-SAVES], [CONV] | tnp_14 |
| Goals | see above | [MH], [GGC] | tnp_14 |
| The cherry's ending | early retirement | [MH], [LIZ] | tnp_14 |
| Practice code | JOYRIDES: no clock, no damage, no quota, no events, no end | [MH-CHEATS] | tnp_01 |
| Reverse code | BACKWARD: reverse steering turned round | [MH-CHEATS], [MH] | tnp_26 |
| Codes stop saving and goals | while a code is on | [MH-CHEATS], [CONV] | tnp_01 |
| The meta message | wreck the truck in the fenced lot and the next newscast has a word for you (our own words) | [MH], [MH-META] | tnp_23 |
| The demo | left alone on the title, the demo player drives a day | [STEAM-DEMO] | tnp_25 |

### Readings we had to choose

- **Tries (settled):** 3 spare tries, shown on the dashboard; the 4th lost
  day ends the week. Four direct texts agree: the manual's "You begin with 3
  'tries'", the Map and Tips guide's "only three continues", the wiki's
  "maximum of four failed attempts" and "back to the very beginning"
  [STEAM-ONION2]. Static Canvas's "three strikes" is the outlier.
- **Turning on the spot (settled):** A+B with a turn while stopped or slow
  turns the truck on the spot: "can also be done with no speed", "if you
  want to do tight turns while moving slowly you have to hold both"
  [STEAM-FUN].
- **Spin-out timing (settled):** after 45 frames of sliding (about a full
  turn, Tyrant's "full 360"), the spin attack about 1.4 s after the slide
  begins ("takes a second to activate") [STEAM-FUN], [STEAM-SPIN].
- **The day's clock:** 45 s every morning (no source gives it). Tuned so a
  steady driver makes the quota with room to spare and a cherry-ready one
  banks about 10 on Monday, the community's yardstick [STEAM-DEATH]; the
  demo player averages 7 to 8 a day. Popcar mentions "how little time you
  get in later stages", which could mean later days start with less; too
  weak to act on.
- **Reverse steering:** by default like a real steering wheel (RIGHT swings
  the nose to the left), and BACKWARD turns that round. Only the code's
  effect is written down; its name ("GOOF") suggests the realistic default
  is the normal one.
- **What counts as "hitting another vehicle":** a town car meeting the
  truck at under 0.6 px a frame only nudges it, without harm (a stuck truck
  would otherwise lose every heart against a waiting car). A car kept
  waiting by the truck for over a second creeps on and edges it aside. The
  gang's and police cars never hurt by contact: "neither type of vehicle
  causes damage directly" [MH].
- **The wall swing** (low confidence): a hit turns the truck by 0.095 rad
  for each px a frame of impact, fading over a few frames, so a hard hit at
  top speed swings it 90 to 180° ("will spin the car around" [MH]; "trying
  to realign yourself after this took an era and a half" [STATIC]).
- **Numbers no source gives:** top speed 2.6 px a frame (156 px a second),
  0.055 to full speed, the brake 0.045 a frame, a turn of 0.055 rad a frame
  at full lock, grip 0.86, the sidestep's 1.4 (0.8 at rest), a slide twice
  as quick, the spin-out's 100 frames, 60 frames of safety after a hit, a
  heart back after 2.5 s at 92 % of top speed, ramps from 1.6 px a frame,
  rocks keeping 95.5 % of the speed a frame.
- **Crates:** 7, all over hazards past ramps (the wiki says "several").
  Each needs close to top speed off its ramp; the two over the canal need
  the most.
- **The events' numbers:** the hydrants spray 56 px across the road for
  100 of every 240 frames each way, pushing 0.12; a wave crosses its road
  every 4 s at 1.6 px a frame, a 20-px band pushing 0.1. The beet charges at
  3 px a frame for 32 frames, rests 80 (68 after a hit), sees 170 px, rolls
  at 1.6. Up to 36 mushmen in hordes of 3 to 12, 0.35 px a frame; squashing
  one keeps 55 % of the truck's speed and pulls it 40 % of the way onto
  the body. One or two chases; a chase lasts 30 to 60 s, then the pair
  leaves when out of sight; gang cars chase within 100 px in clear sight,
  shoot within 84 px every 90 frames in a 70° cone. 28 puddles round the
  truck, a spin-out above 1.5 px a frame. 1 big, 2 middle and 5 small
  saucers, bombs on the truck's spot ± 18 px after 50 frames, every 80 /
  100 / 160 frames by size.
- **Destinations:** uniform among the 33, but never the last one and never
  within 160 px of the truck.
- **The city's size and layout** are ours (no map was read); its kinds of
  place follow the wiki's description: blocks, parks, an industrial area with
  pools and ramps, dead ends, one-way entries, bridges.
- **Rain and regeneration:** the rain doesn't stop hearts coming back by
  itself; it is hard to hold top speed among the puddles, which is what the
  players describe.
- **Days 2 to 7** keep their event when a day is tried again.
- **Left out on purpose:** the original's bugs (the wall-clip deaths after
  a zombie or hydrant bounce) [STEAM-BUG].

## What is ours

- **Name:** TURNIP TRUCK (1986, Beamdown Softworks).
- **People:** Zib, the one-eyed driver; Granny Root, who owns the turnip
  depot; Glenda Glorp, the Channel 9 anchor.
- **The city:** Blipton and its nine districts (Old Blipton, Depot Row,
  Saucer Heights, Westgreen, Downtown, the Canal, the Pickle Works, the
  Market, the Docks), all 33 places from the Gloop Diner to the Space
  Clinic, and the whole layout.
- **The chaos:** Brine Burst, the Big Beet, the Mush Mob, the Radish Ring,
  the Downpour and the Moon Raid, with their newscasts.
- **The dashboard's art:** Zib at the wheel, his faces, the speedometer.
- **Music:** "Turnip Truck" (title), "News at Nine" (newscast), "On the
  Clock" (the day), "After Hours" (overtime), "Rain on the Roof", "Clocked
  Out", "Written Off", "Pink Slip" and "Sunday Off" (ending).
- **Words:** the newscasts, the fenced lot's message, the endings, the codes
  (JOYRIDES, BACKWARD), the goal lines and every label.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
UFO 40 goals, which are Onion Delivery's own. UFO 40 has no terminal, so the
two codes sit on the title's CODES page, as in other UFO 40 cartridges.
The Miasma Tower riddle ("park at the park") belongs to the collection's
hidden 51st game, so it is left out (the park is still below the depot).

## Controls

| Input | Action |
|---|---|
| Hold A | gas |
| Hold B | brake (weakly); once stopped, reverse |
| LEFT / RIGHT held | turn the truck to its own left / right |
| LEFT / RIGHT tapped | sidestep (also from a stand) |
| A + B + a turn | powerslide (held too long: a spin-out) |
| A + B + a turn, stopped or slow | turn on the spot |
| LEFT / RIGHT in a spin-out | nudge the spin |
| UP / DOWN | nothing |
| START | pause |

### Not confirmed

| Control or rule | Our reading | Why it is unconfirmed |
|---|---|---|
| Reverse steering's default | like a steering wheel; BACKWARD flips it | only the code's effect is written down |
| Tap length for a sidestep | 8 frames | "tapped" is not timed anywhere |
| UP / DOWN | do nothing | no source mentions them |
| Gentle bumps with town cars | harmless under 0.6 px a frame | sources only say "hitting another vehicle" |
| The wall swing | up to 90–180° on a hard hit | low confidence: "spin the car around" is all |
| Steering in a spin | 0.02 rad a frame | one player's "control the rotation a little" |
| Starting time | 45 s every day | not documented |

(Settled since the first version and no longer in this table: the 3 spare
tries, turning on the spot with A+B, and the spin-out's timing.)

## Tests

`tests/tnp_01` … `tnp_31` drive the rules with button presses, or set up a
moment with cheats and then play it. The demo player (`tnp_bot.c`) plans
its route over the city's cells (round the beet while it is about), keeps
to the right-hand side of the road, never cuts a corner, eases off early
for corners, puddles and cars (the brakes are weak), steers round cars and
backs out when stuck, turns on the spot by the brine, stays slow enough in
the rain not to spin, and weighs up each overtime delivery against the
clock and the trip home. With real presses it clears each kind of day
(`tnp_30`) and two whole weeks from the title through every newscast and
screen to the ending, with over 50 deliveries (`tnp_28`, `tnp_29`).
`tnp_31` flies off all seven ramps through all seven crates; `tnp_24`
checks the city as written.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Onion Delivery" (rendered and raw):
  controls, hit points, the timer's values, crates, the map, hazards, the
  six events (zombies "do not cause damage"; gang and police cars do none
  "directly"), records, codes, the meta message.
  https://ufo50.miraheze.org/wiki/Onion_Delivery
- [MH-CHEATS] Miraheze, "Cheats": GOOF-ZOOM, ROAD-RAGE.
  https://ufo50.miraheze.org/wiki/Cheats
- [MH-META] Miraheze, "Meta Messages". https://ufo50.miraheze.org/wiki/Meta_Messages
- [MH-MP] Miraheze, "Multiplayer" (not listed). https://ufo50.miraheze.org/wiki/Multiplayer
- [SEARCH-MANUALS] Steam guide "The missing manuals" (read directly):
  A accelerate, B brake/reverse, tap to jink, A+B while turning to
  powerslide, the red arrow, red and blue circles, the left-hand panel with
  the driver at the wheel, the timer turning red with a tone, two counters,
  3 hearts and the speedometer to their right, the area name and the radar,
  regeneration, full health on delivery, "You begin with 3 'tries'".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [MAPTIPS] Steam guide "Onion Delivery Map and Tips" (read directly by
  the review): "only three continues", "boxes suspended by balloons …
  obtainable by driving into them by hitting a ramp", healing speed "noted
  by your HUD on the left", drifting for 180s, letting go of the gas to
  turn. https://steamcommunity.com/sharedfiles/filedetails/?id=3345275590
- [GGC] Steam guide "Gift, Gold & Cherry" (read directly): "Beat 2 Rounds /
  Beat 7 Rounds / Make 50+ Deliveries in 1 Week".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [TVT] TV Tropes, the Onion Delivery recap (read directly): the driver's
  face turning the wheel and shocked on a crash, the cab in flames and the
  static, the car exploding at 0, rain instead of music, the gang and the
  police careening about.
  https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game23OnionDelivery
- [STEAM-CTRL] "Onion Delivery Controls": Z accelerate, X brake and reverse.
  https://steamcommunity.com/app/1147860/discussions/0/4700161534027487209/
- [STEAM-ONION] "Onion delivery": random distances, regeneration at top
  speed, balloon crates, rain spawning puddles right ahead, steering a
  puddle spin, reversing through puddles, the useless brake.
  https://steamcommunity.com/app/1147860/discussions/0/4849903998511983608/
- [STEAM-ONION2] "Onion Delivery": limited continues, back to the beginning.
  https://steamcommunity.com/app/1147860/discussions/0/4700161643034710787/
- [STEAM-FUN] "Fun when it wants to be": turning, the spin attack after a
  full 360, U-turns with no speed, strafing at 0 speed, the hydrants at the
  road's edge, "the brakes barely function".
  https://steamcommunity.com/app/1147860/discussions/0/4852155320351838128/
- [STEAM-BUG] "Onion Delivery Instant Death Bug": zombies deal no damage;
  the wall-clip deaths; hydrants on either side of the one-block alleys.
  https://steamcommunity.com/app/1147860/discussions/1/4700161643034744658/
- [STEAM-SPIN] the spin attack; jumps for time bonuses.
  https://steamcommunity.com/app/1147860/discussions/0/4849903998513660096/
- [STEAM-ZOMBIE] https://steamcommunity.com/app/1147860/discussions/0/6757179594730160228/
- [STEAM-NITPICK] the smaller HQ circle.
  https://steamcommunity.com/app/1147860/discussions/0/4700161870965816157/
- [STEAM-AUDIO] the monster's dash sound.
  https://steamcommunity.com/app/1147860/discussions/1/4852155152092271634/
- [STEAM-DEMO] the attract demo.
  https://steamcommunity.com/app/1147860/discussions/0/592888028698036906/
- [STEAM-DEATH] 10–12 deliveries on day 1 before trying the cherry.
  https://steamcommunity.com/app/1147860/discussions/0/581680338186580567/
- [STEAM-SAVES] "Which games save progress?".
  https://steamcommunity.com/app/1147860/discussions/0/4849904176633630339/
- [LIZ] Lizstar's review (the gang coming after you).
  https://lizstar64.github.io/reviews/2024/10/15/UFO50-23.html
- [STATIC] Static Canvas, "The UFO 50 Diaries: Onion Delivery".
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-onion-delivery
- [POPCAR] Popcar ("puddles everywhere", "how little time you get in later
  stages"). https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [ANI] https://anigamers.com/posts/ufo-50-mini-reviews-every-game/
- [RESET-44] https://www.resetera.com/threads/ufo-50-ot-50-games-for-the-price-of-1.981834/page-44
- [WIKI-EN] https://en.wikipedia.org/wiki/UFO_50
- [BANDCAMP] https://phlogiston.bandcamp.com/album/ufo-50
- [CONV] `00-ufo50-conventions.md` (research folder).
