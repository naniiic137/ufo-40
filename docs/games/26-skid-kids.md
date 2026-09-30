# 26 · SKID KIDS

*Internal design document. Not shown in the product.*

## Tribute to

**Hot Foot** (UFO 50 game #26, Mossmouth, "August 1986"). The rules were
researched from text only: the community wiki (raw pages and old
revisions), Steam guides, threads and patch notes, written reviews, TV
Tropes and YouTube descriptions (listed under Sources; the full notes are
in the research folder, `26-hot-foot.md`). No UFO 50 images, video,
sprites, music, court art or text were used as references, and nothing was
taken from a UFO 50 install.

**Content: one court, a random draft.** Hot Foot is played on a single
top-down gym court, so there are no levels; ours is our own gym (a painted
cinder-block wall with a school banner, a wooden floor, a red centre line
and circle, blue mats on the back walls). What is random follows the
original: the five kids of a draft, the teams you meet, the Coach's items
and where they land, and the CPU's movement.

| Full scale | Hot Foot | SKID KIDS |
|---|---|---|
| Players on court | 2 v 2, one kid of a pair driven at a time [MAN], [W] | the same |
| Kids | 12 [W] | 12 |
| Passives | 12, one a kid [W] | the same 12, one for one |
| Special throws | 4, each on 3 kids [W] | 4, each on the same 3 kids |
| Special actions | 4, each on 3 kids; 12 of the 16 throw/action pairs [W] | the same pairs |
| Boss partners | 2, three passives each and 2 specials of their own [W], [TVT-Y] | 2: the kangaroo and the robot, the same passives and the same kinds of specials |
| Things on the floor | bags, drinks, puddles, banana peels, the robot's blockade [W] | bags, juice boxes, puddles, marbles, the sweeper wall |
| Team select | Draft Team (5 random: pick 1, they pick 2, pick 1 of 2) and Build Team [W], [MAN] | the same two, named TAKE TURNS and HAND-PICK |
| Tournament | 6 matches, single elimination: 4 teams, the monkey's team, the left-over kid and the robot [W], [TVT-R] | the same 6 |
| Match | first to 15 hits [W] | the same |
| Modes | 1P, 2P co-op, 2P versus, an attract demo [W-OLD], [W-META] | the same four |
| Codes | 5 (bosses in Build Team, switch on its own button, co-op as the bosses, custom rules for 1P and for versus) [W-CHEATS] | 5 of our own, the same effects |
| Goals | 3 [W], [GGC] | the same 3 |
| Stats | Athletes Used, Athletes Won With [W] | the same two under our own names, KIDS PLAYED and KIDS CROWNED, on a RECORDS screen |
| Pre-match talk | a line for each kid leading the other side, some pairs' exchanges ("three, maybe more"), your kids reacting to odd opponents [W], [TVT-R] | a line for each of the 12 kids and both partners, the left-over kid's line, 4 pair exchanges, 1 secret one and 3 reactions |

## Mechanics checklist

| Mechanic | How SKID KIDS does it | Source | Test |
|---|---|---|---|
| The sport | top-down 2 v 2 in a gym; a sliding bag from the other team that touches a kid is a point for the thrower's team | [W], [LIZ] | skid_06 |
| First to 15 | the first team to 15 wins; nothing counts after that | [W], [PATCH] | skid_14 |
| The court | your team on the left, a centre line nobody crosses but the cheat, back walls the bags bounce off | [MAN], [W-OLD], [ESC] | skid_15 |
| HUD | two score boxes at the top, red for the left team, blue for the right; each kid's stars; the Coach at top middle | [MAN] | (drawn) |
| Moving | 8 ways on your half | [MAN] | skid_15 |
| Pick up (tap B) | next to a bag or juice box with free hands: the kid bends down for a moment, and can be hit meanwhile | [MAN], [W], [LIZ] | skid_01 |
| Swap (tap B) | nothing within reach, hands empty: control swaps to the other kid | [MAN], [W] | skid_01 |
| Pass (tap B) | with a bag in hand: it's tossed to the partner as you swap to them, if the partner's hands have room (full, it's only the swap: nothing is dropped); tapped twice, the second tap swaps back, so the pass goes without swapping | [MAN], [W], [ST-CONTROLS] | skid_02 |
| Wind-up (hold B) | with a bag: the kid can't move, the pad aims (8 ways, arrows on the meter), letting go throws | [W], [MAN] | skid_03 |
| Light toss | a tap-and-let-go is a short lob that lands by your feet and stops, to be picked up again; at point blank it knocks down; the window is the same for every kid (the quick wind-up only makes the full throw sooner) | [POWER], [BULLY], [CHEESE], [W] | skid_04, skid_25 |
| Full wind-up | fast and far, and always knocks the kid it hits down | [POWER], [YT-6B] | skid_03 |
| Calling it off | A during a wind-up cancels it and jumps, in the same press ("you can cancel a throw by jumping"); the bag stays in hand | [MAN] | skid_05 |
| No dropping | a bag can only be thrown or passed; juice only with empty hands | [ST-CONTROLS] | skid_11 |
| The forced throw | a bag held too long throws itself | [POWER] | skid_13 |
| Jump (A) | by default both kids of your team jump at once | [MAN], [LIZ], [POPCAR] | skid_07, skid_08 |
| Half-stars | jumping over a moving bag, or a juice box, is half a star; one jump can earn both kids one; three stars at most, per kid | [W], [MAN], [EPYO] | skid_07, skid_11 |
| Powered up | with a full star a star circles the kid | [MAN], [W] | (drawn) |
| Special throw | a full wind-up with a star spends it on the kid's special throw | [W], [MAN] | skid_09 |
| Special move | A again in the air with a star spends it on the kid's special move | [W], [MAN], [LIZ] | skid_10 |
| Knockdowns | a full wind-up, a light toss at point blank, a push, the stomp, slipping, a juice box on the head; a kid who is down can still be hit and scored on but stays down no longer | [W], [TVT-Y], [BULLY], [CHEESE] | skid_06 |
| Getting up | a moment helpless; the quick-up kid in half | [W] | skid_17 |
| Several bags | a limit on bags on the court; the Coach tops them up | [W-CHEATS] | skid_12 |
| The Coach | explains the game before the first tournament, referees, barks, and throws bags and juice boxes in, mostly onto the centre line; he tosses the first one there to start | [W-COACH], [STATIC], [MAN], [W] | skid_12, skid_47 |
| The Coach's bag | one that lands on a kid is a point for the other team; his juice box only knocks down | [W] | skid_11, skid_12 |
| The Coach's pet | with the mascot kid in the match the Coach favours that side with items, throws water balloons at the other team, and the other team's first bag lies further away | [W], [TVT-R] | skid_26 |
| Blowout / close game | the loser on 9 or less (the patched rule), or on 13 or more | [W], [PATCH] | skid_14 |
| One loss | ends the tournament: game over | [W] | skid_33 |
| TAKE TURNS (Draft Team) | 5 kids at random; you pick 1, the other side 2, you 1 of the last 2; the one left over cries | [W], [MAN], [TVT-R] | skid_30 |
| HAND-PICK (Build Team) | any 2 of the 12, from the first play | [MAN], [ST-CRASH1] | skid_31 |
| The tournament | 6 matches; match 5 a kid and the kangaroo; match 6 the left-over kid and the robot it built to get even | [W], [TVT-R], [W-LIST] | skid_32, skid_36 |
| The ending | the credits, and a code in them | [IC], [YT-6B], [TVT-Y] | skid_35 |
| 2P co-op | each player drives one kid of one team through the tournament; the goals count | [W-OLD], [IC], [ST-2P] | skid_37 |
| 2P versus | each player drives both kids of a team | [W-OLD] | skid_38 |
| The demo | CPU against CPU if the title sits; the score keeps climbing | [W-META], [TVT-R] | skid_43 |
| Codes | bosses in HAND-PICK; switch on its own button; co-op as the two bosses (hinted by the gym banner); custom rules for 1P and for versus (points 5-30, bags 1-7, special cost 1 or 2, partner fetches bags or plays alone, jumping together or alone); no saving or goals while on | [W], [W-CHEATS], [W-META] | skid_39, skid_40, skid_41, skid_53 |
| Menu stats | the original's two (the kids used, the kids a tournament was won with), as KIDS PLAYED and KIDS CROWNED | [W] | skid_42 |
| Goals | Beacon: win 3 matches in one tournament; Saucer: win the tournament; Alien: win it with a blowout in every match (co-op counts, codes don't) | [W], [GGC], [PATCH], [IC] | skid_32, skid_34, skid_37, skid_39 |
| Your partner | fetches bags only and never passes one by itself: it holds it in the corner of your half away from you (until the forced throw), and the bag is yours to swap to, or to swap over and straight back for (it's tossed across as you swap); it jumps when you jump, is bad at dodging and walks to the front for bags, but doesn't walk into a throw coming at it or to a bag in a throw's way (patch v1.2.2) | [MAN], [POWER], [ESC], [ST-BEST2P], [ST-RANT], [POPCAR], [PATCH] | skid_46 |
| The CPU | both kids play on their own and can wind up at once; they hold a full wind-up for someone to bend down; they chase every item the Coach throws in, into an ambush if need be; the cheat crosses to steal bags; later teams pile on a kid who is down; they read a wiggly throw as a straight one; they steer round puddles (the kid who never slips doesn't bother) | [ST-RANT], [POPCAR], [BULLY], [CHEESE], [TVT-Y], [POWER], [PATCH] | skid_45, skid_16, skid_54 |
| The curve | matches 1-4 easy, 5 and 6 a wall | [ESC], [BULLY], [CHEESE], [W-LIST] | skid_52 |
| Secrets | the demo left to reach 999 each; player 2 holding a direction on the 2P draft screen for 30 s; a special exchange for one pairing | [W-META], [TVT-R] | skid_43, skid_44, skid_36 |

### The twelve kids, one for one

Every kit follows the original's table [W] exactly: the same passive, the
same special throw and the same special move, each throw and each move on
three kids, and the same 12 of the 16 pairs. The names, looks and words are
ours.

| Hot Foot | SKID KIDS | Passive | Special throw | Special move | Test |
|---|---|---|---|---|---|
| Jerry | NOODLE (curls and freckles) | throws wiggle as they slide | COMET (Energy Ball) | STOMP (Earthquake) | skid_16 |
| Amy | PIPPA (an orange pigtail) | gets up in half the time | RIPPLE (Wave) | GUST (Wind) | skid_17 |
| Chandar | HOPS (a lime headband) | jumps higher and quicker | RIPPLE | REEL (Vortex) | skid_18 |
| Benjy | MILO (big glasses) | arcing throws, a high far light toss | POPPER (Bomb) | REEL | skid_19 |
| Rizzik | SPARKY (spikes and goggles) | winds up in half the time | POPPER | SPLASH (Water Balloon) | skid_20 |
| Bea | NELL (a bob and a clip) | every half-star goes to her partner too | YO-YO (Boomerang) | STOMP | skid_21 |
| Yoka | KIKI (two buns) | carries two bags | YO-YO | GUST | skid_22 |
| Suze | ROXIE (a pink ponytail) | runs faster, never slips | COMET | GUST | skid_23 |
| Marc | TOBY (a cap on backwards) | picks up in half the time | COMET | SPLASH | skid_24 |
| Edgar | SID (a quiff and a smirk) | ignores the centre line | POPPER | STOMP | skid_25 |
| Mascot | BUZZY (the school's hornet suit) | the Coach's pet | RIPPLE | SPLASH | skid_26 |
| July | MOOSE (a big kid) | walking into a rival knocks it down (the pad held toward it is enough, even against the line); jumping into the robot knocks it down | YO-YO | REEL | skid_27, skid_29 |
| Zobo (monkey, match 5) | BOOMER (a kangaroo who came on a field trip and never went home) | high quick hops + two bags + fast feet | MARBLE BAG (Banana Bomb) | POUCH GRAB (Monkey Brains) | skid_28 |
| Blorg (robot, match 6) | BENCHBOT (built by the kid nobody picked) | arcing throws + a quick wind-up + a shove that beats any other when both stand | HOMING POPPER (Boomerang Bomb) | SWEEPER (Magnetize) | skid_29 |

What each special does:

- **COMET:** faster, farther, and a much bigger hit.
- **RIPPLE:** a wave along the floor to the other team's back wall, the bag
  riding on it; everyone of that team in its band goes down (a point each)
  unless they jump it, and things lying on the floor are nudged along.
- **YO-YO:** out along the aim, knocking down whoever it meets, then home,
  hitting again on the way back, into the thrower's hand.
- **POPPER:** thrown like a bag; where it stops it goes off, and everyone of
  the other team near it (and not in the air) is knocked down, a point
  each. The hit before counts too.
- **STOMP:** the floor under the other team shakes: everyone of it on the
  floor falls down (no point); winding up is cut short.
- **GUST:** for a while everything on the floor is blown toward the other
  team's wall: bags lying on your side and bags thrown at you turn into
  hits on them.
- **REEL:** the nearest bag, lying or flying (a pass too), flies into your
  hand; with your hands full it's gone.
- **SPLASH:** a water balloon; whoever it lands on falls, and it leaves a
  puddle that trips whoever runs through it.
- **MARBLE BAG:** thrown like a popper; it bursts into a ring of marbles,
  and a kid who steps on one slips and lies helpless for longer; each
  marble is spent once slipped on.
- **POUCH GRAB:** a reel that pulls two bags, one after the other.
- **HOMING POPPER:** thrown like a popper; it goes off, then flies home to
  the robot.
- **SWEEPER:** a wall slides from the other team's back wall to the centre
  line: it trips everyone of that team on the floor and drags every bag it
  meets over to the robot's side, where some come out crushed.

### Readings we had to choose

- **Who you meet.** The wiki doesn't say whether the CPU's two draft picks
  are your first opponents; ours are. The seven kids who weren't in the
  draft then fill matches 2 to 4 and the kangaroo's side, so every one of
  the twelve kids turns up once. In HAND-PICK one of the other ten, at
  random, is the kid left over for the final. HAND-PICK is open from the
  first play (the manual and a crash report; one guide says it unlocks).
- **Numbers no source gives** (at our 320 × 180 screen): walking 1 pixel a
  frame, the fast kids 1.3; a jump lasts 0.52 s and tops out at 16 pixels
  (the high jumpers 0.45 s and 18); B held 10 frames with a bag starts a
  wind-up, one let go before 20 frames is the light toss (the same for
  every kid), and it is full after 60 (the quick wind-up 30); a thrown bag
  leaves at 1.3 to 3.8 pixels a frame, harder the nearer the full wind-up,
  and slows by 0.03 a frame; bending down for a bag takes 15 frames (the
  quick hands 7); down for 75 frames (the quick riser 38), a flinch 14, a
  marble slip 150; the forced throw after 5 s in hand; five bags on the
  court at most; the Coach every 4 to 8 s, 70 % of his items within 10
  pixels of the line.
- **Knockdowns.** A full wind-up always knocks down; a light toss that hits
  before it lands knocks down; any other throw is a point and a flinch. A
  kid who is down can be hit for a point but stays down no longer, and
  there is no safe time after getting up (so the stun-lock is real).
- **Which specials score.** The ripple, the popper's blast (and the homing
  popper's) and bags turned by the gust score. The stomp, the splash, the
  sweeper, a push, a slip and a juice box on the head only knock down.
- **Friendly fire:** none. Your own team's bags pass you by; the Coach's
  bags hit anyone.
- **Arcing throws** (MILO, BENCHBOT): a low arc for the first stretch of the
  throw, cleared only from the top of a jump; their light toss goes high
  and three times as far.
- **The top and bottom edges** bounce bags like the back walls. Each team
  starts with a bag on its side; the Coach tosses a third onto the centre
  line at the whistle.
- **Your partner** fetches bags only (the custom rules call the default
  "get bags only"). Nothing says it passes, and players get its bag by
  switching to it, or switching over and back [POWER]; so a bag in its
  hands stays there, in its corner, until you do or the forced throw comes.
  It stops where it is while a throw is coming at it, and skips a bag
  lying in a throw's way or by a fizzing popper: the patch that stopped it
  running toward threats [PATCH]. It still doesn't dodge (it jumps when you
  do).
- **Pushes:** the pusher next to a rival with the pad held toward it, even
  pressed against the line (the centre-line trick players describe
  [BULLY]); in the air, the pusher knocks down even the robot (the answer to
  match 6 [BULLY], [TVT-Y]); nobody in the air can be pushed, and between
  two standing kids the robot's shove wins.
- **Puddles and the CPU:** a patch fixed the fast kid's AI "still avoiding
  puddles" [PATCH], so the CPU steers round a puddle lying on the floor,
  and the kid who can't slip walks straight through. A water balloon in
  the air isn't a puddle yet: the CPU never sees it coming [ST-CONTROLS].
- **Wiggly throws:** the CPU reads them as straight throws, and so often
  fails to jump them ("the AI does not adjust for it" [POWER]); only the
  demo player allows for the wiggle.
- **Co-op:** each player drives one kid; nobody swaps, and each kid jumps on
  its own button. The CPU's kids always jump on their own.
- **Reel range:** the nearest bag within 150 pixels.
- **The robot's wall** is full height and trips (no point); a third of the
  bags it drags come out crushed.
- **The kangaroo's passives** are the three the sources give (high quick
  jumps, two bags, fast feet); he slips like anyone.
- **Coach's pet:** 70 % of his items land 20 to 60 pixels onto that side, and
  a quarter of his throws are a water balloon at a kid of the other team.
- **Match music:** unreported; ours plays tunes during matches.
- **Close game:** named by the wiki without an effect; ours is a banner on
  the result card, like the blowout's.
- **The tutorial:** the Coach explains the game before the first tournament
  of each session.
- **The codes:** UFO 40 has no terminal, so the title has a CODES screen that
  spells XXXX-XXXX codes. While any is on, nothing is saved and no goal or
  record is kept, until the cartridge is left.
- **The CPU's skill** (reaction, share of bags jumped, share of full
  wind-ups, how near the line it throws from, whether it holds a full
  wind-up in ambush, whether it piles on a kid who is down) rises from match
  1 to 4 and jumps for 5 and 6.
- **The demo** starts after 12 s on the title and never ends by itself.
- **The HUD:** the two score boxes and the Coach [MAN]; each kid's stars sit
  beside the scores (halves too), since the circling star alone can't show
  halves, with the kid's name to tell them apart.

### Controls we couldn't confirm

The functions of the two buttons are confirmed everywhere (the jump on A,
everything else on B, tap versus hold); these details are ours.

| Control | Our reading | Why it's flagged |
|---|---|---|
| Tap, toss or throw | B held 10 frames with a bag starts the wind-up (shorter is a tap); let go before 20 frames it's the light toss, for every kid | no source times it; Rizzik's passive is only a faster charge [W] |
| Pass without swapping | two quick taps (the second swaps back) | one player's tip [ST-CONTROLS] |
| Aim | the last way pressed during the wind-up sticks; none pressed is straight at the other team | not described |
| Two-bag kid by a bag | a tap picks the second bag up instead of passing | the bible marks the overlap unconfirmed |
| Tap with a bag, the partner's hands full | only the swap; nothing is dropped | the wiki only says the bag is tossed across when the other kid has none |
| In the air | a tap passes; a wind-up waits for the floor; B by a bag does nothing | not described |
| Special move aim | the SPLASH goes the way the pad points (straight ahead with none) | not described |
| Co-op | no swapping; each kid jumps on its own | the bible marks it unconfirmed |
| Swapping while down | allowed: B swaps to the partner while the kid you drive is down or flinching | not described |
| SWAP-ONLY code | B only swaps, and no bag goes across; A picks up, passes with a tap when holding a bag (you keep the kid you drive), winds up when held (a short hold tosses; B calls it off) and jumps with empty hands; so no jumping with a bag in hand | the wiki says pick up, throw and pass move from the switch button to the jump button [W-CHEATS]; the rest is ours |

## What is ours

- **Name:** SKID KIDS (1986, Beamdown Softworks).
- **The kids** and their looks: NOODLE, PIPPA, HOPS, MILO, SPARKY, NELL,
  KIKI, ROXIE, TOBY, SID, BUZZY and MOOSE, in red and blue gym bibs; BOOMER
  the kangaroo and BENCHBOT, the robot made from a gym bench, a science-fair
  volcano and a solar calculator. The Coach (cap, whistle, tracksuit).
- **The gym:** our own court and wall, the HORNETS banner, the hornet crest
  on the centre spot, the blue mats.
- **Every name of a thing:** COMET, RIPPLE, YO-YO, POPPER, MARBLE BAG, HOMING
  POPPER, STOMP, GUST, REEL, SPLASH, POUCH GRAB, SWEEPER; juice boxes;
  the Gym Class Cup; TAKE TURNS and HAND-PICK; KIDS PLAYED and KIDS
  CROWNED.
- **The codes:** ALLS-TARS (the bosses in HAND-PICK, shown in the credits),
  SWAP-ONLY (switch on its own button), HORN-ETS! (co-op as the kangaroo and
  the robot, hinted by the banner STING 'EM, HORNETS!), GYMR-ULES (house
  rules for 1P and co-op), DUEL-RULE (house rules for versus).
- **Words:** every kid's line, the four pair exchanges (SID and BUZZY, ROXIE
  and KIKI, MOOSE and PIPPA, NELL and NOODLE), your kids' reactions to the
  kangaroo, the robot and the hornet, the Coach's tutorial and barks, the
  ending (with an extra word from the Coach after six blowouts) and the
  credits.
- **Secrets of our own:** the robot's secret (with MILO and SPARKY on your
  team it turns out to be built from their science-fair projects); a note
  found under the bleachers (player 2 holding UP for 30 s on the versus
  draft); the Coach calling it a day if the demo reaches 999 each.
- **Music:** "Skid Kids" (title), "Pick Me, Pick Me" (teams), "The Big
  Board" (the tournament), "Gym Class" and "Recess Rally" (matches),
  "Field Trip" (the kangaroo's match), "Benchbot" (the final), "Gym
  Champions" (ending), "Hit the Showers" (credits) and three jingles.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
UFO 40 goals, which are Hot Foot's own (gift, gold, cherry). The CODES
screen stands in for UFO 50's terminal, where these codes are entered in
the original. The garden gift has no place in UFO 40. On the Vita (one
controller) the 2P modes are locked, as in the other 2P cartridges.

## Controls

| Input | Action |
|---|---|
| D-pad | run 8 ways; while winding up, aim |
| B (tap) | by a bag or juice box: pick it up; with a bag: pass it as you swap (only the swap if the partner's hands are full); otherwise: swap kids |
| B (hold) | with a bag: wind up; let go to throw (a quick tap-and-let-go tosses it by your feet) |
| A | jump (the whole team); in the air, with a star: the special move; while winding up: call it off and jump |
| START | pause |

Split keyboard for two players: player 1 WASD + F / G, player 2 arrows +
K / L.

## Tests

`tests/skid_01` … `skid_54`, driven by button presses (cheats only set a
moment up): the one-button rules (01-05), hits and knockdowns (06), jumps
and half-stars (07-08), specials and their cost (09-10), juice and the
Coach (11-12), the forced throw (13), first to 15 with blowouts and close
games (14), the line (15), every kid's kit (16-29), TAKE TURNS, HAND-PICK,
the tournament, game over, the ending and credits (30-36), co-op and versus
(37-38), the codes and their no-saving rule (39-41, 53), the records
(42), the demo and the secrets (43-44), the CPU and the partner (45-46),
the tutorial (47), the tunes (48), the curve (52) and the CPU and puddles
(54).

The demo player (`skid_bot_pad` in `skidkids_ai.c`, the "bot" query) drives
your kid with real buttons every frame: it jumps every bag it sees in time
(a jump calls a wind-up off in the same press), keeps back while a rival
holds a bag, camps by the line where the Coach's items fall and the rivals
come running, and throws short: a light toss at point blank, a quick throw
at a rival bent over a bag or winding up, a full one at a kid about to get
up. The partner never passes, so for its bag the demo player swaps over to
it and straight back, as players do. From the title, through the draft
and all six matches, it wins the tournament on three seeds (`skid_49` …
`skid_51`); over 120 seeds it won 95, conceding on average about 6 points
in matches 1 to 4, 6.5 against the kangaroo and 9 in the final, which is
where 16 of its 25 losses came.

## Review fixes

An independent review (2026-09-30) found twelve things off; all are fixed.

1. **The partner fed you bags** (invented): it passed you every bag it
   picked up. Now it never passes by itself; it holds the bag in its
   corner, and the bag is yours to swap to, or to swap over and back for
   [POWER] (skid_46).
2. **Jumping didn't jump out of a wind-up:** A only called it off. Now one
   press calls it off and jumps [MAN] (skid_05).
3. **MOOSE's push needed him to actually move,** so it failed pressed
   against the line. Now the pad held toward a rival counts, blocked or
   not: the centre-line trick [BULLY] (skid_27).
4. **Jumping into the robot didn't knock it down.** Now a pusher in the air
   knocks down any standing rival, the robot too; the robot still wins
   when both stand [BULLY], [TVT-Y] (skid_29).
5. **The light toss window shrank for the quick wind-up kids** (to 4
   frames). Now it's the same for every kid, let go before 20 frames; a
   throw's power grows from there (skid_04).
6. **The CPU allowed for the wiggly throw.** Now it reads it as straight
   [POWER] (skid_16).
7. **A tap with a bag tossed it even when the partner's hands were full**
   (it fell on the floor). Now that's only a swap [W] (skid_02).
8. **The partner walked into danger for bags.** Now it stops while a throw
   is coming at it and skips bags in a throw's way or by a fizzing popper
   [PATCH] (skid_46).
9. **The CPU never steered round puddles.** Now it does, except the kid
   who never slips; a balloon in the air is still unseen [PATCH]
   (skid_54).
10. **The kangaroo never slipped** (invented). He slips like anyone now
    (skid_28).
11. **SWAP-ONLY: a tap of A with a bag tossed it,** and passing was left on
    B. Now a tap of A passes (you keep your kid), a short hold tosses, and
    B only swaps [W-CHEATS] (skid_40).
12. **UFO 50's words** on the team-select menu and the records: now TAKE
    TURNS and HAND-PICK, KIDS PLAYED and KIDS CROWNED (skid_30, skid_31,
    skid_42).

## Sources

- [W] UFO 50 Wiki (Miraheze), "Hot Foot" (raw page): the rules, the
  characters' powers, throws and actions, the bosses, the Coach, the goals,
  the stats. https://ufo50.miraheze.org/wiki/Hot_Foot
- [W-OLD] The same page's older revisions (modes, "Player 1 is on the
  left", the old special names).
- [W-CHEATS] https://ufo50.miraheze.org/wiki/Cheats
- [W-META] https://ufo50.miraheze.org/wiki/Meta_Messages
- [W-LIST] https://ufo50.miraheze.org/wiki/List_of_Games
- [W-COACH] https://ufo50.miraheze.org/wiki/Coach_Blazer
- [MAN] Steam guide "The missing manuals", section 26.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [GGC] Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [BULLY] Steam guide "Hot Foot Cherry Completion Bully Strategy" and its
  comments. https://steamcommunity.com/sharedfiles/filedetails/?id=3342081790
- [CHEESE] Steam guide "Hot Foot UFO 50 - Cheese Strategy for first five
  levels". https://steamcommunity.com/sharedfiles/filedetails/?id=3345608809
- [POWER] Steam guide "Hot Foot - Power-Focused Team".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3386590610
- [PATCH] Steam patch notes v1.1.4, v1.2.2, v1.3.1, v1.4.0, v1.4.1.
- [ST-2P], [ST-CRASH1], [ST-RANT], [ST-CONTROLS], [ST-BEST2P], [ST-TIER]
  Steam threads (the research file lists each URL).
- [TVT-R], [TVT-Y] TV Tropes recap and YMMV pages for Hot Foot.
- [LIZ] Lizstar, "UFO 50 Retrospective Part 26 - Hot Foot".
  https://lizstar64.github.io/reviews/2024/10/15/UFO50-26.html
- [STATIC] Static Canvas, "The UFO 50 Diaries: Hotfoot".
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-hotfoot
- [POPCAR], [EPYO], [ESC], [IC], [PUNISHED], [PIXELDIE] reviews and forum
  threads (the research file lists each URL).
- [YT-6B] YouTube description, "UFO 50 - Hot Foot - Cherry win (6
  blowouts)". https://www.youtube.com/watch?v=dqGxqM_0D54
