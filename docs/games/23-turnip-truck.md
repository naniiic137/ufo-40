# 23 · TURNIP TRUCK

*Internal design document. Not shown in the product.*

## Tribute to

**Onion Delivery** (UFO 50 game #23, Mossmouth, "April 1986"). The rules
were researched from text only: the community wiki's raw and rendered
pages, the Steam "missing manuals" guide, Steam threads, written reviews
and search-engine summaries of pages that refused a direct fetch (listed
under Sources; the full notes are in the research folder,
`23-onion-delivery.md`). No UFO 50 images, video, sprites, music, maps or
text were used as references, and nothing was taken from a UFO 50 install.
A second pass (2026-10-06) re-read the wiki's raw page (timer values,
"four failed attempts", the six events, the goals) and the Steam thread on
limited continues; the "Map and Tips" guide still answered HTTP 429.

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
| City | one hand-made city that wraps in every direction [MH] | Blipton: 32 × 32 cells of 48 px (1,536 px a side, about 6 × 8.5 screens of the driving view), wrapping both ways |
| Destinations | random named places [MH], [SEARCH-MANUALS] | 33 named places |
| Areas named on the dashboard | yes (Quartain Heights, Cheap Storage…) [MH-META] | 9 districts |
| Time crates | several, at fixed spots [MH] | 12 |
| Drop zones ("+" circles) | some, near the slime [MH] | 9 |
| Pipes at crossings, gas canisters, ramps, rocks, slime pools | yes [MH] | 10 pipes, 4 canister stacks, 5 ramps, 2 rocky yards, a canal, ponds and a harbour basin |
| Hearts | 3 [MH], [SEARCH-MANUALS] | 3 |
| Tries | a pool for the whole week (see the readings) [MH], [SEARCH-MANUALS] | 3 spare (the 4th lost day ends the week) |
| Goals | 3 [MH], [SEARCH-GGC] | the same 3 |
| Stat | Daily Best [MH] | Daily Best (on the title and the newscast) |
| Codes | ROAD-RAGE (endless practice), GOOF-ZOOM (reverse steering flipped) [MH-CHEATS] | JOYRIDES and BACKWARD on the title's CODES page |
| Music | Newscast, Day Job, Overtime; rain instead of music on the rainy day [BANDCAMP], [SEARCH-TVT] | 9 tunes of our own (below) |

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

| Goal | Onion Delivery [MH], [SEARCH-GGC], [LIZ] | TURNIP TRUCK | Test |
|---|---|---|---|
| Beacon (gift) | complete 2 days of deliveries | CLEAR TWO WORKDAYS: clocking out on Tuesday | tnp_14 |
| Saucer (gold) | survive a week on the job | SURVIVE A WEEK ON THE JOB: clocking out on Sunday | tnp_14, tnp_28 |
| Alien (cherry) | finish with 50+ deliveries | FINISH THE WEEK WITH 50 DELIVERIES: checked at the ending, over the cleared days | tnp_14, tnp_28 |

## Mechanics checklist

