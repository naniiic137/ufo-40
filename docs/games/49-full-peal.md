# 49 · FULL PEAL

*Internal design document. Not shown in the product.*

## Tribute to

**Campanella 3** (UFO 50 game #49, Mossmouth, "June 1989"), the last game
of the Campanella series: the bell ship turned into a faux-3D shooter that
flies into the screen. The rules were researched from text only: the
community wiki (rendered page), the Steam guides "The missing manuals",
"Campanella 3 Cherry Guide", "Tips & Tricks for Every Game", "Gift, Gold &
Cherry", "A Secret in Campanella 3" and "The ULTIMATE Player 2 guide for
Campanella 3", Steam threads and written reviews (listed under Sources; the
full notes are in the research folder, `49-campanella-3.md`). No UFO 50
images, video, sprites, music, wave layouts or text were used as
references, and nothing was taken from a UFO 50 install.

**Map type: fixed.** Campanella 3's waves are "the same every time" [LZ]:
fixed sequences of formations that come back and get remixed [MH], [ST].
Ours are too (`fpl_waves.c`): 37 formations of our own, named after
methods rung on church bells, built into twenty waves of our own. Only the
structure follows the original: five stages of four waves and a boss, most
waves four to seven formations long and the longest (D3 and E4) about eleven and twelve (ours: twelve each)
[MH], new kinds of foe arriving stage by stage [CH], and nothing random.

**The series.** FULL PEAL is the third of UFO 40's Campanella tributes,
after BELLHOP (17) and CLARION CALL (35): Ansel flies the Tinkler (the
chime ship of CHIME CIRCUIT, 19) and his sister Clary is on the radio, as
Pilot and Isabell are in the original [MH], [CH].

| Full scale | Campanella 3 | FULL PEAL |
|---|---|---|
| Players | 1, and a hidden second-controller mode [MH], [SECRET] | 1, and Clary's console for a second controller |
| View | behind the ship, into the screen; the ship moves on a flat plane facing the screen [MH], [MM], [LZ] | the same |
| The plane | a 6 × 4 grid of lanes; shots in a lane hit what is in that lane [LZ] | 6 × 4 |
| Guns | a forward shot into the distance and a side blaster along the plane [MH], [MM], [CH] | the same two |
| Stages | 5 (A to E), each 4 waves and a boss [MH] | 5, the same |
| Waves | 20, fixed formations [MH], [LZ] | 20, 4 to 12 formations each (37 kinds) |
| Foes | 16 kinds [MH] | 16 kinds, one for each of the original's, plus the bosses' company (wisps, flares) |
| Bosses | 5 [MH], [CH] | 5 |
| Grading | each wave 0–100 by the share of its foes destroyed [MH], [LZ] | the same |
| Bonus round | after a stage with a 100 or a 0; balloons, red 1, orange 3; 50 buys a continue [MH], [TIPS], [CH], [MM] | the same |
| Lives | 3 a credit, not refilled between stages; losing them restarts the stage; three times and it's over [ST], [CH], [MM] | 3 ships, 2 continues |
| Final score | the wave grades plus 100 per continue [MH] | the same |
| Goals | 3 [MH], [GGC] | the same 3 |
| Stats | Top Score A–E [MH] | the best total of each stage, on the title, with the best final score, the furthest stage, runs and wins |
| Code | PACI-FIST: no weapons, only moving [MH-CHEATS] | HUGS-ONLY, the same |
| Micro-games | 50 on a second controller, in the cockpit's monitor, four colours, a score each and a total [MH], [SECRET], [P2] | 50, the same |
| Saving | none (one sitting; the high scores kept) [STEAM-SAVES] | the same |

**How long it takes.** The demo pilot (which reads every foe and shot on
the real rules and presses the real buttons) wins from the title in
140,538 frames, about 39 minutes, losing seven ships and two credits (it
plays stage E three times), with a final count of 2,261 (`fpl_28`).

## Structure

