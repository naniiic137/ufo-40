# 20 · LIMPET

*Internal design document. Not shown in the product.*

## Tribute to

**Warptank** (UFO 50 game #20, Mossmouth, "November 1985"). The rules were
researched from text only: the community wiki's raw pages (Warptank, Cheats,
Terminal, Meta Messages, Multiplayer), the Steam guides "The missing
manuals" (section 20), "Warptank - Cherry Clear + Map" (text only, maps not
viewed) and "Gift, Gold & Cherry", Steam threads, written reviews and TV
Tropes (listed under Sources; the full notes are in the research folder,
`20-warptank.md`). No UFO 50 images, video, sprites, music, maps or text were
used as references, and nothing was taken from a UFO 50 install.

**Map type: fixed.** Warptank's sectors and hub are hand-made and the same
every game [MH], [GUIDE]. Ours are hand-made too (`lmp_levels.c`, carved by a
small script of ours), all our own layouts. Only the structure follows the
original:

| Full scale | Warptank | LIMPET |
|---|---|---|
| Players | 1 [MH], [MH-MP], [TVT] | 1 |
| Levels | 27 sectors: 26 and a secret 27th, each entered from a capsule in the station hub [GUIDE], [MH] | 27 decks: 26 and the secret Comb, each entered from a pod in Spindle Station |
| Survival levels | 3 of the 26, with no coffee [GUIDE] | 3 of the 26 (Mess Hall, Marrow, Furnace), five or six waves each, no honey |
| Hidden collectible | a coffee in 23 sectors, the 24th from the final boss [GUIDE] | a jar of honey in 23 decks, the 24th from the Dreadnought |
| Bosses | Renegade (mid-game, gift) and Big Tank (final, gold) [MH], [TVT] | the Turncoat and the Dreadnought, each in its own arena pod |
| Hub | a massive station that is "one big navigation puzzle": many rooms, hidden paths and hidden sectors (one under the floor, a dead end that isn't, one entered sideways by warping from a wall), zones with their own music (normal, an infested zone left of the start, a cafe zone, an upper area), areas that open as sectors are cleared [MANUALS], [STATIC], [STEAM-12], [STEAM-MUSIC], [STEAM-LEVEL], [GUIDE] | Spindle Station: a maze of 20 screen-sized rooms (5 x 4): the Concourse (start), the Canteen (cafe, right), the Gristle Wing (infested, left of the start), the Upper Ring (above); 29 pods dotted about, five hidden (a pocket under the Canteen's floor through a marked plate, a false wall at a dead end in the Gristle Wing, the Turncoat up a side way of plates, a slot in a wall reached only by warping from the far wall, a pocket under the Upper Ring's floor); five people; three paint bays |
| Gating | parts open as sectors are cleared; two obstacles move at 9; many stuck at 12 until a hidden side path; Renegade opens a wall in the first room; the final boss after about 22 [MANUALS], [STEAM-12], [STEAM-STUCK], [STEAM-RENEGADE], [GUIDE] | the crate at 4; the Gristle Wing's doors and the Canteen's back room at 9; the Turncoat up a side way of one-way plates from the Gristle Wing (the 18 decks before it run out at 18, so the way on has to be found); the Turncoat opens a shaft in the Concourse's ceiling to the Upper Ring; the Dreadnought's wall at 22 |
| Secret level | all 24 coffees: a bridge of coffee bubbles near the octagonal race track [GUIDE], [STEAM-LEVEL] | all 24 jars: a bridge of honey appears in the Canteen's back room, by Rolo's eight-sided loop line |
| Length | about two hours for a first clear; ~5 minutes or less a sector; speedrun records: sectors 13.6-80.3 s (median 30.5 s), the final sector 69.5 s, Gold 17.8 min, Cherry 23.3 min [POPCAR], [SR] | decks of eight screen-sized rooms (the Well nine, the Comb fifteen) with a C plate at every room and the deck's idea taught, repeated and then put under fire; the demo player (which never hesitates) clears a deck in 21-41 s (median about 30 s), the Comb in about 65 s, and the whole gold run from the title in about 19 minutes |
| Goals | 3 [MH], [GGC] | the same 3 |
| Stat | Total of All Best Times [MH] | each deck's best time, and their total on the title screen |

## Mechanics checklist

| Mechanic | How LIMPET does it | Source | Test |
|---|---|---|---|
| Clinging | the tank is always on a surface: floor, wall or ceiling; it can't fall and can't jump | [MANUALS], [MH], [TVT-MAIN] | lmp_02 |
| Rolling | the pad moves it along its surface, one tile at a time (grid steps, seven frames a tile); on a wall UP and DOWN roll it; the other two ways do nothing | [MANUALS], [KAYIN], [STEAM-CAM] | lmp_02 |
| Ramps | slopes carry it round corners: into a corner and up the wall, round the end of a ledge; a sheer wall stops it, and it can't drive off a ledge | [MANUALS], [MH], [HANS] | lmp_02, lmp_03 |
| The warp | instant, straight out from the surface to the opposite one, flipping the tank (floor and ceiling, wall and wall) | [MANUALS], [MH], [LIZ] | lmp_04 |
| A warp that can't land | nothing happens but a puff (a magnet floor, or no face to land on) | [LIZ], [MANUALS] | lmp_04 |
| Warping onto spikes | kills | [MANUALS] | lmp_04 |
| Shots | straight out from the surface (up from a floor, down from a ceiling, sideways from a wall); so the shot line is the warp line | [MANUALS], [MH], [POPCAR] | lmp_05 |
| Firing | a press fires one shot; holding fires a burst of three; pressing again and again fires faster | [MH], [MANUALS] | lmp_05 |
| Warp flares | landing throws flares two tiles each way along the surface; they hurt foes | [MANUALS], [MH], [XENETH] | lmp_06 |
| One hit | spikes, a foe, a shot, a flame or a crusher: the tank is scrapped | [MANUALS], [TVT] | lmp_04, lmp_07, lmp_11, lmp_13 |
| Lives | none to count: after every death, TRY AGAIN (from the last C plate) or BACK TO THE STATION | [LIZ], [MANUALS] | lmp_07 |
| Checkpoints | C plates, taken by rolling or landing on them, often just before a hard part | [TVT], [STATIC], [POPCAR] | lmp_07 |
| The exit | a pod choked with gristle until a red bulb nearby is shot; reaching it clears the deck | [TVT], [STEAM-STUCK], [MH] | lmp_09 |
| Hidden honey | one jar a deck, hidden in a surface that looks like the hull round it (a shade off, no rivet); touching it (rolling or landing on it) gives the jar at once: no marked block and no second place to reach; carried, it shows in the corner; it counts only if the deck is left alive (a death loses it; it can be taken again); banked, the pod turns gold | [MH], [GUIDE], [STEAM-GUIDE-COMMENTS], Tips & Tricks, "Warptank coffee location" | lmp_08 |
| Honey lost to the deck's state | e.g. Pantry: the jar's pocket is under an outline block the way on needs shot solid; take the jar first | [GUIDE] | play |
| Hint after the end | once the Dreadnought is scrapped, a bee buzzes by each deck's honey | [GUIDE], [TVT] | lmp_16 |
| No clock | a deck shows no timer while it is played; the time shows when the pod shuts, and each deck keeps its best | Tips & Tricks | lmp_09 |
| Unlock scenes | when a clear opens something in the station, the camera goes over to it, it opens, and the camera comes back: no words, no arrow | "Levels after 6 in Warptank", [POPCAR], [STEAM-RENEGADE] | lmp_15, lmp_25 |
| Camera | it snaps along in 40-pixel steps instead of scrolling | [POPCAR], [STEAM-CAM] | lmp_21 |
| Tiles | breakable blocks; outline blocks (solid once shot, a warp passes until then); teal and pink swap blocks and their switches; one-way plates; magnet floors; push blocks; doors and their switch; timed doors; portals in pairs; hopper pads; C plates; the red bulb; moving platforms; crushers (which kill only what they catch) | [TVT], [POPCAR], [GUIDE], [STATIC], [XENETH] | lmp_10, lmp_12, lmp_13 |
| Foes | turrets (fire down their line when the tank is on it, then cool), wall lasers (quick beams on a beat), drones (slow beams that bounce), oglers (rings), gristle crawlers (tank-sized, roll along surfaces), sacs that let out specks, cable worms, a switch worm whose head flips the swap blocks, spiked balls, flame vents | [TVT], [GUIDE] | lmp_11, lmp_12 |
| Survival decks | five or six waves; when the last is beaten the pod opens | [GUIDE] | lmp_14 |
| The Turncoat | a tank like Limpet: rolls, rests, fires bursts of three down its line, glows and warps with flares; 16 hits, and after each it can't be hurt for a moment and scuttles off out of the line, so it has to be lined up again (the demo player takes about 40 s) | [MH], [STEAM-RENEGADE] | lmp_15 |
| The Dreadnought | a giant tank on the ceiling, barrel hanging down; hurt only by a shot straight up the barrel while the core is open after a shell, and three hits shut the core again, so all three phases happen (24 hits); blows up in a chain and drops the 24th jar | [TVT], [GUIDE], [STEAM-SOFTLOCK] | lmp_16 |
| Hub people | Wren (her bees got out; no hints), Gus (the crane and its crate), Mother Brindle (the thing on the roof), Rolo (his loop line and the bridge), the night clerk in a hidden corner (a word from outside the game) | [STEAM-STUCK], [STEAM-LEVEL], [GUIDE], [MH-META] | lmp_18 |
| Paint | paint bays in three unlockable parts repaint the tank | [TVT], [STEAM-LEVEL] | lmp_18 |
| Music | a tune for each part of the station, four shared deck tunes, the survival decks, each boss and the Comb | [STEAM-MUSIC] | lmp_22 |
| Saving | everything saves: decks, honey, best times, bosses, paint, the last pod; the title offers CONTINUE | [MH], [STATIC] | lmp_19 |
| After the end | the station starts again at the Concourse; only now do the deck and honey counters appear (in the hub and on the title, which before shows only the best-time total), and the bees are out | [TVT], [STEAM-FLOWER-BUG] | lmp_16, lmp_25 |
| Goals | Beacon: scrap the Turncoat. Saucer: scrap the Dreadnought. Alien: bank all 24 jars and clear the Comb | [MH], [GGC], [GUIDE] | lmp_15, lmp_16, lmp_24 |
| Playable start to end | the demo player, with real button presses from the title, clears every deck through the hub's gates (gold), and on a honey run takes every jar and clears the Comb (cherry) | | lmp_23, lmp_24 |

### Readings we had to choose

- **Ramps.** A ramp is a slope tile in a corner (inside or outside); the
  tank rolls over it without stopping and comes out turned a quarter. It
  can't stop, shoot or warp on the slope itself.
- **Holding the pad round a corner** keeps the tank going the same way
  round until the button is let go; a fresh press is read afresh.
- **A warp that can't land** (a magnet floor under the tank, or nothing
  facing it) does nothing but puff; warps never fly off the map. Warping
  doesn't hit things in the way; landing on a foe is the end of the tank.
