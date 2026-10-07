/* HOOPLA - everything on screen: the pit, the fighters and their shots,
 * the hoops, the HUD (names and hoops bottom left and right, the count to
 * the next hoop bottom centre), and every menu, card and ending. Drawing
 * never touches the match's dice. */
#include "hoop.h"

static const uint8_t GRAD_GOLD[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};
static const uint8_t GRAD_TEAL[] = {C_WHITE, C_ICE, C_CYAN, C_TEAL};

/* ---- the pit ----------------------------------------------------------------- */

static void draw_pit(const Match *m) {
    const uint8_t *P = HOOP_PAL[m->pal];
    gfx_cls(P[0]);
    /* the far wall: tall glass panes, a glint on each, and the crowd's
     * lamps low down */
    for (int x = 0; x < SCREEN_W; x += 40) {
        gfx_rectb(x + 4, 18, 32, HOOP_AB - 30, P[1]);
        gfx_dither(x + 5, 19, 30, HOOP_AB - 32, P[1], 2);
        gfx_line(x + 8, 40, x + 18, 24, P[1]);
        gfx_line(x + 8, 46, x + 22, 26, P[1]);
    }
    for (int x = 12; x < SCREEN_W; x += 20) gfx_pset(x, HOOP_AB - 6 + (x / 20) % 2, P[1]);
    /* lamps along the top */
    for (int x = 24; x < SCREEN_W; x += 48) {
        gfx_rect(x, HOOP_AT - 4, 4, 3, C_YELLOW);
        gfx_dither(x - 6, HOOP_AT - 1, 16, 8, C_CREAM, 3);
    }
    /* walls, ceiling, floor */
    gfx_rect(0, 0, HOOP_AL, HOOP_AB, C_INK);
    gfx_rect(HOOP_AR, 0, SCREEN_W - HOOP_AR, HOOP_AB, C_INK);
    gfx_rect(0, 0, SCREEN_W, HOOP_AT - 4, C_INK);
    gfx_vline(HOOP_AL - 1, HOOP_AT, HOOP_AB, P[3]);
    gfx_vline(HOOP_AR, HOOP_AT, HOOP_AB, P[3]);
    gfx_hline(HOOP_AL, HOOP_AR - 1, HOOP_AT - 1, P[3]);
    gfx_rect(0, HOOP_AB, SCREEN_W, 4, P[4]);
    gfx_hline(0, SCREEN_W - 1, HOOP_AB, P[3]);
    for (int x = 4; x < SCREEN_W; x += 12) gfx_pset(x, HOOP_AB + 2, PAL_DARKER[P[4]]);
    /* the ledges */
    for (int i = 0; i < m->nledges; i++) {
        const Ledge *l = &m->ledge[i];
        gfx_rect(l->x, l->y, l->w, HOOP_PLAT_H, P[2]);
        gfx_hline(l->x, l->x + l->w - 1, l->y, P[3]);
        gfx_hline(l->x, l->x + l->w - 1, l->y + HOOP_PLAT_H - 1, PAL_DARKER[P[2]]);
        for (int x = l->x + 3; x < l->x + l->w - 2; x += 8) gfx_pset(x, l->y + 2, PAL_DARKER[P[2]]);
        bool moving = HOOP_ARENA[m->arena].ledge[i].mv != MV_NONE;
        if (moving) {
            gfx_pset(l->x + 1, l->y + 1, C_YELLOW);
            gfx_pset(l->x + l->w - 2, l->y + 1, C_YELLOW);
        }
    }
}

/* ---- fighters ------------------------------------------------------------------ */

void hoop_draw_fighter_big(int kind, int pal, int x, int y, int scale, int t) {
    uint8_t map[PAL_COUNT];
    hoop_remap(kind, pal, map);
    const Sprite *s = &hoop_spr[HSP_FIGHTER(kind, (t / 20) % 2 ? HSP_STAND : HSP_STAND)];
    for (int sy = 0; sy < s->h; sy++)
        for (int sx = 0; sx < s->w; sx++) {
            uint8_t c = s->px[sy * s->w + sx];
            if (c != TRANSPARENT) gfx_rect(x + sx * scale, y + sy * scale, scale, scale, map[c]);
        }
}

