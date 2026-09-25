# 09 · BANNERFALL

*Internal design document. Not shown in the product.*

## Tribute to

**Attactics** (UFO 50 game #9, Mossmouth). The rules were researched from
text only: the community wiki, written reviews and forum threads (listed under
Sources). No UFO 50 images, video, sprites, music or text were used as
references.

**Map type: fixed board, random spawns.** Attactics has no hand-made maps:
every battle is played on the same 6 × 8 field between two castles, and units
arrive on random rows. Ours does the same (`bf_spawn`). What changes from
battle to battle is data (flags, unit pools, the enemy's extra units), and we
copy those numbers from the wiki's campaign table one for one.

## Mechanics checklist

| Mechanic | How BANNERFALL does it | Source |
|---|---|---|
| The field | 6 rows × 8 columns, a keep at each end | [W] "six rows and eight columns"; [R2] |
| Two armies | the player is the left army; the CPU is the right | [W] "red army" vs blue; [MM] |
| Spawning | at the start of every turn each side gets one unit in the column next to its keep, on a random row | [W] |
| Turn timer | counts down from 9; while it runs you rearrange your units | [W] |
| Timer speed | one count every half second (ours; the sources only say "a few real-time seconds") | — |
| Repositioning | hold the grab button (A) and use the D-pad: up, down and back freely; forward only as far as the column the unit was picked up from | [W] |
| Swapping | moving onto an ally swaps the two; you can't move onto an enemy | [W] |
| Pushing | swaps can carry a unit from the column by the keep up to the fourth column, three columns forward | [W] |
| Deployment zone | only the four columns nearest your keep can be rearranged | [W] |
| The CPU | never moves its units: they only march; it gets extra units instead | [W] "the blue units opposing them are not moved"; [L] "the AI can't move at all" |
| Fast-forward | holding B speeds the game up (not in 2P Versus) | [W] |
| End of turn, order | 1 knives, 2 other attacks, 3 moves (a clash if two enemies step into the same empty tile), 4 keep attacks, 5 riders and champions move again | [W] "Action Order" |
| Melee | a unit hits the enemy directly in front of it and doesn't move that turn; 1 damage | [W] |
| Clash | two enemies stepping into the same empty tile damage each other as usual and neither moves | [W] |
| Keep attack | a unit that marches into the enemy keep destroys one flag and reappears on its own side | [W] |
| Win / lose | take all enemy flags before they take yours; both losing their last flag at once is a draw | [W] |
| Extra spawn per flag | a side gets one extra unit per turn for every enemy flag it has taken | [W] |
| Handicap | "N% more units": a running total, e.g. 10% = 1 unit for nine turns, 2 on the tenth | [W] |
| Matched spawns | when both unit pools are the same, both sides get the same unit; if the CPU gets several, the player's matches one; a hero turn is the exception | [W] |
| Promotion | a unit that kills and survives is promoted and deals 2 damage (the attack plays twice) | [W] |
| Heroes | every fifth promotion, a Champion replaces that turn's normal spawn; it arrives promoted and doesn't count toward the next one; only player sides get them (campaign 24, ranked, survival, 2P) | [W] |
| Footman (Grunt) | 3 HP; takes no melee damage while part of a column of 3+ allies, unless the attacker is a footman in such a column too | [W] |
| Bowman (Archer) | 2 HP; shoots down its row at the first enemy ahead and doesn't move when it does; melee only when an enemy is right in front | [W] |
| Warden (Shieldman) | 5 HP; while at 4+ HP it doesn't attack and arrows can't hurt it | [W] |
| Rider (Cavalry) | 3 HP; after moving, moves again if the tile ahead (or the keep) is free; the second move can only attack the keep | [W] |
| Pikeman (Spearman) | 3 HP; attacks the two tiles ahead, hitting an enemy on each | [W] |
| Shade (Assassin) | 2 HP; throws knives straight up and down at the nearest enemy each way, and can still move that turn | [W] |
| Powderman (Sapper) | 2 HP; when it dies it explodes for 3 damage in a 2 × 3 area, hitting both sides | [W] |
| Champion (Hero) | 5 HP; the Pikeman's reach and the Rider's second move; arrives promoted | [W] |
| Campaign | 24 battles in order; flags, pools and the CPU's extra units copied from the wiki table | [W] |
| Ranked | endless 3-flag battles with the same full pool; your rank is the CPU's extra-unit percentage; it changes with the flag difference: +10 / +4 / +2 / 0 / −1 / −3 / −5 | [W] |
| Survival | you have 3 flags, the enemy none; 1 point per turn survived, +1 more per turn for every hit on the enemy keep; the enemy spawns more and more; the colours shift from normal to yellow, pink, red and black | [W] |
| 2P Versus | both players rearrange at once; presets Beginner (F F B R), Moderate (+ P W) and Advanced (+ S X), 3 flags each; Custom (each side's pool, Champions allowed, 1–5 flags a side, no handicaps) and Random (a random pool each, no Champions in it, 3 flags); Champions from promotions in every 2P mode | [W] |
| Double / Triple Time | if a battle drags on the timer counts from 6, then from 4 | [W]; the trigger turns are ours |

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
| 15 | Powder Keg | 5/5 | R B F F W P | F F F F R X | 40 |
| 16 | Smoke and Sparks | 5/5 | W X P R | F B R P | 40 |
| 17 | Hollow Woods | 5/5 | S R F F X X | F F R P P W | 40 |
| 18 | Call to Arms | 5/5 | R B F F W X P S | F F B R S P X W | 60 |
| 19 | Crowded Lanes | 5/5 | R B F W W X P S | F B R S P X W W | 60 |
| 20 | Champions Rise | 5/5 | W S B C | F B R S P X | 100 |
| 21 | Plain Steel | 5/5 | F F F F | F F B R S P X W | 20 |
| 22 | Barrels and Bows | 5/5 | X X B B | S S P P | 60 |
| 23 | The Grey Field | 5/5 | R B F F W X P S | F F B R S P X W | 70 |
| 24 | Bannerfall | 5/5 | R B F F W X P S (+ Champions from promotions) | F F B R S P X W | 80 |

Battle 20's player pool includes the Champion as a normal spawn; battle 24
summons Champions from promotions.

### Readings we had to choose

The sources name these rules but don't pin down every detail:

- **Timer speed:** one count per half second, so a turn lasts 4.5 s (3 s in
  Double Time, 2 s in Triple Time). Fast-forward runs it four times faster.
- **Double / Triple Time triggers:** turn 40 and turn 60.
- **Arrows and knives** fly over allies and stop at the first enemy. A Warden at
  4+ HP stops arrows (they hit it and do nothing).
- **Melee** means any attack that isn't an arrow, a knife or a blast (the
  Pikeman's reach counts as melee for the Footman's column rule).
- **Shades** also hit the enemy directly in front, like every unit ("all units
  have a base attack of 1").
- **The Powderman's 2 × 3 blast** covers its own column and the one ahead of
  it, one row up and down. A blast can set off another Powderman.
- **Kills and promotion:** every unit that damaged an enemy in the attack that
  killed it is promoted, if it survives.
- **Full spawn column:** a unit that has no free tile to spawn (or reappear) on
  is lost.
- **Ranked pool:** the full pool of battles 18 and 23 for both sides, with
  Champions from promotions for the player.
- **Survival:** the enemy's extra units grow by 5% per turn; the colour shifts
  come at turns 20, 40, 60 and 80.
- **Riders' second move:** two enemies trying to take the same free tile on
  their second move both stay put.

## What is ours

- **Name:** BANNERFALL (1984, Beamdown Softworks).
- **Setting:** the Marigold March, a meadow between two hill keeps. The player
  leads the **Marigold Guard** (gold tabards) against the **Thistle Host**
  (violet), whose Duke wants the valley's honey.
- **Unit names and designs:** Footman, Bowman, Warden, Rider (on a hill
  pony), Pikeman, Shade, Powderman, Champion.
- **Battle names** (all 24), rank titles and every line of text.
- **Pixel art:** keeps with banners that fall when taken, soldiers, arrows,
  knives, powder blasts.
- **Music:** original title march, battle drums and jingles (UFO-MML).

## Additions: none

Only the platform needs every UFO 40 cartridge has:

- the START pause menu;
- saving: battles won, campaign losses, rank and best survival score;
- the three UFO 40 goals, which replicate Attactics' own three:

| UFO 40 goal | Condition | Attactics' goal |
|---|---|---|
| Beacon | win the first 12 campaign battles | gift: beat the first 12 levels |
| Saucer | win all 24 campaign battles | gold: beat all 24 levels |
| Alien | win the campaign and reach rank 100 | cherry: beat the campaign and reach rank 100 |

On the Vita (one controller) 2P Versus is shown but locked: it needs two
players. On PC and the web it takes two gamepads or a split keyboard.

## Controls

| Input | Action |
|---|---|
| D-pad | move the cursor |
| hold A + D-pad | drag a unit (up, down, back; forward to where you picked it up) |
| hold B | fast-forward (1 player only) |
| START | pause menu |

2P Versus on one keyboard: player 1 WASD + F (grab), player 2 arrows + K
(grab). Two gamepads work too.

UFO 50's Button II (the Z key) grabs and Button I (the X key) fast-forwards;
those keys are our A and B.

## Sources

- [W] UFO 50 Wiki (Miraheze), "Attactics": board, turn order, movement rules,
  every unit, promotions and heroes, spawns and handicaps, the 24-level
  campaign table, ranked, survival and 2P Versus modes, goals.
  https://ufo50.miraheze.org/wiki/Attactics
- [L] Lizstar's Trashcan, "UFO 50 Retrospective Part 9 - Attactics": unit
  behaviours; "the AI can't move at all, so it usually gets like, 70% more
  units". https://lizstar64.github.io/reviews/2024/10/10/UFO50-9.html
- [R2] Popcar's Blog, "Reviewing Every Single UFO 50 Game": troops advance every
  few seconds; the CPU gets more units. https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [ST] Steam thread "Attactics tactics": grunts in columns of three, archers in
  their own lane, spearmen one tile away.
  https://steamcommunity.com/app/1147860/discussions/0/4852155152088697159/
- [MM] Steam guide "The missing manuals - How to play UFO 50 games" (search
  summary): the red army against the blue army.
