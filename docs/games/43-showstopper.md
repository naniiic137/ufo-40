# 43 · SHOWSTOPPER

*Internal design document. Not shown in the product.*

## Tribute to

**Elfazar's Hat** (UFO 50 game #43, Mossmouth, "October 1988"). The rules
were researched from text only: the community wiki's raw page and its
Cheats, Debug mode and Terminal pages, the Steam guides "The missing
manuals" (section 43), "Elfazar's Hat Guide" and "Gift, Gold & Cherry",
Steam threads, written reviews and TV Tropes (listed under Sources; the
full notes are in the research folder, `43-elfazars-hat.md`). No UFO 50
images, video, sprites, music, stage layouts or text were used as
references, and nothing was taken from a UFO 50 install.

**Map type: fixed.** The original's stages, formations, hidden ticket
spots and bosses are the same every attempt [STATIC], [MH]; only what a
beaten foe drops is left to chance. Ours are fixed too (`sst_stages.c`,
every row our own), with the card colours rolled. Only the structure
follows the original:

| Full scale | Elfazar's Hat | SHOWSTOPPER |
|---|---|---|
| Players | 1, or 2 in co-op [MH] | 1 now; 2 in the menu, greyed out until the next update (see the last section) |
| Stages | 5: a scrolling snow village; one screen on a train; a longer town in two halves; the longest, a palace in two halves; a short tower (lift, then the final boss) [MH], [TVT] | the same 5: Frostbell Lane (200 rows); the Night Freight (one screen); Lanternbury and the Canal Side (242 rows, longer than I); the Glass Palace and the Upper Halls (280 rows, the longest); Morrow's Tower (the lift, then the crown room) |
| Fights | I: a mini-boss and a boss; II: waves only; III: a tank that scrolls the road, then a boss; IV: two mini-bosses and the magician; V: the demon form, and on a no-continue run a last one [MH], [TVT] | I: the Snow Engine, Granny Goggle; II: fourteen waves; III: the Boiler (the chase), the Doppels; IV: Old Wicks, the Ribbon, Morrow the Marvellous; V: twelve waves, the Morrowmaw, and on a clean run the Heartstack |
| Foes | the stage lists in the bible (blobs, lanterns, clowns, window bats, turrets, snowballs; cloaked pumpkin heads, divers, toy trains; roof bombers, snowmen, rocket launchers, copter blobs, wall guns; turtles, a harder clown, skull flame jets, splitting flames, book trains, bomb books, racecars, torches) [MH], [TVT] | the same 22 kinds, plus the bosses' minions (a clockwork mouse, eggs and hopping skulls, roaming fire, bats, spike blocks) |
| Cards | 5 colours, A (pair) and B (three of a kind) [MH], [MANUALS] | the same 10 powers |
| Hidden tickets | 9: three each in stages I, III and IV [GUIDE] | 9 star passes, three each in I, III and IV |
| Lives | a counter of extra lives, 9 at most [MANUALS], [MH-DEBUG], [GUIDE]; "another 3 lives" a continue [POPCAR] | 3 a credit (the counter shows the 2 spares), 9 spares at most |
| Endings | 2 [TVT] | 2 |
| Goals | 3 [MH], [GGC] | the same 3 |
| Stat | Tickets Found [MH] | Passes found (the most in one run), the only record on the title |
| Length | ~30 min; a speedrun ~13 min [POPCAR], [SR] | the demo player (perfect aim, hit a handful of times, few powers) finishes in about 12.7 minutes, 11.5 of them in the stages |

What each stage brings: I snow streets with houses whose eaves drop snow,
two icy stretches, a frozen pond, pits, crates and barrels, gumdrops,
lanterns, jesters, window bats, wall guns and snowballs; the Snow
Engine's yard (pits along both sides) half way, Granny Goggle's stage at
the top. II the roof of three cars with gaps between them (a walkway in
the middle of each) and the ground rushing by on both sides. III a town
with roof bombers, snowmen, rocketeers, copters, triple guns, ice and a
canal; the winding road with pits on both sides where the Boiler backs
away, then the Doppels' bridge over a chasm. IV a palace with turtles,
harlequins, skull jets in the walls, emberlings, book trains, page imps,
racecars and torches; Old Wicks's crypt ends the first half, the Ribbon's
ballroom is half way up the second, Morrow's stage is at the top. V the
lift, open on every side, and the crown room.

