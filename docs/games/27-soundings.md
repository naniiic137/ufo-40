# 27 · SOUNDINGS

*Internal design document. Not shown in the product.*

## Tribute to

**Divers** (UFO 50 game #27, Mossmouth, "December 1986"). The rules were
researched from text only: the community wiki's raw page, the Steam guides
"The missing manuals" (section 27), "Divers 100% Guide", "Divers Guide
[WIP]" and "What the shop / loot items in Divers do" with their comments,
the community data sheet those guides' numbers come from [SHEET] (added
after the fidelity review), Steam threads, written reviews and TV Tropes
(listed under Sources; the full
notes are in the research folder, `27-divers.md`). No UFO 50 images, video,
sprites, music, map or text were used as references, and nothing was taken
from a UFO 50 install.

**Map type: fixed.** Divers has one hand-made map, so SOUNDINGS has one too
(`sdg_map.c`, 96 × 80 cells of 16 px). It was laid out for this cartridge
from hand-placed rooms and tunnels; only the structure follows the
original's, as the text sources describe it:

| Full scale | Divers | SOUNDINGS |
|---|---|---|
| Base | one surface base, menus only: shop, equip, dive, behind three unlabelled pictures, with an info box [MM], [W] | the raft: a shed, an anchor and the ladder into the water, with the log beside them (level and XP notches, gold, deepest depth, the eight relics) |
| Regions | 8: the Shallows, the Slime Cave, the Anemone Cave, the Temple/Ruins, the Jelly Cave, the Spider Cave, the Trench and the empty green room [G100], [TVT] | 8: the Sunlit Shelf, Gumwell, Frond Hollow, the Sunken Shrine, Lantern Cave, the Stilt Pit, the Red Deep and the Still Room |
| Party | Dylan, Orlok and Thyme, three lizard brothers, alike, two hands each [W], [MM] | Moss, Reed and Skip, three axolotl siblings, alike, two hands each |
| Weapons | spears, hammers, shields, each in three elements; spears and shields in three tiers, hammers in two; three unique finds [WIP], [W] | pokers/gaffs/harpoons, clubs/mauls, plates/targes/bulwarks in reef, zap and ooze (24 in the shop); the leech club, the spine targe, the lamp rod |
| Other items | small, medium, large, holy and evil potions, egg, bomb, mist orb, godblood, flippers [W], [ITEMS] | sip, draught, flagon, hallow, bitters, egg, charge, ink sac, old blood, fins |
| Relics | 8 kinds, 9 of each at most; two unique (one from the mid-boss, one from a late chest) [W], [G100] | foam, gum, pebble, cog, spiral, bone, the sun disc (the Abbot) and the moon disc (the Stilt Pit's floor) |
| Creatures | 18 [W], [G100] | 18: fizzle, needler, frond, nipper, glob, smog eel, clamper, tinfin, whorl, glimmer, groper, burrnut, stilter, slater, haunt, dusksquid, grinfish, mawworm |
| Bosses | Guelap (with two anemones) and Skeor (two eyes, two arms) [W], [G100] | the Abbot (with two fronds) and the Gloamheart (two eyes, two arms) |
| Heads | 3, each behind a fake wall in a different region [W], [WIP] | 3: a duskling figure (Gumwell), a miner's helmet (Frond Hollow), a stone axolotl head (the shrine, past the Abbot); two are UFO 40's own Mooncat and Barbuta tributes, as the original's are its sister games |
| Chests | 17: gold 100, 500, 500, 500 (unseen), 1,500 and 2,000; eight items as pearls; the candy, cross and B relics [SHEET] Misc, [G100] | 17 the same way: gold 100, 500, 500, 500 (unseen), 1,500 (unseen) and 2,000; eight pearls; the gum, bone and moon relics (3 chests unseen until A finds them) |
| Levers, doors | a lever just left of the start opens a shortcut; one opens the temple's shortcut; the mid-boss opens the ruins' doors; levers in the jelly and spider caves open the last door [G100] | the same four: the west gate, the shrine's shortcut, the shrine's doors and the last door's two levers |
| Cracked rock | bombable walls and floors [G100] | 3 |
| Lore spots | a crashed UFO, an old spear, a skull, statues, a creature's remains [G100] | 7: a skull, an anchor, two statues, a sunk round vessel numbered 40, a ribcage and a breathing wall |
| Endings | 2 (with or without the three heads) [W], [TVT] | 2 |
| Stats | Level, Doors Open, Chests Open [W] | the same three, on the title |
| Goals | 3 [W], [GG] | 3 (below) |

### The structure

The raft floats over the Sunlit Shelf, a bowl of open water under the sun.
West, a gate stands shut with its lever on the far side; the long way round
to it passes a soft wall hiding the fins. Gumwell lies west (globs, smog
vents, clamper vents, a sunk anchor and, high in its west end, the duskling
figure); its floor drops to the Sunken Shrine. East of the shelf a nest of
nippers guards the way to Frond Hollow (fronds, a slater, the leech club in
its top corner, the miner's helmet behind a soft wall at its east end).
The shrine (tinfins, nippers, fronds, a whorl, statues) has a lever for the
shortcut up to the shelf, and the Abbot sits below its nave; when it falls
the shrine's doors open: west to Lantern Cave (glimmers, gropers, burrnuts,
the flagon, one lever of the last door), east past the sunk vessel's room
(the spine targe, the axolotl head) to the Stilt Pit (stilters, slaters,
haunts, the moon disc, the other lever, and a narrow way to the Still
Room). Both caves drop into the Red Deep, red flesh walls round a long gut
(dusksquids, mawworms, grinfish, gropers, haunts), with the lamp rod unseen
before the last door and the Gloamheart at the very bottom. Charges break
the shelf's floor (the zap gaff), Gumwell's side wall (800 gold) and Frond
Hollow's floor (1,000 gold).

## The three goals

| | Ours (wording ours) | Divers |
|---|---|---|
| Beacon | BRING HOME THE LEECH CLUB: its pearl opens at the raft | collect the Parasite Hammer [W], [GG] |
| Saucer | STILL THE GLOAMHEART | beat the final boss [W], [GG] |
| Alien | THREE HEADS, THEN STILL IT: all three heads held when the Gloamheart falls | all three heads, then beat the final boss [W], [GG], [G100] |

## Mechanics checklist

| Mechanic | How SOUNDINGS does it | Source | Test |
|---|---|---|---|
| The start | level 1, 500 HP each, 500 gold, depth 0000; enough for a poker of each element and a sip | [WIP], [G100] | sdg_01, sdg_02 |
| The raft | three unlabelled pictures (shop, kit, dive) and the log | [MM], [W] | sdg_01 |
| The shop | pages (LEFT/RIGHT), rows (UP/DOWN); kind and element, name, owned/most, price; relic costs at the bottom; white if affordable, purple if not; A buys, B or RETURN back | [MM] | sdg_02 |
| Bought once | tonics, charges and the ink sac are bought once and refill every surfacing | [G100], [ITEMS] | sdg_04 |
| The kit | storage on the left with the item's numbers, three divers' two hands on the right; A picks up and puts down (swapping), a hand item put into itself goes to storage; only at the raft | [MM], [W] | sdg_03 |
| Saving | only at the raft (surfacing, and buying or changing the kit there); leaving mid-dive loses the dive | [G100], [MM] | sdg_04 |
| Surfacing | heals all, revives the fallen, refills every use, banks the relics found, opens the pearls, writes the log | [MM], [G100] | sdg_04, sdg_12 |
| A wipe | everything since the last surfacing is gone (gold, XP, relics, pearls, chests, levers) | [G100], [MM] | sdg_11 |
| Swimming | free 8-way, slow; fins double it from any hand, even a fallen diver's | [MM], [G100] | sdg_34 |
| The dark | only a small light round the diver (the sun near the top, the green room lit); the caves are silent | [LIZ], [STATIC], [TVT] | sdg_25 |
| Music | fights, the Red Deep and the Still Room have music; the other caves none; in the Red Deep the deep's music plays on through its ordinary fights | [LIZ], [TVT] | sdg_25 |
| Sudden sounds | a creature that darts at the diver, or a groper or grinfish that locks on, breaks the silence with a sting | [STATIC] | (sound) |
| The panel | level and XP notches, gold, each diver's HP, depth and deepest | [MM] | (drawn) |
| A | opens a chest, pulls a lever, reads a note on the wall | [MM], [G100] | sdg_20 |
| B: items | the six hands with their uses, the relics carried, the heads; A uses a tonic, egg, lamp rod (on a diver picked with LEFT/RIGHT), charge or ink sac | [MM], [ITEMS] | sdg_24, sdg_20 |
| Chests | gold and relics at once, items as pearls opened at the raft; some unseen until A finds them | [G100], [MH] | sdg_12, sdg_20 |
| Levers and doors | the west gate and the shrine's shortcut each by a lever; the Abbot opens the shrine's doors; the last door needs both deep levers | [G100] | sdg_20, sdg_21 |
| Cracked rock | a charge used from the item menu breaks it nearby | [G100] | sdg_20 |
| Soft walls | look a little darker; the diver swims through | [G100], [WIP] | sdg_27 |
| Heads | three, behind soft walls, picked up by touch, shown in the item menu | [WIP], [G100] | sdg_20 |
| Visible creatures | touching one starts a fight; one sprite can be several | [W], [TVT] | sdg_28 |
| How they move | fizzles and tinfins dart; needlers and burrnuts wander; fronds, whorls and slaters stay and shoot; nippers burst from a nest as a school and chase; globs go wall to wall; smog eels come out of vents now and then, clampers when you come near; glimmers float in a square; gropers and grinfish come at you; stilters walk, hop and shoot; haunts turn up near you and pass through rock; dusksquids weave across; mawworms reach out of the wall | [W], [G100], [WIP] | sdg_28 |
| Shots | sting a diver (40) but never finish anyone | [W] | sdg_24 |
| Fleeing removes it | a creature beaten or fled from is gone from the map until the divers surface | [MM] | sdg_33 |
| Fights a sprite stands for | the original's lists, about three creatures a fight (table below); Frond Hollow's fronds are slater packs guarding the leech club | [SHEET] Encounters | sdg_28, sdg_38 |
| Gold | 9,999 at most | [SHEET] Misc | sdg_37 |
| Orders | Moss, Reed, Skip in turn: left hand, right hand, wait or run; an item: UP/DOWN attack or defend (the lamp rod: attack or mend), LEFT/RIGHT the creature or diver; B goes back | [MM] | sdg_05, sdg_06 |
| Turn order | the divers left to right, then the creatures left to right | [G100], [MH] | sdg_05 |
| A use is a use | any use costs one: a miss, a guard nobody needed | [MM], [G100] | sdg_06 |
| Running | a flat 25 %; a failure throws away the round's orders, those given before too | [G100], [MM] | sdg_07 |
| Shields | cut every hit while held (plate 20 %, targe 45 %, spine targe 50 %, bulwark 60 %; two multiply); defending with one squares what is left, for the defender's own blows and the ones he takes for another; two bulwarks and a defend take a blow to nothing; a hammer defends like a shield of its cut, a polearm only takes the blow | [SHEET] Weapons and Misc, [G100], [TIPS], [W] | sdg_08, sdg_09 |
| Elements | reef beats zap beats ooze beats reef; one weak spot each, shown by a sparkle; the glimmer changes colour every round | [W], [TVT], [G100] | sdg_10, sdg_18 |
| Levels | one bar for all, alive or not; HP 500 at 1, 2,200 at 13, 2,500 at 16; a level gained on a dive heals nothing | [G100], [WIP] | sdg_13 |
| Old blood, bitters | a level more for its holder (nothing at 16); a level less | [W], [ITEMS] | sdg_13 |
| Telegraphs | clampers, slaters and the Gloamheart's arms look at a diver, then strike him heavily | [W], [G100] | sdg_14 |
| Healers and revivers | fronds, the Abbot, haunts and the eyes mend; haunts and the Abbot bring the fallen back (the Abbot always, once both fronds are down) | [W], [G100] | sdg_15 |
| Spines | needlers, burrnuts and a hidden whorl prick whoever strikes them, except with a shield | [W], [ITEMS] | sdg_16, sdg_36 |
| The whorl's shell | hidden, it takes nothing from a weapon (a charge still does half) | [TVT], [G100] | sdg_36 |
| Covering | clampers, grinfish and the arms cover another creature | [W], [G100] | (data) |
| Double blows, drinking | tinfins strike twice, nippers, mawworms and the Abbot sometimes; nippers and mawworms drink what they bite; nippers start hurt | [W], [G100] | sdg_27 |
| Lowest HP | a dusksquid always goes for the weakest diver | [G100], [WIP] | sdg_17 |
| Unique finds | leech club (drinks half, not on the last blow), spine targe (bites back 50-150), lamp rod (mends 300-400, also on the map) | [G100], [ITEMS] | sdg_16, sdg_32 |
| Egg, charge, ink sac | an egg brings a fallen diver back; a charge 1,500-2,000 on one creature (can miss); the ink sac a sure escape from anything but a guardian, and 3,000 m of hiding on the map, used up only while moving | [W], [G100], [ITEMS] | sdg_19, sdg_32 |
| Winning | XP and gold for all, sometimes the creature's relic | [W], [G100] | sdg_05 |
| Relics | 9 of a kind at most; better gear costs gold and relics | [G100], [WIP] | sdg_02, sdg_31 |
| The Abbot | 5,000 HP with two fronds; strikes twice, mends, revives; drops the sun disc and opens the shrine's doors | [W], [G100] | sdg_21 |
| The Gloamheart | two eyes (4,000, weak to zap, mend) and two arms (5,000, weak to reef, look, cover) | [W], [G100] | sdg_22 |
| Endings | without the three heads, the plain one; with them, the other; the heads must be held at the last fight | [W], [TVT], [SECRETS] | sdg_22, sdg_23 |
| The code | MOON-WAKE (our word) on the title starts a new log with three eggs; while it is on nothing is written and no goal is earned, as with codes in the original's collection | [CHEATS], [CONV] | sdg_26 |
| Six of the same | all six hands holding the same item: the shop says something | [META] | (drawn) |
| The green room | big, empty, lit, with music, and nothing in it | [G100], [TVT] | sdg_25 |

### Creatures

| Ours | HP | XP | Gold | Weak | Relic | Ways | Divers |
|---|---|---|---|---|---|---|---|
| Fizzle | 300 | 10 | 20 | reef | pebble | darts; noodles about | Electric Eel |
| Needler | 200 | 30 | 30 | ooze | spiral | spines | Urchin |
| Frond | 600 | 20 | 15 | reef | foam | shoots; mends | Anemone |
| Nipper | 600 (starts 450) | 80 | 30 | zap | foam | nest of three; twice; drinks | Piranha |
| Glob | 400 | 22 | 40 | zap | gum | wall to wall | Slime |
| Smog eel | 350 | 20 | 25 | zap | - | out of vents | Slime Eel |
| Clamper | 800 | 50 | 40 | ooze | spiral | vents; looks; covers | Crab |
| Tinfin | 700 | 100 | 30 | ooze | cog | strikes twice | Mech |
| Whorl | 1,000 | 100 | 30 | ooze | - | shoots; hides in its shell | Snail |
| Glimmer | 500 | 150 | 100 | changes | - | square path; colour each round | Jelly |
| Groper | 1,000 | 300 | 50 | zap | gum | comes at you | Fishman |
| Burrnut | 580 | 100 | 200 | ooze | cog | spines | Walnut |
| Stilter | 500 | 120 | 100 | ooze | - | walks, hops, shoots | Sea Spider |
| Slater | 700 | 300 | 300 | ooze | - | shoots; looks | Isopod |
| Haunt | 1,200 | 500 | 50 | reef | bone (not in the deep) | through rock; mends; revives | Spirit |
| Dusksquid | 1,500 | 1,200 | 600 | ooze | cog | weaves; the weakest diver | Vampire |
| Grinfish | 1,200 | 800 | 1,000 | ooze | - | covers | Fangfish |
| Mawworm | 1,500 (hits 640-665) | 1,500 | 1,000 | zap | - | out of the wall; twice; drinks | Worm |
| The Abbot | 5,000 | 1,040 | 1,030 | ooze | sun disc | twice; mends 800; revives | Guelap |
| Gloam eye ×2 | 4,000 | - | - | zap | - | mends 600 | Skeor's eyes |
| Gloam arm ×2 | 5,000 (hits 430-470, the heavy blow 774-846) | - | - | reef | - | looks; covers | Skeor's arms |

The dusksquid's HP: 1,800 in the wiki and [SHEET], 1,200 in [G100] and
[WIP]; we took 1,500. The mawworm's and the arm's blows are [SHEET]'s
measured bands (the arm's band read as its targeted blow).

### What a sprite is in a fight

One sprite on the map is one of these fights, picked at random when it is
touched (after [SHEET] Encounters; the original's average is about 2.8
creatures a fight).

| Sprite (where) | Fights |
|---|---|
| Fizzle | fizzle; two fizzles; a needler |
| Needler | needler; needler and fizzle |
| Frond (shelf) | frond |
| Frond (Frond Hollow) | three slaters; slater, two nippers, slater |
| Frond (shrine) | frond, whorl, frond |
| Frond (Lantern Cave) | frond, glimmer, groper, frond |
| Nipper nest | three nippers; nipper, needler, nipper |
| Glob | two globs; three globs; glob and clamper |
| Smog vent | three smog eels; smog eel and clamper |
| Clamper vent | clamper and glob; smog eel, clamper, smog eel |
| Tinfin (shrine) | fizzle, frond, fizzle; fizzle, tinfin, fizzle; two tinfins |
| Whorl | whorl; frond, whorl, frond |
| Glimmer | three glimmers; glimmer, burrnut, glimmer, burrnut |
| Groper (Lantern Cave) | two gropers; frond, glimmer, groper, frond |
| Groper (Red Deep) | groper, grinfish, groper |
| Burrnut | two burrnuts; glimmer, burrnut, glimmer, burrnut |
| Stilter | four stilters; stilter, slater, stilter |
| Slater | three slaters; stilter, slater, stilter |
| Haunt (Stilt Pit) | two haunts; haunt, slater, haunt, slater |
| Haunt (Red Deep) | dusksquid and haunt |
| Dusksquid | dusksquid; dusksquid and haunt |
| Grinfish | groper, grinfish, groper |
| Mawworm | mawworm, two grinfish |
| The Abbot | frond, the Abbot, frond |
| The Gloamheart | arm, eye, eye, arm |

### The shop

| Item | Power / uses | Gold | Relics |
|---|---|---|---|
| Poker (reef, zap, ooze) | 200 / 25 | 100 | - |
| Gaff | 300 / 25 | 600 | reef 3 spiral, zap 3 pebble, ooze 2 gum |
| Harpoon | 400 / 25 | 1,500 | reef 2 spiral + 2 cog, zap 3 bone, ooze 6 gum |
| Club | 150 / 15 | 150 | - |
| Maul | 300 / 15 | 800 | reef 3 cog, zap 1 cog + 1 bone, ooze 1 cog + 1 gum |
| Plate (20 %) | 150 / 20 | 150 | - |
| Targe (45 %) | 250 / 20 | 600 | 1 foam + 2 spiral / pebble / gum |
| Bulwark (60 %) | 350 / 20 | 1,200 | reef 2 cog, zap 2 bone, ooze 4 gum |
| Sip | mends 500 / 4 | 200 | - |
| Draught | mends 500 / 8 | 1,000 | 3 foam |
| Hallow (one) | mends 2,500 / 8 | 2,000 | the moon disc |
| Bitters | mends 1,000 / 4, a level less | 400 | - |
| Egg | 1 | 400 | 1 spiral |
| Charge | 1 | 500 | 1 pebble |
| Ink sac (one) | 3 | 1,000 | the sun disc |
| Old blood (one) | always | 800 | - |

Found: the leech club (ooze hammer, 250 / 15), the spine targe (reef
shield, 250 / 10), the lamp rod (zap, 350 / 15), the flagon (mends 500 /
12), the fins.

### Readings we had to choose

- **The damage formula** (no source gives it; the fidelity review's
  proposal): power × (50 + 10 a level) %, so 60 % at level 1, 150 % at 10
  and 210 % at 16; half again on a weak spot, then 90-110 %. It fits a
  player's figures (a lance on a weak spot about 240) and the sheet's
  level-10 build that kills an eye in two rounds. Blows hit 90 % of the
  time (a charge 85 %); creatures' blows too. A heavy (telegraphed) blow is
  nine fifths of a normal one.
- **Shields** (measured by [SHEET], against a hit of 115-130): a lid held
  -20 % and defending -37 %, the thorn shield -55 % and -75 %, the tower
  -62 % and -94 %. Ours: the held cuts above, multiplied over a diver's
  shields; defending with a shield squares what is left (a bulwark 16 %,
  near the measured 6 %); two bulwarks and a defend leave nothing, as the
  guides say. The bonus covers the defender's own blows too ("either the
  defender or the unit being protected is hit"). Hammers defend like a
  shield of their cut without its passive cut (a club like a plate, a maul
  and the leech club like a targe); polearms only take the blow ([W]:
  rods have no defence).
- **Spines**: 60-120, never finishing a diver. **Egg**: back at half HP.
- **The XP curve** (no source gives it): 50, 150, 400, 900, 1,800, 4,600,
  8,800, 15,500, 26,000, 42,000, 65,000, 98,000, 145,000, 210,000 and
  300,000 to reach levels 2-16. It was steepened after the review so the
  pace is about the original's (see Pace); the guides' milestones hold:
  level 3 in the shallows, the Abbot near 10, a long grind in the deep to
  14. HP between the known points (500 at 1, 2,200 at 13, 2,500 at 16) is a
  straight line.
- **Fight messages** stay 50 frames; A moves on after 20.
- **Gold prices of weapons**: none are documented; ours are above. The
  starting 500 buys three pokers and a sip, as the guides say it should.
- **Relic costs** (settled by [SHEET]): the WIP guide's coral and poison
  columns are swapped (the sheet labels them, and the 100% guide buys
  poison gear with candy), so we swapped them back.
- **The medium potion** (settled by [SHEET] and [ITEMS]): 500 a use, 8
  uses; the wiki's 1,000 × 4 is the outlier. Bombs: 3 at most (no source
  gives a stock).
- **Where sources differ on a creature**: the urchin's XP and gold (25-40:
  30), the piranha's XP (75-87: 80), the slime's (20-25 and 35-40: 22, 40),
  the walnut's HP (550-600: 580), the vampire's HP (1,200-1,800: 1,500), the
  worm's HP (1,000-2,000: 1,500), the sea spider's XP and gold (100-150: 120,
  100). Relic drops follow the 100% guide; a relic drops 10 % of the time
  ("a small chance", "quite rare").
