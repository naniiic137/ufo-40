/* IRONVOW - drawing glyphs. B starts a spell: the pad traces strokes, and
 * when the time runs out the glyph is read. A known glyph with the mana to
 * pay for it is cast; anything else fizzles and still costs a little. In a
 * fight, the fight waits while the glyph is drawn. LORE gives more time.
 * Every spell can be cast from the first step if its glyph is known: the
 * walls only teach them. */
#include "ivw.h"

#define FIZZLE_COST 2

int ivw_cast_window(void) { return 90 + 6 * ig.hero.stat[STAT_LORE]; }

void ivw_cast_begin(void) {
    IvwCast *c = &ig.cast;
    memset(c, 0, sizeof *c);
    c->on = 1;
    c->t_max = (int16_t)ivw_cast_window();
    ivw_sfx("ivw_cast");
}

static void resolve(void) {
    IvwCast *c = &ig.cast;
    IvwHero *h = &ig.hero;
    c->on = 0;
    c->result_t = 50;
    int sp = ivw_glyph_spell(c->strokes);
    if (sp < 0) {
        c->result = -1;
        h->mp = (int16_t)imax(0, h->mp - FIZZLE_COST);
        if (c->n) ivw_say("THE GLYPH FIZZLES.");
        ivw_sfx("ivw_fizzle");
        return;
    }
    if (h->mp < IVW_SPELL[sp].mana) {
        c->result = -2;
        h->mp = (int16_t)imax(0, h->mp - FIZZLE_COST);
        ivw_say("%s: NOT ENOUGH MANA.", IVW_SPELL[sp].name);
        ivw_sfx("ivw_fizzle");
        return;
    }
    h->mp = (int16_t)(h->mp - IVW_SPELL[sp].mana);
    c->result = (int8_t)sp;
    ivw_sfx("ivw_spell");
    if (ig.state == VS_FIGHT) {
        ivw_say("%s!", IVW_SPELL[sp].name);
        ivw_fight_spell(sp);
    } else ivw_spell_world(sp);
}

void ivw_cast_update(void) {
    IvwCast *c = &ig.cast;
    if (!c->on) return;
    static const int DIRS[4] = {BTN_UP, BTN_RIGHT, BTN_DOWN, BTN_LEFT};
    static const char CH[4] = {'U', 'R', 'D', 'L'};
    for (int d = 0; d < 4; d++)
        if (btnp(DIRS[d]) && c->n < IVW_GLYPH_MAX) {
            c->strokes[c->n++] = CH[d];
            c->strokes[c->n] = 0;
            ivw_sfx("ivw_stroke");
        }
    if (++c->t >= c->t_max) resolve();
}

/* ---- spells cast while exploring -------------------------------------------------------------- */
void ivw_spell_world(int sp) {
    IvwHero *h = &ig.hero;
    IvwWorld *w = &ig.w;
    int ax = h->x + IVW_DX[h->dir], ay = h->y + IVW_DY[h->dir];
    int lore = h->stat[STAT_LORE];
    switch (sp) {
    case SP_CHART: ivw_set_state(VS_MAP); return;
    case SP_OPEN:
        if (ivw_tile(ax, ay) == T_HIDDEN) {
            w->tile[ay][ax] = T_DOOR;
            ivw_say("THE OUTLINE BECOMES A DOOR.");
            ivw_sfx("ivw_rumble");
        } else ivw_say("THE WALL STAYS A WALL.");
        break;
    case SP_MEND: h->hp = (int16_t)imin(ivw_max_hp(h), h->hp + 15 + lore); ivw_say("MEND. YOU FEEL BETTER."); break;
    case SP_RESTORE: h->hp = (int16_t)imin(ivw_max_hp(h), h->hp + 40 + 2 * lore); ivw_say("RESTORE. YOU FEEL WHOLE."); break;
    case SP_LANCE: {
        int sx, sy, sd;
        ivw_floor_start(0, &sx, &sy, &sd);
        if (h->floor == 0 && h->x == sx && h->y == sy && w->turn == 0 && ivw_opaque(ax, ay)) {
            ig.easter = 1;
            ivw_say("THE WALL IS NOT IMPRESSED.");
            ivw_say("NOR, FRANKLY, IS THE BUCKLER.");
        } else ivw_say("YOUR THRUST STRIKES ONLY AIR.");
        break;
    }
    case SP_KINDLE:
        if (ivw_tile(ax, ay) == T_TORCH && !w->st[ay][ax]) {
            w->st[ay][ax] = 1;
            ivw_say("THE TORCH CATCHES.");
            ivw_sfx("ivw_kindle");
        } else ivw_say("SPARKS DANCE AND DIE.");
        break;
    case SP_GALE: {
        int x = h->x, y = h->y, n = 0, lx = h->x, ly = h->y;
        for (int k = 0; k < 6; k++) {
            int nx = x + IVW_DX[h->dir], ny = y + IVW_DY[h->dir];
            if (!ivw_walkable(nx, ny)) break;
            x = nx; y = ny; lx = x; ly = y;
            int f = ivw_flame_at(x, y);
            if (f >= 0) { w->flame[f].alive = 0; n++; }
        }
        /* things lying in the line blow to its far end */
        for (int i = 0; i < w->ndrop; i++) {
            IvwDrop *d = &w->drop[i];
            int dx = d->x - h->x, dy = d->y - h->y;
            int along = dx * IVW_DX[h->dir] + dy * IVW_DY[h->dir];
            bool in_line = (IVW_DX[h->dir] ? dy == 0 : dx == 0) && along > 0 &&
                           along <= iabs(lx - h->x) + iabs(ly - h->y);
            if (in_line) { d->x = (uint8_t)lx; d->y = (uint8_t)ly; }
        }
        ivw_say(n ? "THE GALE BLOWS THE FLAMES AWAY." : "A GALE HOWLS DOWN THE PASSAGE.");
        break;
    }
    case SP_VEIL: w->veil = (uint8_t)(10 + lore); ivw_say("YOU FADE FROM SIGHT."); break;
    case SP_FLOAT: w->fly = 10; ivw_say("YOUR FEET LEAVE THE GROUND."); break;
    case SP_FLARE: {
        int x = h->x, y = h->y;
        for (int k = 0; k < 5; k++) {
            x += IVW_DX[h->dir]; y += IVW_DY[h->dir];
            if (ivw_opaque(x, y)) break;
            int f = ivw_flame_at(x, y);
            if (f >= 0) { w->flame[f].burned = 1; ivw_say("THE FLAME BURNS!"); return; }
        }
        ivw_say("FIRE SPLASHES DOWN THE PASSAGE.");
        break;
    }
    default: ivw_say("%s: NOTHING HERE TO CAST IT ON.", IVW_SPELL[sp].name); break;
    }
    if (ig.state == VS_EXPLORE) ivw_pass_turn();
}
