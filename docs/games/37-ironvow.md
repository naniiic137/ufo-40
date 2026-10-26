# 37 · IRONVOW

*Internal design document. Not shown in the product.*

## Tribute to

**Valbrace** (UFO 50 game #37, Mossmouth, "January 1988"). The rules were
researched from text only: the community wiki's raw page (re-read on
2026-10-26 for this build: the enemy table, groups of 1 to 4, at most two
in a row, foes that do not pass through each other, the spell list), its
Cheats and Multiplayer pages, the Steam guides "The missing manuals"
(section 37: the HUD, both control sets, casting, saving) [MM] and
"Valbrace Ultimate Guide" by Llamakazi (its text only: per-floor notes,
items, enemies, spells, secrets, the Red Knight's lines and, in the
comments, his favour rules) [GUIDE], the "Gift, Gold & Cherry" guide,
Steam threads, TV Tropes and written reviews (listed under Sources; the
full notes are in the research folder, `37-valbrace.md`). No UFO 50
images, video, sprites, music, maps or text were used as references, and
nothing was taken from a UFO 50 install. The guide's floor maps are
pictures and were not looked at.

**Map type: hand-made, fixed floors; random encounters and much of the
loot.** Valbrace's floors, fixed chests, glyph murals, bosses and NPCs are
the same every game; what comes out of a flame, most loot from floor 2 on,
the Crone's later shelves and the Red Knight's rewards are rolled [GUIDE],
[LIZ]. Ours are the same: seven floors drawn for UFO 40 from scratch
(`ivw_floors.c`), with only the structure kept (floor count, the three
parts, sizes, the order things open in, roughly where the key rooms and
bosses sit). The rolls are seeded per run.

| Full scale | Valbrace | IRONVOW |
|---|---|---|
| Players | 1, no 2-player mode [MH], [MH-MP] | 1 (nothing to grey out) |
| Floors | 7 (0-6), three biomes: Dungeon 0-2, Hive 3-4, Star Castle 5-6 [MH], [GUIDE] | 7 (0-6): the Gaol 0-2, the Warren 3-4, the Starvault 5-6 |
| Floor sizes | not given; the Map spell shows 16 x 16; floor 0 a short tutorial, floor 2 "large"; floors take 30+ minutes [GUIDE], [POPCAR] | floor 0 16 x 16 (126 open tiles), floor 1 24 x 24 (308), floors 2-6 32 x 32 (441, 554, 536, 728, 537) |
| View | first person on a grid, a step a tile, 90-degree turns [MH], [MM] | the same; steps and turns glide over 8 and 6 frames |
| Foes on the map | flames that chase; touched, a fight; drawn from the floor's pool; endless [MH], [MM], [POPCAR] | the same |
| Groups | 1 to 4, at most two a row; up to ~6 with summons [MH], [STATIC] | 1-2 on floor 0, 1-3 on 1, 2-3 on 2, 2-4 below; summons fill free places |
| Combat | real time on the X axis, two rows, guard, roll, three-hit combo, stamina, exhaustion [MH], [MM] | the same |
| Spells | 17, glyphs on the walls of floors 1-5 (4, 4, 3, 3, 2), one never taught [GUIDE] | 17, our own glyphs, taught on the same floors in the same numbers; WARD never |
| Foes | 16 kinds (slime to hand, silver formid, guard) [MH], [GUIDE] | 17: ooze, gnawwing, lidmaw, tusker, pedlar, boneguard, skink, egg, grub, chitin, puffball, coiler, puppet, mummer, grasper, silver chitin, regent's guard (and the beetle gone wild) |
| Bosses | Phantom Knight (end of 2), Brood Queen (end of 4), Abyss Lord (6) [GUIDE] | the Wraith Warden, the Comb Matron, the Pit Regent |
| People | the Crone's shop (1-6), the Red Knight (1, 3, 5) [GUIDE] | the pedlar (1-6), the Amber Knight (1, 3, 5) |
| Thrones | one a floor from 1, 3 points each; cost rising, the last free [GUIDE], [TVT] | 150, 300, 450, 600, 750 motes, then free |
| Secrets | 4 [GUIDE] | 4: the same chain |
| Saving | on arriving at a new floor only; death restarts the floor [GUIDE], [MM] | the same |
| Stats | Battles, Deaths, Secrets [MH] | the same, on the title |
| Endings | 2: plain, and with the Red Star Sword [GUIDE] | 2: the grey ending and the dawn ending |
| Title | New Game, Continue, an attract mode [GUIDE] | the same |
| Run length | casual about 5 h [POPCAR]; speedrun 7:08 (Gold/Cherry, 2nd place) [SR] | the demo player, which knows every floor and reacts in one frame, takes 10.5-14 min for gold and cherry (38,000-50,000 frames over eight seeds) with about 70 battles; a player mapping the floors and learning the fights will take hours |
| Goals | 3 [MH], [GGC] | the same 3 |

