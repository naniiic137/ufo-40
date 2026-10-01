# 39 · BUZZBOLT

*Internal design document. Not shown in the product.*

## Tribute to

**Star Waspir** (UFO 50 game #39, Mossmouth, "May 1988", file VERT.UFO).
The rules were researched from text only: the community wiki's raw and
rendered page and its edit history, the wiki's list of games, Steam
guides and threads, speedrun.com's leaderboard and rules, and written
reviews (listed under Sources; the full notes are in the research folder,
`39-star-waspir.md`). No UFO 50 images, video, sprites, music, wave
layouts, boss patterns or text were used as references, and nothing was
taken from a UFO 50 install.

**Map type: fixed.** Star Waspir's waves are hand-placed and learnable
("linear column waves (memorizable)") and its letter cycle is
deterministic [MONTAGE], [MH], so ours are too (`buzzbolt_waves.c`, all
our own groups, paths and timings). Only the make-up of each wave follows
the original: which kinds of foe turn up in which wave [MH], and what each
wave is for [MONTAGE], [STEAM-W3].

### Full scale

| | Star Waspir | BUZZBOLT |
|---|---|---|
| Players | 1 [MH], [MH-LIST], [WIKI-EN] | 1 |
| Ships | 3 (gray, yellow, red), each with its own spread, focused fire, option and two specials [MH] | 3: lacewing, shieldbug, firefly, the same roles one for one |
| Waves | 5, each ending in a boss; beat wave 5 to win [MH] | 5 |
| Foe kinds | 15 (fly, beetle, squito, heli, locust, big fly, ladybug, big asteroid, asteroid, crab, barrier, hornet, mantis, moth, flower) [MH] | 15: gnat, ironback, midge, whirler, cricket, bloatfly, blister, big puffball, puffball, queen tick, rot wall, goldbug, scythewing, dustwing, sporeheart |
| Bosses | beetle pair (1), big fly (2), crab (3), mantis and moth (4), flower (5) [MH], [MONTAGE] | the Ironback pair, the Bloatfly, the Queen Tick, Scythewing and Dustwing, the Sporeheart |
| Midbosses | "midboss and wave boss enemies drop three" [MH] | the two Ironbacks that come back in wave 3 |
| Pickups | only the E and G blocks [MH] | only the B and Z letters |
| Words | EGG, GGG, EEE, GEE; anything else resets [MH] | BZZ, ZZZ, BBB, ZBB; anything else resets |
| Lives | 2 ships; +1 at 25,000, 100,000, 200,000 [MH], [PUNISHED] | the same |
| Length | the fastest full run takes 6:29; most listed runs take 6:30 to 8:30 [SRC] | waves of 63, 74, 77 and 68 s before their bosses, wave 5 is the boss alone: about 6.5 min for a quick clear |
| Goals | 3 [MH] | the same 3 |
| Stats | highest multiplier, gray best, yellow best, red best [MH-HIST] | the same four, on the high-score screen |

### The waves

| Wave | Star Waspir (text sources) | BUZZBOLT | Foes before the boss |
|---|---|---|---|
| 1 | flies only, in groups that fire once then break away, then reinforcements that fire several times, linear columns; two large beetles with spread volleys end it; the place to build the multiplier [MH], [MONTAGE] | OUTER MEADOW: gnats only, 32 groups (dives, columns, side runs, arcs); the Ironback pair | 110 gnats |
| 2 | flies, squitos, helis, locusts; the first homing shots; the big fly fires green homing shots in repeating patterns and flies worth 100 turn up during the fight [MH], [MONTAGE] | THE THICKET: 36 groups; crickets bring the homing shots; the Bloatfly calls gnats in (worth 100) | 52 gnats, 84 midges, 19 whirlers, 6 crickets |
| 3 | the difficulty spike, "the hardest one by far": red ladybugs' dense patterns, asteroids big and small, homing shots, popcorn, locusts, beetles; a crab with rings that bounce off the bottom [MH], [STEAM-W3], [MONTAGE], [LIZ] | THE ROT: blisters' rings and fans, crickets, spore-rocks (big ones burst into two small), popcorn midges, two Ironback midbosses; the Queen Tick | 82 midges, 5 crickets, 28 blisters, 10 big and 21 small puffballs, 2 Ironbacks |
| 4 | walls with gates (safe multiplier building; walls give way to enough firepower), then the yellow bugs, left side first, then the right: "a huge scoring opportunity"; mantis and moth with bigger homing shots; "smooth sailing" after wave 3 [MH], [MONTAGE], [STEAM-W3] | THE WALLS: eight rows of rot wall (three of them gate rows), then the golden swarm from the left, then from the right; Scythewing and Dustwing | 51 gnats, 114 wall blocks, 48 goldbugs |
| 5 | "a single boss encounter with a disruptive fly" [MONTAGE]; the flower is worth nothing [MH] | THE HEART: the Sporeheart and a gnat that keeps coming back | none |

Wave 3 is checked to be the spike: in the demo player's full runs it has
more enemy shots on screen, on average, than waves 1, 2 and 4 (`bzz_24` …
`bzz_26`).

## Mechanics checklist

| Mechanic | How BUZZBOLT does it | Source | Test |
|---|---|---|---|
| The field | a vertical scroller over the whole wide screen (320 × 180), fast scrolling | [SEARCH-BL], [RESET-40], [LIZ] | bzz_02 |
| Moving | eight ways, 2 px a frame; much slower (0.85) while fire is held | [MH], [RESET-40] | bzz_02 |
| Tap fire | each tap fires the ship's spread and keeps it going for a moment, at full speed | [MH], [RESET-40] | bzz_02 |
| Held fire | after a moment held, focused fire straight ahead, stronger, and the ship slows | [MH], [ANI] | bzz_02 |
| One hit | a shot or a foe's body destroys the ship | [MH] | bzz_12 |
| Losing a ship | the multiplier goes back to x1; options, orbs, the power-up and bombs are lost | [MH] | bzz_12 |
| Lives | two ships; one more at 25,000, 100,000 and 200,000 only; no continues | [MH], [PUNISHED], [STATIC] | bzz_13, bzz_23 |
| Lacewing (gray) | tap: weaving pairs; hold: twin bolts straight up; not firing at all charges a vertical lance that pierces, bigger the longer it charged, fired by the next press | [MH] | bzz_03 |
| Shieldbug (yellow) | tap: a wide arc of seven; hold: a crescent | [MH] | bzz_04 |
| Firefly (red) | tap: three streams; hold: a laser to the first foe in its way; highest damage | [MH], [STEAM-SW] | bzz_05 |
| B | only the firefly uses it: launch a stored bomb | [MH], [STATIC] | bzz_03, bzz_04, bzz_05 |
| Letters | every kill drops one; midbosses and bosses drop three; caught by flying into them | [MH] | bzz_06 |
| The cycle | B, Z, Z, B, Z, Z ... from the run's first kill, bosses' three included | [MH] | bzz_06 |
| The slots | three, top left | [MH], [SEARCH-SLOTS] | (drawn) |
| BZZ | the multiplier +1, no limit | [MH], [SEARCH-MULT] | bzz_07 |
| ZZZ | an option, two at most | [MH] | bzz_07, bzz_08 |
| Valid words keep the combo | ZZZ, BBB and ZBB don't touch the multiplier | [MH], [STEAM-RANT] | bzz_07 |
| Any other word | the multiplier goes back to x1 | [MH], [STEAM-MECH] | bzz_07 |
| Two letters can always be saved | BZ→Z, ZZ→Z, BB→B, ZB→B | [MH] | bzz_07 |
| Lacewing's option | beside the ship firing up while tapping; rushes at foes up close while fire is held; breakable | [MH], [STEAM-W3] | bzz_08 |
| Shieldbug's option | beside the ship copying its fire; moves in front as a shield while fire is held; takes a lot | [MH], [STEAM-W3] | bzz_08 |
| Firefly's option | stays close; homing rockets while fire is held, nothing while tapping; few hit points | [MH] | bzz_08 |
| Lacewing BBB | two orbs circle the ship, soak up any number of shots, break on a foe's body | [MH] | bzz_09 |
| Lacewing ZBB | a huge ally rises under where the ship is and beams straight up for a short while | [MH] | bzz_09 |
| Shieldbug BBB | both fire modes much stronger for a while; a second one restarts the time, never adds | [MH] | bzz_10 |
| Shieldbug ZBB | every enemy shot wiped, a little damage to every foe | [MH] | bzz_10 |
| Firefly BBB | one more bomb, three at most, trailing behind; B launches it, it goes off after a short delay in a moderate radius, a blue circle that wipes shots | [MH], [SEARCH-BOMB] | bzz_11 |
| Firefly ZBB | a pink ally crosses the screen and fires nine shots, three angles at a time; it blocks some shots | [MH], [STEAM-SW] | bzz_11 |
| Scoring | a kill is worth the foe's points times the multiplier; the original's points one for one | [MH] | bzz_16 |
| Second boss's gnats | worth 100 instead of 10 | [MH] | bzz_16 |
| The last boss | worth nothing | [MH] | bzz_16 |
| Time bonus | at the end of each wave, for a quick clear, never multiplied | [MH], [MONTAGE] | bzz_14 |
| Foes leave | most give up and leave if not shot, often firing as they go, with their letters | [MH], [MONTAGE] | bzz_15 |
| Bosses stay | the wave ends only when its boss is beaten | [MH] | bzz_15, bzz_17 … bzz_21 |
| Aimed shots | most are aimed where the ship is when fired, then fly straight: they can be herded | [MONTAGE], [RESET-40] | bzz_29 |
| Homing shots | green and quick; they turn after the ship for a while (sharply enough to come back round) | [MONTAGE], [STEAM-W3] | bzz_29 |
| Bouncing rings | the wave 3 boss's rings bounce off the bottom of the screen | [MH], [MONTAGE] | bzz_19, bzz_29 |
| Walls and gates | rows of blocks; gates switch on and off; enough fire breaks a wall | [MH], [MONTAGE] | bzz_28 |
| The swarm | golden bugs from the left first, then from the right; 300 each | [MH], [MONTAGE] | bzz_27 |
| The pest | in wave 5 a gnat harries the ship; shot down, another comes | [MONTAGE] | bzz_21 |
| Structure | title, ship select, waves 1-5, ending, credits, high-score entry | [MH], [CONV] | bzz_01, bzz_22 |
| No save mid-run | a run left is gone; the table and stats stay | [STEAM-SAVES], [MH] | bzz_01, bzz_22 |
| High scores | an arcade table with names; its default initials spell a message | [SEARCH-HS] | bzz_22, bzz_30 |
| Stats | highest multiplier and each ship's best score | [MH-HIST] | bzz_22 |
| Goals | see below | [MH], [GGC], [STEAM-RANT] | bzz_22, bzz_23 |
| Completion | a demo player clears all five waves with each ship, pressing buttons | — | bzz_24, bzz_25, bzz_26 |

### The three goals

| Goal | Star Waspir | BUZZBOLT |
|---|---|---|
| Beacon (gift) | reach a 10x multiplier | REACH A 10X MULTIPLIER, given the moment it happens |
| Saucer (gold) | beat 5 waves | BEAT ALL FIVE WAVES |
| Alien (cherry) | win with at least 300,000 points | WIN WITH 300,000 POINTS, checked at the ending |

**10x or 20x?** The wiki's infobox says "Obtain a 10x multiplier" [MH];
the "Gift, Gold & Cherry" guide's table says "20x Combo" [GGC]. A player
on Steam trying for the gift writes "WHY IS THE GIFT IN STAR WASPIR
ACTUALLY IMPOSSIBLE!? get a 10x combo sure ok …" [STEAM-RANT], which
agrees with the wiki. So we chose **10x**; the guide's 20x looks like a
slip (20x by the end of wave 2 is the cherry pace in [MONTAGE]).

### Readings we had to choose

- **Which button fires:** A fires, B is the firefly's bomb. One review
  says the player "taps A" [LIZ]; the UFO 50 convention is Button 2 (A)
  for the main action [CONV]. Unconfirmed (see the table below).
- **Tap or hold:** a press held for 10 frames turns into focused fire; a
  tap keeps the spread going for 18 frames, so tapping about three times
  a second fires without a break. Fast 2 px a frame, slow 0.85.
- **The lance:** it starts charging when nothing is firing, needs 20
  frames to fire at all and grows for 150 frames (width 7 to 16, damage 12
  to 38 to each foe it passes through).
- **Letters:** they pop up a little, then fall at most 0.9 px a frame and
  are lost off the bottom; they can't be shot. The cycle runs through the
  whole run, across waves and lives. A boss's three spread out sideways.
  A half-spelt word is lost with the ship (the wiki says every benefit
  goes). After the last boss of a wave there are 2.5 s to catch its letters.
- **A third ZZZ** with two options out mends both.
- **Option hit points:** lacewing 10, shieldbug 24, firefly 8; each shot
  that touches one costs a point, and bumping a foe costs one every few
  frames. The patch notes that changed them [SEARCH-PATCH] aren't readable.
- **Specials' numbers:** the power-up lasts 10 s and doubles damage; the
  screen clear does 10 damage to every foe on screen; the bomb flies for
  0.6 s and blasts 40 px for 0.4 s; the hiveship beams for 2.5 s; the
  dragonfly fires its nine shots in three volleys of three.
- **The new ship:** it comes in at the bottom middle a second after the
  loss and can't be hit for 2 s; while it blinks it can sit inside a boss,
  which is our reading of the speedrunners' "boss kamikaze strats" [SRC].
- **Time bonus:** 100 points for every second under the wave's par (100,
  120, 130, 120 and 80 s), counted from the wave's start to its boss's fall.
