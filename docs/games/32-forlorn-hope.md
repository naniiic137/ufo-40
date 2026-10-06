# 32 · FORLORN HOPE

*Internal design document. Not shown in the product.*

## Tribute to

**Mortol II** (UFO 50 game #32, Mossmouth, "July 1987"). The rules were
researched from text only: the community wiki's raw pages, the Steam "missing
manuals" guide (its Mortol II section, read directly), the TV Tropes recap,
Steam guides and threads, speedrun listings and written reviews (listed
under Sources; the full notes are in the research folder, `32-mortol-ii.md`,
and the independent review is `ufo40-review/32-review.md`). No UFO 50
images, video, sprites, music, maps or text were used as references (the
wiki's spoiler map was not opened; only its file size was read), and nothing
was taken from a UFO 50 install.

**Map type: fixed.** Mortol II is "a fixed single map where all enemy
placements and key locations are fixed each run" [MH], so ours is too: one
hand-made map, all our own (`tools/forlorn/make_map.py` lays it out and
writes `forlorn_map.c`). Only the structure follows the original: a camp
whose obvious way on is a blind drop into spikes, a tree only a double jump
(or a stone) climbs, three plates that must be held down to raise blocks
elsewhere, loose rock and out-of-place brick for the bomber, worms blocking
shafts, a tower of slime with a fly-maker at the top, a sealed way into the
hearts' room that bombers open, a bottom-left pocket of keys, a bottom-right
climb that needs every plate held and ends in brick you bomb into the
hearts' room, and a castle of keys and locked doors.

| Full scale | Mortol II | FORLORN HOPE |
|---|---|---|
| Map | "a single, massive level" [MM]; "one giant level" [POP]; the wiki's stitched map file is 2,400 x 2,380 px [MH-CAT], about 6 x 11 of its 384 x 216 screens if drawn at native scale (its scale isn't stated) | 190 x 170 tiles of 10 px (1,900 x 1,700 px), about 6 x 10 of our 320 x 168 views: the camp and the Old Yew, the meadow, the undercroft and its cellar, the caves and the vault, the deep, the sump, the roots, the undermarsh, the chimney, the tower, Thornkeep (courtyard, two halls, the dungeon), the bell loft, the far-east cache and the heart chamber |
| Lives | 99, no way to gain more; the counter over the base's door [MH], [MM]; "100 lives" [LZ], [START] | 99 in reserve on the counter over the troop's door; the last one goes out at 0 (100 in all) |
| Classes | 5: Warrior, Gunner, Scout, Engineer, Bomber [MH] | 5: mason, hunter, runner, tinker, sapper, with the same attacks, damage, ammo and gifts; the cursor starts on the mason, as on the warrior [START] |
| Foe kinds | 17 (Wall Blob, Flatworm, Fly, Drone, Snail, Cyclops, Baby Bird, Rammer, Head, Wyvern, Hive, Dark Knight, Blobus, Chicken, Minotaur, Scorpion, Centipede) [MH] | 17: wall-eye, oozle, midge, hornet, shellback, hatcheteer, squawker, tusker, idol, drake, hornet bell, rust knight, bloater, broodhen, hornhead, stingback, gulper, with the same hit points |
| Final targets | 4 Dark Hearts, 30 each [MH]; they "ominously linger on the black transition screen between deaths" [TVT] | 4 thorn hearts, 30 each, the only thing on the black screen between lives |
| Worms | "Several areas are blocked off by tall vertical worms" [TVT], "red bugs that span multiple screens" [MAPG] | 3 gulpers; the roots' one is 57 tiles long, its head four screens above the passage its body blocks |
| Keys / doors | generic keys counted on the HUD; locked doors; a bottom-left area with "all 5 keys", one guarded by a charger in a nook only the scout reaches from below; a top-right key that needs bombing [MM], [PIPE], [MAPG] | 15 keys, 7 doors; the sump holds four (one in a runner-only nook with a tusker, one behind loose rock), the far east one behind sealed brick, and an idol guards one under the camp |
| Switches | held down to make blocks appear, "colored yellow, green, and blue ... which switch activates which blocks" [TVT]; an expert route presses three [ROUTE] | 3 plates, raising yellow, green and blue blocks (in several places each) |
| Bombable walls | many brick sections out of place in the castle's tiling; secret blocks before the hearts' room [MAPG], [ROUTE] | loose rock in the caves, the sump and the roots; sealed brick (laid in square blocks, in walls thicker than the usual one-tile border) at the seal, the heart chamber's east wall, the far-east cache, the dungeon floor, the courtyard floor and hall 1's wall to the tower |
| Placed foes | not in the text sources | 87, plus 10 drains and 4 combs (and 3 hornet bells among the 87) sending out more |
| Goals | Gift: activate a switch; Gold: destroy the Dark Hearts; Cherry: win having lost fewer than 50 [MH], [GGC] | Beacon: weigh down a floor plate; Saucer: burst the four thorn hearts; Alien: burst them with fewer than 50 lost |
| Stats | Most Doors Unlocked, Most Switches Hit [MH] | the same two, on the title |
| Music | an intro and one track per class [BC], [LZ] | an intro and one tune per trade, a game-over jingle and the ending |
| Best runs | 13 lost, the best-known route [ROUTE]; Gold/Cherry speedruns 2:16 (6th place) [SRC] and 3:42 [START]; a reviewer's first win at 48 lost [LZ] | the expert plan wins having lost 12 in 4:50; the demo player having lost 21 in 4:12 |

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
  over the caves. Past the pit, a shaft down to a cellar where an idol guards
  a key under the camp. The meadow's hole drops into it (one way).
- **Above** (the canopy): the comb's midges, a squawker, and plate 2 at the
  far end of the branches.
- **The caves**: a hatcheteer and a squawker; a spike chasm that only plate
  1's bridge crosses; steps that only plate 2 raises, up to a ledge with
  plate 3 and the first gulper. West, the vault (door 5, a key, and a shaft
  down to the deep's west shelf).
- **The deep**: under the gulper's shaft a spike pool that only plate 3's
  bridge crosses; the stingback; the tower's foot. From its west shelf a
  shaft drops into **the sump**: an east hall of ledges with an idol by a
  key, an upper room whose floor holds the second gulper's head, a nook
  above it that only a double jump reaches (a tusker and a key), and,
  beyond the gulper, a west hall with a key, a comb and a cache of loose rock
  over a fourth key. Door 6 leads east into **the roots**: a great cavern of
  dead-end ledges, a hidden key behind loose rock, a hornet bell, and, high
  in its upper chamber, the head of the third gulper, whose 57-tile body
  blocks the way east four screens down. Past it, **the undermarsh** (a
  spike floor and stepping stones) and door 7 into **the chimney**: 50
  ledges up, three pairs of them blocks of the three plates' colours, to a
  passage that ends at sealed brick in the heart chamber's east wall.
- **The tower**: 29 ledges up, one drain at the top whose oozles roll down
  the ledges, a wall-eye, and the hornet bell in an alcove at the top. Its
  top passage east is sealed by five columns of brick: two blasts open two
  columns each, a third the last. Doors 3 and 4 cross it halfway up, from the
  dungeon to the heart chamber.
- **Thornkeep**: the gate (door 1) off the meadow; the courtyard (a tusker,
  a drake, a hatcheteer, and a key behind an idol on the walkway); hall 1
  (two rust knights, a hornet bell, a comb, a key, door 2); hall 2 (the
  trench that plate 3's blue blocks also bridge, a rust knight, a drake, and
  a key behind the bloater that fills a low corridor); the dungeon (a
  hornhead, a broodhen, an idol, wall-eyes, a key, door 3). Sealed brick
  shortcuts join the courtyard to hall 1, hall 1 to the tower and the
  dungeon to the deep. East of the hearts, a sealed cache with a key and a
  drop into the bell loft (a hornhead, a broodhen, a knight, a comb, a key).
- **The heart chamber**: a walkway across the top with two holes, the four
  hearts on four platforms (upper and lower, left and right), steps up both
  walls, and a spike floor with stepping stones.

The ways in (all tested): the **tower way**, the original's expert route in
structure (pit stone, key, three plates held by stones, the gulper fed one
volunteer, the stingback, the fly maker killed and the slime maker capped,
the seal blasted, the hearts burst); the **bottom-right way** up the
chimney, which needs every plate held and is bombed into the hearts' room;
and the **castle way** (keys, doors, the plate-3 bridge in hall 2, the
bloater's corridor, the dungeon and the tower's crossing).

## Mechanics checklist

| Mechanic | How FORLORN HOPE does it | Source | Test |
|---|---|---|---|
| Choose a class each life | at the troop's door LEFT/RIGHT browse the five, A sends one out | [MM] | frl_01 |
| Walk, jump, air control | a variable jump (tap low, hold to 2.4 tiles), steering in the air; TIN TROOP's run, acceleration and coyote frames | [MM], [TT] | frl_04 |
| Scout double jump | the runner only: let go of A in the air and press again | [MM] | frl_04 |
| Tap B attacks | mallet (2, close), musket (1, long, 20 shots), throwing knives (1, middle, 5), spanners in an arc (2, 15); the sapper has none | [MH], [MM] | frl_05, frl_06, frl_08, frl_09 |
| Hold B, flash, let go | after 0.75 s the volunteer flashes (and only that: no meter); letting go then gives them up for their trade's gift | [MM] | frl_05 |
| Moving while charging | allowed (the route's statue "just a hair past the apex" of a jump; bombers touching foes while charging) | [ROUTE], [MH] | frl_09 |
| Death while charging | a death while B is held sets the gift off: a mason on spikes leaves a stone over them, a sapper touching a foe blows up | [MH] | frl_09, frl_10 |
| Charge through waystones and chutes | the charge carries through both and can be let go there: through a waystone it goes off on arrival (a mason's stone just below the waystone, the original's endless climb); in a chute it goes off where the volunteer is (a tinker's second chute, which joins the first) | [MH], [PIPE] | frl_26, frl_08 |
| Warrior's stone | a stone where the mason was, on the ground or in mid-air; no gravity; anyone stands on it; it holds plates down, blocks foes and their shots, shields from falling oozles, caps drains and combs | [MH], [MM], [ROUTE] | frl_05, frl_13, frl_15, frl_28 |
| Gunner's pouch | fills the hunter's, runner's and tinker's ammo; never used up; as many as you like | [MH], [MM] | frl_06 |
| Scout's teleporter | one at a time (a new one replaces it), made anywhere, mid-air too; two-way between it and the base: UP on the pad by the door goes there, UP at it comes back; the pad turns red while a foe waits by it | [MH], [MM], [TVT] | frl_07, frl_26 |
| Engineer's pipe | goes "straight downwards and exactly 4 tiles at a time ... through any terrain": ours runs five cells from the tinker's own cell, so its exit is four below; DOWN while standing on it goes down it (not in mid-air); no collision; overlapping chutes act as one | [MH], [MM], [TVT], [LZ] | frl_08 |
| Bomber's explosion | 15 to everything close (24 px); breaks loose rock, sealed brick and combs | [MH], [ROUTE], [MAPG] | frl_09, frl_30 |
| One hit | "All of the classes will die the moment they touch an enemy or a projectile" | [TVT] | frl_10, frl_15 |
| Lives | 99 on the counter over the door; each lost volunteer, given up or killed, costs one; the counter reaches 0 with one last volunteer to send; then the run is over and back to the title | [MH], [MM], [META] | frl_03 |
| Between lives | a black screen with only the thorn hearts still beating on it, then the door (not before the first volunteer, not after the win) | [TVT] | frl_27 |
| Persistence | wounds, kills, stones, pouches, the waystone, chutes, keys and open doors stay for the run | [MM], [MH] | frl_11 |
| No saving | a run is one sitting; quitting loses it; only records are kept | [SAVES], [MH] | frl_19 |
| Keys and doors | keys are all alike, counted top left with the ammo; a door opens to a key and stays open | [MM], [MH] | frl_12 |
| Switches | down only while something weighs on them; three plates, each raising its own yellow, green or blue blocks | [MH], [TVT] | frl_13, frl_31 |
| Spawners | drains, combs and bells "can be blocked off or destroyed": a stone over a drain or in a comb's mouth stops it; a comb breaks after 10 damage, a bell (hive) after 10 | [TVT], [ROUTE] | frl_15, frl_28, frl_25 |
| Worms | several, impervious; walking or jumping into a head spends the volunteer and removes that worm (only that one) for good | [MH], [TVT], [MAPG] | frl_14, frl_29 |
| Foes keep their wounds | foe hit points never come back | [MH] | frl_11, frl_23 |
| Each foe's behaviour | wall-eye shoots when close; oozles crawl from drains (on a timer and when you come near) and fall from ceiling drains; midges from combs; hornets from bells, endlessly while the bell hangs; shellback paces; hatcheteer throws axes in an arc; squawker hops madly; tusker charges on sight and keeps going past; idol spits bubbles one way (jump it or get behind it); drake breathes a fan; rust knight slashes curved crescents; bloater lets out rings and blocks a corridor; broodhen shoots a sparse rotating pattern; hornhead leaps onto you; stingback throws oozles | [MH] | frl_15, frl_16 |
| Hit points | 1, 1, 1, 2, 2, 3, 3, 6, 10, 10, 10, 10, 30, 50, 50, 50, unhurtable; hearts 30, harmless at range but deadly to touch ("no way of fighting back") | [MH], [TVT] | frl_06, frl_09 |
| Win | the last heart bursts: Thornkeep's thorns die, the ending, the credits | [MH] | frl_17 |
| Goals | Beacon: a plate pressed; Saucer: the win; Alien: the win with fewer than 50 lost (checked at the win) | [MH], [GGC] | frl_13, frl_17, frl_18 |
| Stats | most doors unlocked, most plates pressed (the only records on the title) | [MH] | frl_19 |
| 2 players | co-op: the players take turns, a volunteer each, from one pool | [MH-MP], [2P] | frl_21 |
| The last bomber | at 0, a sapper sent out says the last of Holloway carries a lit fuse | [META] | frl_22 |
| Terminal code | a code word lets only three of the five trades out, at random; no goals or records while it is on | [CHEATS] | frl_20 |
| Class music | each trade marches out to its own tune, and the door has the intro's | [BC], [LZ], [STATIC] | (audio) |
| Completable, at par | the demo player wins from the title with real presses having lost 21; the expert plan wins having lost 12 | [ROUTE] | frl_23, frl_25 |
| The other ways in | the chimney (all plates held) and the castle way, walked with real presses (foes away) | [MAPG], [MH] | frl_31, frl_24 |

### Readings we had to choose

- **Co-op**: undocumented for Mortol II (the TV Tropes infobox says only
  "1-2p"); Mortol #6 has players "control one Mortolian citizen at a time"
  and players call it "trading off controllers" [MH-M], [2P]. So the two
  players take turns, a volunteer each, from one pool of 99; each uses
  their own pad. Locked on the Vita (one controller), like every 2P mode in
  UFO 40.
- **No fall damage**, and bodies don't stay (the spikes tip implies they
  don't make bridges) [MH].
- **Charging**: B held for at least 10 frames counts as charging (shorter
  is a tap); the flash comes at 45 frames.
- **99 or 100**: the counter shows the reserve; a volunteer can still be
  sent at 0, so 100 go out in all; the meta message needs that [MH], [LZ],
  [START], [META]. The Alien is fewer than 50 lost: a win with 49 lost
  counts, one with 50 doesn't (a sapper whose blast bursts the last heart
  counts as lost) [MH]; the GGC guide's "50- Deaths" reads the same way.
- **Ammo** is full for every new volunteer.
- **The waystone**: UP pressed on the pad or the stone; a waystone made in
  mid-air drops you there.
- **The pouch** drops to the ground where the hunter was.
- **Chutes**: you come out at the lowest open cell (a chute ending in rock
  lets you out where there's room).
- **Spawners**: what drains, combs and bells sent out is swept away when a
  volunteer is lost; nothing else respawns.
- **The black screen** shows the remaining hearts in a row, for 50 frames.
- **Numbers no source gives**: the blast's radius (24 px), the weapons'
  speeds and reach (musket 4 px a frame for 45 frames, knives 3 for 24,
  spanners an arc of about 50 px), every foe's speed and cadence (a wall-eye
  every 110 frames within 80 px; the stingback an oozle every 2.5 s, two at
  a time), a comb's 10 hit points, and which plate raises what where.
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
- **The place:** every region named above, and the whole map.
- **The foes:** every name and look above, and the four thorn hearts.
- **Music:** "The Forlorn Hope" (intro and the door), "Stone and Mortar",
  "Powder and Shot", "Light Feet", "Gears and Grit", "Short Fuse" (the five
  trades), "The Last Name on the Roll" (game over), "The Roll Is Read"
  (the ending).
- **Words:** the story, the goal lines, the ending (the thorns let go of
  the keep and turn to dust, Holloway's bells ring again, the counter comes
  down and the roll is read aloud, name by name), the last volunteer's
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
| B (hold) | charge: after a moment the volunteer flashes; let go then to give them up (also in a chute or through a waystone) |
| DOWN | standing on a chute: go down it |
| UP | on the pad by the door: go to the waystone; at the waystone: back to the pad |
| START | pause |

### Not confirmed (flagged)

| Item | Our reading | Why it's flagged |
|---|---|---|
| UP to use the waystone (at the pad and at the stone) | UP | the sources say you teleport from the base and back, not which button |
| Two players | take turns, one pool | not documented for Mortol II |
| How long a hold is a charge (death sets the gift off) | 10 frames | "if already charging" only |
| The flash time | 0.75 s | "a short time" [MM] |
| Ammo on a new unit | full | not documented |
| Chute exit through rock | lowest open cell | "through any terrain" [TVT]; one bug report drops you out of the map |
| Three plates | one per block colour | three colours [TVT] and three presses [ROUTE] point to three; the count itself isn't stated |
| Cherry boundary | fewer than 50 lost | "less than 50" [MH] vs a reviewer's "VERY close" at 52 left [LZ] |
| Map scale | about 6 x 10 views | the wiki file's scale isn't stated [MH-CAT] |
| Every speed, range, radius and cadence | see above | no numbers in any text source |

## Tests

`tests/frl_01` … `frl_31` press real buttons, or set a moment up with
cheats and then play it. The demo player (`forlorn_bot.c`, a plan of
commands for each volunteer, turned into button presses frame by frame)
proves the game can be completed: `frl_23` plays from the title along the
tower way, 21 volunteers, every heart burst, all three goals. `frl_25` is
the expert plan: 12 lost, with one mason dueling the stingback, killing the
wall-eye and the bell and capping the drain, three sappers on the seal and
one mason bursting all four hearts and living. `frl_24` walks the castle way
and `frl_31` climbs the chimney (foes away, to check the map itself).
`frl_26` is the endless waystone-and-stone climb, `frl_27` the black screen,
`frl_28` the combs, `frl_29` the worms, `frl_30` sealed brick. `frl_02`
checks the map: one base and pad, three plates, four hearts, fifteen keys,
seven doors, three gulpers, all seventeen kinds, no foe inside rock.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Mortol II" (raw page): infobox (goals,
  stats, 2P co-op), premise, the classes table (damage, ammo, gifts), the
  enemies table (hit points, hits to kill, notes), the ending, the tips
  (stone in mid-air, endless pouch, pipes without collision, the scout's
  portal tricks and the endless climb, the portal turning red, enemies not
  regaining health, the bomber exploding on touch while charging, the
  warrior's box on spikes, overlapping pipes). https://ufo50.miraheze.org/wiki/Mortol_II
- [MH-CAT] Miraheze, "Category:Mortol II maps": only the map file's size,
  2,400 x 2,380 px. https://ufo50.miraheze.org/wiki/Category:Mortol_II_maps
- [MH-M] Miraheze, "Mortol": "Players control one Mortolian citizen at a
  time". https://ufo50.miraheze.org/wiki/Mortol
- [MH-MP] Miraheze, "Multiplayer". https://ufo50.miraheze.org/wiki/Multiplayer
- [META] Miraheze, "Meta Messages": the last bomber's line.
  https://ufo50.miraheze.org/wiki/Meta_Messages
- [CHEATS] Miraheze, "Cheats": a random three of the five classes.
  https://ufo50.miraheze.org/wiki/Cheats
- [MM] Steam guide "The missing manuals - How to play UFO 50 games",
  section 32: a single, massive level; choosing a class, the d-pad, down on
  a pipe, the jump and double jump, tap B to attack, hold to flash and
  release, the HUD (ammo and keys top left), the counter over the door,
  every class, persistence, game over.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [TVT] TV Tropes recap "UFO 50 Game 32 Mortol II The Confederacy Of
  Nilpis": one hit kills, the hearts on the black screen between deaths,
  several tall worms, spawners that can be blocked off or destroyed, the
  three block colours, two-way teleporting, pipes four tiles through any
  terrain, the hearts' "no way of fighting back".
  https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game32MortolIITheConfederacyOfNilpis
- [MAPG] Steam guide "Mortol II - Full Map" (its text and comments only):
  the map's size, worms spanning screens, bombable bricks out of place in
  the castle's tiling, getting every switch and bombing into the chamber
  from below. https://steamcommunity.com/sharedfiles/filedetails/?id=3335017863
- [ROUTE] Steam thread "Mortol 2 - Least lives lost" (13 lives: statues on
  spikes and buttons, the scout's tree climb and portals, bombers on rocks
  and a secret path, the worm, the scorpion, the tower's slimes and fly
  maker, four hearts on four platforms).
  https://steamcommunity.com/app/1147860/discussions/0/6757179594727520825/
- [START] Steam thread "Why does Mortol 2 start like this?" (the blind
  drop, the tree, the key you see below, 100 lives, the warrior as the
  default pick, a 3:42 run).
  https://steamcommunity.com/app/1147860/discussions/0/4626979145080947652/
- [PIPE] Steam thread on pipe oddities (a second pipe made mid-pipe; the
  bottom-left keys and the charger in the scout's nook; the three-class code).
  https://steamcommunity.com/app/1147860/discussions/1/604141990686276324/
- [SAVES] Steam thread "Which games save progress?".
  https://steamcommunity.com/app/1147860/discussions/0/4849904176633630339/
- [2P] Steam thread "Best 2-Player Games?" (Mortol: "trading off
  controllers"). https://steamcommunity.com/app/1147860/discussions/0/4849904631717074845
- [GGC] Steam guide "Gift, Gold & Cherry", read directly: "Activate 1
  Switch / Beat the Game / Win w/ 50- Deaths".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [SRC] speedrun.com, Mortol II Gold/Cherry, 2:16.167, 6th place (from a
  search listing; the site refuses direct reads).
  https://www.speedrun.com/UFO_50/runs/ydvgjd0y
- [LZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 32": 100 lives, the
  classes, 5-tall pipes, a tune per class, 52 left.
  https://lizstar64.github.io/reviews/2024/10/17/UFO50-32.html
- [STATIC] The Static Canvas, "The UFO 50 Diaries: Mortol II": "you die hard
  and fast"; the class tunes restarting with every life.
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-mortol-ii
- [POP] Popcar's Blog, "Reviewing Every Single UFO 50 Game": one giant level.
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [BC] the soundtrack's track list: an intro and one track per class.
  https://phlogiston.bandcamp.com/album/ufo-50
- [TT] TIN TROOP (UFO 40's Mortol tribute), `docs/games/06-tin-troop.md`:
  the run, jump and air tuning reused.
