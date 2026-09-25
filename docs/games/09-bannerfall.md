# 09 · BANNERFALL

*Internal design document. Not shown in the product.*

## Tribute to

**Attactics** (UFO 50 game #9, Mossmouth). The rules were researched from
text only: the community wiki, written reviews, a Steam guide and forum
threads (listed under Sources). No UFO 50 images, video, sprites, music,
code or text were used as references.

**Map type: fixed board, random spawns.** Attactics has no hand-made maps:
every battle is played on the same 6 × 8 field between two castles, and units
arrive on random rows. Ours does the same (`bf_spawn`). What changes from
battle to battle is data (flags, unit pools, the enemy's extra units), and we
copy those numbers from the wiki's campaign table one for one.

## "The type never changed"

The owner played battle 1 for more than 15 turns and saw only footmen. No
bug in the pools, the random rows, saving or unlocking: a log of every
campaign battle, ranked and survival (`bf_22_spawn_log`) shows each side
spawning exactly its pool. Battle 1 is footmen only on purpose (Attactics'
level 1 is Grunt against Grunt), but ours gave no hint of what it teaches,
showed later battles as "? ? ?", and could stall for dozens of turns because
clashes never resolved (11 of 400 hands-off battles never ended). Now the
wiki's clash rule settles head-on meetings, battles that bring a new troop
open with a line about it (as Attactics' levels do), the map shows the next
battle's name and new troops (battle 2: the bowman), and battles 1 to 3 are
won through the menus with button presses at a human pace
(`bf_23_campaign_presses`). An independent review confirmed the pacing: with
no input at all, battle 1 ends between turns 9 and 22 in 40 seeds, as the
field's width dictates.

## Mechanics checklist

Every line was compared with the code; the tests named prove it with button
presses.

| Mechanic | How BANNERFALL does it | Source | Test |
|---|---|---|---|
| The field | 6 rows × 8 columns, a keep at each end | [W], [P] | bf_01 |
| Two armies | the player is the left army; the CPU is the right | [W], [MM] | bf_01 |
| Spawning | at the start of every turn each side gets one unit in the column next to its keep, on a random row | [W] | bf_14, bf_22 |
| Turn timer | a number counting down from 9; while it runs you rearrange your units | [W] | bf_14 |
| Cursor | without a unit in hand it roams only your own half | [W] | bf_17 |
| Repositioning | hold the grab button (A) and use the D-pad: up, down and back freely; forward only as far as the column the unit was picked up from | [W] | bf_01 |
| Swapping | moving onto an ally swaps the two; you can't move onto an enemy | [W] | bf_01 |
| Pushing forward | swaps can carry a unit from the column by the keep up to the fourth column | [W], [R] | bf_01 |
| The CPU | never moves its units: they only march; it gets extra units instead | [W], [L] | bf_14, bf_22 |
| Fast-forward | holding B speeds the game up (not in 2P Versus) | [W] | bf_22 |
| End of turn, order | 1 knives, 2 other attacks, 3 moves, 4 keep attacks, 5 riders and champions move again | [W], [T] | bf_02, bf_05 |
| Melee | a unit hits the enemy directly in front of it and doesn't move that turn; 1 damage | [W] | bf_02 |
| Clash | two enemies stepping into the same free tile hit each other; then the one with more of its own men lined up behind advances, and an even line holds both | [W] | bf_02, bf_16 |
| Keep attack | a unit that marches into the enemy keep takes one flag and reappears on its own side (one Reddit player remembers the unit used up; we follow the wiki) | [W] | bf_07 |
| Win / lose | take all enemy flags before they take yours; both at once is a draw | [W] | bf_09 |
| Extra spawn per flag | one extra unit per turn for every enemy flag taken | [W] | bf_07 |
| Handicap | "N% more units" as a running total: 10% = 1 unit for nine turns, 2 on the tenth | [W] | bf_14 |
| Matched spawns | in campaign battles with mirrored pools, ranked and mirrored 2P (not survival), both sides get the same unit; if the CPU gets several, the player's matches one; a champion turn is the exception | [W] | bf_14 |
| Promotion | a unit that kills and survives deals 2 (every attack plays twice); if several units dealt the killing blow, all are promoted; promoted health shows as stars | [W], [T] | bf_08 |
| Champions | every fifth promotion a Champion replaces that turn's spawn, with a banner; it arrives promoted and doesn't count toward the next; the counter (up to four) sits on your keep; player sides only | [W] | bf_08, bf_21 |
| Footman (Grunt) | 3 HP; no melee damage in a column of 3+ allies, unless the attacker is a footman in such a column | [W], [L], [ST] | bf_03 |
| Bowman (Archer) | 2 HP; shoots down its row when the first unit ahead is a foe and doesn't move; a friend in front blocks it | [W], [T], [R] | bf_04 |
| Warden (Shieldman) | 5 HP; at 4+ HP it doesn't attack and arrows can't hurt it (the shield drops at 3) | [W] | bf_04 |
| Rider (Cavalry) | 3 HP; after moving, moves again if the tile ahead (or the keep) is free; the second move only attacks the keep | [W] | bf_05 |
| Pikeman (Spearman) | 3 HP; hits the two tiles ahead, one enemy on each | [W], [L] | bf_05 |
| Shade (Assassin) | 2 HP; throws knives up and down at the first unit each way if it is a foe, and still moves | [W], [L] | bf_05 |
| Powderman (Sapper) | 2 HP; dies in a 2 × 3 blast of 3 damage that hits both sides | [W], [T] | bf_06 |
| Champion (Hero) | 5 HP; a pike's reach and a rider's second move; arrives promoted | [W] | bf_08 |
| Campaign | 24 battles in order; flags, pools and extra units from the wiki table; new troops introduced with a line | [W] | bf_09, bf_21, bf_22, bf_23 |
| Campaign map | the next battle to win shows its name and the troops it brings in, still locked; later ones stay hidden (our reading: the sources don't show Attactics' level list) | [W] | bf_24 |
| Ranked | 3 flags each, the same full pool; rank = the CPU's extra-unit percentage, starting at 10, never below 0; changes +10 / +4 / +2 / 0 / −1 / −3 / −5 by flag difference; a title every ten ranks, none below 10 | [W] | bf_12, bf_18 |
| Survival | 3 flags against none, each side drawing its own troops; 1 point a turn, plus 1 for every hit on their keep so far; the enemy brings more and more; the colours go yellow, pink, red, black | [W], [R] | bf_13 |
| 2P Versus | three presets (F F B R; + P W; + S X), 3 flags each, matched spawns; Custom: a pool of places each (d-pad picks a place, A/B turn it through the units, Champion included), flags 1–5 a side below, the banner above starts; Random: a pool each, no Champion, down deals again, no matching; Champions from promotions in every 2P mode | [W] | bf_15, bf_19, bf_20 |
| Faster turns | when a battle drags on the count starts from 6, then from 4 | [W] | bf_14 |
| Goals | win 12 battles; win all 24; win all 24 and reach rank 100 | [W] | bf_10 |

### The campaign table

Flags are player / CPU. "+%" is the CPU's extra units. F Footman, B Bowman,
W Warden, R Rider, P Pikeman, S Shade, X Powderman, C Champion.

| # | Ours | Flags | Player pool | CPU pool | +% |
|---|---|---|---|---|---|
| 1 | Morning Muster | 2/2 | F | F | 0 |
| 2 | Bows at Dawn | 3/3 | B F | F F | 50 |
| 3 | The Long Field | 3/3 | B F | B F | 30 |
| 4 | Rain of Reeds | 3/3 | W F F F | F B B B | 30 |
| 5 | Hoofbeats | 3/3 | R B F F | F F B R | 30 |
| 6 | Full Gallop | 5/5 | R B F F | F F B R | 50 |
| 7 | Last Banner | 1/5 | W R B F | F F B R | 50 |
| 8 | Long Reach | 5/5 | R B F F P P | F F B R W P | 40 |
| 9 | Hedge of Pikes | 5/5 | P P P F F W | F F F F R R | 60 |
| 10 | Mud and Mettle | 5/5 | R B F F W P | F F F B R P | 50 |
| 11 | Night Knives | 5/5 | B B F F P S S R | F F B B R S S P | 40 |
| 12 | Thorn Ridge | 5/5 | P P R R | B B R R | 50 |
| 13 | The Flood | 5/5 | S R B F W P | F F F F F F | 75 |
| 14 | No Rest | 5/5 | R B F F W P S | F F B B R S P | 50 |
| 15 | Lit Fuses | 5/5 | R B F F W P | F F F F R X | 40 |
| 16 | Smoke and Sparks | 5/5 | W X P R | F B R P | 40 |
| 17 | Hollow Woods | 5/5 | S R F F X X | F F R P P W | 40 |
| 18 | Call to Arms | 5/5 | R B F F W X P S | F F B R S P X W | 60 |
| 19 | Crowded Lanes | 5/5 | R B F W W X P S | F B R S P X W W | 60 |
| 20 | Champions Rise | 5/5 | W S B C | F B R S P X | 100 |
| 21 | Plain Steel | 5/5 | F F F F | F F B R S P X W | 20 |
| 22 | Barrels and Bows | 5/5 | X X B B | S S P P | 60 |
| 23 | The Grey Field | 5/5 | R B F F W X P S | F F B R S P X W | 70 |
| 24 | Bannerfall | 5/5 | R B F F W X P S (+ Champions from promotions) | F F B R S P X W | 80 |

Battles 1, 2, 4, 5, 8, 11, 15 and 24 open with a line about their new
troop or rule, as Attactics' levels 1, 2, 4, 5, 8, 11, 15 and 24 do.

### Readings we had to choose

- **Timer speed:** one count per half second (a 4.5 s turn; 3 s and 2 s when
  faster); the sources say only "a few real-time seconds". Fast-forward runs
  four times faster.
- **Faster turns** start at turn 40 and turn 60.
- **Clash lines** count the unbroken run of a side's units straight behind
  its man.
- **The Powderman's 2 × 3 blast** covers its own column and the one ahead,
  one row up and down; a blast can set off another Powderman.
- **Full spawn column:** a unit with no free tile to spawn on is lost.
- **Ranked pool:** the full pool of battles 18 and 23.
- **Survival:** the enemy's extra units grow 5% a turn; the colours change
  at turns 20, 40, 60 and 80.
- **2P pools** have eight places in two rows of four.

## What is ours

- **Name:** BANNERFALL (1984, Beamdown Softworks).
- **Setting:** the Marigold March, a meadow between two hill keeps. The player
  leads the **Marigold Guard** (gold tabards) against the **Thistle Host**
  (violet), whose Duke wants the valley's honey.
- **Unit names and designs:** Footman, Bowman, Warden, Rider (on a hill
  pony), Pikeman, Shade, Powderman, Champion.
- **Battle names** (all 24), rank titles, the preset names (Recruits,
  Regulars, Full Muster), the faster-turn calls (QUICK MARCH!, DOUBLE
  QUICK!) and every line of text.
- **Pixel art:** keeps with banners that fall when taken, soldiers, arrows,
  knives, powder blasts.
- **Music:** original title march, battle drums and jingles (UFO-MML).

## Additions: none

Only the platform needs every UFO 40 cartridge has: the START pause menu,
saving (battles won, campaign losses, rank, best survival score), and the
three UFO 40 goals, which are Attactics' own three (the first 12 battles;
all 24; all 24 and rank 100). On the Vita (one controller) 2P Versus is
shown but locked.

## Controls

| Input | Action |
|---|---|
| D-pad | move the hand (your half only) |
| hold A + D-pad | drag a unit (up, down, back; forward to where you picked it up) |
| hold B | fast-forward (1 player only) |
| START | pause menu |
| 2P setup | D-pad picks a place, A / B turn it, ↓ from the bottom row: flags, ↑ from the top row: the banner; A or START on the banner fights; ↓ deals Random again |

2P Versus on one keyboard: player 1 WASD + F (grab), player 2 arrows + K
(grab). Two gamepads work too. UFO 50's Button II grabs and Button I
fast-forwards; those are our A and B.

## Sources

- [W] UFO 50 Wiki (Miraheze), "Attactics": board, cursor, movement and
  clash rules, turn order, every unit, promotions and heroes, spawns and
  handicaps, the 24-level campaign table and its teaching text, ranked,
  survival, 2P Versus and its setup screen, double and triple time, goals.
  https://ufo50.miraheze.org/wiki/Attactics
- [T] TV Tropes, "UFO 50 Game 9 Attactics" recap: archers "blocked by your
  own troops in front of them", the action order, the 2×3 blast.
  https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game9Attactics
- [R] r/UFO50 threads (read through the Pullpush archive): "archers don't
  fire if you have another unit in front of them", pushing a unit forward by
  swapping, survival scoring.
- [ST] Steam thread "Attactics tactics": grunts in columns of three, archers
  in their own lane.
  https://steamcommunity.com/app/1147860/discussions/0/4852155152088697159/
- [L] Lizstar's Trashcan, "UFO 50 Retrospective Part 9 - Attactics": unit
  behaviours; the AI can't move and gets more units.
  https://lizstar64.github.io/reviews/2024/10/10/UFO50-9.html
- [P] Popcar's Blog, "Reviewing Every Single UFO 50 Game": troops advance
  every few seconds. https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [MM] Steam guide "The missing manuals - How to play UFO 50 games": the red
  army against the blue.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
