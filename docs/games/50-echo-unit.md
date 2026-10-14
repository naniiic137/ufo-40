# 50 · ECHO UNIT

*Internal design document. Not shown in the product.*

## Tribute to

**Cyber Owls** (UFO 50 game #50, Mossmouth, "July 1989"). The rules were
researched from text only: the community wiki's raw page and its Cheats
page, the Steam guides "Cyber Owls" (Kapitan Jack), "Tips & Tricks for
Every Game" and "Gift, Gold & Cherry", Steam threads, written reviews and
TV Tropes excerpts (listed under Sources; the full notes are in the
research folder, `50-cyber-owls.md`). A second pass on 2026-10-14 re-read
the wiki and two reviews; the Steam guides answered HTTP 429 every time
and could not be re-read, so their facts come from the research notes. No
UFO 50 images, video, sprites, music, stage layouts or text were used as
references, and nothing was taken from a UFO 50 install.

**Map type: fixed missions, random jailbreaks.** Cyber Owls' four city
missions and its Antarctica base are hand-made and the same every run; the
rescue missions are rolled anew each time [MH]. Ours are the same: the
four cities and the Caldera are fixed (`eko_levels.c`, all our own
screens, rooms, waves and roads), and every jailbreak is rolled
(`eko_rescue.c`). Only the structure follows the original:

| Full scale | Cyber Owls | ECHO UNIT |
|---|---|---|
| Players | 1, no 2-player mode [MH], [TVT-S] | 1 (nothing to grey out) |
| Team | four owls, one genre each, plus Commander Wells [MH] | four bats, one genre each, plus Colonel Hush |
| City missions | 4, chosen freely; the base after all four [GUIDE], [LIZ] | 4 on a mission board, any order; the Caldera opens when all four are done and nobody is locked up |
| Brawler (Octavio, Chicago) | two parts, an elevator section, Pyrat [GUIDE] | FLINT in Marseille: the docks (8 screens) and the container yards (8), the crane lift (a 45 s ride of 12 waves), Scrapjaw on the roof |
| Gallery (Engle, Congo) | the shortest, about 2.5 min, two bosses at once [LIZ], [TVT-S] | VESPER in the Andes: 106 spawns over about 2 min 5 s, then Ramrod (ground) and Vultra (air) together |
| Bike (Huxley, Hong Kong) | one long autoscroll, Road Toad [MH], [TVT-S] | DART in Cairo: 26,200 px of ring road (about 2 min 25 s), Hog Wild at the end |
| Stealth (Guin, Moscow) | a big maze, green card to the boss, Hackoon [GUIDE] | SABLE in Istanbul: 18 rooms (green, red and blue doors, a cracked wall), Cipher at the bottom |
| Finale | Antarctica: Guin's stealth hub with sections for the others that earn keycards, mid-boss Dr. Dillo, final boss fought by Engle [MH], [GUIDE], [STEAM-AE] | the Caldera: Sable's halls (10 rooms) in three zones; Flint's secret-lab climb (6 screens high, Dr. Talpa) earns the red card, Dart's lava tunnel the blue one, and Vesper fights Madame Coil's gunship at the launch pad |
| Lives | 2 a mission [MH-CHEATS] | 2 a mission (two health bars) |
| Capture | all lives lost: captured, mission locked until another owl frees it [TVT-S], [STATIC] | the same |
| Rescues | random, turn-based block-push stealth, card then cell, two moves per owl [MH], [GUIDE] | the same: three rooms on a 19 x 9 grid, the card ringed by guards, two moves per bat |
| Run length | speedruns about 16 min; first clears take hours [SR], [LIZ] | the demo player's full run is about 18 min (Marseille 2.3, the Andes 3, Cairo 2.6, Istanbul 4.9, the Caldera 4.9) |
| Saving | none [STEAM-SAVES] | none; only the records |
| Stats | Chicago, Congo, Hong Kong, Successful Rescues [MH] | best scores in Marseille, the Andes and Cairo, and successful rescues (on the title) |
| Goals | 3 [MH], [GGC] | the same 3 |

## Mechanics checklist

| Mechanic | How ECHO UNIT does it | Source | Test |
|---|---|---|---|
| Choosing missions | a board of the four bats (READY, DONE, CAPTURED) and the Caldera (LOCKED until all four are done and free); Colonel Hush briefs each mission | [GUIDE], [LIZ], [TVT-S] | eko_01 |
| Two lives a mission | each mission (and each bat in the Caldera) starts with two; a life is a full health bar: FLINT 8, VESPER 6, DART 6, SABLE 4 | [MH-CHEATS], [GUIDE] | eko_22 |
| Extra lives | found (a red bat-heart), and one for every 800 points of a mission | [GUIDE] | eko_22 |
| Health | mangos (from crates, green trucks, hidden spots) give back 4 | [GUIDE], [TVT-S] | eko_11, eko_13 |
| Capture | both lives gone: the bat is captured and its mission locked until another bat breaks it out; with all four caught, game over | [TVT-S], [STATIC], [GUIDE] | eko_22 |
| FLINT: the jump | one fixed jump, 40 px, a tap and a hold the same | [MH] | eko_02 |
| FLINT: blows | A, A, A: punch, punch, kick; DOWN + A a crouching jab; A in the air a jump kick | [MH], [GUIDE] | eko_03 |
| FLINT: the charge | hold A standing still (40 frames) and let go: a lunge that goes through foes and can carry him into a pit; walking or crouching throws the charge away | [MH], [LIZ] | eko_04 |
| FLINT: barrels | DOWN + A beside one lifts it, A throws; it floors a foe and bounces up, and a kick sends it on into the next | [MH], [GUIDE] | eko_05 |
| FLINT: multi-kills | each extra foe one lunge or one barrel takes is worth one more times its points | [MH], [GUIDE] | eko_04, eko_05 |
| FLINT: the key | a set trooper (red beret) carries the key; the locked wall at the end of the docks blows once it is used | [GUIDE], [STEAM-SL] | eko_06 |
| FLINT: secrets | a mango in the crates before the lift; a mango behind a wall that gives near the end of the yards | [GUIDE], [STEAM-CO] | eko_06 |
| FLINT: foes | troopers (punch or shoot), grenadiers (lob grenades that spoil fruit on the ground), jetpackers (hover, dive) | [GUIDE] | eko_07 |
| FLINT: the lift and Scrapjaw | waves drop in while the lift climbs; Scrapjaw swings a hook, charges across and tosses grenades | [GUIDE] | eko_07 |
| VESPER: moving and aiming | LEFT / RIGHT walk and dodge, UP / DOWN raise the sight | [MH] | eko_08 |
| VESPER: fire | a tap shoots at once and fast tapping fires fastest (every 5 frames); held, slower (every 11) and planted: the pad then steers the sight anywhere | [MH], [TIPS] | eko_08 |
| VESPER: combo meter | kills feed it; it multiplies the points (one more for every three kills in a row) and runs out if she stops scoring | [MH], [GUIDE] | eko_09 |
| VESPER: foe fire | aimed at where she stood; step aside to dodge | [MH] | eko_10 |
| VESPER: red barrels, green trucks | a barrel's blast takes everything near it (bosses too); a truck leaves a mango | [GUIDE] | eko_11 |
| VESPER: two bosses at once | Ramrod paces the near row with spreads and shells, Vultra swoops dropping bombs; either left alone gets angrier | [TVT-S], [LIZ] | eko_11 |
| DART: the road | scrolls on its own; four-way movement; B jumps; the gun fires ahead in the bike's lane | [MH] | eko_13 |
| DART: heat | a tap fires at once and never heats (tapping is fastest); holding fires slower and heats the barrel; boiled over, the bike blows up and a life is gone | [MH], [GUIDE], [TIPS] | eko_12 |
| DART: traffic | cones (no points), runners (few), vehicles that come and go, the semi truck (500), green trucks (mango) | [MH], [GUIDE] | eko_13 |
| DART: Hog Wild | shotgun spreads, mines rolled back, a ram down the bike's lane | [TVT-S] | eko_14 |
| SABLE: frail | four pips, a blink of safety (20 frames) after a hit | [MH], [GUIDE] | eko_18 |
| SABLE: the kit | 3 darts and 3 pistol rounds to start; more rounds, stun grenades, grenades and an extra life found on site; A uses, B picks the next | [MH], [GUIDE] | eko_15 |
| SABLE: guards | on their beats; hear a shot and come to look, then go back; see her and hunt and shoot; leaving the room makes them forget | [GUIDE], [TIPS], [LIZ] | eko_15, eko_16 |
| SABLE: cameras and lights | one that sees her sets off every guard in the room | [GUIDE] | eko_16 |
| SABLE: room score | +100 for a room left unseen, 50 off for a noise, nothing if seen | [MH], [GUIDE] | eko_17 |
| SABLE: cards and walls | green, red and blue doors (only green is needed for Cipher); a grenade opens a cracked wall | [GUIDE], [STEAM-AE] | eko_18 |
| SABLE: Cipher | 26: two grenades (10) and a couple of shots (3) | [GUIDE] | eko_18 |
| Spider robot | the Caldera's walking robot: long sight, quicker, darts and stun bounce off | [STEAM-AE], [STEAM-SB] | play |
| Jailbreak | random rooms each time; one step a turn, then the guards; crates push; seen guards come and strike when next to the bat; card, then cell | [MH], [GUIDE], [STATIC] | eko_19, eko_21 |
| Jailbreak moves | two per bat (see Readings); Dart has 4 health, a gun that drops anything and a dash that hurts | [GUIDE], [TVT-S] | eko_20 |
| Jailbreak lost | the rescuer is locked up too | [STATIC] | eko_21 |
| The Caldera | no second chances: a bat's last life lost there ends the game; Sable hands over to Flint, Dart and Vesper, whose parts earn the cards | [TVT-S], [GUIDE] | eko_23 |
| Goals | gift: win a jailbreak; gold: bring down Madame Coil; cherry: do it without ever emptying a health bar, anywhere, the Caldera included | [MH], [GGC], [STEAM-CHERRY] | eko_24 |
| No saving | a run is never saved; the records are | [STEAM-SAVES] | eko_28 |
| Playable start to end | the demo player, with real presses from the title, finishes the game, and does it once with no life lost | | eko_26, eko_27 |

### Readings we had to choose

- **Buttons no source names:** Vesper's B is a roll (dodge, safe while it
  lasts); Sable's B picks the next weapon; Sable walks four ways.