## Mechanics checklist

| Mechanic | How SHOWSTOPPER does it | Source | Test |
|---|---|---|---|
| Moving | eight ways at one pace (1.25 px a frame, the same speed on the diagonals); walls and crates stop her | [MH], [MANUALS] | sst_03 |
| Ice | on ice tiles (stages I and III only) she speeds up and slows down slowly, and slides | [TVT] | sst_03 |
| The wand | a press fires at once the way the pad points (or she faces); held, it keeps firing in bursts of three and the facing stays put while she walks any way; tapped fast it fires more than holding does, but the facing follows the pad between taps | [MH], [MANUALS], [PONETT], [GUIDE] | sst_04 |
| The dash | the second button: 14 frames at 3.5 px the way she last moved; nothing hurts her meanwhile; she crosses pits; foes dashed through are stunned; no harm done by it; the locked aim is kept | [MANUALS], [MH], [TVT], [GUIDE] | sst_05 |
| Hearts | 3, 4 at most with Extend; a hit costs one, then a moment's safety | [MH], [TVT] | sst_06 |
| Pits | a fall costs a life at once, whatever the hearts | [MH], [MANUALS] | sst_06 |
| The bubble | a lost life brings her back on the spot in a bubble that floats anywhere (over pits too); a button pops it on firm ground; its time runs out and it pops anyway; a moment's safety after; all card powers gone | [MANUALS], [MH] | sst_06 |
| Lives and continues | 3 a credit; the counter shows the extra lives (2 to start), 9 at most; all gone: CONTINUE?, waiting for an answer: yes starts the stage again with 3, as often as wanted, and closes the true ending; no is game over | [MH], [MH-DEBUG], [POPCAR], [MANUALS] | sst_10, sst_12 |
| Cards | dropped by foes (about half the time) and always by crates and barrels; three slots; the third card makes a hand and empties them; order doesn't matter: a pair and one other gives the pair's A, three of a kind its B, three different nothing | [MH], [MANUALS], [TVT] | sst_07, sst_09 |
| Purple | A heals a heart; B (Extend) raises the hearts to 4 and heals one (with 4 already, it heals) | [MH], [TVT] | sst_08 |
| Green | A: the first shot of each volley flares two more out on the diagonals; B: the same, and every shot bigger (2 harm) | [MH], [GUIDE] | sst_08 |
| Orange | A: a one-eyed blob follows and fires when she fires; B: a stubby round one that also keeps firing while A is held | [MH], [GUIDE] | sst_08 |
| Red | A: a bomb where each dash starts; B: a trail of three | [MH], [TVT] | sst_08 |
| Blue | A: one spiked ball circles her and stops shots until it breaks; B: five | [MH], [TVT] | sst_08 |
| Powers last | through stages, until a life is lost; colours stack | [MH], [GUIDE], [POPCAR] | sst_06, sst_08 |
| Hidden spots | an odd-looking spot or object; it rings when shot and gives up a pass after five shots; a pass scrolled off is lost (the view never goes back) | [GUIDE], [TVT], [STEAM-DEMO] | sst_11 |
| The bonus round | after a stage, holding passes: the Hop, or later (passes kept); playing it spends them all, each pass a try; Jelly hops square to square on a board seen at an angle, on by itself the way the pad last set; holes, spikes and mouths cost a try; coins fill a meter whose marks are lives | [MH], [GUIDE], [STEAM-WHAT], [LIZ] | sst_12 |
| The secret hand | red, then blue, then green as one hand flashes a code and opens the bonus round for endless practice on the title (falls cost nothing; B leaves) | [MH-CHEATS], [GUIDE] | sst_13 |
| Stage I foes | gumdrops creep at her; lanterns let out fireballs; jesters fire fans; bats wait behind windows; wall guns; snowballs hop; snow drops from eaves | [MH], [TVT] | sst_14 |
| Train and town foes | gourd cloaks from both sides with wavy pairs, sometimes circling the car leaving bomb shots that can be shot; puffers dive over; toy trains run along; roof bombers; snowmen rush then fire stars from the nose; rocketeers' rockets land on a red X and blow up a 3 x 3 area; copter blobs with long arms (shoot the middle); wall guns | [MH], [TVT] | sst_15 |
| Palace foes | turtles fire rings; harlequins drop many bombs; skulls breathe long blue flames one way; emberlings split into two smaller ones at every hit, and these fly off; book trains of separate books; page imps toss bombs; racecars leave a slick that hurts; torches spit fire | [MH], [TVT] | sst_16 |
| The Snow Engine | strafes on a big yard; homing critter; the crows on top stream shots; snowballs hop in; hit anywhere | [MH], [TVT] | sst_17 |
| Granny Goggle | stays put; only the eye takes harm; the gloves throw fire, fire volleys, turn into spiked balls that hunt; she coughs up bombs | [MH], [TVT] | sst_17 |
| The Boiler | backs up the winding road with the view scrolling after it; only its head; missiles land on marks; rings of shots | [MH], [TVT] | sst_17 |
| The Doppels | two with one bar; urchins diving across the bridge, devil faces with spreads, aliens laying spike blocks | [MH], [TVT] | sst_17 |
| Old Wicks | blinks about; eggs hatch into hopping skulls; four torches keep sending out roaming fire | [MH], [TVT] | sst_18 |
| The Ribbon | slithers; rings of shots; a slime trail that hurts (dash over it); only the head | [MH], [TVT] | sst_18 |
| Morrow | floats on his stage, hit anywhere; a stream from the hat, a star wall to dash through, a bat swarm, two copies that fire together from the sides | [MH], [TVT] | sst_18 |
| The Morrowmaw | stays put; the two great eyes hold all its life; the arms' eyes fire rounds and can be shot shut a while; the mouth lets out a snake that only shooting drives back | [MH], [TVT] | sst_19 |
| The Heartstack | only with no continue: 23 blocks circling clockwise; red ones fire across the room, green ones break; with twelve left a heart in the middle that sprays rings until broken | [MH], [TVT], [KAYIN] | sst_19 |
| Endings | with a continue: the magician's true face beaten, the friends not found; with none: the Heartstack, then the friends freed | [TVT], [MH] | sst_19 |
| Goals | Beacon: win a life in the Hop; Saucer: beat the Morrowmaw; Alien: do it with no continue (and so beat the Heartstack) | [MH], [GGC], [GUIDE] | sst_12, sst_19, sst_20 |
| No saving | a run is one sitting; the records and the goals are kept | [MH] | sst_20 |
| Music | a tune for each stage (two for the two-half stages), the map, the bosses, the true face and the last form each their own, the Hop, the endings | [POPCAR], [LIZ], [GUIDE] | sst_23 |
| Playable start to end | the demo player, with real button presses from the title, plays all five stages and every boss to the true ending; another plays the last stage after a continue to the sad one | | sst_24, sst_25 |