| Stage | Name | New foes | Boss | Clary's call |
|---|---|---|---|---|
| A | OUTER BELFRY | clappers (some crossing the plane), crossheads, pendulums | the Gloameye | "CLARY HERE. I'LL BE ON THIS CHANNEL ALL THE WAY TO KNELL." |
| B | TIN NEBULA | sallies, forkers, tenors, lookouts | Knucklebell | "THE BELLS ON KNELL HAVE STARTED RINGING BY THEMSELVES." |
| C | THE MURK | motes, bourdons, quick sallies, nibblers, spites | the Inkwell | "MY CHARTS ARE NO GOOD NOW. KNELL HAS GROWN A SHELL!" |
| D | TOLLGATE | trebles, caltrops, brooders | Shellback | "THE RINGING IS SHAKING MY CONSOLE APART. BE QUICK, ANSEL!" |
| E | KNELL | dodgers, and all the rest | Queen Sordina | a scrambled call spelling ANSEL |

- **Title**: START or CODE over the stars and Knell, with the records (the
  best total of each stage A to E, the best final score, wins, the furthest
  stage, runs). B goes back to the library.
- **A stage**: Clary's call on the cockpit monitor and the stage's name;
  waves 1 to 4, each followed by its grade (an owl for a 100); the boss;
  the balloon round if the stage had a 100 or a 0; the next stage.
- **The end**: Queen Sordina beaten (and the last balloon round, if
  earned), a short and abrupt ending: the two ships go down to Knell, a
  flash, smoke, THE END. Then the final count: the twenty grades, the
  continues left × 100, and the total. Losing the last ship of the last
  credit is GAME OVER, which waits for player 1.
- **Not kept**: a run is played in one sitting (START pauses it); the
  records and the console's high scores are saved.

## The three goals

| UFO 40 goal | Condition | Campanella 3's goal [MH], [GGC] |
|---|---|---|
| Beacon | "WIN A CONTINUE IN A BALLOON ROUND": 50 points in one | gift: clear the bonus round |
| Saucer | "SILENCE QUEEN SORDINA": beat the last boss | gold: beat the game |
| Alien | "FINISH THE RUN WITH 1,500 POINTS": a final count of 1,500 or more | cherry: win with 1,500+ |

The goal lines and the blurb are in our own words. A run with HUGS-ONLY on
earns none of them and leaves the records alone.

## Mechanics checklist

