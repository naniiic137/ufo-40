# 21 · TUSKWIND

*Internal design document. Not shown in the product.*

## Tribute to

**Waldorf's Journey** (UFO 50 game #21, Mossmouth, "February 1986"). The
rules were researched from text only: the community wiki's raw page, Steam
guides and threads, written reviews and TV Tropes (listed under Sources; the
full notes are in the research folder, `21-waldorfs-journey.md`). No UFO 50
images, video, sprites, music, maps or text were used as references, and
nothing was taken from a UFO 50 install.

**Map type: random.** Waldorf's Journey makes "a new map layout" for every
run [W], so TUSKWIND does too (`tuskwind_map.c`). Its generator follows the
rules the sources give (a chain of floating platforms from the start to one
goal platform with a door at the far right, fixed quotas of everything on it,
natural winds at about 40 % and 80 %) and fills in the rest with rules of our
own, listed under *Readings*. No layout of the original is known to us or
copied; the hall at the end is a fixed room of our own that holds the
original's three end-room secrets.

### Full scale

| | Waldorf's Journey | TUSKWIND |
|---|---|---|
| Modes | 1P journey, 2P versus Battle [W], [MP] | the same: JOURNEY and BRAWL |
| The map | random every run, a chain of platforms to a door at the far right [W] | random every run: 8,000 px of sky, a chain of 70-80 islets from the first to the door, about 50 more above and below it |
| Lives | 6 puffins a run, one at each lighthouse [W] | 6 terns, one at each of 6 lighthouses |
| Loose shells | exactly 36 a run, scallops (1) and conches (4) [W] | exactly 36: cockles (1) and three or four whelks (4) |
| Chests and keys | always 2 chests and 2 keys; extra keys from squids; a secret third chest in the final room [W] | the same; keys from mantas; the third chest on the hall's hidden shelf |
| End-room shells | 3 a puffin not used (walk left); a shaft of 16 [W] | 3 a tern not used (walk west); a shaft of 16 cockles |
| Most shells | 110 [W] | 109 or 112 (three or four whelks) |
| Wind | always at about 40 % (dies out) and 80 % (stays); weather vanes toggle it [W]; a vane works once [CG] | the same; five wind bells, each rings once |
| Shops | 2 random items each, one visit, then she disappears [W] | 3 stalls of the Duchess Auk, the same rules |
| Items | beach ball 1, binoculars 2, harpoon 3, balloon 5, propeller 6, canned fish 8; one beach ball to start [W] | bobber 1, spyglass 2, grapnel 3, kite 5, spinner 6, sprat tin 8; one bobber to start |
| Signs | 22, plus a hidden one reached by riding a squid to the top left [W], [META], [TVT] | 22 of our own (the first at the start), and a hidden one only a manta ride reaches |
| Endings | 2: under 50 shells / 50 or more [W], [TVT] | the same two |
| Goals | 3 [W], [GG] | the same 3 |
| Stats | Signs Read, Most Puffins Released, Most Chests Opened [W] | the same three, on the title |
| Music | 4 tracks: the dream, the end room, the ending, the battle [OST] | 4 tunes of our own for the same places, and two short jingles |

## Mechanics checklist

