# 22 · KIPPER

*Internal design document. Not shown in the product.*

## Tribute to

**Porgy** (UFO 50 game #22, Mossmouth, "March 1986", the first game under
the UFO Soft name). The rules were researched from text only: the
community wiki's raw page, the Steam guides "The missing manuals" (section
22) and "Gift, Gold & Cherry", the text and comments of YellowAfterlife's
Porgy map guide (its images were not opened), Steam threads, written
reviews, TV Tropes and speedrun.com's leaderboard (listed under Sources;
the full notes are in the research folder, `22-porgy.md`, and the
independent review is `ufo40-review/22-review.md`). No UFO 50
images, video, sprites, music, maps or text were used as references, and
nothing was taken from a UFO 50 install.

**Map type: fixed.** Porgy's sea is one hand-made map, the same every
game: "map, item positions, boss roster and areas are fixed" [MH]. Ours is
fixed too and all our own: Gullwater Sound, laid out cavern by cavern,
tunnel by tunnel, with every gate, block, crack and pick-up placed by hand
in `tools/kipper/make_sea.py`, which writes `src/games/kipper/kpr_sea.c`
and `tools/kipper/sea.txt`. The script also plays the sea greedily (take
the nearest thing the fuel can reach and bring home, install it, repeat)
to check that all 70 pick-ups come in, in a sensible order. Only the
structure follows the original:

| Full scale | Porgy | KIPPER |
|---|---|---|
| Players | 1, no 2-player mode [MH], [MH-MP] | 1 |
| The sea | one connected map in 3 areas stacked by depth: lit shallows, pillared ruins, a pitch-dark deep [TVT], [MH], [G-MAP] | Gullwater Sound, 160 x 150 tiles (about 130 screens): the Kelpwood (rows 3-49), the Drowned Arcade (rows 50-99, ruins and columns), the Blackwater (rows 100-149, dark, eyes, steam vents) |
| Docks | at least two, both on the surface in Area 1, one top-middle; none in Areas 2 and 3 [STATIC], [STEAM-SUGG], [TVT-YMMV] | the Pier (top middle, where the game starts) and the Lighthouse (top right) |
| Fuel tanks | 20, +1 bar each, 4 bars to start [MH], [TVT] | 20: 9 in the Kelpwood, 6 in the Arcade, 5 in the Blackwater; 4 to 24 bars |
| Torpedo boosters | 20, more damage and range [MH] | 20 torpedo kits: 8, 7, 5 |
| Fish eggs | 20 for Fish Mom, a fish companion every 5 [MH], [TVT] | 20 eggs for Auntie Roe: 9, 6, 5; four companions |
| Equipment | 10, fitted 2 at once, 3 and 4 with two and four strange lights [MH] | the same 10 (our names): 4 in the Kelpwood (2 of them reached from the Arcade below), 3 in the Arcade, 3 in the Blackwater |
| Bosses | 6: a shark and an octopus (Area 1), a moray eel and a nautilus (Area 2), a vampire squid and the alien (Area 3), 500 / 500 / 600 / 800 / 1000 / 1400 [MH] | Gnasher, Inkwell, Longjaw, the Whorl, Nightcape, Xolloth, with the same hit points |
| Creatures | 18 kinds [MH] | 18 kinds, 85 placed (33, 30, 22) and the filchers the polyps let out |
| Hidden things | many items in walls or behind suspicious tiles; some need a depth charge [G-MAP], [STEAM-TIPS] | 5 pick-ups behind cracked walls (shoot or rub) or sealed walls (a blast only), and the Seeker Fins down a hidden way |
| Length | Gold record 8:33, Cherry record 33:58 (speedrun.com) [SR]; about 5 hours to 100 % for a first player [LIZ] | the demo player takes everything and beats Xolloth in 59 minutes of play (212,447 frames, 71 dives) |
| Goals | 3 [MH], [GGC] | the same 3 |
| Stats | Fuel Tanks, Torpedo Upgrades, Eggs Returned [MH] | the same three, on the title |

## The structure