- **Midbosses:** the beetle is the original's only big foe that appears
  outside a boss fight (in wave 3) [MH], so our two Ironbacks there are the
  midbosses; they drop three letters and leave after 13 s if not beaten.
- **Big puffballs** burst into two small ones (an asteroid reading).
- **Gates:** on for 0.7 to 1 s, then off as long; off, they are harmless
  and can't be hit.
- **Hit points and patterns** of every foe and boss, and every wave's
  groups and timings, are ours.
- **The ending, credits and table:** the original's are not described; the
  table holds eight names of three letters.

## Not confirmed: controls

| Control | Our reading | Why it is unconfirmed |
|---|---|---|
| Fire on A, bombs on B | A fires (tap or hold), B launches the firefly's bombs | the wiki names a "fire button" and an "alternate-fire button" but not which is which; one review says "tapping A" [LIZ] |
| B for the lacewing and shieldbug | does nothing | the wiki only gives the alternate button a use for the red ship; one review calls it "predominantly a one button game" [STATIC] |
| How long a hold must be | 10 frames | no source |
| How much slower | 0.85 against 2 px a frame | sources say "considerably faster while not firing" [MH] |

## What is ours

- **Name:** BUZZBOLT (1988, Beamdown Softworks).
- **The Hive Wing:** the lacewing, the shieldbug and the firefly; the
  hiveship and the dragonfly.