| Mechanic | How FULL PEAL does it | Source | Test |
|---|---|---|---|
| No gravity, no fuel, no walls | the ship moves freely in eight directions on its plane and stays where it is let go; the plane's edges stop it and nothing else | [LZ], [ST], [MM] | fpl_02 |
| Edge markers | orange marks along an edge when the ship comes near it | [MH], [LZ], [MM] | (drawn) |
| The forward gun | A (the owner's layout; see Controls) fires into the distance down the lane the ship is in; it hits what is out there in that lane, generously | [LZ], [MM], [CH], [PD] | fpl_03 |
| Two depths | the forward gun passes over foes that have reached the plane; only the side blaster reaches them | [MH], [CH] | fpl_03, fpl_04 |
| The side blaster | B fires along the plane, up, down, left or right, away from the way the ship is moving; held, it keeps that direction | [MM], [CH], [TIPS] | fpl_04 |
| Both at once | the side blaster wins | [MM] | fpl_04 |
| One hit | a shot, a shell landing or a foe on the plane costs a ship | [ST], [MM] | fpl_05 |
| Respawn | the next ship is there at once, blinking and safe for a moment; the wave goes on | [ST], [MM] | fpl_05 |
| Foes in lanes | foes come out of the distance down the 24 lanes; some sit on the plane, some keep their distance and shoot | [MH], [LZ] | fpl_06 … fpl_09 |
| Clapper (Beat) | comes in and holds on the plane, then flies off; some cross the plane end to end after an arrow at that edge | [MH], [CH] | fpl_06 |
| Tenor (Heavy Beat) | the same, three hits | [MH], [CH] | fpl_03, fpl_07 |
| Treble (Light Beat) | the same, much quicker | [MH], [CH] | fpl_09 |
| Dodger (Off Beat) | slower, two hits, slips a lane sideways when hit | [MH], [CH] | fpl_09 |
| Bourdon (Baritone) | comes in from an end of the screen to a lane marked on the plane, sits, leaves; can't be hurt | [MH], [CH] | fpl_08 |
| Crosshead (Cannon) | keeps its distance; after a pause lobs a shell that bursts in a cross of four shots that linger on the plane | [MH], [CH] | fpl_06 |
| Forker (Hunter) | keeps its distance; its shell splits into two, up and down | [MH], [CH] | fpl_07 |
| Pendulum (Tremolo) | swings top to bottom on a fixed path as it comes | [MH], [CH] | fpl_06 |
| Spite (Vile Tremolo) | the same, and bursts in a cross when shot | [MH] | fpl_08 |
| Sally (Drone) | homes a little; reaching the plane it bursts into three spinning shots | [MH], [CH] | fpl_07 |
| Quick sally (Vile Drone) | faster, wriggling | [MH], [CH] | fpl_08 |
| Lookout (Sniper) | appears at an end of the plane and fires along it, three shots and three more, then goes | [MH], [CH] | fpl_07 |
| Mote (Poison) | appears on the plane and drifts straight at where the ship was | [MH], [CH] | fpl_08 |
| Nibbler (Piranha) | appears on the plane and chases, two hits | [MH], [CH] | fpl_08, fpl_09 |
| Caltrop (Spike) | comes down its lane and rams through the plane; can't be hurt | [MH], [CH] | fpl_09 |
| Brooder (Charkas Jr.) | keeps its distance and lobs motes that land on the plane and drift at you (one hit each, not counted); shoot it before they land | [MH], [CH] | fpl_09 |
| Grades | each wave 0–100 by the share of its counted foes shot down (rounded down); the ones that can't be hurt don't count; waves end by themselves | [MH], [LZ], [TIPS] | fpl_10 |
| The owl | a 100 shows the owl | [LZ] | fpl_10 |
| Exactly 50 on the first wave | a strange message | [MH-META] | fpl_10 |
| The Gloameye (Galbrain) | curves; three orbs take turns lobbing crosses; the eye is the weak point; at half it turns gold and fires twice as often | [MH], [CH] | fpl_12 |
| Knucklebell (Robopoke) | flies a square; two fists close in from both ends of the plane, knocked back by the side blaster, unhurt by anything (no change at half health), bouncing apart when they meet; the nose is the weak point | [MH], [CH] | fpl_13 |
| The Inkwell (Joe Pulp) | curves; flares down the plane from the top; now and then two pairs of crosses; at half, faster and only flares | [MH], [CH] | fpl_14 |
| Shellback (Eggsaber) | sweeps along the top; fans of 3, 4 and 3 from the plane's top edge, spreading as they fall, so the bottom is the safest place; two pods fire one aimed shot each now and then; no change at half health | [MH], [CH] | fpl_15 |
| Queen Sordina (Queen Zu) | curves; only her open mouth can be hurt; a hit shuts it; left open, caltrops fly out; wisps (pink skulls) cross the plane; emptied, she heals to full; Clary comes and weakens her; then any hit counts, her two pods lob crosses, more wisps, no caltrops, and Clary fires too | [MH], [CH] | fpl_16 |
| The balloon round | after the boss of a stage with a 100 or a 0; red 1, orange 3; more 100s, more orange; 50 points = a continue | [MH], [TIPS], [CH], [MM] | fpl_17 |
| Ships and credits | 3 ships, not given back between stages; all lost: a continue, three ships, the stage again from wave 1; none left: game over | [ST], [CH], [MM] | fpl_18 |
| The final count | the twenty grades plus 100 for each continue left | [MH] | fpl_19 |
| Records | the best of each stage, the best final score, the furthest stage | [MH], [GLITCH] | fpl_20 |
| PACI-FIST | HUGS-ONLY: no guns at all, only moving; records and goals off | [MH-CHEATS], [P2] | fpl_21 |
| The console | a second controller holds Button II (A) for a couple of seconds; the cockpit monitor becomes a menu of 50 micro-games; it runs whatever the main game is doing | [SECRET], [P2], [MH] | fpl_22 |
| Four colours | black, white, yellow and red | [SECRET] | (drawn) |
| After a game over | the console keeps going as long as player 1 presses nothing | [SECRET], [P2] | fpl_22 |
| Scores | every game keeps a high score, blank until it is above 0, then four digits, five past 9999 (99,999 at most); the menu shows the total of all fifty, which stops at 999,999 | [P2] | fpl_22 |
| The fifty games | each with its original's rule and scoring unit (below) | [MH], [SECRET], [P2] | fpl_23 … fpl_27 |
| Fixed | nothing in the main game is random: the same inputs give the same run | [LZ], [ST] | fpl_11 |
| Whole game | won from the title with real presses, all three goals | | fpl_28 |

### The fifty micro-games

Every one is ours in name and picture; the rule and the scoring unit are
the original's as the guides describe them [MH], [SECRET], [P2].

| # | Ours | Rule | Score |
|---|---|---|---|
| 01 | SHOO | run a grid in the way last pointed; chasers from the corners in clockwise turn, half your speed | 10 a second |
| 02 | KILN | paddle and ball; tiles creep down; red ones spit a shot at you, gold ones take two and don't | 2 a tile |
| 03 | FLAP | flap (either button) to stay up, steer; gold to take, red from both sides; floods after 100 | 5 a gold |
| 04 | PEEK | uncover squares; misses show arrows toward the face; 50 arrows or B end it | 10 a face |
| 05 | PIKE | a grid with a pike in front; chasers from the top and bottom crosses by turns | 10 a kill |
| 06 | TIPTOE | mines shown for a second (only in every other column), then dark; cross to the gold | 10 a crossing |
| 07 | ANGLER | steer the hook; the first fish to touch it ends the game, scored by kind and size; a whale after 30 fish | the catch |
| 08 | BURROW | a tunnel scrolling at you, narrower and faster | 2 a second |
| 09 | SLOPE | tilt to steer downhill; the slope never slows | 10 a gold |
| 10 | OGRE 1 | take a boss apart from below; the score counts down from 400 and only a win keeps it | what's left |
| 11 | PLINK | shoot the bird between two gold ships, never the ships | 5 a bird |
| 12 | DRIP | tap as the drop reaches the fingertip (seven stages of swelling first) | 10 a drop |
| 13 | SHOO 2 | move freely; bouncing chasers from two corners, gold from the other two | 5 a gold |
| 14 | LOOPY | always moving; LEFT and RIGHT turn; each prize leaves a chaser | 10 a prize |
| 15 | FLITS | bats that take two hits and shoot back; a new one at once | 10 a bat |
| 16 | FORT | five guns on five buttons: outer guns, inner lasers, the cannon | 5 a saucer |
| 17 | HAMMER | eight seconds; every press counts | 1 a press |
| 18 | CUPID | find the 2–5 hearts among the hidden symbols; four misses or B end it | 10 a heart |
| 19 | BONK | paddle and ball against creatures creeping down | 10 a creature |
| 20 | SPY | shoot the impostor (aim LEFT, none, RIGHT); a person or dawdling ends it; 60 s | 5 each |
| 21 | HURDLE | run, jump low and high fences, gold in the air | 10 a gold |
| 22 | CUE | aim, strike (hold the button to brake), roll into the prize; a pocket or a miss ends it | 10 a prize |
| 23 | PIKE 2 | quicker chasers, and blocks only you can break | 5 a kill or block |
| 24 | TWINS | two of you either side of a wall, moving together, a block on each side | 5 a second |
| 25 | HALT | stop each red block before it reaches the right (three rows) | 10 a block |
| 26 | PIVOT | a turret turning one step at a time; creepers from eight points clockwise | 2 a creeper |
| 27 | LANES | tilt to steer; rows flash at the top, then rush down | 1 a second |
| 28 | ZAP | you are the cloud; strike the umbrella; a miss ends it | 5 a strike |
| 29 | BOING | a ball bouncing by itself over drifting platforms that spread out | 5 a gold |
| 30 | OGRE 2 | a sturdier boss with straight guns and blocks nothing breaks; from 700 | what's left |
| 31 | RING | an arena with an unbeatable gun at each side (a laser every fifth rock); two rocks drift, two home | 2 a rock |
| 32 | THAW | tiles melt once jumped off; you walk on air until you jump; a bat above | 10 a second |
| 33 | LAMP | a slug in the dark; press while it's in the lamp | 10 a slug |
| 34 | SAYSO | do what the screen says, quicker and quicker; 60 s | 1 each |
| 35 | TACK | a tack slides along the floor; step so it passes under a lifted foot | 10 a wall |
| 36 | PICK | orange, pear, apple, banana, cherry, round and round from the apple, faster; press on the cherry | 10 a cherry |
| 37 | WHICH? | 99 dots for a moment, less time each round: more gold (UP) or red (DOWN)? | 5 each |
| 38 | DRAW | press when the other one reaches (first after a second, then one of eight waits); 60 s | 5 each |
| 39 | BOBBLE | a bouncing ball, fences from above and below; busier after 70 gold | 10 a gold |
| 40 | PENNY | coins pour down, some fakes | 2 a coin |
| 41 | SHOVE | a wall of gunners closing in; shots push them back; past the left edge they're gone | 10 a second |
| 42 | KEEPUP | one ball, then more every few seconds; drop none | 10 a second |
| 43 | WARP | only jumps of 3 or 4 squares, through walls that sweep in every 2 s | 5 a second |
| 44 | THRONG | shoot through red walls; the fifth is one gold block: never touch or shoot it | 5 a block |
| 45 | BLAST | run and gun (UP aims up): rollers 3 hits with ground shots, hulks 5, bats 2, posts can't be hurt | 10 a kill |
| 46 | STILL | move only while the eyes are shut; the apple on the far side; 60 s | 10 an apple |
| 47 | BRIAR | saucers sow a hedge that creeps toward you; shoot sowers and seeds | 2 a second |
| 48 | OGRE 3 | a boss that moves and fires lasers; from 999 | what's left |
| 49 | WARPTOE | the minefield, crossed only by jumps of 3 or 4 | 10 a crossing |
| 50 | FACES | match the face: UP hair, LEFT eyes, RIGHT nose, DOWN mouth (six each), a button for a new one; 10 s a face | 10 a match |

### Readings we had to choose

- **Moving and aiming:** the ship moves smoothly (not cell to cell); the
  forward gun fires down the lane the ship is in, and hits anything within
  0.78 of a lane from that lane's middle (a little into the next lane on
  either side: the "generous" hit detection [PD]). The ship crosses the
  plane in about 1.5 s.