static void draw_fighter(const Match *m, int i) {
    const Fighter *f = &m->f[i];
    if (!f->on) return;
    int x = HPX(f->x) - 1, y = HPX(f->y) - 2;
    if (f->g < 0) y = HPX(f->y);
    int fr = HSP_STAND;
    if (f->act_t > 0 || hoop_melee_on(f) || f->aiming || f->held_bomb >= 0) fr = HSP_ACT;
    else if (f->ground == GND_AIR) fr = HSP_AIR;
    else if (f->vx != 0 && (f->step / 6) % 2) fr = HSP_STEP;
    if (f->kind == HF_GULP && f->charge_t > 0) fr = HSP_AIR;
    int flags = (f->face < 0 ? SPR_FLIPX : 0) | (f->g < 0 ? SPR_FLIPY : 0);
    /* blink while safe after a hit */
    if (f->inv_t > 0 && f->hurt_t == 0 && (f->inv_t / 3) % 2 == 0) return;
    uint8_t map[PAL_COUNT];
    hoop_remap(f->kind, f->pal, map);
    if (f->hurt_t > 0 && (f->hurt_t / 2) % 2) {
        spr_draw_ex(&hoop_spr[HSP_FIGHTER(f->kind, HSP_AIR)], x, y, flags, NULL, C_WHITE);
    } else if (f->kind == HF_GULP && f->charge_t >= HOOP_GLOW_T && (m->t / 3) % 2) {
        spr_draw_ex(&hoop_spr[HSP_FIGHTER(f->kind, fr)], x, y, flags, NULL, C_YELLOW);
    } else {
        if (f->glow_jump || f->swing_t > 0) spr_draw_outline(&hoop_spr[HSP_FIGHTER(f->kind, fr)], x, y, flags, C_YELLOW);
        else if (f->melee_t > 0) spr_draw_outline(&hoop_spr[HSP_FIGHTER(f->kind, fr)], x, y, flags, C_WHITE);
        spr_draw_ex(&hoop_spr[HSP_FIGHTER(f->kind, fr)], x, y, flags, map, -1);
    }
    /* the block: a shield on the facing side */
    if (f->blocking) {
        int bx = f->face > 0 ? HPX(f->x) + HOOP_FW + 1 : HPX(f->x) - 3;
        gfx_rect(bx, HPX(f->y) + 1, 2, HOOP_FH - 2, C_CYAN);
        gfx_vline(bx + (f->face > 0 ? 2 : -1), HPX(f->y) + 3, HPX(f->y) + HOOP_FH - 3, C_ICE);
    }
    /* dizzy stars */
    if (f->dizzy_t > 0) {
        int hy = f->g > 0 ? HPX(f->y) - 6 : HPX(f->y) + HOOP_FH + 1;
        for (int k = 0; k < 3; k++) {
            int a = (m->t * 3 + k * 24) % 72;
            int sx = HPX(f->x) + HOOP_FW / 2 + HOOP_COS[a] * 8 / 256 - 2;
            int sy = hy + HOOP_SIN[a] * 2 / 256 - 2;
            spr_draw(&hoop_spr[HSP_STAR], sx, sy, 0);
        }
    }
    /* the lift under Collier */
    if (f->lift) {
        int cy = HPX(f->y) + HOOP_FH;
        spr_draw(&hoop_spr[HSP_CAGE], HPX(f->x) - 2, cy, 0);
        gfx_vline(HPX(f->x) - 1, HOOP_AT, cy, C_GREY);
        gfx_vline(HPX(f->x) + HOOP_FW, HOOP_AT, cy, C_GREY);
    }
    /* Clamp's claw and line */
    if (f->claw) {
        int hx = HPX(f->x) + HOOP_FW / 2, hy = HPX(f->y) + 2;
        gfx_line(hx, hy, HPX(f->cx), HPX(f->cy), C_LIGHT);
        spr_draw(&hoop_spr[HSP_CLAW], HPX(f->cx) - 3, HPX(f->cy) - (f->claw == 2 ? 1 : 3), 0);
    }
    /* Gulp's sight */
    if (f->kind == HF_GULP && f->aiming) {
        int a = f->aim_t < 4 ? (f->face > 0 ? 0 : 36) : hoop_gulp_angle(f);
        int cx = HPX(f->x) + HOOP_FW / 2, cy = HPX(f->y) + HOOP_FH / 2 - 2;
        int rx = cx + HOOP_COS[a] * 24 / 256, ry = cy + HOOP_SIN[a] * 24 / 256;
        gfx_circb(rx, ry, 3, (m->t / 4) % 2 ? C_RED : C_WHITE);
        gfx_pset(rx, ry, C_RED);
    }
    /* Gulp's darts locked */
    if (f->kind == HF_GULP && f->lock_t > 0 && (m->t / 8) % 2)
        gfx_rect(HPX(f->x) + 2, HPX(f->y) - 5, 6, 2, C_RED);
    /* charging bar */
    if (f->kind == HF_GULP && f->charge_t > 0) {
        int w = imin(f->charge_t, HOOP_GLOW_T) * 12 / HOOP_GLOW_T;
        gfx_rect(HPX(f->x) - 1, HPX(f->y) + HOOP_FH + 2, 12, 2, C_INK);
        gfx_rect(HPX(f->x) - 1, HPX(f->y) + HOOP_FH + 2, w, 2, f->charge_t >= HOOP_GLOW_T ? C_YELLOW : C_ORANGE);
    }
    /* Astra's flame */
    if (f->kind == HF_ASTRA && f->thrust_t > 0) {
        int fx = HPX(f->x) + (f->face > 0 ? 1 : HOOP_FW - 3);
        int fy = HPX(f->y) + HOOP_FH - 2;
        gfx_rect(fx, fy, 3, 3 + (m->t % 3), C_ORANGE);
        gfx_rect(fx + 1, fy, 1, 2 + (m->t % 2) * 2, C_YELLOW);
    }
}

/* ---- shots and things -------------------------------------------------------------- */