- **The letters:** B and Z, so the words are BZZ, ZZZ, BBB and ZBB.
- **The Blight:** gnats, midges, whirlers, crickets, blisters, puffballs,
  rot walls and goldbugs; the Ironbacks, the Bloatfly, the Queen Tick,
  Scythewing and Dustwing, and the Sporeheart.
- **The waves:** the Outer Meadow, the Thicket, the Rot, the Walls and the
  Heart, every group and path in them, and every boss pattern.
- **Music:** "Buzzbolt" (title), "Choose Your Wing", "Outer Meadow", "The
  Thicket", "The Rot", "The Walls", "The Big Ones" (bosses), "The
  Sporeheart", "Bloom" (ending), "Flying Home" (credits), "Sign Your Name"
  and two jingles.
- **Words:** the title line, the ending, the credits, the table's names
  (their initials spell NOT ALONE) and every label.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu (with the
controls page) and the three UFO 40 goals, which are Star Waspir's own
(gift, gold, cherry). The terminal code for infinite lives and the meta
message are UFO 50 collection features; UFO 40 has no terminal, so they are
left out.

## Controls

| Input | Action |
|---|---|
| D-pad | fly, eight ways |
| Tap A | the spread, at full speed |
| Hold A | focused fire, slower |
| Rest | (lacewing) the lance charges; the next press fires it |
| B | (firefly) launch a bomb |
| START | pause |
| Title | A start, B library |
| Ship select | LEFT/RIGHT choose, A launch, B back |
| Name entry | UP/DOWN letter, LEFT/RIGHT move, A next, START done |