### Readings we had to choose

- **Lives:** 3 a credit, counting the one in play, so the counter (which
  shows the extra lives, as the original's does) starts at 2 and goes up
  to 9; a continue gives 3 again (reviewers say a continue grants "another
  3 lives").
- **Card drops:** foes 1 in 2 (the big ones always: copters, turtles,
  harlequins, racecars), crates and barrels always; a random colour. A
  card lies for 9 s. A small hand never takes a big power away; the blue
  small hand on a big shield tops up its weakest ball.
- **Hands:** the hand in the slots is kept when a life is lost (only the
  powers go).
- **Numbers no source gives:** walking 1.25 px a frame; the dash 14
  frames at 3.5 px, 10 frames before the next; a hit leaves her safe 1.5
  s, the bubble 2 s; the bubble lasts 5 s; shots fly 5 px a frame; a press
  fires at most every 5 frames; held, the first burst after 12 frames,
  three shots 6 frames apart every 40 frames; at most 14 shots at once; a
  dash stuns for 1.25 s; a hidden spot takes 5 shots; spiked balls take 3
  shots each; bombs blow up after 40 frames, 26 px across the blast, 4
  harm.
- **Hops:** the Hop's marks at 10, 25, 45, 70, 100, 135, 175, 220 and 270
  coins; a hop takes 20 frames; spikes are up 40 frames in 120, mouths
  open 45 in 150; the board is made as Jelly goes (from a seed), harder the
  further on, with one lane always safe (a way on that never needs a
  diagonal hop); it ends 150 rows on, and any tries left there are over
  (the passes were spent when the round began). The practice board has no
  end and no limit of tries.
- **Emberlings:** each size takes one hit: a big one splits into two
  middle ones, each of those into two small ones, which go out.
- **Every boss** shows itself for 1.5 s before it fights. Boss health (in
  shots of 1): the Snow Engine 150, Granny Goggle 220, the Boiler 170, the
  Doppels 300, Old Wicks 180, the Ribbon 170, Morrow 340, the Morrowmaw
  2 x 200, the Heartstack 11 green blocks of 18 and 12 red of 24.
- **The view:** it climbs as she goes and never comes back down; a boss
  wakes while still above it, then the view stops with the arena framed.
  The scroll is upward only.
- **The Boiler:** it backs up the road at 0.375 px a frame and the view
  goes with it; the bottom of the view pushes her on. Its low hull doesn't
  stop shots (they fly over it at the turret).
- **The lift's waves** (the original's are not described): bats, gourd
  cloaks, puffers, emberlings, page imps, a book train and copters, twelve
  waves in all.
- **The secret hand's code** flashes FREE-RIDE, our own word.

## What is ours

- **Name:** SHOWSTOPPER (1988, Beamdown Softworks).
- **Heroine:** Clover, a white show rabbit with a wine scarf and the
  magician's wand. Her helpers: the one-eyed blob and the stubby one.
  Jelly, the pink blob of the Hop.
- **The villain:** Morrow the Marvellous, who keeps his show's animals
  in cages; his true face, the Morrowmaw; and the Heartstack behind it.
- **The foes:** gumdrops, lanterns, jesters, bats, wall guns, snowballs,
  gourd cloaks, puffers, toy trains, bombers, snowmen, rocketeers,
  copters, triple guns, turtles, harlequins, skull jets, emberlings, book
  trains, page imps, racecars, torches; the Snow Engine, Granny Goggle,
  the Boiler, the Doppels, Old Wicks, the Ribbon.
- **The world:** Frostbell Lane, the Night Freight, Lanternbury and the
  Canal Side, the Glass Palace and the Upper Halls, Morrow's Tower; the
  Hop's board. Every row of every stage.
- **Cards and powers:** our faces (a heart, a crescent, a cross, a target,
  a star) and names (heal, extend, spread, big spread, blinker, stubby,
  bomb, bomb trail, spike ball, five balls). Star passes for tickets.
- **Music:** "Showstopper" (title), "The Cage Door" (story), "The Route"
  (map), "Frostbell Lane", "The Night Freight", "Lanternbury", "The Canal
  Side", "The Glass Palace" (a waltz), "The Upper Halls", "Going Up" (the
  lift), "Big Top Brawl" (bosses), "The Morrowmaw", "The Heartstack",
  "Jelly's Hop", "Curtain" and "Lights Down" (jingles), "An Empty Cage"
  and "Encore" (the endings), "Take a Bow" (credits).