- **Jailbreak moves no source names:** Flint punches (A, the guard in front
  is out) and braces (B, blocks the next blow); Vesper shoots (three shots,
  loud) and rolls two tiles; Dart shoots (no limit, loud) and dashes up to
  three tiles; Sable darts a guard asleep for six turns (silent, three
  darts) and whistles to bring guards within six tiles to look. Health in
  a jailbreak: Flint 3, Vesper 2, Dart 4 (the guide's number), Sable 2.
  Guards there do not shoot: they strike when next to the bat. The grid is
  19 x 9 in three rooms; a rolled jail is checked for a way to the card
  and back, at least two guards round the card and nobody watching the
  start.
- **The Caldera's lives:** every bat starts it with two; Wells' "no
  retries" is read as: losing any bat's last life there ends the game.
- **Extra lives:** one every 800 points of a mission (the guide mentions
  about 800 points in the Moscow part).
- **The key bug:** knocking the original's key soldier off-screen can lose
  the key for good [STEAM-SL]. Ours always drops the key where he fell.
  (A question for the owner: should the softlock be kept?)
- **Where things are:** the key wall at the end of the docks; the lab climb
  for the mid-boss (Dr. Talpa); the final boss as a gunship over the
  launch pad (the "missile boss" players mention).
- **Numbers no source gives:** Flint walks 1.25 px a frame, jumps 40 px,
  charges in 40 frames, lunges 3.5 px a frame for 18; Vesper walks 1.5,
  locks after holding 8 frames, rolls 22 frames; Dart moves 1.5, the road
  scrolls 3, a jump is 28 px high, heat 300 (rising 3 a frame once A has
  been held 10 frames, falling 2); Sable walks 1, guards see 84 px within
  28 degrees (spider robots 124), a dart sleeps 10 s, a stun 5 s; boss
  health: Scrapjaw 36, Dr. Talpa 40, Ramrod and Vultra 50 each, Hog Wild
  90, Cipher 26, the gunship's pods 20, cannon 30, core 60.
- **Points:** troopers 20-50 in Marseille; 10-50 in the Andes before the
  meter; Cairo's runners 10, cars and bikers 20, green trucks 50, the semi
  500; bosses 200-500.

## What is ours

- **Name:** ECHO UNIT (1989).
- **The team:** FLINT (a big brown bat in a red bandana), VESPER (grey,
  goggles, a long rifle), DART (an orange kid on a hover-bike), SABLE (the
  black-clad leader), Colonel Hush in the belfry under London.
- **The villains:** V.I.P.E.R., the serpent syndicate, and its Dawn Cannon;
  lizard troopers; Scrapjaw the dock croc, Ramrod and Vultra, Hog Wild,
  Cipher the chameleon, Dr. Talpa and his drill, Madame Coil.
- **The places:** Marseille, the Andes, Cairo, Istanbul and the Caldera:
  all 23 brawl screens, 28 rooms, two gallery scripts and two roads.
- **Music:** "Echo Unit", "Hanging in the Belfry", "Sound the Roost",
  "Colonel Hush", "Dockside Dust-Up", "Going Up", "Thin Air", "Sand and
  Throttle", "Silent Halls", "Cell Block", "Big Bad", "Into the Caldera",
  "The Secret Lab", "Lava Run", "Madame Coil", "Mango Feast" and three
  jingles.
- **Words:** the story, every briefing, the ending and every label.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
UFO 40 goals, which are Cyber Owls' own. The Terminal codes (endless
jailbreaks, straight to the base, 106 lives) are UFO 50 collection
features; UFO 40 has no terminal, so they are left out.

