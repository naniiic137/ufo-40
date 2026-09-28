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

**Map type: one fixed overworld, a few things drawn per save.** "While the
map is generally the same each game, some routes are randomly blocked"
[SSB]; "dungeons and caves stay the same, as do their entry points within
the wild zone. But those entrances may not lead to the same place" [MH];
"transparent barriers (as well as Dungeon and NPC room entrances) are
randomized each playthrough" [MH]. So HOMESPUN has one overworld of its own
(`homespun_world.c` lays it out from a fixed constant, `HS_LAYOUT`, so it
is the same in every save): its regions, the roads that are always open,
the eleven cave mouths, the hopstones, and the ruins with Mother Loom's lair
in the north-east ([MH]: "in the north-east"). Each save draws only which
barrier roads are shut, what each cave mouth leads to (the three dungeons
among them), which chest out in the open holds the gear [ST-GEAR], and
which four of the seven noodler places are used. Nothing changes from trip
to trip except foes, pots and chests coming back. A new loop after an
escape draws those per-save things again [MH], [ST-NG]. The dungeon
interiors are hand-made, as the original's are ("hidden rooms or shortcuts
to the boss" [SRCH]): three dungeons drawn from scratch. Nothing here is the
original's map.

| Full scale | Pilot Quest | HOMESPUN |
|---|---|---|
| Camp | the Moon Crystal, the Gumma Tree's plants, the workbench, the Meat Shack, 2 houses (3 workers each), 6 workbenches, the Research Machine, 2 silos, the research list, Unktomi's silk, the NG+ statues [MH], [SSB], [MM] | the Glowstone, Old Burl's glintbuds, the workbench, Gristle's smokehouse, 2 huts (3 hands each), 6 anvils, the Thinker, 2 bins, Dr. Orrery's 6 researches, Mother Loom's thread, the 3 standing stones |
| Plants | six seeds: 10, 40, 160, 640, 2,560, 10,240 drops [MH] | 6 glintbuds at the same prices in glints |
| The Wilds | one overworld with caves; barriers and cave contents change by save [MH], [LIZ], [SSB] | one fixed map of 8 × 6 screens in six regions (meadow, marsh, woods, dunes, crags, ashlands), 11 caves, 4 hopstones and camp's own |
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
| The Moon Crystal | each yo-yo hit knocks out glints equal to the yo-yo's damage; a bar instead 1 in 300 with the first yo-yo, 1 in 80 (1.25 %) with the steel one, 1 in 10 with the star one | [SSB], [MH] | hs_01 |
| Walk over them | glints lie where they fall until Wick walks over them | [SSB] | hs_01 |
| Plants | Old Burl plants six glintbuds at 10, 40, 160, 640, 2,560 and 10,240; each makes 1 glint every 2 s, 1.5 s with Fertilizer | [MH] | hs_02 |
| Ingots | the workbench crafts a bar from 1,000 glints | [MH], [SSB], [MM] | hs_03 |
| Meat Shack | 1 bar to build; the first strip free; a strip for 500 glints; six on the racks at most; restocks over time | [SSB], [ST-PQ], [MH], [STATIC] | hs_03 |
| Houses | two huts, 5 bars, rooms for 20 and 100: 1 to 3 hands each | [MH] | hs_04 |
| Workbenches | six anvils at 5 bars; a hand makes a bar every 2 min, two a minute with the Pep Pill | [MH] | hs_04 |
| Research Machine | fixed by a gear found in a chest out in the Wilds (a different chest each save); up to six hands, 1 data a minute each | [MH], [ST-LEN], [ST-GEAR] | hs_05, hs_09 |
| Research | Steel yo-yo 150 bars (damage 2), Fertilizer 100 data, Big Bins 200 bars + 150 thread, Pep Pill 100 bars + 300 data + 100 thread, Star yo-yo 500 data + 200 thread (damage 3), Starfuel 1,000 bars + 1,000 data (after Big Bins, the only prerequisite) | [MH] | hs_05 |
| A trade | Dr. Orrery trades 100 bars for 20 data | [ST-PQ] | hs_05 |
| Storage caps | 2,500 glints and 5 jerky to start; bins raise them (7,500 and 15 with two, 25 jerky at most) | [ST-PQ], [SSB], [STATIC] | hs_06 |
| Silos | two bins at 50 bars; raised once each (200) after Big Bins | [MH], [ST-PQ] | hs_06 |
| Red at the limit | a store at its limit shows in red | [MM] | (drawn) |
| Silk | Mother Loom moves to camp once beaten and spins thread; every hand not at a job speeds her up | [MH], [ST-LEN], [ST-SILK] | hs_15 |
| The gate | Tolly lets nobody through without jerky; walking out eats all of it, 120 s a strip | [MM], [SSB], [MH] | hs_07 |
| Time is health | the clock runs down; each hit costs time; at 0 Tolly drags Wick home and everything found on the trip is lost | [MH], [MM], [SSB], [STATIC] | hs_08, hs_09 |
| Hit cost | 20 s from low foes, 30 s from most; the heavy hitters (red clacker, treadle, rocks, bolts, bombs, blasts, shell shots, the guardians) 45 s, Mawbo 60 s | [SSB], [STATIC] | hs_08 |
| Banking | walking back into camp (or hopping home) adds the trip's finds to camp | [MH], [MM] | hs_09, hs_12 |
| Zoldnaks | odds from foes, pots and chests (at the loot tables' rates); kept between trips, lost when time runs out; spent in the Wilds | [MH] | hs_09, hs_13, hs_24 |
| Yo-yo | B throws it the way Wick faces; short reach; foes take 3 to 10 hits; damage 1, 2, 3 | [MH], [SSB], [owner] | hs_10 |
| Blunderbuss | Hush's letter to Tanger earns the thunderpipe: 1 glint a shot, ranged, stuns, and its muzzle hits up close too | [MH], [ST-LEN], [SRCH] | hs_11 |
| Teleporters | four hopstones woken for 1, 5, 10, 15 odds; then free hops between them and home | [MH], [ST-SPIDER] | hs_12 |
| Vendors | four noodlers in seven dungeon places: 10 data for 3 odds, 15 thread for 5, a tonic for 3, a tonic for 2 (each +100 s) | [MH], [SRCH], [ST-CH], [ST-FB] | hs_13 |
| Gambling | Madame Shuffle deals six shuffled chests for 1 odd a go: 1 glint, 4 bars, 8 bars, 2 odds, a strip of jerky, or a driplet that jumps out (and drops nothing) | [MH] | hs_13 |
| Isabell | Kit, Wick's sister, in a cave with a pair of chests that fill up every trip; they roll on the open's table like any chest | [MH] | hs_13 |
| Energy potions | a tonic adds 100 s; only noodlers, the guardians, the Volthog, Mother Loom and Mawbo (each time it falls) give one; ordinary foes never do | [MH], [ST-FB] | hs_13, hs_14, hs_16 |
| Loot tables | low foes: 99 % 3 glints, 1 % a bar; normal: 70 % 5 glints, 10 % 1–2 bars, 10 % an odd, 10 % jerky; high: 50 % 20 glints, 25 % 1–3 bars, 12.5 % 1–3 odds, 12.5 % jerky; a boss: 20 glints, a tonic and one roll (81.25 % a glint, 6.25 % each a bar, an odd, jerky) | [MH] | hs_24 |
| Pots and chests | a pot is one roll, a chest ten, on the environment table: in the open (Kit's cave too) 22/32 1–5 glints, 4/32 a bar, 3/32 an odd, 2/32 a driplet jumps out, 1/32 jerky; in dungeons 16/26, 4/26, 3/26, 2/26, 1/26 | [MH] | hs_24 |
| Dungeons | three, each guarding a ship part, with shortcuts and hidden rooms behind cracked walls | [MH], [ST-LEN], [SRCH] | hs_10, hs_14, hs_19 |
| Per-save randomising | on one fixed map: which barrier roads are shut, which cave holds what (the dungeons among them), the gear's chest, the noodlers' places | [MH], [LIZ], [SSB], [ST-GEAR] | hs_19 |
| Per-trip changes | none to the map; foes, pots and chests come back each trip; parts, the gear and Mother Loom don't | [MH] | hs_19 |
| ElectroBomber | the Volthog, bolts and bombs, guards the ruins and the only way to Mother Loom; back every trip | [MH], [ST-SPIDER] | hs_15 |
| Unktomi | Mother Loom in her lair beside the ruins; beating her is the Beacon | [MH], [ST-SPIDER] | hs_15 |
| Nozzlo | Mawbo appears once all three parts are home; it uses every guardian's attacks and some foes'; 100 base hits; each defeat drops a tonic (+100 s) and it flees to a random screen; six times in one trip drops the Grinning Idol | [MH], [SRCH], [ST-CH], [ST-NOZ] | hs_16 |
| Escape | three parts and Starfuel: the Tumbleweed launches | [MH], [SSB] | hs_17 |
| New Game Plus | each escape carves a mark; three stones (HASTE, HIDE, HUNGER) up to 5 each: bars 5 % and the rest 10 % faster, 2 s less per hit, 20 s more per strip; a new camp, with the barriers, caves, gear and noodlers drawn again | [MH], [ST-CH], [ST-NG] | hs_17 |
| Goals | Beacon: beat Mother Loom. Saucer: escape. Alien: beat Mawbo six times in one trip, then escape with its idol | [MH], [SRCH] | hs_15, hs_16, hs_17 |
| Saving | automatic; a trip in progress resumes where it was | [MM] | hs_18 |

### Readings we had to choose

- **The real-time service.** Production counts only while the console is
  on ([ST-TIME]: "Time does NOT advance when UFO 50 is completely closed").
  Time during a stalled frame loop (a hidden browser tab) counts; it
  arrives all at once on the next update. Camp is saved every 30 s of real
  time wherever you are, so pulling the plug loses at most half a minute.
- **Numbers no source gives:** the smokehouse smokes a strip every 2 min
  (six on the racks); Mother Loom spins 1 thread every 30 s, 4 s sooner
  for each hand not at a job ("a few seconds" each [ST-SILK]) but never
  quicker than 8 s; bars, data and
  thread are capped at 250 and bins add 250 each (so Starfuel's 1,000 needs
  three bin levels); the glint and jerky caps follow [ST-PQ]'s 2,500 → 7,500
  and 5 → 15, carried on to 12,500 and 25. The workbench (which crafts bars
  from glints) is there from the start, so the first bar can be made before
  any anvil exists.
- **Hits:** low foes cost 20 s and most others 30 s ("most enemies"
  [SSB]). "Certain adversaries … knock whole minutes off" [STATIC], so the
  heavy hitters (red clacker, treadle, the guardians, the Volthog, Mother
  Loom, and rocks, bolts, bombs, blasts and shell shots) cost 45 s and
  Mawbo's touch 60 s. Which foes count as heavy is our choice. Wick is safe
  for 1.5 s after a hit and knocked back (never out of the room).
- **Tonics:** every tonic is +100 s, a noodler's too ("2 coins = 100s"
  [ST-FB]); ordinary foes drop none [MH].
- **The driplet from a chest or from Shuffle** is an ordinary driplet that
  drops nothing when beaten ([MH]: "an enemy, which drops no loot").
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
- **Guardians' halls** shut behind Wick until the guardian falls, and
  guardians keep off the doorways; nothing is written about how the
  original's boss rooms behave.
- **A HUD dot map:** the corner shows Wick's screen on an 8 × 6 grid, the
  way the era's top-down adventures did; it reveals nothing else.
- **World size:** 48 screens; the original's size is not written anywhere.
  The fixed map is ours: our own generator lays it out from one constant,
  never from the original's map. About 55 % of the barrier places are shut
  in a save, and every save keeps every screen reachable (checked in
  `hs_19`).
- **Pace:** with the wiki's prices, rates and loot, the demo player flies
  home in the `hs_22` runs after 22 to 35 hours with the console on. It
  plays a minute at a time with five minutes away between spells, and half
  its trips fade. Players report about 12 hours on the clock and 3.5 active
  [ST-LEN]. We take the gap to be the demo player's slow play rather than
  the economy, so no rate was changed to close it.

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
- **The Wilds:** the fixed overworld (laid out by our own generator) and
  its six regions, the ruins and the lair, the three dungeons (27 rooms),
  eleven cave rooms, hopstones.
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

`tests/hs_01` … `hs_21` and `hs_24` drive the rules with button presses (a few set up
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
finish three saves from the title (the Saucer), with real time passing
between spells of play as it does while the console sits in other games.
`hs_23` has it earn the Alien the way players do: odds from Kit's chests,
tonics from a noodler, then six Mawbo fights in one trip, with Mawbo's own
tonics keeping the trip going. To stay quick, it starts from the camp a
gold run ends with (set up by cheats) rather than from the title. Its
fighting isn't good enough to earn the Alien on every save, so that test
uses one where it does; `hs_16` checks Mawbo's rules on any save. `hs_19`
checks:

- 200 saves: every screen reachable, every way into a screen open, the lair
  reached only through the ruins, four noodlers, a gear in a chest, and the
  same layout whatever the save;
- the three dungeons.

`hs_24` rolls every loot table 10,000 times against the wiki's rates.

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
- [ST-FB] Steam thread "Pilot Quest small piece of feedback (cherry
  spoilers)" (4849904828209766913): the vendors' potions are 100 s; the
  cherry prep is a Zoldnak grind. https://steamcommunity.com/app/1147860/discussions/0/4849904828209766913/
- [ST-NG] Steam thread "What sorts of things does NG+ unlock in Pilot
  Quest?" (4849903793440811481): the overworld is slightly randomised again
  on each run (cave contents, blocked paths). https://steamcommunity.com/app/1147860/discussions/0/4849903793440811481/
- [ST-SILK] Steam thread "Pilot Quest Silk" (4852155320350199492): each
  worker not assigned to a job speeds the spider's silk up by a few seconds.
  https://steamcommunity.com/app/1147860/discussions/0/4852155320350199492/
- [ST-GEAR] Steam thread "Cannot find very basic required item in Pilot
  Quest" (4849904176632802118): the gear replaces a chest somewhere in the
  overworld. https://steamcommunity.com/app/1147860/discussions/0/4849904176632802118/
- [ST-NOZ] Steam thread "Nozzlo" (4849904176635395889): after each defeat
  it moves to a random screen. https://steamcommunity.com/app/1147860/discussions/0/4849904176635395889/
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
