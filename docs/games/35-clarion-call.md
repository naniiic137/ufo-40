# 35 · CLARION CALL

*Internal design document. Not shown in the product.*

## Tribute to

**Campanella 2** (UFO 50 game #35, Mossmouth, "October 1987"), the sequel
that turned Campanella's ship into a roguelike with a Blaster Master split:
fly a big map, land, and go through its doors on foot. The rules were
researched from text only: the community wiki's rendered and raw pages,
the Steam guides "The missing manuals" (its full #35 section), "Campanella
2 Guide", "Tips & Tricks" and "Gift, Gold & Cherry", the TV Tropes recap,
Steam threads and written reviews (listed under Sources; the full notes
are in the research folder, `35-campanella-2.md`, and an independent
fidelity review corrected several readings). No UFO 50 images, video, sprites, music,
maps, room layouts or text were used as references, and nothing was taken
from a UFO 50 install.

**Map type: generated.** Every area but the last is generated afresh for
each run, as the original's are [W], [C2-GUIDE] (`clc_gen.c`, all our own
generator and cave pieces); the Spire's third area, the Crown, is fixed, as
Crepelia III is [W]. Only the structure follows the original.

**A sequel to BELLHOP, as Campanella 2 is to Campanella.** The heroine is
Clary, Ansel's sister from CHIME CIRCUIT (her ship, the Clarion, is the
chime ship she races there, in her colours); Ansel is the brother she goes
after; Lady Hush (BELLHOP's last boss) is behind it, as Queen Zu is in the
original. **The Clarion flies on CHIME CIRCUIT's flight model**
(`chime_flight.c`) with BELLHOP's numbers, so the Campanella family feels
the same: gravity always on, a held thrust, a tapped hover, steering with a
long drift, the slash to the side last steered. What Campanella 2 changes
is here: walls bounce the ship back and cost a point at anything over a
gentle nudge; it can land; the bar holds 8; the tank is one tank for the
whole run, and it runs low.

| Full scale | Campanella 2 | CLARION CALL |
|---|---|---|
| Players | 1 [W], [MP] | 1 |
| Areas per run | 9: four regions × two, then a fixed third area of the last [W], [C2-GUIDE] | 9: the Cellars I-II, the Arboretum or the Icehouse I-II, the Gullet, Cogtown or the Cloister I-II, the Spire I-II, the Crown |
| Regions | 7: Burrows first, Moire Woods or the Rink, Gut, Vaalpolis or Temple Grounds, Crepelia last [W] | 7: the Cellars, the Arboretum, the Icehouse, the Gullet, Cogtown, the Cloister, the Spire |
| Generated | every area but Crepelia III [W] | every area but the Crown (2,800 maps checked in the tests) |
| Choosing the way | the station map between regions [MM] | the station map after each region's second area |
| Stars | 10 open the gold door; more lie about; the spares become purple coins (25) [W], [MM] | 14 notes an area; the tenth opens the gold door; the spares become plum coins (25) |
| The dash | 60 on the clock counting double (30 s); rings +200 fuel and a pause; Eggers hatch; at 0 Time Jellies [W] | the same: a clock of "60" over 30 s, clock rings, pods, latecomers |
| Bar | 8, +2 an upgrade; armour halves damage; one bar for ship and pilot [W], [STATIC] | 8 (16 half points); the Heart Pin +2; the Tin Plate halves |
| Fuel | 800, +200 an upgrade; one tank for the run; coins burn when dry [W] | the same |
| Upgrades | 16 (the MEGA-BELL cheat grants "8 of the 16") [W], [CHEATS] | 16 |
| Orb Machines | 3 (Burrows II, regions 2 and 3, second areas) [W] | 3 Hush Engines, in the same places |
| Mini-boss | Rotondo, 12 hits, in every second area but Temple Grounds II [W] | the Lobber, the same |
| Bosses | Queen Zu (100), Klord (100, secret) [W] | Lady Hush (100), Grandsire Tock (100, secret) |
| Sages | each gives a scroll and names the next one's region; three scrolls open the way to Klord [W], [C2-GUIDE] | sextons and peal sheets, the same |
| Endings | 3: without Pilot, with Pilot, Klord beaten [W] | 3 |
| Goals | 3 [W], [GGC] | the same 3, in our words |
| Stat | Gear Bought [W] | the same, with runs, the furthest area and the wins |
| Saving | none: one sitting [ST-SAVES] | the same; only the records are kept |

**How long it takes.** No run times were found [research §4]; players call
it "actually short, the hard part is not dying" and our estimate was
20–45 minutes. The demo player finishes in about 13 minutes of game time
(`clc_s1`: about 47,000 frames, from the title to the ending).

## Structure

| Region | Order | Map | Behind the doors | Doors (beyond the cave, the friendly sort, the shop and the gold door) | Chest (red cave) |
|---|---|---|---|---|---|
| THE CELLARS | always first | roomy and plain: flitters, creepers, insect nests that let out flitters, loose grey stones in the ceilings that drop on whoever passes under (a crush kills outright, ship or Clary); the first area is small (5 × 3 cells) and crowded with notes | stingers (duck their sting), droppers, wisp nests | I: no shop; the friendly sort is the wriggler trial one time in three. II: one time in two a health stall; the first sexton | I Penny Purse; II the yellow key |
| THE ARBORETUM | second (or the Icehouse) | tall trunks and shafts, hungry for fuel: big grubs, snap traps, chasers | lice, swarming | I: a stall that sells only the Homing Charm, for 100. II: one time in two the cursed one (its prize: the Fan Shot) | I Feather Boots; II 200 coins |
| THE ICEHOUSE | second | long low halls: fire drones and wind drones, fire jets | ice underfoot; brutes (hit hard); jets of fire | I: one time in two a health stall. II: a yellow door to the boon (one of three, free) instead of the cave; the trial one time in three | I the Lucky Thimble |
| THE GULLET | third (or Cogtown or the Cloister) | lumpy and winding: leeches, eye turrets, acid pods, blobs (they come back: a farm) | ringworms, slurps in acid pits, pelters | I: a yellow door to free armour (the Tin Plate) instead of the friendly sort | I the Magnet; II Seeker Bells |
| COGTOWN | third | square rooms on a grid: sentries the ship can't hurt (they stop while Clary is out, and she can shoot them), snakes, wall crabs | hoverbots, troopers | I: a yellow door straight to the Spire instead of the cave (skipping Cogtown II and its engine); the trial one time in three. II: the mega store; the friendly sort, the trial or a health stall | II Big Bang |
| THE CLOISTER | third | dark (a circle of light round Clary) and sparse (half the usual enemies); a labyrinth of dead ends; fake notes that wake as ghosts; jets of fire; eyes in the walls that drink the ship's fuel close by | spitters, cocoons and bees, fire wheels | I: six empty red doors, and the friendly sort's is the light switch. II: no Lobber before the engine; the friendly sort, the trial or the cursed one (its prize: the Twin Swipe) | I Spit Gun; II Bounce Beam |
| THE SPIRE | always last | everything at once: boomers, worms, eye turrets, chasers, leeches, fire drones; magnets | the others' things, mixed | I: a yellow door to the magnet switch instead of the friendly sort. II: one time in two a health stall; the last sexton | I Spit Gun; II Feather Boots |
| THE CROWN | the Spire's third | fixed: a long hall climbing in three terraces; after the last sexton, a short hall instead | | the health stall and the shop on the middle terrace, the gold door to Lady Hush at the top. After the last sexton the short hall has only the secret door: straight to Grandsire Tock, with no stall, no shop and no Lady Hush | |

Every region's second area has a sexton behind a red door (met out of
order, he says something vague and gives nothing). On every map a few
enemies are gold champions (one in twelve): half again the hits, a
stronger attack (an eye turret fires three) and double the coins. The big
belly sends out two slow homing missiles now and then. Grey landing pads
lie under every door's landing spot (and a few more). Before the tenth
note the gold door's spot is marked by note-coloured tiles in the rock
above it.