- **Fire while held:** both guns keep firing while their button is held,
  every 7 frames.
- **The blaster standing still** fires away from the way the ship last
  moved; moving diagonally, the side-to-side part decides.
- **Foes' hit points:** the sources name a count only where it is more
  than one (Off Beat 2, Heavy Beat 3, piranhas 2), so every other foe,
  the four gunners included, falls to one hit. How long a clapper sits on the plane (4 s) and a gunner
  keeps its distance (7 s) are ours.
- **The cannon's cross:** "fires 4 bullets in a cross" [MH] and "shoots in
  4 directions but lasts for a while" [CH]: a shell that lands where the
  ship was and bursts into four shots along the plane.
- **Credits:** three ships and two continues: "once your three lives are up
  … back to the start of wave 1. Once this has happened three times the
  game is over" [ST]. A continue restarts the stage from its first wave and
  wipes that stage's grades so far.
- **Continues at the end:** the ones left over count 100 each.
- **The balloon round:** 30 s, 45 balloons, 4 of them orange plus 4 for
  each 100 in the stage (a 0 opens the round but adds none). A round also
  follows the last boss when earned ("stage 6" in a report of quitting in a
  bonus stage [GLITCH]).
- **Bosses:** health 150, 190, 230, 270 and 300; they score nothing. Queen
  Sordina's mouth opens for up to 2.5 s; Clary comes 4 s after she has
  healed and takes her to a third; Clary hits her about twice a second.
