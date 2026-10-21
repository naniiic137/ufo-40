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
and could not be re-read, so their facts come from the research notes. A
third pass on 2026-10-21 read the Steam guide "The missing manuals" [MM],
the strongest source on the controls, and the owner played the original
to confirm that the controls change with each owl; the controls and the
jailbreak below follow it as closely as we can. No UFO 50 images, video,
sprites, music, stage layouts or text were used as references, and
nothing was taken from a UFO 50 install.

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
| Rescues | random, turn-based block-push stealth, card then cell, two skills per owl on cooldowns, a side panel, guards that shoot [MH], [GUIDE], [MM] | the same: three rooms on a 15 x 10 grid beside the bat's panel, the card ringed by guards, two skills per bat |
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
| FLINT: the jump | one fixed jump, 40 px, a tap and a hold the same; LEFT / RIGHT steer it in the air, but he faces the way he took off until he lands | [MH], [MM] | eko_02 |
| FLINT: blows | A, A, A: punch, punch, kick; DOWN + A a crouching jab; A in the air a jump kick | [MH], [GUIDE], [MM] | eko_03 |
| FLINT: the charge | hold A standing still until he flashes (40 frames), let go: a ki blast hurled a short way forward (96 px) that goes through every foe in its path; walking or crouching throws the charge away | [MM], [MH] | eko_04 |
| FLINT: barrels | DOWN + A beside one lifts it, A throws; it floors a foe and bounces up, and a kick sends it on into the next | [MH], [GUIDE] | eko_05 |
| FLINT: multi-kills | each extra foe one ki blast or one barrel takes is worth one more times its points | [MH], [GUIDE] | eko_04, eko_05 |
| FLINT: the key | a set trooper (red beret) carries the key; the locked wall at the end of the docks blows once it is used. Knock him down off the screen (past an edge, or down a pit) and the key never appears: the wall stays shut for good, the original's bug, kept on the owner's word | [GUIDE], [STEAM-SL] | eko_06 |
| FLINT: secrets | a mango in the crates before the lift; a mango behind a wall that gives near the end of the yards | [GUIDE], [STEAM-CO] | eko_06 |
| FLINT: foes | troopers (punch or shoot), grenadiers (lob grenades that spoil fruit on the ground), jetpackers (hover, dive) | [GUIDE] | eko_07 |
| FLINT: the lift and Scrapjaw | waves drop in while the lift climbs; Scrapjaw swings a hook, charges across and tosses grenades | [GUIDE] | eko_07 |
| VESPER: moving and aiming | LEFT / RIGHT walk and dodge, UP / DOWN raise the sight; she can only shoot within a stretch in front of her (the sight stays within 96 px of her) | [MH], [MM] | eko_08, eko_29 |
| VESPER: fire | a tap shoots at once and fast tapping fires fastest (every 5 frames); held, slower (every 11) and planted: the pad then steers the sight | [MH], [TIPS], [MM] | eko_08 |
| VESPER: picking up | standing on an item, a tap of A picks it up instead of shooting (a mango); a grenade lobbed at her lands and lies with its fuse burning (half a second): a tap picks it up, the next throws it back at the sight, where it goes off; held too long, it goes off in her hands | [MM], [GUIDE] (a comment) | eko_29 |
| VESPER: the dive | B dives aside, safe while it lasts | [MM] | eko_10 |
| VESPER: combo meter | kills feed it; it multiplies the points (one more for every three kills in a row) and runs out if she stops scoring | [MH], [GUIDE] | eko_09 |
| VESPER: foe fire | aimed at where she stood; step aside to dodge | [MH] | eko_10 |
| VESPER: red barrels, green trucks | a barrel's blast takes everything near it (bosses too); a truck leaves a mango | [GUIDE] | eko_11 |
| VESPER: two bosses at once | Ramrod paces the near row with spreads and shells, Vultra swoops dropping bombs; either left alone gets angrier | [TVT-S], [LIZ] | eko_11 |
| DART: the road | scrolls on its own; four-way movement; B jumps (every stretch is road, so the jump is always there); the gun fires ahead in the bike's lane | [MH], [MM] | eko_13 |
| DART: heat | a tap fires at once and never heats (tapping is fastest); holding fires slower and heats the barrel; held too long, the bike overheats and glows red while the gun holds half a second; still held when it comes back, the bike blows up at once and a life is gone; a break in the fire saves it | [MH], [GUIDE], [TIPS], [MM] | eko_12 |
| DART: traffic | cones (no points), runners (few), vehicles that come and go, the semi truck (500), green trucks (mango) | [MH], [GUIDE] | eko_13 |
| DART: Hog Wild | shotgun spreads, mines rolled back, a ram down the bike's lane | [TVT-S] | eko_14 |
| SABLE: frail | four pips, a blink of safety (20 frames) after a hit | [MH], [GUIDE] | eko_18 |
| SABLE: the kit | 3 darts and 3 pistol rounds to start; more rounds, stun grenades, grenades and an extra life found on site; A uses the item in hand, B picks the next she carries | [MH], [GUIDE], [MM] | eko_15 |
| SABLE: eight ways | the pad walks her all eight ways, but she faces only four (the newest way pressed) and her items go only straight that way, never on a slant | [MM] | eko_30 |
| SABLE: the kit on screen | bottom left: the item in hand with its uses left, the others she carries, and the one B picks next marked | [MM] | eko_30 |
| SABLE: guards | on their beats; hear a shot and come to look, then go back; see her and hunt and shoot; leaving the room makes them forget | [GUIDE], [TIPS], [LIZ] | eko_15, eko_16 |
| SABLE: cameras and lights | one that sees her sets off every guard in the room | [GUIDE] | eko_16 |
| SABLE: room score | +100 for a room left unseen, 50 off for a noise, nothing if seen | [MH], [GUIDE] | eko_17 |
| SABLE: cards and walls | green, red and blue doors (only green is needed for Cipher); a grenade opens a cracked wall | [GUIDE], [STEAM-AE] | eko_18 |
| SABLE: Cipher | 26: two grenades (10) and a couple of shots (3) | [GUIDE] | eko_18 |
| Spider robot | the Caldera's walking robot: long sight, quicker, darts and stun bounce off | [STEAM-AE], [STEAM-SB] | play |
| Jailbreak | random rooms each time; one step a turn (four ways only), then the guards; crates push; card, then cell | [MH], [GUIDE], [STATIC], [MM] | eko_19, eko_21 |
| Jailbreak guards | they react to what they see along their line (or a noise); seen, a guard takes a turn to line up its gun (the line shows), then fires down it at whatever stands first there: the bat, or another guard (lure them into each other) | [MM] | eko_19 |
| Jailbreak skills | two per bat (see Readings), A and B; used, a skill comes back after a few turns; pressed before then, the bat only waits a turn; Dart has 4 health, a gun that drops anything and a dash that hurts | [MM], [GUIDE], [TVT-S] | eko_20 |
| Jailbreak panel | the left side: the bat, its health now and at most, its two skills with their buttons; a used skill's icon turns into the turns left until it is back | [MM] | eko_20 (the counts), the shots |
| Jailbreak lost | the rescuer out of health is locked up too; all four locked up, the game is over | [STATIC], [MM] | eko_21, eko_22 |
| The Caldera | no second chances: a bat's last life lost there ends the game; Sable hands over to Flint, Dart and Vesper, whose parts earn the cards | [TVT-S], [GUIDE] | eko_23 |
| Goals | gift: win a jailbreak; gold: bring down Madame Coil; cherry: do it without ever emptying a health bar, anywhere, the Caldera included | [MH], [GGC], [STEAM-CHERRY] | eko_24 |
| No saving | a run is never saved; the records are | [STEAM-SAVES] | eko_28 |
| Playable start to end | the demo player, with real presses from the title, finishes the game, and does it once with no life lost; it wins every rolled jail it is given | | eko_25, eko_26, eko_27 |

