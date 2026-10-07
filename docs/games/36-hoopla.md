# 36 · HOOPLA

*Internal design document. Not shown in the product.*

## Tribute to

**Hyper Contender** (UFO 50 game #36, Mossmouth, "January 1988"). The rules
were researched from text only: the community wiki's raw page and its
Cheats, Terminal, Debug mode and Meta Messages pages, the Steam guide "The
missing manuals" (section 36), a Steam cherry guide and threads, written
reviews and the TV Tropes recap (listed under Sources; the full notes are in
the research folder, `36-hyper-contender.md`). No UFO 50 images, video,
sprites, music, arena layouts or text were used as references, and nothing
was taken from a UFO 50 install.

**Map type: fixed.** The original's arena layouts are fixed and only their
order and colours are drawn at random each match [W]; so are ours
(`hoop_arenas.c`, all twelve our own layouts).

| Full scale | Hyper Contender | HOOPLA |
|---|---|---|
| Fighters | 8, each with its own maneuver, weapon, alt weapon and melee [W] | 8: TANSY, CLAMP, MOSS, BRISTLE, PEWIT, COLLIER, ASTRA, GULP |
| Ways to move | double jump, claw grapple, gravity flip, mines, wings, elevator, jetpack, charge jump [W] | the same eight: double jump with a spin, claw line, gravity turned over, spring traps, wings, a cage lift, a rocket pack, a charged leap |
| Colours | two per fighter (P1 and P2), the clone in the mirror match wears the other [TVT], [STEAM-ALL] | two per fighter, picked on the select screen; the mirror opponent wears the other |
| Arenas | fixed layouts in a random order and palette; debug mode: "only 1-12 are unique" [W], [DEBUG] | 12 layouts, 6 palettes, both drawn at random every match (never the same layout twice running); three have moving ledges |
| Modes | Tournament, Draft Battle (1P or 2P), Exhibition (2P), Options [W], [MM] | the same four |
| Tournament | all 8 in a random order, the last a mirror match; one rematch per opponent [W], [TVT] | the same |
| Draft | snake draft of 3 each (P1, P2, P2, P1, P1, P2) from 7; first to 5 match wins [W], [TVT] | the same |
| Hoops | start with 1, one spawns at the start and every 15 s, on fire at first; 5 to win [W], [MM] | the same |
| Endings | one per fighter, a note after the second-colour Donkus's [W], [MH-META] | one per fighter (ours), a note after GULP's in his second colours |
| Goals | 3 [W], [GGC] | the same 3 |
| Stats | Characters Used, Least Rematches Used to Win [W] | FIGHTERS USED, CHAMPIONS and FEWEST REMATCHES on the title |
| Tunes | the select theme rearranges Kick Club's boss music [TVT] | 9 loops and 3 jingles, all ours; the select and ladder theme rearranges HAT TRICK's boss tune "Cup Final" (cartridge 11, our Kick Club tribute) |

## Mechanics checklist

| Mechanic | How HOOPLA does it | Source | Test |
|---|---|---|---|
| The pit | one screen; walls, a floor and a ceiling; one-way ledges you jump up through and land on | [W], [MM] | hoop_17 |
| Walk, block | the d-pad walks; DOWN held on the ground blocks (CLAMP can block hanging from his line) | [W], [MM], [GUIDE] | hoop_07 |
| Drop through | DOWN + the move button on a ledge | [W], [MM] | hoop_17 |
| Weapon, alt, melee | the weapon button shoots; UP + it the alt shot; DOWN + it the lunge | [W], [MM] | hoop_09 … hoop_16 |
| The exceptions | CLAMP's and GULP's melee is on the move button (the claw's first swing, the glowing leap), so DOWN + weapon is just their weapon; BRISTLE's traps are on the move button | [W] | hoop_10, hoop_12, hoop_16 |
| Hoops, not health | everyone starts with one; hold five at once to win | [W], [MM] | hoop_03, hoop_06 |
| New hoops | one at a random spot at GO and every 15 s after; the count to the next is bottom centre | [W], [MM] | hoop_03 |
| On fire | a new hoop burns for a while; touching it is a hit | [W], [MM] | hoop_04 |
| A hit | a short stun and one hoop knocked out to bounce round the pit at speed; the first to touch it has it | [W], [MM] | hoop_05 |
| Dizzy | a lunge into a block leaves the attacker dizzy; a hit on a dizzy fighter knocks out two | [W] | hoop_05, hoop_07 |
| Safe after a hit | a fighter can't be hit again for a moment, but can still take hoops | [GUIDE] | hoop_04 |
| The triangle | the lunge beats shots (it goes through them), shots beat the block, the block beats the lunge | [W], [TVT] | hoop_07, hoop_08 |
| When shots meet | most pass (GULP's aimed dart too); GULP's fast dart knocks every other shot out except COLLIER's charge ("Donkus's alt-fire darts outprioritize all projectiles except Gilroy's bombs"), and a charge in hand shields him | [W] | hoop_14, hoop_18 |
| TANSY | double jump of variable height; the second is a spin that turns any knife (hers too) away diagonally; the knife bounces off two walls and can cut her on the way back; the alt flies a little and drops, and can't cut her | [W] | hoop_09 |
| CLAMP | a claw line that bites ledges and the ceiling, reels in and swings; a diagonal line's first swing is a melee; the claw takes hoops it passes; B again lets go with a hop; mid-swing he blocks for free (a lunge into him leaves the lunger dizzy), and DOWN blocks while hanging; a cog out straight and back below (the alt: back above), hitting both ways | [W], [STEAM-ALL], [GUIDE] | hoop_10 |
| MOSS | turns his own gravity over at will and walks on the ceiling and under ledges; left and right stay as they are unless the option turns them over too; a rocket that curves a little with his gravity and bursts on anything but a ledge; the alt doesn't burst | [W], [MM] | hoop_11 |
| BRISTLE | spring traps that arm in 1 s; armed, her own throws her high and anyone else's foot (another BRISTLE's too) sets it off on them; three quills, short, up on the ground and down in the air, stopping her in the air; a long lunge that ignores gravity; no jump (the traps are her only Button 2 move [W]; TV Tropes' Rocket Jump) | [W], [TVT] | hoop_12 |
| PEWIT | jumps of variable height on the ground, flaps in the air for as long as you like; a horseshoe in an arc, the alt higher; a lunge in the air leaves her hovering | [W], [GUIDE] | hoop_13 |
| COLLIER | a lift straight up or down that stops level with ledges; getting off keeps some speed; a charge in an arc that goes off on a fighter or after 2 s; held, the fuse burns faster and the charge shields him in front; the alt thrown up | [W] | hoop_14 |
| ASTRA | a rocket pack that pushes harder the longer it burns and leaves some speed when it stops; a ray down and ahead that bounces once and is gone at the next surface, the alt up and ahead; a kick down and ahead in the air | [W], [GUIDE] | hoop_15 |
| GULP | a leap higher the longer B is held, leaned by the pad; after 1 s he glows and the leap is a blockable uppercut to its top; a dart whose sight swings 90° up on the ground or down in the air while held; the alt is fast and straight, and stuck in a wall it locks his darts for 2 s | [W] | hoop_16 |
| Tournament | pick one; beat all eight one at a time in a random order, the last a mirror match against your own fighter in the other colours; lose and you may ask for a rematch (once per opponent) or give up; lose twice to one and you're out | [W], [MM], [TVT] | hoop_19, hoop_s1 |
| The second foe | on the hardest challenge each tournament match brings a second, random opponent | [W] | hoop_22 |
| Draft Battle | seven of the eight on the board; P1 picks first, P2 second and third, P1 fourth and fifth, P2 sixth; then matches, each side fielding its three in a random order, until one side has five wins; against the CPU or a second player | [W], [TVT], [MM] | hoop_20, hoop_21, hoop_s2 |
| Exhibition | two players pick, one match, then back to the pick; UP or DOWN with the move button picks at random | [W] | hoop_01, hoop_21 |
| Options | the challenge, the antigravity control setting and the hoop rules | [MM] | hoop_02 |
| The old rules | the hidden code for the scrapped design: no hoops, four health each | [MH-CHEATS], [LZ] | hoop_23 |
| The demo | the title plays a CPU match when left alone; the twentieth time it shows a note | [MH-META] | hoop_24 |
| Endings | one per fighter, and a note after GULP's in his second colours | [W], [MH-META] | hoop_26 |
| The CPU | plays every fighter's way of moving and fights; as MOSS it now and then turns his gravity over and then just stays where it ends up for a while ("It'd invert gravity and just live where it ended up for a while") | [STEAM-ALL] | hoop_27, hoop_28 |
| Goals | gift: win a Draft Battle; gold: win the tournament; cherry: win it with 4 different fighters, kept across sessions; all on standard hoops | [W], [GGC], [STEAM-SAVES] | hoop_25, hoop_s1 … s3 |
| Save | the options, the champions (and in which colours), the fewest rematches and the counts; no run is saved mid-way | [STEAM-SAVES] | hoop_02, hoop_25 |

### Readings we had to choose

- **Numbers no source gives** (the wiki gives none for movement): walking
  1.25 px a frame; gravity 0.22 px a frame²; TANSY's jumps reach about 40
  and 33 px (a quick tap much less); PEWIT's flap sets her rising at 2 px a
  frame; ASTRA's pack pushes from 0.27 to 0.63 px a frame² over half a
  second, top speed 3.5 px a frame; COLLIER's lift moves 2 px a frame and
  he keeps 60 % of it when he steps off; GULP's leap runs from about 10 px
  (a tap) to about 130 px (a full second), floor to ceiling, as TV Tropes
  says Donkus's does ("from the bottom of the stage straight to the top"
  [TVT]); BRISTLE's spring throws her about
  90 px; CLAMP's claw flies 110 px at 6 px a frame and reels in at 0.8 px
  a frame.
- **Times:** a hit stuns for 0.6 s and keeps you safe for 0.73 s, only just
  longer, so a hit timed to the end of the safe time catches a fighter who
  has barely got going again: the cherry guide's stun lock ("Get used to the
  time it takes enemies to lose iframes to stun lock them" [GUIDE]); dizzy lasts
  1.3 s; a new hoop burns for 2.5 s; a knocked-out hoop can't be caught for
  the first 0.23 s (so the one who dropped it can't just stand there); the
  lunge lasts 0.23 s (BRISTLE's 0.37 s) and can be used every 0.57 s.
- **Knocked-out hoops** fly off upwards at 3.2 to 4 px a frame and keep
  bouncing; they never go away.
- **The CPU's challenge levels** are ours: CALM, ROUGH (the default) and RIOT
  (ROUGH with the second foe). The wiki names only "Hyper", where each match
  adds a second, random opponent; we don't know how the hoops work then, so
  with three in the pit it's simply the first to five, and the CPUs go for
  the player first.
- **The hoop options:** HOOPS TO WIN 3–9, HOOPS AT THE START 0–3 (always
  fewer than to win), A NEW HOOP EVERY 5–30 s, and NEW HOOPS on fire or
  cool. The original's ranges aren't documented.
- **The claw** takes a hoop it passes that isn't on fire. It bites the
  underside of ledges and the ceiling, not walls.
- **MOSS upside down** lands on the undersides of ledges (one-way the other
  way) and on the ceiling. His DOWN + B turns him over rather than dropping
  through (it's his only use of B). His rocket's burst doesn't hurt him.
- **TANSY's spin** sends a knife off down and away from the way it came.
- **COLLIER's charge** that goes off in his hand hurts him; thrown, it
  doesn't. DOWN + B is the lift going down (from a ledge, through it).
- **GULP's sight** swings at 2.5° a frame; a quick tap throws straight
  ahead. While he charges a leap he can't walk.
- **When shots meet** the fast dart and the charge win as the wiki's
  priority tip says; equal ranks knock each other out.
- **How many shots at once, and how often** (our numbers): TANSY 2 knives
  (a throw every 0.37 s), CLAMP 1 cog (0.33 s), MOSS 1 rocket (0.5 s),
  PEWIT 2 horseshoes (0.47 s), ASTRA 2 rays (0.5 s), COLLIER 2 charges
  (0.4 s), BRISTLE's quills every 0.43 s and at most 3 traps (setting a
  fourth removes the oldest), GULP's darts every 0.43 s (the fast dart
  0.5 s).
- **Blocking** works from either side.
- **The draft pool** is TV Tropes' seven; the CPU drafts by a list of its own
  (carelessly on CALM).
- **The select's colours:** UP or DOWN swaps a fighter's colours when it is
  let go; if B was pressed while it was held, that was the random pick
  instead and the colours stay as they were.
- **The old rules code** is our own button code on the title (DOWN UP DOWN
  UP LEFT LEFT RIGHT RIGHT), since UFO 40 has no terminal; under it the
  goals don't count, as with UFO 50's cheats.
- **The demo** starts after 12 s alone on the title.
- **The CPU's MOSS weakness:** a CPU MOSS thinks half again as slowly and
  turns over late; now and then (about one look in twelve) it turns his
  gravity over for no reason and then stands where it ends up for 1 to 2.5
  s, still shooting if it can [STEAM-ALL]. Whether the CPU gets harder over
  a tournament isn't known; ours doesn't.

## Not confirmed

| Control or rule | Our reading | Why it's flagged |
|---|---|---|
| A = weapon, B = move | **Owner's change:** the original has the move on Button 2 (A) and the weapon on Button 1 (B) [W], [MM]; the owner puts the main attack on A. The card's CONTROLS page can swap A and B back. | owner's layout |
| UP / DOWN on the select screen | swap the fighter's colours | the original's colour pick isn't described |
| UP or DOWN + B on the select | anyone at random | the wiki's "Button 2" is our B after the swap |
| The rematch prompt | REMATCH or GIVE UP, both on A | the manual names both, not the buttons |
| DOWN + B for MOSS | turns him over (no drop) | not described |
| DOWN + A for CLAMP and GULP | their weapon | the wiki only says their melee is on Button 2 |
| The old rules code | DOWN UP DOWN UP LEFT LEFT RIGHT RIGHT on the title | the original's is a terminal code |

## What is ours

- **Name:** HOOPLA (1988, Beamdown Softworks), ring nights in the Glass Pit
  (BRAVADO's pit, as Hyper Contender shares Overbold's pits).
- **Fighters:** TANSY the knife juggler, CLAMP the dock crane, MOSS the cave
  hermit, BRISTLE the badger trapper, PEWIT the lapwing girl, COLLIER the pit
  miner, ASTRA the star champion and GULP the bullfrog; their looks, both
  colours, their weapons (knife, cog, rocket, quills, horseshoe, charge,
  ray, dart) and their endings.
- **The twelve pits:** The Bandstand, The Stairs, The Long Table, The Lift
  Shaft, The Scaffold, The Ferry, The Twin Towers, The Crossbeams, The
  Lean-To, The Pillars, The Rafters and The Carousel; six colour sets.
- **Music:** "Ring Night" (title), "Pick Your Corner" (select and ladder),
  "Glass Pit Brawl", "Hoops in the Air", "Ledge to Ledge" (matches), "The
  Mirror Match", "Snake Draft", "Purse in Hand" (ending), "Under the Lamps"
  (credits), and the jingles "Five Hoops", "Knocked Loose" and "Last One
  Standing".
- **Words:** every label, the endings, the night porter's note and the goal
  lines. The endings share no plot with the originals': they are new
  stories (a swimming hole in an old seam, a fighting school, a chapel
  bell, a stage on a lily pad, traps for the valley's henhouses...).

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu (with BACK TO
THE TITLE under RESUME) and the three UFO 40 goals, which are Hyper
Contender's own. The Terminal riddle (the attract match between two named
fighters) and the debug keys are UFO 50 collection features and are left
out.

## Controls

| Input | Action |
|---|---|
| LEFT / RIGHT | walk |
| DOWN (held) | block |
| A | the fighter's weapon |
| UP + A | the alt shot |
| DOWN + A | the lunge (CLAMP, GULP: the weapon) |
| B | the fighter's way of moving (BRISTLE: a spring trap) |
| DOWN + B | drop through a ledge (MOSS: turn over; COLLIER: lift down) |
| START | pause |
| Select screen | LEFT / RIGHT choose, UP / DOWN colours, A pick, UP or DOWN + B anyone |
| Menus | UP / DOWN move, A choose, B back |

Two players: on a PC the keyboard splits in two (or a second pad); on the
Vita, which has one controller, 2 PLAYERS is greyed out with NEEDS TWO
CONTROLLERS, as in the other 2P cartridges.

## Tests

`tests/hoop_01` … `hoop_28` drive the rules with button presses, or set up
a moment with cheats and then play it with presses: the menus and options,
the hoops (fresh, on fire, knocked out, five to win), the block and the
triangle, each of the eight fighters' moves, the ledges, shots meeting, the
tournament with its rematch and mirror match, the draft's snake and series,
2P exhibition and draft, RIOT's three-way matches, the old rules, the demo
and its note, the goals and the save, the endings, and the CPU (every
fighter, alone against a still player, collects five hoops; CPU-only matches
all end; the demo player beats the CPU with each fighter and the CPU still
beats it once). The demo player is the CPU's own brain on its sharpest
setting, pressing the same buttons a player would. From the title it wins a
tournament (`hoop_s1`, gold), a draft battle (`hoop_s2`, gift) and four
tournaments with TANSY, MOSS, PEWIT and ASTRA (`hoop_s3`, cherry). All play
is whole numbers (1/256 px, a fixed angle table), so it plays the same on
every machine.

## Sources

- [W] UFO 50 Wiki (Miraheze), "Hyper Contender" (raw page): the infobox
  (goals, stats, history note), controls and button exceptions, blocking,
  the hoop rules, arenas, modes, every character's maneuver and weapon,
  strategy tips. https://ufo50.miraheze.org/wiki/Hyper_Contender
- [DEBUG] Miraheze, "Debug mode": give rings, both players AI, the spawn
  timer, the right player's rings, skip to match 7, "only 1-12 are unique"
  layouts. https://ufo50.miraheze.org/wiki/Debug_mode
- [MH-CHEATS] Miraheze, "Cheats": the code for the scrapped health rules.
  https://ufo50.miraheze.org/wiki/Cheats
- [MH-META] Miraheze, "Meta Messages": the attract mode's twentieth run, the
  second-colour Donkus ending. https://ufo50.miraheze.org/wiki/Meta_Messages
- [MM] Steam guide "The missing manuals - How to play UFO 50 games",
  section 36: the title menu and options, Tournament or Draft first, A
  moves and B attacks, UP and DOWN with B, drop through, the counter bottom
  centre, burning hoops, the names and hoops bottom left and right, rematch
  or give up. https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [GUIDE] Steam guide "Hyper Contender Cherry Guide" (chocolatecake5000):
  hovering at the top, the block only stopping some attacks, the lunge,
  taking hoops in the safe frames, Sephy's flutter and air lunge, Voltana's
  two lasers, Yogo blocking while hanging.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3341000349
- [GGC] Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [STEAM-ALL] Steam thread "Decided to win with every Hyper Contender…": the
  CPU and Brazz's gravity, Yogo's claw taking rings, uneven mobility.
  https://steamcommunity.com/app/1147860/discussions/0/4699034745339898348/
- [STEAM-SAVES] Steam thread on which games save: the champions are kept.
  https://steamcommunity.com/app/1147860/discussions/0/4849904176633630339/
- [TVT] TV Tropes recap (through the research notes): the mirror match in
  the other colours, the draft pool of seven, the triangle.
  https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game36HyperContender
- [LZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 36": the scrapped
  health system behind a code, the 2P recolours.
  https://lizstar64.github.io/reviews/2024/10/18/UFO50-36.html
- [STATIC] Static Canvas, "The UFO 50 Diaries: Hyper Contender".
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-hyper-contender
- [POPCAR] Popcar's Blog, "Reviewing Every Single UFO 50 Game".
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
