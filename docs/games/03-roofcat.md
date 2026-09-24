# 03 · ROOFCAT

*Internal design document. Not shown in the product.*

## Modelled on

**Ninpek** (UFO 50 game #3, Mossmouth). Mechanics were researched from text
sources only (the community wiki, written reviews and guide summaries). No
UFO 50 images, video, sprites, music, stage layouts or text were used as
references.

## Core mechanics we keep

- **Auto-scroller.** The screen scrolls right at a steady pace and carries the
  hero along with it, so the rooftops slide underneath. If you stand still you
  drift towards the end of whatever you're standing on.
  - Holding left holds your ground.
  - Holding right runs ahead.
  - If a wall shoves you off the left edge of the screen, you die. So does
    falling into a pit.
- **Double jump**, full air control, and down + jump to drop through one-way
  ledges.
- **Short-range thrown projectile.** Only one can be on screen at first.
  **Power-ups** drop from every third enemy you defeat until you hold two.
  Each one raises your fire rate and the number of projectiles allowed on
  screen (up to three). You lose them when you lose a life.
- **Scoring:**
  - Defeated enemies drop point tokens.
  - Food pickups are placed around the stages.
  - Some hazards and objects are worth points too.
- **Extra lives:**
  - At score milestones a 1-up float drifts up the screen. Hit it with a
    projectile to get the life.
  - Lives are shown as heads in the HUD.
- **Spirit mode on death:**
  - Losing a life turns you into an invulnerable spirit. The spirit moves
    freely but slowly and fires twin shots.
  - It ends on a timer, or earlier if you press jump. You come back to life
    right where the spirit is, with no invincibility frames, so choose the
    spot carefully.
- **Boss at the end of every stage.** The scroll stops for the fight.
- **Second loop.** After the ending you can go round again, with faster and
  nastier enemies.

## What is ours

- **Name:** ROOFCAT (1984, Beamdown Softworks).
- **Hero:** Harissa, a spicy courier cat. She throws jasmine stars.
- **Story:** the Magpie Mob snatched a birthday parcel from Harissa's mail
  bag, and she chases them across a sunny Mediterranean seaside town.
- **Stages:**
  1. Whitewash Rooftops: white plaster, blue doors, domes. Boss: the Pigeon Tank.
  2. Spice Souk: striped awnings and pottery. Boss: the Pot Tower.
  3. Harbour at Dusk: piers, boats, a lighthouse. Boss: Old Crab.
  4. Fort Walls at Night: crenellations and lanterns. Boss: the Magpie King.
- **Loop 2:** the Night Route, the same town after dark with tougher foes.
- **Enemies:**
  - pigeons (patrol)
  - gulls (sine-wave fliers)
  - alley rats (hoppers)
  - slingers (lob pebbles)
  - mirages (phase in and out)
  - bobbers (floating bombs)
  - chimney vents (spawn rats)
  - magpie divers
- **Pickups:**
  - fish tokens from enemies
  - dates, oranges, pomegranates and flatbread for points
  - one hidden lost letter per stage
  - paper-lantern 1-ups
  - catnip power-ups
- **Levels:** original hand-authored rooftop chunks, sequenced by hand per
  stage (`roofcat_levels.c`).
- **Music:**
  - one original track per stage (bright major, Hijaz-flavoured souk,
    breezy harbour, driving night)
  - a boss theme
  - jingles

## Controls

| Input | Action |
|---|---|
| D-pad left/right | hold ground / run ahead |
| A | jump, then press again in the air to double jump |
| Down + A | drop through a ledge |
| B | throw a jasmine star |
| Spirit mode: D-pad | float |
| Spirit mode: B | twin shots |
| Spirit mode: A | come back to life |
| START | pause |

## Goals

| Tier | Goal |
|---|---|
| Beacon | Score 15,000 points in a single run |
| Saucer | Deliver the parcel (beat the first loop) |
| Alien | Clear the Night Route (beat the second loop) |
