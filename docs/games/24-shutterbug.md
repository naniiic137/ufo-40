# 24 · SHUTTERBUG

*Internal design document. Not shown in the product.*

## Tribute to

**Caramel Caramel** (UFO 50 game #24, Mossmouth, "May 1986"). The rules
were researched from text only: the community wiki's raw and rendered
pages, the Steam "missing manuals" guide (its Caramel Caramel section,
downloaded as a page), the Steam cherry guide and threads, and written
reviews (listed under Sources; the full notes are in the research folder,
`24-caramel-caramel.md`). No UFO 50 images, video, sprites, music, stage
layouts or text were used as references, and nothing was taken from a UFO
50 install.

**Map type: fixed.** Caramel Caramel's waves and stages are hand-placed,
"the same layout every run" [POPCAR], so ours are too
(`shutterbug_stages.c`, all our own terrain and waves). Only the structure
follows the original:

| Full scale | Caramel Caramel | SHUTTERBUG |
|---|---|---|
| Stages | a tutorial prologue and 5 stages: 3 planets and 2 open "Orb Shower" stages between them [MH], [MH-CHEATS] | the prologue Home Orbit; Teatime Planet, Comet Rain A, Gloom Planet, Comet Rain B, Fossil Planet |
| Planets | outdoor stretches and caves; Snack Planet has two caves, the second with "scrollable sections" [MH], [CHERRY-G] | each planet has outdoor stretches and two or more caves; Teatime's second cave has two tall rooms the view follows you up and down |
| Mid-bosses | 2: Bella (Snack), Toadead (Ghost, optional) [MH] | 2: Madame Scone (Teatime), Old Croak (Gloom, optional) |
| Stage bosses | 3: the Egg, Connie, Dinolord [MH] | 3: the Teapot, the Signalman, King Thunderjaw |
| True boss | 1, behind the letters U, F and O [MH], [CHERRY-G] | the Kaleidoscope, behind U, F and O |
| Big fights in all | 5 bosses and mid-bosses on the way, then the true boss | the same 5, then the true boss |
| Foe kinds | Siners, bread, green foes, erupters, asteroids, bursters, ghosts, trains, Railbombs, homing missiles, "!" missiles, hoppers, Squirts, Pteros, laser walls, generators, turrets, Wallbombs, Dolphins [MH], [CHERRY-G] | 25: sugar cubes, toasties, mints, geysers, big and small comets, poppers, wisps, ghost coaches, rail crawlers, launchers and their rockets, darts, bounders, nippers, kitewings, laser gates, generators, spouts, clingers, danglers, swirls, seedpuffs, bone chutes, shards |
| Red repair foes | one per planet [MH] | one per planet: a sugar cube, a rail crawler, a spout |
| Hidden letters | U (Snack), F (Ghost), O (Dino) [CHERRY-G] | U (Teatime), F (Gloom), O (Fossil), in the same kinds of places |
| Secrets | odd things in the scenery that shake out score orbs [MH], [CHERRY-G] | 5: cups with faces, two on Teatime and Gloom, one on Fossil |
| Extends | 8, at 8k, 20k, 36k, 56k, 80k, 108k, 140k, 186k [MH] | the same 8 |
| Run length | "a successful run is under 20 minutes" [POPCAR]; the cherry record about 11:47 [SEARCH-SRC] | the demo player's true run takes 14 minutes (50,911 frames) |
| Players | 1, or 2 in co-op (Melon and Pluron) [MH] | 1, or 2 in co-op (Poppy and Sprig) |
| Stats | Most Enemy Types Photoed, Most Secret Photos Taken [MH] | the same two, on the title |

The brief's "5 bosses + 2 mid-bosses" counts Bella and Toadead twice:
every source lists five big fights before the true boss (two mid-bosses
and three stage bosses) and no boss in either Orb Shower, whose page
describes only waves [MH]. SHUTTERBUG has the same five.

## Structure

| # | Stage | What's in it |
|---|---|---|
| 0 | **Prologue: Home Orbit** | a tip at each step (the gun, the rings, rock that does no harm, the camera, double points, green foes, the bulbs); seedpuffs and mints. Finished with no points at all, its clear screen shows a message of its own |
| 1 | **Teatime Planet** | the lawn (sugar cubes, toasties, mints, a spout, a secret) → the first cave (clingers, spouts, two moving blocks) → the red sugar cube just outside it → **Madame Scone** → the lawn → the second cave, with two tall rooms (the **U** at the bottom right of the second, just before the boss) → **the Teapot** in a parlour with holes in its ceiling |
| 2 | **Comet Rain A** | open space: swirls, geysers that burst from the top or bottom after a warning, big and small comets, poppers that burst if left alive, mints |
| 3 | **Gloom Planet** | the haunted line (wisps, ghost trains, a secret) → the first cave (rail crawlers, launchers and homing rockets, clingers) → **Old Croak**'s hall (a chasm, the **F** in a notch of the ceiling) → the second cave (the red rail crawler, danglers, a secret) → the sidings, where ghost trains run on three tracks → **the Signalman** |
| 4 | **Comet Rain B** | darts from the left after a "!" (the last set stops in mid-screen and turns back), bounders that bounce harder and fire, swirls, comets, poppers |
| 5 | **Fossil Planet** | the **O** among the fronds at the very start → nippers and kitewings → the first laser maze → the red spout → the valley → the second laser maze → the hanging gallery (danglers over nippers) → the generator hall (four generators; the view waits until they are gone) → **King Thunderjaw** |
| 6 | **The Lens** (only with U, F and O lit) | **the Kaleidoscope**, in open space |

A run: title → story → prologue → stages 1–5 → the ending (or, with all
three letters, the true boss and the true ending) → credits → title. A
lost ship with a spare starts the stage again; with none, the game is over.

## Mechanics checklist

| Mechanic | How SHUTTERBUG does it | Source | Test |
|---|---|---|---|
| Scrolling | forced, to the right; the ship flies freely within the screen and always faces right | [MH] | shb_07 |
| Moving | 8 ways, a steady 1.3 px a frame; no speed-ups | [MH], [LIZ], [ANI] | shb_23 |
| The gun | hold B: fully automatic, to the right | [MH], [MM] | shb_02 |
| The rings | held long enough, two dots light in front; letting go throws 4 rings fanned forward that bounce off rock | [MH], [MM] | shb_02 |
| The camera | tap A with the meter full: a photo centred on a cursor a third of the screen ahead | [MM], [MH-RENDER] | shb_03, shb_31 |
| Recharge | slowly by itself; pink bulbs from some foes (and some boss projectiles), two to a full meter | [MH], [MM], [CHERRY-G] | shb_03, shb_04, shb_21 |
| Bulbs and firing | bulbs drift slowly while B is held, and fly in from across the screen when it's let go | [MH], [MM], [CHERRY-G] | shb_04 |
| Stun | a photographed foe stops still, takes double damage and is worth double | [MH], [CHERRY-G] | shb_03 |
| The bang | a stunned foe dies with a blast that hurts the foes round it, and takes every foe of its kind in the same photo with it | [MM], [CHERRY-G], [SEARCH-TVT] | shb_05 |
| Retaliation | green foes leave a parting shot that floats then drifts after you, unless photographed; laser gates answer shots | [MH] | shb_05, shb_32 |
| Trains | one coach in a photo stops the whole train; one shot down takes them all | [MH], [CHERRY-G] | shb_13 |
| Wall and ceiling foes | spouts, clingers and danglers drop off when photographed, die when they land and kill what they land on | [CHERRY-G] | shb_06 |
| Ghosts | can't be hurt (or hurt you) while faded; a photo makes them solid | [MH] | shb_12 |
| Scenery | a photo freezes a moving block, weakens crumbly rock to one shot, shakes orbs out of an odd thing | [MH], [CHERRY-G] | shb_11, shb_26 |
| Score orbs | 20, 40, 60 ... up to 200 each, rising with every orb shot in the same life | [CHERRY-G] | shb_11 |
| Rock | touching it does no harm; only being pinned by the scroll or a moving block hurts | [MH], [RESET], [STEAM-0LIVES] | shb_07, shb_26 |
| Two hits | the first takes the armour (Poppy turns blue underneath), the second the ship | [MH], [MM], [LIZ] | shb_08 |
| Repair | one red foe per planet becomes a wrench when photographed: armour back, or points | [MH], [CHERRY-G] | shb_10, shb_22 |
| Lives | none to start with; 8 from points at the original's thresholds | [MH], [STEAM-LIVES] | shb_09 |
| A lost ship | back to the start of the stage, the shown score back to 0, the letters kept | [MH], [POPCAR], [CHERRY-G] | shb_08 |
| No continues | out of ships, the game is over | [MH] | shb_08 |
| Final score | the highest the shown score ever reached | [MH] | shb_08, shb_30 |
| "To next" | the lost-ship screen shows the points to the next ship; the bonus bar in the HUD fills toward it | [MM], [STEAM-LIVES] | shb_09 |
| Letters | U, F and O, one per planet, sparkle now and then, light up when photographed and stay lit after a lost ship | [CHERRY-G] | shb_14, shb_27 |
| Madame Scone | spreading volleys, crumbs falling from the sky (every other one a bulb when shot); a photo stuns her | [MH], [STEAM-CC] | shb_21 |
| The Teapot | bounces about; drips from the holes in the ceiling (bulbs when shot); toasties pop bouncing crumbs from the far end; hurt only through its lid, which only a photo lifts; safe under the ceiling between the holes | [MH], [STEAM-CC] | shb_16 |
| Old Croak | hops, spits shots that ricochet; worth 5,000; left alone he slinks into the chasm and the way goes on | [MH] | shb_20 |
| The Signalman | a ghost: hurt only while photographed; green sparks to his tracks set pink sparks crawling along them; wisps and trains join in | [MH] | shb_17 |
| King Thunderjaw | comes once the generators are gone; hurt only in the jaw, which opens to fire and which a photo forces open; bone chutes top and bottom; darts that stab up | [MH] | shb_18 |
| The Kaleidoscope | shards circle the core and soak up shots; a photo of the core unwinds them to wander the screen while the core can be hurt; red volleys; toasties from the top and bottom | [MH], [CHERRY-G] | shb_19 |
| Comet Rain | geysers after a rising burst; poppers that burst if left alive; darts after a "!", the last set stopping mid-screen and turning back; bounders | [MH] | shb_24 |
| Fossil Planet | nippers leap; kitewings' darts stab upward when they land; laser gates switch on and off | [MH] | shb_32, shb_33 |
| Co-op | Sprig on the second pad, with her own gun, rings, camera and armour | [MH], [MH-MP] | shb_23, shb_29 |
| Goals | Beacon: beat the Teapot; Saucer: beat King Thunderjaw; Alien: photograph U, F and O in one run and beat the true boss | [MH], [GGC], [CHERRY-G] | shb_16, shb_15, shb_19, shb_27 |
| After the ending | without the cherry, the ending says the album has an empty page: defeat the true boss | [MH], [CHERRY-G] | shb_15 |
| Saving | no run is saved; the best score and the two stats are | [MH], [CONV] | shb_30 |
| Completion | the demo player wins from the title with real presses, by the true route and without the letters | | shb_27, shb_28 |

## Controls

| Input | Action | Confirmed? |
|---|---|---|
| D-pad | fly, 8 ways | yes [MH], [MM] |
| Hold B | the gun, and charge the rings | yes [MH-RENDER], [MM] |
| Let go of B (charged) | four rings | yes [MH], [MM] |
| Tap A | a photo (meter full) | yes [MH-RENDER], [MM] |
| START | pause | UFO 40's pause menu |
| **A while B is held** | **takes the photo (firing doesn't stop it)** | **not confirmed** |
| **Player 2** | **the same buttons on the second pad** | **not confirmed (only "identical controls")** |

## Readings we had to choose

- **The score and the extends.** The wiki says the shown score goes back
  to zero after a lost ship and the final score is the highest it ever
  showed; the manual says the bonus bar and the score are kept; a player
  saw "to next" as low as 5,000 after dying on stage one. We read it as:
  the shown score goes back to 0, and every point earned this run counts
  toward the next ship (the bonus bar and "to next" keep their place).
- **The armour** comes back with each stage and each new ship ("you can
  get hit twice per level"). Liz's "you respawn as your friend" is read as
  the armour breaking: the ship changes colour (red to blue, as the manual
  says).
- **Co-op:** lives and score are shared; each ship has its own armour,
  gun, rings and camera; a ship lost by either player costs a shared life
  and sends both back to the stage's start (with none to spare, it is
  over).
- **Numbers no source gives:** the ship flies 1.3 px a frame; the rings
  charge in 54 frames, fan out at 13 and 40 degrees, do 5 (the gun 1),
  bounce 3 times and last 96 frames; the photo is 64 × 52 with its centre
  107 pixels ahead, stuns for 5 s (bosses 3 s) and the meter refills in
  10 s; the blast reaches 28 pixels and does 6; bulbs drift at 0.35 px a
  frame while firing and fly at 2.8 when not.
- **Points:** 50 to 500 for foes, 1,000 for a generator, 3,000 to 20,000
  for the big ones (Old Croak 5,000, as the sources say), 1,000 for a
  wrench with the armour on, 10 for a shot-down crumb or drip.
- **Stunned foes still hurt to touch**; only the photo's effects listed
  above are claimed.
- **"Scrollable sections"** are read as tall caves where the view follows
  the ship up and down; the U waits at the bottom right of the second.
- **Old Croak** gives up after 25 seconds.
- **The Teapot's weak spot** is under its lid: a photo lifts it for the
  stun's 3 seconds.
- **The Beacon** comes when the Teapot falls and **the Saucer** when King
  Thunderjaw does, true route or not.
- **Laser gates** don't mind photos; the generator hall has four.
- **The prologue's message** for no points is our own wording.

## What is ours

- **Name:** SHUTTERBUG (1986, Beamdown Softworks).
- **Heroes:** Poppy, a red puffer-blimp with a camera for a nose, and her
  pal Sprig (green).
- **The stages:** Home Orbit, Teatime Planet, Comet Rain A and B, Gloom
  Planet, Fossil Planet and the Lens; every terrain profile, cave, tall
  room and wave.
- **The foes:** seedpuffs, mints, spouts, clingers, danglers, sugar cubes,
  toasties, swirls, geysers, comets, poppers, darts, bounders, wisps,
  ghost coaches, rail crawlers, launchers, nippers, kitewings, laser gates
  and generators; Madame Scone, the Teapot, Old Croak, the Signalman, King
  Thunderjaw and his bone chutes, the Kaleidoscope.
- **Music:** "Say Cheese" (title), "New Camera" (story), "Home Orbit",
  "Teatime Planet", "Comet Rain", "Gloom Planet", "Fossil Planet", "Flash
  Point" (bosses), "The Kaleidoscope", "Postcards Home" (ending), "The
  Album" (credits) and three jingles.
- **Words:** the story, the tips, the endings, the prologue's message, the
  goal lines and every label.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
UFO 40 goals, which are Caramel Caramel's own (gift, gold, cherry). The
terminal code that opens a practice stage select is a UFO 50 collection
feature; UFO 40 has no terminal, so it is left out.

## Not confirmed

- Every number above under "Numbers no source gives".
- The score/extend reading, and whether the armour returns each stage.
- How co-op shares lives and score.
- What the true boss's arena and the endings look like.

## Tests

`tests/shb_01` … `shb_33` drive the rules with button presses, or set up a
moment with cheats and then play it. The demo player (`shb_bot_buttons` in
`shutterbug_bot.c`) only chooses buttons: it plans a lane through the rock
ahead, tries every way the pad can point against every shot and foe a few
dozen frames ahead, holds B, lets go to throw ready rings or call bulbs in,
and taps A when the frame would catch a letter, a red foe, a boss's weak
point, a crowd or a foe on the rock. From the title it wins the whole game
by the true route (`shb_27`), without the letters (`shb_28`), and flies
two ships through Teatime Planet (`shb_29`).

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Caramel Caramel" (raw and rendered): the
  modes, extra lives and thresholds, the displayed score and the final
  score, the camera and its effects, recharging and crystals, the repair
  enemies, each stage and boss, the hidden letters.
  https://ufo50.miraheze.org/wiki/Caramel_Caramel
- [MH-RENDER] the same page rendered: Button 1 fires, Button 2 photographs.
- [MH-MP] Miraheze, "Multiplayer". https://ufo50.miraheze.org/wiki/Multiplayer
- [MH-CHEATS] Miraheze, "Cheats" (TEST-LENS). https://ufo50.miraheze.org/wiki/Cheats
- [MM] Steam guide "The missing manuals - How to play UFO 50 games",
  section 24: hold B to fire and charge (two white circles), four rings
  that bounce, tap A for a photo a third of the screen ahead with a full
  flash, the bonus bar, frozen foes explode and damage others, crystals
  half the bar, red then blue, back to the start of the level, no extra
  lives to start. https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [CHERRY-G] Steam guide "Caramel Caramel: Cherry/True Ending Guide" and
  its comments: same-type photo kills, red enemy → wrench, wall foes fall,
  bouncing charge shot, crystals faster when not shooting, background orbs
  up to 200, the three letter places, can't die to terrain.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3338885925
- [GGC] Steam guide "Gift, Gold & Cherry".
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [STEAM-LIVES] Steam thread "What is the deal with the number of lives":
  "To next", the score reset, hit twice per level, 5,000 after dying in
  stage one. https://steamcommunity.com/app/1147860/discussions/0/4852154959747348571/
- [STEAM-0LIVES] Steam thread "Is Caramel Caramel supposed to start you
  with 0 lives?". https://steamcommunity.com/app/1147860/discussions/0/592887665229074173/
- [STEAM-CC] Steam thread on the true boss and boss tactics.
  https://steamcommunity.com/app/1147860/discussions/0/4700161870965854315/
- [POPCAR] Popcar's Blog, "Reviewing Every Single UFO 50 Game".
  https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [LIZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 24": slow ship,
  "you respawn as your friend". https://lizstar64.github.io/reviews/2024/10/15/UFO50-24.html
- [ANI] AniGamers mini-reviews ("speedy movement").
  https://anigamers.com/posts/ufo-50-mini-reviews-every-game/
- [RESET] ResetEra "LTTP: UFO 50", page 3: touching walls is fine.
  https://www.resetera.com/threads/lttp-ufo-50.1394143/page-3
- [SEARCH-TVT] search summaries of the TV Tropes recap: photographed
  enemies explode, wall enemies fall.
- [SEARCH-SRC] search summary of a speedrun.com run, 11:47.
- [CONV] the research folder's `00-ufo50-conventions.md`.