- **Words:** the story, both endings, the goal lines and every label.

## Owner's changes

- **The main action on A.** The original fires with Button 1 (B) and
  dashes with Button 2 (A) [MANUALS]. Here the wand is on A (keyboard Z /
  Space, Vita Cross) and the dash on B; the cartridge card's CONTROLS page
  can swap them back.
- **2 PLAYERS greyed out.** The original's co-op is in the title menu but
  can't be picked on any platform yet: highlighted, it shows
  "2 PLAYERS · NEXT UPDATE". Everything for one player is at full scale.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
UFO 40 goals, which are the original's own (gift, gold, cherry). The
original's Terminal code opens the bonus round's practice; UFO 40 has no
terminal, so the code's word flashes in the game and the practice opens
on the title (the same trigger, the same reward).

## Controls

| Input | Action |
|---|---|
| D-pad | walk eight ways (sets the aim while A is up) |
| A (tap) | shoot the way she faces (or the pad points) |
| A (hold) | keep shooting in bursts, the aim locked while she walks |
| B | dash (safe, over pits) |
| A / B in the bubble | pop it (on firm ground) |
| D-pad in the Hop | the way the next hop goes (up: on; left, right: across; down: back) |
| START | pause |

## Not confirmed

| Control or rule | Our reading |
|---|---|
| The shoot button | Button 1 / B in the original [MANUALS]; A here (owner's layout, swappable) |
| The dash button | Button 2 / A in the original; B here |
| The bubble: which buttons pop it | A or B (the manual: "any button") |
| The Hop's directions | the four pad ways as on, across, back; set during a hop for the next one (players' advice [GUIDE], [STEAM-WHAT]) |
| Starting lives | 3, the one in play counted (2 on the counter) |
| Whether the hand survives a lost life | it does |
| Every number in "Readings we had to choose" | as listed |

