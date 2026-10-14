# 12 · FOSSIL FEUD

*Internal design document. Not shown in the product.*

## Tribute to

**Avianos** (UFO 50 game #12, Mossmouth, "February 1985"). The rules were
researched from text only: the community wiki's raw pages (Avianos,
Cheats, Meta Messages, Debug mode, Terminal), the Steam guides "The
missing manuals" (section 12), "Avianos Ancestors, Miracles, Units,
General Info" and "Gift, Gold & Cherry", Steam threads, written reviews
and TV Tropes (listed under Sources; the full notes are in the research
folder, `12-avianos.md`). No UFO 50 images, video, sprites, music, maps or
text were used as references, and nothing was taken from a UFO 50
install.

**Map type: random.** Avianos deals a fresh board for every match; only
the size and the counts are fixed [MH], [STEAM-EH]. Ours is random too
(`feud_rules.c`), mirrored through the middle so both sides start equal.
Nothing is copied: only the structure follows the original.

| Full scale | Avianos | FOSSIL FEUD |
|---|---|---|
| Players | 1 vs AI, or 2 versus [MH] | 1 vs the Slate Flock; **2 versus greyed out for the next update** |
| Board | 5 × 4 tiles, starts on opposite sides [MH] | 5 × 4, homes on opposite sides, mirrored |
| Castles / flags | 6 castles, each with a flag; custom 2-6; win on 4 flags [MH] | 6 eyries, each with a totem; custom 2-6; win on 4 totems |
| Mountains | 4, custom 0-8 [MH] | 4 peaks, custom 0-8 |
| Ancestors | 5, 3 offered a turn, 3 fixed actions each, 2 blessings an action [MH], [MANUALS] | the same: Grandjaw, Plateback, Longneck, Skycrest, Threehorn |
| Units | 5 [MH] | 5: stomper, lancer, slinger, mystic, swift, with the wiki's numbers |
| Buildings | castle + 4 [MH], [GUIDE] | eyrie + granary, roost, shrine, dustbath |
| Terrain | 7 [MH], [GUIDE] | 7: meadow, thicket, boneyard, scree, gorge, peak, eyrie |
| Resources | seeds (99), workers (9), bones (9) [MH] | grain (99), toilers (9), relics (9) |
| Miracles | 7 (one only with the fog off) [MH], [GUIDE] | 7 omens: tithe, shift peak, hailstorm, far sight, updraft, thunderbolt, hatching |
| Campaign | Hatchling, Fledgling, Adult, then 5 Trials [MH] | First Moult, Second Moult, Full Plume, then Ordeals I-V |
| Custom | castles, mountains, fog [MH], [GUIDE] | eyries, peaks, fog, and the opponent's skill |
| Goals | 3 [MH], [GGC] | the same 3 |
| Stats | picks per Ancestor [MH] | the same, with matches, wins, losses and the fastest win |

## Mechanics checklist

