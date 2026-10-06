# 34 · BRAVADO

*Internal design document. Not shown in the product.*

## Tribute to

**Overbold** (UFO 50 game #34, Mossmouth, "September 1987"). The rules
were researched from text only: the community wiki's raw page, the Steam
guides "The missing manuals" (section 34), "Overbold Cherry Strategy" and
"Gift, Gold & Cherry", Steam threads, written reviews and TV Tropes
(listed under Sources; the full notes are in the research folder,
`34-overbold.md`). No UFO 50 images, video, sprites, music, arena layout
or text were used as references, and nothing was taken from a UFO 50
install.

**Map type: one fixed arena.** Overbold is "a single arena" [LIZ], the same
screen every round, with four corner pads and lava pools [MM]. Ours is our
own floor, with our own pool spots (the original's are not described
anywhere we could read). The fights themselves are random, as in the
original: which pack each raise adds, how big the packs are, and which
gear is on sale or hiked are rolled each time [MM], [GUIDE], [W].

| Full scale | Overbold | BRAVADO |
|---|---|---|
| Arenas | 1, fixed; centre start, 4 corner pads, lava pools [MM] | 1, the Glass Pit; centre start, 4 corner pads, 3 pools |
| Rounds | 8 [MM], [GUIDE] | 8 fights |
| Round 1 | a fixed 100 tutorial fight [W-CHEAT], [YMMV] | one pack of 9 mites for 100 |
| Rounds 2-7 | 1 to 16 random groups, 100 a group [GUIDE], [MM] | the same: 1 to 16 packs, 100 to 1,600 |
| Round 8 | forced full: 12 random groups, then the only boss, 3,200 [TVT], [YMMV], [MM] | the same: 12 packs and the Pit Boss, 3,200 |
| Group kinds | 7: Cat Larvae, Hover Whale, Bunny Men, Pumpkin Drone, Toucan Spider, Cyclops Flower, Lava [GUIDE] | 7: mites, gasbags, brutes, powder kegs, stilters, peepers, slag |
| Group sizes | Cat Larvae and Hover Whales 9 a group; others random [GUIDE], [TVT] | mites and gasbags 9; brutes 3-6, kegs 3-6, stilters 2-5, peepers 2-4; slag 3 pools |
| Boss | 1, round 8 only [TVT] | the Pit Boss |
| Gear | 16 items in a 4 x 4 grid, 7,900 for everything [W], [MM] | 16 items in a 4 x 4 grid, 7,900 for everything |
| Shop changes | one item +100 (HIKE), one -100 (SALE, never under 50) each visit [W] | the same |
| Goals | 3 [W], [GG] | the same 3 |
| Stats | Most Upgrades, Most Kills [W] | the same two, on the title |
| Most cash possible | 100 + 6 x 1,600 + 3,200 = 12,900 [YMMV] | the same |

## Mechanics checklist

| Mechanic | How BRAVADO does it | Source | Test |
|---|---|---|---|
| Moving | eight ways, 1 px a frame (the same along a diagonal), a little faster than walking monsters (mites 0.8) | [W], [UNFAIR] | brv_02 |
| Shooting | a tap shoots once the way Dice faces; held, it fires every 14 frames and the facing stays put, so she strafes; let go and the pad turns her | [W], [MM], [OB] | brv_03 |
| The gun | weak, reaches the whole arena, a slight shove; a monster is safe for 4 frames after a hit, so a fan doesn't hit one monster twice at once | [TVT], [YMMV], [TIPS] | brv_03, brv_12 |
| Bombs | dropped at her feet, go off after 2.5 s and kill every monster in the blast outright (gasbags without splitting); the boss takes 40; they hurt her too, head included | [W], [MM], [YMMV], [UNFAIR] | brv_04, brv_19 |
| Bomb stock | 1 to start, 3 more a BOMB BAG tier (13 at most), back to full every fight | [W], [Q] | brv_04, brv_16 |
| Health | 6 to start; touches, shots and blasts take 6 (a powder keg's blast 8): one hit and the run is over | [MM], [UNFAIR] | brv_05 |
| Lava | a bite of 1 as she steps in and every half second after (6 health last 2.5 s); monsters walk through it; the pools can be walked round over the top, not along the bottom wall | [GUIDE], [UNFAIR] | brv_05 |
| Death | ends the run: no lives, no continues; back to the title | [MM], [POP] | brv_21 |
| The arena | four corner pads; monsters come mostly from the pads, peepers come up under Dice | [MM], [GUIDE] | brv_10, brv_11 |
| On-screen cap | 12 monsters on the floor at once, the rest queue; slag is not part of it | [GUIDE] | brv_10 |
| Spawn rate | quicker as the prize goes up: 100 frames apart at 100 down to 30 at 900, no quicker after | [TVT], [YMMV] | brv_10 |
| HUD | prize top left, monsters left top right; fight number, health and bombs along the bottom | [MM] | (drawn) |
| Winning a fight | every monster in the lineup beaten; the prize goes into the purse; A to the shop | [MM] | brv_06 |
| The shop | SPEND (into the gear grid), RAISE PRIZE (with the prize next to it) and FIGHT along the bottom; the gear window with the cash beside it, a price under each icon (white if affordable, grey if not), SALE or HIKE over an icon, the description at the very bottom; the next fight's window with each pack's icon and size | [MM] | brv_07, brv_09 |
| Raise | +100 and one random pack, shown only once added; 16 packs and 1,600 at most; not in fight 1 or 8 | [MM], [GUIDE], [TVT] | brv_09, brv_18 |
| Sale and hike | each visit one item costs 100 more and another 100 less (never under 50), for that tier only; the first visit has no hike, and only gear the sale would bring to 100 or less can be on sale | [W], [MM] | brv_06, brv_08 |
| The thank-you | three hiked buys in a run and the house thanks you | [META] | brv_08 |
| Gear prices | HEART PLATE 300/300/350/350, FIRST AID 150/300, BLAST GUARD 100/100, SHOT GUARD 100/100, QUICK TRIGGER 150/250, HEAVY ROUNDS 400/400, FAN FIRE 350/350, RICOCHET 350/250, KICKBACK 200, BUDDY BOT 600/300, FIREWALKERS 150/150, ROCKET DASH 350, BOMB BAG 100/100/150/200, CLICKER 300, NAIL BOMBS 100/200, BAIT BOMBS 350 (7,900) | [W] | brv_07 |
| HEART PLATE | +4 most health a tier (22 at most) | [W] | brv_05 |
| FIRST AID | a medkit every 30 s (15 s at tier 2); it heals 6, past her most health, up to 40 | [W], [TIPS] | brv_14 |
| BLAST / SHOT GUARD | shrug off 1 (2) blasts / shots a fight; never a touch | [W], [GUIDE] | brv_13 |
| QUICK TRIGGER | fire every 10, then every 7 frames | [W] | brv_12 |
| HEAVY ROUNDS | shots of 3, then 4 (a plain shot is 2) | [W] | brv_12 |
| FAN FIRE | 2, then 3 shots at once, fanned out | [W] | brv_12 |
| RICOCHET | shots bounce off 1, then 2 walls | [W] | brv_12 |
| KICKBACK | shots shove monsters 4 px instead of 1.5 | [W], [GUIDE] | brv_12 |
| BUDDY BOT | a drone hovering where Dice was 24 frames ago, doing what she did then: shooting, dropping a bomb (its own, which never hurts her); it soaks up shots and breaks after 3 knocks (6 at tier 2), back next fight | [W], [TVT] | brv_15 |
| FIREWALKERS | the first 1 (2) pools she wades into each fight cool to rock | [W], [Q] | brv_17 |
| ROCKET DASH | double-tap B: a dash the way she faces, safe, over lava, that flattens any monster in the way; 100 off the boss (four finish it) | [W], [TVT], [TIPS] | brv_17, brv_19 |
| CLICKER | B sets her bombs off (once they have been down a moment) | [W] | brv_16 |
| NAIL BOMBS | 8 nails of 4 (14 nails of 8 at tier 2) from every bomb; they never hurt her | [W], [GUIDE] | brv_16 |
| BAIT BOMBS | every monster but the peepers goes for the nearest bomb; stilters still shoot at her | [W], [YMMV] | brv_16 |
| Mites | one shot, chase, 9 a pack | [TVT], [GUIDE] | brv_11 |
| Gasbags | float for a while, then follow; a shot splits one in two, the halves flying off 45 degrees either side of the shot, and the halves split again; a bomb or a dash kills it whole | [TVT], [GUIDE] | brv_04, brv_11 |
| Brutes | slow, tough (7 plain shots), chase | [TVT], [GUIDE] | brv_11 |
| Powder kegs | roll about along the eight ways and charge when Dice is on one of their lines; blow up when they die (8 to her, every monster near) | [TVT], [GUIDE], [OB] | brv_11 |
| Stilters | walk about at a distance and shoot with no wind-up; 8 plain shots | [YMMV], [GUIDE], [Q] | brv_11 |
| Peepers | a tone and a mark, and one comes up right under Dice; it fires slow shots with a flash first, sinks, and comes up under her again; tough (7 shots); a bomb dropped on the tone waits for it | [TVT], [GUIDE] | brv_11 |
| Slag | three more pools for the whole fight | [TVT], [GUIDE] | brv_09 |
| The Pit Boss | last fight only, after every pack; drifts, fires fans of seven, sends out pairs of fizzers (yellow runners that blow up when they die or touch her), and charges; 400 health | [TVT], [Q], [TIPS] | brv_18, brv_19 |
| 2P | both players in the pit at once (our reading, below) | [W], [POKY] | brv_22 |
| Goals | Beacon: win a fight worth 500 or more. Saucer: come out of all eight fights standing. Alien: walk out with 4,500 or more | [W], [GG] | brv_20 |
| No mid-run save | only records are saved | [W] | brv_01, brv_21 |
| Stats | most upgrades and most kills in a run, on the title | [W] | brv_21 |

### Readings we had to choose

- **The dash's button.** The wiki's gear text says "double tap Button 2",
  the bomb button; TV Tropes says double-tap the fire button while moving.
  Both are built (`brv_17` plays each); the wiki's reading is the one in
  the game, since its row reads like the in-game text. With the dash
  owned, a single tap of B drops its bomb a moment later (15 frames), once
  it's clear no second tap is coming, at the spot where B was pressed.
- **2P.** No source says how two players share Overbold; the wiki files it
  as co-op and the title calls it "chicken", and a code's text implies
  normal betting adds enemies in 2P too. Ours: both in the pit at once,
  one purse and one shop that either player works, the same gear for both,
  each with their own health, bombs and guards; a player knocked down sits
  out the rest of the fight and is back for the next, and the run ends
  when both are down. Locked on the Vita (one controller).
- **Bombs:** 1 to start (one player's "10 mines" is 1 + 3 x 3), refilled
  every fight. Fuse 2.5 s, blast radius 30 px.
- **Health between fights:** back to full at the start of every fight.
- **Numbers no source gives:** the on-screen cap (12), the spawn gaps (100
  frames at a prize of 100 down to 30 at 900), every monster's health (a
  plain shot 2: mite 2, gasbag 2 a piece, keg 6, brute 14, peeper 14,
  stilter 16, boss 400), every speed, the medkit's heal (6, up to 40),
  the drone's lag (24 frames) and health (3), the boss's patterns.
- **Pack sizes** for brutes, kegs, stilters and peepers (random within the
  ranges above), chosen so that round 2's sensible 4-6 packs make about
  25-30 monsters, as the cherry guide finds.
- **Fight 1's lineup:** fixed, one pack of nine mites.
- **The boss and its fizzers:** the fizzers don't count among the monsters
  left and don't keep the fight open; beating the boss pops them.
- **The thank-you** for buying hiked gear comes after three hiked buys.
- **Lava walk:** the pool cools to rock for the rest of the fight (players
  describe the charges as deleting a pool).

## What is ours

- **Name:** BRAVADO (1987, Beamdown Softworks).
- **Heroine:** Dice, a fox in a red flight jacket who bet her ship on a pair
  of sevens and lost it to the house on Tombola; her brother Domino is
  player two.
- **The place:** the Glass Pit, its floor, rim, pads and pool spots; the
  back room (the shop).
- **The monsters:** mites, gasbags, brutes, powder kegs, stilters,
  peepers, slag, the Pit Boss and its fizzers.
- **The gear's names and words:** HEART PLATE, FIRST AID, BLAST GUARD,
  SHOT GUARD, QUICK TRIGGER, HEAVY ROUNDS, FAN FIRE, RICOCHET, KICKBACK,
  BUDDY BOT, FIREWALKERS, ROCKET DASH, BOMB BAG, CLICKER, NAIL BOMBS, BAIT
  BOMBS, and every icon.
- **Music:** "All In" (title), "Down on My Luck" (story), "House Money"
  (shop), "The Glass Pit" (fights 1-3), "Double or Nothing" (fights 4-7),
  "Full House" (the last fight), "The Pit Boss", "Cashing Out" (ending),
  and the jingles "Payout" and "Busted".
- **Words:** the story, the shop's lines, the ending, the credits, the goal
  lines and every label.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
UFO 40 goals, which are Overbold's own (gift, gold, cherry). The
original's two terminal codes (a bigger first prize; a gear-staking 2P
variant) are UFO 50 collection features; UFO 40 has no terminal, so they
are left out.

**Owner's change:** the original puts the gun on Button 1 (the manual's B)
and bombs on Button 2 (A). The owner wants the main action on A, the
button players press first, so here A shoots and B drops bombs (and the
dash is B twice).

