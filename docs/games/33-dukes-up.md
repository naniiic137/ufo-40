# 33 · DUKES UP

*Internal design document. Not shown in the product.*

## Tribute to

**Fist Hell** (UFO 50 game #33, Mossmouth, "August 1987"). The rules were
researched from text only: the community wiki's raw page, the Steam guides
"The missing manuals" (section 33), "Fist Hell Guide [Gold+Cherry]" (and its
comments) and "Gift, Gold & Cherry", Steam threads, written reviews and TV
Tropes (listed under Sources; the full notes are in the research folder,
`33-fist-hell.md`). No UFO 50 images, video, sprites, music, stage layouts or
text were used as references, and nothing was taken from a UFO 50 install.

**Map type: fixed.** Fist Hell's stages are hand-placed and "the same enemy
groups every run" [STATIC], so ours are too (`dku_nights.c`, all our own
streets, rooms and scripts). Only the structure follows the original:

| Full scale | Fist Hell | DUKES UP |
|---|---|---|
| Fighters | 4, stats 1-3: Jay 2 1 2 2, Cat 1 3 1 2, Victor 3 1 1 1, Amy 1 1 2 3 (POWER, RECOV, TOUGH, THROW) [MH] | 4 with the same four spreads: ROOK, PIP, MACK, DOLLY |
| Stages | 5 "Scares", fixed, left to right, each with a boss [MH], [GUIDE] | 5 nights, each with a boss |
| Areas | streets → club; street → train → arcade; woods → graveyard → ship; boardwalk → pier; beach → mansion → elevator → suite [GUIDE], [TVT] | Market Street → the roller rink; Canal Row → the night ferry → the waxworks; the old orchard → the graveyard → the visitors' ship; the promenade → the fishing jetty → its end; the dunes → the Grand Hotel → the service lift → the penthouse (15 sections) |
| Bosses | Charger, Pigman, two Aliens, giant squid, the Mayor in two phases [GUIDE], [TVT] | Big Ram (a rammer), the Boar Baron (a tusker), the two Visitors, the Undertow, Alderman Grist (two forms) |
| Enemy kinds | 12: zombie, boomer, chick, charger, werewolf, pigman, snail, alien, oni, clown, fishman, tentacle [MH], [GUIDE] | 12: shambler, torch-belly, crowmask, rammer, howler, tusker, sludger, visitor, bulwark, giggler, mudskipper, feeler |
| Elevator | 5 waves, a new one only after the last is beaten, then the final boss [GUIDE] | the service lift: 5 waves the same way, then Grist |
| Health | one bar, never refilled between stages, no lives [MANUALS], [LIZ] | the same: 100, carried from night to night |
| Continues | unlimited; restart the stage; cash and upgrades kept [MH], [STEAM-5H] | the same |
| Shop | after each stage: a full heal or +1 POWER, RECOV or TOUGH [MH], [MANUALS] | the corner shop, the same four, $5 and $20 (see Readings) |
| Food | fries, pizza, hot dog, burger: ~20 %, ~33 %, ~50 %, full; $2, $4, $6, $10 at full health [GUIDE] | apple, sandwich, drumstick, roast chicken: 20 %, 33 %, 50 %, full; $2, $4, $6, $10 |
| Cash | coins $1, dollars $5, gold belt $10 (from the UFO) [MH] | coins $1, a five $5, a gold ring $10 (from the saucer) |
| Weapons | board, chain, pipe, zombie leg; shotgun (6 shots); chainsaw; trash can, zombie head, axe, molotov thrown [MH] | plank, chain, pipe, ghoul arm; scattergun (6 shots); chainsaw; bin, ghoul head, cleaver, firebottle |
| Survival | a hidden gym (walk left at the start), endless waves, HOME and AWAY on the scoreboard, civilians every third wave [TVT], [GUIDE] | the Hornets' gym (the gym of SKID KIDS), the same; HOME and GUEST |
| Endings | 2: a continue → the bad one; none → the good one [TVT] | 2, split the same way |
| Goals | 3 [MH], [GGC] | the same 3 |
| Run length | about 25-30 minutes for a good run [POPCAR] | the demo player's runs: about 12-13 minutes of game time (it never dawdles) |