Every dive starts under a dock with a full tank. The start is a very small
space: the Pier's shaft, a pocket either side of it (Auntie Roe's grotto
is up from the west one) and the Kelp Hall below. Gnasher the shark roams
the hall and the Western Reef; Inkwell the octopus roams the Coral Garden
to the east, under the Lighthouse, and gets stuck in the narrow coral pit
below it. With a few tanks the Lighthouse is in reach and becomes a second
starting point for the east. The **Ram Torpedoes** wait at the Breakwater
in the far east, the **Long Thruster** at Shelf End in the far west.

The Kelpwood's crooked descent leads into the **Drowned Arcade**: the
Colonnade, a long hall of columns where Longjaw the eel roams. The Ram
Torpedoes light the switch over its west gate: beyond, the West Wing winds
down to the **Upshot Rockets**, the first thing that breaks blue blocks.
Rockets open the two sealed pockets at the bottom of the Kelpwood from
below (the **Auger** and the **Sinker Bombs**) and the Reliquary over the
Undercroft (the **Lamp**). The **Seeker Fins** are down a hidden way under
the Colonnade's floor. The Whorl, a nautilus, keeps to its chamber in the
east and never roams.

Two ways lead down into the **Blackwater**, which is pitch dark without
the Lamp: a plug of dirt in the Undercroft's floor (the Auger) and a floor
of blue blocks near the Whorl (a sinker bomb from above). Nightcape roams
the Trench. The **Lean Mix**, the **Hull Plates** and the **Sonar** wait
in its side caves. Xolloth's lair is under the Trench: a floor of blue
blocks (a bomb), then the antechamber, then a switch gate (the Ram
Torpedoes): with the Lamp, the last dive needs three slots, as the
original's does ("Light and Busters are necessary") [STEAM-FINAL].

## Mechanics checklist

