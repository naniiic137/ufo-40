# 02 · GRUB SHIFT

*Internal design document. Not shown in the product.*

## Tribute to

**Bug Hunter** (UFO 50 game #2, Mossmouth). The rules were researched from
text only: the community wiki, written reviews, Steam guides and forum
threads (listed under Sources). No UFO 50 images, video, sprites, music or
text were used as references.

**Map type: random.** Bug Hunter deals new ground for its jobs, so ours does
too (`gs_new_contract`, `deal_terrain`). Generator rules:

| Rule | Value | Source |
|---|---|---|
| Field size | 6 wide × 5 deep | [R], [WP] |
| Ground each morning | 1 sinkhole and 4-6 planters; 2 sinkholes on contracts 2, 5, 8...; 7-8 planters on 3, 6, 9... | [W] job table |
| A new sinkhole | never under Tilly, a grub or a pod | ours |
| Grown colours at the start | none until contract 9; one on 10-12; two from 13 | [W] job table |
| Past contract 15 | contracts 13-15 repeat | [W] |
| Grubs | 5 larvae at the start, 4 more each morning, on free tiles | counts ours |
| Pods | 3 at the start, 4 each morning, on distinct random tiles | counts ours |
| Tilly's start | a random tile away from the walls | ours |

## Mechanics checklist

| Mechanic | How GRUB SHIFT does it | Source |
|---|---|---|
| Turn-based grid, one hero | Tilly on a 6×5 field of floor, planters and holes; all thinking time is free | [W], [R] |
| Controls | the d-pad moves the cursor; A picks a tool, then its target; B cancels | [MM] |
| Seven slots | seven tool slots; each tool works once per shift | [W], [M], [L] |
| Starting kit | 4 × ROLL (Run), SKIP (Hop), ZAP (Shoot), TOSS (Lob) | [W], [M], [L] |
| Rest | CLOCK OUT ends the shift at any time: tools recharge, pods fall, grubs grow, new grubs hatch | [W], [MM] |
| Shop | always open: DOWN on a tool opens it for that slot, A buys | [MM] |
| Shop offers | eight: five cost 2, one costs 3, two cost 4 (the 5-cost SHOO sits with the 4s) | [M], [S] |
| Buying | the new tool overwrites the slot and works at once, even over a spent one | [M], [S] |
| Restocking | a bought offer is replaced at once by another tool of its price; a new shop each shift | [S], [CH] |
| Energy | pods are picked up by rolling onto or across them; hopping over doesn't | [W], [M] |
| Pod stacks | two to a tile; a third (from the night's fall or SEED) blows the tile into a sinkhole and blasts its 8 neighbours | [W], [S] |
| Shot pods | a hit pod explodes: its 8 neighbours at the same height; chains | [W] |
| Pods block hatching | no grub hatches on a pod | [S] |
| Rolls | continuous: can't climb; shove a grub one tile per step, not into higher ground or another grub; a grub shoved into a pit, a pod or a sprayed tile dies | [W], [S] |
| Squash | rolling down off a planter onto a grub squashes it, armour or not, and the roll goes on | [W], [S] |
| Hops | land on a free tile at any height, don't shove | [W] |
| Shots | a straight shot hits every tile to its target; downhill fine, uphill blocked | [W], [S] |
| Lobs | hit one tile, any height | [W] |
| Blasts | only reach tiles at the pod's height; being high is safe | [W] |
| Aiming preview | aiming shows every tile the attack will reach, chains and sparks included | [L] |
| Quota | 30 grubs per contract | [S], [L] |
| Shifts | 10 on contracts 1-3, 9 on 4-6, 8 after | [W], [S] |
| Growing | a colour grows when its larvae survive a night: they become adults and its grubs hatch as adults from then on. One colour a night, two per contract | [W], [S] |
| Ageing | each night every adult becomes a queen and every queen an egg (not on the last night) | [W], [S], [ST] |
| Eggs | an egg left at the end of a shift hatches: you're fired | [W] |
| Eggs don't count | nor do drones | [W] |
| Losing | an egg hatches, the quota isn't met, or Tilly is caught by a blast, sparks or her own attack | [W] |
| Colours | three larva colours, each growing into one of a pair of species; which one flips each contract | [W], [D] |
| Abilities | only attacks and blasts trigger them (not squashes, pits, pods or sprays) | [W] |
| Sparkmite (Shreknid) | popped, sprays sparks 2 tiles each way at its height; higher ground stops them | [W] |
| Hivebug (Dragonfloosh) | grows up with a drone; while it's beside it the hivebug can't be hurt, squashed or popped by OVERDRIVE, but can be shoved (a pit kills it); a drone shoved away dies and doesn't count | [W], [D] |
| Moundmaker (Feloris) | when it appears and every morning, raises its own tile and lowers the planters beside it, unless another moundmaker or a pod is on them | [W], [FD] |
| Sourmite (Quasnar) | spoils the pods beside it (so no pod blast can reach it); when it dies they turn back into pods | [W] |
| Sour pods | picking one up costs energy (not below 0); shooting one adds another; three become an egg; a grub shoved into one grows a stage, a drone becomes a larva, an egg hatches | [W] |
| Shellbug (Armodon) | one extra hit against attacks and blasts | [W] |
| Burrower (Cephalug) | popped by an attack or blast, leaves a hole | [W] |
| Streak | a won contract leads to the next; a lost one ends the streak | [W], [S] |

### The 41 tools

Costs and effects follow the wiki's module list [W]; the names are ours.

| Ours | Bug Hunter | Cost | Effect |
|---|---|---|---|
| ROLL | Run | 2 | roll 1-2 tiles, 4 directions |
| SCURRY | Dash | 2 | roll 1-2 tiles, 8 directions |
| STREAK | Sprint | 2 | roll any distance, 4 directions |
| RUSH | Zoom | 4 | roll any distance, 8 directions |
| SKIP | Hop | 2 | hop to a free tile 1-2 away, 4 directions |
| LEAP | Jump | 2 | hop 1-2 away, 8 directions |
| BOUND | Vault | 4 | hop anywhere free in the 5×5 around Tilly |
| BLINK | Warp | 3 | beam onto a pod, sour or not, and pick it up |
| PERCH | Transport | 3 | beam onto a free planter |
| DIVE | Tunnel | 3 | beam next to a hole |
| HUSTLE | Maneuver | 4 | a ROLL that recharges when a grub dies |
| ZAP | Shoot | 2 | shot 1-2 tiles, 4 directions |
| ARC | Blast | 2 | shot 1-2 tiles, 8 directions |
| BEAM | Snipe | 2 | shot across the field, 4 directions |
| FLARE | Annihilate | 4 | shot across the field, 8 directions |
| TOSS | Lob | 2 | one tile 1-2 away, 4 directions |
| PITCH | Throw | 2 | one tile 1-2 away, 8 directions |
| MORTAR | Launch | 4 | one tile in the 5×5 |
| IGNITE | Detonate | 4 | set off any pod |
| QUAKE | Erupt | 4 | hit the 8 tiles around a hole, any height |
| PULSE | Radiate | 3 | hit the 8 tiles around Tilly |
| HAIL | Bombard | 4 | hit every planter (Tilly too if she's on one) |
| CRACK | Burst | 3 | blow up an egg |
| TRACK | Engage | 4 | a ZAP that recharges when Tilly changes height |
| SEED | Resupply | 2 | drop a pod on any free tile |
| TILL | Shift | 2 | lower one planter, raise another tile |
| OVERDRIVE | Power Up | 4 | for the shift, grubs Tilly touches die, no abilities |
| GATHER | Collect | 2 | collect a pod and every pod around it |
| RELOAD | Barrage | 4 | recharge all attacks |
| REFUEL | Accelerate | 4 | recharge all moves |
| BOOST | Maximize | 4 | for the shift, straight tools reach all the way, in 8 directions |
| BORE | Dig | 4 | open a hole next to a hole; a grub there dies, no ability |
| VOLATILE | Intensify | 4 | for the shift, grubs killed by attacks explode |
| REWIND | Devolve | 3 | any grub (or egg) and its neighbours become larvae |
| MIST | Spray | 2 | mist two free tiles for the contract |
| RESTOCK | Replenish | 4 | four new pods fall (a third on a tile blows) |
| JUMPER | Recharge | 4 | recharge the tools either side of it |
| FLIP | Invert | 2 | swap every tile's height |
| HOVER | Traverse | 2 | for the shift, moves ignore height |
| SIGHT | Aim | 2 | for the shift, attacks ignore height |
| SHOO | Scatter | 5 | every grub steps to a tile beside it (8 ways); a pit or pod there kills it |

### Readings we had to choose

- Ground each morning: "some terrain spawns naturally at the beginning of
  the day, including one hole and a number of platforms" [W] is read as a
  fresh deal every day (a pile-up would fill 30 tiles in days).
- Growth: a colour grows to adults once; the grubs then age nightly.
- Nightly pods fall on distinct tiles, never on Tilly or a grub.
- BOOST adds diagonals; VOLATILE counts kills; CRACK is a normal blast.
- Numbers of grubs and pods per morning (sources give none).

## What is ours

- **Name:** GRUB SHIFT (1983, Beamdown Softworks).
- **Hero:** Tilly, a pint-sized tiller robot on the night shift.
- **Setting:** the glass domes of Orchard Station, a farm on a comet. Days are
  *shifts*, jobs are *contracts*, energy cubes are **fizz pods**, anti-energy
  is **sour pods**, and resting is **clocking out**.
- **Tool names** and every description line.
- **Species designs** (names, sprites, colours): Sparkmite and Hivebug (gold),
  Moundmaker and Sourmite (leaf), Shellbug and Burrower (sky).
- **The certificate:** Employee of the Month, from Orchard Station.
- **Music:** the original "Night Shift" groove, briefing theme and jingles.

## Additions: none

Only the platform needs every UFO 40 cartridge has:

- the START pause menu (resume, restart, controls, quit);
- saving: the contract in progress and the streak are saved, and CONTINUE
  SHIFT resumes them;
- the three UFO 40 goals, which replicate Bug Hunter's own three:

| UFO 40 goal | Condition | Bug Hunter's goal |
|---|---|---|
| Beacon | finish a contract | gift: complete your first job |
| Saucer | 3 contracts in a row | gold: complete 3 jobs in a row |
| Alien | 6 contracts in a row | cherry: complete 6 jobs in a row |

Not included: Bug Hunter's 2P versus. The sources give only its opening
text, not how its turns, shop or scoring work.

## Controls

| Input | Action |
|---|---|
| D-pad | move the cursor over the tools (and CLOCK OUT) or the field |
| A | pick a tool / confirm a target / buy |
| B | cancel / close the shop |
| DOWN on a tool | open the shop to replace that tool |
| START | pause menu |

## Testing hooks

The rules live in one plain `Board` struct, and every action is a pure
function of it. Headless tests set up exact situations with cheats (`bug`,
`pods`, `sour`, `elev`, `hole`, `chip`, `use`) and check the results; others
play through the UI with real buttons. A bot that looks two moves ahead and
breaks eggs first plays whole contracts as a balance check (it wins about
half of contract 1).

## Sources

- [W] UFO 50 Wiki (Miraheze), "Bug Hunter": module list, costs and texts,
  species, anti-energy, pods, terrain, growth, the job table, goals.
  https://ufo50.miraheze.org/wiki/Bug_Hunter
- [CH] UFO 50 Wiki, "Cheats": ACES-SHOP ("buying modules keeps them in the
  shop, instead of replacing them with new ones... stays that way for the
  entire day"). https://ufo50.miraheze.org/wiki/Cheats
- [S] Steam guide "UFO 50 - Bug Hunter Strategy Guide": a squash doesn't end
  the move; shoving and what blocks it; pods falling at the start of each day
  can blow next to a tile holding two; a bought module is replaced by one of
  similar cost; 30 kills in 10 days, then 9 and 8.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3337645343
- [M] MoeGamer, "UFO 50: Strategy meets deckbuilding in Bug Hunter": the
  starting hand, the hand of seven, the shop's five/one/two, using a bought
  module at once.
  https://moegamer.net/2024/09/21/ufo-50-strategy-meets-deckbuilding-in-bug-hunter/
- [MM] Steam guide "The missing manuals - How to play UFO 50 games":
  highlight, A to select and to fire, B to cancel; one use a day.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [L] Lizstar, UFO 50 #2: seven actions, 30 kills in ten days, the
  explosion preview with chain reactions.
  https://lizstar64.github.io/reviews/2024/10/04/UFO50-2.html
- [D] Steam thread "Bug Hunter advice?": colours alternate species by job,
  shoving a hivebug away from its drone, spray.
  https://steamcommunity.com/app/1147860/discussions/0/694249731368441297/
- [ST] Static Canvas, "The UFO 50 Diaries: Bug Hunter": the longer they
  survive the more they evolve. https://staticcanvas.substack.com/p/the-ufo-50-diaries-bug-hunter
- [FD] Search summary of the fandom wiki: "on their turn, all Felorises raise
  their ground".
- [R] Review summary found by search: "a six by five grid".
- [WP] Wikipedia, "UFO 50": the 5×6 grid. https://en.wikipedia.org/wiki/UFO_50