## Mechanics checklist

| Mechanic | How IRONVOW does it | Source | Test |
|---|---|---|---|
| Walking | UP a tile forward, DOWN a tile back, LEFT / RIGHT a quarter turn; a wall is a bump | [MH], [MM] | ivw_02 |
| Time | every step and every interaction passes a turn; the clock moves when it does | [GUIDE], [MM] | ivw_02, ivw_03 |
| The menu | A: PACK and PASS TURN always, then what the knight faces or stands on (doors, levers, torches, fungus, lanterns, gem panels, switches, glyphs, chests, wells, the throne, the pedlar, the knight, the socket, things on the floor, the searched room); B backs out | [MM], [GUIDE] | ivw_03 |
| Doors and keys | doors open from the menu; a locked door takes an iron key (keys go on a ring) | [MM] | ivw_03 |
| Spikes | up two turns, down two; PASS TURN times them | [GUIDE] | ivw_04 |
| Pits | a fall is death unless floating | [GUIDE] | ivw_04 |
| Slime | wet slime poisons; poison takes health down to 1 but never kills | [GUIDE], [MH] | ivw_04 |
| Lava | burns, and burning can kill | [GUIDE] | ivw_04 |
| Flames | they gather at the floor's haunts (one every 8 turns, up to the floor's cap of 2-4), hunt the knight within 7 tiles (three moves in four), and touching one starts a fight; veiled, he walks through | [MH], [MM], [POPCAR] | ivw_05 |
| No mid-floor heal | health never comes back by itself; mana does as turns pass (a cowl speeds it) | [MM], [GUIDE] | ivw_10 |
| The fight's moves | LEFT / RIGHT strafe; A strikes, again for a combo of three (the third 1.5 times); DOWN held guards; DOWN then LEFT / RIGHT within 8 frames rolls (frames 1-12 untouchable, 18 stamina) | [MH], [MM], [GUIDE] | ivw_06 |
| Stamina | swings (cheaper with MIGHT), blocks and rolls cost it; it does not come back while guarding; at 0 the knight is exhausted (no blows, guard or roll, half speed) until half is back | [MH], [MM] | ivw_06 |
| Lining up | a blow lands on whoever stands in line with it, both ways | [MH] | ivw_06, ivw_09 |
| Guarding foes | most foes guard until they strike: a blow on a guard clinks off; three in a row and the foe hits back at once | [MH], [GUIDE] | ivw_07 |
| Counters | a blow during a wind-up cuts the strike short and stuns longer (50 frames) | [GUIDE] | ivw_07 |
| The knight's guard | a blocked blow costs stamina by the shield's share; each shield holds a number of blocks a fight (buckler 5 ... wyrm 18), then gives way; a guard with no stamina breaks (a stagger, exhaustion) | [MH], [GUIDE] | ivw_07 |
| Rows | two rows, at most two a row; most blades reach only the front; a glaive or the oathblade reaches the back; back and front trade places now and then; the back row rarely strikes | [MH], [MM], [GUIDE] | ivw_09 |
| Foes' own ways | skink: a punch, then a stamina-draining cloud, always in turn; coiler: a swing, then an unblockable charge (it flashes yellow); chitin: strikes the moment the knight's stamina runs out, even from the back; egg: hatches into a grub; grub: bites on and on, no guard; puffball: mends friends or rains spikes; lidmaw: a chest that throws pellets; mummer: lobs, calls puppets; grasper: jumps about, drops rocks, throws bolts; boneguard: a wide swing; tusker: blows that drain stamina | [MH], [GUIDE] | ivw_08 |
| Casting | B starts a spell; the pad traces strokes; when the time is up (90 frames, 6 more a point of LORE) the glyph is read; a known glyph or its mirror, with the mana, casts; anything else fizzles and costs 2; the fight waits while the glyph is drawn | [MH], [MM], [GUIDE] | ivw_10 |
| Any spell from the start | no spell is unlocked: the walls only teach the glyphs | [GUIDE], [LIZ] | ivw_10, ivw_28 |
| Spells outside a fight | CHART (what has been seen, 16 x 16), OPEN (an outlined wall becomes a door), KINDLE (lights a torch), GALE (flames down a passage blown away; things lying there pushed to its end), VEIL (10 turns + 1 a point of LORE, through flames and bosses), FLOAT (10 turns; ending over a pit is death), FLARE (a flame ahead starts its fight burning), MEND, RESTORE | [MH], [GUIDE] | ivw_11 |
| Spells in a fight | LANCE (through any guard, down the line, back row too, a short stun), KINDLE (singes, slows), GALE (out of the fight, no motes), MIRE (the front row slowed), RIME (frozen: no guard, no blows; a coiler three times as long), THUNDER (every foe), HUSH (no casting), TWIN (a second knight strikes), WARD (takes blows, unblockable ones too), QUICKEN (much faster feet), VEIL (no foe can guard) | [MH], [GUIDE] | ivw_12 |
| Stats | MIGHT: harder blows, cheaper swings, quicker breath, eases heavy armour; GRIT: 6 stamina a point, 10 health every 3; LORE: 4 mana a point, harder spells, more time to draw | [GUIDE] | ivw_13 |
| Thrones | one a floor from floor 1; 150, 300, 450, 600, 750 motes, the last free; 3 points to place | [GUIDE], [TVT], [STEAM-HELP] | ivw_13 |
| Motes | each foe's worth (the wiki's 10-60 range, ours); the first 6 fights of a floor pay it times (2 + floor) / 2, the next 6 the plain worth, then 5 at most | [TVT], [MH] | ivw_14 |
| The pack | 10 places, 12 at most; gear is worn from it; tonics are drunk from it; monster parts are readied and go off as the next fight starts; what is set down stays there | [MH], [GUIDE], [STEAM-RK] | ivw_15 |
| Gear | 12 blades (dirk to Dawnbrand, MIGHT needs 3-15; under it a blade hits for two thirds and costs half again), 9 armours (plate slows and costs swings), 7 shields and bracers (bracers cannot block) | [GUIDE] | ivw_15, ivw_09 |
| Tonics and parts | red heals 40; blue stamina and mana; black harder blows, green haste, grey a sturdier guard, yellow foes stunned, for the next fight; ooze and bone skulls bounce and stun; a bat heart heals; a tusk stabs; a venom sac poisons; the juggler's ball bounces all fight; a bone scares nearby flames off; a chitin head leaps to a seen tile up to 8 away | [GUIDE] | ivw_15 |
| The pedlar | trades for shards; floor 1's shelf is fixed and holds a boar shield, later shelves are rolled; she buys what the knight carries; stealing always fails: she fights (150 motes), and beaten she leaves the floor and the thing is his | [GUIDE] | ivw_16 |
| The Amber Knight | three lines a floor; gifts build favour, a reward each 10 (5-14 shards, a tonic, +10 health up to +20, +1 pack place up to 12); struck at, he leaves for good | [GUIDE], [STEAM-RK] | ivw_17 |
| The secrets | the oathblade from the pillar hall's last search; given to the knight, the amber coin; the coin in a sweet well, a wish; after HONOUR, the knight's Dawnbrand (only with room for it; otherwise lost for good) | [GUIDE] | ivw_17, ivw_18, ivw_23 |
| Wells | sweet: heals (three drinks); foul: poisons; the coin in a sweet well: CHANGE (-2 each stat, 7 points), TREASURE (20 shards, a thing, a weapon), HONOUR; in a foul well: CHANGE gives only 3, the others nothing | [GUIDE] | ivw_18 |
| The coin | flipped from the pack: heads heals a little, tails poisons | [GUIDE] | ivw_15 |
| Levers | gates open somewhere (the message does not say where); a boulder fills a pit; a trap drops one on the knight | [GUIDE], [POPCAR] | ivw_19 |
| Slime switches | swap which slime is wet across the whole floor | [GUIDE] | ivw_19 |
| The shifting comb | floor 4's levers raise some walls and lower others | [GUIDE] | ivw_19, ivw_24 |
| Gem panels | floor 2: swap gems until no two alike touch; 4-8 shards fall out | [GUIDE] | ivw_19 |
| Torches and lanterns | the Gaol's torches are put out from the menu and lit with KINDLE; the Warren's fungus lights when poked (all stamina gone) or dims; the Starvault's lanterns take 30 motes to light and give 10 back put out | [GUIDE] | ivw_19 |
| Searched rooms | floor 2's pillar hall: a fight, a find, a fight, a find, the oathblade; floor 4's comb room the same, ending in the grub egg; floor 6's vault room four rolls | [GUIDE] | ivw_23, ivw_27 |
| The Wraith Warden | guards the stairs of floor 2; dives and slashes, throws his shield, and below full health may charge flashing yellow, striking 1 + 3 x (lost share) times (each blockable, heavy on stamina); every torch of the Gaol lit: he starts at half health; every one out: he leaves the wraith shield (mana on each block); VEIL slips past him | [GUIDE], [MH] | ivw_20 |
| The Comb Matron | guards the stairs of floor 4 with two chitins; her two claws have their own health and nothing reaches her until both are broken; she spits acid, coughs a cloud, lays eggs; she leaves the mould cloak (spores bite attackers; spending stamina wins back lost health) | [GUIDE], [MH] | ivw_20 |
| The Pit Regent | rises when the three stones are set in the floor 6 pillar; three phases (a sweep no guard holds from the second, quicker blows in the third); his stones mend him (moss), throw fire (cinder) and call his guards (frost); they hover high and sink low to be struck; he cannot be poisoned, burned or frozen | [GUIDE] | ivw_20 |
| The escape | beaten, the vault shakes and the way up opens; walking out is the win | [MH] | ivw_22 |
| Saving and death | the save is made on arriving at each floor; death ends the vow, and A starts the floor again from that save; CONTINUE on the title does the same | [GUIDE], [MM] | ivw_21 |
| The beetle | the grub egg hatches after 6 fights into a fighting grub (it bounces about each fight), after 8 more a war beetle; readied, the beetle fights in the knight's place with its own health (whole after each fight); B calls it back; fallen, it is gone; long in use it may go wild, and silver chitins join the pools | [GUIDE], [TVT] | ivw_27 |
| Goals | gift: 8 in any stat; gold: slay the Pit Regent and climb out; cherry: slay him with the Dawnbrand in hand | [MH], [GGC] | ivw_22 |
| Records | battles, deaths and secrets found, on the title | [MH] | ivw_23 |
| Attract mode | left on the title, the demo player fights through floor 5; any button stops it | [GUIDE] | ivw_29 |
| Playable start to end | the demo player, with real presses from the title, earns all three goals | | ivw_24, ivw_25 |

