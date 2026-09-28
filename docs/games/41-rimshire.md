# 41 · RIMSHIRE

*Internal design document. Not shown in the product.*

## Tribute to

**Lords of Diskonia** (UFO 50 game #41, Mossmouth, "August 1988"). The rules
were researched from text only: the community wiki, two Steam guides (a
knowledge base of every rule and number, and "The missing manuals"), a Steam
strategy thread, written reviews and search summaries (listed under
Sources; the full notes are in the research folder,
`41-lords-of-diskonia.md` and `controls-41-44-46.md`). No UFO 50 images,
video, sprites, music, maps or text were used as references, and nothing
was taken from a UFO 50 install.

**Map type: fixed, and random for the streak.** The campaign's ten wars are
"seeded" and always the same [MH], [WIKI-EN], so ours are too
(`rimshire_levels.c`, ten boards of our own); the streak's wars are random
[MH], [KB], so ours come from our own generator. The battlefields are built
at random from the four board tiles around the spot where a battle starts,
as the original's are [KB], [MM].

| Full scale | Lords of Diskonia | RIMSHIRE |
|---|---|---|
| Layers | a board of roads, then flick battles [MH] | the same |
| Campaign | 10 scenarios, each adding one idea or unit [MH] | 10 wars, the same ideas in the same order |
| Streak | random wars, win as many in a row as you can [MH], [KB] | the same |
| Two players | yes [MH] | yes, pad 1 and pad 2 |
| Disks | 15 for hire and the Blue Queen [MH], [KB] | 15 for hire and the Plum Empress |
| Skills | 8, from old scrolls [MH], [KB] | 8, from tomes |
| Board places | taverns, old scrolls, gold mines, gold caches [MH], [KB] | inns, tomes, seams, chests |
| Terrain | forest, mountain, desert, water [KB] | woods, crags, dunes, lakes |
| Field things | gold nuggets, blue and pink crystals, health potions, holy fountains, gold boulders, crystal formations, props [KB] | coins, blue and pink shards, tonics, springs, coin rocks, shard rocks, trees |
| Army | 8 in the field, 6 in reserve [KB] | the same |
| Goals | 3 [MH] | the same 3 |

The ten boards (all ours) run from 13 × 3 to 16 × 7 nodes: 512 nodes in all,
with 22 inns, 15 tomes, 16 seams and 20 chests.

| War | Ours | Its idea (the original's scenario) | Board |
|---|---|---|---|
| 1 | The Long Lane | one straight road; squire, warden, ferret, brute (Narrow Pass) | 13 × 3, 13 nodes |
| 2 | Two Roads | tomes; the slinger (East Valley) | 13 × 5, 43 nodes |
| 3 | Marshmoot | water; the toadkin (The Island) | 14 × 6, 40 nodes round a mere |
| 4 | Coin Fever | seams; ooze, hexer, menhir (Gold Rush) | 14 × 5, 46 nodes, 6 seams |
| 5 | Adder Nest | Brass marches with a warden and two adders (War Spiders) | 14 × 6, 45 nodes |
| 6 | The Keep | homes become castles; the friar (Promotion) | 14 × 6, 64 nodes |
| 7 | Pipers' March | a warden and two pipers; the piper and the leech (Tuned Up) | 15 × 6, 60 nodes |
| 8 | Deep Diggings | three delvers, no reserves, no coin on the board; delver and wyrm (Miner Issue) | 15 × 6, 50 nodes |
| 9 | Three Bridges | two shores and three crossings (Land Bridges) | 15 × 7, 71 nodes |
| 10 | The Empress | the Plum Empress in the enemy's reserve (The Regicide) | 16 × 7, 80 nodes |

## The three goals

| UFO 40 goal | Lords of Diskonia's own | RIMSHIRE |
|---|---|---|
| Beacon | gift: beat 5 levels [MH] | win five wars of the campaign |
| Saucer | gold: beat 10 levels [MH] | win all ten, then the ending |
| Alien | cherry: win in Streak mode 3 times in a row [MH], [SEARCH-DK] | win three streak wars in a row |

## Mechanics checklist

| Mechanic | How RIMSHIRE does it | Source | Test |
|---|---|---|---|
| Turns on the board | Brass (the player) always moves first; one move on the war's first turn, then two a turn (three with SCOUTING); two players toss for who starts | [KB], [MH] | rsh_02, rsh_05 |
| Moving | the d-pad points at a road, A ("confirm") takes the banner one space along it; every move must be used | [KB], [OWNER] | rsh_02 |
| The trail | a banner leaves a trail and can't step onto it again | [KB], [MH], [LIZ] | rsh_02 |
| Going home | holding B takes the banner home at any time in its turn (even between moves), wipes its trail and ends the turn; not from home | [MM], [KB], [OWNER] | rsh_03 |
| Nowhere to go | the only way out is home | [KB] | rsh_02 |
| Battles on the board | stepping onto the other banner, its home or its trail; a banner caught on its trail is pulled to that spot; the side that stepped acts first | [KB], [MM], [MH] | rsh_21 |
| After a battle | both trails wiped; the winner stays, the loser goes home, all its reserves join its field army, and it moves next with a full turn | [KB], [SEARCH-CASTLE] | rsh_19 |
| A second stand | beaten at its own home, a side fights again at once with its reserves | [SEARCH-CASTLE] | rsh_19 |
| Castles | beaten at home, a castle's side loses the war | [SEARCH-CASTLE], [MH] | rsh_19 |
| Winning a war | a battle would start and one side has no disks at all | [KB], [MH], [MM] | rsh_20 |
| Stalemate | five full rounds with nothing touched: both go home, no reserves, the side that didn't start moves next | [KB] | rsh_25 |
| Inns | the same three disks all war, bought as often as you can pay; three visits (by either side) and the inn closes, with the three messages | [KB], [MH] | rsh_04 |
| A full field army | a bought disk still joins; you pick which field disk goes to the reserve; nothing more once both are full | [KB] | rsh_04 |
| Tomes | pick one of three skills you don't have; you must pick one | [KB], [MH] | rsh_05 |
| Seams | stepping on one claims it; it pays 1 a turn (2 with PROSPECTING) from its holder's next turn, ten times, then it is gone; taking it over keeps the count | [KB], [MH] | rsh_05, rsh_05b |
| Chests | 4 or 5 coins, once | [KB], [MH] | rsh_05 |
| Starting armies | two squires in the field, a warden and two squires in reserve (streak and two players; the campaign varies by war) | [KB], [MH] | rsh_19 |
| The field | built from the four board tiles round the spot: each tile makes one corner | [KB], [ADV], [MM] | rsh_26 |
| Woods | grass with trees and huts to bounce off; sometimes a little sand or a pond | [KB] | rsh_26 |
| Crags | cliffs close the corner in along both edges, cut at an angle in the corner; sometimes sand | [KB] | rsh_26 |
| Dunes, lakes | sand, or water, round the corner's edges and a bunker or pool nearer the middle | [KB], [MM] | rsh_26 |
| Setting up | each army's disks are shuffled into a queue and dropped at random in its corner: Brass top left, Plum bottom right | [KB] | (drawn) |
| The queue | pick one of the first three disks (five with COMMAND); after its turn it goes to the back | [KB], [MH] | rsh_18 |
| Aiming | the aim turns the short way round toward the held direction and stops there; it can still turn while charging | [KB] | rsh_06 |
| Charging | hold A: a pip every 12 frames up to the disk's charge; let go to launch; under one pip nothing happens; power can't go down, but held three seconds with nothing else pressed the shot resets | [KB], [MH], [OWNER] | rsh_06, rsh_07 |
| Moves | each disk moves 1, 2 or 3 times (M), then may fire its projectile | [KB], [MH] | rsh_06, rsh_11 |
| Camera | hold B and the d-pad moves the view, any time in your turn, even while charging | [MM], [KB], [OWNER] | rsh_17 |
| Damage | your disk striking a foe does its melee; a foe knocked into one of your disks takes that disk's melee; two foes knocked together take 1 each; a disk hurting a foe again and again hurts it every time; your disks never hurt each other | [KB], [MM], [LIZ], [MH] | rsh_08, rsh_08b |
| Knockback | by size: small, mid and large disks weigh 1, 1.6 and 3 | [KB], [MH] | rsh_09 |
| Water | a disk that can't swim dies as soon as its middle is over water | [KB], [MH], [LIZ] | rsh_09 |
| Sand | slows disks almost to a stop (HOBNAILS ignores it) | [KB], [MH] | (physics) |
| The haze | after both sides have had five turns, "DON'T END YOUR TURN IN THE HAZE!" and it closes in from the edges each round; only the chosen disk ending its turn touching it dies | [KB], [MM], [MH] | rsh_10 |
| Projectiles | a small disk fired after the moves, 1 shard each; it needs room to come out; a tap skips it for nothing; with no shards there is none | [KB], [MH] | rsh_11 |
| Shards | 2 at the start of a battle (5 with STOCKPILE) | [KB], [MH] | rsh_11 |
| Pickups | coins 2, blue shards 1, pink shards 2, tonics +2 HP; the first disk over them takes them, on anyone's turn; never a projectile | [KB] | (physics) |
| Props | springs +1 HP a knock (5 knocks), coin rocks 1 coin (5), shard rocks 1 shard (3); projectiles can mine the rocks, not the springs | [KB] | (physics) |
| Delver | triples every coin and shard it collects | [KB], [MH] | (physics) |
| Stun | the brute's hit and the toadkin's spit; next turn only one move, and a weak one; moving or healing clears it (healed mid-turn, the disk gets its turn back) | [KB], [MH] | rsh_12 |
| Poison | the adder; a poisoned disk dies to any damage; the poison lands before the damage, so a starred adder kills at once; healing cures it | [KB], [ADV], [MH] | rsh_13, rsh_13b |
| Leech | drinks 1 from every foe it touches; tonics and balm hurt it 4, springs 3 | [KB], [ADV], [MH] | rsh_14 |
| Friar's balm | heals any disk it hits 2, friend or foe | [KB] | rsh_14 |
| Piper's tune | +1 melee and ranged to any disk it hits, friend or foe, for the rest of the battle, five at most | [KB] | rsh_15 |
| Hexer's spell | 1 on a direct hit; where it stops, a ring of six embers that burn the first disk to touch them, friend or foe | [KB], [MH] | rsh_15b |
| Menhir | anchored: nothing knocks it about | [KB], [MH], [ADV] | rsh_16 |
| Heal caps | small disks 7 HP, mid 10, large 12; the Empress starts on 12 | [KB] | (physics) |
| Every disk | the knowledge base's moves, melee, ranged, HP, size, charge (and projectile charge) and cost | [KB], [MH] | rsh_01 |
| The skills | COMMAND, STOCKPILE, SCOUTING, HAGGLING, PROSPECTING, SIGHTLINE, HOBNAILS, REMEDY: the original's eight effects | [KB], [MH] | rsh_05, rsh_18 |
| SIGHTLINE | a short line to the first thing the shot meets, then where that thing goes and where the shot bounces | [KB] | (drawn) |
| The computer | sloppy early (one of its better shots, a shaky hand), perfect aim late and in the streak; it loves trick shots that knock many disks and pick up much, doesn't weigh which disks it hurts, and never quite sees its own disks drowning | [LIZ], [KB], [SEARCH-DK2] | rsh_w01-w10 |
| The Empress | in the Plum reserve in war 10; beating her doesn't end the war | [MH], [KB] | rsh_w10 |
| Letters | Lady Brass writes before every war (the Red Queen's letters by pigeon) | [KB] | (drawn) |
| Goals | Beacon: win five wars; Saucer: all ten; Alien: three streak wars in a row | [MH] | rsh_22, rsh_w05, rsh_w10, rsh_23 |

### Readings we had to choose

- **Physics.** The field is 384 × 256 (bigger than the screen, hence the
  camera). A disk leaves at 1.3 + 0.95 × pips pixels a frame, a projectile
  at 1.4 + 0.8 × pips; grass takes 0.06 a frame off, sand 0.22, water
  (swimmers) 0.08, projectiles 0.045. Walls give back 84 %, disks 90 %.
  Radii 6, 8 and 11. A pip every 12 frames; the aim turns 2/256 of a circle a
  frame.
- **Charge values.** The knowledge base's charge for every disk; the stun's
  "drastically reduced charge" is two pips at most.
- **The haze** closes 14 pixels a round (two thirds of that top and bottom)
  to 96, leaving a middle of 192 × 128.
- **Stalemate.** "Five full turns" is read as five rounds (ten turns).
- **Retreating** needs B held a third of a second: the manual says hold,
  the knowledge base says press.
- **Healing a poisoned disk** only cures it, as the knowledge base's
  "returning the health that the disk had" suggests.
- **Inns** sell each of their three disks as often as you can pay.
- **Castles.** Wars 6 to 10 give both sides castles; the streak and two
  players have plain homes.
- **Starting coins, armies and pools** for each war are ours, following each
  scenario's idea; war 8 puts 4 extra coins on every field (no coin on the
  board).
- **The computer's sharpness** runs 1 to 9 over the campaign and 10 in the
  streak; the streak's Plum banner brings one more random disk and 2 more
  coins for every war already won.
- **The streak** opens once the campaign is won; leaving a streak war ends
  the run.
- **Field things:** 2 to 4 coins, 2 or 3 shards (a third of them pink), up
  to 2 tonics, and a spring, a coin rock and a shard rock more often than
  not; one to three trees in each woods corner.
- **Crags:** cliffs 22 to 34 pixels in, the corner cut 18 to 30 pixels.
- **No mid-war save**: a war lasts minutes; progress (wars won, the streak,
  the best streak) is saved between wars.

## What is ours

- **Name:** RIMSHIRE (1988, Beamdown Softworks), a round little land where
  everyone is round too.
- **The sides:** the Brass Banner (the player, led by Lord Brass, with Lady
  Brass's letters) and the Plum Banner and its Empress.
- **The disks:** squire, warden, ferret, brute, slinger, toadkin, ooze,
  hexer, menhir, adder, friar, piper, leech, delver, wyrm and the Plum
  Empress; their emblems and blurbs.
- **The skills:** COMMAND, STOCKPILE, SCOUTING, HAGGLING, PROSPECTING,
  SIGHTLINE, HOBNAILS and REMEDY.
- **The board:** inns, tomes, seams and chests; woods, crags, dunes and
  lakes; the ten wars' names and boards, the streak's generator.
- **Music:** "Rimshire" (title), "The Roads of Rimshire" (board), "Flick and
  Fling" and "Bank Shot" (battles), "The Plum Empress", "Brass at Peace"
  (ending) and four jingles.
- **Words:** the story, the letters, the war cards, the ending and every
  label.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu (with RESTART WAR
and LEAVE WAR under RESUME) and the three UFO 40 goals, which are Lords of
Diskonia's own (gift, gold, cherry). The terminal codes BANK-SHOT and
BUMP-MORE are UFO 50 collection features; UFO 40 has no terminal, so they
are left out.

## Controls

Button 1 of the original is our A, button 2 our B (the owner's check of the
original's prompts: battle, "launch disk" and "camera mode"; board,
"confirm" and "return home").

| Input | Action |
|---|---|
| D-pad, A (board) | point at a road, go one space |
| Hold B (board) | retreat home; the turn ends |
| A / B in the inn | hire / leave; UP and DOWN choose |
| A in a tome | learn; UP and DOWN choose |
| LEFT / RIGHT, A (battle) | choose a disk from the queue |
| D-pad (battle) | turn the aim |
| Hold A, let go | charge and launch (a tap skips a projectile) |
| Hold B + d-pad | look round the field |
| START | pause |

### Not confirmed

| Control | Our reading | Why |
|---|---|---|
| Choosing a disk from the queue | LEFT / RIGHT, then A | no source names the input |
| A step on the board | the d-pad points at a road, A goes one space | the owner's prompt shows A as "confirm" on the board, and the knowledge base says moves are made one by one with a retreat possible between them; no source spells out the input |
| Retreat | B held a third of a second | the manual says hold B, the knowledge base says press |
| Inn, tome and reserve menus | UP / DOWN and A; B leaves the inn or cancels the reserve choice | no source names these |
| Camera | B held with the d-pad; letting go follows the action | the manual says hold B and move the d-pad; the owner's prompt says "camera mode" |

## Tests

`tests/rsh_01` … `rsh_26` drive the rules with button presses, or set up a
moment with cheats (armies, positions, a bare field) and then play it with
presses. The demo player (`rsh_battle_buttons` and `map_bot` in
`rimshire_battle.c` and `rimshire.c`) uses the computer's own planners with
a steady hand and a sense of what each disk is worth: on the board it walks
to the most worthwhile place it can reach and fights only when it is at
least as strong; in battle it tries every disk it may choose at 32 angles
and three powers on a copy of the field, then refines the best. With real
presses it wins all ten wars (`rsh_w01` from the title, `rsh_w02` …
`rsh_w10`; `rsh_w05` earns the Beacon, `rsh_w10` the Saucer and the ending)
and three streak wars in a row for the Alien (`rsh_23`).

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Lords of Diskonia" (raw page): the two
  phases, moves, the trail, battles, taverns, scrolls, mines, caches, the
  eight skills, the queue, melee and ranged, stun, poison, terrain, the fog,
  every unit, the ten scenarios, the goals, the cheats.
  https://ufo50.miraheze.org/wiki/Lords_of_Diskonia
- [KB] Steam guide "Lords of Diskonia Knowledge Base and Tips" by Redo: the
  goal, turns, going home, the trail, being pulled to a trail, the four
  terrains, caches, mines (ten turns), taverns (fixed offers, three visits,
  the reserve swap), scrolls, placement, the shuffled queue, the fog (after
  five turns each), stalemates, aiming, charging, projectiles, damage by
  contact, the end of a battle, every pickup and prop, every disk's stats,
  heal caps, buffs, the skills.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3365414445
- [MM] Steam guide "The missing manuals - How to play UFO 50 games", section
  41: hold B to retreat, trails pull an army, combat when an army has no
  units, hold B and the d-pad for the camera, choosing from three, hold A to
  charge, damage on contact, battlefield resources, water on the edges, the
  fog, reinforcements after the first defeat.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [ADV] Steam thread "Lords of Diskonia: Advanced Tips / Strategy": the first
  turn's tempo, vampires hurt by healing, poison before damage, golems and
  walls, the four corners of a battle point.
  https://steamcommunity.com/app/1147860/discussions/0/6757179594731222009/
- [LIZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 41": ten levels of
  paths, flicking, water traps, bouncing foes into each other, the AI.
  https://lizstar64.github.io/reviews/2024/10/19/UFO50-41.html
- [STATIC] The UFO 50 Diaries, Lords of Diskonia: the feel, the toxic fog,
  no upgrades. https://staticcanvas.substack.com/p/the-ufo-50-diaries-lords-of-diskonia
- [WIKI-EN] Wikipedia, "UFO 50": ten seeded wars. https://en.wikipedia.org/wiki/UFO_50
- [SEARCH-CASTLE] search-engine summary of the knowledge base and guides:
  reserves after a defeat, the second stand at a breached home, a castle
  falls with its owner.
- [SEARCH-DK], [SEARCH-DK2] search-engine summaries: three streak wins in
  a row for the cherry; the streak is random; the AI gets nasty with bounce
  combos.
- [OWNER] the owner's check of the original's on-screen prompts
  (`controls-41-44-46.md`).
