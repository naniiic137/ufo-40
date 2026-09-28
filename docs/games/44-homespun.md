# 44 · HOMESPUN

*Internal design document. Not shown in the product.*

## Tribute to

**Pilot Quest** (UFO 50 game #44, Mossmouth, "November 1988", file
ZOL2.UFO). The rules were researched from text only: the community wiki's
raw page, the Steam guide "The missing manuals" (its Pilot Quest section),
Steam guides and threads (some only through search-engine summaries),
written reviews (listed under Sources; the full notes are in the research
folder, `44-pilot-quest.md`, and the controls in `controls-41-44-46.md`).
The owner confirmed the two buttons from the original's on-screen prompts:
Button 1 (our A) interacts, Button 2 (our B) is the yo-yo. No UFO 50
images, video, sprites, music, maps or text were used as references, and
nothing was taken from a UFO 50 install.

**Map type: generated per save.** "Every new save file randomizes
road-blocks, the locations of dungeons, and other entities in the wild
zone" [MH], "cave entrances are all randomized" [LIZ], and "some routes are
randomly blocked" [SSB]. So HOMESPUN's Wilds come from a generator of its
own (`homespun_world.c`), seeded per save. Only the original's dungeon
*interiors* are hand-made ("hidden rooms or shortcuts to the boss" [SRCH]),
so ours are too: three dungeons drawn from scratch. Nothing here is the
original's map.

| Full scale | Pilot Quest | HOMESPUN |
|---|---|---|
| Camp | the Moon Crystal, the Gumma Tree's plants, the workbench, the Meat Shack, 2 houses (3 workers each), 6 workbenches, the Research Machine, 2 silos, the research list, Unktomi's silk, the NG+ statues [MH], [SSB], [MM] | the Glowstone, Old Burl's glintbuds, the workbench, Gristle's smokehouse, 2 huts (3 hands each), 6 anvils, the Thinker, 2 bins, Dr. Orrery's 6 researches, Mother Loom's thread, the 3 standing stones |
| Plants | escalating 10 → 10,240 drops [MH] | 11 glintbuds, 10, 20, 40 … 10,240 glints |
| The Wilds | an open overworld with caves, some roads blocked by save [MH], [LIZ], [SSB] | 8 × 6 screens in six regions (meadow, marsh, woods, dunes, crags, ashlands), 11 caves, 4 hopstones and camp's own |
| Dungeons | 3 main dungeons, one ship part each [ST-LEN], [MH] | 3: the Burrow (7 rooms), the Bramble Vault (10) and the Hive Spire (10), each with a crack-wall shortcut or hidden room |
| Foes | 19: 2 low, 9 normal, 8 high tier [MH] | 19 in the same tiers (a list is below) |
| Bosses | 4 dungeon-boss icons, the ElectroBomber guarding the ruins, Unktomi, Nozzlo [MH] | the Grumm King, the Thornmother, Zizzik, the Volthog (the ruins), Mother Loom, Mawbo |
| Vendors | 4 in 7 possible places; 10 science for 3, 15 silk for 5, energy for 3 and for 2 [MH], [SRCH] | 4 noodlers in 7 places, the same wares and prices |
| Teleporters | woken for 1, 5, 10 and 15 Zoldnaks [MH] | 4 hopstones, the same prices in the order they wake |
| Folk | Dr. Lumin, Parvina, the Gumma Tree, Slard at the gate, the Shy Friend and its orange friend, Isabell, Lydia [MH], [MM] | Dr. Orrery, Gristle, Old Burl, Tolly, Hush and Tanger, Kit, Madame Shuffle |
| Goals | 3 [MH] | the same 3 |
| Endings | escape; escape with the Donka Donka Idol for the cherry [MH], [SRCH] | two: the Tumbleweed flies home, with or without the Grinning Idol |

## Mechanics checklist