- **HUGS-ONLY:** like PACI-FIST, the bosses can't be beaten with it on
  ("considered impossible" [P2]).
- **The console:** opened by holding A (Button II) for 2 s; LEFT and RIGHT
  step one game, UP and DOWN ten; A plays; B switches it off; after a game
  over A plays again and B goes back to the menu. The sizes, speeds and
  timings inside each game are ours; the three OGRE games count down from
  400, 700 and 999 (each player's best scores in [P2] fit those). The
  original's TOTAL SCORE display has a known adding-up bug [P2]; ours adds
  up correctly.
- **The records:** a stage's best is the total of its four grades. The
  furthest stage, shown in the original's library [GLITCH], is on our title.
- **The meta message** for exactly 50 on the first wave is our own line,
  "HALF A PEAL IS STILL A PEAL".
- **Shellback's weak point** is its beak, underneath, one row below the
  top: the sources give it no weak point, and the beak keeps the fight
  low, where the cherry guide says to be.
- **The guns' buttons (the owner's layout):** the original has the forward
  gun on Button 1 (our B) and the side blaster on Button 2 (our A). UFO 40
  puts the main action on A by the owner's choice, so the forward gun,
  which meets most foes and is the only thing that hurts a boss, is on A
  and the side blaster on B; pressing both, the side blaster still wins.
  The cartridge card's CONTROLS page can swap them back.