- **Warp flares** at the landing spot, two tiles each side, two damage.
- **Numbers no source gives:** a tile 10 px; seven frames to roll a tile;
  ten frames before the next move after a warp; shots 4 px a frame, six at
  most on screen, a held burst every nine frames and a fresh press every
  five; turrets aim 14 frames and cool 90; a timed door stays open 6 s.
- **Survival decks** are five waves (six in the Furnace); a wave comes 100
  frames after the last is beaten.
- **Hoppers** (named only in one review) are pads that fling the tank: a
  forced warp a moment after it arrives on one, even off a magnet floor.
- **Crushers** kill only a tank they catch (coming down on it, or carrying it
  into a wall); their faces are like any hull. Moving platforms carry a tank
  clinging to them.
- **The Turncoat** rests between steps and glows for half a second before
  it warps, so it can be read; its bursts are the tank's own.
- **The Dreadnought** crawls along its ceiling, braces while it charges a
  shell, fires it down its barrel, and holds still with its core open for a
  while after; once hurt, its shells throw sparks along the floor; its last
  third is quicker. The bible's "shooting up its cannon into its core" is
  this shot up the barrel.
- **On a death** foes and hazards are back where they started; the map
  (broken blocks, opened doors, the swap blocks) is as it was when the C
  plate was taken. The hidden jar goes back to its place, hidden again
  unless it had been found before the plate was taken.