static void draw_shots(const Match *m) {
    for (int i = 0; i < HOOP_MAX_SHOTS; i++) {
        const Shot *s = &m->shot[i];
        if (!s->alive) continue;
        int x = HPX(s->x), y = HPX(s->y);
        int fl = s->vx < 0 ? SPR_FLIPX : 0;
        switch (s->kind) {
        case SH_KNIFE:
            if (s->vy != 0) gfx_line(x - isign(s->vx) * 3, y - isign(s->vy) * 3, x + isign(s->vx) * 3, y + isign(s->vy) * 3, C_LIGHT);
            else spr_draw(&hoop_spr[HSP_KNIFE], x - 4, y - 1, fl);
            break;
        case SH_SHORTKNIFE:
            gfx_line(x - 2, y, x + 2, y + (s->vy > 0 ? 2 : 0), C_LIGHT);
            gfx_pset(x - 3 * isign(s->vx), y, C_BROWN);
            break;
        case SH_COG:
            spr_draw(&hoop_spr[(s->t / 4) % 2 ? HSP_COG : HSP_COG2], x - 4, y - 4, 0);
            break;
        case SH_ROCKET:
        case SH_DUD:
            spr_draw(&hoop_spr[HSP_ROCKET], x - 4, y - 2, fl);
            if (s->kind == SH_ROCKET) gfx_pset(x - 5 * isign(s->vx), y, (s->t / 2) % 2 ? C_YELLOW : C_RED);
            else gfx_pset(x - 5 * isign(s->vx), y, C_GREY);
            break;
        case SH_QUILL:
            gfx_line(x - s->vx / 256, y - s->vy / 256, x + s->vx / 512, y + s->vy / 512, C_WHITE);
            gfx_pset(x - s->vx / 256, y - s->vy / 256, C_INK);
            break;
        case SH_SHOE:
            spr_draw(&hoop_spr[HSP_SHOE], x - 3, y - 3, (s->t / 6) % 2 ? SPR_FLIPY : 0);
            break;
        case SH_BOMB:
            spr_draw(&hoop_spr[HSP_BOMB], x - 3, y - 3, 0);
            if (s->fuse < 40 && (s->fuse / 4) % 2) gfx_rect(x - 1, y - 1, 2, 2, C_RED);
            if (s->held) gfx_circb(x, y, 6, (m->t / 3) % 2 ? C_CYAN : C_ICE);
            break;
        case SH_RAY:
            gfx_line(x - s->vx / 128, y - s->vy / 128, x, y, C_CYAN);
            gfx_line(x - s->vx / 256, y - s->vy / 256, x, y, C_WHITE);
            break;
        case SH_DART:
            gfx_line(x - s->vx / 200, y - s->vy / 200, x + s->vx / 400, y + s->vy / 400, C_LIGHT);
            gfx_pset(x - s->vx / 200, y - s->vy / 200, C_RED);
            break;
        case SH_FASTDART:
            spr_draw(&hoop_spr[HSP_DART], x - 3, y - 1, fl);
            break;
        default:
            break;
        }
    }
    for (int i = 0; i < HOOP_MAX_MINES; i++) {
        const Mine *q = &m->mine[i];
        if (!q->alive) continue;
        bool armed = q->arm_t >= HOOP_MINE_ARM;
        spr_draw(&hoop_spr[armed && (m->t / 10) % 2 ? HSP_MINE_ARMED : HSP_MINE], HPX(q->x) - 4, HPX(q->y) - 4, 0);
    }
    for (int i = 0; i < HOOP_MAX_BLASTS; i++) {
        const Blast *b = &m->blast[i];
        if (!b->alive) continue;
        int r = b->r * imin(b->t + 2, 8) / 8;
        gfx_dither_circle(b->x, b->y, r, b->t < 8 ? C_YELLOW : C_ORANGE, 16 - b->t / 2);
        gfx_circb(b->x, b->y, r, C_RED);
    }
}

static void draw_rings(const Match *m) {
    for (int i = 0; i < HOOP_MAX_RINGS; i++) {
        const Ring *g = &m->ring[i];
        if (!g->alive) continue;
        int x = HPX(g->x) - 5, y = HPX(g->y) - 5;
        bool shine = (g->t / 8) % 4 == 0;
        if (g->state == RG_LOOSE && g->nograb_t > 0 && (g->t % 2)) continue;
        spr_draw(&hoop_spr[shine ? HSP_RING2 : HSP_RING], x, y, 0);
        if (g->state == RG_FRESH && g->burn_t > 0) {
            spr_draw(&hoop_spr[(g->t / 5) % 2 ? HSP_FIRE1 : HSP_FIRE2], x - 1, y + 4, 0);
            spr_draw(&hoop_spr[(g->t / 5) % 2 ? HSP_FIRE2 : HSP_FIRE1], x - 1, y - 2, SPR_FLIPY);
        }
    }
}

static void draw_parts(const Match *m) {
    for (int i = 0; i < HOOP_MAX_PARTS; i++) {
        const Part *p = &m->part[i];
        if (p->life > 0) gfx_pset(HPX(p->x), HPX(p->y), p->col);
    }
}

/* ---- the HUD -------------------------------------------------------------------------- */

static void hud_side(const Match *m, int i, int x, int y, bool right) {
    const Fighter *f = &m->f[i];
    const HoopFighterDef *d = &HOOP_DEF[f->kind];
    int col = f->pal ? d->alt_main : d->col_main;
    char name[24];
    snprintf(name, sizeof name, "%s", d->name);
    int w = text_width(name);
    int nx = right ? x - w : x;
    gfx_rect(nx - 2, y - 1, w + 4, 9, C_INK);
    text_draw(name, nx, y, f->on ? C_WHITE : C_GREY);
    gfx_rect(right ? nx - 6 : nx + w + 3, y + 1, 3, 5, col);
    int n = m->old_rules ? HOOP_OLD_HP : m->to_win;
    int have = m->old_rules ? f->hp : f->rings;
    for (int k = 0; k < n; k++) {
        int rx = right ? x - 9 - k * 9 : x + k * 9;
        int ry = y + 10;
        if (m->old_rules) {
            gfx_rect(rx + 1, ry + 1, 6, 5, k < have ? C_RED : C_DUSK);
        } else {
            gfx_circb(rx + 4, ry + 3, 3, k < have ? C_YELLOW : C_DUSK);
            if (k < have) gfx_pset(rx + 2, ry + 1, C_WHITE);
        }
    }
}