| Mechanic | How TURNIP TRUCK does it | Source | Test |
|---|---|---|---|
| Gas | hold A (Button 2) | [MH], [SEARCH-MANUALS], [STEAM-CTRL] | tnp_04 |
| Brake, then reverse | hold B (Button 1): brakes, and once stopped reverses "at a much slower speed" (0.9 against 2.6) | [MH], [SEARCH-MANUALS], [STEAM-CTRL] | tnp_04 |
| Car-relative steering, fixed camera | LEFT/RIGHT turn the truck to its own left and right; the camera never turns (north is up); held, the truck keeps turning | [MH], [SEARCH-MANUALS], [STEAM-CTRL], [ANI] | tnp_03 |
| No turning from a stand | the turn grows with speed, up to full lock at 1.2 px a frame | [LIZ] | tnp_03 |
| Let up on the gas for sharper turns | with the gas down at speed the turn is 25 % wider | [LIZ], [MH] | tnp_03 |
| Sidestep | a turn tapped (let go within 8 frames) at speed puts the heading back and slides the truck a few px to that side | [MH], [SEARCH-MANUALS] | tnp_05 |
| Powerslide / U-turn | A+B with a turn at speed: the truck swings round twice as fast while it carries on the old way (a U-turn "on a dime") | [MH], [SEARCH-MANUALS], [LIZ] | tnp_06 |
| Turning on the spot | A+B with a turn while (nearly) stopped turns it on the spot | [RESET-44], [STEAM-FUN] | tnp_06 |
| Spin-out and the spin attack | a slide held over 45 frames spins out of control, sliding straight on; after a 40-frame wind-up, for 40 frames it smashes any car it touches and takes no harm | [MH], [STEAM-SPIN], [STEAM-FUN] | tnp_06 |
| Grip | low: sideways speed fades by 14 % a frame (1.5 % in a slide) | [LIZ], [MH] | tnp_03, tnp_05 |
| Walls | never hurt; a hit at speed bounces the truck and spins it round | [MH], [STATIC] | tnp_07 |
| Hearts | 3, full at the start of every day (and every try) | [MH], [SEARCH-MANUALS] | tnp_02 |
| What costs a heart | crashing into a car, a fall into the brine, gas canisters (they explode), the chaos (the beet, bullets, bombs, a mob of mushmen) | [MH], [SEARCH-MANUALS] | tnp_11, tnp_16 to tnp_20 |
| Brine and drop zones | a fall costs a heart and puts the truck back at the nearest drop zone (a "+" circle) | [MH] | tnp_11 |
| Speed heals | 2.5 s at 92 % of top speed or more, unhurt, brings back one heart, with a sound and a filling bar on the dashboard; walls, rocks and slowing reset it | [MH], [SEARCH-MANUALS], [LIZ], [POPCAR] | tnp_08 |
| A delivery heals | full hearts | [MH], [SEARCH-MANUALS] | tnp_08 |
| No hearts left | the truck is wrecked and the day is lost | [MH] | tnp_11 |
| The clock | one countdown, always on screen, red when 10 s or less | [MH], [SEARCH-MANUALS] | tnp_09 |
| Time added | deliveries 1 to 4 +8 s, the 5th +30 s, overtime +0; a crate +12 s | [MH] | tnp_09 |
| 00 | the truck catches fire and explodes; the day is lost | [MH], [SEARCH-TVT] | tnp_10 |
| Deliveries | the dashboard names the place and the radar shows a red dot; a red arrow shows for a moment after each new one and again when it is close; drive into the red circle; the next appears at once | [MH], [SEARCH-MANUALS], [LIZ] | tnp_02, tnp_09 |
| Random destinations | any of the 33 but the last one, not within 160 px of the truck | [MH], [STEAM-ONION] | tnp_09 |
| The quota and the depot | after the 5th, a blue dot for the depot on the radar and the overtime tune; clock out in the depot's half circle (radius 11 against 16 for a delivery, only on the street side of the door) | [MH], [SEARCH-MANUALS], [STEAM-NITPICK] | tnp_12 |
| Back before 00 | the quota is not enough: the truck must reach the depot in time | [MH] | tnp_10, tnp_12 |
| Tries | a pool for the week; a lost day uses one and starts that day again; its deliveries don't count; losing with none left ends the week, back to Monday | [MH], [SEARCH-MANUALS], [STEAM-ONION2] | tnp_13 |
| The newscast | every morning, naming the day's event | [SEARCH-MANUALS], [LIZ], [BANDCAMP] | tnp_02 |
| The wrapping city | drive off any edge and you are on the other side | [MH] | tnp_24 |
| Dead ends, places off the main roads, areas with one way in | cul-de-sacs, alleys, the dead-end plaza, the fenced lot (one gate), streets that end in the canal | [MH], [STEAM-FUN] | tnp_24 |
| Bridge warnings | BRIDGE painted on the road before each bridge | [MH] | (drawn) |
| Ramps | at speed (1.6 px a frame or more, the ramp's way) the truck flies over brine, canisters and rocks; too slow, it rolls off the end | [MH] | tnp_22 |
| Pipes at crossings | knock the truck about like a wall | [MH] | (drawn), tnp_15 |
| Gas canisters | a heart, and the stack explodes (gone for the rest of the try) | [MH] | tnp_11 |
| Rocks | harmless, very slow (arriving fast carries you further) | [MH] | tnp_22 |
| Crates | float at fixed spots, broken even from the air, all back for every try | [MH], [STEAM-ONION] | tnp_09 |
| Traffic | right-hand lanes; cars never appear ahead of a moving truck going its way; they slow down or pull over for the truck but never leave the road; they turn round where a road ends | [MH], [STEAM-ONION], [STEAM-FUN] | tnp_21 |
| Limited view | the camera shows about 2.7 × 1.9 cells each way, a little ahead of where the truck is going | [STEAM-FUN] | (drawn) |
| Day 1 | no event | [MH], [STATIC] | tnp_02 |
| Events | Tuesday to Sunday, each of the six once, in an order shuffled every week | [MH], [STATIC], [LIZ] | tnp_02 |
| Brine Burst (Sludge Overflow) | the crossing pipes spray brine along the roads in turn and push the truck; waves roll along the Pickle Works' streets and push it; no damage | [MH] | tnp_15 |
| The Big Beet (Onion Monster) | charges at the truck when it sees it (faster than the truck, in bursts) and rolls along the streets after it when it doesn't; a touch is a heart; a looping dash sound | [MH], [STEAM-AUDIO] | tnp_16 |
| Mush Mob (Zombies) | crowds everywhere; running one over is harmless but slows the truck a lot; a slow truck they gather round loses hearts | [MH], [STEAM-ZOMBIE] | tnp_17 |
| Radish Ring (Garlic Gang) | black gang cars flee police cars all over town; close to the truck they shoot, badly | [MH], [SEARCH-TVT] | tnp_18 |
| Downpour (Severe Weather) | rain all day and only the rain to hear (the overtime tune still comes in); puddles appear at random on the roads and spin the truck out when hit fast; reversing through them is safe; a little harder to see | [MH], [STEAM-ONION], [STEAM-FUN], [SEARCH-TVT] | tnp_19 |
| Moon Raid (Alien Invasion) | saucers of three sizes fly erratically and bomb where the truck is when they come near; the big one fires slowly | [MH] | tnp_20 |
| Saving | none for a week; records only | [STEAM-SAVES], [CONV] | tnp_14 |
| Goals | see above | [MH], [SEARCH-GGC] | tnp_14 |
| The cherry's ending | early retirement | [MH], [LIZ] | tnp_14 |
| Practice code | JOYRIDES: no clock, no damage, no quota, no events, no end | [MH-CHEATS] | tnp_01 |
| Reverse code | BACKWARD: reverse steering turned round | [MH-CHEATS], [MH] | tnp_26 |
| Codes stop saving and goals | while a code is on | [MH-CHEATS], [CONV] | tnp_01 |
| The meta message | wreck the truck in the fenced lot and the next newscast has a word for you (our own words) | [MH], [MH-META] | tnp_23 |
| The demo | left alone on the title, the demo player drives a day | [STEAM-DEMO] | tnp_25 |

### Readings we had to choose

- **Tries:** 3 spare tries, shown on the dashboard; the 4th lost day ends
  the week. This is the wiki's "maximum of four failed attempts" and the
  manual's "You begin with 3 'tries'"; reviewers' "three strikes" is the
  other reading.
- **The day's clock:** 45 s every morning (no source gives it). Tuned so a
  steady driver makes the quota with room to spare and a cherry-ready one
  banks about 10 to 12 on Monday, the community's own yardstick [STEAM-DEATH];
  the demo player averages 7 to 8 a day.
- **Car-relative turning** is kept everywhere except reversing: by default
  reversing steers like a real steering wheel (RIGHT swings the nose to the
  left), and BACKWARD turns that round. Which of the two is the original's
  default is not written anywhere.
- **What counts as "hitting another vehicle":** cars meeting at under
  0.6 px a frame only nudge each other, without harm (a stuck truck would
  otherwise lose every heart against a waiting car). A car kept waiting by
  the truck for over a second creeps on and edges it aside.
- **Numbers no source gives:** top speed 2.6 px a frame (156 px a second),
  0.055 to full speed, a turn of 0.055 rad a frame at full lock, grip 0.86,
  a sidestep of about 10 px, a slide twice as quick, the 45-frame slide
  limit, the spin-out's 100 frames, 60 frames of safety after a hit, a
  heart back after 2.5 s at 92 % of top speed, ramps from 1.6 px a frame,
  rocks keeping 95.5 % of the speed a frame.
- **The events' numbers:** the beet charges at 3 px a frame for 32 frames,
  rests 80 (150 after a hit), sees 170 px, rolls at 1.6; up to 36 mushmen,
  0.35 px a frame, squashing one keeps 55 % of the truck's speed, three
  round a slow truck take a heart every 50 frames; three gang cars with
  their police, shots within 84 px every 90 frames in a 70° cone; 28
  puddles round the truck, a spin-out above 1.5 px a frame; 1 big, 2 middle
  and 5 small saucers, bombs on the truck's spot ± 18 px after 50 frames,
  every 80 / 100 / 160 frames by size.
- **Destinations:** uniform among the 33, but never the last one and never
  within 160 px of the truck.
- **The city's size and layout** are ours (no map was read); its kinds of
  place follow the wiki's description: blocks, parks, an industrial area with
  pools and ramps, dead ends, one-way entries, bridges.
- **Rain and regeneration:** the rain doesn't stop hearts coming back by
  itself; it is hard to hold top speed among the puddles, which is what the
  players describe.
- **Days 2 to 7** keep their event when a day is tried again.

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
| Hold B | brake; once stopped, reverse |
| LEFT / RIGHT held | turn the truck to its own left / right |
| LEFT / RIGHT tapped | sidestep |
| A + B + a turn | powerslide (held too long: a spin-out) |
| A + B + a turn, stopped | turn on the spot |
| UP / DOWN | nothing |
| START | pause |

### Not confirmed

| Control or rule | Our reading | Why it is unconfirmed |
|---|---|---|
| Reverse steering's default | like a steering wheel; BACKWARD flips it | only the code's effect is written down |
| Tap length for a sidestep | 8 frames | "tapped" is not timed anywhere |
| A+B turning at low speed | turns on the spot | one forum post and one thread describe it loosely |
| Spin-out timing | after 45 frames of sliding; the attack from 40 to 80 frames into it | "takes about a second" is the only number |
| UP / DOWN | do nothing | no source mentions them |
| Gentle bumps | harmless under 0.6 px a frame | sources only say "hitting another vehicle" |
| Starting time | 45 s every day | not documented |
| Tries | 3 spare, game over on the 4th loss | sources disagree (see the readings) |

## Tests

`tests/tnp_01` … `tnp_30` drive the rules with button presses, or set up a
moment with cheats and then play it. The demo player (`tnp_bot.c`) plans
its route over the city's cells, keeps to the right-hand side of the road,
never cuts a corner, slows for corners, puddles and cars, steers round
cars and backs out when stuck, turns on the spot by the brine, and weighs
up each overtime delivery against the clock and the trip home. With real
presses it clears each kind of day (`tnp_30`) and two whole weeks from the
title through every newscast and screen to the ending, with over 50
deliveries (`tnp_28`, `tnp_29`). `tnp_24` checks the city as written.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Onion Delivery" (rendered and raw):
  controls, hit points, the timer's values, crates, the map, hazards, the
  six events, records, codes, the meta message.
  https://ufo50.miraheze.org/wiki/Onion_Delivery
- [MH-CHEATS] Miraheze, "Cheats": GOOF-ZOOM, ROAD-RAGE.
  https://ufo50.miraheze.org/wiki/Cheats
- [MH-META] Miraheze, "Meta Messages". https://ufo50.miraheze.org/wiki/Meta_Messages
- [MH-MP] Miraheze, "Multiplayer" (not listed). https://ufo50.miraheze.org/wiki/Multiplayer
- [SEARCH-MANUALS] Steam guide "The missing manuals" (read directly):
  A accelerate, B brake/reverse, tap to jink, A+B while turning to
  powerslide, the red arrow, red and blue circles, the left-hand panel,
  3 hearts, regeneration, full health on delivery, "You begin with 3
  'tries'". https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [STEAM-CTRL] "Onion Delivery Controls": Z accelerate, X brake and reverse.
  https://steamcommunity.com/app/1147860/discussions/0/4700161534027487209/
- [STEAM-ONION] "Onion delivery": random distances, regeneration at top
  speed, balloon crates, rain, reversing through puddles.
  https://steamcommunity.com/app/1147860/discussions/0/4849903998511983608/
- [STEAM-ONION2] "Onion Delivery": limited continues, back to the beginning.
  https://steamcommunity.com/app/1147860/discussions/0/4700161643034710787/
- [STEAM-FUN] "Fun when it wants to be": turning, the spin attack, U-turns,
  the camera, rain and regeneration.
  https://steamcommunity.com/app/1147860/discussions/0/4852155320351838128/
- [STEAM-SPIN] the spin attack.
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
- [LIZ] Lizstar's review. https://lizstar64.github.io/reviews/2024/10/15/UFO50-23.html
- [STATIC] Static Canvas, "The UFO 50 Diaries: Onion Delivery".
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-onion-delivery
- [POPCAR] https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [ANI] https://anigamers.com/posts/ufo-50-mini-reviews-every-game/
- [RESET-44] https://www.resetera.com/threads/ufo-50-ot-50-games-for-the-price-of-1.981834/page-44
- [WIKI-EN] https://en.wikipedia.org/wiki/UFO_50
- [BANDCAMP] https://phlogiston.bandcamp.com/album/ufo-50
- [SEARCH-TVT] search-engine summaries of TV Tropes' recap (the car explodes
  at 0, rain replaces the music, the Garlic Gang).
- [SEARCH-GGC] search-engine summary of the Steam guide "Gift, Gold &
  Cherry". https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [CONV] `00-ufo50-conventions.md` (research folder).