- **Gating counts we had to set:** the crate at 4, the Gristle Wing and the
  back room at 9 (the "two obstacles" of the original), the Dreadnought at 22
  (the guide's "at least 22").
- **The hint bee** buzzes at each deck's honey surface (the original's
  flower is "near the coffee").
- **The hidden honey's look:** hull a shade off with no rivet, never a
  marked block (players found "no visual cue", or a tile "colored slightly
  different").
- **The counters** appear after the Dreadnought (the sources: "an unlockable
  reward for beating the game, along with the counter").
- **The Turncoat after a hit:** three quarters of a second it can't be hurt,
  and six quick steps along its surface away from the tank's line.
- **The Dreadnought's core** shuts after three hits in one opening.
- **The unlock scene** lasts two and a half seconds; each one plays when the
  clear that opened it ends.
- **The camera** snaps in 40-pixel steps, both ways; no smooth option.

## What is ours

- **Name:** LIMPET (1985, Beamdown Softworks).
- **Hero:** Limpet, the last hull tank of Spindle Station, in station teal
  (and three paints from the bays).
- **The world:** Spindle Station in high orbit, overrun by the Gristle from
  the Underneath; the Concourse, the Canteen, the Gristle Wing and the Upper
  Ring; 26 decks (Hatchway, Rampway, Bulkhead, Spike Row, Coldroom, Sieve,
  Pantry, Mess Hall, Boiler, Swaps, Press, Hoppers, Gullet, Ribcage, Marrow,
  Wormway, Oglery, Loopway, Lifts, Drone Bay, Furnace, Spindle, Switchback,
  Ballroom, Relay, The Well) and the Comb.
- **The foes:** the station's turrets, wall lasers and drones; the Gristle's
  oglers, crawlers, sacs and specks, cable worms and the switch worm; spiked
  balls and flame vents; the Turncoat and the Dreadnought.
- **The people:** Wren the beekeeper, Gus on the crane, Mother Brindle the
  cook, Rolo the railway man and the night clerk.
- **Honey and bees** instead of coffee and flowers.
- **Music:** "Limpet" (title), "The Crew Fled" (story), "Concourse",
  "Canteen Swing", "The Gristle Wing", "Upper Ring", "Hull Crawl",
  "Pressure", "Cold Iron", "Deep Decks", "Hold the Line" (survival),
  "Turncoat", "Dreadnought", "The Comb", "Quiet Orbit" (endings), and the
  jingles "Pod Shut", "Honey Banked" and "Scrapped".
- **Words:** the story, both endings, the people's lines and every label.
- **Layouts:** every deck is built from screen-sized rooms carved by a
  script of ours (each room with its own puzzle: the deck's idea, the
  station's foes, ramps, plates, outline bridges, doors, swaps, vents,
  crushers, hoppers, portals, lifts, push blocks, timed doors, the worm's
  channel), and the hub's maze is drawn room by room.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu (with LEAVE DECK
in a deck) and the three UFO 40 goals, which are Warptank's own (gift,
gold, cherry). The SLIM-TANK Terminal code is a UFO 50 collection feature;
UFO 40 has no terminal, so it is left out. The original's Terminal riddle
(parking on a corpse's teeth) is left out with it.

## Controls

The owner's layout puts the main action, the gun, on **A**; Warptank has the
gun on Button 1 (B) and the warp on Button 2 (A) [MANUALS], [MH]. The
cartridge card's CONTROLS page can swap them back. Going into a pod is on the
warp button, as in the original.

| Input | Action |
|---|---|
| D-pad | roll along the surface (left/right on floors and ceilings, up/down on walls) |
| A (tap) | fire straight out from the surface |
| A (hold) | a burst of three |
| B | warp to the opposite surface |
| B at a pod | go in |
| START | pause (LEAVE DECK in a deck) |

## Not confirmed

| Control or rule | Our reading |
|---|---|
| The gun's button | B in the original [MANUALS], [MH]; A here (owner's layout, swappable) |
| Entering a capsule | on the warp button, as the sources say (B here) |
| Holding the pad: one step or continuous | continuous, one tile at a time |
| What a failed warp does | nothing but a puff |
| Where the warp's flares come out | the landing spot, two tiles each side |
| How survival sectors end | the last of five (six) waves beaten |
| Renegade's and Big Tank's patterns | ours, as in "Readings we had to choose" |
| Every number in "Readings we had to choose" | as listed |