static void draw_hud(const Match *m) {
    gfx_rect(0, HOOP_AB + 4, SCREEN_W, SCREEN_H - HOOP_AB - 4, C_INK);
    hud_side(m, 0, 4, HOOP_AB + 5, false);
    hud_side(m, 1, SCREEN_W - 4, HOOP_AB + 5, true);
    if (m->n > 2) {
        const Fighter *f = &m->f[2];
        char b[32];
        snprintf(b, sizeof b, "%s %d", HOOP_DEF[f->kind].name, m->old_rules ? f->hp : f->rings);
        tiny_center(b, 160, HOOP_AB + 17, f->on ? C_LIGHT : C_SLATE);
    }
    if (!m->old_rules) {
        char t[16];
        snprintf(t, sizeof t, "%d", iclamp((m->spawn_t + 59) / 60, 0, 99));
        int col = m->spawn_t < 180 && (m->t / 6) % 2 ? C_YELLOW : C_WHITE;
        gfx_rect(146, HOOP_AB + 5, 28, 10, C_NIGHT);
        gfx_rectb(146, HOOP_AB + 5, 28, 10, C_DUSK);
        text_center(t, 160, HOOP_AB + 7, col);
    } else {
        tiny_center("OLD RULES", 160, HOOP_AB + 8, C_RED);
    }
}

static void draw_match(void) {
    const Match *m = &hg.m;
    int sx = 0, sy = 0;
    if (m->shake > 0) { sx = (m->shake % 2) ? 1 : -1; sy = (m->shake / 2 % 2) ? 1 : 0; }
    gfx_camera(sx, sy);
    draw_pit(m);
    draw_rings(m);
    for (int i = 0; i < m->n; i++) draw_fighter(m, i);
    draw_shots(m);
    draw_parts(m);
    gfx_camera(0, 0);
    draw_hud(m);
    if (m->ready_t > 0) {
        const char *w = m->ready_t > 30 ? "READY" : "GO!";
        ui_fancy_center(w, 160, 60, 3, GRAD_GOLD, 4, C_INK, C_MAROON);
        tiny_center(HOOP_ARENA[m->arena].name, 160, 90, C_WHITE);
    }
    if (m->winner >= 0) {
        const Fighter *f = &m->f[m->winner];
        char b[48];
        snprintf(b, sizeof b, "%s TAKES IT", HOOP_DEF[f->kind].name);
        int w = ui_fancy_width(b, 2);
        gfx_rect(160 - w / 2 - 6, 52, w + 12, 24, C_INK);
        ui_fancy_center(b, 160, 56, 2, GRAD_GOLD, 4, C_INK, C_MAROON);
    }
    if (hg.state == HS_ATTRACT) {
        if ((hg.frame_t / 30) % 2) tiny_center("DEMO - PRESS ANY BUTTON", 160, 2, C_WHITE);
    }
}

/* ---- menus -------------------------------------------------------------------------- */

static void backdrop(int t) {
    gfx_cls(C_NIGHT);
    for (int x = 0; x < SCREEN_W; x += 32)
        for (int y = 8; y < SCREEN_H; y += 28) gfx_dither(x + 3, y, 26, 22, C_DUSK, 5);
    for (int x = 24; x < SCREEN_W; x += 48) {
        int glow = ((t / 20) + x) % 3 == 0 ? C_CREAM : C_YELLOW;
        gfx_rect(x, 0, 4, 3, glow);
    }
    gfx_rect(0, 162, SCREEN_W, 18, C_INK);
}

static void menu_list(const char *const *items, int n, int sel, int y, int t, int disabled) {
    for (int i = 0; i < n; i++) {
        int yy = y + i * 12;
        bool s = i == sel;
        int col = s ? C_WHITE : C_GREY;
        if (i == disabled) col = s ? C_GREY : C_SLATE;
        text_center(items[i], 160, yy, col);
        if (s) ui_cursor(160 - text_width(items[i]) / 2 - 10, yy, t);
    }
}

static void draw_hoop_logo(int y, int t) {
    ui_fancy_center("HOOPLA", 160, y, 4, GRAD_GOLD, 4, C_INK, C_MAROON);
    /* five hoops over the logo */
    for (int k = 0; k < 5; k++) {
        int a = (t * 2 + k * 14) % 72;
        int x = 120 + k * 20 - 5, yy = y - 14 + HOOP_SIN[a] * 3 / 256;
        spr_draw(&hoop_spr[(t / 8 + k) % 4 == 0 ? HSP_RING2 : HSP_RING], x, yy, 0);
    }
}

static int popcount_champs(void) {
    int n = 0;
    for (int k = 0; k < HOOP_FIGHTERS; k++) n += (hsv.champs >> k) & 1;
    return n;
}

static void draw_title(void) {
    int t = hg.frame_t;
    backdrop(t);
    draw_hoop_logo(26, t);
    text_center("RING NIGHTS IN THE GLASS PIT", 160, 62, C_ICE);
    static const char *items[3] = {"1 PLAYER", "2 PLAYERS", "OPTIONS"};
    ui_panel(108, 76, 104, 44, C_NIGHT, C_TEAL);
    menu_list(items, 3, hg.title_sel, 82, t, plat_kind() == PLAT_VITA ? 1 : -1);
    /* the champions so far: a lit portrait for each who won a tournament */
    for (int k = 0; k < HOOP_FIGHTERS; k++) {
        int x = 72 + k * 22, y = 128;
        bool won = (hsv.champs >> k) & 1;
        if (won) hoop_draw_fighter_big(k, (hsv.champs_alt >> k) & 1, x, y, 1, t);
        else spr_draw_ex(&hoop_spr[HSP_FIGHTER(k, HSP_STAND)], x, y, 0, NULL, C_DUSK);
    }
    char b[80];
    int used = 0;
    for (int k = 0; k < HOOP_FIGHTERS; k++) used += (hsv.used >> k) & 1;
    if (hsv.least_rematches == 0xFFFF) snprintf(b, sizeof b, "FIGHTERS USED %d/8   CHAMPIONS %d/8", used, popcount_champs());
    else
        snprintf(b, sizeof b, "FIGHTERS USED %d/8   CHAMPIONS %d/8   FEWEST REMATCHES %d", used, popcount_champs(),
                 hsv.least_rematches);
    tiny_center(b, 160, 148, C_LIGHT);
    if (hg.title_sel == 1 && plat_kind() == PLAT_VITA) tiny_center("NEEDS TWO CONTROLLERS", 160, 168, C_LIGHT);
    else tiny_center("1988 BEAMDOWN SOFTWORKS", 160, 168, C_SLATE);
    if (hg.old_rules) {
        gfx_rect(250, 166, 66, 10, C_MAROON);
        tiny_center("OLD RULES", 283, 168, C_WHITE);
    }
}