| Mechanic | How TUSKWIND does it | Source | Test |
|---|---|---|---|
| The jump | the "+" cursor is swung in an arc with UP/DOWN; hold A to fill the meter under him, let go to jump toward the cursor; after that it is ballistic | [MM], [W], [TVT], [HANS] | tkw_02 |
| Walking | LEFT/RIGHT scoot slowly along the islet | [MM], [W], [TOP5] | tkw_02 |
| Flat jump | aimed parallel to the ground: forward with no airtime | [W] | tkw_03 |
| Flippers | in the air, hold A to fly: it always lifts, and LEFT/RIGHT trim the speed across; it drains the yellow bar; with it empty he just follows the jump | [MM], [W], [CG] | tkw_04 |
| Calling a jump off | B while charging cancels it; A must be pressed afresh | [MM] | tkw_02 |
| Fish | about a quarter of the bar each, at once, in the air too; they drift to him | [W] | tkw_04, tkw_06 |
| Stop on a dime | landing stops him dead, except when a tailwind carries him on | [STATIC], [W] | tkw_05, tkw_08 |
| Platforms | normal ones are one-way; blocks are solid and bump him away from underneath or the side, keeping his speed; clouds go a short time after he lands, just long enough for a full charge, and don't come back | [W], [TVT], [CG] #20 | tkw_05 |
| Pickups | shells, keys and fish drift in when he is close | [W], [TVT] | tkw_06 |
| Chests | need a key, give 10 shells; keys carry into the end room; most chests counts one run | [W], [KEY] | tkw_06, tkw_16 |
| Puffins | landing near a lighthouse frees its puffin (ours: anywhere on its islet); each saves one fall and puts him back on the platform he last jumped from, and the ride back picks up whatever it passes; none left: he wakes, game over | [MM], [TVT], [W], [CG] #13, #15 | tkw_07 |
| Wind | left or right; natural at about 40 % (dies out) and 80 % (stays); vanes turn it on or off, once each | [W], [CG] | tkw_08 |
| Pigs | alerted when he lands on their platform; after a short delay they charge (whether he is still there or not) and knock him away; jumped over, they run into the sea; flown into before alerted, they die | [W], [TVT] | tkw_09 |
| Squids | fly near the top; touching one knocks him away; it can be ridden; a fast beach ball, the propeller or the harpoon destroys it and leaves a key; only a ride all the way to the top left ("Ride a squid all the way to the left") shows a hidden sign | [W], [META], [TVT] | tkw_10 |
| Item menu | hold B (not while charging); the d-pad picks; letting go uses it, or nothing on the "X" | [MM] | tkw_11 |
| The items | beach ball (a test jump; the next meter shows a red line at its power), binoculars (look far, no time limit, gone when put away), harpoon (straight line, pulled onto the first platform), balloon (low gravity, cheaper flapping, until the next landing), propeller (free, slow flight with no gravity and its own bar; ours ends when he lands), canned fish (full bar) | [W] | tkw_10, tkw_11, tkw_12 |
| Shops | 2 random items, one visit, then she's gone; spending lowers the score; a press must buy only once | [W], [SHOP] | tkw_13 |
| Signs | 22, read by passing them, the first one beside him at the start; counted as a stat | [W], [LIZ] | tkw_14 |
| The map | new every run, from the start to a door at the far right, with the fixed quotas | [W] | tkw_15 |
| Limited view | the camera shows little ahead; aiming up or down shows more that way; binoculars for the rest | [W], [STATIC] | tkw_12 |
| The sun sets | the sky darkens and the weather worsens along the way | [PC] | (drawn) |
| Progress | the % moves each time he lands; 100 % is the door | [W], [CG] | tkw_17 |
| The end room | walk left to cash in puffins (3 each); the shaft of 16 shells for a full jump and about half the bar; the secret third chest far left over the chasm, hidden from the binoculars, for a full jump and about a quarter of the bar; falling there is game over, since the puffins have left | [W], [TVT] | tkw_16 |
| Endings | at the old walrus: under 50 shells a short warning and "run for the sea"; 50+ the ancestor's name and a way to stop the hunters | [W], [TVT] | tkw_18 |
| Goals | gift: 50 % of the journey; gold: reach the end; cherry: win with 50 or more shells | [W], [GG] | tkw_17, tkw_18 |
| Battle | 1-on-1 on a single wrapping screen, knock the other into the sea, platforms disappear over time, choose the number of rounds | [W], [TVT], [NIAHAK], [PIXELDIE] | tkw_19 |
| No mid-run save | a journey starts afresh every time; records kept | [W] | tkw_01, tkw_22 |
| Completable | a journey can be won, both endings | — | tkw_20, tkw_21 |

### Readings we had to choose

- **The flap** follows [MM]: in the air, holding A again flies. It always
  lifts (never climbing faster than 1.8 px a frame); LEFT/RIGHT held with
  it add a push across of 0.12 a frame, up to 3 px a frame; UP and DOWN do
  nothing to it. A full bar is four seconds of flying; every journey starts
  with it full; landing gives none back. B while charging calls the jump
  off [MM].
- **The cursor.** UP/DOWN swing it through an arc from 45 degrees below
  flat to straight up, on the side Burl faces; LEFT/RIGHT walk, and a press
  the other way turns him on the spot first. The camera leans about 40 px
  the way he aims. A jump aimed below flat drops him through a one-way
  islet (off a rock it is a lunge). The same turning rule holds in the
  brawl.
- **The meter** fills in one second and then holds (not an oscillating
  meter). Launch speed runs from 1.2 to 4.6 px a frame; gravity is 0.11
  (a full jump straight up rises about 96 px, about half a screen). A flat
  jump slides along the islet, slowing at 0.07 a frame.
- **Foam** (our clouds) lasts 72 frames from the first landing, a full
  charge being 60, and is gone for good; only a tern setting him down on
  the foam he jumped from brings that one back. Rocks bounce him at 80 %
  of his speed.
- **Rams** (our pigs) notice him the moment he lands and take his
  bearing; 40 frames later they charge that way at 2 px a frame whether he
  is still there or not, so a hop over or away sends them into the sea. A
  hit knocks him 3 px a frame sideways and up. Flying into an unwarned ram
  at 1.2 px a frame or more sends it off.