## Tests

`tests/lmp_01` … `lmp_22` and `lmp_25` drive each rule with button presses, or set up a
moment with cheats and then play it. The demo player (`lmp_bot.c`) finds its
way with a search over the tank's own moves (roll either way, warp) on the
real map, following a short plan per deck (go to a mark, fire, warp, shoot
the bulb, take the honey, leave). Near danger it runs copies of the game two
moves deep and keeps the move that lives longest; it fires only when a run
ahead shows the shot will hit, and in fights it predicts where foes will be
and waits there. `lmp_23` plays from the title through the hub's gates,
every deck and both bosses to the ending (gold); `lmp_24` plays the honey
run to all 24 jars and the Comb (cherry).

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Warptank" (raw page).
  https://ufo50.miraheze.org/wiki/Warptank
- [MH-CHEATS], [MH-META], [MH-TERM], [MH-MP] the same wiki's Cheats, Meta
  Messages, Terminal and Multiplayer pages.
- [MANUALS] Steam guide "The missing manuals - How to play UFO 50 games",
  section 20. https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [GUIDE] Steam guide "Warptank - Cherry Clear + Map" (text only), and
  [STEAM-GUIDE-COMMENTS] its comments: 26 + 1 sectors, the three survival
  sectors, 23 + 1 coffees, the death rule, gold entrances, the hint flowers,
  coffee lockouts, the final boss after 22, the secret sector's bridge.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335511903