static void draw_mode(void) {
    backdrop(hg.frame_t);
    draw_hoop_logo(26, hg.frame_t);
    const char *items1[2] = {"TOURNAMENT", "DRAFT BATTLE"};
    const char *items2[2] = {"EXHIBITION", "DRAFT BATTLE"};
    ui_panel(88, 76, 144, 34, C_NIGHT, C_TEAL);
    menu_list(hg.players == 1 ? items1 : items2, 2, hg.mode_sel, 83, hg.frame_t, -1);
    const char *say;
    if (hg.mode_sel == 1) say = "PICK TEAMS OF THREE BY TURNS. FIRST TEAM TO FIVE WINS TAKES IT.";
    else if (hg.players == 1) say = "BEAT ALL EIGHT, ONE AT A TIME. THE LAST ONE IS YOU. ONE REMATCH EACH.";
    else say = "ONE MATCH, THEN BACK TO THE PICK.";
    text_wrap(say, 40, 122, 240, C_LIGHT, 10);
}

static const char *const CHALLENGE_NAME[3] = {"CALM", "ROUGH", "RIOT (TWO FOES)"};

static void draw_options(void) {
    backdrop(hg.frame_t);
    ui_fancy_center("OPTIONS", 160, 10, 2, GRAD_TEAL, 4, C_INK, C_NAVY);
    ui_panel(40, 30, 240, 120, C_NIGHT, C_TEAL);
    char v[8][32];
    snprintf(v[0], 32, "%s", CHALLENGE_NAME[hsv.challenge % 3]);
    snprintf(v[1], 32, "%s", hsv.antigrav ? "TURNED" : "STEADY");
    snprintf(v[2], 32, "%d", hsv.to_win);
    snprintf(v[3], 32, "%d", hsv.start_rings);
    snprintf(v[4], 32, "%d S", hsv.every);
    snprintf(v[5], 32, "%s", hsv.burn ? "ON FIRE" : "COOL");
    v[6][0] = v[7][0] = 0;
    static const char *labels[8] = {"CHALLENGE", "MOSS UPSIDE DOWN", "HOOPS TO WIN", "HOOPS AT THE START",
                                    "A NEW HOOP EVERY", "NEW HOOPS", "STANDARD HOOPS", "BACK"};
    for (int i = 0; i < 8; i++) {
        int y = 38 + i * 13;
        bool s = i == hg.opt_sel;
        text_draw(labels[i], 60, y, s ? C_WHITE : C_GREY);
        if (v[i][0]) text_draw(v[i], 196, y, s ? C_YELLOW : C_LIGHT);
        if (s) ui_cursor(50, y, hg.frame_t);
    }
    const char *help = NULL;
    switch (hg.opt_sel) {
    case 0: help = "HOW HARD THE CPU FIGHTS. RIOT ADDS A SECOND FOE TO EVERY TOURNAMENT MATCH."; break;
    case 1: help = "TURNED: WHEN MOSS IS UPSIDE DOWN, LEFT AND RIGHT TURN OVER TOO."; break;
    case 6: help = "BACK TO 5 TO WIN, 1 AT THE START, ONE EVERY 15 S, ON FIRE. THE GOALS NEED THESE."; break;
    default: help = hoop_ring_rule_default() ? "STANDARD HOOPS: THE GOALS COUNT." : "NOT STANDARD: THE GOALS WON'T COUNT."; break;
    }
    text_wrap(help, 30, 154, 260, C_LIGHT, 9);
}

static void draw_portrait_card(int kind, int pal, int x, int y, bool sel, int t, int col) {
    gfx_rect(x, y, 34, 44, sel ? C_DUSK : C_NIGHT);
    gfx_rectb(x, y, 34, 44, col);
    hoop_draw_fighter_big(kind, pal, x + 5, y + 4, 2, t);
}