- **Title**: START or CODE over the Carillon, with the records along the
  bottom. B goes back to the library.
- **A run**: the story, then each area's card (AREA n OF 9, the region and
  I/II/III), then the map, with Clary standing beside the Clarion.
- **An area**: fly, land, walk, go through doors; ten notes start the
  dash; the gold door's cave leads to the next area (the first area), or
  past the Lobber and a Hush Engine to the station map (the second).
- **The end**: Lady Hush (gold door) or Grandsire Tock (secret door), then
  the escape shaft, then the ending, the credits and THE END. Losing the
  bar is LOST IN THE BELFRY, then the title.
- **Not kept**: a run is played in one sitting (START pauses it).

## The three goals

| UFO 40 goal | Condition | Campanella 2's goal [W], [GGC] |
|---|---|---|
| Beacon | "GET THE HOMING CHARM" | gift: obtain the Friendship Bracelet |
| Saucer | "FLY OUT OF THE CARILLON. ANSEL TOO?": beat Lady Hush, with or without Ansel (the bad ending counts: Ansel is optional, as the original's question mark says); Grandsire Tock's ending earns it too | gold: "Escape with Pilot?" (a guide calls the lonely way the "alternative" gold ending) [W], [C2-GUIDE] |
| Alien | "STOP GRANDSIRE TOCK" | cherry: save the planetoid (beat Klord) |

The goal lines, the blurb, the story and every line of text are our own
words. A run begun with the FULL-PEAL code earns none of them and leaves
the records alone.

## Mechanics checklist

| Mechanic | How CLARION CALL does it | Source | Test |
|---|---|---|---|
| The ship's flight | CHIME CIRCUIT's model with BELLHOP's numbers: gravity 18/256 px a frame each frame, thrust 44, steering 12 with a 2 a frame drift | [LIZ], [W] ("the same UFO engine"), [MM] | clc_02 |
| Fall faster | hold DOWN: twice the gravity | [C2-GUIDE] | clc_02 |
| The slash | A (the owner's layout; the original's is B), to the side last steered, 2 damage, breaks coin blocks; mashed (11 frames apart) it holds a slow, fuel-free glide | [W], [MM], [C2-GUIDE] | clc_07 |
| Walls | bounce the ship back; a point off the bar when hit at over 0.4 px a frame | [W] ("walls … remove HP"), [ST-FRUST] | clc_03 |
| Landing | slowly (under 0.75 px a frame down) and nearly straight onto level ground; a mark shows on the floor below where a landing would take; any floor contact faster than that is a bad landing (a point); Clary hops out at once | [W], [MM] ("If you fall slowly enough … you will land, and Isabell will hop out"), [C2-GUIDE] | clc_03 |
| Landing pads | grey pads under every door's landing spot and a few more: they take a landing up to 1 px a frame down and 0.8 across | [C2-GUIDE] | clc_03, clc_18 |
| Boarding | UP beside the ship | [MM] ("push up on the d-pad to enter your ship") | clc_02, clc_05 |
| One tank | 800, 5 units every 4 frames of thrust, carried through the whole run; refilled only by pickups; the demo player's tank falls under a quarter in most areas from the third on, and runs dry now and then | [W], [TIPS] ("Fuel is EVERYTHING"), [ST-C2] | clc_04, clc_s1 |
| Dry tank | coins burn instead, one every frame of thrust (they go fast); with none the ship can't thrust, drops and bursts on the ground: the run is over | [W] ("rapidly"), [POPCAR] | clc_04 |
| The bar | 8 points; in the ship, enemies and shots cost two (the big belly three), walls and bad landings one; behind doors Clary takes it (hits there cost two) | [W], [STATIC], [ST-C2] ("3-4 hits") | clc_03, clc_09 |
| Outside on foot | no bar: one touch, or a drop of more than a tile, ends the run; a single tile's drop and her own jumps are fine | [W], [C2-GUIDE] | clc_05 |
| Clary's moves | walk; B jumps (hold for higher, steer in the air); no turning in the air; DOWN crouches; DOWN + B drops through thin floors; ladders | [MM], [W] | clc_05, clc_10 |
| The pistol | A: never more than three shots in the air; held fires steadily, a fresh press as soon as one is free (so mashing up close is faster; enemies have no grace after a hit); UP aims up, DOWN in the air aims down | [W], [MM], [C2-GUIDE] | clc_05, clc_09 |
| Doors | UP in front; behind them the view comes closer | [MM], [W] | clc_05, clc_12 |
| Notes | 14 an area; the first eight +100 fuel, the ninth and tenth +200; HUD: five icons in halves; the remaining notes grow at nine | [W], [MM] | clc_06 |
| The dash | the tenth: the gold door shows, an arrow by the ship points to it, all other doors lock, the spare notes turn to plum coins (25), clock rings appear (+200, the clock stops 2 s), pods hatch and shoot, the clock reads 60 and counts double | [W], [MM] | clc_06 |
| Too late | at 0 the edges of the view darken and the latecomers pour in; a touch ends the run whatever the bar | [W], [MM] | clc_06 |
| Red caves | left to right, a skull drifting in from the left that shoots when close, blocks (a coin each), barrels (a blast with a bonus for every enemy it takes; it never hurts Clary), the chest at the end, a door back out | [W], [MM], [C2-GUIDE], [ST-NECK], [TVT] | clc_10 |
| Chests | three free things to choose one of: the area's item in the middle (the wiki's table, by region; greyed out and untakeable if she has it), and one from each of the other kinds of upgrade beside it (the third kind also offers the stats and toffee if hurt, the sack at a full bar); a region with no item of its own draws the middle one too | [W], [C2-GUIDE], [LIZ] | clc_10, clc_12 |
| Coins | on the map and behind doors they are pickups: they drift in to her from close by (from further with the Magnet), and behind doors they bounce and fade | [TVT], [ST-ITEMS] | clc_08, clc_10, clc_11 |
| Gold caves | always fuel at the end (+500); in a second area the Lobber and a Hush Engine first | [C2-GUIDE], [W] | clc_13 |
| Rooms | back out by the door she came in by; once she is out, that door shuts for good (every door but the gold one) | [MM], [C2-GUIDE] ("The door will still shut behind you") | clc_10, clc_11, clc_12 |
| Shops | four things as icons with prices and no names; touch one to buy; one item per shop; the mega store nine at a tenth more, one of them | [W], [C2-GUIDE], [MM] | clc_11 |
| Health stall (blue door) | toffee (+5, 100) and the heart pin (+2 bar, 150) | [W], [C2-GUIDE] | clc_11 |
| The friendly sort | 500 fuel once, and advice (sixteen lines of our own); now and then a rude one gives nothing | [W], [C2-GUIDE] | clc_11 |
| Wriggler trial | seven wrigglers loop round the room; a shower of 150 coins | [W], [C2-GUIDE] | clc_11 |
| Cursed encounter | splits in two and attacks, no leaving till both are down; a prize (the Arboretum: Fan Shot; the Cloister: Twin Swipe) | [W] | clc_11 |
| Yellow door | the yellow key from the Cellars' second chest; one door a run (the key is used up); like every door, it shuts once she is back out | [W], [ST-ITEMS] | clc_12 |
| Boon | three free upgrades, one from each kind (the ship's, Clary's, the rest), one to keep | [W] | clc_11 |
| Armour, shortcut, magnet switch | as the region table above; the magnet switch holds for the whole region | [W] | clc_11, clc_12, clc_08 |
| Darkness | the Cloister; the light switch lights the whole region | [W] | clc_08 |
| Map enemies | each region's own (table above); large ones drop 50 coins; one in twelve a gold champion (more hits, a stronger attack, double coins); the big belly's slow missiles | [W], [C2-GUIDE], [ST-ITEMS] | clc_07, clc_08, clc_18 |
| Loose stones, nests | the Cellars: a stone over her (ship or on foot) shudders and drops, and a crush kills; nests let out flitters, two at a time | [TVT] | clc_08, clc_18 |
| Wall eyes | the Cloister: an eye in the wall drinks the ship's fuel while it is within about 2 tiles; a slash kills it | [C2-GUIDE] | clc_08, clc_18 |
| The gold door's spot | marked before the tenth note by note-coloured tiles in the rock above it; the Door Dowser points to it too | [W], [C2-GUIDE], [ST-ITEMS] | (drawn: the shots) |
| Leeches | latch on, weigh the ship down; landing shakes them off | [W] | clc_08 |
| Sentries | the ship can't hurt them; they stand down while Clary is out; she can shoot them | [W] | clc_08 |
| Magnets | the Spire's pull and push the ship; the switch stops them | [W] | clc_08 |
| Fake notes | wake as ghosts when close; two slashes | [W] | clc_08 |
| Boomers | their boomerangs fly through rock and go when the boomer does | [W] | clc_08 |
| The Lobber | 12 hits (4 with the Big Bang); bombs when Clary keeps her distance, 3 hits each or a blast on their own; the blast spares other enemies; 20 coins (30 with the purse) | [W] | clc_13 |
| Hush Engines | 20 hits, only once the Lobber is down; the Cloister's stands alone | [W] | clc_13 |
| Lady Hush | 100: her saucer high above, energy shots down, a swarm of slurps, or the whole saucer dropped on its spikes; shoot up | [W] | clc_14 |
| Grandsire Tock | 100: only his head, when the hatch is open, from on top of his machine; it rams, throws rings, rains missiles (shootable) | [W] | clc_15 |
| The escape | after either boss: up the shaft through the opened gates past fuel, arrows and a clock ring by each gate (+200 fuel, the clock stops 2 s), 99 s | [C2-GUIDE], [ST-END] | clc_14 |
| Sextons | one in every region's second area; the Cellars' names the next region; one met out of order gives nothing; the Spire's sends her on: the Crown is then a short hall to the secret door, with no stall, no shop and no Lady Hush | [W], [C2-GUIDE], [ST-SCRIBE], [ST-ZONES] | clc_15 |
| The station map | the next region's choices after each region | [MM] | clc_16 |
| Endings | without Ansel (an engine left), with him, Tock beaten | [W] | clc_14, clc_15, clc_s1-s4 |
| The code | FULL-PEAL: eight of the sixteen (the Tin Plate among them), 16 on the bar, a tank of 1,600 holding 800; the bar (not the tank) full again every area; no goals or records | [CHEATS] (MEGA-BELL), [C2-GUIDE] | clc_17 |
| The secret | shoot the dozing tortoise a hundred times: it mumbles the code; once shot it shoots back now and then | [C2-GUIDE] (the thinking otter: about 100; the UFO Companion says 200, we follow the guide) | clc_17 |
| Saving | none; records only | [ST-SAVES] | clc_17 |