- [GGC] Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [STEAM-CAM] "Warptank's Camera"; [STEAM-FLOWER-BUG] "Is my Warptank
  bugged?"; [STEAM-LEVEL] "I cant find one of the levels"; [STEAM-STUCK]
  "Stuck on Warptank"; [STEAM-12] "warptank levels after 12";
  [STEAM-RENEGADE] (the wall that opens after Renegade); [STEAM-SOFTLOCK];
  [STEAM-MUSIC]. Steam community discussions for UFO 50 (app 1147860).
- [TVT] TV Tropes recap "UFO 50 Game 20 Warptank"; [TVT-MAIN] the UFO 50
  page.
- [POPCAR] popcar.bearblog.dev, "Reviewing every UFO 50 game".
- [STATIC] Static Canvas, "The UFO 50 Diaries: Warptank".
- [LIZ] Lizstar, "UFO 50 Retrospective Part 20".
- [KAYIN] kayin.moe, UFO 50 notes.
- [XENETH] xeneth.design, "Fifty days of UFO 50, day 5".
- [HANS] thehans255.com, "One unique thing about every UFO 50 game".
- [PUNISHED] Punished Backlog, "Best UFO 50 games".
- [ANI] AniGamers, "UFO 50 mini reviews".
- [SR] speedrun.com, Warptank any% (about 24 minutes).
