/* IRONVOW - drawing: the side panels (the knight's numbers on the left, the
 * map and the clock on the right), the view, the fight over the knight's
 * shoulder, every screen the menu opens, the title, the endings. */
#include "ivw_art.h"

#define VX 64
#define VY 4
#define VW 192
#define VH 128
#define MSG_Y 135

extern const char *const IVW_IA_NAME[8];

static const char *const STORY[] = {
    "FOR THREE WINTERS THE PIT REGENT HAS",
    "RULED FROM BLACKSPIRE KEEP, AND THE",
    "LAND HAS STARVED. YOU SWORE AN IRON",
    "VOW TO END HIM.",
    "",
    "HIS GAOLERS TOOK YOU AT THE GATE. THEY",
    "TOOK YOUR SWORD, YOUR MAIL, YOUR NAME,",
    "AND THREW YOU DOWN INTO THE DARK.",
    "",
    "THEY FORGOT ONLY THE VOW.",
};
static const char *const END_DAWN[] = {
    "THE DAWNBRAND BLAZES AS THE REGENT FALLS.",
    "THE KEEP CRACKS FROM VAULT TO SPIRE.",
    "",
    "YOU CLIMB OUT INTO A MORNING THE LAND",
    "HAS NOT SEEN IN THREE WINTERS.",
    "",
    "THE AMBER KNIGHT WAITS AT THE GATE,",
    "AND BOWS.",
    "",
    "THE VOW IS KEPT.",
};
static const char *const END_PLAIN[] = {
    "THE PIT REGENT FALLS, AND HIS KEEP",
    "COMES DOWN AROUND HIM.",
    "",
    "YOU CLAW YOUR WAY UP THROUGH THE",
    "RUBBLE INTO A GREY MORNING.",
    "",
    "BUT DEEP IN THE VAULT, SOMETHING",
    "STILL BREATHES IN THE DARK...",
    "",
    "THE VOW IS KEPT. FOR NOW.",
};

/* tiny words with the main font's button glyphs mixed in */
static int mixed_width(const char *s) {
    int w = 0;
    char one[2] = {0, 0};
    for (const char *p = s; *p; p++) {
        one[0] = *p;
        w += ((unsigned char)*p >= 0x80) ? text_width(one) + 1 : tiny_width(one);
    }
    return w;
}

static void mixed_draw(const char *s, int x, int y, int col) {
    char one[2] = {0, 0};
    for (const char *p = s; *p; p++) {
        one[0] = *p;
        if ((unsigned char)*p >= 0x80) { text_draw(one, x, y - 1, col); x += text_width(one) + 1; }
        else { tiny_draw(one, x, y, col); x += tiny_width(one); }
    }
}

static void mixed_center(const char *s, int cx, int y, int col) { mixed_draw(s, cx - mixed_width(s) / 2, y, col); }

/* ---- small pieces --------------------------------------------------------------------------------- */
static void bar(int x, int y, int w, int v, int max, int col, int back) {
    gfx_rect(x, y, w, 4, back);
    if (max > 0) gfx_rect(x, y, iclamp(v * w / max, 0, w), 4, col);
    gfx_rectb(x - 1, y - 1, w + 2, 6, C_INK);
}

static void box(int x, int y, int w, int h) { ui_panel(x, y, w, h, C_NIGHT, C_GREY); }

static void item_icon(int item, int x, int y) {
    const IvwItem *it = &IVW_ITEM[item];
    switch (it->kind) {
    case IK_WEAPON: gfx_line(x + 1, y + 6, x + 6, y + 1, C_LIGHT); gfx_rect(x + 1, y + 5, 2, 2, C_AMBER); break;
    case IK_ARMOUR: gfx_rect(x + 1, y + 1, 6, 6, item == IT_RAGS ? C_TAN : C_GREY); gfx_rect(x + 3, y + 1, 2, 2, C_INK); break;
    case IK_SHIELD: gfx_rect(x + 1, y + 1, 6, 5, C_EARTH); gfx_rect(x + 2, y + 6, 4, 1, C_EARTH); gfx_rectb(x + 1, y + 1, 6, 5, C_TAN); break;
    case IK_POTION: {
        static const uint8_t COL[6] = {C_RED, C_DUSK, C_SKY, C_LEAF, C_YELLOW, C_GREY};
        gfx_rect(x + 3, y, 2, 2, C_LIGHT);
        gfx_rect(x + 2, y + 2, 4, 5, COL[iclamp(item - IT_RED, 0, 5)]);
        break;
    }
    case IK_PART: gfx_circ(x + 4, y + 4, 3, item == IT_HEART ? C_RED : item == IT_BALL ? C_PINK : C_CREAM); break;
    case IK_KEY:
        if (item == IT_IRONKEY) { gfx_rect(x + 1, y + 3, 6, 2, C_GREY); gfx_rect(x + 1, y + 2, 2, 4, C_GREY); }
        else gfx_circ(x + 4, y + 4, 3, item == IT_CINDER ? C_RED : item == IT_MOSS ? C_LEAF : C_CYAN);
        break;
    default:
        if (item == IT_COIN) gfx_circ(x + 4, y + 4, 3, C_AMBER);
        else gfx_circ(x + 4, y + 4, 3, item == IT_BEETLE ? C_TEAL : C_CREAM);
        break;
    }
}