## Tests

`tests/bzz_01` … `bzz_30` drive the rules with button presses, setting
up a moment with cheats (a still foe, a letter dropped by the cycle, a
shot) and then playing it: tap and hold and the slowdown (02), each
ship's fire (03-05), the letter cycle and every word (06, 07), options
(08), every special (09-11), losing a ship (12), extra ships (13), the
time bonus (14), foes leaving (15), scoring (16), each boss beaten by the
demo player (17-21), goals and saving (22, 23), the structure (27), walls
and gates (28), enemy shots (29) and the table (30). The demo player
(`buzzbolt_bot.c`) tries every way the pad can point, at full speed and
slowed, looks 22 frames ahead at every shot (homing ones followed again
for each move) and foe near it, and takes the safest move that also leads
its target or catches a letter that keeps its word good. With real
presses and no cheats it clears all five waves from the title with each
ship (`bzz_24` … `bzz_26`).

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Star Waspir", raw and rendered: controls,
  fire modes, the charge shot, letters, the cycle, words, the multiplier,
  death, extra lives, the wave bonus, every ship's options and specials,
  the enemy table with points, the goals, TRUE-BLUE.
  https://ufo50.miraheze.org/wiki/Star_Waspir
- [MH-HIST] Its infobox (tracked stats; gift "Obtain a 10x multiplier")
  and edit history. https://ufo50.miraheze.org/w/index.php?title=Star_Waspir&action=history
