# 14 · CUTLASS CUP

*Internal design document. Not shown in the product.*

## Tribute to

**Bushido Ball** (UFO 50 game #14, Mossmouth). The rules were researched from
text only: the community wiki, Steam guides and threads, and written reviews
(listed under Sources). No UFO 50 images, video, sprites, music or text were
used as references.

**Map type: one fixed court.** Bushido Ball is played on a single arena, so
there are no levels. Ours follows the described structure: a centre line, a
starting circle on each side, a line near each circle that marks how far past
the centre a player may go, rails along the top and bottom that the ball
bounces off, open ends (a ball that leaves there is a point), and the judge at
the bottom middle.

## Mechanics checklist

Every line was compared with the code; the tests named prove it with button
presses.

| Mechanic | How CUTLASS CUP does it | Source | Test |
|---|---|---|---|
| The sport | strike the ball past your rival and off the far edge for a point | [W] | cc_01 |
| Winning | first to 8 points; POINTS can be 4 to 20 | [W], [SC] | cc_12, cc_14 |
| Moving | 8 ways, past the centre up to the line just in front of the rival's circle, close enough to strike a rival waiting there | [W], [MM], [SB] | cc_16 |
| The serve | the judge at the bottom middle rolls the ball along the deck to whoever lost the last point | [MM] | cc_01 |
| Strike (B) | up or down angles it; back launches it into the air (a lob) | [W], [G] | cc_02 |
| Rally speed | every return is faster than the last; a lob adds nothing, nor does returning one; a new point starts slow | [SH] | cc_03 |
| Roll (A) | a quick move the way you are going, steerable, cancelled by a strike | [W], [MM] | cc_03 |
| Rolling strike | a big speed boost; rolling up or down, a curve shot that swerves and bounces hard off the rails | [W], [G] | cc_03, cc_20 |
| Back of a roll | the back third of a rolling body knocks the ball backwards | [W] | cc_19 |
| Sweet spot | a strike at the very start of the swing wallops the ball | [SC] | cc_03 |
| Body hits | the ball bounces off and stuns that player; no meter either way | [W] | cc_04 |
| Meter | every 2 weapon strikes fill half a bar; 3 bars at most; each player's in their side's top corner | [W], [G], [L], [MM] | cc_02 |
| Trick (B twice) | half a bar; a hit stuns, pushes back and takes half a bar | [W], [G] | cc_05 |
| Broadside (hold B) | one bar; with spare bars a longer charge upgrades it (faster, or more fakes), still for one bar | [W], [L] | cc_06 |
| Struck back | a Super Ball (always bright orange) struck with a weapon or trick flies back as one | [W] | cc_08 |
| Caught | a Super Ball that hits your body is caught and you slide back; mash B to send it back, or it slips over your head | [W] | cc_07 |
| Fouls | dawdling (the ball too long on your side), a blade on the rival (who is stunned), striking the other side's serve first (not with an urchin), after which play goes on; the third gives the rival a point and clears; shown as red marks on the bottom rail beside the judge | [W], [G], [MM], [SB] | cc_09, cc_10 |
| FOULS off | an option | [W] | cc_11 |
| Options | POINTS, TIME (none or 1 to 10 minutes), FOULS, SPEED (three) | [W] | cc_14, cc_15 |
| No music in play | only a jingle when a point is won; music on the menus | [L], [P] | cc_01 |
| Tournament | five matches against the other five fighters in a random order (so the final rival varies), each rival tougher | [W], [SF], [P] | cc_12, cc_21 |
| Continues | as many as you like; the Alien needs a run with none | [W], [SF] | cc_13 |
| 2P Versus | on PC and the web; the Vita has one controller | [W] | cc_16 |
| 2P Co-op | doubles against CPU pairs; two wins clear the other four | [W], [SH] | cc_17 |
| The CPU | returns almost everything, but guesses worse the faster the ball; a lob over it near the middle fools it; it never fouls on purpose; the last rivals roll to where the ball is going | [SB], [SF], [W] | cc_20, cc_21 |

### The six fighters

Stats are Speed / Control / Power, 1 to 3, as in [W]. Kits follow [W] and
[G] one for one; names, looks and weapons are ours.

| Ours | Bushido Ball | S/C/P | Reach | Trick (half a bar) | Broadside (one bar) |
|---|---|---|---|---|---|
| FINN, the young corsair | Kotaro | 2/2/3 | medium | a lucky coin thrown at the rival; it knocks the ball once and can be struck back once; medium stun (short if struck back) | Comet: very fast in the chosen direction |
| MAE, the deckhand | Ayumi | 3/2/2 | shortest (12 × 9) | lobs a sea urchin; struck, it slides forward, and the rival's changes sides; the ball touching it goes toward the owner's rival, downward unless up was held when it was thrown or struck; long stun | Wall Runner: turns to the chosen rail and slides along it |
| GRETA, the harpooner | Tomoe | 2/3/2 | tall and long (21 × 22) | a harpoon fired at the ball pulls it in and strikes it, unless caught at full range; it can't catch a high, fast or far ball; medium stun | Mirage: one fake per level beside the real one, looking just like it (no faster per level) |
| SILAS, the sabre dancer | Raizo | 2/1/3 | thin and long (26 × 7) | the squall, a flurry that travels forward hitting the ball and anyone in it again and again and cutting through the other side's tricks, farther with more than one bar; a ball struck while it is out becomes a Super Ball; medium stun | Whirl: flies ahead, loops twice, then turns; with forward held it curves down at once instead |
| WREN, the shadow | Chiyome | 3/1/2 | short but tall (13 × 18) | a dart straight ahead; can be struck back; medium stun. She blinks a set distance instead of rolling, leaving a decoy barrel the ball bounces off, so she has no curve shot | Sea Fog: the ball turns invisible, trailing smoke, and curves |
| BRUNO, the gunner | Yamada | 1/3/2 | very thin, covering most of the deck from the middle (9 × 58) | lobs a powder pot that blows up after a while or when struck; the blast stuns (long) and throws the ball a little into the air with a strong curve, whatever its height. He rolls only forward and back; up or down thrusts his trident a long way | Plunge: runs up or down to the middle line, then turns 90° toward the rival |

### Readings we had to choose

- **Numbers no source gives:** moving 0.95 / 1.3 / 1.65 px a frame by
  Speed; striking 2.9 / 3.3 / 3.8 by Power; aiming up to 17° / 31° / 45° by
  Control; each return +0.5 up to +7; a roll strike ×1.4; the sweet spot
  ×1.25; Brisk and Gale ×1.25 and ×1.5; a broadside after 12 frames of
  holding, each upgrade 36 more; dawdling after 5 s.
- **The first serve** goes to either side at random; a serve nobody touches
  goes off that side like any ball.
- **A Super Ball struck back** stays one and comes back faster, like any
  return.
- **"Pushed backwards"** by a rolling body: toward the roller's own side.
- **The blade foul** stuns the rival too (a thread's tactic depends on it).
- **TIME** ending: the leader wins; a tie goes to the next point.
- **Traps:** two urchins or pots out at most.

## What is ours

- **Name:** CUTLASS CUP (1985, Beamdown Softworks).
- **Setting:** the Cutlass Cup, played every summer on the deck of the old
  galley *Merry Mackerel*, moored in a whitewashed harbour. The judge is old
  Bosun Crabbe, who rolls the ball out from the bottom rail and hands out red
  marks.
- **The six fighters**, their weapons, the trick and broadside names, the
  foul and option names (dawdling, blade foul, early swing; POINTS, FOULS,
  CALM, BRISK, GALE) and all art.
- **Music:** a title tune, a crew-select tune, the bracket theme, the cup
  theme and jingles. As in the original, there is no music during play.

## Additions: none

Only the platform needs every UFO 40 cartridge has: the START pause menu,
saving the options and the tournament record, and the three UFO 40 goals,
which are Bushido Ball's own (gift: 3 opponents; gold: the tournament;
cherry: no continues). 2P Versus and Co-op need a second player: two gamepads
or a split keyboard on PC and the web; on the Vita only the 1P tournament is
offered. Removed as invented: fighter names and the goal count in the HUD,
player tags over the fighters, button hints on the select and continue
screens, the continue countdown, the title's cups and best-run line, the
bracket's continue count, the result card's opponent count and goal icons,
a re-serve after an intercepted serve, and wording copied from UFO 50
("Secondary Weapon", "Super Shot", "Laws", "Hyper", the three foul names).

## Controls

| Input | Action |
|---|---|
| D-pad | run |
| B | strike (up/down: angle, back: lob) |
| B twice | trick (half a bar) |
| hold B | charge a broadside (one bar) and swing on release |
| A | roll (Wren: blink; Bruno up/down: thrust) |
| START | pause menu |

Split keyboard for two players: player 1 WASD + F (roll) / G (strike),
player 2 arrows + K (roll) / L (strike).

## Not confirmed

- The exact speed curve, stun lengths and charge times.
- Whether a Super Ball struck back stays one, and which way a rolling body
  pushes the ball.
- Mae's urchin: the Bushido guide sends the ball toward the urchin's owner;
  ours follows the wiki's tip (toward the owner's rival).