## Next update (Phase 2): 2-player co-op

The original lets a second player join as the pigeon on the same screen.
For the next update this cartridge will need:

- **Player 2 on the same screen:** a second Clover-sized hero (our own
  pigeon, in its own colour), moved by the second pad (`btn2`, the
  versus/spare input split) with the same moves: eight-way walk, the held
  wand with its locked aim, the dash.
- **Own cards and HUD:** each player's three card slots, powers, helper,
  bombs and spiked balls are their own, shown on their own side of the
  HUD strip (the right-hand box is kept free for it now); drops go to
  whoever picks them up.
- **Lives:** the sources don't say whether lives and continues are shared
  in co-op. Our plan: separate hearts and bubbles, one shared pool of
  lives and continues (a continue restarts the stage for both), so the
  no-continue cherry stays one rule.
- **The view:** it climbs with the leading player and the bottom edge
  pushes the other on, as the Boiler's chase already does; a bubble may
  float anywhere on screen.
- **The Hop:** each pass is still one try; the players take turns.
- **The demo player and tests:** a second bot hand for player 2 and tests
  for the shared rules.

## Tests

`tests/sst_01` … `sst_23` drive each rule with button presses, or set up a
moment with cheats and then play it. The demo player (`sst_bot.c`) taps A
at the nearest foe it has a clear line to (or the boss's soft spot), and
when trouble is near tries about twenty-five moves in a copy of the game
(walking each of eight ways, dashing each of eight ways, each followed by
a walk up the stage's guide, or standing its ground) for 30 frames,
keeping the one that loses nothing, lines up its shots and gets furthest
or hurts the boss most; in the Hop it searches four hops ahead with the
spikes' and mouths' timing. `sst_24` plays from the title through all five
stages and eight bosses to the true ending with no continue; `sst_25`
plays the last stage after a continue to the sad ending.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Elfazar's Hat" (raw page): infobox,
  gameplay, the card table, the bonus game, stages and bosses.
  https://ufo50.miraheze.org/wiki/Elfazar%27s_Hat
- [MH-CHEATS] UFO 50 Wiki, "Cheats": the code from a red, blue and green
  card. https://ufo50.miraheze.org/wiki/Cheats
- [MH-DEBUG] UFO 50 Wiki, "Debug mode": the map at each stage; lives to
  9. https://ufo50.miraheze.org/wiki/Debug_mode
- [MANUALS] Steam guide "The missing manuals - How to play UFO 50 games",
  section 43: the buttons, the dash, the bubble, the cards.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [GUIDE] Steam guide "Elfazar's Hat Guide" (Llamakazi): colours, the
  nine tickets (three each in stages I, III and IV), banking tickets, the
  bonus round, lives to 9, the goals.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3347253495
- [GGC] Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [STEAM-DEMO], [STEAM-WHAT], [STEAM-EH] Steam threads on the attract
  demo, the bonus stage and the cherry.
  https://steamcommunity.com/app/1147860/discussions/0/510696183800318067/
  https://steamcommunity.com/app/1147860/discussions/0/6757179594729823963/
  https://steamcommunity.com/app/1147860/discussions/0/604150365670110300/
- [TVT] TV Tropes recap: every stage's foes and bosses, the endings.
  https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game43ElfazarsHat
- [LIZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 43".
  https://lizstar64.github.io/reviews/2024/10/20/UFO50-43.html
- [STATIC] The UFO 50 Diaries: Elfazar's Hat (fixed formations).
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-elfazars-hat
- [POPCAR] Popcar's Blog, "Reviewing Every Single UFO 50 Game".
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [PONETT] ponett, "My 14 favorite games from UFO 50" (the aim lock).
  https://ponett.dog/blog/my-14-favorite-games-from-ufo-50
- [KAYIN] Kayin, "UFO 50". https://kayin.moe/ufo-50
- [SR] speedrun.com: a 13:08 run.