| Mechanic | How KIPPER does it | Source | Test |
|---|---|---|---|
| Swimming | eight ways, slow (a pixel a frame), a little momentum; knocked back when hit | [MANUALS], [TVT], [POPCAR] | kpr_02 |
| Fuel is health and time | a bar is 360 frames of swimming; swimming burns it, sitting still or pushing on rock doesn't | [MH], [MANUALS], [STEAM-REVIEW] | kpr_02 |
| The dash | a tap of the dash button: a quick burst the way the pad points (or the nose, sitting still) for half a bar, as much as a knock ("when you dash 2 times, that's one bar of fuel gone") | [MANUALS], [MH], [STEAM-TIPS] | kpr_02 |
| A knock | a creature, a shot, a steam vent or a big one: half a bar and a push away, then a second of mercy | [MH], [MANUALS] | kpr_06 |
| Torpedoes | held fire: one every 14 frames, the facing locked (back away firing forward); tapped: as fast as every 7, the facing free; firing costs no fuel | [MANUALS] | kpr_03 |
| Torpedo kits | damage 3 + 1 a kit (3 to 23), reach 40 + 6 px a kit (40 to 160 px) | [MH], [STEAM-REVIEW] | kpr_03 |
| Stun, not kill | enough hits pacify a creature: small and blue and harmless until the sub docks or changes area; now and then one leaves a drop of fuel | [MH], [TVT], [STATIC] | kpr_07 |
| The trail | anything picked up trails behind on the sub's wake, three at most; nothing counts until it's docked | [MANUALS] | kpr_04 |
| Docking | nose up under a dock: what's towed is installed, the tank filled, the dive banked and saved, every creature wakes | [MANUALS], [MH] | kpr_04 |
| What a dock keeps | pick-ups, damage to the big ones, blocks blasted on that dive; dirt grows back and gates shut | [MANUALS], [TVT-YMMV], [MH] | kpr_05, kpr_12, kpr_13 |
| Out of fuel | towed back to the last dock; the dive's finds go back where they were, the big ones' damage and the blasted blocks too | [MANUALS], [MH], [STEAM-CHANGES] | kpr_05 |
| Respawning | creatures wake and go home at a dock and on crossing into another area | [MH], [LIZ] | kpr_07 |
| Polyps and filchers | a polyp shoots, and while the sub tows anything (sinker bombs too) it lets out a filcher that snatches the last thing in the trail and takes it home; pacify the filcher and it drops it | [MH], [TVT], [STEAM-CHANGES] | kpr_08 |
| Bloaters' clouds | a bloater puffs shots and clouds every way when the sub comes close or hits it; a cloud knocks the last thing in the trail loose (a sinker bomb knocked loose or snatched goes off) | [MH], [TVT] | kpr_08 |
| The equipment screen | at a dock: the ten pieces, fitted or not, as many as there are slots | [MANUALS], [MH] | kpr_10 |
| Slots | 2; 3 with two deep lights; 4 with four | [MH], [TVT] | kpr_10, kpr_19 |
| Ram Torpedoes | strong and short (damage x2 + 2, reach 32-72 px); they light switches and hurt a shellback in its shell | [MH] | kpr_03, kpr_09, kpr_11 |
| Auger | a dash (or the thruster) digs through dirt, and runs through the creatures it meets (a hit of 40, no knock to the sub; "with Long Boost + Digger, you instant kill most enemies"); dirt grows back whenever the sub docks | [MH], [STEAM-TIPS] | kpr_12 |
| Long Thruster | holding the dash button keeps it going (2.6 px a frame, 2 fuel a frame): a dear start, then a cruise cheaper than swimming ("more efficient than swimming slowly, as long as you can get long boosts") | [MH], [STEAM-TIPS] | kpr_15 |
| Sinker Bombs | two ride in the trail from a dock; sitting still, hold the dash button to ready one and let go to drop it (touching the pad first calls it off; letting go too soon is a dash); it sinks and goes off, breaking blue blocks and sealed walls, hitting the big ones for 350 (two halve Xolloth) and the sub for half a bar | [MH], [TVT], [STEAM-CHANGES], [STEAM-FINAL] | kpr_14 |
| Upshot Rockets | hold fire to ready one, let go: it goes straight up; its blast breaks blue blocks and sealed walls | [MH] | kpr_13 |
| Seeker Fins | torpedoes steer at the nearest creature ahead | [MH] | kpr_15 |
| Lamp | the Blackwater is black past a few pixels without it; with it, a circle of light round the sub | [TVT], [MH], [G-MAP] | kpr_16 |
| Sonar | big ones shown on the radar's rim from anywhere in their area; pick-ups as yellow dots, hidden ones too | [MH], [MANUALS] | kpr_16 |
| Lean Mix | swimming, dashing and the thruster burn three quarters | [MH] | kpr_15 |
| Hull Plates | three plates from a dock; a knock takes a plate instead of fuel; a plate knocked off floats and can be picked up again | [MH], [STEAM-FINAL] | kpr_06 |
| The radar | top left; big ones nearby as red dots | [MANUALS] | kpr_16 |
| Blocks and walls | blue blocks (a rocket or a bomb; a block breaks as one piece), dirt (the Auger), switches (Ram Torpedoes), cracked walls (shoot them or rub on them), sealed walls (a blast only) | [MH], [STEAM-TIPS], [G-MAP] | kpr_11-14, kpr_17 |
| Auntie Roe | swim to her with eggs in tow: they're home for good; every fifth brings a hatchling along: blue ones circle the sub and stop shots, red ones blow bubbles at creatures whenever a torpedo goes | [MH], [TVT] | kpr_18 |
| The big ones | they roam their area along loose loops and come after the sub when they see it (a rumble warns first); out of sight a second, or far off, and they give up; the nautilus keeps to its chamber; damage stays only once docked; beaten, one flees in a chain of blasts and leaves a deep light | [MH], [TVT], [HANS], [KAYIN], [TVT-YMMV] | kpr_19, kpr_20 |
| Gnasher | the shark: lunges when level with the sub | [MH] | kpr_20 |
| Inkwell | the octopus: arms out of the walls near the sub, ink in threes | [TVT] | kpr_20 |
| Longjaw | the moray eel: a long body that hurts too, lunges close in | [MH] | kpr_20 |
| The Whorl | the nautilus: bounds about its chamber spraying spirals of shots; can't be hurt from outside it | [TVT-YMMV] | kpr_20 |
| Nightcape | the vampire squid: turns its cape inside out (no harm gets in) and sheds rings of spines | (ours) | kpr_20 |
| Xolloth | the alien in its lair: a laser that trails the sub (cross it diagonally), a whirling spin round the lair (keep away), spores | [MH], [STEAM-FINAL] | kpr_20 |
| Steam vents | the Blackwater's vents blow every three seconds | [TVT] | kpr_21 |
| The end | Xolloth beaten: the ending (two of them) and the credits; CONTINUE goes back to the last save with Xolloth there again | [MH] | kpr_22 |
| Goals | Beacon (gift): drive off Gnasher; Saucer (gold): beat Xolloth; Alien (cherry): beat Xolloth with everything done | [MH], [GGC] | kpr_22 |
| Saving | at every dock (and when Auntie Roe takes eggs, and when a big one flees); NEW GAME / CONTINUE | [MH], [MANUALS] | kpr_23 |
| Stats | Fuel Tanks, Torpedo Upgrades, Eggs Returned, best ever, on the title | [MH] | kpr_23 |
| The demo | left alone on the title, the sub swims home towing three finds with a full tank; any button ends it | [STEAM-TIPS] | kpr_24 |
| Playable start to end | the demo player, with real presses from the title, takes all 70 pick-ups and five big ones, beats Xolloth and earns all three goals | [SR] | kpr_26 |