- Co-op's exact rules (ours: two players against CPU pairs).
- Open, not done: late CPUs (level 3 and up) deliberately striking urchins
  on their side; the originals' stats (fighters used, fighters won with),
  which would need a platform stats line.

The final rival is random, as ours is: the same thread that first seemed to
tie it to your fighter ends "so I guess it's random" [SF].

## Tests

`tests/cc_01` … `cc_22`, all driven by button presses. `cc_21_demo_tournament`
is a demo player (`cutlass.c`, the `bot` query) that presses real buttons the
way the best CPU plays and, continuing when it loses, beats three rivals for
the Beacon. `cc_22_no_stall` plays three best-CPU matches to the end.

## Sources

- [W] UFO 50 Wiki (Miraheze), "Bushido Ball": controls, points, the arena
  lines, the roll and its back third, rolling strikes and curve shots, the
  meter, secondary weapons and Super Shots (upgrades take longer to charge
  but cost one bar), stopping and catching Super Balls, body hits, the fouls
  and penalty points, all six characters, options, modes, goals.
  https://ufo50.miraheze.org/wiki/Bushido_Ball
- [MM] Steam guide "The missing manuals - How to play UFO 50 games": 8-way
  movement, the referee at the bottom middle rolls the ball to whoever lost
  the last point, the roll is a fast move the way you were going, three
  infractions give the point, up to 3 meters.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [G] Steam guide "Bushido Ball": forward/up/down strikes, back lobs, dash +
  up/down spins the ball, half a bar per 2 hits, half a bar lost to a special,
  Tomoe's vertical reach, Yamada's reach from the centre and weakness to
  lobs, Raizo's piercing wave, the three penalties.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3339728484