/* ---- the side panels -------------------------------------------------------------------------------- */
static void left_panel(void) {
    IvwHero *h = &ig.hero;
    gfx_rect(0, 0, VX, SCREEN_H, C_INK);
    char buf[32];
    tiny_draw("MOTES", 4, 4, C_AMBER);
    snprintf(buf, sizeof buf, "%d", (int)h->essence);
    text_draw(buf, 4, 11, C_YELLOW);
    static const char *const SN[3] = {"MIGHT", "GRIT", "LORE"};
    static const uint8_t SC[3] = {C_RED, C_LEAF, C_SKY};
    for (int s = 0; s < 3; s++) {
        tiny_draw(SN[s], 4, 26 + s * 10, SC[s]);
        snprintf(buf, sizeof buf, "%d", h->stat[s]);
        text_draw(buf, 44, 24 + s * 10, C_WHITE);
    }
    bool beetle = ig.state == VS_FIGHT && ig.f.beetle && !ig.f.beetle_out;
    tiny_draw(beetle ? "BEETLE" : "HEALTH", 4, 62, C_LIGHT);
    snprintf(buf, sizeof buf, "%d", beetle ? h->beetle_hp : h->hp);
    tiny_draw(buf, 60 - tiny_width(buf), 62, C_WHITE);
    bar(4, 69, 56, beetle ? h->beetle_hp : h->hp, beetle ? 50 : ivw_max_hp(h), h->poison ? C_LEAF : C_RED, C_MAROON);
    if (ig.state == VS_FIGHT && ig.f.recover_hp > 0) {
        int w = 56 * iclamp(h->hp + ig.f.recover_hp, 0, ivw_max_hp(h)) / ivw_max_hp(h);
        int w0 = 56 * imax(0, h->hp) / ivw_max_hp(h);
        gfx_rect(4 + w0, 69, imax(0, w - w0), 4, C_GREY);
    }
    tiny_draw("STAMINA", 4, 78, C_LIGHT);
    bar(4, 85, 56, h->st, ivw_max_st(h), ig.f.exhausted && ig.state == VS_FIGHT ? C_GREY : C_YELLOW, C_EARTH);
    tiny_draw("MANA", 4, 94, C_LIGHT);
    snprintf(buf, sizeof buf, "%d", h->mp);
    tiny_draw(buf, 60 - tiny_width(buf), 94, C_WHITE);
    bar(4, 101, 56, h->mp, ivw_max_mp(h), C_SKY, C_NAVY);
    snprintf(buf, sizeof buf, "SHARDS %d", h->shards);
    tiny_draw(buf, 4, 112, C_CYAN);
    snprintf(buf, sizeof buf, "KEYS %d", h->keys);
    tiny_draw(buf, 4, 120, C_GREY);
    int y = 130;
    if (h->poison) { tiny_draw("POISONED", 4, y, C_LIME); y += 8; }
    if (h->burn) { tiny_draw("BURNING", 4, y, C_ORANGE); y += 8; }
    if (ig.w.veil && ig.state != VS_FIGHT) { tiny_draw("VEILED", 4, y, C_VIOLET); y += 8; }
    if (ig.w.fly && ig.state != VS_FIGHT) { tiny_draw("FLOATING", 4, y, C_ICE); y += 8; }
    snprintf(buf, sizeof buf, "FLOOR %d", h->floor);
    tiny_draw(buf, 4, 170, C_SLATE);
}

static void minimap(int x0, int y0) {
    IvwHero *h = &ig.hero;
    gfx_rect(x0, y0, 54, 54, C_NIGHT);
    for (int dy = -4; dy <= 4; dy++)
        for (int dx = -4; dx <= 4; dx++) {
            int x = h->x + dx, y = h->y + dy;
            int px = x0 + (dx + 4) * 6, py = y0 + (dy + 4) * 6;
            if (x < 0 || y < 0 || x >= IVW_W || y >= IVW_H || !ig.w.seen[y][x]) continue;
            int t = ivw_tile(x, y);
            int c = C_DUSK;
            if (t == T_WALL || IVW_IS_DECOR(t) || t == T_HIDDEN || (t == T_SHIFT_A && !ig.w.shift) || (t == T_SHIFT_B && ig.w.shift) ||
                (t == T_EXIT && !ig.w.escape))
                c = C_SLATE;
            else if (t == T_DOOR || t == T_LOCKED) c = C_BROWN;
            else if (t == T_GATE) c = C_GREY;
            else if (t == T_PIT) c = C_INK;
            else if (t == T_LAVA) c = C_RED;
            else if (t == T_STAIRS || t == T_EXIT) c = C_YELLOW;
            else if ((t == T_SLIME_A && !ig.w.slime_b) || (t == T_SLIME_B && ig.w.slime_b)) c = C_LEAF;
            gfx_rect(px, py, 6, 6, c);
            if (ivw_flame_at(x, y) >= 0) gfx_rect(px + 2, py + 2, 2, 2, C_MAGENTA);
        }
    /* the knight: an arrow */
    int cx = x0 + 24 + 3, cy = y0 + 24 + 3;
    gfx_rect(cx - 1, cy - 1, 2, 2, C_WHITE);
    gfx_pset(cx + IVW_DX[h->dir] - (IVW_DX[h->dir] < 0), cy + IVW_DY[h->dir] - (IVW_DY[h->dir] < 0), C_YELLOW);
    gfx_pset(cx + 2 * IVW_DX[h->dir] - (IVW_DX[h->dir] < 0), cy + 2 * IVW_DY[h->dir] - (IVW_DY[h->dir] < 0), C_YELLOW);
    gfx_rectb(x0 - 1, y0 - 1, 56, 56, C_GREY);
}

static void clock_icon(int x, int y) {
    gfx_circ(x, y, 7, C_CREAM);
    gfx_circb(x, y, 7, C_TAN);
    float a = (ig.w.turn * 0.5f) + (ig.clock_t > 0 ? (24 - ig.clock_t) * 0.02f : 0);
    gfx_line(x, y, x + (int)(5 * sinf(a)), y - (int)(5 * cosf(a)), C_INK);
    gfx_line(x, y, x + (int)(3 * sinf(a / 12)), y - (int)(3 * cosf(a / 12)), C_WINE);
}