| Mechanic | How HOMESPUN does it | Source | Test |
|---|---|---|---|
| Two layers | an idle camp and timed trips into a top-down wild zone | [MH], [MM] | hs_07 |
| Real time | camp produces while the console is on, in any scene or cartridge; not while it is off | [MH], [MM], [ST-TIME] | hs_20, shell_realtime |
| The Moon Crystal | each yo-yo hit knocks out glints equal to the yo-yo's damage; 1 in 300 a bar instead | [SSB], [MH] ("0.33 %") | hs_01 |
| Walk over them | glints lie where they fall until Wick walks over them | [SSB] | hs_01 |
| Plants | Old Burl plants eleven glintbuds, 10 doubling to 10,240; each makes 1 glint every 2 s, 1.5 s with Fertilizer | [MH] | hs_02 |
| Ingots | the workbench crafts a bar from 1,000 glints | [MH], [SSB], [MM] | hs_03 |
| Meat Shack | 1 bar to build; the first strip free; a strip for 500 glints; six on the racks at most; restocks over time | [SSB], [ST-PQ], [MH], [STATIC] | hs_03 |
| Houses | two huts, 5 bars, rooms for 20 and 100: 1 to 3 hands each | [MH] | hs_04 |
| Workbenches | six anvils at 5 bars; a hand makes a bar every 2 min, two a minute with the Pep Pill | [MH] | hs_04 |
| Research Machine | fixed by a gear found in the Wilds; up to six hands, 1 data a minute each | [MH], [ST-LEN] | hs_05, hs_09 |
| Research | Steel yo-yo 150 bars (damage 2), Fertilizer 100 data, Big Bins 200 bars + 150 thread, Pep Pill 100 bars + 300 data + 100 thread, Star yo-yo 500 data + 200 thread (damage 3), Starfuel 1,000 bars + 1,000 data (after Big Bins) | [MH] | hs_05 |
| A trade | Dr. Orrery trades 100 bars for 20 data | [ST-PQ] | hs_05 |
| Storage caps | 2,500 glints and 5 jerky to start; bins raise them (7,500 and 15 with two, 25 jerky at most) | [ST-PQ], [SSB], [STATIC] | hs_06 |
| Silos | two bins at 50 bars; raised once each (200) after Big Bins | [MH], [ST-PQ] | hs_06 |
| Red at the limit | a store at its limit shows in red | [MM] | (drawn) |
| Silk | Mother Loom moves to camp once beaten and spins thread | [MH], [ST-LEN] | hs_15 |
| The gate | Tolly lets nobody through without jerky; walking out eats all of it, 120 s a strip | [MM], [SSB], [MH] | hs_07 |
| Time is health | the clock runs down; each hit costs time; at 0 Tolly drags Wick home and everything found on the trip is lost | [MH], [MM], [SSB], [STATIC] | hs_08, hs_09 |
| Hit cost | 30 s from most foes | [SSB] | hs_08 |
| Banking | walking back into camp (or hopping home) adds the trip's finds to camp | [MH], [MM] | hs_09, hs_12 |
| Zoldnaks | odds from foes, pots and chests; kept between trips, lost when time runs out; spent in the Wilds | [MH] | hs_09, hs_13 |
| Yo-yo | B throws it the way Wick faces; short reach; foes take 3 to 10 hits; damage 1, 2, 3 | [MH], [SSB], [owner] | hs_10 |
| Blunderbuss | Hush's letter to Tanger earns the thunderpipe: 1 glint a shot, ranged, stuns, and its muzzle hits up close too | [MH], [ST-LEN], [SRCH] | hs_11 |
| Teleporters | four hopstones woken for 1, 5, 10, 15 odds; then free hops between them and home | [MH], [ST-SPIDER] | hs_12 |
| Vendors | four noodlers in seven dungeon places: 10 data for 3 odds, 15 thread for 5, a tonic for 3, a tonic for 2 | [MH], [SRCH], [ST-CH] | hs_13 |
| Gambling | Madame Shuffle deals six shuffled chests for a bet; on average 2 bars and 0.16 jerky per odd | [MH] | hs_13 |
| Isabell | Kit, Wick's sister, in a cave with a pair of chests that fill up every trip | [MH] | hs_13 |
| Energy potions | a tonic adds time; guardians and mini-bosses drop a big one (+100 s) | [MH] | hs_13, hs_14 |
| Loot tables | low foes: 3 glints (1 % a bar); normal: 70 % glints, 5 % bar, 5 % odd, 10 % jerky, 10 % tonic; high: 50 % glints, 25 % a bar, 12.5 % a mix, 12.5 % tonic | [MH] | (code) |
| Dungeons | three, each guarding a ship part, with shortcuts and hidden rooms behind cracked walls | [MH], [ST-LEN], [SRCH] | hs_10, hs_14, hs_19 |
| Per-save randomising | roads, roadblocks, which cave holds what (the dungeons among them), hopstones, the ruins, the noodlers' places | [MH], [LIZ], [SSB] | hs_19 |
| Per-trip changes | some roadblocks open or close from trip to trip; foes, pots and chests come back each trip; parts, the gear and Mother Loom don't | [MH] | hs_19 |
| ElectroBomber | the Volthog, bolts and bombs, guards the ruins and the only way to Mother Loom; back every trip | [MH], [ST-SPIDER] | hs_15 |
| Unktomi | Mother Loom in her lair beside the ruins; beating her is the Beacon | [MH], [ST-SPIDER] | hs_15 |
| Nozzlo | Mawbo appears once all three parts are home; it uses every guardian's attacks and some foes'; 100 base hits; beaten, it flees to a random screen; six times in one trip drops the Grinning Idol | [MH], [SRCH], [ST-CH] | hs_16 |
| Escape | three parts and Starfuel: the Tumbleweed launches | [MH], [SSB] | hs_17 |
| New Game Plus | each escape carves a mark; three stones (HASTE, HIDE, HUNGER) up to 5 each: bars 5 % and the rest 10 % faster, 2 s less per hit, 20 s more per strip; a new camp on the same moon | [MH], [ST-CH] | hs_17 |
| Goals | Beacon: beat Mother Loom. Saucer: escape. Alien: beat Mawbo six times in one trip, then escape with its idol | [MH], [SRCH] | hs_15, hs_16, hs_17 |
| Saving | automatic; a trip in progress resumes where it was | [MM] | hs_18 |