### Readings we had to choose

- **The buttons:** every one is now named by the manual [MM]; only the
  owner's layout differs (main action on A, see Controls).
- **Jailbreak skills no source names** (only Huxley's are known: 4
  health, a gun that kills anything in one shot, a dash that hurts):
  Flint punches (A, the guard in front is out; back in 2 turns) and
  braces (B, the next shot that turn bounces off; 4); Vesper shoots (A,
  the first guard down her line, loud; 3) and rolls two tiles (B; 2);
  Dart's gun drops the first guard down his line (A, loud; 2) and his
  dash goes up to three tiles, felling a guard in the way (B; 3); Sable
  darts a guard asleep for six turns (A, silent; 3) and whistles to bring
  guards within six tiles to look (B; 2). The number is the turns a skill
  takes to come back after use. A or B pressed while that skill is still
  coming back: the bat waits a turn (so it can never be boxed in for
  good). Health in a jailbreak: Flint 3, Vesper 2, Dart 4 (the guide's
  number), Sable 2.
- **Jailbreak guards:** a guard that sees the bat lines up its gun at the
  end of that turn and fires on its next one ("often take a turn to
  act" [MM]); a hunting guard with a straight, open line to the bat lines
  up, otherwise it closes in; a shot hits the first thing in the line
  (the bat, any guard, asleep or not) and a crate or wall stops it. The
  grid is 15 x 10 in three rooms, right of the panel; a rolled jail is
  checked for a way to the card and back, at least two guards round the
  card and nobody watching the start, and the demo player wins each of
  160 rolled jails tried (40 a bat; the tests replay 37 of them).
- **The jump:** the manual's "a road vehicle" means the bike must be on a
  road to jump; every stretch of both our courses is road, so the jump is
  always there.
- **The Caldera's lives:** every bat starts it with two; Wells' "no
  retries" is read as: losing any bat's last life there ends the game.
- **Extra lives:** one every 800 points of a mission (the guide mentions
  about 800 points in the Moscow part).
- **The key bug:** knocking the original's key soldier off-screen can lose
  the key for good [STEAM-SL]. The owner chose (14/10) to keep it: ours
  is lost when the key trooper goes down outside the screen (past either
  edge, or down a pit), and the wall then stays shut. As in the original,
  the way on is to lose both lives (and be broken out) or quit the run.
  The demo player steers clear of it.
- **Where things are:** the key wall at the end of the docks; the lab climb
  for the mid-boss (Dr. Talpa); the final boss as a gunship over the
  launch pad (the "missile boss" players mention).
- **Numbers no source gives:** Flint walks 1.25 px a frame, jumps 40 px,
  charges in 40 frames, the ki blast flies 4 px a frame for 24 frames (96
  px) and hits for 4 (a kick is 2); Vesper walks 1.5, locks after
  holding 8 frames, dives 22 frames, shoots within 96 px of herself, a
  grenade lies 30 frames before it goes off (picked up at the last
  moment it still gives her 16), and thrown back it flies 16 frames and
  blasts 28 px round (3, bosses 6); Dart moves 1.5, the road scrolls 3, a
  jump is 28 px high, heat 300 (rising 3 a frame once A has been held 10
  frames, falling 2), glowing red for 30 frames before the blow-up;
  Sable walks 1 (a diagonal 0.71 each way), guards see 84 px within 28
  degrees (spider robots 124), a dart sleeps 10 s, a stun 5 s; boss
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
| FLINT | LEFT / RIGHT, DOWN | walk, crouch; in the air LEFT / RIGHT only steer |
| | A (A, A) | punch, punch, kick; in the air a jump kick |
| | DOWN + A | crouching jab, or lift a barrel (A throws it) |
| | A held standing still until he flashes, let go | the ki blast |
| | B | jump (one height) |
| VESPER | LEFT / RIGHT, UP / DOWN | walk and dodge, raise and lower the sight |
| | A tapped | shoot (fastest when tapped fast); standing on an item, pick it up; a grenade in hand, throw it at the sight |
| | A held | keep shooting, planted: the pad steers the sight |
| | B | dive |
| DART | pad | move four ways |
| | A | shoot ahead (holding heats the gun: it glows red, then blows) |
| | B | jump |
| SABLE | pad | walk eight ways (face four) |
| | A / B | use the item in hand (straight ahead) / pick the next |
| Jailbreak | pad | one step, four ways (toward a wall or a guard: turn on the spot) |
| | A, B | the bat's two skills (not back yet: wait a turn) |
| All | START | pause |

## Not confirmed

Confirmed on 2026-10-21 by "The missing manuals" [MM] and taken off this
table: Octavio's charge (hold the attack button, flash, a ki projectile),
his facing through a jump, Engle's button 2 (a dive), his pick-up and
his limited shooting area, Huxley's overheat (glows red, then explodes),
Guin's button 2 (next item), her eight-way walk and straight-only items
and the item icons, and the jailbreak's cooldowns, side panel, four-way
steps and shooting, indiscriminate guards.

| Control or rule | Our reading |
|---|---|
| The main button | button 1 in the original; A here (owner's layout, swappable) |
| Octavio's, Engle's and Guin's jailbreak skills | as in Readings |
| Every skill's cooldown length | as in Readings (2 to 4 turns) |
| How the base's lives work | two each; a last life lost there ends the game |
| Waiting a turn in a jailbreak | a skill pressed before it is back |
| Every speed, time and score in Readings | as listed |

## Tests

`tests/eko_01` … `eko_30` drive each rule with button presses, or set up a
moment with cheats and then play it. The demo player (`eko_bot.c`) reads
the game the way a player reads the screen and answers with real
presses: in the action pieces it tries a few dozen ways to play the next
second (walk, jump at different moments, kick, charge, aim, dive, change
lanes, wait, dart a guard...) in a copy of the game run ahead with the
real rules, keeps the one that stays healthiest and gets furthest, and
sticks with a plan until its next step; in Sable's halls it walks a
breadth-first path to the next card, supply or handover point; in a
jailbreak it looks five turns ahead (turning on the spot before a skill
when that helps, and taking a knock for headway when it has stalled).
`eko_25` wins five rolled jails with button presses and 32 more played
out turn by turn, eight for each bat; `eko_26` plays from the title
through all five missions to the ending; `eko_27` does it with no health
bar ever emptied.

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
- [MM] Steam guide "The missing manuals - How to play UFO 50 games": each
  owl's controls, Engle's pick-up and shooting area, Huxley's overheat,
  Guin's eight ways and item icons, the jailbreak's panel, cooldowns,
  four-way steps and shooting guards.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