- [SH] Steam thread "Is Bushido Ball too hard?": the ball gets too fast after
  a couple of volleys, the ball can't build speed when returning a lob,
  penalties off, co-op needs two wins.
  https://steamcommunity.com/app/1147860/discussions/0/595144212454885425/
- [SF] Steam thread on the final match: the last CPU rolls straight to where
  the ball is going, the final rival varies ("so I guess it's random"), lob
  close to the middle, 30 to 40 continues.
  https://steamcommunity.com/app/1147860/discussions/0/4852155152087803136/
- [SB] Steam thread "Bushido Ball Help?": the AI returns shots almost
  perfectly, lobs confuse it, rolling at the rival and hitting them with
  penalties off, Raizo's wave hitting ball and rival together.
  https://steamcommunity.com/app/1147860/discussions/0/523083510998705141/
- [L] Lizstar's Trashcan, "UFO 50 Retrospective Part 14": half a meter per
  two hits, double tap, hold for the full charge, co-op doubles, no music
  during play except a jingle. https://lizstar64.github.io/reviews/2024/10/12/UFO50-14.html
- [SC] Static Canvas, "The UFO 50 Diaries: Bushido Ball": 8 points, a well
  timed hit wallops the ball. https://staticcanvas.substack.com/p/the-ufo-50-diaries-bushido-ball
- [P] Popcar's Blog, "Reviewing Every Single UFO 50 Game": every fighter plays
  differently, no in-game music, the campaign gets harder.
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