static void right_panel(void) {
    int x0 = VX + VW;
    gfx_rect(x0, 0, SCREEN_W - x0, SCREEN_H, C_INK);
    if (ig.state == VS_FIGHT || ig.state == VS_LOOT) {
        IvwFight *f = &ig.f;
        int y = 6;
        for (int i = 0; i < f->nfoe && y < 168; i++) {
            const IvwFoe *e = &f->foe[i];
            if (!e->alive) continue;
            tiny_draw(IVW_FOE[e->kind].name, x0 + 4, y, e->row ? C_GREY : C_WHITE);
            bar(x0 + 4, y + 7, 54, e->hp, e->hpmax, e->kind == FK_STONE ? C_VIOLET : C_RED, C_MAROON);
            y += 16;
        }
        return;
    }
    minimap(x0 + 5, 6);
    clock_icon(x0 + 32, 76);
    const IvwFloorDef *fd = &IVW_FLOOR[ig.hero.floor];
    tiny_center(IVW_BIOME_NAME[fd->biome], x0 + 32, 92, C_GREY);
    char buf[24];
    snprintf(buf, sizeof buf, "TURN %d", (int)ig.w.turn);
    tiny_center(buf, x0 + 32, 102, C_SLATE);
    static const char *const D[4] = {"NORTH", "EAST", "SOUTH", "WEST"};
    tiny_center(D[ig.hero.dir & 3], x0 + 32, 110, C_SLATE);
    text_draw(GLYPH_A, x0 + 3, 149, C_LIGHT);
    tiny_draw("MENU", x0 + 12, 150, C_GREY);
    text_draw(GLYPH_B, x0 + 3, 159, C_LIGHT);
    tiny_draw("SPELL", x0 + 12, 160, C_GREY);
}

static void messages(void) {
    gfx_rect(VX, MSG_Y - 2, VW, SCREEN_H - MSG_Y + 2, C_INK);
    gfx_hline(VX, VX + VW - 1, MSG_Y - 2, C_DUSK);
    for (int i = 0; i < IVW_MSG_LINES; i++) {
        if (!ig.msg[i][0]) continue;
        int col = i == IVW_MSG_LINES - 1 ? C_WHITE : C_GREY;
        tiny_draw(ig.msg[i], VX + 4, MSG_Y + 2 + i * 9, col);
    }
}

/* ---- the glyph being drawn -------------------------------------------------------------------------- */
static void cast_overlay(void) {
    const IvwCast *c = &ig.cast;
    if (!c->on) return;
    int x = VX + 46, y = VY + 14, w = 100, h = 92;
    box(x, y, w, h);
    tiny_center("DRAW THE GLYPH", x + w / 2, y + 5, C_VIOLET);
    for (int gy = 0; gy < 7; gy++)
        for (int gx = 0; gx < 9; gx++) gfx_pset(x + 10 + gx * 10, y + 16 + gy * 9, C_DUSK);
    if (c->n) ivw_draw_glyph(c->strokes, c->n, x + w / 2, y + 46, 8, C_PINK, C_WHITE);
    else gfx_rect(x + w / 2 - 1, y + 45, 3, 3, C_WHITE);
    bar(x + 8, y + h - 10, w - 16, c->t_max - c->t, c->t_max, C_VIOLET, C_NIGHT);
}

/* ---- the fight --------------------------------------------------------------------------------------- */
void ivw_draw_foe(const IvwFoe *e, int x, int y, int scale, int t) {
    int k = e->kind;
    int pose = e->state == FS_ACTIVE || (e->state == FS_WINDUP && e->t > 6) || e->state == FS_CAST;
    const Sprite *s = &ivw_foe_spr[k][pose];
    uint8_t map[PAL_COUNT];
    pal_identity(map);
    bool remap = false;
    if (k == FK_STONE) {
        static const uint8_t SC[3] = {C_RED, C_LEAF, C_CYAN};
        pal_swap(map, C_WHITE, SC[e->alt % 3]);
        pal_swap(map, C_LIGHT, PAL_LIGHTER[SC[e->alt % 3]]);
        remap = true;
    }
    /* the tell: heavy blows flash yellow, others flash white */
    bool heavy = (k == FK_WARDEN && e->strikes) || (k == FK_COILER && e->alt) || (k == FK_REGENT && e->alt == 2);
    if ((e->state == FS_WINDUP || e->state == FS_CAST) && (t / 3) % 2 == 0) {
        for (int c = 1; c < PAL_COUNT; c++) map[c] = heavy ? (c % 3 ? C_YELLOW : C_AMBER) : PAL_LIGHTER[map[c]];
        remap = true;
    }
    if (e->freeze > 0) { for (int c = 1; c < PAL_COUNT; c++) map[c] = c % 2 ? C_ICE : C_CYAN; remap = true; }
    else if (e->state == FS_STUN && (t / 2) % 2) { for (int c = 1; c < PAL_COUNT; c++) map[c] = PAL_DARKER[map[c]]; remap = true; }
    /* scale is in quarters: 4 = the sprite's own size */
    int w = s->w * scale / 4, hgt = s->h * scale / 4;
    int sx = x - w / 2, sy = y - hgt;
    for (int yy = 0; yy < hgt; yy++)
        for (int xx = 0; xx < w; xx++) {
            int u = xx * 4 / scale, v = yy * 4 / scale;
            uint8_t c = s->px[v * s->w + u];
            if (c == TRANSPARENT) continue;
            if (remap) c = map[c];
            if (e->slow && ((u + v + t / 4) % 7) == 0) c = C_LIME;
            if (e->row == 1 && e->kind != FK_STONE && e->kind != FK_MATRON) c = PAL_DARKER[c] ? PAL_DARKER[c] : c;
            gfx_pset(sx + xx, sy + yy, c);
        }
    /* a guarding foe holds its guard up: a little bracket */
    const IvwFoeDef *d = &IVW_FOE[k];
    if ((d->flags & FF_GUARD) && e->state == FS_MOVE && !e->freeze && ig.f.invis <= 0) {
        gfx_rect(x - 6, y - hgt / 2, 12, 2, C_GREY);
    }
    if (e->burn) gfx_pset(x + ((t * 3) % 9) - 4, y - hgt + (t % 5), C_ORANGE);
    if (e->poison) gfx_pset(x + ((t * 5) % 9) - 4, y - hgt / 2 + (t % 7), C_LIME);
}

static int arena_x(int px16) { return VX + 8 + px16 / 16; }