### Readings we had to choose

- **The buttons:** Valbrace's main button (Button 2 = A) already opens the
  menu and strikes, and Button 1 (B) casts [MM], [MH]. The owner's layout
  (the main action on A) is therefore the original's own here; it is
  noted below anyway, and the card's CONTROLS page can swap A and B.
- **What "main" means here:** the menu while exploring (doors, levers and
  everything else are done through it) and the strike in a fight.
- **Turning:** turning on the spot passes no turn; steps and every menu
  action do. The sources say every step and interaction passes a turn and
  are silent about turns.
- **Casting:** the window runs from the press of B whether or not B stays
  held, and ends on its own (90 frames, 6 more a point of LORE); a fizzle
  costs 2 mana, a known glyph without the mana costs the same; the mirror
  image is LEFT and RIGHT swapped.
- **The flames:** they hunt within 7 tiles by path and move three turns in
  four (otherwise a flame could never be outrun); a new one every 8 turns
  ("swarmed every ~10 steps" [POPCAR]); none gathers within 4 tiles of a
  lit lantern, which is our use for the lanterns the guide describes
  without an effect.
- **Fungus:** the knight's hint that "others thrive in shadow" [GUIDE] and
  the Hive's hidden doors being "much harder to see" became one rule: in
  the Warren an outlined wall only shows within 3 tiles of a lit fungus
  (OPEN works on it either way).