| Mechanic | How FOSSIL FEUD does it | Source | Test |
|---|---|---|---|
| The board | 5 × 4 tiles; your home eyrie on the left edge, theirs mirrored on the right; each with 2 stompers, a granary and a roost | [MH], [MANUALS] | feud_02 |
| Starting stock | 10 grain, 3 toilers, 3 relics each | [GUIDE] (derived) | feud_02 |
| Tile drops | open ground gives its first claimer 12 grain, 3 toilers or 4 relics; a peak two of those; an eyrie nothing | [MH], [GUIDE] | feud_02, feud_08 |
| Caps | grain 99, toilers and relics 9; past that is lost | [MH] | feud_09 |
| Praying | three of the five on offer; the one prayed to rests for the next two turns (theirs and your next), so the pool cycles and a pick denies it; the first offers at random | [MH], [GUIDE] | feud_03 |
| Blessings | every second prayer to an Ancestor: pick one of its actions to raise a step, two steps each; the screen previews the blessed action; maxing one Ancestor takes 12 prayers; blessings start afresh each match | [MANUALS], [MH], [GUIDE], [STEAM-GIFT] | feud_04 |
| Actions in order | the three actions run top to bottom; GATHER and CALL UP need a confirm, the rest a tile and a choice | [MANUALS] | feud_03 |
| Grandjaw | GATHER +10 grain, +3 a granary / +1 toiler a dustbath / +18; ENLIST stomper 4, slinger 6 / 3, 4, stomper 1 toiler / 2, 3, lancer 1 toiler; MARCH plain / diagonal / + surprise | [MH], [GUIDE] | feud_05, feud_06, feud_08 |
| Plateback | MARCH plain / surprise / + through peaks; BUILD (toilers) granary 2, roost 3, shrine 3 / 1, 2, 2 / 1, 1, 1; ENLIST lancer 8, swift 10 / 5, 7, mystic 2 relics / 4, 5, 1 | [MH], [GUIDE] | feud_07 |
| Longneck | GATHER +4 grain a granary, +1 relic a shrine, +1 toiler a dustbath / 5, 1, 2 / 6, 2, 2; BUILD dustbath 4 grain, granary 2, shrine 3 toilers / 3, 1, 2 / 2, 1, 1; OMEN tithe 3, shift peak 3, hailstorm 6 (7 with the fog off) / 2, 2, 3 (5) / 1, 1, 2 (3) | [MH], [GUIDE] | feud_07, feud_10 |
| Skycrest | CALL UP 2 stompers a roost / + a lancer a roost and a mystic a shrine / + a swift a dustbath; MARCH plain / through peaks / + diagonal; BARTER 1 relic for 4 grain, 4 grain for a toiler / 6, 3 / 9, 2 | [MH], [GUIDE] | feud_05, feud_08, feud_09 |
| Threehorn | GATHER +3 relics, +2 a shrine / + 8 grain / + 16 grain; ENLIST mystic 8 / 5, swift 8, slinger 1 toiler / 4, 6, mystic 1 toiler; OMEN fog on: far sight 3, updraft 6, thunderbolt 9 / 2, 4, 7 / 1, 3, 4; fog off: hatching 3, updraft 6 / 2, 5, thunderbolt 9 / 1, 4, 6 | [MH], [GUIDE] | feud_10 |
| Enlist | at your eyries and roosts only, as many as you can pay for | [MH], [GUIDE] | feud_06 |
| Build | on any tile you hold, three buildings a tile; the roost only through Plateback, the dustbath only through Longneck | [MH], [GUIDE] | feud_07 |
| March | from a tile, choose how many of each kind go, then a tile next door; mountains bar the way unless blessed; diagonals when blessed; onto their birds is a battle, onto open or empty enemy ground claims it, buildings and all | [MANUALS], [MH] | feud_08 |
| Surprise attack | the defenders don't stir until the attackers strike; if you never strike you can look and leave | [GUIDE] | feud_11 |
| The omens | tithe: +1 favour with any Ancestor; shift peak: a peak moves (only the ground, never birds, buildings or totems), scree is left; hailstorm: their birds on a tile freeze and you are told how many even in the fog, never an empty tile; far sight: see into their tile; updraft: birds fly up to two tiles, onto eyries or peaks, again and again while relics last; thunderbolt: up to 2 of each kind die (10 at most); hatching (fog off): a bird at random, the costly ones likelier as the war drags on | [MH], [GUIDE] | feud_10 |
| A peak on an eyrie | the walls are gone for good; still a totem | [GUIDE] | feud_12 |
| Battles | real time, by themselves; the Rust Roost always on the left; the ground lays out the field: eyrie walls with one gate and battlements, a gorge's two bridges, thicket and boneyard cover, scree's rocks and rises, a peak's slope | [MANUALS], [MH] | feud_11, feud_12 |
| Orders | CHARGE, STAND (still shoots what's in reach), WRECK (pull the buildings down, your own too), FLEE (attackers go home; defenders lose the tile and go to a tile of theirs next door); changed at any time with LEFT / RIGHT | [MANUALS], [GUIDE] | feud_11 |
| Forty a side | at most 40 a side on the field, the rest wait and come on as others fall | [MH], [GUIDE] | feud_11 |
| Nothing happening | nobody hurt and nobody moving for 3 s: the attackers go home (20 slingers against one shielded lancer standing still) | [GUIDE] | feud_11 |
| The birds | stomper 6 HP, 1-3 (1 against lancers and mystics), can't reach swifts; lancer 5 HP + 5 shield that stops slings until it is in a melee, 2-5 (2 against mystics), can't reach swifts; slinger 4 HP, 0-3, reach 32-160, backs off when too close; mystic 3 HP, 3-5 (2-3 against mystics, 3 against swifts), reach 88; swift 4 HP, flies, 4-6 (5-7 against stompers, 2 against mystics, 1-2 against swifts) in a splash; speeds 0.4 / 0.4 / 0.2 / 0.2 / 0.6 px a frame, strikes every 80 / 80 / 100 / 110 / 110 frames | [MH], [TVT], [GUIDE] | feud_11 |
| Battle habits | slingers go for lancers first; shooters leave swifts till last; swifts fly past the front to hit what is behind | [TVT], [GUIDE] | feud_11 (play) |
| Defence | favoured, eyries most: up high, shots do half and blows from below one less; in cover half the shots miss; the Slate Flock holds its walls, chasm and slopes | [GUIDE] | feud_12 |
| Fog of war | their tiles show the ground and buildings, not the birds ("?"), nor their stock; off in a custom match | [MANUALS], [GUIDE] | feud_01, feud_10 |
| Winning | hold four totems when your turn ends and still when your next starts; a side with no tiles left loses | [MH], [MANUALS] | feud_13 |
| The campaign | three moults against sharper planners, the ending after the full plume, then five ordeals, each opened by the one before | [MH] | feud_01, feud_13, feud_20 |
| The ordeals | I extra grain; II a second eyrie; III a blessing; IV the Ancestors' actions dealt out afresh (only here is a MARCH's blessing previewed); V all of those, and two totems on open ground | [MH] | feud_16 |
| Custom match | eyries 2-6, peaks 0-8, fog on or off, and which planner | [MH], [GUIDE] | feud_01 |
| The secret | LEFT RIGHT RIGHT LEFT LEFT LEFT while choosing whom to pray to: a line of our own | [MH-META] | feud_17 |
| Goals | Beacon: max all three actions of one Ancestor in a match; Saucer: win the three moults; Alien: pass all five ordeals | [MH], [STEAM-GIFT], [GGC] | feud_04, feud_14 |
| Saving | the scenarios won, the gift, prayers to each Ancestor, matches, wins, losses, the fastest win and the custom match; a match in progress is not kept | [MH], [STEAM-SAVES] | feud_14 |
| Playable start to end | the demo player, with real button presses from the title, wins all eight scenarios | | feud_19, feud_20 |