### The sixteen upgrades

| Ours | Theirs (guides' names) | Price | Effect |
|---|---|---|---|
| Twin Swipe | Mirror Slash | 100 | the slash both ways |
| Seeker Bells | Homing Missiles | 250 | a slow homing bell each slash, two at most, slash damage |
| Bounce Beam | Laser Shot | 150 | a ricocheting beam each slash, a little wild, slash damage |
| Fuel Siphon | Fuel Leecher | 250 | 100 fuel a kill made by the ship |
| Spit Gun | Beam Gun | 300 | a short shot each slash, less than the slash |
| Feather Boots | Winged Boots | 100 | B held slows her fall (behind doors) |
| Fan Shot | Diagonal Shot | 200 | two short shots in a spread, each a hit |
| Ghost Lead | Phantom Fire | 150 | shots fly through walls |
| Creeper Caps | Crawler Bombs | 250 | where a shot hits, a cap crawls on |
| Big Bang | Sniper Shot | 300 | a tap: triple damage, a coin; held fire stays plain |
| Lucky Thimble | Talisman | 150 | nothing (a dud by design in the original too) |
| Homing Charm | Friendship Bracelet | 400 (100 at the Arboretum's stall) | a second jump in the air calls the ship |
| Magnet | Star Magnet | 200 | notes and coins come to her (through walls) |
| Door Dowser | Compass | 100 | marks at the screen's edge for the doors that aren't red, and for the gold door even before it shows |
| Penny Purse | Coin Booster | 200 | drops half again (rounded up) |
| Tin Plate | Legendary Armor | 800 | halves every hit |

Also: the heart pin (+2 bar, 150), the spare tank (+200 tank and 200 now,
150), toffee (+5, 100), the fuel flask (+500, 100), the fuel drum
(+1,000, 250), the money sack (200 coins), the yellow key, peal sheets.

### Readings we had to choose

- **Damage**: no source gives numbers [research §8]. In the ship enemies
  and shots cost two points (the big belly three), walls and bad landings
  one; behind doors two (brutes three, Lady Hush's spikes three); after a
  hit the ship has 50 frames of grace, Clary 60. Players: "Getting hit by
  almost anything takes 3+ hit points" [ST-C2].
- **Walls hurt above 0.4 px a frame**: a gentle nudge is free, anything
  firmer costs a point; the wiki says walls "remove HP".
- **Burn rates**: no source gives a number. Ours: 5 units of fuel every 4
  frames of thrust, set so that the demo player's tank falls under a
  quarter in most areas from the third on and runs dry now and then
  ("Fuel is EVERYTHING. You will run out of fuel constantly" [TIPS]); with
  the tank dry, a coin every frame (the original's coins go "rapidly").
- **The fall**: more than one tile kills (the wiki's and TV Tropes'
  wording; floors sit on an 8 px grid, so a one-tile drop is fine and a
  two-tile drop is not); a guide says "more than 2".
- **Landing speeds**: under 0.75 px a frame down and 0.63 across sets down
  (on a pad 1 and 0.8); any floor contact faster than that is a bad
  landing and costs a point.
- **Notes**: fourteen an area (the original has "more than 10"); two near
  the gold door and the rest spread; only in the Cellars does one wait
  near the start ("the first act is small and has plenty of Stars").
- **The gold door** is in the map from the start, hidden until the tenth
  note (the manual: it "will spawn somewhere in the level").
- **The generator**: 8 × 5 cells (the Cellars' first area 5 × 3), a tree of passages plus region-sized
  loops, chambers shaped by region, passages three or four wide (two-tile
  chutes in some regions), ladders in some shafts, coin blocks in walls.
  A map is kept only if every note, landing spot and door is reachable.
- **Sexton doors look like any red door**, and every second area has one;
  the sexton's region is said once and never shown again (players: "Nope,
  there isn't [an indicator]").
- **The station map** lets Clary pick the next region freely; the sextons'
  chain only holds if she goes where she was sent.
- **The wriggler trial** doesn't lock the door; the cursed encounter does.
- **The Lucky Thimble** does nothing: settled, a dud by design (TV Tropes
  quotes the developer: it was meant for cut ghost mechanics, and the UFO
  Companion says it "doesn't seem to work").
- **Coins**: pickups everywhere, drifting in from close by.
- **Pistol**: three shots in the air at most; held, a shot every 14
  frames; a fresh press whenever a slot is free.
- **The escape**: settled, 99 seconds after either boss [C2-GUIDE]; a
  clock ring stops the clock 2 s (the guide's "delay the timer for 2
  seconds").
- **The yellow key**: settled, one door a run (a thread: an unused key can
  open the Vaalpolis shortcut instead).
- **Engines**: 20 hits; Rotondo-style bombs: 3 hits, a 3 s fuse.
- **Endings and goals**: any win over Lady Hush earns the Saucer (the
  guide's "alternative" gold ending); Tock's ending earns it too.
- **The dozing tortoise** stands in for the thinking otter: a hundred
  shots, the guide's count (the UFO Companion says 200).

## What is ours

- **Name:** CLARION CALL (1987, Beamdown Softworks).
- **People:** Clary in the Clarion (CHIME CIRCUIT's), Ansel (BELLHOP's),
  Lady Hush (BELLHOP's), Grandsire Tock, the sextons, the station hands,
  the shopkeeper, the dozing tortoise.
- **Places:** the Carillon, a bell-tower station, its seven regions and
  the Crown; every map generator, all sixteen cave pieces, every room, the
  Crown and the escape shaft.
- **Things:** notes, plum coins, clock rings, pods, latecomers, the Hush
  Engines, the Lobber, peal sheets, the sixteen upgrades and the rest.
- **Enemies:** flitters, creepers, grubs, snap traps, fire and wind drones,
  leeches, eye turrets, acid pods, blobs, sentries, snakes, wall crabs,
  ghosts, fire jets, boomers, worms, chasers, the big belly; stingers,
  droppers, lice, jets, brutes, ringworms, slurps, pelters, hoverbots,
  troopers, cocoons and bees, spitters, fire wheels, wisps and their nests,
  the skull, the wrigglers, the cursed faces.
- **Music:** "Clarion Call", "Down in the Cellars", "Under the Boughs",
  "Thin Ice", "Something Digesting", "Cogtown Shift", "Candles Out", "Top
  of the Spire", "Underfoot", "The Gold Door", "The Lobber", "Lady Hush
  Returns", "Grandsire Tock", "Ninety-Nine Seconds", "One per Customer",
  "The Station Map", "Home with Ansel", "The Bells Ring Again", "Home
  Alone" and two jingles.
- **Words:** the story, the advice, the sextons' lines, the endings, the
  goal lines, the code (FULL-PEAL) and every label.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
UFO 40 goals, which are Campanella 2's own (gift, gold, cherry). The
Terminal riddle (hold six stars) belongs to the collection's terminal,
which UFO 40 doesn't have.

## Controls

| Input | In the ship | On foot |
|---|---|---|
| ◀ ▶ | steer | walk (she turns only on the ground) |
| A | slash | pistol (hold, or mash for faster) |
| B | thrust (hold) | jump (hold for higher); with the Homing Charm, again in the air to call the ship |
| ▼ | fall faster | crouch; with B, drop through a thin floor; climb down |
| ▲ | | aim up; climb; go through a door; board the ship beside it |
| START | pause | pause |

**The owner's layout.** The original puts thrust and the jump on A and
the slash and the pistol on B [MM]. UFO 40 keeps the main action on A
(keyboard Z / Space, the Vita's Cross), so here the two are swapped: A
slashes and shoots, B thrusts and jumps. The rules are otherwise the
same.

### Not confirmed

| What | Our reading | Why it is uncertain |
|---|---|---|
| Thrust and slash buttons | B thrusts, A slashes (the owner's layout) | the manual has them the other way round for Campanella 2 [MM] |
| Getting out | automatic on landing | [MM]: "Isabell will hop out" |
| Boarding, doors | UP | [MM] |
| Fall faster | DOWN | one guide [C2-GUIDE] |
| Aiming down | DOWN in the air | [W] says "all four directions"; [MM] mentions only up |
| Mashing | a fresh press fires sooner | [W] ("mashing fires faster") |
| Damage numbers, grace frames | see Readings | no source |
| Burn rates | see Readings (fuel ours; a coin a frame) | no number in any source |
| Yellow key: one door | used up on the first yellow door | [W] calls it a one-time item |
| The fall height | more than one tile | the wiki and TV Tropes agree; one guide says two |
| How the next region is chosen | the station map | [MM] (one source) |

## Tests

`tests/clc_01` … `clc_20` drive the rules with button presses, or set up a
moment with cheats and then play it with presses. `clc_18` generates 2,800
maps (every generated area of every region over 200 seeds) and checks that
each can be finished: every note, door and landing spot reachable on the
real collision rules. The demo player (`clc_bot.c`) plays whole runs with
real presses: on a map it plans the cheapest round of the notes it still
needs (climbing dear, falling cheap) that ends at the gold door, over a
distance field of the cells the Clarion fits through, kept away from
walls, from turrets' sight and from under loose stones, and picks among
nine headings by flying each 20 frames ahead on a copy of the map (40 by
the Cellars' stones, with two "bait" plans that bring a stone down and
back off); it slows by the walls, turns to slash what comes near, clears
the landing spot, lands, walks in, and only takes a detour it has the
fuel for; behind doors it looks 30 frames ahead over a dozen moves,
shooting all the while, and picks the best of a chest's three. `clc_s1`
and `clc_s4` reach the gold ending on two generated runs (`clc_s1` also
checks that the tank ran low), `clc_s2` the true ending (following the
sextons), `clc_s3` the bad one (Cogtown's shortcut).

## Sources

- [W] UFO 50 Wiki (Miraheze), "Campanella 2" (rendered and raw): the bar,
  fuel and landing, the gold door and the dash, the sub-areas, shops, the
  items table and prices, the guaranteed chest items, door odds by region,
  Rotondo, Queen Zu, Klord, the sages and scrolls, the endings.
  https://ufo50.miraheze.org/wiki/Campanella_2
- [CHEATS] Miraheze, "Cheats": MEGA-BELL. https://ufo50.miraheze.org/wiki/Cheats
- [MM] Steam guide "The missing manuals - How to play UFO 50 games", its
  Campanella 2 section: the HUD, landing and hopping out, UP for the ship
  and doors, the pistol and aiming up, thin floors, the skull, items with
  prices, the station map. https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [C2-GUIDE] Steam guide "Campanella 2 Guide": hold DOWN to fall faster,
  the bracelet's double jump, the fall height, door types and counts, the
  doors that shut behind her, NPC hints (and the rude ones), the 3-bullet
  cap, the slash glide, landing pads, the gold door's star tiles, the
  99-second escape after either boss, the "alternative" gold ending, the
  otter. https://steamcommunity.com/sharedfiles/filedetails/?id=3355836169
- [TVT] TV Tropes, recap of UFO 50 game 35: the falling blocks and nests,
  barrels that don't hurt, coins drawn to the player, the Thimble.
  https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game35Campanella2
- [ST-ITEMS], [ST-GOLD], [ST-FRUST] Steam threads: items and champions,
  the key's two uses, the dowser and the exit; the gold route; damage.
  https://steamcommunity.com/app/1147860/discussions/0/4699034922680633601/ ,
  https://steamcommunity.com/app/1147860/discussions/0/4700161008285364090/ ,
  https://steamcommunity.com/app/1147860/discussions/0/4700161643034717662/
- [TIPS] Steam guide "Tips & Tricks": "Fuel is EVERYTHING".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3340576461
- [GGC] Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [ST-SAVES] Steam thread on which games save.
  https://steamcommunity.com/app/1147860/discussions/0/4849904176633630339/
- [ST-C2], [ST-NECK], [ST-END], [ST-SCRIBE], [ST-ZONES] Steam threads on
  Campanella 2: hits in caves, the necklace that does nothing, the escape,
  no reminder of the next sage's region, the chain by region.
  https://steamcommunity.com/app/1147860/discussions/0/4849904828215574814/ ,
  https://steamcommunity.com/app/1147860/discussions/0/4626978969929922913/ ,
  https://steamcommunity.com/app/1147860/discussions/0/6757179594728836047/ ,
  https://steamcommunity.com/app/1147860/discussions/0/4638238788728914773/ ,
  https://steamcommunity.com/app/1147860/discussions/0/4638238788728915182/
- [LIZ] Lizstar's Trashcan, UFO 50 #35: the same UFO engine, landing and
  stepping out. https://lizstar64.github.io/reviews/2024/10/18/UFO50-35.html
- [STATIC] Static Canvas, "The UFO 50 Diaries: Campanella 2": one lifeline.
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-campanella-2
- [POPCAR] Popcar's Blog, "Reviewing Every UFO 50 Game": coins as fuel.
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [MP] Miraheze, "Multiplayer": not listed. https://ufo50.miraheze.org/wiki/Multiplayer
