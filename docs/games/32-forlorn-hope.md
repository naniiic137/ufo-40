# 32 · FORLORN HOPE

*Internal design document. Not shown in the product.*

## Tribute to

**Mortol II** (UFO 50 game #32, Mossmouth, "July 1987"). The rules were
researched from text only: the community wiki's raw page, the Steam "missing
manuals" guide (its Mortol II section, read directly), Steam threads and
written reviews (listed under Sources; the full notes are in the research
folder, `32-mortol-ii.md`). No UFO 50 images, video, sprites, music, maps or
text were used as references (the wiki's spoiler map was not opened), and
nothing was taken from a UFO 50 install.

**Map type: fixed.** Mortol II is "a fixed single map where all enemy
placements and key locations are fixed each run" [MH], so ours is too: one
hand-made map, all our own (`tools/forlorn/make_map.py` lays it out and
writes `forlorn_map.c`). Only the structure follows the original: a camp
whose obvious way on is a blind drop into spikes, a tree only a double jump
(or a stone) climbs, plates that must be held down by stones to raise
blocks elsewhere, loose rock for the bomber, a centipede blocking a shaft,
a tower of slime-makers with a fly-maker at the top, a sealed way into the
hearts' room that bombers open, and a castle of keys and locked doors.

| Full scale | Mortol II | FORLORN HOPE |
|---|---|---|
| Lives | 99, no way to gain more; the counter over the base's door [MH], [MM] | 99 in reserve on the counter over the troop's door; the last one goes out at 0 (100 in all) |
| Classes | 5: Warrior, Gunner, Scout, Engineer, Bomber [MH] | 5: mason, hunter, runner, tinker, sapper, with the same attacks, damage, ammo and gifts |
| Foe kinds | 17 (Wall Blob, Flatworm, Fly, Drone, Snail, Cyclops, Baby Bird, Rammer, Head, Wyvern, Hive, Dark Knight, Blobus, Chicken, Minotaur, Scorpion, Centipede) [MH] | 17: wall-eye, oozle, midge, hornet, shellback, hatcheteer, squawker, tusker, idol, drake, hornet bell, rust knight, bloater, broodhen, hornhead, stingback, gulper, with the same hit points |
| Final targets | 4 Dark Hearts, 30 each [MH] | 4 thorn hearts, 30 each |
| Map | one large connected map, "many ways through", the castle and its surroundings [MH], [ROUTE], [START] | 160 x 80 tiles (about 5 x 5 screens): the camp and the Old Yew, the meadow, the undercroft, the caves, the deep, the tower, and Thornkeep (courtyard, two halls, the dungeon, the heart chamber) |
| Keys / doors | generic keys counted on the HUD; locked doors; at least 5 keys [MM], [PIPE] | 8 keys, 5 doors |
| Switches | held down to make blocks appear; an expert route presses 3 [MH], [ROUTE] | 4 plates, each raising its own blocks (3 on the tower route, 1 in the castle) |
| Placed foes | not in the text sources | 40 (two of them hornet bells), plus 8 drains and 2 combs sending out more |
| Goals | Gift: activate a switch; Gold: destroy the Dark Hearts; Cherry: sacrifice fewer than 50 lives to win [MH] | Beacon: weigh down a floor plate; Saucer: burst the four thorn hearts; Alien: burst them with fewer than 50 lost |
| Stats | Most Doors Unlocked, Most Switches Hit [MH] | the same two, on the title, with runs, wins and the fewest lost |
| Music | an intro and one track per class [BC], [LZ] | an intro and one tune per trade, a game-over jingle and the ending |
| Best known run | 13 lives lost [ROUTE]; a reviewer won with 52 left [LZ] | the demo player wins having lost 28 |

## The structure

The camp sits at the west of the surface. The troop's door has the counter
over it and a waystone pad beside it.

- **The obvious way on is a blind drop.** Walk east off the camp and you
  fall four columns into a spike pit you can't see from the top. It is
  avoidable from the first life: the Old Yew beside the door climbs (double
  jumps only) to the canopy; a stone over the drop's mouth carries the next
  volunteer across to the meadow; a stone in the pit gives the next a safe
  landing [START].
- **Below** (the undercroft): key 1 on a shelf, plate 1 in the far corner, a
  drain in the floor and one in the ceiling, a wall-eye, and the loose rock
  over the caves (only a blast opens it). The meadow's hole drops into it.
- **Above** (the canopy): the comb's midges, a squawker, and plate 2 at the
  far end of the branches.
- **The caves**: a hatcheteer and a squawker; a spike chasm that only plate
  1's bridge crosses; steps that only plate 2 raises, up to a ledge with
  plate 3 and the gulper's head in its shaft. West, the vault (door 5, a key,
  and a shaft down to the deep's west shelf).
- **The deep**: under the gulper's shaft a spike pool that only plate 3's
  bridge crosses; the stingback; the tower's foot.
- **The tower**: 29 ledges up, four drains on its right-hand ledges, a
  wall-eye in its wall and a hornet bell at the top. Its top passage east is
  sealed by five columns of brick that look like the rest: two blasts open
  two columns each, a third the last. Doors 3 and 4 cross it halfway up,
  from the dungeon to the heart chamber.
- **Thornkeep**: the gate (door 1) off the meadow; the courtyard (a tusker,
  a drake, a hatcheteer, and a key behind an idol on the walkway); hall 1
  (two rust knights, a hornet bell, a comb, a key, door 2); hall 2 (plate 4
  and the spike trench its bridge crosses, a rust knight, a drake, and a key
  behind the bloater that fills a low corridor); the dungeon (a hornhead, a
  broodhen, an idol, wall-eyes, a key, and door 3 into the tower).
- **The heart chamber**: a walkway across the top with two holes, the four
  hearts on four platforms (upper and lower, left and right), steps up both
  walls, and a spike floor with stepping stones.

The two ways in (both tested): the **tower way**, the original's expert
route in structure (pit stone, key, three plates held by stones, the
gulper fed one volunteer, the scorpion-like stingback worn down, the fly
maker shot, the slime makers capped, the seal blasted, the hearts burst),
and the **castle way** (keys and doors, plate 4, the bloater's corridor, the
dungeon and the tower's crossing). Mixing them works too: the vault's door
drops into the deep, and doors 3 and 4 join the tower to the castle.

## Mechanics checklist

| Mechanic | How FORLORN HOPE does it | Source | Test |
|---|---|---|---|
| Choose a class each life | at the troop's door LEFT/RIGHT browse the five, A sends one out | [MM] | frl_01 |
| Walk, jump, air control | a variable jump (tap low, hold to 2.4 tiles), steering in the air; TIN TROOP's run, acceleration and coyote frames | [MM], [TT] | frl_04 |
| Scout double jump | the runner only: let go of A in the air and press again | [MM] | frl_04 |
| Tap B attacks | mallet (2, close), musket (1, long, 20 shots), throwing knives (1, middle, 5), spanners in an arc (2, 15); the sapper has none | [MH], [MM] | frl_05, frl_06, frl_08, frl_09 |
| Hold B, flash, let go | after 0.75 s the volunteer flashes; letting go then gives them up for their trade's gift | [MM] | frl_05 |
| Death while charging | a death while B is held sets the gift off: a mason on spikes leaves a stone over them, a sapper touching a foe blows up | [MH] | frl_09, frl_10 |
| Warrior's stone | a stone where the mason was, on the ground or in mid-air; no gravity; anyone stands on it; it holds plates down, blocks foes and their shots, shields from falling oozles, caps drains | [MH], [MM], [ROUTE] | frl_05, frl_13, frl_15 |
| Gunner's pouch | fills the hunter's, runner's and tinker's ammo; never used up; as many as you like | [MH], [MM] | frl_06 |
| Scout's teleporter | one at a time (a new one replaces it), made anywhere, mid-air too; UP on the pad by the door goes there, UP at it comes back; the pad turns red while a foe waits by it | [MH], [MM] | frl_07 |
| Engineer's pipe | five cells down from where the tinker was, through the ground; DOWN on it goes down; no collision; overlapping chutes act as one | [MH], [MM], [LZ] | frl_08 |
| Bomber's explosion | 15 to everything close (24 px); breaks loose rock and the seal | [MH], [ROUTE] | frl_09 |
| Lives | 99 on the counter over the door; each lost volunteer, given up or killed, costs one; the counter reaches 0 with one last volunteer to send; then the run is over and back to the title | [MH], [MM], [META] | frl_03 |
| Persistence | wounds, kills, stones, pouches, the waystone, chutes, keys and open doors stay for the run | [MM], [MH] | frl_11 |
| No saving | a run is one sitting; quitting loses it; only records are kept | [SAVES], [MH] | frl_19 |
| Keys and doors | keys are all alike, counted top left with the ammo; a door opens to a key and stays open | [MM], [MH] | frl_12 |
| Switches | down only while something weighs on them; each raises its own blocks | [MH] | frl_13 |
| Breakable rock, secret path | loose rock and a sealed wall only blasts break | [ROUTE] | frl_09, frl_23 |
| Centipede | can't be hurt; walking or jumping into its head spends the volunteer and removes it for good | [MH], [ROUTE] | frl_14 |
| Foes keep their wounds | foe hit points never come back | [MH] | frl_11, frl_23 |
| Each foe's behaviour | wall-eye shoots when close; oozles crawl from drains (on a timer and when you come near) and fall from ceiling drains; midges from combs; hornets from bells, endlessly while the bell hangs; shellback paces; hatcheteer throws axes in an arc; squawker hops madly; tusker charges on sight and keeps going past; idol spits bubbles one way (jump it or get behind it); drake breathes a fan; rust knight slashes curved crescents; bloater lets out rings and blocks a corridor; broodhen shoots a sparse rotating pattern; hornhead leaps onto you; stingback throws oozles | [MH] | frl_15, frl_16 |
| Hit points | 1, 1, 1, 2, 2, 3, 3, 6, 10, 10, 10, 10, 30, 50, 50, 50, unhurtable; hearts 30 | [MH] | frl_06, frl_09 |
| Win | the last heart bursts: Thornkeep falls, the ending, the credits | [MH] | frl_17 |
| Goals | Beacon: a plate pressed; Saucer: the win; Alien: the win with fewer than 50 lost (checked at the win) | [MH], [GGC] | frl_13, frl_17, frl_18 |
| Stats | most doors unlocked, most plates pressed | [MH] | frl_19 |
| 2 players | co-op: the players take turns, a volunteer each, from one pool | [MH-MP], [2P] | frl_21 |
| The last bomber | at 0, a sapper sent out says the last of Holloway carries a lit fuse | [META] | frl_22 |
| Terminal code | a code word lets only three of the five trades out, at random; no goals or records while it is on | [CHEATS] | frl_20 |
| Class music | each trade marches out to its own tune, and the door has the intro's | [BC], [LZ] | (audio) |
| Completable | the demo player wins from the title with real presses, 28 lost | [ROUTE] | frl_23 |
| The castle way | walked with real presses (foes away) | [MH] | frl_24 |

### Readings we had to choose

- **Co-op**: undocumented for Mortol II; Mortol #6 has players "control one
  Mortolian citizen at a time" and players call it "trading off
  controllers" [MH-M], [2P]. So the two players take turns, a volunteer
  each, from one pool of 99; each uses their own pad. Locked on the Vita
  (one controller), like every 2P mode in UFO 40.
- **Units die in one hit** of anything (foes, shots, spikes); no fall
  damage; bodies don't stay (the spikes tip implies they don't make
  bridges) [STATIC], [MH].
- **Charging**: B held for at least 10 frames counts as charging (shorter
  is a tap); the flash comes at 45 frames. A volunteer can walk and jump
  while charging.
- **99 or 100**: the counter shows the reserve; a volunteer can still be
  sent at 0, so 100 go out in all; the meta message needs that [MH], [LZ],
  [META]. The Alien is fewer than 50 lost: a win with 49 lost counts, one with
  50 doesn't (a sapper whose blast bursts the last heart counts as lost).
- **Ammo** is full for every new volunteer.
- **The waystone** works both ways (the wiki's "duck into the teleporter
  and return"), with UP pressed on the pad or the stone; arriving is
  instant after a short flash, and a waystone made in mid-air drops you there.
- **The pouch** drops to the ground where the hunter was.
- **Chutes**: you can get on anywhere along one; you come out at the lowest
  open cell of it (a chute ending in rock lets you out where there's room).
- **Drains, combs and bells**: combs can't be broken; a drain under a stone
  is stopped for good (the expert route caps "the topmost slime-creator");
  bells (hives) die to damage. What drains, combs and bells sent out is
  swept away when a volunteer is lost; nothing else respawns.
- **The hearts** don't attack but kill on touch (the route's "precise
  jumps" suggest they're dangerous to touch).
- **Numbers no source gives**: the blast's radius (24 px), the weapons'
  speeds and reach (musket 4 px a frame for 45 frames, knives 3 for 24,
  spanners an arc of about 50 px), the chute's length (five cells, after
  "5-tall pipes" [LZ]), every foe's speed and cadence (a wall-eye every
  110 frames within 80 px; the stingback an oozle every 2.5 s, two at a
  time), and which plate raises what.
- **The seal**: five columns of brick, two broken a blast, so the
  original's three bombers still open it.
- **Game over** goes back to the cartridge's title (the original returns
  to the collection's menu).

## What is ours

- **Name:** FORLORN HOPE (1987, Beamdown Softworks): the old name for the
  band that goes in first, knowing most won't come back.
- **The folk:** the volunteers of Holloway, and their five trades: the
  mason (mallet; a stone with a statue's face), the hunter (musket; a
  powder pouch), the runner (knives; a waystone), the tinker (spanners; a
  chute) and the sapper (a keg on the back).
- **The place:** the camp and the troop's hut, the Old Yew, the meadow, the
  undercroft, the caves and the vault, the deep, the tower, Thornkeep and
  its heart chamber. The whole map.
- **The foes:** every name and look above, and the four thorn hearts.
- **Music:** "The Forlorn Hope" (intro and the door), "Stone and Mortar",
  "Powder and Shot", "Light Feet", "Gears and Grit", "Short Fuse" (the five
  trades), "The Last Name on the Roll" (game over), "Statues in the Square"
  (the ending).
- **Words:** the story, the goal lines, the ending, the last volunteer's
  line, the code (SLIM-PICK) and every label.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
UFO 40 goals, which are Mortol II's own (gift, gold, cherry). The terminal
riddle about "90 remaining" belongs to the hidden 51st game, which UFO 40
doesn't have, so it is left out. The code screen stands in for the
terminal, as on other UFO 40 cartridges.

## Controls

| Input | Action |
|---|---|
| LEFT / RIGHT | walk, steer in the air; at the door, browse the trades |
| A | jump (tap low, hold higher); the runner jumps again in the air; at the door, send the trade out |
| B (tap) | attack |
| B (hold) | charge: after a moment the volunteer flashes; let go then to give them up |
| DOWN | on a chute: go down it |
| UP | on the pad by the door: go to the waystone; at the waystone: back to the pad |
| START | pause |

### Not confirmed (flagged)

| Item | Our reading | Why it's flagged |
|---|---|---|
| UP to use the waystone (at the pad and at the stone) | UP | the manual and wiki say you teleport from the base, not which button |
| Two players | take turns, one pool | not documented for Mortol II |
| Moving while charging | allowed | not documented |
| How long a hold is a charge (death sets the gift off) | 10 frames | "if already charging" only |
| The flash time | 0.75 s | "a short time" [MM] |
| A unit's hit points | 1 | inferred from reviews |
| Ammo on a new unit | full | not documented |
| Chute exit through rock | lowest open cell | not documented |
| Combs breakable | no | not documented |
| Hearts | harmless at range, deadly to touch | not documented |
| Cherry boundary | fewer than 50 lost | "less than 50" vs a reviewer's "VERY close" at 52 left |
| Every speed, range, radius and cadence | see above | no numbers in any text source |

## Tests

`tests/frl_01` … `frl_24` press real buttons, or set a moment up with
cheats and then play it. `frl_23` is the proof the game can be completed:
from the title, the demo player (`forlorn_bot.c`, a plan of commands for
each volunteer, turned into button presses frame by frame) plays the
tower way: 28 volunteers, every heart burst, all three goals. `frl_24`
walks the castle way into the heart chamber (foes away, to check the map
itself). `frl_02` checks the map: one base and pad, four plates, four
hearts, eight keys, five doors, all seventeen kinds, no foe inside rock.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Mortol II" (raw page): infobox (goals,
  stats, 2P co-op), premise, the classes table (damage, ammo, gifts), the
  enemies table (hit points, hits to kill, notes), the ending, the tips
  (stone in mid-air, endless pouch, pipes without collision, the scout's
  portal tricks, the portal turning red, enemies not regaining health,
  the bomber exploding on touch while charging, the warrior's box on
  spikes, overlapping pipes). https://ufo50.miraheze.org/wiki/Mortol_II
- [MH-M] Miraheze, "Mortol": "Players control one Mortolian citizen at a
  time". https://ufo50.miraheze.org/wiki/Mortol
- [MH-MP] Miraheze, "Multiplayer". https://ufo50.miraheze.org/wiki/Multiplayer
- [META] Miraheze, "Meta Messages": the last bomber's line.
  https://ufo50.miraheze.org/wiki/Meta_Messages
- [CHEATS] Miraheze, "Cheats": a random three of the five classes.
  https://ufo50.miraheze.org/wiki/Cheats
- [MM] Steam guide "The missing manuals - How to play UFO 50 games",
  section 32: choosing a class, the d-pad, down on a pipe, the jump and
  double jump, tap B to attack, hold to flash and release, the HUD (ammo
  and keys top left), the counter over the door, every class, persistence,
  game over. https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [ROUTE] Steam thread "Mortol 2 - Least lives lost" (13 lives: statues on
  spikes and buttons, the scout's tree climb and portals, bombers on rocks
  and a secret path, the worm, the scorpion, the tower's slimes and fly
  maker, four hearts on four platforms).
  https://steamcommunity.com/app/1147860/discussions/0/6757179594727520825/
- [START] Steam thread "Why does Mortol 2 start like this?" (the blind
  drop, the tree, the key you see below).
  https://steamcommunity.com/app/1147860/discussions/0/4626979145080947652/
- [PIPE] Steam thread on pipe oddities and the three-class code (5 keys).
  https://steamcommunity.com/app/1147860/discussions/1/604141990686276324/
- [SAVES] Steam thread "Which games save progress?".
  https://steamcommunity.com/app/1147860/discussions/0/4849904176633630339/
- [2P] Steam thread "Best 2-Player Games?" (Mortol: "trading off
  controllers"). https://steamcommunity.com/app/1147860/discussions/0/4849904631717074845
- [GGC] Steam guide "Gift, Gold & Cherry" (via a search summary: fewer than
  50 deaths). https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [LZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 32": 100 lives, the
  classes, 5-tall pipes, a tune per class, 52 left.
  https://lizstar64.github.io/reviews/2024/10/17/UFO50-32.html
- [STATIC] The Static Canvas, "The UFO 50 Diaries: Mortol II": "you die hard
  and fast". https://staticcanvas.substack.com/p/the-ufo-50-diaries-mortol-ii
- [BC] the soundtrack's track list: an intro and one track per class.
  https://phlogiston.bandcamp.com/album/ufo-50
- [TT] TIN TROOP (UFO 40's Mortol tribute), `docs/games/06-tin-troop.md`:
  the run, jump and air tuning reused.
