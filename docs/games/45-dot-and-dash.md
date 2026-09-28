# 45 · DOT & DASH

*Internal design document. Not shown in the product.*

## Tribute to

**Mini & Max** (UFO 50 game #45, Mossmouth). The rules were researched from
text only: the community wiki, the Steam guides "The missing manuals" and
"Mini & Max Ultimate Guide", Steam forum threads and written reviews (listed
under Sources). No UFO 50 images, video, sprites, music, maps or code were
used as references, and nothing was read from an installed copy of the game.
No room layout, town or cave was reconstructed.

**Map type: one hand-made room, with generated worlds inside it.** Mini &
Max's "world map" is a single storage room seen at four nested sizes. The
room is hand-made; the worlds inside its pixels are generated, but fixed and
the same for every player, with hand-placed towns and landmarks [MH], [SM].
Ours is built the same way:

| Structure | Mini & Max (text sources) | DOT & DASH |
|---|---|---|
| Story | locked in the storage room while big sister throws a party; the dog suggests getting small [MH], [LZ] | Dot and her talking dog Dash, locked in the lumber room by big brother Otto during his party |
| Sizes | full, small, micro, deeper micro; the two smallest need Magic Potion I and II [MH], [GU] | FULL, SMALL, MICRO, DEEP; Shrink Tonic I and II |
| Full size | the room as a level select: where you shrink decides where you arrive [PD], [MM] | the room at 2 px a tile; every spot has its own world |
| Small | "a big wide room while small, all one big level": carpet, pots, bookshelf, door, clock, lamp, walls [LZ] | the same room at 8 px a tile, 160 × 90 tiles: rug, two pots, hanging shelf with the clock, money box, bookshelf, the lamp, the door |
| Micro | "a nearly endless procedurally-generated microscopic world", the same for everyone, with upgrade copies at fixed spots [SM], [LZ] | every exposed tile becomes a 40 × 32 chunk, generated from its material and position; strips of up to 48 chunks |
| Deeper micro | needed for the final boss and some items [GU] | every micro tile opens again, one level deeper |
| Towns | Beldor, Protis, Yarn, Doorknob City, the Lampian city, Mol Dok city, Termite City, the Dust Bunny Warren [MH], [GU] | Loamton, Fernby, Tuftville, Latchtown, Glimmer, Tickburg, Wormwood, Fluff Hollow, and 13 smaller hand-made places |
| Between the walls | reached through the fried power outlet; Termite City, the Roach [GU], [SR] | BETWEEN THE WALLS, behind the fried outlet: Wormwood and the Great Earwig |
| The war book | a board-game book on the middle shelf with a tank boss [SS] | SIEGE!, a board-game box on the middle shelf with the Siege Engine |
| Critical path | shinies, pay King Mittens 500, he kidnaps the dog, beat Gearhead with Potion II, escape [GU] | glints, pay Queen Tabitha 500, she keeps Dash, beat Sir Sprocket from inside with Tonic II, escape |
| True path | the lamp off with the Switchkeeper, the Din's hint, Pongo behind the throne, 1,000 to the Clockkeeper [GU] | the Wick Warden turns the lamp off, the Old Filament's hint, Nib behind the throne, 1,000 to Tock |

## Mechanics checklist

Every line was compared with the code; the tests named prove it with button
presses.

| Mechanic | How DOT & DASH does it | Source | Test |
|---|---|---|---|
| Walk, jump | ← → walk; A jumps, held for height; steering in the air | [MM], [MH] | dd_02 |
| Shrink | hold ↓ (90 frames) to shrink into what is under you, one size | [MM], [ST], [GU] | dd_03 |
| Grow | hold ↑ (90 frames) to grow back one size | [MM], [LZ] | dd_03 |
| Place follows size | where you stand when you shrink is where you arrive, and growing puts you in front of the same spot | [MM], [PD] | dd_03, dd_07 |
| The clock | growing to full size, or falling to 0, moves the clock on a minute; it starts at 8:05; no time limit | [GU] | dd_02, dd_03 |
| Carried things change size | what Dot carries grows and shrinks with her | [MH] | dd_03 |
| Doors | ↑ at a doorway or entrance | [MM] | dd_20 |
| Lift from below | B lifts the thing Dot stands on overhead; B again throws | [MM], [MH] | dd_04 |
| High throw | ↑ + B throws in a high arc | [MM] | dd_04 |
| Place | ↓ + B sets it down under her feet, to build steps | [MM] | dd_04, dd_05 |
| Blocks | plain, cracker (explodes), dart (flies straight), heart box, gift box (upgrade), boomer (comes back), roller (rolls, hurts only foes), glue, hourglass (freezes all), quake (knocks foes over), venom (poison that spreads), mold fruit, spore (a bouncy mushroom), pupa (a flutter to ride), hard blocks | [MH] | dd_05 |
| Critters | Grip Mitt lifts critters; Grip Mitt II lifts faster | [MH] | dd_04 |
| Dash as a weapon | pick Dash up and throw him; hurt, he vanishes for a while; with Dash's Plate he can't be hurt | [SM2], [GU] | dd_08 |
| Shrink on creatures | stand on Dash (fleas: nips) or Puffin the dust bunny (mites) and shrink into their fur | [MH] | dd_08, dd_21 |
| HUD | top right: hearts, the energy bolt, the glint count | [MM] | shots |
| Item strip | at full size ↑ opens the upgrades along the top of the screen, each with what it does; B, A or ↓ closes it | [MM] | dd_16 |
| Health | hearts with halves; start 3, up to 10 | [GU], [MH] | dd_01 |
| Falling to 0 | grow to full size with full health; the clock +1; what is carried is lost unless in the Satchel | [MM], [GU] | dd_02, dd_07 |
| Fall damage | a fall over 12 tiles hurts, over 24 hurts more; the Feather Charm stops it | [MH] | dd_02 |
| Hazards | thorns, slime, acid, poison; Spore Stew makes slime and acid harmless | [MH] | dd_10 |
| Currency | glints worth 1, 5 and big glints worth 50 | [MH] | dd_11 |
| Big glints | two in each of ten dangerous caves, and more on landmarks | [GU] | dd_11, dd_42 |
| Micro upgrade copies | an upgrade in the micro world sits at several fixed spots; once one is taken, the others are small glints | [SM] | dd_13 |
| Energy | pep eggs raise energy by one (up to 9); thrown damage rises with it | [MH], [GU] | dd_04, dd_26 |
| Kicks | Kick Clogs: ↑ + B kicks things; Kick Clogs II: kicks hurt | [MH] | dd_06 |
| Spin | Spin Top: jump into shots and hoppers to knock them away; Spin Top II: they get hurt | [MH] | dd_06 |
| Sprint | Fizz Drop: double tap ← or → | [MH] | dd_07 |
| Drop through | Drop Bangle: tap ↓ twice on a ledge | [MH] | dd_07 |
| Higher jumps | Spring Bean, Spring Bean II (↑ + A) | [MH] | dd_02 |
| Satchel | ↓ + A stores a thing; Satchel II stores two; they survive a fall to 0 | [MH], [GU] | dd_07, dd_12 |
| Command bugs | Buzzbook: stand on a bug, ↑ commands it (moths fly up and carry Dot, buzzers dart ahead); Buzzbook II: germs too | [MH] | dd_09 |
| Bug Musk | bugs can't hurt Dot | [MH] | dd_10 |
| Whistle | Pup Whistle: double tap ↑ calls Dash | [MH] | dd_08 |
| Armour | Shell Vest halves damage; Dash's Plate makes Dash unhurtable | [MH], [GU] | dd_08, dd_10, dd_23 |
| Wings | a quake block thrown by the stone owl gives Kite Wings: Dash flies | [MH] | dd_08, dd_30 |
| Poison spreads | a poisoned creature infects others it touches | [MH] | dd_05 |
| Wall turrets | pods in the walls shoot what passes, bosses too: the lair's pod pelts the Great Earwig | [MH] | dd_16 |
| Pink blob | a gulp eats ants | [MH] | dd_30 |
| Shops | five, each with a cheap thing, a tool and one 25-glint upgrade | [GU] | dd_14 |
| Autosave | on glints, upgrades, quest steps, a fall to 0, growing to full size | [GU] | dd_12 |
| No location save | every session starts in the room at full size | [MM] | dd_01, dd_12 |
| Bosses | the Rotifer (stomp it and it gapes, then hit it), the Great Earwig (huge health; axes, venom, pods or a plated Dash), the Siege Engine, Sir Sprocket (he throws his head; climb on his shoulders and shrink inside to break the mainspring) | [SS], [GU], [SR] | dd_24, dd_23, dd_29, dd_31 |
| Endings | escape through the door after Sir Sprocket (gold); the true ending after restoring balance (cherry); reloading after the credits, Granny hints Otto locked Dot in again | [GU] | dd_31, dd_32 |
| Secret | a grasshopper past the pink blob speaks about play | [MH] | dd_30 |
| Goals | 100 glints; escape the lumber room; restore the room's balance | [MH], [GG] | dd_11, dd_31, dd_32 |

## Full-scale counts

| | Mini & Max | DOT & DASH |
|---|---|---|
| Sizes | 4 | 4 |
| Upgrades tracked | 39 [SL] | 39: 23 abilities, 8 heart buttons (6 whole, 2 half), 8 pep eggs |
| Hearts | start low, up to 10 | 3 at the start, 10 with every button |
| Energy eggs | 8 | 8 |
| Shops | 5, each with one 25-shiny upgrade | 5: Taper in Tuftville (Shrink Tonic II), Sprig in Fernby (Spring Bean), Mother Fluff in Fluff Hollow (Satchel), Clod in Loamton (Drop Bangle), Knot in Wormwood (Feather Charm) |
| Quests | 12 on the wiki, plus unlisted ones | all 12 in kind, plus the outlet, the war book, the lamp pilgrimage, the dry soldier, the blue thread, the mixologist, and the smaller favours in the towns |
| Bosses | Water Bear, Giant Roach, the war book's tank, Gearhead | the Rotifer, the Great Earwig, the Siege Engine, Sir Sprocket and his mainspring |
| Door price, true price | 500, 1,000 | 500, 1,000 |
| Towns and landmarks | several cities and many small places | 21 hand-made places in the room's worlds, 6 areas and 5 special worlds |
| Dangerous caves | "often 2 big shinies per cave" | 10 caves, 2 big glints each, and 4 more big glints on landmarks |

### The quests and their counterparts

| Mini & Max [MH], [GU] | DOT & DASH |
|---|---|
| A Chat With William (Eleanor's kettle) | Granny Thimble's spectacles for Professor Crumb in the west pot: Shrink Tonic |
| Max's Misfortune (fleas) | nips in Dash's fur |
| Ambulance for Alfie (dust mites) | mites in Puffin's fluff, for Mother Fluff |
| Silverfish Slaying (books) | six paperfish in the books, for Inky the bookworm |
| Lost Babies | lost waxlings home to Tuftville |
| A Critic's Cravings (a green virus) | a green germ for Madame Silk |
| Muted Monarch (the queen's egg) | Queen Wriggla's egg from the Lancers' Den: Wormwood opens |
| Hungry Hungry Termites (green wood) | three green twigs for Chef Borer |
| The Missing Shipment | Captain Peat's crate from the crash site on the coins |
| Algaean Lovers (letters) | Lichen in Fernby and Sorrel in Loamton |
| The Blacksmith's Gift | a hard block for the Moss Smith: Dash's Plate |
| An Engineer's Request (a gear) | a gear from the walls for Spinner in the money box |
| the scientist at the outlet | Dr. Volt throws the breaker: the fried outlet becomes a door |
| the war book | SIEGE!, and the Big Bang firecracker at its far end |
| the Lampian pilgrimage, the Switchkeeper | Glimmer under the shade, the High Lumen, the Wick Warden |
| the dehydrated soldier | a jar of water for Private Peat-Moss |
| the wall spider's blue thread | Weepy's spool |
| the mouse mixologist | Barley the shrew mixes mushroom drinks |
| the water bear | the Rotifer in the whirlpool west of Fernby |

## The three goals

| Goal | Mini & Max | DOT & DASH |
|---|---|---|
| Beacon | collect 100 shinies | COLLECT 100 GLINTS |
| Saucer | escape the storage room | ESCAPE THE LUMBER ROOM |
| Alien | find Pongo and restore balance | RESTORE THE ROOM'S BALANCE |

## Readings we had to choose

- **Four sizes:** the small size needs no tonic; Shrink Tonic I opens micro
  and Tonic II deep, reading the potions' "shrink even smaller" and "three
  levels down" [MH], [GU].
- **The hold** to shrink or grow is 90 frames ("hold down for a few
  seconds" [ST]).
- **The micro world:** each exposed tile of the parent becomes a 40 × 32
  chunk generated from its material and position, so the same spot always
  holds the same world; a walk runs to 48 chunks either way, then the ground
  ends in a wall. Deep strips work the same inside micro tiles.
- **Glints are finite:** each glint, once taken, stays taken (the save
  keeps 1,536 of them), so the 1,500 for both prices come from quests and
  big glints as the guide says ("exceeds what natural exploration
  provides").
- **Energy** is only the thrown-damage level (1 to 9); no source says it is
  spent.
- **Fall damage:** over 12 tiles one heart, over 24 two.
- **Both armours** can be had: the Moss Smith makes Dash's Plate from a
  hard block, and the Great Earwig leaves the Shell Vest. The wiki says one
  per game, the guide says the Roach gives the Chitin Armor; we follow the
  guide for the vest and let the smith make only the plate.
- **Where the eggs and heart buttons are:** our own spots, in towns,
  caves, quests and the deep stacks.
- **What falls with you:** the carried thing is lost at 0 health, the
  Satchel's aren't; at full size a carried thing is pocketed and comes back
  when Dot shrinks.
- **The honey and acid jars:** a jar fills from what Dot wades in or stands
  under; honey makes a mushroom drink with Barley, acid eats through a
  rusty grate between the walls.
- **The Siege Engine** is our tank for the war book: a board-game box on
  the middle shelf, with the Big Bang at its far end for the Old Filament.
- **Latchtown's inside:** the city on the door has a tin mage who opens the
  throne hall for 50 glints, a prisoner (Pell) and his guard, and a cook.
  The sources name Doorknob City's king and the hidden Pongo but nothing
  else inside; these are our reading of an unlisted court.
- **The bug armies at war:** once Dot has found 333 glints (or met the
  queen) red lancers turn up in the room and the micro world; past 667 the
  queen's tin mice come out too and fight them where they meet.
- **The jump:** about five tiles high with A held at small size; the
  Spring Bean jumps (↑ + A) go higher.

## What is ours

- **Name:** DOT & DASH (1989, Beamdown Softworks).
- **Story:** Mum and Dad are at the theatre; big brother Otto throws a
  party and locks Dot and Dash in the lumber room. Dash, who talks only when
  nobody else listens, suggests getting small.
- **Characters:** Dot, Dash, Granny Thimble, Professor Crumb, Queen Tabitha
  the cat, Sir Sprocket, Tock the clockkeeper, Nib, the Old Filament, the
  waxkin of Tuftville, the moss folk of Fernby and Loamton, the sporelings
  of Tickburg, the woodworms of Wormwood, the lumens of Glimmer, the tin
  court of Latchtown, Mother Fluff and Puffin, and 30 more.
- **The room**, every tile of it, every town and landmark, the generator
  and its chunk rules, all text.
- **Things and upgrades:** glints, crackers, darts, boomers, rollers, quake
  blocks, hourglasses, venom, pupae, spores; the Grip Mitt, Shrink Tonics,
  Drop Bangle, Satchel, Spring Bean, Feather Charm, Kick Clogs, Bug Musk,
  Buzzbook, Fizz Drop, Spin Top, Pup Whistle, Spore Stew, Shell Vest, Dash's
  Plate and Kite Wings.
- **Pixel art and music:** twelve tunes and two jingles (DOT & DASH, FULL
  SIZE, BUTTON HIGH, SPECK COUNTRY, THE MOTES, LITTLE MARKET, BETWEEN THE
  WALLS, BIG TROUBLE, LATCHTOWN, SIEGE!, THE PARTY, IN BALANCE, SOMETHING
  NEW, BACK TO SIZE).

## Additions: none

Only the platform needs every UFO 40 cartridge has: the START pause menu,
saving, and the three UFO 40 goals, which are Mini & Max's own three.

## Controls

| Input | Action |
|---|---|
| ← → | walk |
| A | jump (hold for height) |
| hold ↓ | shrink one size |
| hold ↑ | grow one size |
| ↑ | enter a door; at full size, the item strip |
| B | lift what you stand on; throw |
| ↑ + B | throw high; with Kick Clogs, kick |
| ↓ + B | set it down under your feet |
| ↓ + A | store in the Satchel / take out |
| ↓ ↓ | drop through a ledge (Drop Bangle) |
| ← ← / → → | sprint (Fizz Drop) |
| ↑ ↑ | call Dash (Pup Whistle) |
| on a bug, ↑ | command it (Buzzbook) |
| START | pause menu |

## Tests

34 scripts, `tests/dd_*.ufs`, every one driven by button presses (the
cheats only set up a scene). The quests (dd_20 to dd_33) are played through;
the route tests prove every part can be reached:

- **dd_40:** the whole room at small size, from the rug to every shelf,
  pot, the door top and the lamp.
- **dd_41:** every area and special world (Granny's thimble, the walls,
  Fluff Hollow, SIEGE!, the lair, the clockworks, the fur worlds, Sir
  Sprocket's insides, behind the throne, the deep stacks).
- **dd_42:** every micro strip of the room and the walls, walked by the
  demo player and checked with a flood fill: no glint, upgrade or door out
  of reach.
- **dd_43:** deep strips sampled across every material.

## Not confirmed

- Whether the Chitin Armor and Dog Armor exclude each other: the wiki says
  so, a guide says the Roach gives the armour. We give both.
- The exact hold time to change size, and the fall-damage heights.
- What else lives inside Doorknob City.
- Whether the deeper micro scale is a third or a fourth level (we read it
  as the fourth).
- Where each heart and energy egg lies (ours).

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Mini & Max": story, controls, upgrades and
  their effects, blocks, shinies, quests, goals, the secret.
  https://ufo50.miraheze.org/wiki/Mini_%26_Max
- [MM] Steam guide "The missing manuals - How to play UFO 50 games", Mini &
  Max section: the buttons, hold ↓ / ↑ to change size, ↑ + B high throw, ↓ + B
  place under your feet, ↑ at full size for the item section, the HUD at the
  top right, 0 health grows you to full size, no location save.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [GU] Steam guide "Mini & Max Ultimate Guide": sizes, shops and prices,
  zones, both paths, bosses, the clock, what autosaves.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3348941442
- [GG] Steam guide "Gift, Gold & Cherry": the three goals.
  https://steamcommunity.com/sharedfiles/filedetails/?id=3335446139
- [SM] Steam thread on whether the micro layouts are identical: the same
  for everyone, upgrade copies turning into small shinies.
  https://steamcommunity.com/app/1147860/discussions/0/4699034922679313774/
- [SS] Steam thread on the story order: the water bear, the war book.
  https://steamcommunity.com/app/1147860/discussions/0/4849903998515318846/
- [SR] Steam thread on the Roach: ways to beat it.
  https://steamcommunity.com/app/1147860/discussions/0/4849904427677964160/
- [SL] Steam thread on the last items: 39 upgrades, the outlet as a door.
  https://steamcommunity.com/app/1147860/discussions/0/4849904345519051705/
- [LZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 45 - Mini & Max".
  https://lizstar64.github.io/reviews/2024/10/20/UFO50-45.html
- [ST] Static Canvas, "The UFO 50 Diaries: Mini and Max".
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-mini-and-max
- [PD] PixelDie, "Ranking every UFO 50 game after 100 hours".
  https://pixeldie.com/2024/11/13/ranking-every-ufo-50-game-after-100-hours/
- [SM2] Search-result summaries of reviews: the dog thrown as a
  projectile, poison drinks.