static void draw_shot(const IvwShot *s, int t) {
    int x1 = arena_x(s->x), x0 = arena_x(s->x0);
    float p = s->t_max ? (float)s->t / s->t_max : 1.0f;
    int ground = VY + VH - 10;
    switch (s->kind) {
    case SH_CLOUD:
        gfx_dither_circle(x1, ground - 8, 18, C_LIME, 8 + (t / 8) % 3);
        return;
    case SH_MIST: gfx_dither_circle(x0, VY + 60, 22, C_PINK, 6); return;
    case SH_LIGHTNING:
        if (s->t < s->t_max - 4) { for (int y = VY; y < ground; y += 6) gfx_vline(x1, y, y + 2, C_YELLOW); }
        else for (int y = VY; y < ground; y += 3) gfx_rect(x1 - 2 + (y / 3) % 3, y, 3, 3, C_WHITE);
        return;
    case SH_ROCK: {
        gfx_rect(x1 - 6, ground - 2, 12, 3, C_INK);
        int ry = VY + (int)(p * (ground - VY - 8));
        gfx_circ(x1, ry, 5, C_SLATE);
        return;
    }
    case SH_MYBOLT: {
        int y = ground - 20 - (int)(p * 70);
        gfx_rect(x1 - 1, y, 3, 8, C_CREAM);
        gfx_rect(x1, y - 2, 1, 12, C_YELLOW);
        return;
    }
    default: break;
    }
    int x = x0 + (int)((x1 - x0) * p);
    int y = VY + 50 + (int)(p * (ground - VY - 64)) - (int)(sinf(p * 3.14159f) * 20);
    int c = s->kind == SH_FIRE ? C_ORANGE : s->kind == SH_ACID ? C_LIME : s->kind == SH_BALL ? C_SKY
        : s->kind == SH_SHIELD ? C_CYAN : s->kind == SH_SPIKE ? C_PINK : s->kind == SH_BOLT ? C_VIOLET : C_AMBER;
    gfx_circ(x, y, s->kind == SH_SHIELD ? 4 : 2, c);
    gfx_pset(x, y, C_WHITE);
}

static void fight_scene(void) {
    IvwFight *f = &ig.f;
    int t = ig.frame_t;
    gfx_clip(VX, VY, VW, VH);
    gfx_darken_rect(VX, VY, VW, VH, 2);
    int ground = VY + VH - 10;
    gfx_hline(VX, VX + VW - 1, VY + 70, C_DUSK);
    /* back row, then stones, then front row */
    for (int pass = 0; pass < 3; pass++)
        for (int i = 0; i < f->nfoe; i++) {
            const IvwFoe *e = &f->foe[i];
            if (!e->alive) continue;
            bool stone = e->kind == FK_STONE;
            int x = arena_x(e->x);
            if (pass == 0 && e->row == 1 && !stone) {
                bool big = e->kind == FK_MATRON;
                ivw_draw_foe(e, x, VY + 76 - (IVW_FOE[e->kind].flags & FF_FLYER ? 10 : 0) + (big ? 6 : 0), big ? 4 : 3, t);
            }
            if (pass == 1 && stone) {
                int y = e->aux > 0 ? VY + 40 + (int)(4 * sinf((t + i * 30) * 0.08f)) : ground - 20;
                ivw_draw_foe(e, x, y, 4, t);
            }
            if (pass == 2 && e->row == 0 && !stone) {
                bool big = e->kind == FK_WARDEN || e->kind == FK_REGENT || e->kind == FK_MATRON;
                int fly = ivw_foe_hovering(e) ? 16 : 0;
                ivw_draw_foe(e, x, ground - 2 - fly, big ? 4 : 6, t);
            }
        }
    for (int i = 0; i < IVW_MAX_SHOTS; i++)
        if (f->shot[i].alive) draw_shot(&f->shot[i], t);
    if (f->ball_kind) {
        int bx = arena_x(f->ball_x), by = ground - 6 - (int)(fabsf(sinf(t * 0.12f)) * 30);
        gfx_circ(bx, by, 3, f->ball_kind == IT_GRUB ? C_CREAM : f->ball_kind == IT_BALL ? C_PINK : C_LIGHT);
    }
    /* the knight */
    int kx = arena_x(f->px);
    bool beetle = f->beetle && !f->beetle_out;
    int spr = beetle ? SPR_BEETLE_BACK : f->pstate == PS_GUARD ? SPR_KNIGHT_GUARD
        : f->pstate == PS_SWING || f->pstate == PS_CHARGE ? SPR_KNIGHT_SWING : f->pstate == PS_DODGE ? SPR_KNIGHT_ROLL : SPR_KNIGHT;
    const Sprite *ks = &ivw_spr[spr];
    if (!(f->invis > 0 && (t / 2) % 2) && !(f->flash_hurt && (t / 2) % 2)) spr_draw(ks, kx - ks->w / 2, VY + VH - ks->h + 14, 0);
    if (f->twin > 0) spr_draw_ex(&ivw_spr[SPR_KNIGHT], kx - 20 + 26, VY + VH - 48 + 22, 0, NULL, C_VIOLET);
    if (f->ward > 0) gfx_circb(kx, VY + VH - 14, 22, (t / 3) % 2 ? C_CYAN : C_ICE);
    if (f->pstate == PS_CHARGE && f->pt >= 40 && (t / 2) % 2) gfx_circb(kx + 12, VY + VH - 26, 6, C_YELLOW);
    /* blocks left on the shield */
    int shd = ivw_shield_item();
    if (shd && !(IVW_ITEM[shd].c & 1) && !beetle)
        for (int b = 0; b < f->blocks_left && b < 20; b++) gfx_rect(VX + 4 + b * 4, VY + 4, 3, 3, C_TAN);
    if (f->exhausted && !beetle) tiny_center("EXHAUSTED", VX + VW / 2, VY + 6, (t / 8) % 2 ? C_GREY : C_WHITE);
    if (f->invis > 0) tiny_draw("VEIL", VX + VW - 20, VY + 4, C_VIOLET);
    gfx_noclip();
}

/* ---- the screens on top ------------------------------------------------------------------------------- */
static void list_cursor(int x, int y, int sel_y) { ui_cursor(x, sel_y, ig.frame_t); (void)y; }

