# 46 · MANDIBLES

*Internal design document. Not shown in the product.*

## Tribute to

**Combatants** (UFO 50 game #46, Mossmouth). The rules were researched from
text only: the community wiki, written reviews and Steam threads and guides
(listed under Sources; the notes are in the research folder,
`46-combatants.md` and `controls-41-44-46.md`). The controls were confirmed
by the owner from the original's own on-screen prompts. No UFO 50 images,
video, sprites, music, maps or text were used as references.

**Map type: fixed, hand-made.** Combatants is a string of hand-made fields
on a branching road to the capital, so ours are too
(`mandibles_maps.c`, all our own layouts). Only the structure follows the
original: red placed better and next to its food, two or three red queens
on the later fields, giant spiders on some, one pointless extra field.

| Full scale | Combatants | MANDIBLES |
|---|---|---|
| Campaign fields | 12, gating the way to the final one, "retake the capital" [MH] | 12, the 12th is the capital |
| Route | branching: "skip Mission 3 entirely by going north instead of west" [PT]; the final can be beaten before the hard optional fields [OF] | branching: field 2 opens 3 (west) and 4 (north); the shortest road to the capital is 7 fields; 8 and 11 are side roads |
| Bonus field | a hidden, pointless extra mission [MH] | POINTLESS HILL, shown once the capital is taken |
| Modes | 1P campaign, 2P versus [MH] | the same two; 3 versus fields |
| Ant types | your soldier ant, workers, soldiers, queens [MH] | the same four, blue and red |
| Orders | Follow, Halt/Stay, Instinct, Soldier Follow; the queen's workers/soldiers switch [LZ], [DD], [ST], [MH] | the same five |
| Neutral creatures | giant spiders [HW], [OF] | the longlegs, on 7 of the 13 fields (two on the capital, three in the garden) |
| Goals | 3 [MH] | the same 3 |

What each field brings: 1 the basics, one red queen; 2 a red soldier; 3 the
Gauntlet, a walled road with a soldier on every narrow (the wall of the
game); 4 the easy road north; 5 the first longlegs, a stream with fords; 6
two red queens; 7 a single pass with a longlegs asleep in it; 8 the wide
lawn, miles of grass, two red queens on a heap of sap, twice our numbers and
a longlegs to lead over (the Open Field of ours); 9 rows of roots that break
sight lines; 10 three queens behind one wall with a single gap (the
hardest); 11 three longlegs and two red queens that nobody needs to fight
(the cherry field); 12 the capital, three queens and two longlegs, the one on
the east side roaming through the red half; 13 the bonus.

**The difficulty curve**, measured with the demo player (a skilled cheeser:
it kites, stands off at angles, leads the longlegs and never panics), in
eight seeds per field: fields 1, 4 and 13 every time; 2, 6 and 12 seven
times; 5 six; 9 five; 3 and 7 four; 11 three; 10 twice; 8 once. That is
the original's shape: a gentle start, a wall at 3 with an easy road round
it, the Open Field and DEATHSDOOR as the hardest fields, a cherry field just
as bad, and a capital that players often take first try [PT], [OF]. With no
input at all the reds win (mnd_16).

## Mechanics checklist

| Mechanic | How MANDIBLES does it | Source | Test |
|---|---|---|---|
| Controls | d-pad walks; hold A opens the command menu; B spits | owner's check of the on-screen prompts ("hold Z: command menu", "E: spit"); [CTRL] | mnd_02, mnd_04 |
| Your ant | one blue soldier ant; walks 8 ways and spits the way it faces | [MH], [HW] | mnd_02 |
| Eight directions | spit (yours and every soldier's) flies in one of 8 directions | [HW], [CO] | mnd_02, mnd_08 |
| Slow | everything walks slowly; your ant is faster than the AI ants, so none can catch it | [PD], [HW], [PT] | mnd_02 |
| Shout radius | orders reach only your ants within a few blocks (five tiles) | [SU] | mnd_04 |
| Follow | ants trail you | [MH], [LZ] | mnd_04 |
| Soldier Follow | the same, soldiers only | [DD] | mnd_04, mnd_05 |
| Halt | they stand where they are; soldiers spit at anything that comes near | [MH], [ST] | mnd_04, mnd_14 |
| Instinct | the AI takes over: fetch beads it can see and carry them home, fight what it sees | [PT], [LZ] | mnd_06 |
| New ants | hatch on Instinct and wander off ("tell them to halt as soon as they come out") | [ST] | mnd_06 |
| "!" | an ant that took an order shows "!" and you hear the shout | [CTRL] | mnd_04 |
| Followers spit with you | soldiers on Follow spit when you spit, the way you face; a stack of five and you put six into one ant | [MH], [DD] | mnd_05 |
| Stale orders | friendly ants are worthless unless ordered in the last 1–2 s: a Follow goes stale after 2 s | [SU] | mnd_05 |
| Stumbling | one step in four goes the wrong way, ordered or not | [SU] | (rules) |
| Out of sight, out of mind | ants act only on what they can see; a follower that loses sight of you for half a second forgets you | [PT], [SU] | mnd_18 |
| Shouting again | a fresh order skips an ant's pause after each step, so shouting Follow at every pause moves the squad twice as fast | [HW] | (bot) |
| Sap beads | green beads lie on the field; walk over one to carry it, touch your queen to feed her | [MH], [ST] | mnd_06 |
| Costs | worker 1 bead, soldier 2; the queen lays whichever she is set to | [MH], [LZ] | mnd_06 |
| The queen's switch | at your queen the menu's last line turns her from workers to soldiers and back | [LZ], [MH] | mnd_04, mnd_06 |
| Health | blue ants 3, red ants 6 ("die in 3 shots", "die in 6") | [MH], [SU], [CO] | mnd_02 |
| A spit | does 1 | [SU], [DD] | mnd_02 |
| Melee | ants that touch fight at once: the healthier one lives with the difference (one red worker can kill you and a soldier) | [MH], [CO] | mnd_03 |
| Respawn | only your ant comes back; it hatches again at your queen for a bead while she is fed, and dying is the cheapest ant there is | [MH], [ST] | mnd_07 |
| Win | every red ant down, queens included | [LZ] | mnd_11, routes |
| Lose | your queen dies, or you are dead and she stays empty | [MH] | mnd_07, mnd_16 |
| Red AI: food | beelines for food at all times, from several parts of the field at once; turns for home the moment it picks one up, even mid-fight | [SU], [HW], [CO] | mnd_09 |
| Red AI: corners | beelines, so it gets stuck on corners | [HW], [PT] | (rules) |
| Red AI: bait | red workers always come at you when they see you and their jaws are empty | [HW], [ST] | mnd_02, mnd_09 |
| Red AI: locked spit | a red soldier picks its direction when it first spits and keeps it until the target dies or leaves its range; come at it from an angle | [HW], [OF], [CO], [DD] | mnd_08 |
| Red AI: stuck | it keeps spitting at a target it can't hit | [PT], [ST] | mnd_08 |
| Red AI: no food | when the beads are gone it marches on your queen | [LZ] | mnd_09 |
| Queen as shield | enemy workers that reach a queen only nibble her; soldiers halted beside her barrage them | [HW], [ST] | mnd_14 |
| Handicap | red has twice the health, more ants and its food next to its queen; left alone it wins | [LZ], [CO], [HW] | mnd_16 |
| Longlegs | neutral, much bigger, very slow; follows the nearest ant it sees, either colour; eats ants; only spit hurts it; left unshot it clears the field | [HW], [LZ], [OF] | mnd_10 |
| Luring | walk near a longlegs and it follows you to the reds | [LZ], [OF] | (bot) |
| Branching road | see "Full scale" | [PT], [OF] | mnd_12 |
| Briefings | each field opens with a letter from your general | [MH] | mnd_01 |
| 2P versus | blue against red, the same ants on both sides | [MH], [SU] | mnd_13 |
| Goals | Beacon: slay a longlegs; Saucer: retake the capital; Alien: win every campaign field | [MH] | mnd_10, mnd_11 |
| Every field can be won | the demo player wins each of the 13 with button presses | — | mnd_r01 … mnd_r13 |

### Readings we had to choose

- **The command menu (unconfirmed layout).** The owner confirmed "hold
  button 1: command menu", but no source shows the menu itself. Ours is a
  list of five lines: FOLLOW, SOLDIER FOLLOW, HALT, INSTINCT and, while your
  queen is in earshot, QUEEN: MAKE SOLDIERS / WORKERS. Up and down move the
  cursor, letting go of A shouts the line under it, and a quick tap shouts
  the same order again. You stand still while the menu is open. All of this
  is in one table at the top of `mandibles.h` (`MND_MENU_*`, `MI_*`), marked
  unconfirmed, so it can change in one place.
- **Beads need no button (unconfirmed):** walk over one to pick it up, touch
  your queen to hand it over.
- **Holding B keeps your facing** (you can back away while spitting).
- **Numbers no source gives:** your ant walks half a pixel a frame; an AI
  ant takes a one-tile step in 16 frames and then stops for 18 (so a shout
  at every stop doubles its speed); you spit every 18 frames, blue soldiers
  every 24, red every 26; a spit flies 56 px at 2 px a frame; ants see
  64 px, soldiers stand and spit from 48 px; the shout radius is 40 px.
- **Mistakes:** a blue step goes a random way one time in four (the source's
  25 %), one in two once a Follow is stale; a red one goes astray one time in
  ten ("there's more of them so it doesn't matter").
- **Queens:** blue 20 health, red 40 (twice, like every red ant); a gestation
  takes a second and a half; workers nibble an enemy queen for 1 a second.
- **Respawn:** a second and a half after you die, for one bead. A queen
  never spends her last bead on ants while you might need it, and if you are
  dead with her empty for ten seconds the field is lost.
- **Longlegs:** 45 health, 3 a bite every ¾ s (a blue ant in one bite, a red
  one in two), it moves at under a fifth of a pixel a frame, remembers where
  it last saw prey for three seconds, and it doesn't fight back against
  melee. Spit from both sides hurts it; only your spit counts for the goal.
- **Mission numbering:** the wiki's "twelve levels that gate progression to
  the final 12th level" is read as twelve fields with the capital as the
  12th.
- **The bonus field** shows on the map once the capital is taken (how the
  original reveals it isn't written down); it counts for no goal.
- **Versus:** both sides get blue's numbers (the "balanced" mode), three
  mirrored fields, and the win count is kept until you leave.

## What is ours

- **Name:** MANDIBLES (1989, Beamdown Softworks).
- **Setting:** the Bluebell Colony (blue) against the Rust Horde (red), a
  garden and a meadow on the way back to the Old Colony. General Stag writes
  the briefings; the giant spiders are the longlegs, the food is sap beads.
- **All 16 field layouts** (12 campaign, the bonus, 3 versus), their names,
  the campaign map and every line of text (briefings, the ending, menus).
- **Pixel art:** top-down ants in three views (flipped to eight), queens,
  longlegs, beads, spit, rocks, roots, grass, water, the general.
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
| B | spit; hold to keep spitting (your facing stays while B is held) | confirmed (the button); the hold is ours |
| hold A | command menu | confirmed |
| in the menu: ↑ ↓, let go of A | pick a line, shout it | our reading |
| tap A | shout the last order again | our reading |
| START | pause | UFO 40 |

UFO 50's Button 1 (the owner's Z) and Button 2 are our A and B.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Combatants": the goal, units and costs,
  health, melee, orders, followers spitting, respawn, the twelve levels,
  the bonus level, the goals. https://ufo50.miraheze.org/wiki/Combatants
- [LZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 46": orders, the
  queen's switch, the AI, the spider following you, winning by killing every
  red ant, 2–3 red queens. https://lizstar64.github.io/reviews/2024/10/20/UFO50-46.html
- [HW] Steam, "How are you meant to play combatants?": raiding, AI quirks,
  eight-direction soldiers that don't re-aim, spiders, re-shouting Follow.
  https://steamcommunity.com/app/1147860/discussions/0/592888815005273090/
- [ST] Steam, "Struggling with Combatants": halting new ants, the queen as a
  shield, only your ant respawns. https://steamcommunity.com/app/1147860/discussions/0/4849903998513025145/
- [PT] Steam, playtesting thread: the north road past Mission 3, line of
  sight, Instinct. https://steamcommunity.com/app/1147860/discussions/0/4852155320355414724/
- [CO] Steam, "Combatants": half the health, red workers, soldiers that
  never change direction. https://steamcommunity.com/app/1147860/discussions/0/592887778739529134/
- [SU] Steam, "Combatants sucks ass": shout radius, 25 % bad steps, orders
  worthless after 1–2 s, 3 and 6 shots.
  https://steamcommunity.com/app/1147860/discussions/0/4849903793441417087/
- [OF] Steam, "Open Field": two queens, luring the spider between them,
  soldiers that can't turn once they shoot, the final beaten first.
  https://steamcommunity.com/app/1147860/discussions/0/4700161008287353037
- [DD] Steam guide "Combatants Strategy: DEATHSDOOR": soldiers first, Soldier
  Follow, kiting the spider, one-shotting with a stack.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3337984631
- [PD] Pixeldie, "Ranking every UFO 50 game": "walk through molasses".
  https://pixeldie.com/2024/11/13/ranking-every-ufo-50-game-after-100-hours/
- [CTRL] UFO 40 research notes, `controls-41-44-46.md`: the owner's check of
  the prompts, the option set, "!" on ants that take an order.