## Controls

The owner's layout puts every bat's main action on **A**; Cyber Owls has
it on button 1. The cartridge card's CONTROLS page can swap them back.

| Bat | Input | Action |
|---|---|---|
| FLINT | LEFT / RIGHT, DOWN | walk, crouch |
| | A (A, A) | punch, punch, kick; in the air a jump kick |
| | DOWN + A | crouching jab, or lift a barrel (A throws it) |
| | A held standing still, let go | the charged lunge |
| | B | jump (one height) |
| VESPER | LEFT / RIGHT, UP / DOWN | walk and dodge, raise and lower the sight |
| | A tapped | shoot (fastest when tapped fast) |
| | A held | keep shooting, planted: the pad steers the sight |
| | B | roll |
| DART | pad | move four ways |
| | A | shoot ahead (holding heats the gun) |
| | B | jump |
| SABLE | pad | walk |
| | A / B | use the weapon / pick the next |
| Jailbreak | pad, A, B | step, the bat's two moves |
| All | START | pause |

## Not confirmed

| Control or rule | Our reading |
|---|---|
| The main button | button 1 in the original; A here (owner's layout, swappable) |
| Octavio's charge button | holding the attack button (A) |
| Engle's button 2 | a roll |
| Guin's button 2, and 4 or 8 ways | next weapon; four ways |
| Octavio's and Engle's jailbreak moves; Guin's second | as in Readings |
| How the base's lives work | two each; a last life lost there ends the game |
| The key soldier softlock | not kept (a question for the owner) |
| Every speed, time and score in Readings | as listed |

## Tests

`tests/eko_01` … `eko_28` drive each rule with button presses, or set up a
moment with cheats and then play it. The demo player (`eko_bot.c`) reads
the game the way a player reads the screen and answers with real
presses: in the action pieces it tries a few dozen ways to play the next
second (walk, jump at different moments, kick, charge, aim, roll, change
lanes, wait, dart a guard...) in a copy of the game run ahead with the
real rules, keeps the one that stays healthiest and gets furthest, and
sticks with a plan until its next step; in Sable's halls it walks a
breadth-first path to the next card, supply or handover point; in a
jailbreak it looks four turns ahead. `eko_25` wins five rolled jails, one
for each bat; `eko_26` plays from the title through all five missions to
the ending; `eko_27` does it with no health bar ever emptied.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Cyber Owls" (raw page): the four owls and
  their rules, goals, rescues, stats. https://ufo50.miraheze.org/wiki/Cyber_Owls
- [MH-CHEATS] UFO 50 Wiki, "Cheats": two lives a mission (JOES-ERVO),
  endless rescues, straight to the base. https://ufo50.miraheze.org/wiki/Cheats
- [GUIDE] Steam guide "Cyber Owls" (Kapitan Jack): mission order, the key,
  secrets, foes, the stealth rules, Hackoon, Huxley in rescues.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3339195486
- [TIPS] Steam guide "Tips & Tricks for Every Game".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3340576461
- [GGC] Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [STEAM-CO] Steam thread "Cyber Owls" (cherry pain points, secret health).
  https://steamcommunity.com/app/1147860/discussions/0/4849904345518666568/
- [STEAM-CHERRY] Steam thread "Question regarding Cyber Owl's cherry".
  https://steamcommunity.com/app/1147860/discussions/0/4849904427677025132/
- [STEAM-SL] Steam thread on the Chicago key softlock.
  https://steamcommunity.com/app/1147860/discussions/0/4849904828217378925/
- [STEAM-AE] Steam thread on Antarctica's Guin section and blue door.
  https://steamcommunity.com/app/1147860/discussions/1/604149834549567533/
- [STEAM-SB] Steam thread "Antarctica Spider Bot Fatal Error" (listing).
- [STEAM-SAVES] Steam thread on which games save.
  https://steamcommunity.com/app/1147860/discussions/0/4849904176633630339/
- [TVT-S] TV Tropes recap (search excerpts): bosses, Wells, capture, ending.
- [LIZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 50".
  https://lizstar64.github.io/reviews/2024/10/21/UFO50-50.html
- [STATIC] Static Canvas, "The UFO 50 Diaries: Cyber Owls".
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-cyber-owls
- [SR] speedrun.com (search listing): Gold 16:09, Cherry 16:40.
