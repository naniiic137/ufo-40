# 45 · DOT & DASH

*Internal design document. Not shown in the product.*

## Tribute to

**Mini & Max** (UFO 50 game #45, Mossmouth). The rules were researched from
text only: the community wiki, the TV Tropes recap, four Steam guides ("The
missing manuals", the "Ultimate Guide", Lasagevev0's powerup guide and the
"All Quests, Items and Upgrade locations" guide), Steam forum threads and
written reviews (listed under Sources). No UFO 50 images, video, sprites,
music, maps or code were used as references, and nothing was read from an
installed copy of the game. No room layout, town or cave was reconstructed.
An independent review of the first build against these sources was applied
in full (see the end of this document).

**Map type: one hand-made room, with generated worlds inside it.** Mini &
Max's "world map" is a single storage room seen at several sizes. The room
is hand-made; the world inside its pixels is generated, but fixed and the
same for every player, with hand-placed towns and landmarks [MH], [SM].
Ours is built the same way:

| Structure | Mini & Max (text sources) | DOT & DASH |
|---|---|---|
| Story | locked in the storage room while big sister throws a party; the dog suggests getting small [MH], [LZ] | Dot and her talking dog Dash, locked in the lumber room by big brother Otto during his party |
| Sizes | full, small, micro; Potion II "lets you shrink further to fit through one-block gaps while in the microscopic world" [PU], [GAP], [AQ] | FULL, SMALL, MICRO, and TINY: the second tonic makes Dot half her micro height in the same place, for one-tile gaps |
| Full size | the room as a level select: where you shrink decides where you arrive [PD], [MM] | the room at 2 px a tile; every spot has its own world |
| Small | "a big wide room while small, all one big level" [LZ] | the same room at 8 px a tile, 160 × 90 tiles: rug, two pots, the hanging shelf with the clock, the portrait and a reading lamp, the money box, the bookshelf, the great lamp, the door and its knob |
| Micro | "a nearly endless procedurally-generated microscopic world", the same for everyone [SM], [LZ] | every exposed tile becomes a 40 × 32 chunk, generated from its material and position; strips of up to 48 chunks; some chunks have nooks behind one-tile gaps |
| Towns | Beldor, Protis, Yarn, Doorknob City, Lamptopia, Mol Dok town, Termite City, the Dust Bunny Warren [MH], [GU] | Loamton, Fernby, Tuftville, Latchtown (on the door knob), Glimmer (on the reading lamp's shade), Tickburg, Wormwood, Fluff Hollow, and 13 smaller hand-made places |
| The door knob | Doorknob City on the knob; pay the Plasmage 50 to lower the bridge into the knob, where the throne room is [GU], [AQ] | Latchtown on the knob; the tin mage lowers the bridge for 50; the throne room is inside the knob at small size |
| Getting up there | a block grown to full size and set under your feet, then shrink on it; or a flying bug [AQ] | the same: a block carried up to full size and set down, a hop onto the knob; or a commanded moth |
| Between the walls | four sections: the outlets and Termite City, beside the floor, bookshelf to clock, and a big drop with a hijacked mouse [GU] | BETWEEN THE WALLS (the outlets, Wormwood, the Great Earwig) and THE LONG DROP (a shaft from the hanging shelf to the floor, Barley the shrew at the bottom) |
| The war book | "reach the far right side of the warzone past all the tanks and planes" and pick up the nuke [GU], [AQ] | SIEGE!, a game box on the middle shelf: tin tanks and paper planes all the way to the Big Bang |
| Critical path | pay King Mittens 500, he kidnaps the dog, beat Gearhead from inside with Potion II and the engineer's code [GU], [AQ] | pay Queen Tabitha 500, she keeps Dash, Sir Sprocket attacks; inside him, tiny, to his console, with Spinner's code |
| True path | only after escaping: Pongo behind the throne, then 1,000 to the Clockkeeper [TV], [GU], [AQ] | only after escaping: the queen's wall is gone, Nib waits in the keyhole in the corner; Tock takes 1,000 |

## Mechanics checklist

Every line was compared with the code; the tests named prove it with button
presses.

| Mechanic | How DOT & DASH does it | Source | Test |
|---|---|---|---|
| Walk, jump | ← → walk; A jumps, held for height; steering in the air | [MM], [MH] | dd_02 |
| Shrink, grow | hold ↓ (90 frames) to shrink into what is under you, hold ↑ to grow back one size | [MM], [ST], [GU] | dd_03 |
| Tiny | with the second tonic, ↓ again inside the micro world: half height, through one-tile gaps; no growing back under a low ceiling | [PU], [GAP], [AQ] | dd_03, dd_43 |
| Place follows size | where you stand when you shrink is where you arrive, and growing puts you back | [MM], [PD] | dd_03 |
| The clock | growing to full size, or falling to 0, moves the clock on a minute; no time limit | [GU] | dd_02, dd_03 |
| Carried things change size | what Dot carries grows and shrinks with her: at full size a block is big | [MH], [AQ] | dd_03, dd_31 |
| Full-size steps | at full size ↓ + B sets the carried block under her feet; the knob is out of a plain jump's reach | [AQ] | dd_31 |
| Carrying makes her taller | a thing held overhead needs the room above her head | [AQ] | dd_21, dd_29 |
| Doors | ↑ at a doorway | [MM] | dd_20 |
| Lift from below | B lifts the thing Dot stands on; B again throws | [MM], [MH] | dd_04 |
| High throw, place | ↑ + B throws in a high arc; ↓ + B sets it under her feet | [MM] | dd_04, dd_05 |
| Blocks | plain, cracker, dart, heart box, gift box, boomer, roller, glue, hourglass (stops time; can't be lifted till it runs; cracks after three), quake (cracks after three), venom (spreads), mold fruit, spore, pupa, hard, cinder, the throwing axe (comes back) | [MH], [GU] | dd_05, dd_34 |
| The Big Bang | clears the screen of creatures and weak blocks, knocks Dot for two hearts; another waits at the end of SIEGE! | [GU], [AQ] | dd_34 |
| Critters | the Grip Mitt lifts critters; the second, faster | [MH] | dd_04 |
| Dash | ambles about on his own, very slowly; ↑ on his back stops him or sets him off; stand beside him and he sits: ↑ and he gives a tip | [PU], [GU] | dd_08 |
| Dash as a weapon | thrown, he bites; hurt, he runs off and comes back only at full size or to the whistle; with his plate he can't be hurt | [PU], [TV], [GU] | dd_08 |
| Dash's nose | with his fleas gone, set down at small size he stops over any spot worth shrinking into and points | [MH], [TV] | dd_08 |
| Kite Wings | Dash walks off ledges and on through the air at the same height; thrown, he flies out and home like a boomerang, biting everything | [PU], [AQ] | dd_08, dd_30 |
| Found towns | a pennant on their tile at small size | [TV] | dd_35 |
| Shrink on creatures | Dash (fleas), Puffin the dust bunny (mites), Barley the shrew (the tin mage), the ferry fly (its cockpit), Sir Sprocket's shoulders | [MH], [GU], [AQ] | dd_08, dd_21, dd_22, dd_31 |
| HUD, item strip | top right: hearts, energy, glints; ↑ at full size opens the upgrades | [MM] | dd_16 |
| Health | hearts with halves; 3 at the start, up to 10 | [GU], [PU] | dd_01 |
| Falling to 0 | back to full size with full health; the clock +1; the carried thing is lost, the satchel's aren't | [MM], [GU] | dd_02, dd_07 |
| Long falls | a fall over 12 tiles sends Dot back to full size; the feather charm stops it, and a flyer in hand makes it a glide | [TV], [AQ] | dd_02, dd_22 |
| Hazards | thorns, slime, acid, poison; pink stalactites drip slime that burns (a jar held up catches it); spore stew protects | [MH], [TV], [AQ] | dd_10, dd_22, dd_34 |
| Currency | glints worth 1, 5, and big glints worth 50, two in each dangerous cave | [MH], [GU] | dd_11 |
| Upgrade spots | a spot gives the first level of its line, or the second once Dot has the first, or glints once full; many spots each; hearts and eggs from any heart or egg spot | [PU], [GU] | dd_13, dd_34 |
| Energy | pep eggs; thrown damage rises with it | [MH], [PU] | dd_04, dd_26 |
| Kicks, spin | Kick Clogs (↑ + B), the second hurts; the Spin Top turns shots and hoppers away whenever Dot is in the air, the second hurts them | [PU] | dd_06 |
| Sprint, drop | Fizz Drop (double tap ← or →); Drop Bangle (double tap ↓ on a ledge) | [MH] | dd_07, dd_31 |
| Higher jumps | Spring Bean, Bean II (↑ + A) | [MH], [PU] | dd_02 |
| Satchel | ↓ + A stores the thing held or takes one out; full, it swaps them | [PU] | dd_07, dd_34 |
| Command bugs | Buzzbook: stand on a bug, ↑; Buzzbook II: germs too | [MH], [PU] | dd_09 |
| Bug Musk, whistle | bugs can't hurt Dot; ↑↑ calls Dash | [MH] | dd_08, dd_10 |
| Armour | Shell Vest (from the Great Earwig) halves damage; Dash's Plate (the smith, from a hard block) | [GU], [PU], [LAST] | dd_10, dd_23, dd_26 |
| Creatures | wall climbers (ride them up walls), spitters (their fire breaks weak blocks; carried, B fires), cave octopods (one drops a red egg), tanks and planes, the pink blob (swallows a thing or a small creature and spits it out) | [GU], [AQ], [MH] | dd_27, dd_34 |
| Flyports | a ferry fly between the two pots, and one between the reading lamp and the great lamp | [GU], [AQ] | dd_29, dd_34 |
| The pilgrimage | the pilgrim by the beacon in Glimmer; the ferry to the great lamp; at its far end, shrink, climb to the Glow; back in Glimmer, a Lumen | [AQ] | dd_29 |
| Shops | five, each a cheap thing, a tool and one 25-glint upgrade | [GU] | dd_14 |
| Autosave, no location save | on nearly every action; every session starts in the room at full size | [GU], [MM] | dd_01, dd_12 |
| Bosses | the Rotifer (stomp it and it gapes; hit the tongue), the Great Earwig (huge health; axes, venom, the wall pod, a plated Dash), Sir Sprocket (he throws his head; inside him to the console) | [STORY], [GU], [ROACH], [AQ] | dd_24, dd_16, dd_23, dd_31 |
| Endings | escape (gold); after the escape, Nib and Tock's 1,000 (cherry); after the credits, locked in again | [GU], [TV] | dd_31, dd_32 |
| Goals | 100 glints; escape the lumber room; restore the room's balance | [MH], [GG] | dd_11, dd_31, dd_32 |

## Full-scale counts

| | Mini & Max | DOT & DASH |
|---|---|---|
| Sizes | 4 | 4 (full, small, micro, tiny) |
| Upgrades tracked | 39 [LAST], [PU] | 39: 23 abilities, 8 heart buttons (6 whole, 2 half), 8 pep eggs |
| Hearts | 3 to 10 | 3 to 10 |
| Upgrade spots | "many, many, many locations" [GU] | 65 fixed spots in the micro world and on hand-made places, plus shops and favours |
| Shops | 5 | 5: Taper (Shrink Tonic II), Sprig (Spring Bean), Mother Fluff (Satchel), Clod (Drop Bangle), Knot (Feather Charm) |
| Favours | 12 wiki quests and many more [MH], [AQ] | all of them in kind (below) |
| Bosses | Water Bear, the Roach, Gearhead | the Rotifer, the Great Earwig, Sir Sprocket |
| Door price, true price | 500, 1,000 | 500, 1,000 |
| Places | several cities and many small places | 21 towns and landmarks in the micro world, 10 areas at small size, 7 hand-made micro places |
| Dangerous caves | "often 2 big shinies per cave" | 10 caves, 2 big glints each, and 4 more big glints |
| Music | 25 tracks [GU] | 15 tunes and 2 jingles |

### The favours and their counterparts

| Mini & Max [MH], [GU], [AQ] | DOT & DASH |
|---|---|
| A Chat With William (Eleanor's tea) | Granny Thimble's spectacles for Professor Crumb in the west pot: Shrink Tonic |
| Max's fleas | nips in Dash's fur |
| the sick Dust Bunny | mites in Puffin's fluff, for Mother Fluff |
| blue dust for a new bunny | Tufty and the blue drift |
| Silverfish Slaying | six paperfish for Inky the bookworm |
| Lost Babies | lost waxlings home to Tuftville |
| the artifact | the Buzzbook's cover stone from Snarl |
| a variety of virus; the scroll | a purple germ and a Lumen scroll for Elder Tallow |
| A Critic's Cravings | a green germ for Madame Silk, in her web in the top corner, reached by a wall climber |
| Muted Monarch | Queen Wriggla's egg from the Lancers' Den |
| Hungry Hungry Termites | a green twig for Chef Borer |
| The Missing Shipment | Captain Peat's crate from the crash site |
| Algaean Lovers | Lichen and Sorrel's letters |
| slime blocks for a builder | glue blocks for Mason Moss |
| the dying soldier | a jar of water for Private Peat-Moss |
| The Blacksmith's Gift | a hard block for the Moss Smith: Dash's Plate |
| An Engineer's Request | the gear for Spinner in the money box: the console code |
| the outlet engineer | Dr. Volt throws the breaker |
| honey for the bee | Mrs. Humble in the hive |
| the half face | Half a Face in the red book, and its other half in the blue |
| the exiled Lamptopian; the exiled Mol Dok | Wickless on the portrait; Old Smut on the SIEGE box |
| the Lamptopian throwing shinies | a lumen lad on the shelf; glints on the plant's top leaf |
| the living shiny | the living glint in the corner by the door |
| the historian | Archivist Tussock of Loamton |
| the tablet; the blue eye; the seed; the red egg; the pink slime | Old Cap, One-Eye, the Seedkeeper, Gill, Chef Morel of Tickburg |
| the strong drink for the sheriff spider | a mushroom drink for Cowpoke in the caboose |
| the hijacked mouse | Barley the shrew and the tin mage in his head |
| the pilgrimage; the nuke for the Din | the Glow at the top of the great lamp; the Big Bang for the Old Filament (80) |
| the angel statue | the stone owl and a quake block: Kite Wings |
| the key: guard or prisoner | the cell key: the tin knight (10) or Pell (30 later, in Tuftville) |
| the cat food | Granny's cat food for Cook Ratchet |
| the tax | the tin mage's bridge, 50 |

## The three goals

| Goal | Mini & Max | DOT & DASH |
|---|---|---|
| Beacon | collect 100 shinies | COLLECT 100 GLINTS |
| Saucer | escape the storage room | ESCAPE THE LUMBER ROOM |
| Alien | find Pongo and restore balance | RESTORE THE ROOM'S BALANCE |

## Readings we had to choose

- **The fourth size** is Dot herself at half her micro height, in the same
  place, not another world inside every micro tile; ↓ takes it, ↑ leaves it
  where there is room [PU], [GAP].
- **The hold** to shrink or grow is 90 frames ("hold down for a few
  seconds" [ST]).
- **The micro world:** each exposed tile of the room becomes a 40 × 32
  chunk generated from its material and position, so the same spot always
  holds the same world; a walk runs to 48 chunks either way.
- **Glints are finite:** each one taken stays taken (the save keeps up to
  6,000), so the 1,500 for both prices come from favours and big glints
  ("exceeds what natural exploration provides").
- **Long falls:** over 12 tiles; a flyer carried in the hands caps the fall
  speed (our form of "use a floating enemy to reset your fall distance").
- **Full-size blocks** are 16 px, the size of Dot's head; the knob is 14
  tiles up, out of a plain full-size jump.
- **Dash** sits after Dot has stood still beside him for about a second; ↑
  then hears his tip. Standing on him, ↑ stops or starts him.
- **Both armours** can be had: the smith makes Dash's Plate and the Great
  Earwig leaves the Shell Vest [LAST], [GU].
- **Tock** gives the true ending as soon as he is paid, since he only takes
  the thousand after the escape.
- **The living glint** gives 25 glints when caught; no source says what it
  gives.
- **The archivist** takes 5 glints and gives nothing back; the source says
  only that he wants a donation.
- **Where things are:** every spot, town and cave is ours.
- **The bug armies:** red lancers turn up after 333 glints (or on meeting
  the queen), the queen's tin mice after 667 [GU].

## What is ours

- **Name:** DOT & DASH (1989, Beamdown Softworks).
- **Story:** Mum and Dad are at the theatre; big brother Otto throws a
  party and locks Dot and Dash in the lumber room.
- **Characters:** Dot, Dash, Granny Thimble, Professor Crumb, Queen Tabitha
  the cat, Sir Sprocket, Tock, Nib, the Old Filament, the Glow, the waxkin,
  the moss folk, the sporelings, the woodworms, the lumens, the tin court,
  Mother Fluff and Puffin, Mrs. Humble, Wickless, Old Smut and 30 more.
- **The room**, every tile, every town and landmark, the generator, all text.
- **Things and upgrades:** glints, crackers, darts, boomers, rollers, quake
  blocks, hourglasses, venom, pupae, spores, cinder blocks, the Big Bang;
  the Grip Mitt, Shrink Tonics, Drop Bangle, Satchel, Spring Bean, Feather
  Charm, Kick Clogs, Bug Musk, Buzzbook, Fizz Drop, Spin Top, Pup Whistle,
  Spore Stew, Shell Vest, Dash's Plate and Kite Wings.
- **Pixel art and music:** 15 tunes and two jingles (DOT & DASH, FULL SIZE,
  BUTTON HIGH, SPECK COUNTRY, MOSS AND LOAM, TIN AND GLASS, THE DANGEROUS
  CAVES, THE MOTES, LITTLE MARKET, BETWEEN THE WALLS, BIG TROUBLE,
  LATCHTOWN, SIEGE!, THE PARTY, IN BALANCE, SOMETHING NEW, BACK TO SIZE).

## Additions: none

Only the platform needs every UFO 40 cartridge has: the START pause menu,
saving, and the three UFO 40 goals, which are Mini & Max's own three.

## Controls

| Input | Action |
|---|---|
| ← → | walk |
| A | jump (hold for height) |
| hold ↓ / hold ↑ | shrink / grow one size |
| ↑ | doors, talk, shops; on a bug, command it; on Dash, stop or start him |
| ↑ at full size | the upgrades strip |
| B | lift what you stand on; throw |
| ↑ + B | throw high; with Kick Clogs, kick |
| ↓ + B | set it down under your feet |
| ↓ + A | satchel |
| ↓ ↓ | drop through a ledge (Drop Bangle) |
| ← ← / → → | sprint (Fizz Drop) |
| ↑ ↑ | call Dash (Pup Whistle) |
| START | pause menu |

## Tests

36 scripts, `tests/dd_*.ufs`, every one driven by button presses (the
cheats only set up a scene). The favours (dd_20 to dd_35) are played
through; the route tests prove every part can be reached:

- **dd_40:** the whole room at small size: every pot, shelf, lamp, the
  lintel, the doorways, and the reach of every part from the rug.
- **dd_41:** every area and hand-made micro place, with what is walled off
  or out of reach before its time (the queen's corner, the long drop
  without the feather).
- **dd_42:** every micro strip of the room and the walls (136 + 26), each
  flooded with the demo player's moves at micro size and tiny: nothing out
  of reach.
- **dd_43:** a nook behind a one-tile gap: out of reach at micro size, in
  reach tiny.

## Not confirmed

- Whether Chitin and Dog Armour exclude each other: the wiki says so, other
  sources don't. We give both.
- The exact hold time to change size and the exact fall height.
- The rewards for the living shiny and the historian.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Mini & Max". https://ufo50.miraheze.org/wiki/Mini_%26_Max
- [TV] TV Tropes recap, "UFO 50 Game 45: Mini and Max": long falls, Max
  returning, found towns shown, Pongo after the escape.
  https://tvtropes.org/pmwiki/pmwiki.php/Recap/UFO50Game45MiniAndMax
- [MM] Steam guide "The missing manuals", Mini & Max section.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [GU] Steam guide "Mini & Max Ultimate Guide" (Llamakazi).
  https://steamcommunity.com/sharedfiles/filedetails/?id=3348941442
- [PU] Steam guide, Lasagevev0's powerup guide: Potion II and gaps, Max's
  stop and go, tiers and spots, Sacred Wings, Spin Bauble, the backpack
  swap. https://steamcommunity.com/sharedfiles/filedetails/?id=3343009452
- [AQ] Steam guide "All Quests, Items, and Upgrade locations": the
  doorknob route with a block, the Plasmage bridge, Gearface's console and
  code, the pilgrimage, every favour.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3349437433
- [GG] Steam guide "Gift, Gold & Cherry": the three goals.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335446139
- [SM] Steam thread: the micro layouts are the same for everyone.
  https://steamcommunity.com/app/1147860/discussions/0/4699034922679313774/
- [GAP] Steam thread: "the item that lets me go through one block gaps".
  https://steamcommunity.com/app/1147860/discussions/0/4849904427675883309/
- [STORY] Steam thread on the story order: the water bear, the war book.
  https://steamcommunity.com/app/1147860/discussions/0/4849903998515318846/
- [ROACH] Steam thread on the Roach.
  https://steamcommunity.com/app/1147860/discussions/0/4849904427677964160/
- [LAST] Steam thread on the last items: 39 upgrades, both armours.
  https://steamcommunity.com/app/1147860/discussions/0/4849904345519051705/
- [LZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 45 - Mini & Max".
  https://lizstar64.github.io/reviews/2024/10/20/UFO50-45.html
- [ST] Static Canvas, "The UFO 50 Diaries: Mini and Max".
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-mini-and-max
- [PD] PixelDie, "Ranking every UFO 50 game after 100 hours".
  https://pixeldie.com/2024/11/13/ranking-every-ufo-50-game-after-100-hours/
- An independent review of the first build against these sources (the
  fourth size, the true-ending gate, upgrade tiers, lethal falls, full-size
  blocks, Dash's ways, the SIEGE boss, and a list of missing favours and
  creatures) was applied in full.
