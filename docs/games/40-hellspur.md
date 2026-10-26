# 40 · HELLSPUR

*Internal design document. Not shown in the product.*

## Tribute to

**Grimstone** (UFO 50 game #40, Mossmouth, "June 1988"). The rules were
researched from text only: the community wiki's raw page, the Steam guides
("Grimstone Ultimate Guide", "The missing manuals", "Gift, Gold & Cherry",
the teeth-farming guide), Steam threads and written reviews (listed under
Sources; the full notes are in the research folder, `40-grimstone.md`). No
UFO 50 images, video, sprites, music, maps or text were used as references,
and nothing was taken from a UFO 50 install.

**Map type: fixed.** Grimstone's world, towns and dungeons are hand-made and
the same every game; only which wisps appear, and where, is random [MH]
[GUIDE]. Ours is fixed too, and all our own: the Territory of Calico (a
128 × 96 painted map, `spur_world.c`), the Scorched Reach in Hell, eleven
hand-laid towns and nine small places (`towns` in `spur_world.c`), and
twenty dungeon floors cut by a seeded generator from a table of what each
holds (the same floors every game). Only the structure follows the
original: how many towns and dungeons, which way they gate one another, and
roughly where they sit (the start in the south-west, a river to cross, a
prison to the north-east, a mountain pass, a desert of mesas, a ghost town
reached through a mine, burnt hills crossed by tunnels, the last frontier
town, a gate into Hell, a tower of four riders, and a crater citadel at the
end).

| Full scale | Grimstone | HELLSPUR |
|---|---|---|
| Posse | 8 strangers, the angel saves 1 to 4; the rest die [MH] [LIZ] [MM] | the same: Tucker, Sage, Hollis, Quill, Vera, Hattie, Cora and Biscuit the hound |
| Towns and safe places | about 11 (Pleasant, Santonio, Mission Francesco, Heston, Auster, Fort Jason, Rio Valle, El Pasaje, Lawbuck, Agartha, the monastery) [GUIDE] | 11: Gladwell, Meridian, St. Clare Mission, Hope Springs, Steepleton, Fort Patience, Dolores, Hollow Gap, Last Chance, Ashmarket, the Hermitage |
| Smaller places | the burnt saloon, Sam's house, the bandit hideout, Mission Angelina, the island house, the cactus temple, a cloud saloon, a demon gunsmith [GUIDE] | 9: the Golden Garter (the opening), Pike's shack, the Outlaw Roost, Our Lady of Thorns, the widow's house, the Agave Shrine, the Sky Bar, the demon smithy, the summit |
| Dungeon floors | about 20: desert cave, first mine, prison ×2, mountain pass, mission basement, hideout mine, El Pasaje mine, smugglers' pass, serpent's path ×2, spirit realm ×2, Black Tower ×4, Grimstone ×2 and its top [GUIDE] | 20: Dry Wash Cave, Sixpenny Mine, the Deep Pen ×2, Widow's Notch, the Crypt, the Roost Mine, Hollow Gap Mine, Smugglers' Cut, the Snake Tunnels ×2, the Dreaming ×2, the Bone Tower ×4, Hellspur ×2, and its summit |
| Regions | 4 + Hell [GUIDE] | 4 (the Sweetgrass, the Bottoms, the High Mesa with Hollow Gap's valley, the Burnt Hills) + the Scorched Reach |
| Kinds of foe | about 75 named in the bestiary, 31 with HP on the wiki [MH] [GUIDE] | 77, many as colour sets of one drawing, as a cartridge of the day would |
| Bosses | Big Alp, Lasher, Xafan (optional), Gobbler, Hyena Joe, Possum (optional), Shapeshifter, Bad Betty, the fort's siege, Thunderbird, Scoria Worm, Mescalita and her hands, the four Horsemen, Vilentcube (optional), the Dark Lord, Malus and Biggan [GUIDE] | 19 fights: the Puck King, the Scourge, Brutus (optional), the Tallyman, Jackal Jack, Slick Magpie (optional), the Mimic, Black Molly, the siege of Fort Patience, the Storm Crow, the Cinder Worm, the Saguaro Mother and her arms, Blight, Carnage, Hunger and the Reaper, the Juggernaut (optional), the Governor, Zenith and Nadir |
| Items | about 80 weapons, armour, medicine and ores [GUIDE] | 83 |
| Skills | about 37 actives and 11 passives [GUIDE] | 37 actives, 11 passives |
| Sage's animals | 10, each a choice of 2 [GUIDE] | 10 (our animals) |
| Train stops | 5 [GUIDE] | 5 |
| Goals | 3 [MH] [GGC] | the same 3 |
| Stats | Battles, Posse Deaths [MH] | the same two, on the title screen |

**How long it takes.** A first clear of Grimstone is about 15 hours; the
Gold/Cherry speedrun record is 55:59 [LIZ] [SR]. Our demo player, which
knows the way and wastes no time reading, reaches the ending in about
700,000 frames (3 hours 15 minutes of game time, 560 or so fights, with
the levels it grinds for each step: 6 before the first mine, 16 for the
siege, 28 for the Cinder Worm, around 45 by the last fight). A first-time
player who talks, explores and gets lost should take several times that.

## Mechanics checklist

| Mechanic | How HELLSPUR does it | Source | Test |
|---|---|---|---|
| The opening | the angel walks the burning saloon, reads each stranger's two lines and chooses whether to carry them out; four at most, at least one; the rest burn; the posse is permanent | [MH] [MM] [TVT-S] [LIZ] | spur_02 |
| Fewer than four | a posse of one, two or three is allowed (and levels faster) | [GUIDE] | spur_02, spur_14 |
| Walking | tile by tile; the posse follows in a line, the fallen too | [MH] [STATIC] | spur_04 |
| Wisps (map enemies) | appear around you on the map and in dungeons, step once for every step you take, toward the nearest of the posse; a touch on any of the posse, even a dead one at the back, starts a fight; no random battles | [MH] [MM] [STATIC] [LIZ] | spur_04 |
| Horses | hired at stables (100, 200, 500); the wisps step once for every two of yours; they go home when you reach a town; no horses on the train | [GUIDE] | spur_04, spur_11 |
| Wards | smudge (200) and Sage's BLEND IN thin the wisps out for a while | [GUIDE] | (bot, menus) |
| Battles | side-on, the posse on the left with health and soul; orders for each from the top (ATTACK, SKILL, ITEM, DEFEND, RUN), then everyone acts by speed, the posse winning ties in posse order | [MM] [GUIDE] | spur_06 |
| The timing bar | every plain attack: A on yellow hits (can be dodged), on red is a critical that can't be dodged, anywhere else or no press misses; guns that fire twice or thrice put that many marks on the bar | [MH] [MM] [GUIDE] | spur_05 |
| Shotguns, heavy guns, bows | a shotgun's bar is one big yellow, with splash on the others (more often on big foes); a heavy gun fires while A is held across its band, up to 8 splashing shots, and must reload next turn; a bow is all red in anyone's hands but Sage's | [GUIDE] | spur_05 |
| Hold RIGHT | the marker runs twice as fast | [GUIDE] | spur_05 |
| The hound | Biscuit can't be given orders: he swings on his own, often misses the bar, hits hard (a critical ignores armour), and leaps in front of the badly hurt | [MH] [GUIDE] [STEAM-PARTY] | spur_05 |
| Posse order | the first two are hit more often; ORDER in the menu swaps them | [GUIDE] [MM] | spur_06 |
| Status | poison (in fights and on the road), stiffness (paralysis), death outright from some foes; snakeroot, PURGE, RENEW, holy oil | [GUIDE] | spur_06, (play) |
| The fallen | stay dead after a fight; a chapel raises them for 20 bits a level; holy oil, MEND/SUTURE out of a fight, RAISE | [GUIDE] | spur_07 |
| Everyone falls | wake in the last chapel visited, all on their feet, carrying nothing; the bank keeps its money | [GUIDE] [MM] | spur_07 |
| Money | bits (ours for the original's teeth); early fights pay a little, late ones a lot | [GUIDE] | spur_06 |
| The bank | two in a hundred added after every fight won, rounded up, to 100,000 | [GUIDE] [TEETH] | spur_06, spur_11 |
| Hotels | heal the living and save; 10 in the first town, 1,000 in the demons' town, free at the missions, the fort and a few houses | [GUIDE] | spur_11 |
| Saving | at hotels, at chapels when you wake, and when you leave a town; no saving anywhere else; CONTINUE goes back there | [GUIDE] [LIZ] | spur_11, spur_14 |
| Levels | each stranger levels alone on XP split among those standing; 50 at most; skills and weapon skills come at set levels | [GUIDE] | spur_09 |
| Equipment | right hand, left hand (two of the same one-handed weapon with TWO GUNS), neck, body, feet; each weapon class needs its skill; Hollis has one arm; Cora wears only light clothes; the hound only a neck piece; the hellhide is Tucker's alone | [MM] [GUIDE] | spur_08 |
| The bags | twenty things, two pages of ten | [MM] | spur_08 |
| Doctoring | with the DOCTOR skill, a bandage leaves a half bandage, a med chest a half chest and then a salve, and so on; at DOCTOR 4 a med chest heals 999 and holy oil raises at half | [GUIDE] | spur_10 |
| Sage | ten animals, each offering a choice of two skills | [GUIDE] | spur_10 |
| Quill | OBSERVE learns a foe's trick (sure on a roped foe); nine at most | [GUIDE] | spur_10 |
| Vera | LIFT picks a foe's pocket (bits, or a common or rare item) | [GUIDE] | (bot) |
| Hattie | PAY OFF bribes a foe to leave (no spoils); works on Nadir | [GUIDE] | (play) |
| Lessons | STEADY (5,000) and PROSPECT (8,000) as books | [GUIDE] | (shops) |
| Mining and foraging | PROSPECT on glinting rock seams in mines, RUMMAGE on glinting plants; ores sell for 100 to 50,000 | [GUIDE] | (menus) |
| Trains | between five stations at 40 a stop (120 at most), each opened by the story | [GUIDE] | spur_11 |
| The nightmares | the Reaper's winged horses: MOUNT from the menu, fly over mountains and sea, land on open ground, no wisps in the air | [GUIDE] [TVT-S] | spur_11 |
| The story's gates | the ferry and its crank, the angels at the Notch, the rockfall and the plunger, the cellar door, the fire and its charm, the Black Gate and the iron spur, the three spikes and the citadel's veil | [GUIDE] (structure only) | spur_13 |
| Bosses | each sets the story going; the siege is won when its captain falls; the last fight ends when Zenith falls, and Nadir falls with him | [GUIDE] | spur_12, spur_14 |
| Each stranger's scenes | Quill's rival, Tucker's partner in the past, Hollis's soldier (who dies if you take too long), Vera and Black Molly (black brew after), Hattie's brother's letter, Cora's dance after Slick Magpie, Sage's grandmother, the hound and the coyote, and lines for Biscuit in the widow's house | [GUIDE] | spur_15 |
| Goals | gift: anyone reaches level 10; gold: end the game; cherry: every one the angel saved alive when Zenith falls (raising them in the fight is fine) | [MH] [GGC] [STEAM-CHERRY] | spur_09, spur_14 |
| The demo | left on the title, the cartridge plays itself with Tucker, Hollis, Cora and Biscuit | [GUIDE] | spur_17 |

### Readings we had to choose

- **Formulas.** A plain hit does the swing's power less half the foe's
  defence; a critical twice the power, less half the defence (the hound's
  ignores it). The swing's power is half the stranger's ATK plus the
  weapon's ATK raised by half for each level of its skill past the first.
  Skills that slip past armour use RES and ATK, raised by half a level of
  SPIRIT each. Nothing in the sources gives these.
- **The bar.** It crosses in 44 frames (a little faster with more marks);
  the red is 5 % of the bar (12 % with STEADY or the steady eye, more with
  each KEEN EYE); a shotgun's yellow is 58 %, a heavy gun's band 80 %, and
  heavy shots come every six frames at half power.
- **Who gets hit.** The four places are picked 40, 30, 18 and 12 in 100.
- **Running.** 50 in 100, plus three a point of speed the posse has over
  the foes, plus ten a try, from 15 to 90; never from a boss.
- **The XP curve.** To reach level L: 5(L−1)² + (L−1)⁴/16 (815 for level
  10, 372,305 for 50). Split among those standing at the end.
- **Waking after a wipe.** The Steam guide says every carried tooth is
  lost; the missing manuals say "a portion". We took the guide.
- **Paralysis** lasts till cured, a lucky turn (30 in 100), or the fight's
  end. **Poison** costs a sixteenth of health a round, and a point every
  four steps on the road (never the last point).
- **Wisps.** Three at most on the big map, four in a dungeon; they appear
  five to nine steps away, in sight; they gutter out if left 16 tiles
  behind or hemmed in for six steps.
- **The ward.** Smudge and BLEND IN cut the wisps to a quarter for 250 and
  300 steps.
- **The gift** counts anyone in the posse; **the cherry** counts only those
  the angel saved (a posse of two needs both).
- **Which boss stands where, what each chest holds, the town layouts and
  every word** are ours; the original's secret places (a cloud you walk
  onto, a hidden gunsmith, a rare boss on a peninsula) have counterparts in
  different places of our own (a cloud caught on the southern range, a
  smithy in a pit west of Gladwell, a bluff island in the south sea).
- **The Hollow Gap past** happens when you sleep in the ruined hotel; it
  ends when you sleep again or leave.
- **The soldier** at Fort Patience dies once the Storm Crow falls.

## What is ours

- **Name:** HELLSPUR (1988, Beamdown Softworks); the currency is bits.
- **The posse:** Tucker the cowhand, Sage the spiritwalker, Hollis the
  one-armed surgeon, Quill the scholar with a boxer's ears, Vera the
  outlaw, Hattie the homesteader with her handbag, Cora the showgirl and
  Biscuit the one-eyed hound; Wren, Ambrose and Cassius the angels; the
  widow Tallow; Ezra Pike; Brutus, the Tallyman, Jackal Jack, Slick Magpie,
  the Mimic, Black Molly and old Gil, Mae Flint, Dutch, the Storm Crow, the
  Cinder Worm, the Saguaro Mother, the riders Blight, Carnage, Hunger and
  the Reaper, the Juggernaut, the Governor, Zenith and Nadir.
- **The world:** the Territory of Calico and every place in it, all 77
  kinds of foe, all 83 items, all 37 skills by our names, the ten animals.
- **Music:** "Hellspur", "The Golden Garter", "Main Street", "Open
  Country", "Wisps!", "Something Big", "Down the Shaft", "The Dreaming",
  "The Pit", "The Governor's House", "Old Hundred and One", "Hired Horses",
  "Nightmares", "Ghost Town", "Buy Something", "The Last Hand", "Sun Comes
  Up Yellow" and three jingles.
- **Words:** the story, every person's lines, the endings and every label.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
UFO 40 goals, which are Grimstone's own (gift, gold, cherry). The
terminal's all-dogs code is a UFO 50 collection feature; UFO 40 has no
terminal, so it is left out. There is no 2-player mode in the original, so
nothing is greyed out.

**Owner's change (none needed):** the owner's rule puts the main action on
A (keyboard Z / Space, Vita Cross). Grimstone already has talking,
choosing and the timing press on button 2, which UFO 40 calls A, and the
menu and cancel on button 1 (B), so the layout is the original's.

## Controls

| Input | Action |
|---|---|
| D-pad | walk; move the cursor |
| A | talk, search, open, choose; in a fight, the timing press (hold it for a heavy gun) |
| B | the menu (items, skills, equipment, stats, order, key items, mount); back |
| Hold RIGHT | the timing bar runs twice as fast |
| START | pause |

## Not confirmed

| Point | What we chose |
|---|---|
| B as cancel in fights | B steps back to the previous order (the manuals: B cancels) |
| Hold RIGHT on the bar | the marker runs twice as fast (the guide: "speeds it up significantly") |
| What weapon skills do beyond allowing the weapon | each level adds half the weapon's ATK |
| Bar speeds and zone sizes | as under Readings |
| Flee chance, the lariat's effect | as under Readings (the lariat stuns till struck) |
| Base stats and growth | ours, shaped by the guide's "high HP", "low SP" notes |
| Whether a wipe takes all carried money | all (the guide) |
| Wisp counts, spawn range, chase rules | as under Readings |
| Gift with any member, cherry with a small posse | as under Readings |

## Tests

`tests/spur_01` … `spur_17`. They drive the rules with button presses, or
set up a moment with cheats and then play it: the title and the saloon
pick (spur_01, spur_02), the world's gates on foot (spur_03), walking and
the wisps (spur_04), the timing bar (spur_05), the order of a round and its
spoils (spur_06), deaths, chapels and wipes (spur_07), shops, the bags and
equipment (spur_08), levels and the gift (spur_09), each stranger's way of
learning (spur_10), the counters, trains and flying (spur_11), every boss
(spur_12), the story's gates (spur_13), the goals and the save (spur_14),
the strangers' own scenes (spur_15). The demo player (`spur_bot.c`) presses
buttons only: it walks the shortest way (through the passes when it must),
presses on the red, heals, rests, raises, shops, banks and grinds for each
step's level; spur_16 has it play from the title to the ending and back to
the title; spur_17 runs the title's demo.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Grimstone" (raw page): the infobox, the
  goals, the gameplay paragraph, the strangers, the enemy HP table.
  https://ufo50.miraheze.org/wiki/Grimstone
- [GUIDE] Steam guide "Grimstone Ultimate Guide - Maps, Skills, and More!"
  (Llamakazi): the strangers' levels and skills, every shop and price, the
  bestiary with XP and teeth, the bank, horses, trains, the story order, the
  items, the goals. https://steamcommunity.com/sharedfiles/filedetails/?id=3337445448
- [MM] Steam guide "The missing manuals - How to play UFO 50 games", §40:
  A and B, the menu's pages, the wisps (blue flames), the battle commands,
  the timing bar, waking at a church.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [GGC] Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [TEETH] Steam guide "Grimstone Teeth Farming Guide (and maxing out Rufus)".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3410171118
- [STEAM-CHERRY] Steam thread "Cherry requirement for Grimstone?".
  https://steamcommunity.com/app/1147860/discussions/0/4852155152090765444/
- [STEAM-PARTY] Steam thread "what party for Grimstone?".
  https://steamcommunity.com/app/1147860/discussions/0/4699034745346572817/
- [LIZ] https://lizstar64.github.io/reviews/2024/10/19/UFO50-40.html
- [STATIC] https://staticcanvas.substack.com/p/the-ufo-50-diaries-grimstone
- [TVT-S] TV Tropes recap (search-result excerpts only).
- [SR] speedrun.com (search result): Gold/Cherry 55:59.