- **The ending** is abrupt, as in the original ("they just … left it at
  that?" [LZ], [END]): our own crash and THE END.

## What is ours

- **Name:** FULL PEAL (1989, Beamdown Softworks): in bell-ringing, a peal
  is the long performance at the end; this is the series' last.
- **Cast:** Ansel in the Tinkler and his sister Clary (from CHIME CIRCUIT
  and BELLHOP); Queen Sordina and the bell-planet Knell.
- **Stages:** Outer Belfry, Tin Nebula, The Murk, Tollgate, Knell; Clary's
  five calls.
- **Foes:** clappers, tenors, trebles, dodgers, bourdons, crossheads,
  forkers, pendulums, spites, sallies, quick sallies, lookouts, motes,
  nibblers, caltrops, brooders, wisps and flares; the Gloameye,
  Knucklebell, the Inkwell, Shellback and Queen Sordina; the owl.
- **Waves:** all 37 formations and 20 waves.
- **The console:** all fifty micro-games' names and pictures.
- **Music:** "Full Peal" (title, built from rung changes), "Outer Belfry",
  "Tin Nebula", "The Murk", "Tollgate", "Knell", "Big Bells", "Queen
  Sordina", "Balloon Round", "Ring Out", "The Final Count", and the jingles
  "Incoming", "Wave Graded", "Every One" and "Tinkler Down".
- **Words:** every line, label and goal; the HUGS-ONLY code.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
UFO 40 goals, which are Campanella 3's own. UFO 40 has no terminal, so the
code is typed on the cartridge's title (as BELLHOP does). On PC the second
player can use the numeric keypad and Delete / End (Delete is the
original's default key for player 2's Button II [SECRET]) besides a second
pad; the Vita has one controller, so the console needs a PC or the web.

## Controls

| Input | Action |
|---|---|
| D-pad | fly anywhere on the plane |
| Hold A | forward gun, into the distance down your lane (owner's layout: the original's Button 1) |
| Hold B | side blaster, away from the way you're moving; held, it keeps its aim (the original's Button 2) |
| START | pause |
| Title | UP / DOWN, A: START or CODE; B: the library |
| Second controller: hold A | open Clary's console |

### Not confirmed (flagged)

| Control | What we did | Why it's a reading |
|---|---|---|
| Fire while held | both guns repeat every 7 frames while held | no source says |
| Side blaster when standing still | away from the last way moved | the sources say "opposite to the direction you are moving" |
| Side blaster moving diagonally | the side-to-side part decides | not described |
| Opening the console | A (Button II) held 2 s; A and B together works too | "a couple seconds" [SECRET]; the wiki says both buttons [MH] |
| The console's menu | LEFT / RIGHT one, UP / DOWN ten, A plays, B off | not described |
| After a micro-game over | A again, B to the menu | inferred from [P2]'s note about pressing only cancel |
| Inside the micro-games | each game's buttons as in the table above | most from [P2]; the rest ours |
| PC keys for player 2 | numpad 8 4 2/5 6, 0 or Delete (A), . or End (B) | ours |

## Not confirmed

- Every hit-point count, hold time and speed above; the boss health
  values; the exact moment Clary arrives.
- The number of continues (two, from "three times") and that the leftover
  ones are what count at the end.
- The balloon round's length and balloon count, and whether a 0 adds
  orange balloons.
- The console's menu, and every size and timing inside the micro-games.

## Tests

