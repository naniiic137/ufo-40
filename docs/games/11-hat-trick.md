# 11 · HAT TRICK

*Internal design document. Not shown in the product.*

## Tribute to

**Kick Club** (UFO 50 game #11, Mossmouth, "September 1984"). The rules were
researched from text only: the community wiki's raw page, Steam guides and
threads, written reviews, search summaries of TV Tropes and speedrun.com
(listed under Sources; the full notes are in the research folder,
`11-kick-club.md`). No UFO 50 images, video, sprites, music, screen layouts
or text were used as references, and nothing was taken from a UFO 50
install.

**Map type: fixed.** Kick Club's screens are hand-made, with fixed creature
starts, a fixed ball spot and fixed dessert spots [MH], [STATIC], so ours
are too (`hattrick_levels.c`, all our own layouts). Only the structure
follows the original:

| Full scale | Kick Club | HAT TRICK |
|---|---|---|
| Worlds | 4, in a fixed order, each 9 screens and a boss [MH], [LIZ] | 4: Sandy Court, Lucky Lanes, the Lido, the Big Stadium |
| Screens | 40 single screens, left-right symmetrical [MH] | 40, each written as its left half and mirrored, creatures included |
| Creatures | 3 kinds a world, 12 in all, contact always lethal [MH] | 12, the same three roles a world (below), 254 on the 36 ordinary screens (2 to 12 a screen) |
| Bosses | a big one of a world's kinds per world; world 3 a pair, the Goaltender and the Goal [MH], [SEARCH-TVT] | King Spiker, the Kingpin, the Lifeguard and his Chair, the Captain |
| The hunter | the referee in overtime [MH] | the Timekeeper in extra time |
| Food | fries, pizza, hot dog, burger; candy, chocolate, cake, sundae [MH] | popcorn, pretzel, taco, drumstick; lolly, cookie, donut, parfait (same values) |
| Dessert spots | "certain hidden spots on some levels" [MH] | 16, four in each world, none on the boss screens |
| Lives | 3, extends at 10k, 25k, 50k, 75k and 100k, no continues [MH], [RESET-37] | the same |
| Modes | 1P with two kids, 2P co-op, 2P versus by two codes [MH], [MH-CHEATS] | the same |
| Goals | 3 [MH], [GGC] | the same 3 |
| Stats | Most Desserts Eaten [MH] | the most desserts in one run, with the top score, the furthest screen, runs and wins |
| Music | 4 tracks: intro, gameplay, boss, ending [MH] | the same four (WHISTLE BLOWS, SATURDAY LEAGUE, CUP FINAL, LAP OF HONOUR), three jingles and the versus tune |

What each world brings, in its own clothes:

| World | Kick Club | HAT TRICK | Its three kinds (the original's role) | Boss |
|---|---|---|---|---|
| 1 | Racket World | **Sandy Court** (beach games) | spiker (Tennis Ball: walks its platform, now and then shoots at you), frisbee (Birdie: flies to and fro), beach ball (Ping Pong Ball: straight lines, bounces off walls) | **King Spiker**: three hops at you, then three spades lobbed in arcs (the giant Tennis Ball's jumps and rackets) |
| 2 | Track World | **Lucky Lanes** (a bowling alley) | pin (Runner: never stops, down the holes and round), bowler (Shot Putz: an angled shot down at you), spinner (Athlete's Foot: circles a fixed point) | **the Kingpin**: runs round the edges of the screen throwing cans (Track Star and his drinks) |
| 3 | Ice World | **the Lido** (a swimming pool) | buoy (Curling Stone: slides straight through the top and bottom wraps, or along), polo player (Hockey Player: to and fro, now and then a puck at you), ducky (Skate: a fixed route round its platform) | **the Lifeguard** (Goaltender: rings, very many hits, beaten without ending the fight) and **the Chair** (Goal: only hops about; knocking it down ends the fight and takes him with it) |
| 4 | USA World | **the Big Stadium** | prop (Helmet: throws balls that bounce fast and wild), glove (Basketball: straight up or down through the wraps, fast), bulldog (Baseball: asleep until you carry the ball, then straight at you) | **the Captain**: about the whole screen, throwing rugby balls (the giant Helmet) |

## Mechanics checklist

| Mechanic | How HAT TRICK does it | Source | Test |
|---|---|---|---|
| One fixed screen | each screen fits the 320 × 168 field under the HUD; walls round the edges; wherever an edge has no wall the screen wraps, sideways or top to bottom | [MH], [SEARCH-TVT] | htk_11 |
| Everything wraps | kids, the ball, bodies and creatures all go round | [MH] | htk_07, htk_11, htk_14, htk_15, htk_16 |
| Symmetry | every screen the same on both sides, creatures mirrored | [MH] | htk_24 |
| Start side | in 1P, Teddy starts on the left, Mae on the right | [MH] | htk_01 |
| One ball | one ball, at a fixed spot on each screen; every screen starts empty-handed; co-op shares it | [MH], [HANS] | htk_01, htk_21 |
| Pick-up | walk into the ball on the ground: no button; never in the air | [MH], [SEARCH-MANUALS] | htk_03, htk_31 |
| Kick | tap B; the pad, the run and the jump all change it: a chip standing, a lob (up), up and over (up + side), along the ground (down), a low drive on the run, a volley down in the air | [MH], [SEARCH-MANUALS], [STEAM-SCORE], [LIZ], [QT3] | htk_03 |
| Charged kick | hold B on the ground and let go: 1.6 times as fast, flatter, and three times as hard on a boss | [MH], [STEAM-SCORE], [SEARCH-TVT] | htk_04, htk_17 |
| Slide | B with no ball on the ground: a slide that keeps its line (no gravity, over gaps and thin air), knocks a loose ball on, hurts nothing | [MH], [SEARCH-TVT] | htk_04 |
| Header | B with no ball in the air: knocks the ball up off the kid's head | [MH] | htk_04, htk_05 |
| Crouch | DOWN: shots at chest height go over | [STATIC] | htk_12 |
| Walls | a ball carried, kicked or dropped against a wall stays out of it (the original's lost-ball bug is not copied) | [STEAM-BUG] | htk_31 |
| Jump | one fixed height, tapped or held (38 px, the next ledge is 32 px up); ledges are jumped up through | [SEARCH-MANUALS] | htk_02 |
| Stiff in the air | the pad bends a jump only a little (see Readings) | [SEARCH-MANUALS], [PUNISHED] | htk_02 |
| Big kid | 12 × 19, a large target | [POPCAR] | htk_02 |
| Kills | only a moving ball kills; a resting one does nothing; the ball never hurts a kid | [MH] | htk_06, htk_31 |
| The lit ball | white while the chain is alive; touched (picked up, kicked, slid, headed) it lights | [MH], [STEAM-SCORE] | htk_05 |
| Going dark | a loose ball goes dark on its third bounce or after a second on the ground; a carried one after 3 s without a kick; a falling ball (through a wrap) stays lit | [TWO-SKILLS], [STEAM-SCORE] | htk_05 |
| The chain | kills while lit: popcorn 100, pretzel 200, taco 500, then a drumstick 1,000 from the fourth on; a dark ball's kill is popcorn and starts nothing | [MH], [STEAM-SCORE] | htk_06 |
| Bodies and food | a killed creature's body falls; its food appears only where the body lands, so a body falling down a hole and round takes longer | [MH] | htk_06, htk_07 |
| Desserts | kick the ball through a hidden spot: a dessert appears and drops (lolly 100, cookie 200, donut 500, parfait 1,000 by world); once per spot | [MH], [STEAM-SCORE] | htk_29 |
| Clock | counts 20 to 0, a count every 2.5 s; clearing stops it and pays 100 a count left | [MH] | htk_08 |
| Clear | the last creature down: the clock stops, a short time to pick up the food, then balloons lift the kids off to the next screen | [MH] | htk_08 |
| Extra time | at 0: EXTRA TIME!, no bonus, and the Timekeeper (invincible, slow, homing, through walls) stays until the screen is clear or the run is over, deaths or not; boss screens too | [MH], [SEARCH-TVT], [MH-TERM] | htk_09 |
| One touch | any creature, shot or the Timekeeper takes a life | [MH], [SEARCH-TVT] | htk_09, htk_10, htk_12 |
| Lives | 3; extends at 10,000, 25,000, 50,000, 75,000 and 100,000, none after | [MH] | htk_10 |
| No continues | out of lives is the end; the next game starts at 1-1 | [RESET-37], [POPCAR], [SEARCH-TVT] | htk_10 |
| Bosses | big ones of their world's kinds; beaten, they burst into five bodies and five drumsticks (5,000) | [MH], [SEARCH-TVT] | htk_17 – htk_20 |
| The dual boss | the Lifeguard beaten drops five drumsticks and comes back; the Chair down ends it and takes him too | [MH] | htk_19 |
| Shooters' aim | worlds 1–3 shoot level or down at you, never up ("stay above them"); world 4's rugby balls bounce off floors several times and off walls | [TWO-SKILLS] | htk_13, htk_15, htk_16 |
| Co-op | both kids on one screen, one ball, Teddy left and Mae right; each their own lives and score; the run ends when both are out | [MH], [MH-MP] | htk_21 |
| Versus | codes only: a pitch with two goal mouths, one ball, first to 10 or first to 50 | [MH], [MH-CHEATS] | htk_22 |
| Codes | while one is on, nothing is saved and no goal is given, until the cartridge is left | [MH-CHEATS], [CONV] | htk_22 |
| No run save | a run is never saved; the cartridge keeps records only | [POPCAR], [CONV] | htk_23 |
| Ending | after the Captain: the ending (its own tune), then the credits | [MH] | htk_23, htk_28 |
| Meta secret | ours: the Lido boss's top lane rope spells a message in Morse, as the original's Ice World border tiles do | [MH], [MH-META] | htk_19 |
| Goals | Beacon: beat Lucky Lanes (world 2). Saucer: beat all four worlds. Alien: win with 150,000 points or more (checked at the end) | [MH], [GGC] | htk_23 |
| Difficulty | deceptively hard, three lives; the first worlds simpler, the last a wall | [POPCAR], [RESET-37], [SEARCH-USA], [TWO-SKILLS] | htk_30 |

### Readings we had to choose

| What | Our reading | Why |
|---|---|---|
| Buttons | **A jumps, B kicks** (Button 2 / Button 1) | the manual summary says "the A button jumps" [SEARCH-MANUALS]; the collection's convention [CONV] |
| Aim | five kicks on the ground (chip, lob, up and over, along the ground, the drive on the run) and the volley down in the air; the kid's run adds half its speed, a jump 0.4 of its rise | the number of aims is not documented [TWO-SKILLS] |
| When the kick goes | on the ground a tap kicks when B is let go (so a long press can be a charge); in the air B kicks at once | "tap to kick, hold to charge" [SEARCH-MANUALS] |
| The charge | one level, ready after half a second of B held on the ground: 1.6 times the speed, 0.55 of the rise, three times the damage to a boss for 1.5 s | "high-power, faster, lower" [MH], [STEAM-SCORE]; levels and numbers not documented |
| Air control | a little: the pad changes speed by 0.03 px a frame in the air (0.35 on the ground) | "can't adjust the jump in mid-air" (UNCERTAIN) [SEARCH-MANUALS], "stiff in the air" [PUNISHED] |
| Slide with no run | B on the ground slides even from a standstill | only "running + B" is documented [MH] |
| Speeds | run 1.25 px a frame; jump 38 px high; ball speeds as above | not documented |
| Going dark | third floor bounce, or 60 frames on the ground; carried: 180 frames | "bounces three times", "more than 1 second", "hold too long" [TWO-SKILLS], [STEAM-SCORE] |
| The ball and creatures | it passes through the creatures it kills (one kick can chain several) and comes back off a boss | not documented |
| A dark ball's kill | popcorn, and the chain stays at nothing | "a kill with a dark ball restarts the chain (fries)" [STEAM-SCORE] |
| Bodies | pop up and fall with the ball's drift; food where they land; a body still falling after 15 s no longer holds up the end of the screen | the landing rule is [MH]; the rest ours |
| Desserts by world | one dessert kind a world, in the original's rising values | the value table is [MH]; which dessert where is not documented |
| Death | the kid tumbles, then starts again at their own spot, safe for 2 s; the clock runs on; creatures stay; a carried ball drops and settles (and goes dark) | not documented (open question in the notes) |
| Co-op lives | separate lives and scores; the Alien counts the higher score | not documented |
| Boss strength | King Spiker 12 hits, the Kingpin 15, the Lifeguard 40 (back after 5 s), the Chair 10, the Captain 20; 0.5 s of flashing after each hit | "very large HP" for the Goaltender [MH]; the rest not documented |
| The Timekeeper | 0.42 px a frame, steady; he walks in at the top, on the far side from the kids | "slowly homes in" [MH]; speed not documented |
| Shot timing | spikers fire every 3.3–6.3 s, polo players 3.3–5.7 s, bowlers 3–5.3 s, props 2.5–4.3 s, each after a 0.4 s wind-up; a spiker's or polo player's first shot comes no sooner than 3 s into the screen | "occasionally shoots" [MH] |
| Versus | the pitch has two goal mouths at the bottom corners, only the ball goes in; a slide into the kid with the ball knocks it loose; after a goal the ball drops in the middle | "soccer/pong-style arena where each player tries to score goals" [MH], unfinished in the original; the rest ours |
| The transition | 4 s to pick up the food once the last body has landed, then balloons lift the kids off (about 2 s) | "picked up and carried to the next level" [MH]; "take forever" [POPCAR] |
| Not done | "transitions end faster if the ball finishes on your side" | one player's guess, mechanism unknown [STEAM-SCORE] |

## What is ours

- **Name:** HAT TRICK (1984, Beamdown Softworks), "one ball, forty pitches".
- **The kids:** Teddy (freckles, a ginger mop, green and white hoops) and
  Mae (black braids, a yellow shirt, navy shorts) of the Hat Trick Club.
- **The worlds:** Sandy Court, Lucky Lanes, the Lido and the Big Stadium,
  their backdrops and tiles, and all 40 screens with their names (FIRST
  SERVE ... THE CAPTAIN) and the versus pitch, THE SHOOTOUT.
- **The creatures:** spikers, frisbees, beach balls, pins, bowlers,
  spinners, buoys, polo players, duckies, props, gloves and bulldogs; King
  Spiker, the Kingpin, the Lifeguard and his Chair, the Captain; the
  Timekeeper.
- **The food:** popcorn, pretzels, tacos and drumsticks; lollies, cookies,
  donuts and parfaits.
- **The meta secret:** the Lido boss's top lane rope, short floats and long
  floats, spells GOOD GAME in Morse (`HTK_MORSE_MSG`).
- **The codes:** NUTMEG10 and NUTMEG50 (typed on the title's CODES screen).
- **Music:** "Whistle Blows" (the opening, also on the title), "Saturday
  League" (the match), "Cup Final" (the bosses), "Lap of Honour" (the
  ending and the versus win), "Shootout" (versus), and the jingles "Pitch
  Cleared" and "Full Time".
- **Words:** the story, the ending, the goal lines and every label.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
UFO 40 goals, which are Kick Club's own (gift, gold, cherry). The codes are
typed on the cartridge's own CODES screen because UFO 40 has no terminal.

## Controls

| Input | Action |
|---|---|
| D-pad left/right | run (and aim the kick) |
| D-pad up/down while kicking | aim: up lobs, up + side goes up and over, down goes along the ground (in the air: a volley down) |
| Down | crouch |
| A | jump (always the same height) |
| B with the ball, tap | kick (when B is let go on the ground; at once in the air) |
| B with the ball, hold on the ground | charge, let go for the driven shot |
| B without the ball | slide on the ground, header in the air |
| START | pause |
| 2P | the second pad, or the keyboard's other half on PC (not on Vita) |

## Not confirmed

These controls and rules are our reading, flagged because no source
confirms them (see Readings above for the reasons):

| Item | Our reading |
|---|---|
| Which button jumps | A (Button 2); B kicks |
| Number of kick aims | five on the ground, one in the air |
| Kick on release | a ground tap kicks on release |
| Charge time and levels | one level, 30 frames |
| Air control | a little |
| Slide from standing | allowed |
| Pick-up in the air | never; only a header touches the ball in the air |
| Death and respawn | own start spot, 2 s safe, clock runs on |
| Co-op lives | separate |
| Boss hit points | as listed |
| Versus details | goal mouths, tackles, the restart |
| Dessert kinds per world | one kind a world |

## Tests

`tests/htk_01` … `htk_31` drive the rules with button presses, or set up a
moment with cheats and then play it. The demo player (`htk_bot_buttons`,
`hattrick_bot.c`) presses real buttons: every few frames it makes up short
plans (run so far, jump here, kick there with this aim, slide), plus every
running jump and every straight shot from where it stands, plays each one
ahead on a copy of the screen and keeps the best that loses no life; a
graph of which ledge leads to which tells it how far it is from the ball,
or from a place to shoot from. With it:

- **all 40 screens are cleared** with the real flow between them, world by
  world (`htk_25` … `htk_28`, the last one into the ending), and every boss
  from full strength (`htk_17` … `htk_20`);
- **every screen is sound** (`htk_24`): symmetrical, one start, the ball on
  the middle line, holes only under holes, every place a ball can come to
  rest reachable by a kid and back again (a jump to the next ledge up never
  more than 3 tiles across), each world with its own three kinds;
- **the first world is gentle** (`htk_30`): a modest player (six plans a
  decision, no running-jump or shot search) keeps its life on 1-1 to 1-3 in
  at least 90 % of 24 tries, and a kid who just stands at the start lasts
  four seconds or more there; on 4-4, 4-5 and 4-9 the same player keeps it
  75 % of the time or less (measured: 100, 95 and 100 % against 50, 50 and
  25 %).

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Kick Club" (raw and rendered): the screens
  and wraps, symmetry, the ball, the combo and drops, bodies, desserts, the
  clock and overtime, lives and extends, every creature and boss, modes,
  worlds, music, goals and stats. https://ufo50.miraheze.org/wiki/Kick_Club
- [MH-CHEATS] Miraheze, "Cheats": the two versus codes, to 10 and to 50.
  https://ufo50.miraheze.org/wiki/Cheats
- [MH-MP] Miraheze, "Multiplayer". https://ufo50.miraheze.org/wiki/Multiplayer
- [MH-TERM] Miraheze, "Terminal": "Overtime kicks the referee into action".
  https://ufo50.miraheze.org/wiki/Terminal
- [MH-META] Miraheze, "Meta Messages": the Morse in the Ice World boss's
  border. https://ufo50.miraheze.org/wiki/Meta_Messages
- [TWO-SKILLS] Steam guide "The Two Skills of Kick Club": the ball going
  dark, holding too long, the shooters, staying above them, the bouncing
  footballs. https://steamcommunity.com/sharedfiles/filedetails/?id=3378371530
- [STEAM-SCORE] Steam thread "[Kick Club] How to raise your score": the
  bright and dark ball, "more than 1 second", charged shots, desserts, the
  transitions. https://steamcommunity.com/app/1147860/discussions/0/4849903998515571662/
- [STEAM-BUG] Steam thread "[Kick Club] Two balls bug": kicks against a
  wall. https://steamcommunity.com/app/1147860/discussions/1/4638240322751699409/
- [GGC] Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [LIZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 11".
  https://lizstar64.github.io/reviews/2024/10/11/UFO50-11.html
- [POPCAR] Popcar's Blog, "Reviewing Every Single UFO 50 Game".
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [STATIC] Static Canvas, "The UFO 50 Diaries: Kick Club" (crouching).
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-kick-club
- [HANS] thehans255, "One unique thing about every UFO 50 game".
  https://www.thehans255.com/blog/2024/10/one-unique-thing-about-every-ufo-50-game/
- [PUNISHED] Punished Backlog, "Best UFO 50 games" (stiff in the air).
  https://punishedbacklog.com/best-ufo-50-games/
- [QT3] Quarter to Three forum, UFO 50 thread (trajectories, charge shots,
  slide-kicks, headers).
  https://forum.quartertothree.com/t/ufo-50-50-best-games-for-the-fake-lx-console-from-the-mid-80s/162406/203
- [RESET-37] ResetEra UFO 50 thread, page 37 (no continues; the last world).
  https://www.resetera.com/threads/ufo-50-ot-50-games-for-the-price-of-1.981834/page-37
- [SEARCH-MANUALS] search summaries of the Steam guide "The missing manuals"
  (A jumps, fixed height, tap and hold, the d-pad, dribbling); the guide
  itself answered HTTP 429 again on 1 October 2026.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [SEARCH-TVT] search summaries of TV Tropes' recap (the slide ignores
  gravity, wrap pits, the dual boss, bosses burst into five, the referee
  stays). https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game11KickClub
- [SEARCH-USA], [SEARCH-SRC] search summaries of player comments on the
  last world and of speedrun.com's run pages (run lengths, the cherry
  category).
- [CONV] the collection's conventions, `00-ufo50-conventions.md` in the
  research folder.
