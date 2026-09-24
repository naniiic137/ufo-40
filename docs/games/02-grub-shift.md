# 02 · GRUB SHIFT

*Internal design document. Not shown in the product.*

## Tribute to

**Bug Hunter** (UFO 50 game #2, Mossmouth). The rules were researched from
text only: the community wiki, written reviews, Steam guides and forum
threads (listed under Sources). No UFO 50 images, video, sprites, music or
text were used as references.

**Map type: random.** Bug Hunter builds a new field for every job, so ours
does too (`gs_new_contract`). Generator rules, all from the sources where they
give them:

| Rule | Value | Source |
|---|---|---|
| Field size | 6 wide × 5 deep | [R] "a six by five grid" |
| Raised platforms | 4 on contract 1, one more per contract up to 8, grown in small clumps | [W] "4-8 depending on job"; clumping is ours |
| Holes at the start | 1 or 2, never next to Tilly | [W] "1-2" |
| A new hole | 1 every night, on an empty tile | [W] "one natural hole spawns per day" |
| Grubs at the start | 5 larvae of random colours (ours; not in the sources) | — |
| Grown grubs at the start | none until contract 9; 1 on contracts 10-12; 2 on 13-15; then 13-15 repeat | [W] job table |
| Larvae each night | 4, never on holes, sprays, pods or Tilly (count is ours) | [W] "spawns new bugs" |
| Pods each night | 4, never on Tilly, never a third on one tile (count is ours) | [W], [M] |
| Tilly's start | a random tile away from the walls (ours) | — |

## Mechanics checklist

Every rule below is in the game. **Source** says where it comes from; "ours"
marks a value the sources don't give, which we had to choose.

| Mechanic | How GRUB SHIFT does it | Source |
|---|---|---|
| Turn-based grid, one hero | Tilly on a 6×5 field of floor, planters and holes | [W], [R] |
| Seven module slots | seven tool slots; each tool works once per shift | [M], [S] |
| Starting kit | 4 × ROLL (Run), SKIP (Hop), ZAP (Shoot), TOSS (Lob) | [S], [M] |
| Rest | ends the shift: tools recharge, pods fall, grubs grow and hatch | [W], [MM] |
| Energy | pods are collected by moving over them, and spent on tools | [W], [M] |
| Two pods per tile, a third explodes | the overloaded tile becomes a hole and blasts its 8 neighbours | [W] |
| Chains | an attack on a pod tile sets it off, and blasts set off pods beside them | [W], [M] |
| Blasts stay level | a blast only reaches tiles at the pod's height | [W] "same elevation only" |
| Shop | 8 offers: five cost 2, one costs 3, two cost 4 (the 5-cost SHOO sits with the 4s) | [M], [S]; SHOO's slot is ours |
| Buying | any time; the new tool replaces a slot of your choice and works at once | [M] |
| Shop refresh | a new set of offers every shift (ours) | — |
| Continuous moves | collect every pod they pass, push a grub one tile, can't climb | [W] |
| Pushing | a grub pushed into a hole dies; a push that's blocked isn't allowed | [W] |
| Stomp | rolling down off a planter onto a grub squashes it | [W], [SC] |
| Hops | ignore height and don't push | [W] |
| Shots | a continuous shot hits every tile on its line; higher ground blocks it | [W] |
| Lobs | hit one tile and ignore height | [W] |
| Quota | 30 grubs per contract | [S], [R] |
| Shifts per contract | 10 on contracts 1-3, 9 on 4-6, 8 from 7 | [W], [S] |
| Losing | caught in a blast or sparks, an egg survives a shift, or the quota isn't met | [W] |
| Eggs | a queen becomes an egg overnight (not on the last night); an egg left at the end of the next shift hatches and you're fired | [W], [M] |
| Eggs don't count | breaking an egg doesn't add to the quota | [W], [SC] |
| Growing | one colour grows each night; only two colours grow per contract | [W] |
| Colours and species | three larva colours; each grows into one of a pair of species, and which one flips from contract to contract | [D], [W] |
| Stomps, pits, sprays skip abilities | only attacks and blasts trigger a grub's ability | [W] |
| Sparkmite (Shreknid) | adult and queen spray sparks 2 tiles in the 4 directions when popped | [W] |
| Hivebug (Dragonfloosh) | grows up with a drone; can't be hurt while a drone is beside it; a drone pushed away dies; drones don't count | [W] |
| Moundmaker (Feloris) | on growing up raises its own tile and lowers the planters beside it, once | [W] |
| Sourmite (Quasnar) | turns pods next to it sour; blasts can't kill it | [W] |
| Sour pods (anti-energy) | collecting one costs energy (not below 0); shooting one adds another; three become an egg; a grub pushed into one grows a stage for good; an egg pushed into one hatches | [W] |
| Shellbug (Armodon) | one extra hit against attacks and blasts; stomps and pits still kill | [W] |
| Burrower (Cephalug) | popped by an attack or blast, it leaves a hole | [W] |
| Spray | a sprayed tile never hatches a larva, and a grub pushed onto it dies; spraying every tile voids the contract | [D] |
| Streak | winning moves you to the next contract; losing starts again at contract 1 | [S] |
| Three in a row | the Employee of the Month certificate (Bug Hunter's "Hunter of the Week") | [S] |
| Difficulty loop | past contract 15, contracts 13-15 repeat | [W] |

### The 41 tools

Costs and effects follow the wiki's module list [W] one for one; the names are
ours.

| Ours | Bug Hunter | Cost | Effect |
|---|---|---|---|
| ROLL | Run | 2 | continuous 1-2 tiles, 4 directions |
| DASH | Dash | 2 | continuous 1-2 tiles, 8 directions |
| STREAK | Sprint | 2 | continuous, any distance, 4 directions |
| RUSH | Zoom | 4 | continuous, any distance, 8 directions |
| SKIP | Hop | 2 | hop to a free tile 1-2 away, 4 directions |
| LEAP | Jump | 2 | hop 1-2 away, 8 directions |
| VAULT | Vault | 4 | hop anywhere in the 5×5 around Tilly |
| WARP | Warp | 3 | hop onto a pod |
| PERCH | Transport | 3 | hop onto a planter |
| DIVE | Tunnel | 3 | hop next to a hole |
| HUSTLE | Maneuver | 4 | a ROLL that recharges when a grub dies |
| ZAP | Shoot | 2 | shot 1-2 tiles, 4 directions |
| ARC | Blast | 2 | shot 1-2 tiles, 8 directions |
| BEAM | Snipe | 2 | shot across the field, 4 directions |
| FLARE | Annihilate | 4 | shot across the field, 8 directions |
| TOSS | Lob | 2 | one tile 1-2 away, 4 directions |
| LOB | Throw | 2 | one tile 1-2 away, 8 directions |
| MORTAR | Launch | 4 | one tile in the 5×5 |
| DETONATE | Detonate | 4 | set off any pod |
| QUAKE | Erupt | 4 | hit the 8 tiles around a hole, any height |
| PULSE | Radiate | 3 | hit the 8 tiles around Tilly |
| HAIL | Bombard | 4 | hit every planter; only from the ground |
| CRACK | Burst | 3 | blow up an egg |
| TRACK | Engage | 4 | a ZAP that recharges when Tilly changes height |
| SEED | Resupply | 2 | drop a pod |
| SHIFT | Shift | 2 | lower one planter, raise another tile |
| OVERDRIVE | Power Up | 4 | for the shift, grubs Tilly touches die outright, no abilities |
| GATHER | Collect | 2 | collect a pod and every pod around it |
| RELOAD | Barrage | 4 | recharge all attacks |
| REFUEL | Accelerate | 4 | recharge all moves |
| BOOST | Maximize | 4 | for the shift, tools reach as far as they can |
| DIG | Dig | 4 | open a hole next to a hole |
| VOLATILE | Intensify | 4 | for the shift, grubs popped by attacks explode |
| DEVOLVE | Devolve | 3 | a grub and its neighbours become larvae |
| SPRAY | Spray | 2 | spray two tiles |
| RESTOCK | Replenish | 4 | four new pods fall |
| RECHARGE | Recharge | 4 | recharge the tools on either side of it |
| FLIP | Invert | 2 | swap every tile's height |
| HOVER | Traverse | 2 | for the shift, moves ignore height |
| SIGHT | Aim | 2 | for the shift, attacks ignore height |
| SHOO | Scatter | 5 | every grub moves to a free tile beside it |

### Readings we had to choose

The sources name these rules but don't pin down every detail:

- **Burst (CRACK)** "explodes an egg": the egg bursts with a normal blast
  around its tile.
- **Intensify (VOLATILE)** "attacked bugs explode": grubs *killed* by attacks
  explode.
- **Power Up (OVERDRIVE)**: rolls go straight through grubs, and hops may land
  on one, killing it.
- **Resupply, Devolve, Launch**: range is the 5×5 around Tilly, like Vault.
- **Which colour grows each night**: the two growing colours take turns.
- **Quasnar**: its "can't be killed by cube explosions" is read as immunity to
  blasts.

## What is ours

- **Name:** GRUB SHIFT (1983, Beamdown Softworks).
- **Hero:** Tilly, a pint-sized tiller robot on the night shift.
- **Setting:** the glass domes of Orchard Station, a farm on a comet. Days are
  *shifts*, jobs are *contracts* and energy cubes are **fizz pods**. Anti-energy
  is **sour pods**.
- **Tool names** and every description line.
- **Species designs** (names, sprites, colours): Sparkmite and Hivebug (gold),
  Moundmaker and Sourmite (leaf), Shellbug and Burrower (sky).
- **The certificate:** Employee of the Month, from Orchard Station.
- **Music:** original "Night Shift" groove, briefing theme and jingles.

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

Removed in the faithfulness audit: the old "squash 4 grubs in one action"
Beacon, the "best combo" and "contracts done" stats, the how-to-play pages,
the "growing / stunted" hints on the briefing, and the rest confirmation.

## Controls

| Input | Action |
|---|---|
| D-pad | move the cursor between tool slots, the shop and the field |
| A | pick a tool / confirm a target / buy |
| B | cancel |
| START | pause menu |

## Testing hooks

The rules live in one plain `Board` struct, and every action is a pure
function of it. The headless tests set up exact situations with cheats
(`bug`, `pods`, `sour`, `elev`, `hole`, `chip`, `use`) and check the results.
A greedy autoplay bot tries every tool and target on copies of the board. We
use it as a balance smoke test and to prove a whole contract can be won.

## Sources

- [W] UFO 50 Wiki (Miraheze), "Bug Hunter": module list and costs, bug
  species and abilities, anti-energy, the job table, energy and holes, goals.
  https://ufo50.miraheze.org/wiki/Bug_Hunter
- [M] MoeGamer, "UFO 50: Strategy meets deckbuilding in Bug Hunter": the
  shop's five/one/two offers, swapping a tool in and using it at once, the
  starting hand. https://moegamer.net/2024/09/21/ufo-50-strategy-meets-deckbuilding-in-bug-hunter/
- [S] Steam guide "UFO 50 - Bug Hunter Strategy Guide" (search summary): 4
  Run + Hop + Shoot + Lob, 30 kills in 10 days, Hunter of the Week after three,
  9 days after that and 8 after six.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3337645343
- [D] Steam thread "Bug Hunter advice?": colours alternate species by job,
  spray kills pushed bugs, spraying every tile loses.
  https://steamcommunity.com/app/1147860/discussions/0/694249731368441297/
- [SC] Static Canvas, "The UFO 50 Diaries: Bug Hunter", and the Steam thread
  "Bughunter Tips/Strategies?": stomping from height, eggs don't count.
- [R] Review summary found by search: "a six by five grid"; "ten days to kill
  thirty bugs".
- [MM] Steam guide "The missing manuals - How to play UFO 50 games": Rest via
  the hourglass, one use per module per day.