What each night brings: 1 the punch loop and spawn pacing, shamblers,
torch-bellies and crowmasks, a car that crashes across the street, the
first saucer, a burning barricade near the end, Big Ram among dancing
ghouls; 2 a rammer as an ordinary ghoul, a passer-by cut down by a cleaver
(his scattergun falls), sleeping crowmasks and ghouls on the ferry's
benches, howlers, the Boar Baron; 3 the hard one: mines, holes, a runaway
thresher that pushes the view along, ghouls clawing out of graves, rammers
in pairs, sludgers dropping from the trees, one hidden apple and two more
things under the grass tufts at the bottom of the orchard, a beam up into
the visitors' ship and the two Visitors; 4 bulwarks, gigglers, a tusker
again, a cache of crates, mudskippers out of the water, a narrow jetty,
feelers in the planks, the Undertow; 5 three visitors outside the hotel,
chandeliers, a tusker and a rammer, the five-wave lift and Alderman Grist.

## Mechanics checklist

| Mechanic | How DUKES UP does it | Source | Test |
|---|---|---|---|
| Walking | eight ways on the belt; 14/16 px a frame along the street, 10/16 up and down it | [MH], [MANUALS] | dku_02 |
| Run | double tap left or right (within 12 frames): 2 px a frame, steerable diagonally; stops when you let go or turn | [MH] | dku_02 |
| Dodge | double tap up or down: 24 px in 8 frames, then 10 frames of lag; no shield | [MH], [GUIDE] | dku_02 |
| Jump | B; in place or the way you're walking, farther when running; 4 frames of landing lag; not while charging; with something in hand B throws it instead | [MH], [MANUALS] | dku_02, dku_09 |
| Punch string | A: four jabs (POWER each), the fifth a kick (POWER + 1) that knocks down | [GUIDE], [MH] | dku_03 |
| Resetting the string | 18 frames without a punch, or 4 frames walking up or down, start it over | [GUIDE], [GUIDE-COMMENTS] | dku_03 |
| Stun | every blow stuns a ghoul for 28 frames; four, a breath, four more keeps it helpless, bosses too | [GUIDE] | dku_04 |
| Kill-kick | a jab (or a hit in a grab) that fells a ghoul always brings the kick, which floors everyone in front | [GUIDE] | dku_04 |
| Reach | a blow reaches 10 px up the street and only 3 px down it, for everyone: a little below a ghoul you hit it and it can't hit you | [MH], [GUIDE], [LIZ] | dku_04 |
| No shield | none on a dodge, the floor, the spin or getting up; standing blows hit the fallen, both ways | [GUIDE], [STEAM-NOTFUN], [GUIDE-COMMENTS] | dku_02, dku_05 |
| Spin | A + B at nearly any moment on the ground (even stunned, landing or mid-move); hits all round, throws whoever was behind out in front, sends shots back; costs 6 health only if it hits a ghoul | [MH], [GUIDE] | dku_05 |
| Charged punch | hold A (you can walk, not jump): charged after 44 / 32 / 20 frames by RECOV; let go to lunge and hit (4 + 2 x POWER), knocking down, sending shots back | [MH], [MANUALS], [GUIDE-COMMENTS] | dku_06 |
| Dash attack | A while running: POWER + 2, knocks down, long recovery (shorter with RECOV), sends shots back | [MH], [GUIDE] | dku_06 |
| Flying kick | A in the air: POWER + 3, knocks down, sends shots back | [MH], [GUIDE] | dku_06 |
| Grab | walking into a stunned ghoul grabs it; you can walk with it; it wriggles free after 50 + 30 x THROW frames | [MH], [MANUALS], [GUIDE] | dku_07 |
| In a grab | A hits (POWER), a direction and A throws (farther and quicker with THROW; the body floors whoever it meets; into a hole is the end of it), B jumps with it for a slam you can steer (4 + 2 x POWER, and floors those round) | [MH], [MANUALS], [GUIDE] | dku_07 |
| ROOK | hold B in a grab: a higher slam, 8 + 3 x POWER over a wider ring | [MH], [LIZ] | dku_08 |
| PIP | a flying kick that lands lets her jump again in mid-air, any way, and again | [MH], [TVT] | dku_08 |
| MACK | his charged punch (6 + 3 x POWER) bursts what it fells | [MH] | dku_08 |
| DOLLY | hold A in a grab: the ghoul she throws floors everything in its path and goes off where it lands | [MH], [GUIDE-COMMENTS] | dku_08 |
| Stats | POWER: every blow; RECOV: getting up (50 / 38 / 26 frames), charges, recovery after charged and dash attacks, weapon swings; TOUGH: 3 off every blow a level (1 off fire and blasts); THROW: throws, holds, readying a picked-up weapon | [MH], [GUIDE-COMMENTS] | dku_06, dku_07, dku_10 |
| Health | one bar of 100, yellow turning red; never refilled between nights | [MANUALS], [LIZ], [GUIDE] | dku_16 |
| Picking up | A on something underfoot picks it up (the punch button) | [MANUALS] | dku_09 |
| Food | apple 20 %, sandwich 33 %, drumstick 50 %, roast chicken all; at full health $2, $4, $6, $10 | [GUIDE], [MH] | dku_09 |
| Cash | coins $1, a five $5, a gold ring $10; no score anywhere | [MH], [STEAM-ALMOST] | dku_09 |
| Weapons | plank, chain (longer), pipe, ghoul arm: swung for 3-5 + POWER, knocking down; scattergun: 6 shots; chainsaw: fells a shambler at once, slow; bin, ghoul head, cleaver, firebottle thrown (the firebottle bursts into fire); A uses, B throws; ready after 26 - 8 x THROW frames | [MH], [GUIDE] | dku_09 |
| Carrying | a bin is too heavy to run with; a plank or chainsaw is dropped when you break into a run | [MH], [MANUALS] | dku_09 |
| Things on the floor | can be knocked aside by kicks, dashes and spins | [STEAM-ALMOST], [GUIDE] | (drawn) |
| Containers | bins, boxes, crates, stumps, signs, vases, junk, grass tufts | [MH], [GUIDE] | dku_10 |
| Holes | instant: a fighter (or a ghoul) whose feet go in is gone | [GUIDE], [STEAM-ALMOST] | dku_07, dku_10 |
| Hazards | the car (a horn, then it crosses its lane, flattening ghouls too), the burning barricade, mines, the runaway thresher, chandeliers that drop when you step into their shadow, fire, poison and gas clouds | [GUIDE], [TVT], [STEAM-ALMOST] | dku_10 |
| Spawning | by scroll position, whether or not the last lot are still up; some wait (eating, sitting, dancing, asleep) until you come near | [GUIDE], [STEAM-NOTFUN], [TVT] | dku_11 |
| Lock-screens | the view stops until nobody is left, then GO; the view never scrolls back | [MH], [STEAM-ALMOST] | dku_11 |
| Boss down | the night ends at once and everyone else goes home | [GUIDE] | dku_14 |
| Shambler | flanks to both sides, backs off a flying kick, may drop its head or an arm | [GUIDE], [MH] | dku_04, dku_11 |
| Torch-belly | lobs firebottles (a blast and fire); no fists, so up close it only backs off; goes off a moment after it falls | [MH], [TVT], [GUIDE] | dku_12 |
| Crowmask | throws cleavers along its row; a bin or another ghoul stops them; a little tougher than a shambler | [MH], [GUIDE] | dku_12 |
| Rammer | charges across the screen (a warning first), flattening ghouls on the way; dizzy after; lots of health | [MH], [GUIDE] | dku_12 |
| Howler | keeps its distance and pounces; knocked down, springs up and over, untouchable for a moment | [MH], [GUIDE] | dku_12 |
| Tusker | guards while idle (answers two blows on the guard with its own), a thrown thing knocks it down, its body slam can't touch anyone in the air | [MH], [TVT], [GUIDE] | dku_12, dku_13 |
| Sludger | spits a poison cloud; slow, tough, smaller than it looks | [MH], [GUIDE] | dku_13 |
| Visitor | a slow visible ray, a three-blow combo, blinks behind you (often twice after a knock-down), not past a screen edge | [MH], [TVT], [GUIDE] | dku_13 |
| Bulwark | guards as it walks; four plain blows on the guard break it (not flying kicks or charged punches), so does anything thrown | [MH], [GUIDE] | dku_13 |
| Giggler | hits hard, little health, straight back up | [MH], [GUIDE] | dku_13 |
| Mudskipper | leaps out of the water; fast, frail, a quick jab | [MH], [GUIDE-COMMENTS] | dku_13 |
| Feeler | stays where it is in the planks; slaps whoever comes near | [GUIDE] | dku_13 |
| Saucer | flies over low enough to punch; $10 inside; goes off when beaten | [MH], [GUIDE] | dku_13 |
| Dogs | a tied dog joins you when its leash (or the dog) is hit; bites ghouls, soaks up blows | [GUIDE], [STEAM-5H] | dku_10 |
| Big Ram | night 1, shamblers dancing and streaming in | [GUIDE], [TVT] | dku_14 |
| The Boar Baron | night 2, a tusker with more health | [GUIDE], [TVT] | dku_14 |
| The Visitors | night 3, two of them; both must fall | [GUIDE], [TVT] | dku_14 |
| The Undertow | night 4, out in the water; fists bounce off, only bodies thrown or knocked into it hurt it; mudskippers keep leaping out; its feelers come down where you stand, after a warning shadow | [GUIDE], [TVT], [STEAM-ALMOST] | dku_14 |
| Alderman Grist | night 5: bombs and flying elbows while ghouls stream in; beaten once, he changes: gas clouds and a body slam that steers after you; a roast chicken in a bin; after a minute a pack of dogs joins in | [GUIDE], [TVT], [STEAM-NOTFUN], [STEAM-5H] | dku_14 |
| The lift | five waves wait on the roof and drop in one at a time; the next only after the last is beaten | [GUIDE] | dku_15 |
| Shop | after each night: soup (full health) $5, +1 POWER, RECOV or TOUGH $20 (3 at most); THROW is fixed | [MH], [GUIDE], [GUIDE-COMMENTS] | dku_16 |
| Continue | unlimited; YES restarts the night at its start with full health, cash and stats kept; NO or a count of ten ends the run; tips on the screen, one about the gym | [MH], [MANUALS], [STEAM-5H], [STEAM-NOTFUN] | dku_16 |
| Endings | no continue: Gran comes home; any continue: too late | [TVT], [TVT-YMMV] | dku_17 |
| The gym | walk left as night 1 begins; endless waves; HOME (the wave) and GUEST (your best) on the scoreboard; every third wave passers-by run through first and a punch shakes loose food, a weapon or a dog; a fall ends it | [TVT], [GUIDE], [MH] | dku_18 |
| 2P | both at once (our reading, below) | [MH], [MH-CHEATS] | dku_19 |
| Records | the gym's best wave for one player and for two, nights reached, wins, good endings; no saving mid-run | [MH], [STEAM-5H] | dku_01, dku_17 |
| Goals | Beacon: hold out for nine waves in the gym. Saucer: punch your way up to the penthouse. Alien: make it to the top without a continue | [MH], [GGC], [GUIDE] | dku_17 |