- [MH-LIST] Miraheze, "List of Games": 1 player, Arcade/Shooter,
  Quick/Reflex, the in-game description. https://ufo50.miraheze.org/wiki/List_of_Games
- [GGC] Steam guide "Gift, Gold & Cherry": "20x Combo", "Beat 5 Rounds",
  "300,000+ Score". https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [MONTAGE] Steam guide "Your Star Waspir Training Montage": each stage's
  make-up, herding, homing shots, the walls and the yellow bugs, wave 5's
  fly, cashing in at bosses, the time bonus.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3431379565
- [STEAM-RANT] Steam thread "Rant Thread", posts by Coxy ("get a 10x
  combo") and Fox Tenson ("GGG giving you an option won't drop it").
  https://steamcommunity.com/app/1147860/discussions/0/4849904345520404401/?ctp=7
- [STEAM-W3] Steam thread "Any tips for Star Waspir wave 3?": the hardest
  wave, homing shots, the yellow ship's shield, the gray's drones.
  https://steamcommunity.com/app/1147860/discussions/0/4700161870965916100/
- [STEAM-SW] Steam thread "Star Waspir": the right pilot's laser, drones
  that break, the UFO buddy, slowing while holding.
  https://steamcommunity.com/app/1147860/discussions/0/4849903793437926235/
- [STEAM-MECH] Steam thread "Game Mechanics the game don't explain": EGG,
  GEE, wrong words reset. https://steamcommunity.com/app/1147860/discussions/0/4699034745340532462/
- [STEAM-SAVES] Steam thread on which games save progress.
  https://steamcommunity.com/app/1147860/discussions/0/4849904176633630339/
- [SRC] speedrun.com, UFO 50 category "39 - Star Waspir": the leaderboard
  (full runs 6:29 and up) and its rules (per ship, "kamikaze strats on
  bosses"). https://www.speedrun.com/api/v1/leaderboards/v1pl7876/category/wdmjn95d
- [LIZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 39": tapping A,
  the speed, the asteroids. https://lizstar64.github.io/reviews/2024/10/19/UFO50-39.html
- [STATIC] The UFO 50 Diaries, "Star Waspir": a one-button game, extra
  lives from score. https://staticcanvas.substack.com/p/the-ufo-50-diaries-star-waspir
- [PUNISHED] Punished Backlog, best UFO 50 games: two ships to start.
  https://punishedbacklog.com/best-ufo-50-games/
- [ANI] AniGamers, mini reviews: "Fiendishly hard". https://anigamers.com/posts/ufo-50-mini-reviews-every-game/
- [RESET-40] ResetEra UFO 50 thread, page 40: the wide screen, tapping
  to keep moving. https://www.resetera.com/threads/ufo-50-ot-50-games-for-the-price-of-1.981834/page-40
- [WIKI-EN] Wikipedia, "UFO 50". https://en.wikipedia.org/wiki/UFO_50
- [SEARCH-*] search-engine summaries quoted in the research notes (TV
  Tropes recap, Backloggd reviews, patch-note titles): the full 16:9 width,
  three letter slots, no multiplier cap, the bomb's blast, the high-score
  initials.
- [CONV] the UFO 50 conventions notes (`00-ufo50-conventions.md`).