static void draw_select(void) {
    int t = hg.frame_t;
    backdrop(t);
    bool two = hg.mode == MODE_EXHIB;
    ui_fancy_center(two ? "EXHIBITION" : "TOURNAMENT", 160, 6, 2, GRAD_GOLD, 4, C_INK, C_MAROON);
    for (int k = 0; k < HOOP_FIGHTERS; k++) {
        int x = 10 + k * 38, y = 30;
        bool s0 = hg.cur[0] == k, s1 = two && hg.cur[1] == k;
        int col = s0 && s1 ? ((t / 8) % 2 ? C_RED : C_BLUE) : s0 ? C_RED : s1 ? C_BLUE : C_SLATE;
        int pal = s0 ? hg.pal[0] : s1 ? hg.pal[1] : 0;
        draw_portrait_card(k, pal, x, y, s0 || s1, t, col);
        if (s0) text_draw(hg.picked[0] ? "P1" GLYPH_CHECK : "P1", x + 1, y + 46, C_RED);
        if (s1) text_draw(hg.picked[1] ? "P2" GLYPH_CHECK : "P2", x + 17, y + 46, C_BLUE);
    }
    for (int s = 0; s < (two ? 2 : 1); s++) {
        int k = hg.cur[s];
        const HoopFighterDef *d = &HOOP_DEF[k];
        int x = two ? (s == 0 ? 10 : 166) : 60, w = two ? 144 : 200;
        int y = 90;
        ui_panel(x, y, w, 62, C_NIGHT, s == 0 ? C_RED : C_BLUE);
        hoop_draw_fighter_big(k, hg.pal[s], x + 6, y + 8, 3, t);
        text_draw(d->name, x + 46, y + 6, C_WHITE);
        tiny_draw(d->title, x + 46, y + 16, C_LIGHT);
        text_draw(GLYPH_B, x + 44, y + 26, C_YELLOW);
        tiny_draw(d->move, x + 54, y + 28, C_WHITE);
        text_draw(GLYPH_A, x + 44, y + 36, C_YELLOW);
        tiny_draw(d->weapon, x + 54, y + 38, C_WHITE);
        tiny_draw(hg.pal[s] ? "SECOND COLOURS" : "FIRST COLOURS", x + 46, y + 50, C_SLATE);
        if ((hsv.champs >> k) & 1) text_draw(GLYPH_STAR, x + w - 12, y + 5, C_YELLOW);
    }
    text_center(GLYPH_LEFT GLYPH_RIGHT " CHOOSE  " GLYPH_UP GLYPH_DOWN " COLOURS  " GLYPH_A " FIGHT  " GLYPH_UP "+" GLYPH_B " ANYONE", 160, 167,
                C_LIGHT);
}

static void draw_ladder(void) {
    int t = hg.frame_t;
    backdrop(t);
    ui_fancy_center("THE LADDER", 160, 6, 2, GRAD_GOLD, 4, C_INK, C_MAROON);
    for (int r = 0; r < HOOP_FIGHTERS; r++) {
        int k = hg.order[r];
        int x = 16 + r * 37, y = 50;
        bool now = r == hg.round, done = r < hg.round;
        int col = now ? ((t / 10) % 2 ? C_YELLOW : C_AMBER) : done ? C_JADE : C_SLATE;
        int pal = r == HOOP_FIGHTERS - 1 ? 1 - hg.me_pal : 0;
        draw_portrait_card(k, pal, x, y, now, t, col);
        if (done) text_draw(GLYPH_CHECK, x + 13, y + 46, C_LIME);
        if (hg.used_rematch[r] && !done) tiny_draw("LAST TRY", x + 1, y + 47, C_RED);
        char n[4];
        snprintf(n, sizeof n, "%d", r + 1);
        tiny_draw(n, x + 2, y - 7, C_LIGHT);
    }
    char b[64];
    const char *who = HOOP_DEF[hg.order[hg.round]].name;
    if (hg.round == HOOP_FIGHTERS - 1) snprintf(b, sizeof b, "MATCH 8 OF 8: THE MIRROR MATCH");
    else snprintf(b, sizeof b, "MATCH %d OF 8: %s", hg.round + 1, who);
    text_center(b, 160, 112, C_WHITE);
    snprintf(b, sizeof b, "REMATCHES USED %d", hg.rematches);
    tiny_center(b, 160, 126, C_LIGHT);
    if (hg.won_last && hg.round > 0) tiny_center("WON! ON TO THE NEXT.", 160, 136, C_LIME);
    text_center(GLYPH_A " TO THE PIT", 160, 167, C_LIGHT);
}

static void draw_vs(void) {
    int t = hg.frame_t;
    backdrop(t);
    int pa = hg.mode == MODE_TOURNEY ? hg.me_pal : hg.mode == MODE_EXHIB ? hg.pal[0] : 0;
    int pb = 0;
    if (hg.mode == MODE_TOURNEY && hg.vs_b == hg.me) pb = 1 - hg.me_pal;
    else if (hg.mode == MODE_EXHIB) pb = hg.vs_a == hg.vs_b && hg.pal[0] == hg.pal[1] ? 1 - hg.pal[1] : hg.pal[1];
    else if (hg.vs_a == hg.vs_b) pb = 1;
    int slide = imin(hg.state_t * 6, 60);
    hoop_draw_fighter_big(hg.vs_a, pa, 20 + slide, 40, 5, t);
    hoop_draw_fighter_big(hg.vs_b, pb, 240 - slide, 40, 5, t);
    text_center(HOOP_DEF[hg.vs_a].name, 50 + slide, 126, C_RED);
    text_center(HOOP_DEF[hg.vs_b].name, 270 - slide, 126, C_BLUE);
    ui_fancy_center("VS", 160, 70, 3, GRAD_GOLD, 4, C_INK, C_MAROON);
    if (hg.extra >= 0) {
        char b[40];
        snprintf(b, sizeof b, "AND %s TOO!", HOOP_DEF[hg.extra].name);
        text_center(b, 160, 146, C_ORANGE);
    }
    if (hg.mode == MODE_TOURNEY && hg.round == HOOP_FIGHTERS - 1) text_center("THE MIRROR MATCH", 160, 20, C_YELLOW);
}