### Readings we had to choose

- **Prices.** The wiki says an upgrade needs $20; the cherry guide's table
  says $5 for a full heal and $10 a stat, and a later comment on that guide
  corrects it: "the stat upgrades cost $20 not $10". Two sources against one
  (and the guide's own advice that "ending above $25 is ideal" fits a $5
  heal plus a $20 stat), so a stat is **$20** and the heal **$5**. The heal
  price rests on the guide alone. Possibly a patch changed them.
- **Numbers no source gives:** a fighter's 100 health; every ghoul's health
  and hit (shambler 10 and 8, torch-belly 14, crowmask 13 and 6 or a cleaver
  of 10, rammer 40 and a charge of 14, howler 12 and a pounce of 10, tusker
  60 and 12 or a slam of 16, sludger 30, visitor 28 and 7 a blow or a ray of
  10, bulwark 30 and 12, giggler 8 and 14, mudskipper 6 and 9, feeler 16 and
  8; bosses: Big Ram 90, the Boar Baron 120, each Visitor 40, the Undertow
  48 (a thrown body 12, a knocked one 6; its feelers 10), Grist 110 and then
  140); the stun (28 frames), the string's breath (18),
  the charge times, the get-up times, the grab hold, the reach window (10 up,
  3 down), TOUGH's 3 a level, the spin's 6, every speed and wind-up. The one
  tested number we kept: a shambler of 10 against jabs of POWER (10, 5 and 4
  jabs) and flying kicks of POWER + 3 (3, 2 and 2) [MH].