### Readings we had to choose

- **The real-time service.** Production counts only while the console is
  on ([ST-TIME]: "Time does NOT advance when UFO 50 is completely closed").
  Time during a stalled frame loop (a hidden browser tab) counts; it
  arrives all at once on the next update. Camp is saved every 30 s of real
  time wherever you are, so pulling the plug loses at most half a minute.
- **Numbers no source gives:** the smokehouse smokes a strip every 2 min
  (six on the racks); Mother Loom spins 1 thread every 30 s; bars, data and
  thread are capped at 250 and bins add 250 each (so Starfuel's 1,000 needs
  three bin levels); the glint and jerky caps follow [ST-PQ]'s 2,500 → 7,500
  and 5 → 15, carried on to 12,500 and 25. The workbench (which crafts bars
  from glints) is there from the start, so the first bar can be made before
  any anvil exists.
- **Hits:** low foes cost 20 s, every other touch or shot 30 s ("most
  enemies" [SSB]), bosses too; Wick is safe for 1.5 s after a hit and knocked
  back (never out of the room).
- **Tonics:** a foe's tonic +30 s, a noodler's +60 s, a boss's +100 s [MH].
- **Foes and bosses:** the wiki lists names only, so every behaviour, hit
  point and speed is ours, kept to "three to ten hits" [SSB]. The guardians
  take 24 to 30 base hits. Mawbo borrows each guardian's attacks in turn (at
  half their pace) with its own worms and pellets, and walks slowly after
  Wick. Bosses shake for a third of a second before a leap or a dive.
- **Zoldnaks** are kept between trips (the cherry advice is to hoard them
  across trips [ST-CH]) and lost when time runs out.
- **The Blunderbuss** is picked in the START menu ("USE THE THUNDERPIPE" /
  "USE THE YO-YO"): no source says how it is equipped.
- **Teleporting home** counts as coming home (the trip is banked). Hopping
  from camp's stone to an awake one starts a trip, eating the jerky.
- **Mawbo's count** starts again whenever Wick goes home or time runs out;
  the idol, like parts and the gear, only counts once it is home.
- **A HUD dot map:** the corner shows Wick's screen on an 8 × 6 grid, the
  way the era's top-down adventures did; it reveals nothing else.
- **World size:** 48 screens; the original's size is not written anywhere.

## The real-time service (shell)

A small, general hook, not specific to this cartridge:

- `plat_clock_ms()` (platform.h): a wall clock in ms that keeps running
  while frames stall. SDL: `SDL_GetTicks()`. Headless: a pretend clock that
  moves 1/60 s a frame, and `clock_skip MS` moves it with no frames.
- `GameDef.realtime(uint32_t ms)` (gamedef.h): optional. Every update, in
  every scene (menus, library, any cartridge), `app_update` hands each
  cartridge that sets it the milliseconds since the last update. The first
  update after start-up hands 0, so time while the console was off never
  counts. `shell_query("realtime_s")` reports what was handed over.
- HOMESPUN's hook (`hs_realtime`) loads its save on the first call, runs
  the camp's production as arithmetic on the elapsed time (`hs_produce`:
  whole units made, the part of the next unit kept in the save), and saves
  every 30 s. If the SAVE DATA screen deleted the save meanwhile, the camp
  is dropped rather than written back.
- Headless helpers for tests: `clock_skip MS`, `idle MINUTES` (one frame a
  second) and `botloop KEY OP VALUE ROUNDS FRAMES SKIP_MS` (the demo player
  plays, then real time passes, round after round).

## What is ours

- **Name:** HOMESPUN (1988, Beamdown Softworks).
- **Story and folk:** Wick the courier pilot and her ship the *Tumbleweed*,
  crashed on the moor-moon Oddmoor; Old Burl, Gristle, Dr. Orrery, Tolly,
  the hands, Kit, Hush and Tanger, Madame Shuffle, the noodlers.
- **The camp:** the Glowstone, glintbuds, anvils, huts, bins, the Thinker,
  the standing stones, and their layout.
- **The Wilds:** the generator and its six regions, the ruins and the lair,
  the three dungeons (27 rooms), eleven cave rooms, hopstones.
- **Foes:** spinner, grublet (low); splosh, skitter, stingle, gnatter,
  spitbloom, grummle, lobber, shellback, driplet (normal); clacker, red
  clacker, oozer, puffcap, zipsnake, brooder, treadle, leechling (high).
- **Bosses:** the Grumm King, the Thornmother, Zizzik, the Volthog, Mother
  Loom and Mawbo; the Grinning Idol; the thunderpipe.
- **Music:** "Homespun" (title), "Glowstone Camp", "Into the Wilds",
  "Deeper Down" (dungeons and high Wilds), "Dripping Caves", "Guardian",
  "Mawbo", "The Long Way Home" (ending), and two jingles, "Home Safe" and
  "Tolly's Drag".