### Readings we had to choose

- **Winning:** "at least 4 flags for more than 2 turns" (wiki) and "for a
  full round" (manual): you win at the start of your turn if you held four
  totems when your last turn ended and still do.
- **Totems:** there are always six. With fewer eyries (custom, or ordeal
  V's four) the rest stand on open ground, in mirrored pairs.
- **Starting stock:** 10 / 3 / 3 for both sides (derived from guide
  arithmetic); ordeal I's extra is 20 grain; ordeal II's second eyrie is
  the free one nearest their home; ordeal III blesses one action of one
  Ancestor at random; ordeal IV deals the fifteen actions out at random,
  the same deal for both sides.
- **Turn order:** you move first.
- **A MARCH** moves as many groups as you like, but each bird only once;
  birds that win a battle stay on the tile they took.
- **Peaks** can't be entered at all without "through peaks" (or an
  updraft), and buildings may go on any tile you hold.
- **The hailstorm** lasts through their turn and to the end of your next,
  so a march can take the tile while they stand; frozen birds can't march,
  move, strike or run in a battle.
- **Far sight** lasts until your turn ends. **The updraft** reaches every
  tile within two steps in any direction; flying onto their birds is a
  battle without surprise, and attackers who flee go back where they took
  off. **Hatching** weighs stomper 14, slinger 7, and lancer, mystic and
  swift 2 each plus one every three turns (up to 14).
- **Battle numbers:** a 300 × 110 px field; reach is the upper end of the
  wiki's range (the lower end is how close a slinger will still shoot);
  the shield drops for 1.5 s after a melee; a swift's splash is 12 px; the
  stall ends after 3 s with nobody hurt and nobody moving, and every
  battle after two minutes; high ground adds 24 px of reach; cover takes
  60 % of shots that pass through it and half of those aimed at a bird in
  it.
- **The Slate Flock in battle** flees as attacker below 45 % of the
  defenders' strength and as defender below 25 %; as defender it stands on
  eyries, gorges, peaks and scree.
- **The Slate Flock sees through the fog** (no source says either way).
  First Moult prays at random most of the time, buys the cheapest birds,
  leaves its eyries bare and uses no omen but the tithe; Second Moult
  weighs its prayers with a lot of noise; Full Plume weighs everything
  but still keeps a little too few at home (the "leaves key castles thin"
  that players report).
- **Defenders who flee** go to a tile of theirs next door (orthogonally),
  at random; with none, they are lost.
- **Button 1 (B)** is back, and finishes an action on the board.
- **A match in progress** is not saved; START offers LEAVE THE MATCH.

## What is ours

- **Name:** FOSSIL FEUD (1985, Beamdown Softworks).
- **Peoples:** the Rust Roost (you, red) and the Slate Flock (blue).
- **The Ancestors:** Grandjaw the Old King, Plateback the War Beast,
  Longneck the Tall One, Skycrest the First Flier, Threehorn the Bone Sage.
- **Birds:** stompers, lancers (herons with shields), slingers (gulls),
  mystics (owls) and swifts.
- **Everything else:** grain, toilers and relics; granaries, roosts,
  shrines and dustbaths; meadows, thickets, boneyards, scree, gorges,
  peaks and eyries; the omens' names; the moults and the ordeals' names;
  the story, the ending, the secret line and every label.