- **Continuing:** the night starts again with full health, and the cash and
  stats are as they were when you fell (cash picked up in the lost attempt
  stays). The sources say cash and upgrades persist; how much health you get
  back isn't said.
- **2P:** no source says how two players share Fist Hell beyond "2P co-op",
  a 2P survival record and a code for "Two Player Max Stats". Ours: both on
  the street at once, each with their own fighter (the same one is allowed),
  health and stats; one purse; the shop takes turns, player 1 then player 2;
  fists never hurt a partner; the view waits for whoever is behind; a
  fighter who falls sits out the rest of the night and comes back for the
  next with half health (more if the shop's soup was bought for them); only
  when both are down is it the continue screen. Locked on the Vita (one
  controller).
- **Which carried things drop when you run:** the manual says "some"; ours
  are the plank and the chainsaw; the bin can't be run with at all.
- **The gym's waves:** their make-up isn't documented. Ours grow from 3
  ghouls (3 + 3/4 of the wave number) and add a kind every wave and a half
  (shamblers, then gigglers, torch-bellies, crowmasks, howlers, mudskippers,
  bulwarks; rammers, visitors, sludgers and tuskers only after wave 9), at
  most five on the floor at once; each new kind shows up at least once.
  Passers-by on every third wave (the guide's "at the start of every third
  wave"). A fall ends the gym and goes back to the title; the gym is its own
  run and doesn't touch the no-continue story (the guide's author wasn't
  sure, so we keep them apart).
- **When the dogs come in the last fight:** after 60 s of it.
- **The Undertow's own attack:** feelers that come down on a marked spot.
- **The spin and the charge share A:** pressing B while charging is the
  spin, not a jump (the original's complaint that the spin "comes out when
  you wanted a jump kick" suggests the same loose input).
- **The saucer:** 6 health, low enough to punch only in the middle of its
  pass.
- **The squid boss's name, the passer-by's gun, and every layout** are ours.

## What is ours

- **Name:** DUKES UP (1987, Beamdown Softworks).
- **Fighters:** ROOK "the big quiet" (whose gran was taken), PIP "the
  sprinter", MACK "the slugger" and DOLLY "the life of the party".
- **The town:** Lampwick, and every place in it: Market Street, the roller
  rink, Canal Row, the night ferry, the waxworks, the old orchard, the
  graveyard, the visitors' ship, the promenade, the fishing jetty, the
  dunes, the Grand Hotel, its service lift and penthouse, and the Hornets'
  gym (SKID KIDS's). Every layout and script. The waxworks' figures are
  other Beamdown heroes (Mo, Pim, Dice, Kip and Posy), our nod to the
  original's arcade cabinets of other UFO games.
- **The ghouls:** shamblers, torch-bellies, crowmasks, rammers, howlers,
  tuskers, sludgers, visitors, bulwarks, gigglers, mudskippers and feelers;
  Big Ram, the Boar Baron, the Visitors, the Undertow and Alderman Grist;
  Gran Bess.
- **Things:** apples, sandwiches, drumsticks, roast chickens, coins, fives,
  gold rings, planks, chains, pipes, ghoul arms, the scattergun, the
  chainsaw, bins, ghoul heads, cleavers, firebottles; the corner shop's hot
  soup, heavy bag, vitamin tonic and leather jacket; the runaway thresher,
  Beamdown's saucer.
- **Music:** "Dukes Up" (title), "Pick Your Corner" (select), "Market Street
  Midnight", "Last Ferry", "The Orchard at Night", "Promenade", "The Grand
  Hotel" (the nights), "Heavyweight" (bosses), "Alderman Grist", "The Corner
  Shop", "Home and Guest" (the gym), "Going Up" (the lift), "Gran for
  Alderman" and "Too Late" (endings), and the jingles "Night's Over" and
  "Down for the Count".
- **Words:** the story, the tips, the endings, the credits, the goal lines
  and every label.
- **Art:** every figure, built from posed limbs; every backdrop.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
UFO 40 goals, which are Fist Hell's own (gift, gold, cherry). The original's
terminal codes (a dog with 9999 health, maxed stats, 2P maxed stats) are UFO
50 collection features; UFO 40 has no terminal, so they are left out.

**Owner's change:** the original jumps on Button 2 (the manual's A) and
attacks on Button 1 (B). The owner wants the main attack on A, the button
players press first, so here A punches (and picks up and uses things) and B
jumps (and throws what you hold). The cartridge card's CONTROLS page can
swap them back.

## Controls

| Input | Action |
|---|---|
| D-pad | walk eight ways; tap twice left/right: run; tap twice up/down: dodge |
| A | punch (the fifth of a string is a kick); on something underfoot: pick it up; holding something: use it |
| Hold A, let go | charged punch |
| A while running | dash attack |
| B | jump; A in the air: flying kick; holding something: throw it |
| A + B | spin |
| Walk into a stunned ghoul | grab it: A hits, direction + A throws, B slams (ROOK: hold B; DOLLY: hold A) |
| D-pad on the floor | roll |
| Title | UP / DOWN 1 PLAYER or 2 PLAYERS, A starts, B to the library |
| Select | LEFT / RIGHT, A picks, B back |
| Shop | UP / DOWN, A buys or goes on |
| START | pause |
| Player 2 | the second pad (or the keyboard's other half), the same buttons |

### Controls not confirmed (flagged)

| Control | Status |
|---|---|
| A punches, B jumps | **owner's change**: the original attacks on Button 1 (B) and jumps on Button 2 (A) [MANUALS], [MH] |
| B while charging | **reading**: the spin (both buttons held); the sources only say you can't jump while charging |
| Rolling on the floor | the guide: "roll vertically or horizontally by holding the corresponding movement key"; our speed |
| Shop, select and continue menus | **ours**: the manual doesn't describe them |
| Player 2's controls | **ours**: no source describes them |

## Not confirmed

- Every number in "Readings" above, and the heal price.
- How 2P works.
- The gym's waves and how often the passers-by come.
- When the final fight's dogs come.
- Whether visiting the gym affects the no-continue ending.
- Whether a continue gives back full health.

## Tests

`tests/dku_01` … `dku_22` drive every rule with button presses, or set up a
moment with cheats and then play it with presses: the title, the select
and the card (01), walking, running, dodging and jumping (02), the punch
string and its resets (03), the stun-lock, the reach window and the
kill-kick (04), the spin and the lack of any shield (05), the charged
punch, dash attack and flying kick (06), grabs, throws, slams and holds
(07), each fighter's own move (08), food, cash and weapons (09), props and
hazards (10), spawning by scroll position and lock-screens (11), every
ghoul (12, 13), every boss (14), the lift (15), the shop and continues
(16), the goals, both endings and the records (17), the gym (18) and two
players (19). `dku_20` checks the full scale against the original's (five
nights, the bosses, twelve kinds, five lift waves, the prices, the four stat
spreads) and that every screen's words fit.

The demo player (`dku_bot_buttons` in `dku_bot.c`) only ever answers with
buttons. It plays the way the cherry guide teaches: it walks on a step at a
time so ghouls come a few at once, stands a little below the one it fights
(on whichever side keeps the rest of them in front of it),
punches four times and takes a breath, spins when they get round both
sides or a cleaver is about to land, dodges out of a charge's lane, keeps
away from holes, blasts and the thresher, breaks containers, picks up food
and cash (and, when hurt, the food in a container mid-fight), punches the
saucer, carries mudskippers to the Undertow and throws them in (walking them
out from under a falling feeler), and in the
shop buys a heal when it needs one and then POWER, TOUGH and RECOV. From the
title it beats all five nights and both of Grist's forms with no continue
(`dku_21`, the good ending and all three goals but the beacon) and, walking
left at the start, holds the gym for nine waves (`dku_22`). Integer
arithmetic only, so both play the same on every platform.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Fist Hell" (raw page): controls, moves,
  the spin, stats and the stat tests, the fighters, food and cash, the
  shop's $20, weapons, enemies, bosses, goals, 2P co-op.
  https://ufo50.miraheze.org/wiki/Fist_Hell
- [MH-CHEATS] Miraheze, "Cheats": GOOD-GIRL, HELL-FIST, TEAM-FIST.
  https://ufo50.miraheze.org/wiki/Cheats
- [MANUALS] Steam guide "The missing manuals - How to play UFO 50 games",
  section 33: A jumps, B attacks (tap, hold, while running, in the air),
  picks up and uses; grabs; the health bar and cash; containers; the shop;
  continues. https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [GUIDE] Steam guide "Fist Hell Guide [Gold+Cherry]" by Baszie: every
  move, every enemy, the punch loop, the reach, spawning by scroll, the
  shop table, food values, each stage, the elevator's waves, the final
  boss, survival. https://steamcommunity.com/sharedfiles/filedetails/?id=3352616154
- [GUIDE-COMMENTS] the comments on [GUIDE]: $20 upgrades, what RECOV does,
  Amy's thrown enemies, no wake-up punish, cancelling the string by moving.
- [GGC] Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [STEAM-ALMOST] Steam thread "Fist Hell is almost really good".
  https://steamcommunity.com/app/1147860/discussions/0/4852155556170004326/
- [STEAM-NOTFUN] Steam thread "Fist Hell isnt fun".
  https://steamcommunity.com/app/1147860/discussions/0/4700161643034762292/
- [STEAM-5H] Steam thread "I just played Fist Hell for 5 hours".
  https://steamcommunity.com/app/1147860/discussions/0/4849904631718328854/
- [TVT] TV Tropes, "UFO 50 Game 33 Fist Hell" (recap).
  https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game33FistHell
- [TVT-YMMV] TV Tropes, YMMV page. https://tvtropes.org/pmwiki/pmwiki.php/YMMV/UFO50
- [LIZ] https://lizstar64.github.io/reviews/2024/10/17/UFO50-33.html
- [STATIC] https://staticcanvas.substack.com/p/the-ufo-50-diaries-fist-hell
- [POPCAR] https://popcar.bearblog.dev/reviewing-every-ufo50-game/