static void draw_menu(void) {
    int n = ig.nmenu, w = 100, h = 12 + n * 10;
    int x = VX + VW - w - 6, y = VY + 6;
    box(x, y, w, h);
    for (int i = 0; i < n; i++) {
        int c = i == ig.menu_sel ? C_WHITE : C_GREY;
        tiny_draw(IVW_MA_NAME[ig.menu[i]], x + 14, y + 7 + i * 10, c);
    }
    list_cursor(x + 4, y, y + 6 + ig.menu_sel * 10);
}

static int pack_n(void) {
    int n = 0;
    while (n < ig.hero.inv_cap && ig.hero.inv[n]) n++;
    return n;
}

static void draw_pack(bool with_desc) {
    IvwHero *h = &ig.hero;
    int x = VX + 4, y = VY + 4, w = VW - 8, hh = VH - 8;
    box(x, y, w, hh);
    char buf[48];
    snprintf(buf, sizeof buf, "PACK %d/%d", pack_n(), h->inv_cap);
    tiny_draw(buf, x + 6, y + 5, C_AMBER);
    int n = pack_n();
    for (int i = 0; i < n; i++) {
        int it = h->inv[i];
        int ry = y + 15 + i * 8;
        item_icon(it, x + 12, ry - 1);
        bool worn = i == h->wpn || i == h->arm || i == h->shd;
        int c = i == ig.inv_sel ? C_WHITE : worn ? C_YELLOW : C_GREY;
        tiny_draw(IVW_ITEM[it].name, x + 24, ry, c);
        if (worn) tiny_draw("WORN", x + w - 30, ry, C_YELLOW);
        else if (h->ready[i]) tiny_draw("READY", x + w - 30, ry, C_LIME);
    }
    if (!n) tiny_draw("EMPTY", x + 24, y + 15, C_SLATE);
    if (n) ui_cursor(x + 3, y + 14 + ig.inv_sel * 8, ig.frame_t);
    if (with_desc && n) {
        int it = h->inv[ig.inv_sel];
        const IvwItem *d = &IVW_ITEM[it];
        char lines[3][UI_WRAP_LEN];
        int nl = ui_wrap(d->desc, w - 12, true, lines, 3);
        for (int i = 0; i < imin(nl, 3); i++) mixed_draw(lines[i], x + 6, y + hh - 26 + i * 8, C_LIGHT);
        if (d->kind == IK_WEAPON) {
            snprintf(buf, sizeof buf, "NEEDS %d MIGHT", d->req);
            tiny_draw(buf, x + w - 6 - tiny_width(buf), y + 5, h->stat[STAT_MIGHT] >= d->req ? C_LEAF : C_RED);
        }
    }
}

static void draw_item_menu(void) {
    int x = VX + VW - 74, y = VY + 30, w = 66, h = 10 + ig.nitem_menu * 10;
    box(x, y, w, h);
    for (int i = 0; i < ig.nitem_menu; i++)
        tiny_draw(IVW_IA_NAME[ig.item_menu[i]], x + 14, y + 6 + i * 10, i == ig.item_sel ? C_WHITE : C_GREY);
    ui_cursor(x + 4, y + 5 + ig.item_sel * 10, ig.frame_t);
}

static void draw_pick(void) {
    IvwHero *h = &ig.hero;
    int x = VX + 20, y = VY + 8, w = VW - 40, hh = VH - 16;
    box(x, y, w, hh);
    static const char *const T[4] = {"GIVE WHAT?", "THROW IN WHAT?", "PICK UP WHAT?", "STEAL WHAT?"};
    tiny_draw(T[ig.pick_mode & 3], x + 6, y + 5, C_AMBER);
    uint8_t items[16];
    int n = 0;
    if (ig.pick_mode == 2) n = ivw_drops_here(h->x, h->y, items, 16);
    else if (ig.pick_mode == 3) { for (int i = 0; i < 4; i++) items[i] = ig.w.shop[i]; n = 4; }
    else { n = pack_n(); for (int i = 0; i < n; i++) items[i] = h->inv[i]; }
    for (int i = 0; i < n && i < 12; i++) {
        int ry = y + 16 + i * 8;
        if (items[i]) { item_icon(items[i], x + 12, ry - 1); tiny_draw(IVW_ITEM[items[i]].name, x + 24, ry, i == ig.pick_sel ? C_WHITE : C_GREY); }
        else tiny_draw("-", x + 24, ry, C_SLATE);
    }
    if (n) ui_cursor(x + 3, y + 15 + ig.pick_sel * 8, ig.frame_t);
}

static void draw_shop(void) {
    IvwHero *h = &ig.hero;
    int x = VX + 4, y = VY + 4, w = VW - 8, hh = VH - 8;
    box(x, y, w, hh);
    tiny_draw(ig.shop_buy ? "BUY" : "SELL", x + 6, y + 5, C_AMBER);
    mixed_draw(GLYPH_LEFT GLYPH_RIGHT " BUY / SELL", x + w - 6 - mixed_width(GLYPH_LEFT GLYPH_RIGHT " BUY / SELL"), y + 5, C_SLATE);
    char buf[32];
    if (ig.shop_buy) {
        for (int i = 0; i < 4; i++) {
            int it = ig.w.shop[i], ry = y + 18 + i * 10;
            if (!it) { tiny_draw("(SOLD)", x + 24, ry, C_SLATE); continue; }
            item_icon(it, x + 12, ry - 1);
            tiny_draw(IVW_ITEM[it].name, x + 24, ry, i == ig.shop_sel ? C_WHITE : C_GREY);
            snprintf(buf, sizeof buf, "%d", imax(2, IVW_ITEM[it].value * 2));
            tiny_draw(buf, x + w - 10 - tiny_width(buf), ry, C_CYAN);
        }
        ui_cursor(x + 3, y + 17 + ig.shop_sel * 10, ig.frame_t);
    } else {
        int n = pack_n();
        for (int i = 0; i < n; i++) {
            int it = h->inv[i], ry = y + 18 + i * 8;
            item_icon(it, x + 12, ry - 1);
            tiny_draw(IVW_ITEM[it].name, x + 24, ry, i == ig.shop_sel ? C_WHITE : C_GREY);
            snprintf(buf, sizeof buf, "%d", IVW_ITEM[it].value);
            tiny_draw(buf, x + w - 10 - tiny_width(buf), ry, C_CYAN);
        }
        if (n) ui_cursor(x + 3, y + 17 + ig.shop_sel * 8, ig.frame_t);
    }
    snprintf(buf, sizeof buf, "YOUR SHARDS: %d", h->shards);
    tiny_draw(buf, x + 6, y + hh - 10, C_CYAN);
}