- **Creatures' choices**: a look 45 % of turns, a cover 25 %, a mend 40 %
  (eyes 60 %) when someone is below 60 %, a revive 30 %, a noodle 20 %; a
  whorl hides 30 % and stays hidden half the time.
- **Bosses**: no running and no ink sac against a guardian.
- **Respawning**: a creature beaten or fled from stays gone for the rest
  of the dive and comes back when the divers surface (the original's loop is
  "fight, swim home, repeat"; no source says they return mid-dive). Before
  the review they came back after 30 s; that was ours, not a source's.
- **Numbers on the map**: swimming 0.75 px a frame (fins 1.5); the light 54
  px; a shot 40; a metre of depth (and of ink) is a pixel.
- **The gift**: given when the leech club's pearl opens at the raft.
- **Saving at the raft**: buying and changing the kit at the raft are
  written at once (the raft is the surface).
- **Heads** are picked up by touch.
- **Hammers** (settled by [SHEET] and [WIP]): the original sells two tiers
  of hammer, the mallet and the sledge, plus the Parasite; we kept two. The
  leech club is the only hammer above the maul.
- **The code word**: MOON-WAKE (it was RAFT-EGGS, too close to the
  original's).

### Pace

The demo player's time to still the Gloamheart from a new log (seed 1):
14.8 minutes of game time and 11 dives before the fidelity review; after
it (the damage formula, the steeper XP curve, the bigger fights, the
stronger shields, no respawning mid-dive, 10 % relics) 67 minutes and 44
dives, with the leech club at about 27 minutes. People took 6.5 to 10
hours over the original ([TIPS], [LIZ]); a demo player that never gets
lost and skips every message it can is several times faster.

### Controls (owner's layout: A is the main action)

The original already uses A (Button 2) to choose, buy, interact and give
orders and B (Button 1) for the item menu and to go back, so nothing moved.

| Input | Action |
|---|---|
| D-pad | swim; move in menus |
| A | open, pull, read on the map; choose, buy, give an order |
| B | the item menu on the map; back in menus |
| START | pause |

| Not confirmed | Our reading |
|---|---|
| B while giving orders | goes back to the diver before |
| Picking up a head | by touch (A works on chests, levers and notes) |
| B on the raft | back to the title |
| The order menu's cursor | UP/DOWN through left hand, right hand, wait, run |

## What is ours

- **Name:** SOUNDINGS (1986, Beamdown Softworks).
- **Divers:** Moss, Reed and Skip, three axolotls (olive, gold and pink).
- **Places:** the raft and its shed; the Sunlit Shelf, Gumwell, Frond
  Hollow, the Sunken Shrine, Lantern Cave, the Stilt Pit, the Still Room and
  the Red Deep; the whole map.
- **Creatures:** all 18 above, the Abbot and the Gloamheart.
- **Items:** every name above, the relics (foam, gum, pebble, cog, spiral,
  bone, the sun and moon discs), the three heads.
- **Music:** "Soundings" (title), "The Raft", "Something in the Dark"
  (fights), "The Abbot", "The Gloamheart", "The Red Deep", "The Still
  Room", "Toward the Light" (ending) and three jingles.
- **Words:** every message, the notes on the walls, both endings, the goal
  lines and the code word.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
UFO 40 goals, which are Divers' own (gift, gold, cherry). The respawn
timer is a reading (see above), not an addition. The original's
terminal code and meta message live in the collection's terminal and its
meta messages; UFO 40 has no terminal, so the code is entered on the
cartridge's title (as CLARION CALL does) and the message shows in the shop.

## Tests

`tests/sdg_01` … `sdg_38` drive the rules with button presses, setting up a
moment with cheats where needed: the title, the shop, the kit, surfacing and
saving, the turn order, uses, running, shields and covering, the damage
arithmetic, a wipe, pearls and the gift, levels, telegraphs, mending and
reviving, spines and the leech club, the dusksquid, the glimmer, the ink
sac and the guardians, levers, cracked rock, unseen chests and heads, the
Abbot and the Gloamheart with both endings, the item menu and shots, the
music, the code, the map's checks and the creatures' numbers, a nest
bursting, relics, the egg, lamp rod and charge, respawning, the fins,
pausing, the whorl's shell, the gold cap and the fights a sprite stands
for. The demo player (`sdg_bot.c`) plays from the title with real
presses: it shops, sets the kit, dives along a breadth-first field, opens
chests and levers, breaks rock, grinds where its level says and surfaces
when hurt; `sdg_29` stills the Gloamheart (beacon and saucer) and `sdg_30`
fetches the three heads first (all three goals); each takes about 245,000
frames. Everything the game and
the demo player decide uses whole numbers, so a seed plays the same on
every platform.

## Sources

- [SHEET] The community data sheet behind the WIP guide (tabs Enemies,
  Encounters, Weapons, Items, Locations, Misc): measured shield cuts, the
  encounter lists, the full chest list, max gold 9,999 and the mawworm's
  and the arm's damage bands.
  https://docs.google.com/spreadsheets/d/1D6P9KFkEUvreyWtHJzBFSn8rkGoyQT8l7l9Zuta_3lU

- [W] UFO 50 Wiki (Miraheze), "Divers" (raw page): the elements, the shop
  and key items, the found items, the relic letters, the enemy table, the
  goals and secrets. https://ufo50.miraheze.org/wiki/Divers
- [MM] Steam guide "The missing manuals - How to play UFO 50 games",
  section 27: the base, the shop and equip screens, diving, the panel, the
  item menu, surfacing, combat and running.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [G100] Steam guide "Divers 100% Guide": saving, levels, equipment,
  exploration, turn order, running, the walkthrough, every item, enemy and
  boss, the maps, the goals, the heads, the green room and the lore spots.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3658555453
- [WIP] Steam guide "Divers Guide [WIP]" and comments: the starting state,
  HP by level, the enemy table with damage, the weapon table with relic
  costs, relic sources, the heads. https://steamcommunity.com/sharedfiles/filedetails/?id=3339503135
- [ITEMS] Steam guide "What the shop / loot items in Divers do" and
  comments: every potion, the mist orb, godblood, the found items, shields
  ignoring spines. https://steamcommunity.com/sharedfiles/filedetails/?id=3362360336
- [GG] Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [TIPS] Steam thread "[Divers] General tips".
  https://steamcommunity.com/app/1147860/discussions/0/4849904828213514294/
- [SECRETS] Steam thread "Secrets in Divers?".
  https://steamcommunity.com/app/1147860/discussions/0/4849904427677811564/
- [CHEATS] Miraheze, "Cheats" (FREE-EGGS). https://ufo50.miraheze.org/wiki/Cheats
- [META] Miraheze, "Meta Messages". https://ufo50.miraheze.org/wiki/Meta_Messages
- [TVT] TV Tropes recap. https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game27Divers
- [LIZ] Lizstar's Trashcan, UFO 50 retrospective part 27.
  https://lizstar64.github.io/reviews/2024/10/16/UFO50-27.html
- [STATIC] The UFO 50 Diaries: Divers. https://staticcanvas.substack.com/p/the-ufo-50-diaries-divers
- [CONV] UFO 40's conventions notes (`00-ufo50-conventions.md`): codes
  turn off saving and goals.