## Controls

| Input | Action |
|---|---|
| D-pad | move eight ways; turns Dice unless fire is held |
| A (tap) | shoot once the way she faces |
| A (held) | keep shooting the same way while she moves |
| B | drop a bomb at her feet; with CLICKER, set her bombs off |
| B, B | dash, once she owns ROCKET DASH |
| Shop | LEFT / RIGHT along SPEND, RAISE PRIZE and FIGHT, A chooses; in the gear grid the pad picks, A buys, B (or DOWN off the bottom) goes back |
| Title | UP / DOWN 1 PLAYER or 2 PLAYERS, A starts, B to the library |
| START | pause |
| Player 2 | the second pad (or the keyboard's other half), the same buttons |

### Controls not confirmed (flagged)

| Control | Status |
|---|---|
| A shoots, B bombs | **owner's change**: the original has the gun on B (Button 1) and bombs on A (Button 2) [MM], [W] |
| Dash on B, B | **reading**: the wiki says double-tap Button 2 (bombs, our B); TV Tropes says double-tap fire while moving; the second is built but off |
| A single B tap with the dash owned | **ours**: the bomb drops 15 frames later, where B was pressed |
| CLICKER on B | the wiki: "Press Button 2 to remotely detonate bombs" (our B); when a press drops a bomb and when it detonates is ours |
| B / DOWN out of the gear grid | **ours**: the manual only says the d-pad moves and A chooses |
| Raising in fight 1 | **ours**: there is no shop before fight 1, so it can't be raised |
| Player 2's controls | **ours**: no source describes them |

## Not confirmed

- How 2P works (both in one arena, shared purse and gear).
- Every number in "Readings" above: the cap, the spawn gaps, every monster's
  health and speed, the medkit's heal and cap, the drone, the bombs' fuse
  and radius, the boss's health and patterns.
- Whether health comes back between fights.
- Whether fight 1's lineup is fixed.
- The pack sizes of the kinds that aren't nine a pack.

## Tests

`tests/brv_01` … `brv_25` drive every rule with button presses, or set up
a moment with cheats and then play it with presses: the title and fight 1
(01), moving (02), the held aim (03), bombs and the blast on Dice (04),
one-hit health and lava (05), the payout and the first shop visit (06),
every price and buying (07), sale and hike (08), raising and slag (09), the
spawn rate and the cap (10), every monster (11), the gun's gear (12), the
guards (13), medkits (14), the drone (15), bomb gear (16), lava walk and
both dash readings (17), the last fight and the boss (18, 19), the goals
(20), records, saving and goals rebuilt from them (21) and 2P (22). The
demo player (`brv_bot_buttons` in `bravado_bot.c`) only ever answers with
buttons: in a fight it weighs nine moves against where every monster, shot,
bomb and pool will be a moment later, holds fire while its target is in
line and lets go to turn, and bombs crowds and the peepers' tone; in the
shop it follows a buying list and raises to a target. From the title it
wins all eight fights (`brv_23`, raising to 3, 5, 6, 7, 8 and 8 packs), and
on its cherry plan (4, 6, 8, 10, 12, 12, banking 1,300) it walks out with
4,500 or more and all three goals (`brv_24`). A whole run takes it about
nine minutes of game time (the original: about six for a good run, never
more than twenty). `brv_25` checks every shop line fits the screen.

## Sources

- [W] UFO 50 Wiki (Miraheze), "Overbold" (raw page): the controls, the gear
  table with every tier's text and price, the 7,900 total, the shop's
  sale and hike rules, the story, the goals and stats, 2P co-op.
  https://ufo50.miraheze.org/wiki/Overbold
- [W-CHEAT] Miraheze, "Cheats": CASH-BUMP and FAUX-GONE.
  https://ufo50.miraheze.org/wiki/Cheats
- [META] Miraheze, "Meta Messages": the message for buying hiked items.
  https://ufo50.miraheze.org/wiki/Meta_Messages
- [MM] Steam guide "The missing manuals - How to play UFO 50 games",
  section 34: B shoots (held, it locks the direction), A drops a bomb; the
  centre start, the four corner pads, lava pools; the HUD; health 6 and
  hits of 6; the shop's three buttons, the 4 x 4 gear grid, prices in
  white or grey, SALE and HIKE, the Next Fight window, +100 a raise, the
  Last Fight. https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [GUIDE] Steam guide "Overbold Cherry Strategy": 1 to 16 groups at 100
  each, random group sizes, the on-screen cap, lava outside it and walkable
  round the top only, every enemy's behaviour, shields vs health, round 2
  at 25-30 enemies. https://steamcommunity.com/sharedfiles/filedetails/?id=3363908239
- [GG] Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [Q] Steam thread "Overbold Question".
  https://steamcommunity.com/app/1147860/discussions/0/592888028698025972/
- [TIPS] Steam thread "overBOLD tips and tricks?".
  https://steamcommunity.com/app/1147860/discussions/0/691996377956427147/
- [UNFAIR] Steam thread "Overbold honestly feels unfair".
  https://steamcommunity.com/app/1147860/discussions/0/4700161643034794437/
- [OB] Steam thread "Overbold".
  https://steamcommunity.com/app/1147860/discussions/0/4849903998513480673/
- [TVT] TV Tropes, "UFO 50 Game 34 Overbold" (recap).
  https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game34Overbold
- [YMMV] TV Tropes, YMMV page: the spawn-rate cap at 900, round 8 at 3,200,
  12,900 at most. https://tvtropes.org/pmwiki/pmwiki.php/YMMV/UFO50Game34Overbold
- [LIZ] https://lizstar64.github.io/reviews/2024/10/17/UFO50-34.html
- [POP] https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [POKY] https://pokyfriends.com/blog/post/ufo50-games-ive-been-playin/ ("2-P chicken")
- [STATIC] https://staticcanvas.substack.com/p/the-ufo-50-diaries-overbold