static void draw_throne(void) {
    IvwHero *h = &ig.hero;
    int x = VX + 30, y = VY + 16, w = VW - 60, hh = 92;
    box(x, y, w, hh);
    text_center("THE THRONE", x + w / 2, y + 6, C_AMBER);
    char buf[32];
    snprintf(buf, sizeof buf, "POINTS TO PLACE: %d", ig.throne_pts);
    tiny_center(buf, x + w / 2, y + 18, C_WHITE);
    static const char *const SN[3] = {"MIGHT", "GRIT", "LORE"};
    for (int s = 0; s < 3; s++) {
        int ry = y + 32 + s * 12;
        tiny_draw(SN[s], x + 16, ry, ig.throne_sel == s ? C_WHITE : C_GREY);
        snprintf(buf, sizeof buf, "%d", h->stat[s]);
        tiny_draw(buf, x + 70, ry, h->stat[s] > ig.stat_before[s] ? C_LIME : C_LIGHT);
    }
    tiny_draw("DONE", x + 16, y + 70, ig.throne_pts ? C_SLATE : ig.throne_sel == 3 ? C_WHITE : C_GREY);
    ui_cursor(x + 5, y + 31 + ig.throne_sel * 12 + (ig.throne_sel == 3 ? 3 : 0), ig.frame_t);
    mixed_center(GLYPH_A "/" GLYPH_RIGHT " ADD   " GLYPH_LEFT " TAKE BACK", x + w / 2, y + hh - 10, C_SLATE);
}

static void draw_gems(void) {
    int x = VX + 48, y = VY + 10, w = 96, hh = 108;
    box(x, y, w, hh);
    tiny_center("NO TWO ALIKE SIDE BY SIDE", x + w / 2, y + 5, C_AMBER);
    static const uint8_t G[3] = {C_RED, C_LEAF, C_SKY};
    for (int i = 0; i < 9; i++) {
        int gx = x + 18 + (i % 3) * 22, gy = y + 18 + (i / 3) * 22;
        gfx_rect(gx, gy, 18, 18, C_EARTH);
        gfx_circ(gx + 9, gy + 9, 6, G[ig.gem_board[i] % 3]);
        gfx_pset(gx + 7, gy + 7, C_WHITE);
        if (i == ig.gem_held) gfx_rectb(gx - 2, gy - 2, 22, 22, C_YELLOW);
        if (i == ig.gem_sel && (ig.frame_t / 8) % 2) gfx_rectb(gx - 1, gy - 1, 20, 20, C_WHITE);
    }
    mixed_center(GLYPH_A " TAKE / SWAP  " GLYPH_B " LEAVE", x + w / 2, y + hh - 10, C_SLATE);
}

static void draw_wish(void) {
    int x = VX + 26, y = VY + 30, w = VW - 52, hh = 58;
    box(x, y, w, hh);
    tiny_center(ivw_wish_foul() ? "THE FOUL WELL GURGLES..." : "THE SWEET WELL GLOWS...", x + w / 2, y + 6, C_AMBER);
    static const char *const W[3] = {"STRENGTH ANEW", "RICHES", "A CLEAN OATH"};
    for (int i = 0; i < 3; i++) tiny_draw(W[i], x + 16, y + 20 + i * 11, i == ig.wish_sel ? C_WHITE : C_GREY);
    ui_cursor(x + 5, y + 19 + ig.wish_sel * 11, ig.frame_t);
}

static void draw_glyph_view(void) {
    int x = VX + 36, y = VY + 8, w = VW - 72, hh = 112;
    box(x, y, w, hh);
    const IvwSpell *s = &IVW_SPELL[ig.glyph_view % SP_COUNT];
    gfx_rect(x + 10, y + 10, w - 20, 72, C_GREY);
    ivw_draw_glyph(s->glyph, (int)strlen(s->glyph), x + w / 2, y + 46, 10, C_INK, C_RED);
    text_center(s->name, x + w / 2, y + 86, C_WHITE);
    tiny_center("THE RED DOT: BEGIN THERE", x + w / 2, y + 100, C_SLATE);
}

static void draw_map(bool cursor) {
    IvwHero *h = &ig.hero;
    int cell = 7;
    int x0 = VX + VW / 2 - 8 * cell, y0 = VY + 4;
    gfx_rect(x0 - 2, y0 - 2, 16 * cell + 4, 16 * cell + 4, C_INK);
    int cx = cursor ? ig.tele_x : h->x, cy = cursor ? ig.tele_y : h->y;
    int ox = iclamp(cx - 8, 0, IVW_W - 16), oy = iclamp(cy - 8, 0, IVW_H - 16);
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            int tx = ox + x, ty = oy + y;
            if (!ig.w.seen[ty][tx]) continue;
            int t = ivw_tile(tx, ty);
            int c = C_DUSK;
            if (t == T_WALL || IVW_IS_DECOR(t) || t == T_HIDDEN || (t == T_EXIT && !ig.w.escape) ||
                (t == T_SHIFT_A && !ig.w.shift) || (t == T_SHIFT_B && ig.w.shift)) c = C_SLATE;
            else if (t == T_DOOR || t == T_LOCKED || t == T_DOOR_OPEN) c = C_BROWN;
            else if (t == T_STAIRS || t == T_EXIT) c = C_YELLOW;
            else if (t == T_PIT) c = C_INK;
            else if (t == T_LAVA) c = C_RED;
            gfx_rect(x0 + x * cell, y0 + y * cell, cell, cell, c);
            if (t == T_MURAL) gfx_rect(x0 + x * cell + 2, y0 + y * cell + 2, 3, 3, C_GREY);
        }
    gfx_rect(x0 + (h->x - ox) * cell + 2, y0 + (h->y - oy) * cell + 2, 3, 3, C_WHITE);
    if (cursor && (ig.frame_t / 6) % 2) gfx_rectb(x0 + (ig.tele_x - ox) * cell, y0 + (ig.tele_y - oy) * cell, cell, cell, C_LIME);
    gfx_rectb(x0 - 2, y0 - 2, 16 * cell + 4, 16 * cell + 4, C_GREY);
}