static void draw_rematch(void) {
    backdrop(hg.frame_t);
    ui_fancy_center("KNOCKED OUT", 160, 30, 2, GRAD_TEAL, 4, C_INK, C_NAVY);
    text_center("ONE REMATCH AGAINST THIS FIGHTER.", 160, 64, C_LIGHT);
    ui_choices(160, 92, "REMATCH", "GIVE UP", hg.rematch_sel, hg.frame_t, C_WHITE, C_GREY);
}

static void draw_out(void) {
    backdrop(hg.frame_t);
    ui_fancy_center("OUT OF THE RING NIGHTS", 160, 50, 2, GRAD_TEAL, 4, C_INK, C_NAVY);
    char b[64];
    snprintf(b, sizeof b, "BEATEN TWICE BY %s.", HOOP_DEF[hg.order[hg.round]].name);
    text_center(b, 160, 84, C_LIGHT);
    snprintf(b, sizeof b, "YOU WON %d OF 8.", hg.round);
    text_center(b, 160, 98, C_LIGHT);
}

static void draw_draft(void) {
    int t = hg.frame_t;
    backdrop(t);
    ui_fancy_center("DRAFT BATTLE", 160, 4, 2, GRAD_GOLD, 4, C_INK, C_MAROON);
    for (int i = 0; i < 7; i++) {
        int x = 26 + i * 40, y = 28;
        bool sel = hg.state == HS_DRAFT && i == hg.draft_cur && hg.draft_turn < 6;
        int col = hg.taken[i] ? C_INK : sel ? ((t / 8) % 2 ? C_YELLOW : C_AMBER) : C_SLATE;
        if (hg.taken[i]) {
            gfx_rect(x, y, 34, 44, C_INK);
            spr_draw_ex(&hoop_spr[HSP_FIGHTER(hg.pool[i], HSP_STAND)], x + 11, y + 14, 0, NULL, C_DUSK);
        } else {
            draw_portrait_card(hg.pool[i], 0, x, y, sel, t, col);
        }
        if (sel) tiny_center(HOOP_DEF[hg.pool[i]].name, x + 17, y + 47, C_WHITE);
    }
    for (int s = 0; s < 2; s++) {
        int x = s == 0 ? 16 : 168;
        ui_panel(x, 84, 136, 56, C_NIGHT, s == 0 ? C_RED : C_BLUE);
        const char *who = s == 0 ? "PLAYER 1" : hg.players == 2 ? "PLAYER 2" : "CPU";
        text_draw(who, x + 6, 88, s == 0 ? C_RED : C_BLUE);
        for (int k = 0; k < hg.nteam[s]; k++) {
            hoop_draw_fighter_big(hg.team[s][k], 0, x + 8 + k * 42, 100, 2, t);
            tiny_draw(HOOP_DEF[hg.team[s][k]].name, x + 4 + k * 42, 134 - 1, C_LIGHT);
        }
    }
    if (hg.draft_turn < 6) {
        static const int SIDE[6] = {0, 1, 1, 0, 0, 1};
        char b[48];
        int s = SIDE[hg.draft_turn];
        snprintf(b, sizeof b, "PICK %d OF 6: %s", hg.draft_turn + 1,
                 s == 0 ? "PLAYER 1" : hg.players == 2 ? "PLAYER 2" : "THE CPU");
        text_center(b, 160, 148, C_WHITE);
    }
}

static void draw_series(void) {
    int t = hg.frame_t;
    backdrop(t);
    ui_fancy_center("DRAFT BATTLE", 160, 4, 2, GRAD_GOLD, 4, C_INK, C_MAROON);
    char b[48];
    snprintf(b, sizeof b, "%d  -  %d", hg.score[0], hg.score[1]);
    ui_fancy_center(b, 160, 30, 3, GRAD_TEAL, 4, C_INK, C_NAVY);
    tiny_center("FIRST TO FIVE WINS", 160, 56, C_LIGHT);
    for (int s = 0; s < 2; s++) {
        int x = s == 0 ? 16 : 168;
        ui_panel(x, 66, 136, 64, C_NIGHT, s == 0 ? C_RED : C_BLUE);
        for (int k = 0; k < 3; k++) {
            int kind = hg.team[s][k];
            bool next = hg.state == HS_SERIES && kind == hg.team[s][hg.queue[s][hg.qi[s]]];
            if (next) gfx_rectb(x + 4 + k * 44, 72, 40, 44, (t / 8) % 2 ? C_YELLOW : C_AMBER);
            hoop_draw_fighter_big(kind, 0, x + 12 + k * 44, 78, 2, t);
            tiny_draw(HOOP_DEF[kind].name, x + 8 + k * 44, 119, next ? C_WHITE : C_GREY);
        }
    }
    if (hg.state == HS_SERIES) text_center(GLYPH_A " NEXT MATCH", 160, 167, C_LIGHT);
}

static void draw_result(void) {
    draw_series();
    bool p1 = hg.score[0] >= 5;
    const char *w = p1 ? "PLAYER 1 TAKES THE DRAFT" : hg.players == 2 ? "PLAYER 2 TAKES THE DRAFT" : "THE CPU TAKES THE DRAFT";
    int wd = ui_fancy_width(w, 2);
    gfx_rect(160 - wd / 2 - 6, 136, wd + 12, 22, C_INK);
    ui_fancy_center(w, 160, 140, 2, GRAD_GOLD, 4, C_INK, C_MAROON);
}