- **Music:** "Old Bones" (title), "Under the Ground" (story), "The Totem
  Fields" (your turn), "Slate on the Wind" (their turn), "Talon and Beak"
  (battle), "The Ordeals", "What the Bones Remember" (ending), "Totems
  Held" and "Feathers in the Dust".
- **Art:** every sprite, the boards and battlefields.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
goals, which are Avianos's own (gift, gold, cherry). The custom match's
choice of planner is how a one-player custom game picks its opponent. The
Terminal code that opens every trial (BLUE-BEAK) is a UFO 50 collection
feature; UFO 40 has no terminal, so it is left out.

## Owner's changes

- **The main action is on A** (Z / Space, Vita Cross). Avianos already
  confirms with Button 2, which UFO 40 calls A, so nothing moved; B is
  back. The card's CONTROLS page can still swap them.
- **2 PLAYERS is shown greyed out on the title on every platform**, with
  the line "2 PLAYERS · NEXT UPDATE" when it is highlighted; A does
  nothing there.

## Controls

| Input | Action |
|---|---|
| D-pad | move the cursor: Ancestors, actions, tiles, rows |
| LEFT / RIGHT on a bird row | how many of that kind march |
| A | pray, choose, confirm, buy |
| B | back; on the board, finish the action |
| LEFT / RIGHT in a battle | the army's orders: CHARGE, STAND, WRECK, FLEE |
| START | pause (LEAVE THE MATCH) |

## Not confirmed

| Control or rule | Our reading |
|---|---|
| Button 1 (B) | back, and finish an action (sources only name the confirm button) |
| How many groups one MARCH moves | any number, each bird once |
| Where ENLIST puts birds | your eyries and roosts |
| Buildings a tile | three |
| The flag-hold timing | held at your turn's end and at your next turn's start |
| Starting stock | 10 / 3 / 3 |
| The hailstorm's length, hatching's odds | as in the readings |
| Whether the CPU sees through the fog | it does |
| Whether a match in progress is saved | it isn't |

## Next update (Phase 2): 2-player versus

Shown greyed out now; not built. What it will need, from the research:

- A second side driven by a human: both share the screen and take turns
  on the one board; in battle each player switches its own army's orders,
  player 2 on the second pad (the original's debug menu can put both on
  pad 1, so a "P1 controls both" option for one-pad machines like the
  Vita).
- **No fog by default** (Custom can turn it on for hidden birds between
  two people).
- **Threehorn's omens with the fog off:** HATCHING in place of FAR SIGHT,
  THUNDERBOLT only from the first blessing (already in the rules table,
  used today by custom matches with the fog off).
- **The hailstorm costs one relic more** with the fog off: 7 / 5 / 3
  (also already in the table).
- A versus set-up screen (eyries, peaks, fog) and a result screen naming
  the winner, a "pass the pad" banner between turns, and the shell's
  versus input split (`input_set_versus`).

## Tests

`tests/feud_01` … `feud_18` drive each rule with button presses, setting
up a moment with cheats and then pressing through it. The demo player
(`feud_bot.c`) thinks with the Slate Flock's planner at its best (and
peeks through the fog, which a person can't), growing bolder the longer a
match runs, and works the menus with real presses, letting go of every
button the next frame. `feud_19` wins a custom match against the full
plume from the title; `feud_20` plays the whole campaign from the title,
starting a lost match again, until all eight scenarios are won.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Avianos" (raw page): rules, resources and
  caps, tile drops, terrain, the unit table, the five Ancestor tables,
  difficulties and trials, trivia; Earth Move moves only the ground, not
  birds, buildings or flags. https://ufo50.miraheze.org/wiki/Avianos
- [MH-META] https://ufo50.miraheze.org/wiki/Meta_Messages (the button code)
- [MH-DEBUG] https://ufo50.miraheze.org/wiki/Debug_mode
- [MH-TERM] https://ufo50.miraheze.org/wiki/Terminal
- [MANUALS] Steam guide "The missing manuals", section 12: controls, the
  tile display, fog, orders.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [GUIDE] Steam guide "Avianos Ancestors, Miracles, Units, General Info":
  starting bones, nests as recruiting points, miracle details, surprise
  attack, the stall rule, terrain effects.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3345060455
- [GGC] Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [STEAM-GIFT] https://steamcommunity.com/app/1147860/discussions/0/4849904631720632795/
- [STEAM-EH] https://steamcommunity.com/app/1147860/discussions/0/604150365670110300/
- [STEAM-SAVES] https://steamcommunity.com/app/1147860/discussions/0/4849904176633630339/
- [TVT] https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game12Avianos
- [LIZ] https://lizstar64.github.io/reviews/2024/10/11/UFO50-12.html
- [STATIC] https://staticcanvas.substack.com/p/the-ufo-50-diaries-avianos
- [POPCAR] https://popcar.bearblog.dev/reviewing-every-ufo50-game/