- **Torches:** they count only for the Warden (half health with every
  Gaol torch lit, the wraith shield with every one out), as the guide's
  two findings; floors 0 and 1 are counted as they are left.
- **Bosses on guard:** a boss notices the knight on its tile or beside it
  (our floors' halls are wide; the original's boss blocks the way).
- **The "Guard" on the wiki** (worth 0, no details): the Pit Regent's
  guards, called by the frost stone.
- **The Abyss Lord's gems:** sources say each gem grants powers, one
  summons foes, and they can be broken by ranged attacks or in melee when
  low; ours are moss (mends him), cinder (fire) and frost (guards), sinking
  low for 150 frames now and then; phases at two thirds and one third.
- **The escape:** a walk out through a way in the pillar hall's south wall
  that opens when he falls; no timer.
- **The Red Star Sword:** ours, the Dawnbrand: MIGHT 5, a held A charges a
  blow no guard stops, and at nine tenths of full health each swing sends a
  bolt of light up the line (2 mana) that strikes flyers, the back row and
  stones. The cherry counts if it is the blade in hand at the kill.
- **Numbers no source gives:** start 5 / 5 / 5, 60 health, 70 stamina, 30
  mana, a pack of 10; health 50 + 10 per 3 GRIT (+ the knight's gifts),
  stamina 40 + 6 per GRIT, mana 10 + 4 per LORE; a blade's damage times
  (8 + MIGHT) / 13; armour takes 5-40 percent off a blow; shields 5-18
  blocks; swings 13-46 frames by weight; stamina back 3 + MIGHT / 3
  sixteenths a frame; foes' health 13-46 (bosses 130-340) and blows 3-16;
  sweet wells 3 drinks of 25; a lever's trap 12; spikes 8; poison 1 a turn
  (never below 1); lava 2 a turn; FLOAT 10 turns; VEIL 10 + LORE turns.
- **The knight's rewards:** 45 percent shards, 30 a tonic, 15 more health
  (until +20), 10 a pack place (until 12), as the guide's comments list
  them; the odds are ours.
- **Floor layouts:** all ours. Floor 1 has the boulder lever that fills a
  pit before a chest and an outlined wall before a room holding LANCE's
  glyph; floor 2 the pillar hall in the middle, gem panels, a spiked way
  to the key and the Warden's hall at the south end; floor 3 a chasm across
  its middle (FLOAT, or a slime tunnel the switch dries); floor 4 the comb
  of shifting walls in three bands; floor 5 two rivers of lava with a gated
  bridge; floor 6 three wings (one over lava, one behind an outlined wall
  with a false one over a pit near it, one behind a locked door) round the
  socket hall.

## What is ours

- **Name:** IRONVOW (1988), "a knight's descent into Blackspire Keep".
- **The people:** the knight (no name), the pedlar, the Amber Knight, the
  Pit Regent; the Wraith Warden and the Comb Matron.
- **The foes:** oozes, gnawwings, lidmaws, tuskers, boneguards, skinks,
  eggs and grubs, chitins, puffballs, coilers, puppets, mummers, graspers,
  silver chitins, the Regent's guards.
- **The things:** every item's name and look, the amber coin, the
  oathblade, the Dawnbrand, the cinder, moss and frost stones, motes and
  shards.
- **The glyphs:** all seventeen drawn for UFO 40 (none equal to the
  original's, checked in `ivw_10`), and the spells' names.
- **The floors:** all seven, from scratch.
- **Music:** "Ironvow", "The Gaol", "The Warren", "The Starvault", "The
  Empty Throne", "Steel on Steel", "The Pit Regent", "Daylight" and the
  jingle "The Vow Is Broken".
- **Words:** the story, the knight's lines, the pedlar's, the wishes, both
  endings, every label.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
goals, which are Valbrace's own. The owner's button rule needed no change
(see Readings). The Terminal codes (`4RCH-M4GE`, `BUGG-BODY`) belong to
the UFO 50 collection; UFO 40 has no terminal, so they are left out.

## Controls

| Where | Input | Action |
|---|---|---|
| Exploring | UP / DOWN | a step forward / back |
| | LEFT / RIGHT | turn a quarter |
| | A | the menu (PACK, PASS TURN, and what is in front or underfoot) |
| | B, then the pad | draw a glyph |
| Menus | pad, A, B | choose, take, back |
| Fighting | LEFT / RIGHT | strafe |
| | DOWN held | guard |
| | DOWN, then LEFT / RIGHT at once | roll |
| | A (again) | strike (combo of three); held with the Dawnbrand or coiler bracers: a charged blow |
| | B, then the pad | a spell (the fight waits) |
| | B with the beetle in | call it back, step in |
| All | START | pause |

## Not confirmed

Confirmed by [MM] and [MH]: the movement, the menu on A with PACK and
PASS TURN, B to cast (also in fights), guard, roll, combo, exhaustion.

| Control or rule | Our reading |
|---|---|
| Whether B must stay held while drawing | no: the window runs out on its own |
| Whether turning passes a turn | no |
| Charge attacks: how long A is held | 40 frames |
| The roll's window after DOWN | 8 frames |
| The beetle: whose buttons | the player steers it like the knight; B (the cast button, as [GUIDE] says) calls it back |
| Lanterns' effect | none spawn near a lit one |
| Fungus torches' effect | they show outlined walls nearby |
| Every number in Readings | as listed |

## Tests

`tests/ivw_01` … `ivw_29` drive each rule with button presses, or set up a
moment with cheats and then play it. The demo player (`ivw_bot.c`) plays
the way a player with the floor maps in hand would: a shortest-path walk
(doors opened, locked ones unlocked, outlined walls opened with OPEN, pits
and lava floated over, spikes timed), a plan over both layers of the
shifting comb, the floor's own steps (levers, the switch, the knight, the
well), then chests, torches, gem panels (solved by search), the searched
rooms, the throne and the stairs; in a fight it lines up with the nearest
foe, cuts each wind-up short, guards what it cannot, rolls out of what no
guard holds, heals by spell and throws LANCE and THUNDER. `ivw_24` and
`ivw_25` play from the title to the dawn ending on two seeds, with every
boss, every secret and all three goals.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Valbrace" (raw page): infobox, gameplay,
  combat, rows, enemies with essence, spells, goals.
  https://ufo50.miraheze.org/wiki/Valbrace
- [MH-CHEATS] UFO 50 Wiki, "Cheats". https://ufo50.miraheze.org/wiki/Cheats
- [MH-MP] UFO 50 Wiki, "Multiplayer" (Valbrace not listed).
  https://ufo50.miraheze.org/wiki/Multiplayer
- [MM] Steam guide "The missing manuals - How to play UFO 50 games",
  section 37: the HUD, exploring and fighting controls, casting, saving.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [GUIDE] Steam guide "Valbrace Ultimate Guide - Maps, Spells, and More!"
  (Llamakazi), text and comments only: per-floor notes, items, enemies,
  spells, secrets, the Red Knight, the thrones, the goals.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3334110356
- [GGC] Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [STEAM-HELP] Steam thread "Valbrace help" (throne cost).
  https://steamcommunity.com/app/1147860/discussions/0/4852154959747437380/
- [STEAM-RK] Steam thread "Valbrace red knight upgrade limit?".
  https://steamcommunity.com/app/1147860/discussions/0/592887665229111763/
- [TVT] TV Tropes recap (through the research notes; the page refused a
  fetch on 2026-10-26). https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game37Valbrace
- [LIZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 37".
  https://lizstar64.github.io/reviews/2024/10/18/UFO50-37.html
- [STATIC] Static Canvas, "The UFO 50 Diaries: Valbrace".
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-valbrace
- [POPCAR] Popcar, "Reviewing every UFO 50 game".
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [SR] speedrun.com (search listing): Valbrace Gold/Cherry 7:08 (2nd place).