static void draw_ending(void) {
    int t = hg.frame_t;
    gfx_cls(C_NIGHT);
    for (int i = 0; i < 40; i++) {
        int x = (i * 73 + t / 2) % SCREEN_W, y = (i * 37) % 120;
        gfx_pset(x, y, i % 3 ? C_DUSK : C_YELLOW);
    }
    ui_fancy_center("CHAMPION", 160, 6, 3, GRAD_GOLD, 4, C_INK, C_MAROON);
    hoop_draw_fighter_big(hg.end_kind, hg.end_pal, 20, 40, 5, t);
    for (int k = 0; k < 5; k++) spr_draw(&hoop_spr[HSP_RING], 24 + k * 11, 124, 0);
    text_draw(HOOP_DEF[hg.end_kind].name, 100, 42, C_WHITE);
    tiny_draw(HOOP_DEF[hg.end_kind].title, 100, 52, C_LIGHT);
    text_wrap(hoop_ending_text(hg.end_kind), 100, 64, 206, C_CREAM, 10);
    if (hg.end_note)
        text_wrap("P.S. THE POND SAYS THE SECOND COLOURS SUIT HIM. HE HAS NOT TAKEN THEM OFF SINCE.", 100, 128, 206,
                  C_PINK, 9);
    if (hg.state_t > 120) text_center(GLYPH_A " ON", 160, 168, C_LIGHT);
}

static void draw_credits(void) {
    gfx_cls(C_INK);
    static const char *lines[] = {
        "HOOPLA",
        "",
        "A RING NIGHT IN THE GLASS PIT",
        "",
        "TANSY  CLAMP  MOSS  BRISTLE",
        "PEWIT  COLLIER  ASTRA  GULP",
        "",
        "TWELVE PITS, EIGHT WAYS TO MOVE",
        "",
        "BEAMDOWN SOFTWORKS 1988",
        "",
        "A UFO 40 TRIBUTE TO",
        "HYPER CONTENDER (UFO 50 #36)",
        "",
        "THANK YOU FOR PLAYING",
    };
    int y = 180 - hg.state_t / 3;
    for (int i = 0; i < ARRAY_LEN(lines); i++) {
        int yy = y + i * 14;
        if (yy < -10 || yy > 180) continue;
        text_center(lines[i], 160, yy, i == 0 ? C_YELLOW : C_LIGHT);
    }
}

static void draw_note(void) {
    backdrop(hg.frame_t);
    ui_panel(30, 40, 260, 90, C_NIGHT, C_PINK);
    text_wrap("A NOTE PINNED TO THE PIT GATE: \"TWENTY DEMOS AND NOBODY PRESSED A BUTTON? THE LAMPS ARE ON FOR "
              "YOU, YOU KNOW. GO ON, PICK SOMEONE.\" - THE NIGHT PORTER",
              42, 52, 236, C_CREAM, 11);
}

void hoop_draw(void) {
    gfx_camera(0, 0);
    switch (hg.state) {
    case HS_TITLE: draw_title(); break;
    case HS_MODE: draw_mode(); break;
    case HS_OPTIONS: draw_options(); break;
    case HS_SELECT: draw_select(); break;
    case HS_LADDER: draw_ladder(); break;
    case HS_VS: draw_vs(); break;
    case HS_MATCH: case HS_ATTRACT: draw_match(); break;
    case HS_REMATCH: draw_rematch(); break;
    case HS_OUT: draw_out(); break;
    case HS_DRAFT: draw_draft(); break;
    case HS_SERIES: draw_series(); break;
    case HS_RESULT: draw_result(); break;
    case HS_ENDING: draw_ending(); break;
    case HS_CREDITS: draw_credits(); break;
    case HS_NOTE: draw_note(); break;
    }
}

/* the cartridge label: the pit, two ledges, three fighters and hoops in the air */
void hoop_draw_label(int x, int y, int w, int h, int t) {
    gfx_clip(x, y, w, h);
    gfx_rect(x, y, w, h, C_NIGHT);
    for (int gx = x + 2; gx < x + w; gx += 16) gfx_dither(gx, y + 16, 13, 12, C_DUSK, 6);
    gfx_rect(x, y + h - 6, w, 6, C_SLATE);
    gfx_hline(x, x + w - 1, y + h - 6, C_JADE);
    gfx_rect(x + 8, y + 40, 40, 3, C_TEAL);
    gfx_rect(x + w - 48, y + 32, 40, 3, C_TEAL);
    uint8_t map[PAL_COUNT];
    hoop_remap(HF_TANSY, 0, map);
    spr_draw_ex(&hoop_spr[HSP_FIGHTER(HF_TANSY, HSP_ACT)], x + 20, y + 24, 0, map, -1);
    hoop_remap(HF_PEWIT, 0, map);
    int py = y + 14 + ((t / 8) % 2);
    spr_draw_ex(&hoop_spr[HSP_FIGHTER(HF_PEWIT, HSP_AIR)], x + w / 2 - 6, py, 0, map, -1);
    hoop_remap(HF_GULP, 0, map);
    spr_draw_ex(&hoop_spr[HSP_FIGHTER(HF_GULP, HSP_STAND)], x + w - 30, y + 16, SPR_FLIPX, map, -1);
    hoop_remap(HF_MOSS, 0, map);
    spr_draw_ex(&hoop_spr[HSP_FIGHTER(HF_MOSS, HSP_STAND)], x + w / 2 + 14, y + h - 22, SPR_FLIPX, map, -1);
    for (int k = 0; k < 3; k++) {
        int a = (t * 3 + k * 24) % 72;
        int hx = x + 36 + k * 30, hy = y + 44 + HOOP_SIN[a] * 6 / 256;
        spr_draw(&hoop_spr[(t / 8 + k) % 3 ? HSP_RING : HSP_RING2], hx, hy, 0);
    }
    ui_fancy_text("HOOPLA", x + 4, y + 3, 1, GRAD_GOLD, 4, C_INK, C_MAROON);
    gfx_noclip();
}