- **Words:** every line, name and label.

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu (with the
weapon swap under RESUME once the thunderpipe is Wick's) and the three UFO
40 goals, which are Pilot Quest's own (gift, gold, cherry). The shell's
real-time service is how the console gives the original's "while UFO 50
is open" behaviour to a cartridge, not a feature of the game.

## Controls

| Input | Action |
|---|---|
| D-pad | walk, eight ways [MM] |
| B | throw the yo-yo the way Wick faces, diagonals too [owner]; with the thunderpipe, fire it |
| A | talk, build, buy, research, open a chest, wake or use a hopstone [owner], [MM] |
| START | pause; "USE THE THUNDERPIPE" / "USE THE YO-YO" |
| Menus | UP / DOWN choose, A takes, B closes |
| Shuffle's chests | LEFT / RIGHT pick, A opens |

### Not confirmed (flagged)

| Control | Our reading | Why it is unconfirmed |
|---|---|---|
| Swapping the yo-yo and the Blunderbuss | a START-menu item | no source says how the gun is equipped |
| Yo-yo directions | eight, the way Wick last walked | sources say "all directions" for walking only |
| Holding B | nothing extra | not documented |
| Waking and using a teleporter | A while standing on it | the wiki says they are "activated with Zoldnaks", not how |
| Leaving the Wilds | walk back into camp, or hop home | [MM] says return to base; the way is not described |
| A map screen | none; a position dot in the HUD | not documented |

## Tests

`tests/hs_01` … `hs_21` drive the rules with button presses (a few set up
a moment with cheats first), and `shell_realtime` checks the shell service
on its own. `hs_20` checks the real-time behaviour end to end: production
during another cartridge, a two-minute stall caught up in one frame, an
hour switched off not counted, a part-made unit surviving a restart, and
the 30-second save while another cartridge runs.

The demo player (`homespun_bot.c`) plays with real buttons: it walks camp
to each building and works its menus, farms the Glowstone, waits for
jerky, walks out through the gate, routes across the Wilds and dungeons,
breaks cracked walls, sidesteps shots, lines up its throws, beats every
guardian, the Volthog and Mother Loom, and flies home. `hs_22` has it
finish three generated worlds from the title (the Saucer), with real time
passing between spells of play as it does while the console sits in other
games. `hs_23` has it earn the Alien in a fourth world the way players do
(odds from Kit's chests, tonics from a noodler, then six Mawbo fights in one
trip). Its fighting isn't good enough to earn the Alien on every world, so
that test uses one where it does; `hs_16` checks Mawbo's rules on any.
`hs_19` checks 200 generated worlds (every screen reachable on twelve trips,
every way into a screen open, the lair only through the ruins, four
noodlers) and the three dungeons.

## Sources

- [MH] UFO 50 Wiki (Miraheze), "Pilot Quest" (raw page): the story, the
  base and its structures and costs, research, the wild zone, Zoldnaks,
  teleporters, vendors, the Shy Friend quest, Isabell, Lydia's odds, energy
  potions, loot tables, randomisation, enemies by tier, bosses, Nozzlo, goals,
  NG+. https://ufo50.miraheze.org/wiki/Pilot_Quest
- [MM] Steam guide "The missing manuals - How to play UFO 50 games",
  section 44: eight-way walking, A interacts, B throws the yo-yo, the base
  layout, resources shown red at the maximum, production while UFO 50 is
  open (not when closed), Slard and meat at the gate, time top-centre, losing
  everything when time runs out. https://steamcommunity.com/sharedfiles/filedetails/?id=3350227767
- [SSB] Set Side B, "Getting Started in Pilot Quest": 1 drop a hit, walking
  over drops, 1,000 drops an ingot, 30 s a hit, carry 5 meat and later 25,
  3 to 10 hits a foe, the gun costs drops, some routes blocked at random.
  https://setsideb.com/getting-started-in-pilot-quest-ufo-50/
- [ST-PQ] Steam thread "Pilot Quest" (4852154959749844305): 2,500 drops and
  5 meat at first, 7,500 and 15 with silos, 50-ingot silos, a trade of 100
  ingots for 20 science. https://steamcommunity.com/app/1147860/discussions/0/4852154959749844305/
- [ST-TIME] Steam thread "Pilot Quest" (4849904176634249798): time advances
  while UFO 50 runs, in other games and menus, not when it is closed.
  https://steamcommunity.com/app/1147860/discussions/0/4849904176634249798/
- [ST-CH] Steam thread on the cherry (604148566876826367): hoard Zoldnaks,
  the 2-coin energy vendor, all parts needed for the boss to spawn, the
  worms it summons, loops optional. https://steamcommunity.com/app/1147860/discussions/0/604148566876826367
- [ST-LEN] Steam thread "How long is Pilot Quest?": three main dungeons,
  the blaster for letters, stagger-locking, the spider's silk, the gear.
  https://steamcommunity.com/app/1147860/discussions/0/4849904176633070672/
- [ST-SPIDER] Steam thread "where is the spider to make silk?": the ruins
  with shooty guys and a big enemy of lightning and bombs, the spider beside
  them, a teleporter building. https://steamcommunity.com/app/1147860/discussions/0/4849904345519130141/
- [SRCH] Search-engine summaries of the Steam guide "Pilot Quest: Mechanics
  and Cherry Information" (3336701878; the page itself returned HTTP 429):
  vendor wares and prices, hidden rooms and shortcuts, Nozzlo teleporting
  to a random screen after each defeat, about 100 hits.
- [LIZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 44": paths open in
  some playthroughs, randomised cave entrances, dying loses the trip.
  https://lizstar64.github.io/reviews/2024/10/20/UFO50-44.html
- [STATIC] Static Canvas, "The UFO 50 Diaries: Pilot Quest": the sharp
  storage cap, damage costing an alarming amount of time, waiting for the
  shop to restock. https://staticcanvas.substack.com/p/the-ufo-50-diaries-pilot-quest
- [owner] The owner's check of the original's on-screen prompts:
  Button 1 interacts, Button 2 is the yo-yo.