- **Mantas** (our squids): landing on one from above is a ride; touching
  it any other way knocks him 2.5 px a frame away from it. A bobber thrown
  at three quarters of a full charge or more bursts one. Keys from mantas
  float where they fall. The hidden sign counts only while he rides one.
- **Interacting.** The sources only say he must be grounded; the button
  isn't documented. Walking into the thing does it: a stall, a chest, the
  door, the old one. A lighthouse's tern flies to him as soon as he is on
  its islet ("when you land near them" [MM]). Signs are read by passing them; bells
  ring on any touch, in the air too.
- **Items in the air.** The menu opens in the air as well; the kite, the
  spinner and the tin work there, the aimed items (bobber, spyglass,
  grapnel) only on firm ground. The grapnel's line reaches 240 px. The kite
  is gravity 0.035 and half-price flapping; the spinner flies at 1.4 px a
  frame on a bar of six seconds and is spent when he lands. Each item stacks up to 9.
- **The map's own rules.** 8,000 px long; the route is built hop by hop,
  each new islet placed 12-100 px past the last and up to 48 px higher or
  56 lower, and kept only if a plain jump (calm air, no flapping, below the
  mantas' band) can reach it; failing that a short safe hop. About 13 % of
  the islets are rocks and 12 % foam (none in the first few per cent); 62 %
  of the stretches get a second islet above or below, off the route. The
  six lighthouses sit about 14 % apart from 10 %; the three stalls (how
  many the original has is not documented) at about 22, 48 and 72 %; the
  first sign beside him at the start ([LIZ]: "there's a sign near you")
  and the other 21 evenly from 4 % to 95 %; five bells, two chests on side islets,
  two keys floating high, ten rams (six on side islets, four on the route),
  forty sprats along the hops, eight mantas, and the hidden sign in the top
  left corner. Natural winds blow either way at random; the first dies out
  after 40 seconds.
- **The 110.** With 36 loose pieces worth 1 or 4 and 48 shells in the
  chests and the hall, 110 can't be made exactly; three or four of the 36
  are whelks, so the most is 109 or 112.
- **The hall** is ours: the floor east of a chasm, the door, the perches
  at the floor's west end, the shaft east of the door, the shelf by the far
  west wall (out of the spyglass's view), the old one at the east end.
- **The brawl:** the same jumps and flaps, no items, two thirds of a bar
  each, a sprat now and then; islets start to crumble after 10 seconds, one
  every 2.5 s; a walrus coming in at 1.2 px a frame or more knocks the other
  flying (slower, they just shove); best of 1, 3, 5, 7 or 9; both in the
  sea at once and the round is played again. On the Vita (one pad) it is
  greyed out.

## What is ours

- **Name:** TUSKWIND (1986, Beamdown Softworks).
- **Characters:** Burl, a brown walrus in a blue nightcap; Skerry, the old
  one; the Duchess Auk and her stalls; the terns, rams and mantas.
- **Things:** cockles, whelks and spiral shells, sea chests, sprats, the
  bobber, spyglass, grapnel, kite, spinner and sprat tin, wind bells, foam
  and rock islets, the hall under the sea.
- **Words:** all written fresh for Burl's dream, not following the
  original's lines or their order: 22 signs carved by someone far ahead
  (a few practical tips for a walrus learning to fly, a ship that keeps
  coming nearer, a voice that wears thin), the hidden sign, the story, both
  talks (only the ending's beats are kept: a warning, the hunters' ship,
  three signals, the sea answering), the ending lines, the credits, the
  goal lines and every label.
- **Music:** "Driftsleep" (the dream), "The Deep Hall", "Waking Tide" (the
  endings), "Floe Brawl", and two jingles, "Eyes Open" and "Round to You".
- **Art:** every sprite, the sky that sets as he goes, the rain, the
  hunters' ship on the horizon, both ending scenes.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu and the three
UFO 40 goals, which are Waldorf's Journey's own (gift, gold, cherry). The
terminal code that starts a run with 30 shells, the terminal riddle and the
meta message are UFO 50 collection features; UFO 40 has no terminal, so
they are left out.

## Controls

| Input | Action |
|---|---|
| LEFT / RIGHT | scoot along the islet (a press the other way turns first) |
| UP / DOWN | swing the aim; the view leans that way |
| hold A | charge; let go to jump (aimed flat: a lunge) |
| B while charging | call the jump off |
| in the air, hold A | fly; LEFT/RIGHT adjust the speed |
| hold B | the item row; LEFT/RIGHT pick, let go to use (on the cross: nothing) |
| walk into it | stall, chest, door, the old one (a lighthouse: just be on its islet) |
| stall | LEFT/RIGHT/UP/DOWN pick, A buys one, B or LEAVE goes |
| spyglass | the d-pad looks around; A or B puts it away |
| START | pause |
| Brawl | the same moves for each player, no items |

### Not confirmed (flagged)

| Control | Our reading | Why it is unconfirmed |
|---|---|---|
| Aim arc | UP/DOWN swing from 45 degrees below flat to straight up on the facing side; LEFT/RIGHT walk | the guide says "move the cursor with the d-pad" and "walk"; how the two share the pad is not described [MM] |
| Interact | walk into the thing while grounded (lighthouses: land on the islet) | the wiki says "interacts", without a button [W]; [MM] says birds fly off "when you land near them" |
| Brawl | the journey's controls for each player (B cancels a charge), no items | not documented [W] |

## Tests

`tests/tkw_01` … `tkw_22` drive the rules with button presses, or set up a
moment with cheats (an empty sky, an islet, a ram, a manta) and then play
it with presses. The demo player (`tuskwind_bot.c`) uses only the buttons:
on each islet it walks to whatever it should touch, then tries aims and
charges through the game's own physics, walks to its spot, swings the aim,
holds A for exactly that charge and lets go, flapping only when no plain
jump will do and waiting out a dying wind. `tkw_20` plays three generated
maps from the title to the first ending and back; `tkw_21` plays three more
going after shells, chests and the shaft, and reaches the second ending on
each. After the review fixes it finished 20 of 20 maps plainly and 12 of
12 going after shells.

## Sources

- [W] UFO 50 Wiki (Miraheze), "Waldorf's Journey" (raw page): controls in
  outline, shells, fish, puffins, wind, obstacles, the shop and items, the
  strategy section's end-room secrets, records, the 22 signs, the endings
  and 2P Battle. https://ufo50.miraheze.org/wiki/Waldorf%27s_Journey
- [MP] Miraheze, "Multiplayer". https://ufo50.miraheze.org/wiki/Multiplayer
- [META] Miraheze, "Meta Messages" (the squid ride's hidden sign).
  https://ufo50.miraheze.org/wiki/Meta_Messages
- [MM] Steam guide "The missing manuals - How to play UFO 50 games",
  section 21 (read in full by the fidelity review): walk and move the
  cursor with the d-pad, hold A to charge with a meter under him, B during
  a charge cancels it; in the air hold A to fly, the d-pad adjusting the
  speed left and right; hold B for items, the cursor over one and B let go
  uses it, the "X" cancels; birds on lighthouses fly away "when you land
  near them"; penguins next to boxes; the HUD's energy bar, shells, %,
  birds and keys. https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [CG] Steam guide "Waldorf's Journey How To Cherry": holding flight after
  the launch, vanes working once, the cursor and meter, blocks bouncing from
  underneath or the side (#20), pickups on the puffin's ride back (#13,
  #15).
  https://steamcommunity.com/sharedfiles/filedetails/?id=3341968791
- [GG] Steam guide "Gift, Gold & Cherry": 50 % completion, beat the game,
  win with 50+ shells. https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [KEY] Steam thread "Waldorf's Journey - third key?": keys carry into the
  end room; chests counted per run.
  https://steamcommunity.com/app/1147860/discussions/0/4626979145080974467/
- [LIZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 21": a sign near
  you at the start. https://lizstar64.github.io/reviews/2024/10/14/UFO50-21.html
- [SHOP] Steam thread on one press buying several items.
  https://steamcommunity.com/app/1147860/discussions/0/4849903998512136532/
- [TOP5] Steam thread: learning to walk to reposition.
  https://steamcommunity.com/app/1147860/discussions/0/4852155556170957174/?ctp=5
- [TVT] TV Tropes recap: rigid jumps decided beforehand, the flutter,
  puffins and the beach ball, the endings' scenes, the squid ride, pink
  clouds, one-way platforms, the wrapping 2P mode.
  https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game21WaldorfsJourney
- [STATIC] The UFO 50 Diaries: limited sight, stopping on a dime, loose air
  movement. https://staticcanvas.substack.com/p/the-ufo-50-diaries-waldorfs-journey
- [PC] Popcar's Blog: the sun setting and the weather worsening, one-use
  items. https://popcar.bearblog.dev/reviewing-every-ufo50-game/
- [HANS] thehans255: the artillery-style jump.
  https://www.thehans255.com/blog/2024/10/one-unique-thing-about-every-ufo-50-game/
- [NIAHAK], [PIXELDIE] reviews describing the battle mode.
  https://www.niahak.org/2025/01/ufo-50-thoughts-round-1/ ,
  https://pixeldie.com/2024/11/13/ranking-every-ufo-50-game-after-100-hours/
- [OST] The soundtrack's track list (four tracks).
  https://phlogiston.bandcamp.com/album/ufo-50