`tests/fpl_01` … `fpl_28` drive the rules with button presses (player 2's
for the console), sometimes after setting up a moment with cheats.
`fpl_02` … `fpl_09` cover flight, both guns, hits and every foe;
`fpl_10` and `fpl_11` the grades and the full-scale structure (20 waves,
16 kinds, 12-formation waves, the fixed runs, every glyph on screen);
`fpl_12` … `fpl_16` the five bosses; `fpl_17` the balloon round; `fpl_18`
ships and credits; `fpl_19` the ending and the goals; `fpl_20` the records
across a restart; `fpl_21` the code; `fpl_22` the console; `fpl_23` …
`fpl_27` all fifty micro-games, each reached through the menu, its rule
tried and its end reached. The demo pilot (`fpl_bot.c`) lists everything
that could hit the ship over the next 22 frames, tries the nine ways to
steer and takes the safest that brings it nearest its target (the lane of
a foe in the distance, the row or column of one on the plane, a boss's
weak point, a balloon); for the blaster it presses B while moving away
from its target. In `fpl_28` it wins the whole game from the title with
real presses and all three goals. The shell's library-scroll test now
tries the empty slot 50.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Campanella 3": the game, scoring, the bonus
  level, continues, the enemy and boss tables, the stages and their
  messages, the formation galleries (structure only), the micro-game list.
  https://ufo50.miraheze.org/wiki/Campanella_3
- [MH-CHEATS] Miraheze, "Cheats": PACI-FIST. https://ufo50.miraheze.org/wiki/Cheats
- [MH-META] Miraheze, "Meta Messages": exactly 50 % on the first wave.
  https://ufo50.miraheze.org/wiki/Meta_Messages
- [MM] Steam guide "The missing manuals - How to play UFO 50 games",
  section 49: the d-pad, A shoots horizontally or vertically opposite to
  the movement and holding locks it, B forward, both: sideways wins; the
  top row (stage, comms, lives, continues, progress, percentages, radar);
  the corridor's dotted squares; respawning, continues and game over; a
  perfect wave in a level brings the bonus round.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [CH] Steam guide "Campanella 3 Cherry Guide": X into the distance, A plus
  the opposite direction close by, held A keeps direction; each stage's new
  enemies; the bosses' weak points and half-health changes; the bonus
  round; lives not refilled. https://steamcommunity.com/sharedfiles/filedetails/?id=3360195841
- [TIPS] Steam guide "Tips & Tricks for Every Game": the two guns; not
  everything has to die; balloons 1 and 3.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3340576461
- [GGC] Steam guide "Gift, Gold & Cherry": win the bonus round, beat the
  game, win with 1500+. https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [SECRET] Steam guide "A Secret in Campanella 3": hold Button II on the
  second controller; the fifty games, each described; four colours; playing
  on after player 1's game over. https://steamcommunity.com/sharedfiles/filedetails/?id=3430658639
- [P2] Steam guide "The ULTIMATE Player 2 guide for Campanella 3": each
  game's controls and scoring unit, time limits, fail limits, the total
  (and where its display stops), blank scores growing from 4 digits to 5,
  the three boss games counting down, PACI-FIST, and that 0 % waves also
  open the bonus round. (The missing manuals' comments add that more
  perfect waves give better odds in it.)
  https://steamcommunity.com/sharedfiles/filedetails/?id=3659693716
- [LZ] Lizstar, review of #49: the 6 × 4 grid, the side blaster, the cat on
  a 100, the small second-player screen, "the same every time", the ending.
  https://lizstar64.github.io/reviews/2024/10/21/UFO50-49.html
- [ST] Static Canvas, "The UFO 50 Diaries: Campanella 3": no walls, three
  lives then back to wave 1, three times and over; fixed encounters.
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-campanella-3
- [PD] Pixeldie, ranking: the generous hit detection.
  https://pixeldie.com/2024/11/13/ranking-every-ufo-50-game-after-100-hours/
- [STEAM-SAVES] Steam thread on which games save: Campanella 3 doesn't.
  https://steamcommunity.com/app/1147860/discussions/0/4849904176633630339/
- [END] Steam thread "Is Campanella 3's Ending Bugged?".
  https://steamcommunity.com/app/1147860/discussions/0/4638240954399078382/
- [GLITCH] Steam thread on the library showing the stage reached.
  https://steamcommunity.com/app/1147860/discussions/1/595148763657430243/
- Reviews read for feel: Punished Backlog, AniGamers, Wikipedia's UFO 50
  article (listed in the research notes).