### Readings we had to choose

- **Fuel numbers** (no source gives rates): a bar is 360 frames of
  swimming at a pixel a frame (22.5 tiles), a dash 180 (half a bar, as
  players measure it), the thruster 2 a frame, a knock 180. The Lean Mix
  takes a quarter off
  swimming, dashes and the thruster, not knocks.
- **Idle drain**: none while sitting still; pushing on rock counts as
  sitting still. (Players disagree; the wiki and manual list only moving,
  dashing and knocks.)
- **Eight ways**: the manual's "any direction" over the wiki's "four".
- **Torpedoes**: damage 3 + kits, reach 40 + 6 px a kit; held fire every
  14 frames, taps every 7; Ram Torpedoes double the damage and add 2,
  reach 32 + 2 px a kit. Rockets: hold fire 40 frames; held fire still
  fires torpedoes while a rocket readies.
- **Sinker Bombs** ride in the trail and take its slots (so a dive with
  both carries one find); dropped from the end of the trail, they sink
  0.45 px a frame and go off after 2.5 s (blast 44 px, 350 to a big
  one). A dropped bomb can't be picked up again: the players' "it picks
  it right back up" turned out to be the hold being called off
  [STEAM-CHANGES].
- **Blue blocks** break as whole blocks (the original's are "big
  blocks"): a blast touching any tile of a block breaks all of it.
- **Switch gates** stay open for the rest of the dive and shut at the
  next dock, like dirt.
- **The deep lights**: a big one beaten flees for good at once ("removes
  it permanently") and its light counts at once; damage short of that
  counts only once docked.
- **Eggs**: towed home to a dock they stay in the trail (they belong to
  Auntie Roe); handed to her they are home for good at once. Her grotto is
  near the Pier. Companions alternate blue, red, blue, red.
- **100 %** counts the 20 tanks, 20 kits, 20 eggs handed over, the 10
  pieces of equipment and the five big ones before Xolloth (75 parts).
- **The shellback** walks back and forth along its floor (up to 64 px
  either way) and aims its shots at the sub, as the wiki has it.
- **Chases**: a big one sees the sub within 150 px with a clear line,
  chases for up to 4 s, gives up out of sight for a second or beyond
  260 px, and swims back along its own trail to its loop. Chase speeds
  (Gnasher 0.92, Inkwell 0.6, Longjaw 0.85, Nightcape 0.8 px a frame) are
  just under the sub's, so a dash gets away.
- **Fuel drops**: 12 % of pacified creatures leave half a bar.
- **The camera** scrolls with the sub (a fan map stitched from screenshots
  fits either reading).
- **Corners**: the sub slides round a corner it clips by up to 6 px.
- **The big ones' moves** beyond the documented ones (Nightcape's cape,
  the Whorl's spirals, Inkwell's ink, Xolloth's spores) are ours.
- **The demo** starts after 15 s on the title and runs up to 25 s.

## What is ours

- **Name:** KIPPER (1986, Beamdown Softworks).
- **Characters:** Dr. Juno Marlow, who counts the fish of Gullwater Sound,
  and her little yellow sub Kipper; Auntie Roe, an old grouper lady in a
  shawl; Gnasher, Inkwell, Longjaw, the Whorl, Nightcape and Xolloth; the
  Sludgeworks on the cliff, pouring into the bay.
- **The creatures:** polyps, filchers, glimmers, greenlings, shellbacks,
  kiterays, bluebells, redcaps, lanternmaws, risers, pipehorses, bloaters,
  stiltcrabs, sunstars, billfish, burrs, snapworms and fizzworms.
- **The equipment's names:** Ram Torpedoes, Auger, Long Thruster, Sinker
  Bombs, Upshot Rockets, Seeker Fins, Lamp, Sonar, Lean Mix, Hull Plates;
  torpedo kits, deep lights.
- **The sea:** Gullwater Sound, every cavern of it: the Pier and the
  Lighthouse, the Kelp Hall, the Western Reef, the Coral Garden, Shelf
  End, the Breakwater, the Colonnade, the West Wing, the Reliquary, the
  Whorl's chamber, the Undercroft, the Trench, the Gullet, the Undertow,
  the Eastern Deep, the lair; where every pick-up lies and every hidden
  spot.
- **Music:** "Kipper" (title), "Casting Off" (story), "The Kelpwood",
  "Drowned Arcade", "Blackwater", "Dry Dock", "Big Fish" (the big ones),
  "Xolloth", "Slack Tide" (ending), and the jingles "Installed" and
  "Towed Home".
- **Words:** the story, Auntie Roe's lines, Juno's lines, both endings,
  the credits and every label.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
UFO 40 goals, which are Porgy's own. The owner's change: the main action
(torpedoes) is on **A** and the dash on **B**; Porgy fires on Button 1 (B)
and dashes on Button 2 (A) [MANUALS]. The cartridge card's CONTROLS page
can swap them back. The UFO 50 Terminal's codes and debug keys are
collection features; UFO 40 has no terminal, so they are left out.

## Controls

| Input | Action |
|---|---|
| D-pad | swim, eight ways |
| A (hold) | torpedoes, the facing locked |
| A (tap) | torpedoes, quicker; the facing follows the pad |
| A held, let go (Upshot Rockets) | a rocket straight up |
| B | dash (half a bar) |
| B held (Long Thruster) | keep going |
| B held sitting still, then let go (Sinker Bombs) | drop a bomb (the pad first calls it off) |
| UP under a dock | dock |
| At a dock | D-pad picks, A fits or takes off, B launches |
| START | pause |

## Not confirmed

| What | Reading here |
|---|---|
| Which button fires | A here (the owner's layout, swappable); B in the original [MANUALS] |
| Tapped fire rate | every 7 frames at best, against 14 held |
| Rockets and torpedoes together | held fire keeps firing torpedoes while a rocket readies |
| The rocket's and the bomb's hold times | 40 and 20 frames (the bomb drops on release [STEAM-CHANGES]) |
| The dash sitting still | goes the way the nose points |
| Docking | nose up (UP) under the dock |
| Switch gates | open until the next dock |
| Bomb slots | bombs take trail slots |
| Eggs at a dock | stay in the trail |
| What 100 % counts | pick-ups, eggs handed over, equipment and the five big ones |
| Companion colours | alternate blue, red |
| Boss moves | everything past the documented laser, spin, tentacles and roaming |
| Every number in "Readings we had to choose" | |

## Tests

`tests/kpr_01` … `kpr_25` drive each rule with button presses, or set up
a moment with cheats and then play it. `kpr_25` checks the sea: the
counts, every pick-up and both docks reachable. The demo player
(`kpr_bot.c`) plans like a player who knows the map: at a dock it picks
the nearest things it can reach and bring home with the fuel it has (a
quarter spare and room for knocks), up to three, or a blast to open a
sealed way, or Auntie Roe when it tows eggs, or a fight with a big one
once nothing else is in reach; it fits what the dive needs and fills the
other slots (it leaves the thruster be: with a dash at half a bar, short
boosts don't pay). Out in the sea it follows a distance map, lights switches,
dashes through dirt, rubs cracked walls, shoots at creatures in its way and
at thieves after its trail, goes back for anything knocked loose, and
turns for home as soon as the fuel would only just get it there. Before
each move it runs the real game ahead on a copy to see whether that move
gets knocked or loses something from the trail; if so, it tries the other
ways and keeps the safest that still gets on. `kpr_26` plays from the
title with real presses: all 70 pick-ups, the five big ones and Xolloth,
all three goals, in 59 minutes of play.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Porgy" (raw page): the premise, fuel,
  the collectibles, every item with its notes and area, the creatures, the
  bosses and their hit points, the endings and the goals.
  https://ufo50.miraheze.org/wiki/Porgy
- [MH-MP] UFO 50 Wiki, "Multiplayer" (Porgy not listed).
  https://ufo50.miraheze.org/wiki/Multiplayer
- [MANUALS] Steam guide "The missing manuals - How to play UFO 50 games",
  section 22: controls, the radar and fuel bars, docking, the trail of
  three, the equipment screen. https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [GGC] Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [G-MAP] Steam guide "Porgy map" by YellowAfterlife (text and comments
  only, no images opened): the dark third area, secrets behind walls, some
  needing a depth charge. https://steamcommunity.com/sharedfiles/filedetails/?id=3335347833
- [STEAM-TIPS] Steam thread "beginner tips for porgy?": two dashes are a
  bar, as dear as a knock; a drilling long boost runs creatures through;
  a long boost beats swimming for fuel.
  https://steamcommunity.com/app/1147860/discussions/0/4849903998517165833/
- [STEAM-REVIEW] Steam thread "Porgy Review - a Sub with an Identity Crisis".
  https://steamcommunity.com/app/1147860/discussions/0/4849904176633778261/
- [STEAM-CHANGES] Steam thread "Changes I would bring to Porgy": hold,
  then let go to drop a depth charge (moving calls it off); charges in the
  trail can be stolen.
  https://steamcommunity.com/app/1147860/discussions/0/4849904631720486806/
- [STEAM-SUGG] Steam thread "Suggested changes to Porgy".
  https://steamcommunity.com/app/1147860/discussions/0/4700161359561460813/
- [STEAM-FINAL] Steam thread "What did you bring to the final boss?":
  light and busters needed to reach it; two charges halve its health.
  https://steamcommunity.com/app/1147860/discussions/0/4700161192395966591/
- [TVT] TV Tropes recap. https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game22Porgy
- [TVT-YMMV] TV Tropes YMMV page (boss cheeses, slots, the 1.1.4 block
  change, Neptune). https://tvtropes.org/pmwiki/pmwiki.php/YMMV/UFO50Game22Porgy
- [STATIC] Static Canvas, "The UFO 50 Diaries: Porgy".
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-porgy
- [LIZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 22".
  https://lizstar64.github.io/reviews/2024/10/14/UFO50-22.html
- [POPCAR] Popcar's Blog, "Reviewing Every Single UFO 50 Game".
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [KAYIN] Kayin, "UFO 50". https://kayin.moe/ufo-50
- [HANS] thehans255, "One unique thing about every UFO 50 game".
  https://www.thehans255.com/blog/2024/10/one-unique-thing-about-every-ufo-50-game/
- [SR] speedrun.com, UFO 50, category "22 - Porgy" (read through its API):
  Gold 8:33, 8:48, 9:11, 9:41, 9:58; Cherry 33:58, 34:08, 39:27, 40:04,
  61:32. https://www.speedrun.com/UFO_50
