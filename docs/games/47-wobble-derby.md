# 47 · WOBBLE DERBY

*Internal design document. Not shown in the product.*

## Tribute to

**Quibble Race** (UFO 50 game #47, Mossmouth). The rules were researched from
text only: the community wiki (raw pages), Steam guides and threads, and
written reviews (listed under Sources; the full notes are in the research
folder, `47-quibble-race.md`). No UFO 50 images, video, sprites, music or
text were used as references.

**Content: random, with a fixed book.** Like the original, every meeting is
dealt fresh: twenty unseen races fill the form book, each race draws three
runners from those still racing, and every race is simulated. The 26
wobblers' ratings are fixed, and follow the original's table band for band
(five wobblers in each clumsiness band and each speed band, and one wild
card); their names, colours and markings are ours.

| Full scale | Quibble Race | WOBBLE DERBY |
|---|---|---|
| Racers in the book | 26 [W] | 26 |
| Clumsiness bands | 1-2, 3-4, 5-6, 7-8, 9-10 (five each) and one 1-10 [W] | the same |
| Speed bands | 35-45, 40-50, 45-55, 50-60, 55-65 (five each) and one 35-65 [W] | the same |
| Runners a race | 3 [W] | 3 |
| Races before play | 20, unseen, for the records and the first odds [W] | 20 |
| Punters | you and two rival bettors; up to 3 players hot-seat [W] | the same |
| Faces | 16, looks only, about five of them cameos from other UFO 50 games [W], [LZ] | 16: eleven new aliens and five UFO 40 cameos |
| Meeting length | a choice; gold needs 6 or more, cherry exactly 6 [ST-CHERRY] | 3, 6, 9 or 12 races |
| Things to do before a race | bet, research, the thug (5 jobs and a guard), loans, sponsoring, the trainer [W] | the same six, under our names |
| Thug prices | $50, $20, $50, $80, $120, guard $50 [W] | the same |
| Rare events | 3: meteor shower, garbage truck spill, air pollution [W] | the same 3 |
| Goals | 3 [W], [GGC] | the same 3 |
| Secret | a news line for a $1990 bet [W-META] | the same trigger, our own line |

## Mechanics checklist

| Mechanic | How WOBBLE DERBY does it | Source | Test |
|---|---|---|---|
| The aim | the biggest purse after the last race wins | [W] | wb_12 |
| Setup | pick your face, each rival's face, CPU or player for seats 2 and 3, and the number of races | [search], [W], [ST-CHERRY] | wb_01, wb_15 |
| The form book | 20 races are run before the meeting with nobody watching; their wins and runs are each wobbler's record | [W] | wb_01 |
| The field | three wobblers a race, drawn from those still racing; the screen shows the round and the three | [W], [FG] | wb_01 |
| Turns | every player takes a turn in seat order before the race; the rivals take theirs | [FG] | wb_15 |
| One pick | each punter backs one of the three; several can back the same one | [W] | wb_02 |
| Odds | X : 1 from the record (the 20 book races and every race since), never below 1 : 1; fair odds, no cut for the track | [W], [ST-HELP] | wb_02, wb_18 |
| Payout | a win returns X times the stake plus the stake; second and third pay nothing | [W] | wb_02, wb_03 |
| Bet caps | a cap each round that rises through the meeting; the last race has none | [W] | wb_04 |
| Tip booth (Infobot) | one look a race per punter (the CPUs too): for a fee, one wobbler's speed and clumsiness, and a warning of a rare event in this race | [W], [LZ] | wb_05 |
| One fixer job a race | the thug does one job for a punter each race | [search] | wb_06 |
| Pep snack (boosters, $50) | faster for this race | [W] | wb_07 |
| Peel toss (litter, $20) | litter on its lane; clumsy ones trip on it most; a minder can't stop it | [W], [ST-HELP] | wb_07 |
| Nobble (cripple, $50) | slower for the rest of the meeting | [W] | wb_07 |
| Fizz pills (pills, $80) | it goes wild: erratic running, and it keeps going for the runner in the lane beside it and knocks it over | [W], [LZ] | wb_07 |
| Nightshade (poison, $120) | it dies at its first fall (a trip, litter, a near meteor or a fizzed neighbour), so it may survive a race it never falls in; the news doesn't say why | [W], [ST-HELP], [ST-MECH] | wb_08 |
| Minder (guard, $50) | stops every job on that wobbler, good or bad, but litter; a meddler pays a fine the size of the job's price, double for nightshade ($240) | [W], [LZ], [IHZ] | wb_06 |
| The lender (loan shark) | lends the round's cap (in the last, open race: the cap before); 15 % a round, compounded; one loan at a time; what's owed is taken from the final purse | [W], [ST-HELP] | wb_09 |
| Sponsoring | the stables offer three wobblers that may run in a future race (never this race's runners); up to three each; the fee rises with the wobbler's wins | [W] | wb_10, wb_19 |
| Sponsor bonus | $500 every time a sponsored wobbler wins, bet or no bet (the wiki says a bet is needed; see Not confirmed) | [ST-HELP], [ST-MECH], [IHZ], [SC] | wb_10 |
| The coach (trainer) | only sponsored wobblers; once a race each, two at most a race; speed up for good | [W] | wb_10 |
| The race | run by the machine, no input: speed, clumsiness (trips), the jobs and the weather decide it; a wobbler that crosses the line while tumbling still wins | [W] | wb_02, wb_11 |
| Meteor shower | meteors fall on the lanes; a direct hit kills, a near one knocks it over | [W] | wb_11 |
| Garbage truck spill | litter on every lane | [W] | wb_11 |
| Air pollution | smog: everyone slower and trips more | [W] | wb_11 |
| Deaths | a dead wobbler leaves the book for good (and its sponsor loses it) | [W] | wb_08, wb_11 |
| Ageing | speed slips a little with the races run | [ST-HELP] | wb_17 |
| Retirement | old-timers retire now and then | [LZ], [ST-MECH] | wb_17 |
| The news | after each race: the winner and odds, the places, falls, deaths, fines, sponsor bonuses, retirements | [W] | wb_08, wb_16 |
| Goals | Beacon: sponsor a wobbler that wins a race; Saucer: win a one-player meeting of 6 races or more; Alien: win a one-player meeting of exactly 6 races with $10,000 or more. Winning means being the richest outright: a tie earns nothing | [W], [ST-CHERRY], [GGC] | wb_10, wb_12 |
| The $1990 bet | a line of our own in the news | [W-META] | wb_16 |
| 3-player hot-seat | seats 2 and 3 can be players; a "your turn" screen hands the pad over | [W] | wb_15 |
| Saving | the meeting is kept between races; CONTINUE on the title | ours (see Readings) | wb_13 |

### Readings we had to choose

- **Money:** $1,000 each to start. Bet caps $200, $300, $500, $750 and
  $1,000 in a six-race meeting (the $1,000 fifth-race cap is what the
  $1,000 last-race loan in [ST-HELP] implies), then up by $500 a race in
  longer meetings; the last race is open. Bets go in $10 steps. These were
  checked with a simulation of 800 six-race meetings against the CPUs: a
  sensible punter (one look at the favourite, a pep snack for its pick, a
  full bet only when the price beats its judgement) finishes first in about
  six meetings in ten and reaches $10,000 about one time in ten; one that
  also sponsors and goes all in on a long shot at the end reaches $10,000
  about one time in four but wins less often. Without the tip booth or the
  fixer a punter wins about one in three, the same as the rivals.
- **Meeting lengths:** 3, 6, 9 or 12 races (the sources only say it is a
  choice and that 6 exists).
- **Odds:** (wins + 1) / (races + 3) for each runner, shared out among the
  three, then fair odds, rounded, between 1 : 1 and 20 : 1. No source
  mentions a cut for the track, so there is none.
- **The tip booth's fee** ("variable"): a tenth of the round's cap ($20 to
  $100 in six races). It shows the speed a wobbler can run today (training,
  nobbles and age included) and its clumsiness band. Looking at the same
  wobbler again that race is free.
- **Sponsor fee:** $100 plus $50 a win. One sponsor per wobbler. The
  stables deal a fresh three each race from the wobblers still racing that
  aren't running in it and have no sponsor.
- **Coach:** $50 a session, +2 speed for good.
- **The race:** each runner draws a speed in its range for the race (a
  speed of 50 covers the 272-pixel track in 12 seconds) and a clumsiness in
  its band (the wild card anywhere from 1 to 10). A trip lasts one second
  at a fifth of the pace; on average a runner trips 0.35 times per point of
  clumsiness each race.
- **Jobs:** a pep snack is +10 speed; a nobble is -6 for good. Fizz pills
  swap the pace every half second between a stop, a crawl, normal, a burst
  and a stumble backwards, and every second there is a 60 % chance the
  runner goes for the nearest runner in a neighbouring lane: it veers
  across, speeds up or slows down to reach it, knocks it over on contact
  and falls itself four times in ten, then drifts back (it gives up after
  four seconds). Nightshade follows the Steam players' reading ("outright
  kills them, but only if they fall"): the wobbler dies at its first fall of
  the race, whatever makes it fall. A peel toss drops three
  pieces of litter; each piece trips a runner with a 10 % + 9 % per point
  of clumsiness chance. Fines come out of cash, and on to the slate if the
  cash won't cover them.
- **Weather:** a rare event in about one race in eleven. Meteor showers
  bring four to six meteors aimed near the runners; a hit within 6 pixels
  kills, within 18 knocks over. A spill is three pieces of litter a lane.
  Smog is 15 % slower and twice the trips.
- **Interest** is added after each race except the last; the last race's
  debt is simply collected.
- **Ageing:** after each race a runner has an even chance of losing a point
  of speed. **Retirement:** after each race, a wobbler that didn't run and
  has 6 or more races has a 1 in 25 chance of retiring, while more than 8
  are still racing.
- **The gift:** the wiki's wording (sponsor a winner) over the Gift, Gold &
  Cherry table's "win a round".
- **The cherry** is read as "$10,000 or more" ([GGC]: "1st w/ 10000+").
- **Saving:** no source says whether the original keeps a meeting; ours
  keeps it between races so a long meeting can be finished later. Quitting
  during a race runs it to the end first, so a result can't be undone.
- **CPU punters:** three times in ten they pay for one look at the
  favourite; they back the best value they can see (with a fair bit of
  guesswork, and a hunch one time in five), sometimes use the fixer (a
  minder on a strong pick, nightshade or a nobble on the biggest threat,
  pills, a pep snack or litter), sponsor from the stables early on, and go
  all in at the end if they're behind (with a loan about one time in three).

## What is ours

- **Name:** WOBBLE DERBY (1989, Beamdown Softworks), the meeting at Crater
  Downs.
- **The wobblers:** 26 round, big-eyed hoppers with their own names
  (Pudding, Hotfoot, The Duke, Dicey...), colours and markings (spots,
  stripes, masks, tufts).
- **The punters:** Gloob, Vexa, Zorp, Madame Ooz, K-7, Nibbs, Baron Fuzz,
  Quilla, Ooba, Prof. Zenk and Luma, and five UFO 40 cameos: Mo
  (Underdelve), Tilly (Grub Shift), Pepper (Roofcat), Foxy (Wet Paint) and
  Twig (Tintail), about the share of cameos in the original's sixteen.
- **The track's regulars:** the Tip Booth robot, the Fixer in the alley, the
  Lender (an eel), the Stables' steward, the Coach (an owl) and Dot Dial,
  who reads the Wobble Wire.
- **The fixer's jobs:** the pep snack, peel toss, nobble, fizz pills,
  nightshade and minder.
- **Places and pictures:** Crater Downs under two moons, the stands, the
  track, the garbage truck, the meteors and the tote board.
- **Music:** "Crater Downs" (title), "The Tote Board" (paddock), "Post
  Time" (race), "The Big Payout" (final count), and the jingles "Wobble
  Wire", "Photo Finish", "Torn Ticket" and "Off They Go".
- **Words:** every line, the news and the $1990 message ("A BET OF $1990!
  THE WIRE TIPS ITS HAT TO WHOEVER DREW THE VERY FIRST WOBBLER.").

## Additions: none

Only what every UFO 40 cartridge has: the START pause menu, the save and the
three UFO 40 goals, which are Quibble Race's own (gift, gold, cherry). The
original's terminal codes (watch a race, always a rare event, no bet limit)
are a UFO 50 collection feature and UFO 40 has no terminal, so they are
left out.

## Controls

| Input | Action |
|---|---|
| D-pad | move the cursor; at the bet window LEFT/RIGHT $10, UP/DOWN $100 |
| A | choose, confirm, place the bet, next screen |
| B | back |
| START | pause |
| Title | A: new meeting (or continue); B: back to the library |

## Not confirmed

- Every number the wiki leaves out: starting cash, the caps, the tip fee,
  the sponsor fee, the coach, the job strengths, the race model, how rare
  the events are and what they do exactly, ageing and retirement.
- The meeting lengths on offer.
- Which gift wording is right (sponsor a winner, or win a round).
- Whether the original saves a meeting in progress.
- How the original's CPU bettors decide.
- Whether the sponsor bonus needs a bet: the wiki says it is paid "if bet
  on successfully in a winning race"; [ST-HELP], [IHZ] and [SC] say it is
  paid whether you bet or not, and [ST-MECH] that it is always $500. We
  follow the players.
- Nightshade: the wiki only says it "may" kill; that it kills on a fall is
  the Steam players' reading, with a question mark.
- One Steam player says the Quibbles' stats carry from game to game; ours
  start every meeting fresh from the book (a single source, which may only
  mean the fixed table).

## Tests

`tests/wb_01` … `wb_19`, driven by button presses, with cheats only to set
up a field, a purse, the weather or the stables' list. `wb_14_demo_meeting`
is a demo punter (the `bot` query in `wobble.c`): from the title screen, with
real presses, it starts a six-race meeting, pays the tip booth for one look
at the favourite, buys its pick a pep snack, backs the best value it can
see, sponsors from the stables in the first race, borrows and goes all in on
the last race if it's behind, and reads the news and the pay window until
the final count (it finishes first in five of eight seeds tried).
`wb_18_book` checks the book's bands, the art and the odds;
`wb_19_stables_offer` checks that this race's runners can never be
sponsored.

## Sources

- [W] UFO 50 Wiki (Miraheze), "Quibble Race" (raw page): 3 punters, one
  pick of three, odds, caps, the 20 pre-races, Infobot, every thug job and
  price, the guard and fines, loans and interest, sponsoring three that
  may run in a future race, the trainer, racing, events, deaths, the 26
  Quibbles' bands, the 16 faces, the goals. https://ufo50.miraheze.org/wiki/Quibble_Race
- [W-META] Miraheze, "Meta Messages": the $1990 bet.
  https://ufo50.miraheze.org/wiki/Meta_Messages
- [W-CHEATS] Miraheze, "Cheats": the three terminal codes (left out).
  https://ufo50.miraheze.org/wiki/Cheats
- [W-MP] Miraheze, "Multiplayer".
- [ST-HELP] Steam thread "Quibble Race help": never bet 1:1, the $1,000
  last-round loan, sponsor bonuses paid without a bet, speed declining,
  litter on clumsy ones, AI poisoning, poison killing on a fall.
  https://steamcommunity.com/app/1147860/discussions/0/4849904176633424258/
- [ST-MECH] Steam thread "Game Mechanics the game don't explain": the
  sponsor bonus is always 500; poison seems a sure death; retirement.
  https://steamcommunity.com/app/1147860/discussions/0/4699034745340532462/
- [ST-CHERRY] Steam thread "Cherry Requirements??": gold is a one-player
  win of at least 6 races, cherry exactly 6 races and $10,000.
  https://steamcommunity.com/app/1147860/discussions/0/4699034922676868797/
- [ST-GUIDE] Steam guide "Quibble Race - Quibble stat table and cherry
  unlock tips". https://steamcommunity.com/sharedfiles/filedetails/?id=3347731282
- [GGC] Steam guide "Gift, Gold & Cherry": win a round / 1st place / 1st
  with 10000+. https://steamcommunity.com/sharedfiles/filedetails/?id=3335464605
- [LZ] Lizstar's Trashcan, "UFO 50 Retrospective Part 47": one quibble's
  stats a day, pills make them go wild and attack the others, the guard
  blocks boosts too, quibbles retire, cameo faces.
  https://lizstar64.github.io/reviews/2024/10/21/UFO50-47.html
- [SC] Static Canvas, "The UFO 50 Diaries: Quibble Race": sponsor pays
  whether you bet or not, the final tally.
  https://staticcanvas.substack.com/p/the-ufo-50-diaries-quibble-race
- [IHZ] Indie Hell Zone, "UFO 50: Devilition, Mooncat, Rail Heist, Quibble
  Race": fines instead of bans, guarding drains meddlers, sponsor bonuses.
  https://indiehellzone.com/2024/10/30/ufo-50-devilition-mooncat-rail-heist-quibble-race/
- [TG] TheGamer, "The Best Thinky Games In UFO 50": one of the easiest.
- [FG] funny-games.biz, the older Flash "Quibble Race": players take their
  turns in sequence, the round counter and the three runners on screen.
- [search] search-engine summaries of the pages above (setup choices; one
  thug job a round).
