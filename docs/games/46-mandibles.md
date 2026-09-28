# 46 · MANDIBLES

*Internal design document. Not shown in the product.*

## Tribute to

**Combatants** (UFO 50 game #46, Mossmouth). The rules were researched from
text only: the Steam guide "The missing manuals" (its section 46 describes
the controls, the whole command menu, melee, respawn, losing and queens),
the community wiki, written reviews and Steam threads and guides (listed
under Sources; the notes are in the research folder, `46-combatants.md`,
`controls-41-44-46.md` and the review `46-review.md`). The controls were
also confirmed by the owner from the original's own on-screen prompts. No
UFO 50 images, video, sprites, music, maps or text were used as references.

**Map type: fixed, hand-made.** Combatants is a string of hand-made fields
on a branching road to the capital, so ours are too
(`mandibles_maps.c`, all our own layouts). Only the structure follows the
original: red placed better and next to its food, about twice blue's
numbers, two or three red queens on the later fields, giant spiders on some,
one pointless extra field.

| Full scale | Combatants | MANDIBLES |
|---|---|---|
| Campaign fields | 12, gating the way to the final one [MH], [SR] | 12, the 12th is the Old Colony (the capital) |
| Route | branching: "skip Mission 3 entirely by going north instead of west" [PT]; the final can be beaten before the hard optional fields [OF] | branching: field 2 opens 3 (west) and 4 (north); the shortest road to the capital is 7 fields; 8 and 11 are side roads |
| Bonus field | a hidden, pointless extra mission [MH], [SR] | POINTLESS HILL, shown once the capital is taken |
| Modes | 1P campaign, 2P versus [MH] | the same two; 3 versus fields |
| Ant types | your leader ant, workers, soldiers, queens [MM], [MH] | the same four, blue and red |
| Commands | 9 on a cross: make workers, make soldiers, surrender; follow, soldier follow; instinct, soldier instinct; hold, soldier hold [MM] | the same 9, in our own words |
| Neutral creatures | giant spiders [HW], [OF] | the longlegs, on 7 of the 13 fields (two on the capital, three in the garden) |
| Goals | 3 [MH], [GG] | the same 3 |

What each field brings: 1 the basics, one red queen; 2 a red soldier (and
one of yours); 3 the Gauntlet, a walled road with a soldier on every narrow
(the wall of the game); 4 the easy road north; 5 the first longlegs, a
stream with fords; 6 two red queens; 7 a single pass with a longlegs asleep
in it; 8 the wide lawn, miles of grass, two red queens on a heap of sap,
about twice your numbers and a longlegs to lead over (the Open Field of
ours); 9 rows of roots that break sight lines; 10 three queens behind one
wall with a single gap (the hardest); 11 three longlegs and two red queens
that nobody needs to fight (the cherry field); 12 the capital, three queens
and two longlegs, the one on the east side roaming through the red half; 13
the bonus.

### Starting forces

What the briefing shows ("the name and size of the maps, and the starting
units for each side" [MM]). Blue always has your ant and one queen; W
workers, S soldiers, q red queens, w red workers, s red soldiers. Red lays
at most one soldier in three ("the enemy AI doesn't produce many soldiers"
[HW]); where the sources say you start with a soldier [HW], fields 2, 5, 8,
9 and 12 give you one.

| Field | Size (tiles) | Blue | Red | Beads | Longlegs |
|---|---|---|---|---|---|
| 1 | 60×19 | 1 W | 1 q, 2 w | 22 | 0 |
| 2 | 64×18 | 1 W, 1 S | 1 q, 3 w, 1 s | 25 | 0 |
| 3 | 80×14 | 1 W | 1 q, 4 w, 3 s | 22 | 0 |
| 4 | 60×17 | 2 W | 1 q, 2 w | 17 | 0 |
| 5 | 71×18 | 1 W, 1 S | 1 q, 4 w, 1 s | 27 | 1 |
| 6 | 72×17 | 1 W | 2 q, 4 w, 2 s | 27 | 0 |
| 7 | 78×16 | 1 W | 1 q, 3 w, 1 s | 21 | 1 |
| 8 | 96×27 | 2 W, 1 S | 2 q, 4 w, 3 s | 28 | 1 |
| 9 | 80×16 | 1 W, 1 S | 1 q, 3 w, 1 s | 20 | 1 |
| 10 | 84×17 | 1 W | 3 q, 6 w, 2 s | 24 | 1 |
| 11 | 90×16 | 1 W | 2 q, 7 w, 2 s | 19 | 3 |
| 12 | 96×16 | 1 W, 1 S | 3 q, 4 w, 3 s | 25 | 2 |
| 13 | 70×16 | — | 1 q, 1 w | 9 | 0 |

**The difficulty curve**, measured with the demo player (a skilled cheeser:
it kites, stands off at angles, leads the longlegs and throws its free
respawns away without a second thought) in eight seeds per field within
eleven minutes each: fields 1, 2, 3, 4, 6, 9 and 13 every time; 5 seven
times; 7 and 11 seven times; 8 six; 12 four; 10 three (the others ran out of
time). The original's players agree that the free respawn makes
Combatants cheesable ("you can respawn infinitely with basically zero
drawback" [RB]); the fields are hard for anyone who plays it straight. With
no input at all the reds win (mnd_16).

## Mechanics checklist

| Mechanic | How MANDIBLES does it | Source | Test |
|---|---|---|---|
| Controls | d-pad walks (8 ways); hold A for the command cross; B spits the way you face | [MM], owner's check of the on-screen prompts | mnd_02, mnd_04 |
| The command cross | while A is held a d-pad direction highlights a command; letting go of A gives it | [MM] | mnd_04 |
| Up | lay workers, lay soldiers, withdraw | [MM] | mnd_04, mnd_06 |
| Right | fall in, squad fall in (soldiers only) | [MM], [DD] | mnd_04, mnd_05 |
| Down | free will, squad free will | [MM] | mnd_04 |
| Left | halt, squad halt | [MM], [ST] | mnd_04 |
| Shout radius | ant commands reach your ants within a few blocks (five tiles); the queen commands reach every blue queen from anywhere | [MM] ("generally"), [SU] | mnd_04 |
| Withdraw | gives up the field: straight back to the map, counted as a loss | [MM] | mnd_04 |
| "!" | an ant (or queen) that took a command shows "!" and you hear the shout | [CTRL], [TT] | mnd_04 |
| Fall in | ants step toward you in straight lines | [MM], [LZ] | mnd_04, mnd_05 |
| Followers spit with you | soldiers on Follow spit whenever you do, the way you face; one in five spits the way it happens to face instead; a stack of five and you put six into one ant | [MH], [DD], [CO] | mnd_05 |
| Halt | ants stand; soldiers spit at anything in range | [MH], [ST] | mnd_04, mnd_14 |
| Free will | the AI takes over: fetch beads, carry them home, fight what it sees | [PT], [LZ] | mnd_06 |
| Beeline for food | an ant that has just fed the queen heads straight for the bead nearest her, seen or not, walls or no walls (and gets stuck) | [TT], [HW] | (rules) |
| New ants | hatch on free will and wander off ("tell them to halt as soon as they come out") | [ST] | mnd_06 |
| Stale orders | friendly ants are worthless unless ordered in the last 1–2 s: a Follow goes stale after 2 s | [SU] | (rules) |
| Stumbling | one step in four goes the wrong way, ordered or not | [SU] | (rules) |
| Out of sight, out of mind | ants act only on what they can see; a follower that loses sight of you for half a second forgets you | [PT], [SU] | mnd_18 |
| Shouting again | a fresh command skips an ant's pause after each step, so shouting at every pause moves the squad twice as fast | [HW] | (bot) |
| Sap beads | green beads lie on the field; walk over one to carry it, touch your queen to feed her | [MM], [MH] | mnd_06 |
| Costs | worker 1 bead, soldier 2; each queen lays whichever she is told to | [MH], [MM] | mnd_06 |
| Health | blue ants 3, red ants 6 ("die in 3 shots", "die in 6") | [MH], [SU], [CO] | mnd_02 |
| A spit | does 1 | [SU], [DD] | mnd_02 |
| Melee | enemy ants that touch brawl for a moment in a cloud of dust with stars; then one comes out, chosen by health, type (soldiers fight harder) and luck, keeping the difference in health (at least 1) | [MM], [MH], [TT], [LZ] | mnd_03 |
| Locked aim | every soldier not on Follow, red or blue, picks its direction when it first spits and keeps it until its target dies or leaves its range; come at it from an angle | [HW], [OF], [CO], [DD] | mnd_05, mnd_08 |
| Respawn | only your ant comes back: the queen hatches it again after a delay, free, as long as another blue ant (worker or soldier) lives; dying is the cheapest ant there is | [MM], [ST], [RB] | mnd_07 |
| Queens | immobile, defenceless, a lot of health; a dead queen leaves food behind and an army with no queen can't make ants | [MM] | mnd_07, mnd_19 |
| Win | every red ant down, queens included | [LZ] | mnd_11, routes |
| Lose | no blue ant left (a lone queen can't fight), or you withdraw; losing your queen alone doesn't end the field | [MM] | mnd_07, mnd_16 |
| Red AI: food | beelines for food at all times, from several parts of the field at once; turns for home the moment it picks one up, even mid-fight | [SU], [HW], [CO] | mnd_09 |
| Red AI: scouts | about one new red worker in six walks to your queen first | [SU] | mnd_20 |
| Red AI: few soldiers | the red queens lay at most one soldier in three | [HW] | mnd_20 |
| Red AI: corners | beelines, so it gets stuck on corners | [HW], [PT] | (rules) |
| Red AI: bait | red workers always come at you when they see you and their jaws are empty | [HW], [ST] | mnd_02, mnd_09 |
| Red AI: stuck | a locked soldier keeps spitting at a target it can't hit | [PT], [ST] | mnd_08 |
| Red AI: no food | when the beads are gone it marches on your queen | [LZ] | mnd_09 |
| Queen as a shield | a red worker lured onto your queen only gnaws at her from then on, slowly, whatever passes by; soldiers halted beside her barrage it | [HW], [ST] | mnd_14 |
| Handicap | red has twice the health, about twice the ants and its food next to its queen; left alone it wins | [LZ], [CO], [HW], [OF] | mnd_16 |
| Longlegs | neutral, much bigger, very slow; follows the nearest ant it sees, either colour; eats ants; only spit hurts it; left unshot it clears the field; a dead one leaves a pile of food | [HW], [LZ], [OF] | mnd_10, mnd_19 |
| Luring | walk near a longlegs and it follows you to the reds | [LZ], [OF] | (bot) |
| Branching road | see "Full scale"; your crowned leader walks the road between fields | [PT], [OF], [MM] | mnd_12 |
| Briefings | a letter from your general, with the field's size and both sides' starting ants | [MM], [MH] | mnd_01 |
| 2P versus | blue against red, the same ants on both sides | [MH], [SU] | mnd_13 |
| Goals | Beacon: bring down a longlegs; Saucer: reclaim the Old Colony (field 12); Alien: win every campaign field | [MH], [GG] | mnd_10, mnd_11 |
| Every field can be won | the demo player wins each of the 13 with button presses | — | mnd_r01 … mnd_r13 |

### Readings we had to choose

- **Stepping along an arm (unconfirmed).** The manual lists up to three
  commands per direction ("Up 1 / Up 2 / Up 3") without saying how you reach
  the second and third. Ours: the first press of a direction highlights its
  first command, each further press of the same direction the next one
  (round again at the end). Letting go of A without pressing a direction
  gives the last command again, which is ours. All of this sits in one table
  at the top of `mandibles.h`.
- **Queen commands reach every queen from anywhere.** The manual says
  commands go "generally" to the ants around you; we read the queen
  commands as the exception. `MND_QUEEN_ORDERS_IN_RANGE` in `mandibles.h`
  turns a range check back on.
- **Strafing (unconfirmed).** The manual says you spit "in the direction
  you're facing" and nothing about holding the button, so the d-pad always
  turns you. `MND_STRAFE` in `mandibles.h` locks the facing while B is held,
  should the original turn out to do that.
- **Beads need no button:** walk over one to pick it up, touch your queen
  to hand it over ("they take it into their mouth").
- **Brawls:** 48 frames; each side's strength is its health × (6 for a
  soldier or your ant, 4 for a worker) × a roll of 8 to 12; the stronger
  one survives with its health minus the loser's (at least 1). So a full red
  worker (6) almost always beats your ant (3) and keeps 3, and a blue
  soldier at 3 usually beats a red worker at 3. Ants that touch someone
  already brawling wait their turn.
- **Respawn:** 2½ seconds after you die, at a living queen, provided a blue
  worker or soldier lives (the manual's "other blue ants"; we don't count
  the queen). With no queen you don't come back, but your ants keep the
  field.
- **Food left behind:** 5 beads for a dead queen, 7 for a longlegs.
- **Numbers no source gives:** your ant walks half a pixel a frame; an AI
  ant takes a one-tile step in 16 frames and then stops for 18; you spit
  every 18 frames, blue soldiers every 24, red every 26; a spit flies 56 px
  at 2 px a frame; ants see 64 px, soldiers stand and spit from 48 px; the
  shout radius is 40 px.
- **Mistakes:** a blue step goes a random way one time in four (the source's
  25 %), one in two once a Follow is stale; a red one goes astray one time in
  ten ("there's more of them so it doesn't matter").
- **Queens:** blue 20 health, red 40 (twice, like every red ant); a gestation
  takes a second and a half; workers gnaw an enemy queen for 1 a second.
- **Longlegs:** 45 health, 3 a bite every ¾ s, under a fifth of a pixel a
  frame, remembers where it last saw prey for three seconds, doesn't brawl.
  Spit from both sides hurts it; only your spit counts for the goal.
- **Scouts:** one new red worker in six; it walks until it is within six
  tiles of your queen, then goes back to the food.
- **Mission numbering:** twelve fields with the capital as the 12th; the
  bonus field shows on the map once the capital is taken and counts for no
  goal. Our field 10 (the Deathsdoor of ours) is skippable through field 9.
- **Versus:** both sides get blue's numbers (the "balanced" mode), three
  mirrored fields, and the win count is kept until you leave.

## What is ours

- **Name:** MANDIBLES (1989, Beamdown Softworks).
- **Setting:** the Bluebell Colony (blue) against the Rust Horde (red), a
  garden and a meadow on the way back to the Old Colony. General Stag writes
  the briefings; the giant spiders are the longlegs, the food is sap beads.
- **Command names:** LAY WORKERS, LAY SOLDIERS, WITHDRAW, FALL IN, SQUAD
  FALL IN, FREE WILL, SQUAD FREE WILL, HALT, SQUAD HALT.
- **Goal names:** BRING DOWN A LONGLEGS, RECLAIM THE OLD COLONY, TAKE EVERY
  FIELD.
- **All 16 field layouts** (12 campaign, the bonus, 3 versus), their names,
  the campaign map and every line of text (briefings, the ending, menus).
- **Pixel art:** top-down ants in three views (flipped to eight), queens,
  longlegs, beads, spit, brawl clouds, rocks, roots, grass, water, the
  general.
- **Music:** "Bluebell Colony" (title), "Orders From Above" (map), "Molasses
  March" (fields), "The Old Halls" (capital and bonus), "Picnic War"
  (versus), and the jingles "Field Taken" and "Back to the Nest".

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu (with RESTART
MISSION and RETREAT TO MAP), saving (fields taken, longlegs slain, the map
cursor, plays and losses) and the three UFO 40 goals, which are Combatants'
own. On the Vita (one controller) 2P versus is shown but locked.

## Controls

| Input | Action | Status |
|---|---|---|
| D-pad | walk (8 ways) | confirmed |
| B | spit the way you face; hold to keep spitting | confirmed |
| hold A | command cross | confirmed |
| while A is held: a direction, let go of A | highlight a command, give it | confirmed |
| the same direction again | the arm's next command | our reading |
| tap A | the last command again | ours |
| START | pause | UFO 40 |

UFO 50's Button 1 (the owner's Z, the manual's "A") and Button 2 are our A
and B.

## Sources

- [MM] Steam guide "The missing manuals - How to play UFO 50 games",
  section 46: the controls, the command menu and its layout, Follow,
  melee, respawn, losing, queens and their food, the map and briefing
  screens. https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [MH] UFO 50 Wiki (Miraheze), "Combatants": the goal, units and costs,
  health, the difference rule, followers spitting, the twelve levels, the
  bonus level, the goals. https://ufo50.miraheze.org/wiki/Combatants
- [SR] speedrun.com level list (the twelve missions and the bonus, in order).
  https://www.speedrun.com/api/v1/games/v1pl7876/variables
- [TT] TV Tropes recap: fights in a cloud with stars, the "!" blink,
  instinct ants walking straight at the nearest food.
  https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game46Combatants
- [LZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 46": orders, the
  queen's switch, the AI, the scrum, the spider following you, winning by
  killing every red ant, 2–3 red queens.
  https://lizstar64.github.io/reviews/2024/10/20/UFO50-46.html
- [HW] Steam, "How are you meant to play combatants?": raiding, AI quirks,
  eight-direction soldiers that don't re-aim, few red soldiers, starting
  soldiers, luring onto your queen, spiders, re-shouting Follow, the food
  through the wall. https://steamcommunity.com/app/1147860/discussions/0/592888815005273090/
- [ST] Steam, "Struggling with Combatants": halting new ants, the queen as a
  shield, only your ant respawns, dying is the fastest way to new ants.
  https://steamcommunity.com/app/1147860/discussions/0/4849903998513025145/
- [PT] Steam, playtesting thread: the north road past Mission 3, line of
  sight, Instinct. https://steamcommunity.com/app/1147860/discussions/0/4852155320355414724/
- [CO] Steam, "Combatants": half the health, red workers, soldiers that
  never change direction, followers spitting every which way.
  https://steamcommunity.com/app/1147860/discussions/0/592887778739529134/
- [SU] Steam, "Combatants sucks ass": shout radius, 25 % bad steps, orders
  worthless after 1–2 s, 3 and 6 shots, red scouts.
  https://steamcommunity.com/app/1147860/discussions/0/4849903793441417087/
- [OF] Steam, "Open Field": twice the starting units, food piled at their
  queens, luring the spider between two queens, soldiers that can't turn,
  spiders feeding whoever kills them, the final beaten first.
  https://steamcommunity.com/app/1147860/discussions/0/4700161008287353037
- [RB] Steam, rebalancing thread: infinite respawns "with basically zero
  drawback". https://steamcommunity.com/app/1147860/discussions/0/6757179594727493195/
- [DD] Steam guide "Combatants Strategy: DEATHSDOOR": soldiers first, Soldier
  Follow, kiting the spider, one-shotting with a stack.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3337984631
- [GG] Steam guide "Gift, Gold & Cherry": the three goals.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [PD] Pixeldie, "Ranking every UFO 50 game": "walk through molasses".
  https://pixeldie.com/2024/11/13/ranking-every-ufo-50-game-after-100-hours/
- [CTRL] UFO 40 research notes, `controls-41-44-46.md`: the owner's check of
  the prompts, "!" on ants that take an order.
