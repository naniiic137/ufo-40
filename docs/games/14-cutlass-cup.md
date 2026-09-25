# 14 · CUTLASS CUP

*Internal design document. Not shown in the product.*

## Tribute to

**Bushido Ball** (UFO 50 game #14, Mossmouth). The rules were researched from
text only: the community wiki, written reviews, Steam guides and forum threads
(listed under Sources). No UFO 50 images, video, sprites, music or text were
used as references.

**Map type: one fixed court.** Bushido Ball is played on a single arena, so
there are no levels to generate or lay out. Our court follows the described
structure: a centre line, a starting circle on each side, and a line near each
circle that marks how far past the centre a player may go. Walls run along the
top and bottom; the left and right edges are open (a ball that leaves there is
a point).

## Mechanics checklist

| Mechanic | How CUTLASS CUP does it | Source |
|---|---|---|
| The sport | hit the ball with your weapon past your opponent and off the far edge for a point | [W], [L], [P] |
| Winning a match | first to 8 points by default; GOAL can be set from 4 to 20 | [W], [SC] "8 points for a victory" |
| Movement limit | each player may go past the centre, up to the line nearest the opponent's starting circle | [W] |
| Strike (Button I = our B) | a swing in front of you; up / down changes the angle; back launches the ball into the air (a lob) | [W], [G] |
| Roll (Button II = our A) | a quick roll; you can change direction mid-roll and cancel it with a strike | [W], [G] |
| Rolling strike | striking during a roll gives the ball a big speed boost; rolling up or down and striking is a Curve Shot | [W], [G] |
| Airborne balls | a lobbed ball flies over players; it can only be struck again once it is low | [W], [T] |
| Body hits | a ball that hits a body bounces off and stuns that player briefly; body hits give no meter | [W] |
| Meter | every 2 weapon strikes fill half a bar; up to 3 bars | [W], [G], [L] |
| Secondary weapon | double-tap strike with at least half a bar; costs half a bar; a hit stuns, pushes back and removes half a bar | [W], [G] |
| Super Shot | hold strike with at least one full bar; costs one bar; extra full bars make it faster at no extra cost | [W], [L] |
| Stopping a Super Shot | strike it with your primary or secondary weapon to send it back; if it hits your body you catch it, slide back and must mash strike to reflect it, or it slips over your head | [W] |
| Sweet spot | a strike timed at the very start of the swing wallops the ball | [SC] "if you hit the ball at exactly the right time it absolutely wallops it" |
| The judge's serve | the judge serves the ball to one player; the other may not touch it first | [W], [G] |
| Laws (fouls) | Stalling (the ball stays on your side too long), Weapon Foul (hitting your opponent's body with your primary weapon), Serve Interference (touching the other player's serve first, even with a secondary weapon) | [W], [SG] |
| Penalty | a third foul ends the point at once, gives the opponent 1 point and clears the fouls; fouls show as marks on each side | [W], [G] |
| Options | GOAL 4–20, TIME none or 1–10 minutes, LAWS on/off, SPEED Normal / Fast / Hyper | [W], [D] |
| No music in play | only a short jingle when a point is won; music on the menus | [L] "no sound or music during the game, except for when a player wins a round"; [P] |
| 1P Tournament | beat the other five fighters, one match each | [SG2] "five consecutive 1v1 matches" |
| Continues | lose a match and you may continue (retry it); the Alien needs a run with none | [W], [SD] |
| 2P Versus | two players, one match | [W] |
| 2P Co-op | the two players team up for doubles (2 v 2) against CPU pairs; two wins clear the other four fighters | [SG2], [L] "co-op ... a kind of doubles tennis game" |
| CPU | returns ordinary volleys very reliably, is fooled by lobs (it chases the ball, not where it will land), struggles at Hyper speed, uses its fighter's gimmicks, and never breaks the laws on purpose | [SD], [SH], [W] "The Bots do not try to break the rules" |

### The six fighters

Stats are Speed / Control / Power, 1–3, as in [W]. Kits follow [W] and [G] one
for one; names, looks and weapons are ours.

| Ours | Bushido Ball | S/C/P | Reach | Secondary (half bar) | Super Shot (one bar) |
|---|---|---|---|---|---|
| HAMDI, corsair with a scimitar | Kotaro | 2/2/3 | medium | a spinning coin thrown straight ahead; can be struck back; medium stun (short if reflected) | Comet: a very fast ball in the chosen direction |
| LEILA, quick deckhand with a dagger | Ayumi | 3/2/2 | shortest | lobs a sea urchin that stays on the deck; struck, it slides and changes sides; the ball touching it shoots toward the owner's foe, downward unless up is held; long stun | Wall Runner: the ball turns to the chosen wall and slides along it |
| NOUR, harpooner with a boat hook | Tomoe | 2/3/2 | tallest and long | a harpoon line that pulls the ball in and strikes it automatically, unless caught at full range; can't catch airborne, too fast or too far balls; medium stun | Mirage: fake balls fly beside the real one (one more per extra bar) |
| KARIM, sabre dancer | Raizo | 2/1/3 | longest sideways | a flurry of slashes that travels forward, hitting the ball again and again and cutting through the other side's weapons; goes farther with more bars; medium stun | Whirl: the ball flies ahead, loops twice, then turns (down if forward is held) |
| ZINA, the masked shadow | Chiyome | 3/1/2 | medium | a dart thrown straight ahead; can be struck back; medium stun. She blinks a set distance instead of rolling, leaving a decoy barrel the ball bounces off, so she has no Curve Shot | Djinn: the ball turns invisible, leaving smoke, and curves |
| OMAR, gunner with a trident | Yamada | 1/3/2 | thin but tall | lobs a powder pot that blows up after a while or when struck; the blast stuns (long) and launches the ball in a high curve, whatever its height. He only rolls forward and back; rolling up or down thrusts his trident | Plunge: the ball runs vertically to the middle line, then turns 90° toward the foe |

### Readings we had to choose

- **Court numbers:** the court is 320 × 150 pixels between the rails; the reach
  lines sit 48 pixels past the centre.
- **Speeds:** Speed 1/2/3 moves at 1.1/1.35/1.6 px per frame; Power 1/2/3 hits
  at 3.0/3.4/3.8; Control 1/2/3 aims up to 20°/32°/45°. Fast and Hyper
  multiply everything by 1.25 and 1.5.
- **The serve:** the judge serves toward whoever lost the last point (the first
  serve goes to player 1); a serve nobody touches goes off that side like any
  ball.
- **Stalling:** five seconds on one side.
- **Weapon Foul:** the victim is stunned briefly too.
- **Tournament order:** the five opponents come in a random order.
- **TIME:** when time runs out the leader wins; a tie goes to the next point.
- **Traps:** each fighter can have at most two sea urchins or powder pots out.
- **The sweet spot:** the first active frame of a swing adds 25% speed.

## What is ours

- **Name:** CUTLASS CUP (1985, Beamdown Softworks).
- **Setting:** the Cutlass Cup, played every summer on the deck of the old
  galley *Sabra*, moored in a whitewashed harbour. The judge is old Rais
  Mabrouk, who serves the ball from the rail and hands out red marks.
- **The six fighters**, their weapons, gimmick names and all art.
- **Music:** a title tune, a crew-select tune, the bracket theme and jingles.
  As in the original, there is no music during play.

## Additions: none

Only the platform needs every UFO 40 cartridge has:

- the START pause menu;
- saving: the options and the best tournament result;
- the three UFO 40 goals, which replicate Bushido Ball's own three:

| UFO 40 goal | Condition | Bushido Ball's goal |
|---|---|---|
| Beacon | defeat 3 opponents in one tournament | gift: defeat 3 opponents in tournament |
| Saucer | win the tournament | gold: win the tournament |
| Alien | win the tournament without a continue | cherry: win without using continues |

2P Versus and 2P Co-op need a second player: two gamepads or a split keyboard
on PC and the web. On the Vita (one controller) only the 1P tournament is
offered.

## Controls

| Input | Action |
|---|---|
| D-pad | run |
| B | strike (up/down: angle, back: lob) |
| B twice | secondary weapon (half a bar) |
| hold B | charge a Super Shot (one bar) and swing on release |
| A | roll (Zina: blink) |
| START | pause menu |

Split keyboard for two players: player 1 WASD + F (roll) / G (strike),
player 2 arrows + K (roll) / L (strike).

## Sources

- [W] UFO 50 Wiki (Miraheze), "Bushido Ball": controls, points, laws and
  penalty points, the arena lines, meter, secondary weapons, Super Shots and
  how to stop them, body hits, all six characters with stats and kits,
  options, modes, goals. https://ufo50.miraheze.org/wiki/Bushido_Ball
- [G] Steam guide "Bushido Ball" (summarised): back + strike lobs, dash + up/down
  strike spins the ball, every 2 hits give half a bar, being hit by a special
  costs half a bar, three penalties end the round and give the opponent a
  point. https://steamcommunity.com/sharedfiles/filedetails/?id=3339728484
- [L] Lizstar's Trashcan, "UFO 50 Retrospective Part 14 - Bushido Ball": half a
  meter per two hits, double tap for the special, hold for the full charge
  hit, co-op doubles, no music during play.
  https://lizstar64.github.io/reviews/2024/10/12/UFO50-14.html
- [SC] Static Canvas, "The UFO 50 Diaries: Bushido Ball": 8 points by default,
  shurikens, caltrops, bombs and grappling hooks, the well-timed wallop.
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-bushido-ball
- [P] Popcar's Blog, "Reviewing Every Single UFO 50 Game": curve balls, lobs,
  meter from volleys, no in-game music, tough CPUs late on.
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [SG] Search summary of the Steam discussions: stalling, interception and
  attacking fouls; three fouls give a penalty point; fouls shown as red books.
- [SG2] Steam thread "If you're struggling with Bushido Ball, try this": five
  1v1 matches in the tournament, co-op is 2v2 and needs two wins.
  https://steamcommunity.com/app/1147860/discussions/0/4700161643034778286/
- [SD] Steam threads "Bushido ball" and "Bushido Ball Help?": the CPU returns
  shots almost perfectly, lobs fool it, Hyper speed troubles it, the cherry
  needs no continues. https://steamcommunity.com/app/1147860/discussions/0/4849904427679634659/ ,
  https://steamcommunity.com/app/1147860/discussions/0/523083510998705141/
- [SH] Steam thread "How to easily win every match in Bushido Ball": the CPU
  lets lobs fly over it. https://steamcommunity.com/app/1147860/discussions/0/4849903793440612199/
- [D] Steam thread "Bushido ball": the round count can be lowered and the laws
  switched off.