static void draw_talk(void) {
    int x = VX + 6, y = VY + VH - 50, w = VW - 12, hh = 46;
    box(x, y, w, hh);
    if (ig.talk_page < ig.ntalk) {
        char lines[4][UI_WRAP_LEN];
        int n = ui_wrap(ig.talk[ig.talk_page], w - 12, true, lines, 4);
        for (int i = 0; i < imin(n, 4); i++) tiny_draw(lines[i], x + 6, y + 6 + i * 8, C_WHITE);
    }
    if ((ig.frame_t / 10) % 2) text_draw(GLYPH_A, x + w - 11, y + hh - 10, C_GREY);
}

static void draw_loot(void) {
    IvwFight *f = &ig.f;
    int x = VX + 36, y = VY + 24, w = VW - 72, hh = 26 + imax(1, f->nloot) * 9;
    box(x, y, w, hh);
    text_center("VICTORY", x + w / 2, y + 4, C_YELLOW);
    char buf[32];
    snprintf(buf, sizeof buf, "+%d MOTES", f->essence_won);
    tiny_center(buf, x + w / 2, y + 15, C_AMBER);
    for (int i = 0; i < f->nloot; i++) {
        item_icon(f->loot[i], x + 10, y + 23 + i * 9);
        tiny_draw(IVW_ITEM[f->loot[i]].name, x + 22, y + 24 + i * 9, C_WHITE);
    }
}

static void draw_card(void) {
    gfx_rect(VX, VY, VW, VH, C_INK);
    const IvwFloorDef *fd = &IVW_FLOOR[ig.hero.floor];
    char buf[40];
    snprintf(buf, sizeof buf, "FLOOR %d", ig.hero.floor);
    static const uint8_t grad[2] = {C_AMBER, C_ORANGE};
    ui_fancy_center(buf, VX + VW / 2, VY + 36, 2, grad, 2, C_INK, C_MAROON);
    text_center(fd->name, VX + VW / 2, VY + 66, C_WHITE);
    tiny_center(IVW_BIOME_NAME[fd->biome], VX + VW / 2, VY + 80, C_GREY);
    if (fd->throne_cost || ig.hero.floor == 6) {
        snprintf(buf, sizeof buf, "THE THRONE: %d MOTES", fd->throne_cost);
        tiny_center(buf, VX + VW / 2, VY + 96, C_SLATE);
    }
    tiny_center("SAVED", VX + VW / 2, VY + 112, C_LEAF);
}

/* ---- whole screens -------------------------------------------------------------------------------- */
static void draw_title(void) {
    gfx_cls(C_INK);
    int t = ig.frame_t;
    for (int i = 0; i < 40; i++) gfx_pset((i * 71 + t / 6) % SCREEN_W, (i * 37) % 70, i % 3 ? C_DUSK : C_SLATE);
    /* the keep */
    gfx_rect(120, 80, 80, 100, C_NIGHT);
    gfx_rect(140, 50, 40, 40, C_NIGHT);
    gfx_rect(152, 30, 16, 24, C_NIGHT);
    gfx_rect(157, 22, 6, 10, C_NIGHT);
    for (int i = 0; i < 6; i++) gfx_rect(122 + i * 13, 76, 8, 6, C_NIGHT);
    if ((t / 20) % 4) gfx_rect(158, 38, 4, 6, C_AMBER);
    static const uint8_t grad[3] = {C_LIGHT, C_GREY, C_SLATE};
    ui_fancy_center("IRONVOW", SCREEN_W / 2, 30, 3, grad, 3, C_INK, C_MAROON);
    tiny_center("A KNIGHT'S DESCENT INTO BLACKSPIRE KEEP", SCREEN_W / 2, 58, C_GREY);
    int y = 112;
    const char *items[2] = {"NEW GAME", "CONTINUE"};
    for (int i = 0; i < 2; i++) {
        bool ok = i == 0 || igs.has_run;
        int col = !ok ? C_DUSK : i == ig.title_sel ? C_WHITE : C_GREY;
        text_center(items[i], SCREEN_W / 2, y + i * 12, col);
        if (ok && i == ig.title_sel) ui_cursor(SCREEN_W / 2 - text_width(items[i]) / 2 - 10, y + i * 12, t);
    }
    if (igs.has_run) {
        char buf[32];
        snprintf(buf, sizeof buf, "SAVED ON FLOOR %d", igs.hero.floor);
        tiny_center(buf, SCREEN_W / 2, y + 26, C_SLATE);
    }
    char buf[64];
    int secrets = 0;
    for (int v = igs.secrets_seen; v; v >>= 1) secrets += v & 1;
    snprintf(buf, sizeof buf, "BATTLES %u   DEATHS %u   SECRETS %d/4", (unsigned)igs.battles, (unsigned)igs.deaths, secrets);
    tiny_center(buf, SCREEN_W / 2, 164, C_AMBER);
    mixed_center(GLYPH_A " START   " GLYPH_B " LIBRARY", SCREEN_W / 2, 173, C_SLATE);
}

static void draw_lines(const char *const *lines, int n, int y0, int col) {
    for (int i = 0; i < n; i++)
        if (lines[i][0]) text_center(lines[i], SCREEN_W / 2, y0 + i * 11, col);
}

static void draw_story(void) {
    gfx_cls(C_INK);
    draw_lines(STORY, ARRAY_LEN(STORY), 24, C_LIGHT);
    if (ig.state_t > 20) mixed_center(GLYPH_A " DESCEND", SCREEN_W / 2, 168, C_SLATE);
}

