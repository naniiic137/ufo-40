# 50 · ECHO UNIT

*Internal design document. Not shown in the product.*

## Tribute to

**Cyber Owls** (UFO 50 game #50, Mossmouth, "July 1989"). The rules were
researched from text only: the community wiki's raw page and its Cheats
page, the Steam guides "Cyber Owls" (Kapitan Jack), "Tips & Tricks for
Every Game", "Gift, Gold & Cherry" and "The missing manuals", Steam
threads, written reviews and TV Tropes (listed under Sources; the full
notes are in the research folder, `50-cyber-owls.md`). A second pass on
2026-10-14 re-read the wiki and two reviews. A third pass on 2026-10-21
read "The missing manuals" [MM], the strongest source on the controls,
and the owner played the original to confirm that the controls change
with each owl. A fourth pass the same day followed an independent review
(`ufo40-review/50-review.md`), which read the whole TV Tropes recap
[TVT] for the first time (the richest source on the jailbreak), the
Kapitan Jack guide with its comments [KJ], the tips guide [TIPS] and the
reviews: the jailbreak, Flint's stages, Sable's guards, the gallery's
meter and the Caldera's size were rebuilt on what they say. No UFO 50
images, video, sprites, music, stage layouts or text were used as
references, and nothing was taken from a UFO 50 install.

**Map type: fixed missions, random jailbreaks.** Cyber Owls' four city
missions and its Antarctica base are hand-made and the same every run; the
rescue missions are rolled anew each time [MH]. Ours are the same: the
four cities and the Caldera are fixed (`eko_levels.c`, all our own
screens, rooms, waves, roads and skies), and every jailbreak is rolled
(`eko_rescue.c`). Only the structure follows the original:

| Full scale | Cyber Owls | ECHO UNIT |
|---|---|---|
| Players | 1, no 2-player mode [MH], [TVT] | 1 (nothing to grey out) |
| Team | four owls, one genre each, plus Commander Wells [MH] | four bats, one genre each, plus Colonel Hush |
| City missions | 4, chosen freely; the base after all four [KJ], [LIZ] | 4 on a mission board, any order; the Caldera opens when all four are done and nobody is locked up |
| Brawler (Octavio, Chicago) | two parts, an elevator section, Pyrat; enemies keep coming; hazards with "!" [KJ], [MM] | FLINT in Marseille: the docks (8 screens) and the container yards (8), the crane lift (a 45 s ride of 12 waves), Scrapjaw on the roof; troopers keep coming while he lingers; crane loads and a crane hook with "!" |
| Gallery (Engle, Congo) | the shortest, about 2.5 min, two bosses at once [LIZ], [TVT] | VESPER in the Andes: 86 spawns over about 1 min 40 s, then Ramrod (ground) and Vultra (air) together: about 2.5 min |
| Bike (Huxley, Hong Kong) | one long autoscroll, Road Toad [MH], [TVT] | DART in Cairo: 26,200 px of ring road (about 2 min 25 s), Hog Wild at the end |
| Stealth (Guin, Moscow) | a big maze, green card to the boss, Hackoon [KJ] | SABLE in Istanbul: 18 rooms (green, red and blue doors, a cracked wall), Cipher at the bottom |
| Finale | Antarctica: a second half, maybe longer than the first four together, 90% stealth [POP], [CO]; Guin's halls with sections for the others that earn keycards; Octavio's lab climb and Dr. Dillo; Huxley's flying stage with the missile boss; the final boss fought by Engle [MH], [KJ], [SB], [CO], [TVT] | the Caldera: Sable's halls (32 rooms in three zones); Flint's Magma Works (ten floors and a walk along the top to Dr. Talpa) earns the red card; Dart's flight over the lava sea (27,200 px) ends with Major Rattle's missile jet and earns the blue card; Vesper fights Madame Coil's gunship at the launch pad |
| Lives | 2 a mission [MH-CHEATS] | 2 a mission (two health bars) |
| Capture | all lives lost: captured, mission locked until another owl frees it [TVT], [STATIC] | the same |
| Rescues | random small maps, turn-based block-push stealth, card then cell, two skills per owl on cooldowns, a side panel, five colours of guard with health, shots that fly [MH], [KJ], [STATIC], [MM], [TVT] | the same: one to three rolled rooms on the grid beside the bat's panel, the cell in any outer wall, the card anywhere well away from it, ringed by guards |
| Run length | speedruns about 16 min; first clears take hours [SR], [LIZ] | the demo player's full run is about 25 min: Marseille 2.3, the Andes 2.5, Cairo 2.6, Istanbul 4.3 (the cities 12.0), the Caldera 12.8 |
| Bad ending | a picture of the doomsday weapon going off, and a caption [TVT] | the Dawn Cannon's beam landing on a city at noon, and our own caption |
| Saving | none [STEAM-SAVES] | none; only the records |
| Stats | Chicago, Congo, Hong Kong, Successful Rescues [MH] | best scores in Marseille, the Andes and Cairo, and successful rescues (on the title) |
| Goals | 3 [MH], [GGC] | the same 3 |

## Mechanics checklist

| Mechanic | How ECHO UNIT does it | Source | Test |
|---|---|---|---|
| Choosing missions | a board of the four bats (READY, DONE, CAPTURED) and the Caldera (LOCKED until all four are done and free); Colonel Hush briefs each mission | [KJ], [LIZ], [TVT] | eko_01 |
| Two lives a mission | each mission (and each bat in the Caldera) starts with two; a life is a full health bar: FLINT 8, VESPER 6, DART 6, SABLE 4 | [MH-CHEATS], [KJ] | eko_22 |
| Extra lives | found (a red bat-heart), and one for every 800 points of a mission | [KJ] | eko_22 |
| Health | mangos (from crates, green trucks, cargo drones, hidden spots) give back 4 | [KJ], [TVT] | eko_11, eko_13 |
| Capture | both lives gone: the bat is captured and its mission locked until another bat breaks it out; with all four caught, game over | [TVT], [STATIC], [KJ] | eko_22 |
| FLINT: the jump | one fixed jump, 40 px, a tap and a hold the same; LEFT / RIGHT steer it in the air, but he faces the way he took off until he lands | [MH], [MM] | eko_02 |
| FLINT: blows | A, A, A: punch, punch, kick; DOWN + A a crouching jab; A in the air a jump kick | [MH], [KJ], [MM] | eko_03 |
| FLINT: the charge | hold A standing still until he flashes (40 frames), let go: a ki blast hurled a short way forward (96 px) that goes through every foe in its path, and he lunges 28 px forward with it (off a ledge, too); walking or crouching throws the charge away | [MM], [MH], [LIZ] | eko_04 |
| FLINT: against shots | the ki blast wipes out the bullets in its path; a jump kick that meets a grenade sends it back at the nearest foe ahead, and its blast is his | [TIPS] | eko_31 |
| FLINT: barrels | DOWN + A beside one lifts it, A throws; it floors a foe and bounces up, and a kick sends it on into the next | [MH], [KJ] | eko_05 |
| FLINT: multi-kills | each extra foe one ki blast, one barrel or one kicked-back grenade takes is worth one more times its points | [MH], [KJ] | eko_04, eko_05 |
| FLINT: they keep coming | while he lingers, another trooper (or grenadier) walks on from either edge, or a jetpacker drops in, every 4 s while fewer than three are about; never in a boss arena or the lift (it has its waves) | [MM], [XEN] | eko_31 |
| FLINT: hazards | a red "!" first (45 frames), then it comes: a crane load drops where the "!" stood at the top of the screen (2), a crane hook sweeps in at chest height from where the "!" stood at the right edge (1; crouch under it or jump), a lump of slag drops in the lab (2); they hit foes too. Three in each Marseille part, six in the lab | [MM], [TVT] | eko_31 |
| FLINT: the key | a set trooper (red beret) carries the key; the locked wall at the end of the docks blows once it is used. Knock him down off the screen (past an edge, or down a pit) and the key never appears: the wall stays shut for good, the original's bug, kept on the owner's word | [KJ], [STEAM-SL] | eko_06 |
| FLINT: secrets | a mango in a cracked block under a crane in the docks; a mango in a crate stack up on a container in the yards | [KJ], [CO] (spots our own) | eko_06 |
| FLINT: foes | troopers (punch or shoot), grenadiers (lob grenades that spoil fruit on the ground), jetpackers (hover, shoot down at him, dive), dock rats (run low and bite: a standing punch goes over them, a crouching jab, a barrel or a jump kick gets them) | [KJ], [TVT] | eko_07, eko_31 |
| FLINT: gulls | gulls sit on bollards, two in each Marseille part; every one never hit is worth 100 when the part ends | [TVT] (cats there, gulls here) | eko_31 |
| FLINT: the lift and Scrapjaw | waves drop in while the lift climbs (the jetpackers shooting down; a crate of fruit drops in once); Scrapjaw swings a hook, charges across and tosses grenades | [KJ] | eko_07 |
| VESPER: moving and aiming | LEFT / RIGHT walk and dodge, UP / DOWN raise the sight; she can only shoot within a stretch in front of her (the sight stays within 96 px of her) | [MH], [MM] | eko_08, eko_29 |
| VESPER: fire | a tap shoots at once and fast tapping fires fastest (every 5 frames); held, a little slower (every 7) and planted: the pad then steers the sight | [MH], [TIPS], [MM] | eko_08 |
| VESPER: picking up | standing on an item, a tap of A picks it up instead of shooting (a mango); a grenade lobbed at her lands and lies with its fuse burning (half a second): a tap picks it up, the next throws it back at the sight, where it goes off; held too long, it goes off in her hands | [MM], [KJ] (a comment) | eko_29 |
| VESPER: the dive | B dives aside, safe while it lasts | [MM] | eko_10 |
| VESPER: combo meter | kills feed it; it multiplies the points and the damage of her shots (one more for every three kills in a row) and runs out if she stops scoring | [MH], [KJ], [TVT] | eko_09 |
| VESPER: tanky foes | riflemen take two shots, mortar crews three; a red barrel takes ten, cracking as it softens, so it can be worn down and set off when a boss comes near | [KAYIN], [KJ] | eko_09, eko_11 |
| VESPER: foe fire | aimed at where she stood; step aside to dodge | [MH] | eko_10 |
| VESPER: red barrels, green trucks | a barrel's blast takes everything near it (bosses too); a truck leaves a mango | [KJ] | eko_11 |
| VESPER: two bosses at once | Ramrod paces the near row with spreads and shells, Vultra swoops dropping bombs; either left alone gets angrier | [TVT], [LIZ] | eko_11 |
| DART: the road | scrolls on its own; four-way movement; B jumps (every stretch is road, so the jump is always there); the gun fires ahead in the bike's lane | [MH], [MM] | eko_13 |
| DART: the flight | in the Caldera the bike is off the road: it flies anywhere over the lava sea and there is no jump; rock pillars hang from the roof and rise from the lava (turrets on their tips), lava spouts, falling rocks, drones, a cargo drone with a mango | [MM], [CO], [SB] | eko_33 |
| DART: Major Rattle | the flight's boss: a missile jet that fires fans of straight missiles and pairs that home in for a while (shootable), and rams across; down, it gives the blue card | [CO], [SB] | eko_33 |
| DART: heat | a tap fires at once and never heats (tapping is fastest); holding fires a little slower (every 6 frames, not 4) and heats the barrel; held too long, the bike overheats and glows red while the gun holds half a second; still held when it comes back, the bike blows up at once and a life is gone; a break in the fire saves it | [MH], [KJ], [TIPS], [MM] | eko_12 |
| DART: traffic | cones (no points), runners (few), vehicles that come and go, the semi truck (500), green trucks (mango) | [MH], [KJ] | eko_13 |
| DART: Hog Wild | shotgun spreads, mines rolled back, a ram down the bike's lane | [TVT] | eko_14 |
| SABLE: frail | four pips, a blink of safety (20 frames) after a hit | [MH], [KJ] | eko_18 |
| SABLE: the kit | 3 darts and 3 pistol rounds to start; more rounds, stun grenades, grenades and an extra life found on site, up to twenty of each; A uses the item in hand, B picks the next she carries | [MH], [KJ], [MM] | eko_15, eko_32 |
| SABLE: eight ways | the pad walks her all eight ways, but she faces only four (the newest way pressed) and her items go only straight that way, never on a slant | [MM] | eko_30 |
| SABLE: the kit on screen | bottom left: the item in hand with its uses left, the others she carries, and the one B picks next marked | [MM] | eko_30 |
| SABLE: thrown things | a stun grenade goes off without a sound; lobbed, stun grenades and grenades fly over crates and walls (never out of the room) and burst where they land | [KJ] | eko_32 |
| SABLE: guards | on their beats; hear a shot and come to look, then go back; walk close by one, out of its sight, and it comes to look where she was; see her and hunt her; leaving the room makes them forget | [KJ], [TIPS], [LIZ] | eko_15, eko_16, eko_32 |
| SABLE: hunting guards | not clever: one keeps the way it first saw her for a second, fires along it, then looks round; it walks slower than she does, so she can run past | [TVT], [LIZ] | eko_32 |
| SABLE: guards and guards | a guard walking past a darted friend wakes it; a guard's shot hits a guard in the way (a stun, not a kill) | [KJ] | eko_32 |
| SABLE: cameras and lights | one that sees her sets off every guard in the room; a pistol round breaks a camera, and a room's fuse box (shot, or blown with a grenade) puts out its cameras and searchlights | [KJ] | eko_16, eko_32 |
| SABLE: spider robots | long sight, darts and stuns bounce off, two pistol rounds; seeing her, one charges straight at her until something stops it, running down any guard in its way (2 to her) | [SB], [STEAM-AE] | eko_32 |
| SABLE: room score | +100 for a room left unseen, 50 off for a noise, nothing if seen | [MH], [KJ] | eko_17 |
| SABLE: cards and walls | green, red and blue doors (only green is needed for Cipher); a grenade opens a cracked wall | [KJ], [STEAM-AE] | eko_18 |
| SABLE: Cipher | 26: two grenades (10) and a couple of shots (3) | [KJ] | eko_18 |
| Jailbreak | random small maps each time; one step a turn (four ways only), then the guards' side; crates push; card, then cell | [MH], [KJ], [STATIC], [MM] | eko_19, eko_21 |
| Jailbreak guards | five kinds by colour, with health: green with a gun (walks a beat; seen, chases and fires a shot after each move), green with a knife (seen, closes in every turn and stabs), red (stands; seen, shoots along any open line), orange (seen, backs off and lobs grenades that blow up its own side too), purple (seen, a shot, then a jump to two tiles off the bat), grey gatling (spins up a turn, then fires four turns the same way; double health); seen, a guard raises its "!" and acts on it the turn after | [TVT], [MM] | eko_19 |
| Jailbreak shots | a guard's shot flies three tiles a turn and hits the first thing it comes to, the bat or another guard (lure them into each other); step out of its line, or dash across it | [TVT], [MM] | eko_19 |
| Jailbreak skills | two per bat on A and B; used, a skill comes back after a few turns, and pressed before then it does nothing; Flint's punch knocks a guard two tiles back and down a row; Dart's dash goes exactly two tiles, through guards; Sable's dart sleeps three turns (a Z gone each turn); Sable's cloak hides her up to five turns and recharges as long as it was on (four at most) | [MM], [KJ], [TVT] | eko_20 |
| Jailbreak blows | pressing toward a guard turns the bat to face it; pressing again strikes it (Sable's knife harder) | [TVT] | eko_20 |
| Jailbreak keycard | it stops two shots; spent, only Vesper's shots go through it | [TVT] | eko_20 |
| Jailbreak panel | the left side: the bat, its health now and at most, its two skills with their buttons; a used skill's icon turns into the turns left until it is back (the cloak shows its turns on) | [MM] | eko_20 (the counts), the shots |
| Jailbreak lost | the rescuer out of health is locked up too; all four locked up, the game is over | [STATIC], [MM] | eko_21, eko_22 |
| The Caldera | no second chances: a bat's last life lost there ends the game. Sable's green zone runs out to the card and back to the handover; Flint climbs the Magma Works for the red card; the red door is back past the card, so Sable's red zone starts with the long way round; Dart's flight earns the blue card; Sable's blue zone ends at the launch pad, where Vesper takes over | [TVT], [KJ], [SB], [POP] | eko_23, eko_28 |
| The bad ending | game over (all four locked up, or a life lost in the Caldera) shows the Dawn Cannon's beam landing on a city at noon, and a caption | [TVT] | eko_23 (the layout) |
| Goals | gift: win a jailbreak; gold: bring down Madame Coil; cherry: all the way without ever emptying a health bar, the Caldera included | [MH], [GGC], [STEAM-CHERRY] | eko_24 |
| No saving | a run is never saved; the records are | [STEAM-SAVES] | eko_28 |
| Playable start to end | the demo player, with real presses from the title, finishes the game, and does it once with no life lost; it wins every rolled jail it is given | | eko_25, eko_26, eko_27 |

### Readings we had to choose

- **The buttons:** every one is named by the manual [MM]; only the owner's
  layout differs (main action on A, see Controls). Dart's B in the flight
  does nothing: the manual's jump is "if on a road vehicle" and no source
  gives the flight a second button.
- **Jailbreak skills no source names:** Flint's brace (B, the shots that
  turn bounce off; 4), Vesper's shot (A, 2 damage down her line, loud; 3)
  and roll (B, two clear tiles; 2). Flint's punch (A; 2) follows the
  recap's knock-back and chain; the punch's own 2 is ours, so a lone guard
  still drops. Dart's gun (A; 2) drops anything [KJ], his dash (B; 3) is
  exactly two tiles through guards, 2 to each [TVT]. Sable's dart (A; 3)
  and cloak (B) follow the recap; pressing B again ends the cloak early
  and takes no turn (ours). The number is the turns a skill takes to come
  back after use. Health in a jailbreak: Flint 3, Vesper 2, Dart 4 (the
  guide's number), Sable 2.
- **Blows in a jailbreak:** every bat strikes a guard it faces by stepping
  into it (1; Sable's knife 2): the recap gives Sable a knife and knife
  guards that stab; the others' blows are ours, and they mean a bat can
  never be boxed in for good (the tile it came from is always free or holds
  a guard it can strike). A skill pressed while cooling does nothing [MM].
- **Jailbreak guards:** the five kinds follow the recap; ours are the
  numbers: health 2 (gatling 4, "double"); a shot flies three tiles a turn
  and does 1 to anyone; a stab does 1; a grenade lands on the bat's tile
  and goes off two turns later, three tiles square, 2 to everyone there; a
  red guard fires every other turn; an orange one backs off within two
  tiles and throws within six, every third turn; a gatling rests two turns
  after its volley. Vesper's and Dart's guns are loud: walking guards
  within five tiles come to look.
- **The jail's shape:** no source gives a size, so the grid is all the
  screen holds beside the panel (15 x 10). Each jail rolls one room, two or
  three (one or two walls with doorways, a few pillars, three to six
  crates), puts the cell in any outer wall, the card at least nine tiles
  from it with two or three guards on its sides, and the bat at least seven
  tiles from the card. It is checked for a way to the card and back (crates
  counted as movable), at least two guards round the card, nobody watching
  or standing near the start, and a free first step; the demo player wins
  each of 400 rolled jails tried (100 a bat; the tests replay 105 of them).
- **Flint's stages:** troopers come every 4 s while fewer than three are
  about; the hazards are ours (a crane load, a crane hook, slag in the
  lab), three in each Marseille part and six in the lab; gulls are our
  version of the recap's cats; the lift drops one crate of fruit in.
- **Sable's guards:** a guard comes to look when she passes within 30 px
  out of its sight; it wakes a sleeper it passes within 20 px; a guard's
  shot stuns another guard for 1.5 s; a hunting guard turns once a
  second, fires every 50 frames and walks 0.875 px a frame (she walks 1);
  a robot charges at 3 px a frame. The guide says cameras and lights can
  be disabled without saying how: ours are a pistol round on a camera and
  a fuse box per watched room.
- **The jump:** the manual's "a road vehicle" means the bike must be on a
  road to jump; every stretch of Cairo is road, and the Caldera's part is
  a flight.
- **The Caldera's lives:** every bat starts it with two; "running out of
  lives will cause an immediate Game Over" [TVT]: losing any bat's last
  life there ends the game.
- **The Caldera's size:** reviewers call it a second half as long as the
  first four sections together, Guin's part "really long" [POP], [LIZ];
  ours is 32 halls in three zones, the Magma Works (ten floors and two
  screens along the top), a 27,200 px flight and a longer launch pad: the
  demo player takes 12.8 min there against 12.0 for the four cities.
- **Extra lives:** one every 800 points of a mission (the guide mentions
  about 800 points in the Moscow part).
- **The key bug:** knocking the original's key soldier off-screen can lose
  the key for good [STEAM-SL]. The owner chose (14/10) to keep it: ours
  is lost when the key trooper goes down outside the screen (past either
  edge, or down a pit), and the wall then stays shut. As in the original,
  the way on is to lose both lives (and be broken out) or quit the run.
  The demo player steers clear of it.
- **Where things are:** the key wall at the end of the docks; the Magma
  Works climb for the mid-boss (Dr. Talpa); the missile boss (Major
  Rattle) at the end of Dart's flight, as players place it [CO], [SB];
  the final boss as a gunship over the launch pad.
- **Numbers no source gives:** Flint walks 1.25 px a frame, jumps 40 px,
  charges in 40 frames, the ki blast flies 4 px a frame for 24 frames (96
  px) and hits for 4 (a kick is 2), and he lunges 2 px a frame for its 14
  frames; Vesper walks 1.5, locks after holding 8 frames, dives 22 frames,
  shoots within 96 px of herself, a grenade lies 30 frames before it goes
  off (picked up at the last moment it still gives her 16), and thrown
  back it flies 16 frames and blasts 28 px round (3, bosses 6); Dart moves
  1.5, the road scrolls 3, a jump is 28 px high, heat 300 (rising 3 a frame
  once A has been held 10 frames, falling 2), glowing red for 30 frames
  before the blow-up; Sable walks 1 (a diagonal 0.71 each way), guards see
  84 px within 28 degrees (spider robots 124), a dart sleeps 10 s, a stun
  5 s, a lob flies 18 frames; boss health: Scrapjaw 36, Dr. Talpa 40,
  Ramrod and Vultra 50 each, Hog Wild 90, Major Rattle 150, Cipher 26, the
  gunship's pods 20, cannon 30, core 60.
- **Points:** troopers 20-50 in Marseille, rats 30, gulls 100 each; 10-50
  in the Andes before the meter; Cairo's runners 10, cars and bikers 20,
  green trucks 50, the semi 500; drones 30, turrets 50, missiles 10;
  bosses 200-500.

## What is ours

- **Name:** ECHO UNIT (1989).
- **The team:** FLINT (a big brown bat in a red bandana), VESPER (grey,
  goggles, a long rifle), DART (an orange kid on a hover-bike), SABLE (the
  black-clad leader), Colonel Hush in the belfry under London.
- **The villains:** V.I.P.E.R., the serpent syndicate, and its Dawn Cannon;
  lizard troopers, dock rats; Scrapjaw the dock croc, Ramrod and Vultra,
  Hog Wild, Cipher the chameleon, Dr. Talpa and his drill, Major Rattle
  and his missile jet, Madame Coil.
- **The places:** Marseille, the Andes, Cairo, Istanbul and the Caldera:
  all 27 brawl screens, 50 rooms, two gallery scripts, a road and a sky.
- **Music:** "Echo Unit", "Hanging in the Belfry", "Sound the Roost",
  "Colonel Hush", "Dockside Dust-Up", "Going Up", "Thin Air", "Sand and
  Throttle", "Silent Halls", "Cell Block", "Big Bad", "Into the Caldera",
  "The Magma Works", "Lava Run", "Madame Coil", "Mango Feast" and three
  jingles.
- **Words:** the story, every briefing, the bad ending's caption, the
  ending, the goals and every label.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
UFO 40 goals, which are Cyber Owls' own. The Terminal codes (endless
jailbreaks, straight to the base, 106 lives) are UFO 50 collection
features; UFO 40 has no terminal, so they are left out. The gulls, the
fuse boxes and the lift's crate of fruit are our own versions of things
the sources describe (cats on fences, cameras that can be disabled,
health found in stages), not new rules.

## Controls

The owner's layout puts every bat's main action on **A**; Cyber Owls has
it on button 1. The cartridge card's CONTROLS page can swap them back.

| Bat | Input | Action |
|---|---|---|
| FLINT | LEFT / RIGHT, DOWN | walk, crouch; in the air LEFT / RIGHT only steer |
| | A (A, A) | punch, punch, kick; in the air a jump kick (it sends grenades back) |
| | DOWN + A | crouching jab, or lift a barrel (A throws it) |
| | A held standing still until he flashes, let go | the ki blast (he lunges with it) |
| | B | jump (one height) |
| VESPER | LEFT / RIGHT, UP / DOWN | walk and dodge, raise and lower the sight |
| | A tapped | shoot (fastest when tapped fast); standing on an item, pick it up; a grenade in hand, throw it at the sight |
| | A held | keep shooting, planted: the pad steers the sight |
| | B | dive |
| DART | pad | move four ways (in the air, anywhere) |
| | A | shoot ahead (holding heats the gun: it glows red, then blows) |
| | B | jump (on the road) |
| SABLE | pad | walk eight ways (face four) |
| | A / B | use the item in hand (straight ahead) / pick the next |
| Jailbreak | pad | one step, four ways (toward a wall: turn on the spot; toward a guard: turn to face it, then strike) |
| | A, B | the bat's two skills (not back yet: nothing happens); Sable's B again ends her cloak |
| All | START | pause |

## Not confirmed

Confirmed on 2026-10-21 by "The missing manuals" [MM] and taken off this
table: Octavio's charge (hold the attack button, flash, a ki projectile),
his facing through a jump, Engle's button 2 (a dive), his pick-up and
his limited shooting area, Huxley's overheat (glows red, then explodes),
Guin's button 2 (next item), her eight-way walk and straight-only items
and the item icons, and the jailbreak's cooldowns, side panel, four-way
steps and shooting, indiscriminate guards. Confirmed the same day by the
recap [TVT] and taken off: the five guard kinds and their health, Guin's
dart (three turns) and cloak, Octavio's knock-back punch, Huxley's dash,
the keycard's two health, the Caldera's "no retries" and the bad ending.

| Control or rule | Our reading |
|---|---|
| The main button | button 1 in the original; A here (owner's layout, swappable) |
| Octavio's and Engle's second jailbreak skills, Engle's first | as in Readings |
| Every skill's cooldown but Guin's dart and cloak | as in Readings (2 to 4 turns) |
| Bump blows for Octavio, Engle and Huxley | 1 damage; Guin's knife 2 |
| The jail's size and how its rooms are rolled | 15 x 10, one to three rooms |
| Guard shot speed, damage and the grenade's fuse and blast | 3 tiles a turn, 1; two turns, 3 x 3, 2 |
| The keycard on Engle's own shot after it is spent | it goes through (the recap); it still blocks the others |
| How often Octavio's foes come back, the hazards' kinds | every 4 s, at most three; loads, a hook, slag |
| How cameras and lights are disabled | a pistol round on a camera; a fuse box per room |
| How the base's lives work | two each; a last life lost there ends the game |
| What button 2 does in the flight | nothing |
| Every speed, time and score in Readings | as listed |

## Tests

`tests/eko_01` … `eko_33` drive each rule with button presses, or set up a
moment with cheats and then play it. The demo player (`eko_bot.c`) reads
the game the way a player reads the screen and answers with real
presses: in the action pieces it tries a few dozen ways to play the next
second (walk, jump at different moments, kick, charge, aim, dive, change
lanes, wait, dart a guard...) in a copy of the game run ahead with the
real rules, keeps the one that stays healthiest and gets furthest, and
sticks with a plan until its next step (with troopers that keep coming,
it stops counting kills once it has stalled); in Sable's halls it walks a
breadth-first path to the next card, supply or handover point; in a
jailbreak it looks four turns ahead (turning on the spot before a skill
or a blow when that helps, and taking a knock for headway when it has
stalled). `eko_25` wins five rolled jails with button presses and 100
more played out turn by turn, 25 for each bat; `eko_26` plays from the
title through all five missions to the ending; `eko_27` does it with no
health bar ever emptied.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Cyber Owls" (raw page): the four owls and
  their rules, goals, rescues, stats. https://ufo50.miraheze.org/wiki/Cyber_Owls
- [MH-CHEATS] UFO 50 Wiki, "Cheats": two lives a mission (JOES-ERVO),
  endless rescues, straight to the base. https://ufo50.miraheze.org/wiki/Cheats
- [KJ] Steam guide "Cyber Owls" (Kapitan Jack) and its comments: mission
  order, the key, secrets, foes, the elevator's jetpackers, the stealth
  rules (silent stun grenades over walls, investigating, waking, shots on
  guards, cameras that can be disabled, 15ish bullets), red barrels,
  Hackoon, Huxley in rescues.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3339195486
- [TIPS] Steam guide "Tips & Tricks for Every Game": the charge deletes
  some projectiles, the jump kick deflects grenades.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3340576461
- [GGC] Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [MM] Steam guide "The missing manuals - How to play UFO 50 games": each
  owl's controls, enemies that keep spawning and hazards with "!", Engle's
  pick-up and shooting area, Huxley's overheat, Guin's eight ways and item
  icons, the jailbreak's panel, cooldowns, four-way steps and shooting
  guards. https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [TVT] TV Tropes recap (the whole page, fetched with curl on 2026-10-21):
  the jailbreak's guard colours and health, each owl's skills, the keycard,
  the meter as a damage multiplier, the cats and rats in Chicago, the dumb
  Moscow guards, "no retries", the bad ending, the bosses.
  https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game50CyberOwls
- [CO] Steam thread "Cyber Owls" (cherry pain points, the missile boss in
  the flying stage, secret health, the final mission 90% stealth).
  https://steamcommunity.com/app/1147860/discussions/0/4849904345518666568/
- [STEAM-CHERRY] Steam thread "Question regarding Cyber Owl's cherry".
  https://steamcommunity.com/app/1147860/discussions/0/4849904427677025132/
- [STEAM-SL] Steam thread on the Chicago key softlock.
  https://steamcommunity.com/app/1147860/discussions/0/4849904828217378925/
- [STEAM-AE] Steam thread on Antarctica's Guin section and blue door.
  https://steamcommunity.com/app/1147860/discussions/1/604149834549567533/
- [SB] Steam thread "Antarctica Spider Bot Fatal Error": the keys after
  the flying and beat-em-up sequences, spider bots sent crashing.
  https://steamcommunity.com/app/1147860/discussions/1/523083718983684491/
- [STEAM-SAVES] Steam thread on which games save.
  https://steamcommunity.com/app/1147860/discussions/0/4849904176633630339/
- [LIZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 50".
  https://lizstar64.github.io/reviews/2024/10/21/UFO50-50.html
- [POP] Popcar, "Reviewing every UFO 50 game": the final level a second
  half, Guin's part really long. https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [XEN] Xeneth, "Fifty days of UFO 50", day 25: enemies that keep coming.
  https://www.xeneth.design/2024/10/fifty-days-of-ufo-50-day-25.html
- [KAYIN] kayin.moe, "UFO 50": everything in Engle's stage is tanky.
  https://kayin.moe/ufo-50
- [STATIC] Static Canvas, "The UFO 50 Diaries: Cyber Owls".
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-cyber-owls
- [SR] speedrun.com (search listing): Gold 16:09, Cherry 16:40.