static void draw_over(void) {
    gfx_cls(C_INK);
    static const uint8_t grad[2] = {C_RED, C_MAROON};
    ui_fancy_center("THE VOW IS BROKEN", SCREEN_W / 2, 60, 2, grad, 2, C_INK, C_NIGHT);
    tiny_center("BUT THE KEEP REMEMBERS WHERE YOU STOOD", SCREEN_W / 2, 92, C_GREY);
    if (ig.state_t > 60) {
        mixed_center(GLYPH_A " TRY THE FLOOR AGAIN", SCREEN_W / 2, 120, C_WHITE);
        mixed_center(GLYPH_B " TO THE TITLE", SCREEN_W / 2, 132, C_GREY);
    }
}

static void draw_ending(void) {
    gfx_cls(ig.ending_kind ? C_NAVY : C_INK);
    int t = ig.state_t;
    if (ig.ending_kind) gfx_circ(SCREEN_W / 2, 200 - imin(t / 6, 60), 40, C_AMBER);
    const char *const *L = ig.ending_kind ? END_DAWN : END_PLAIN;
    int n = ig.ending_kind ? ARRAY_LEN(END_DAWN) : ARRAY_LEN(END_PLAIN);
    for (int i = 0; i < n && i * 40 < t; i++)
        if (L[i][0]) text_center(L[i], SCREEN_W / 2, 16 + i * 12, C_WHITE);
    if (t > 600) tiny_center(ig.ending_kind ? "THE DAWN ENDING" : "THE GREY ENDING", SCREEN_W / 2, 150, C_AMBER);
    if (t > 700 && !ig.ending_kind) tiny_center("A KNIGHT WITH A BRIGHTER BLADE MIGHT END IT BETTER.", SCREEN_W / 2, 162, C_GREY);
}

void ivw_draw(void) {
    switch (ig.state) {
    case VS_TITLE: draw_title(); return;
    case VS_STORY: draw_story(); return;
    case VS_OVER: draw_over(); return;
    case VS_ENDING: draw_ending(); return;
    default: break;
    }
    gfx_cls(C_INK);
    if (ig.state == VS_CARD) draw_card();
    else ivw_view_draw(VX, VY, VW, VH);
    if (ig.state == VS_FIGHT || (ig.state == VS_LOOT)) fight_scene();
    gfx_rectb(VX - 1, VY - 1, VW + 2, VH + 2, C_DUSK);
    left_panel();
    right_panel();
    messages();
    switch (ig.state) {
    case VS_MENU: draw_menu(); break;
    case VS_INV: draw_pack(true); break;
    case VS_ITEM: draw_pack(false); draw_item_menu(); break;
    case VS_PICK: draw_pick(); break;
    case VS_SHOP: draw_shop(); break;
    case VS_THRONE: draw_throne(); break;
    case VS_GEMS: draw_gems(); break;
    case VS_WISH: draw_wish(); break;
    case VS_GLYPH: draw_glyph_view(); break;
    case VS_MAP: draw_map(false); break;
    case VS_TELEPORT: draw_map(true); break;
    case VS_TALK: draw_talk(); break;
    case VS_LOOT: draw_loot(); break;
    case VS_CONFIRM: ui_confirm_box("GO DOWN THE STAIRS?", "THERE IS NO WAY BACK UP.", "DOWN", ig.confirm_sel, C_GREY, NULL, ig.frame_t); break;
    default: break;
    }
    cast_overlay();
    if (ig.demo) {
        gfx_rect(VX + VW / 2 - 30, VY + 2, 60, 11, C_INK);
        text_center((ig.frame_t / 30) % 2 ? "DEMO" : "PRESS A BUTTON", VX + VW / 2, VY + 4, C_AMBER);
    }
}

/* ---- the cartridge label ------------------------------------------------------------------------------- */
void ivw_draw_label(int x, int y, int w, int h, int t) {
    gfx_clip(x, y, w, h);
    gfx_rect(x, y, w, h, C_INK);
    for (int i = 0; i < 4; i++) gfx_rect(x + (i * 23 + 7) % w, y + h - 18 - (i % 2) * 8, 10, 18 + (i % 2) * 8, C_NIGHT);
    if (ivw_spr[SPR_KNIGHT].px) spr_draw(&ivw_spr[SPR_KNIGHT], x + w - 44, y + h - 48, 0);
    if (ivw_spr[SPR_FLAME].px && (t / 10) % 2) spr_draw(&ivw_spr[SPR_FLAME], x + 4, y + h - 34, 0);
    static const uint8_t grad[2] = {C_LIGHT, C_GREY};
    ui_fancy_text("IRONVOW", x + 6, y + 6, 1, grad, 2, C_INK, C_MAROON);
    gfx_noclip();
}

/* tests: the words of the title, the story, the endings and the over screen
 * stay on the screen and clear of each other */
int ivw_layout_audit(void) {
    int bad = 0;
    ui_audit_begin("ironvow story", true);
    ui_audit_area("screen", 0, 0, SCREEN_W, SCREEN_H);
    for (int i = 0; i < ARRAY_LEN(STORY); i++)
        if (STORY[i][0]) ui_audit_text("story", STORY[i], SCREEN_W / 2 - text_width(STORY[i]) / 2, 24 + i * 11);
    bad += ui_audit_end();
    for (int k = 0; k < 2; k++) {
        const char *const *L = k ? END_DAWN : END_PLAIN;
        ui_audit_begin("ironvow ending", true);
        ui_audit_area("screen", 0, 0, SCREEN_W, SCREEN_H);
        for (int i = 0; i < 10; i++)
            if (L[i][0]) ui_audit_text("ending", L[i], SCREEN_W / 2 - text_width(L[i]) / 2, 16 + i * 12);
        bad += ui_audit_end();
    }
    /* the log wraps: every line it keeps fits */
    for (int i = 0; i < IVW_MSG_LINES; i++) if (tiny_width(ig.msg[i]) > IVW_LOG_W) bad++;
    char buf[64];
    snprintf(buf, sizeof buf, "BATTLES %u   DEATHS %u   SECRETS %d/4", 99999u, 99999u, 4);
    if (tiny_width(buf) > SCREEN_W) bad++;
    for (int i = 0; i < IT_COUNT; i++) if (tiny_width(IVW_ITEM[i].name) > VW - 70) bad++;
    return bad;
}
