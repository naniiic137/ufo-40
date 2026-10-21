/* ECHO UNIT - drawing: the four bats' worlds, the HUD, the jailbreak board,
 * the briefing room and every screen around a run. */
#include "eko.h"

static const uint8_t TITLE_GRAD[3] = {C_YELLOW, C_ORANGE, C_RED};
static const int OWL_COL[OWL_COUNT] = {C_BROWN, C_GREY, C_ORANGE, C_PURPLE};
static const int FACE[OWL_COUNT] = {SP_FACE_FLINT, SP_FACE_VESPER, SP_FACE_DART, SP_FACE_SABLE};

static uint32_t hash2(int x, int y) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

static void parts_draw(int ox, int oy) {
    for (int i = 0; i < EKO_MAX_PARTS; i++)
        if (eg.part[i].life > 0) gfx_rect(EKO_PX(eg.part[i].x) - ox, EKO_PX(eg.part[i].y) - oy, 2, 2, eg.part[i].col);
}

static void msg_draw(int y) {
    if (eg.msg_t <= 0) return;
    int w = text_width(eg.msg);
    gfx_rect(160 - w / 2 - 4, y - 2, w + 8, 11, C_INK);
    text_center(eg.msg, 160, y, eg.msg_t % 8 < 4 ? C_YELLOW : C_WHITE);
}

/* ---- the HUD ------------------------------------------------------------------------- */

static void hud(void) {
    gfx_rect(0, 0, SCREEN_W, 18, C_INK);
    gfx_hline(0, SCREEN_W - 1, 18, C_DUSK);
    int o = eg.cur_owl;
    text_draw(EKO_OWL_NAME[o], 4, 2, OWL_COL[o] == C_PURPLE ? C_VIOLET : OWL_COL[o] == C_BROWN ? C_TAN : OWL_COL[o]);
    int hp = eg.mode == MODE_RESCUE ? eg.rs.hp : eg.owl[o].hp;
    int mx = eg.mode == MODE_RESCUE ? EKO_RS_HP[o] : EKO_OWL_HP[o];
    for (int i = 0; i < mx; i++) gfx_rect(48 + i * 7, 3, 5, 6, i < hp ? C_RED : C_MAROON);
    char b[48];
    if (eg.mode != MODE_RESCUE) {
        snprintf(b, sizeof b, "x%d", eg.owl[o].lives);
        tiny_draw(b, 48, 11, C_WHITE);
        snprintf(b, sizeof b, "%06d", eg.score);
        text_draw(b, SCREEN_W - 4 - text_width(b), 2, C_WHITE);
    }
    switch (eg.mode) {
    case MODE_BRAWL:
        if (eg.br.p.key) { spr_draw(&eko_spr[SP_KEY], 120, 4, 0); tiny_draw("KEY", 130, 6, C_YELLOW); }
        if (eg.br.p.charge >= BR_CHARGE_T) tiny_draw("CHARGED", 150, 6, (eg.frame_t / 4) % 2 ? C_YELLOW : C_ORANGE);
        break;
    case MODE_GALLERY:
        snprintf(b, sizeof b, "COMBO %d  x%d", eg.ga.combo, 1 + eg.ga.combo / 3);
        tiny_draw(b, 120, 3, eg.ga.combo > 1 ? C_YELLOW : C_GREY);
        gfx_rect(120, 10, 60, 3, C_NIGHT);
        gfx_rect(120, 10, eg.ga.meter * 60 / GA_COMBO_T, 3, C_YELLOW);
        break;
    case MODE_BIKE: {
        tiny_draw("HEAT", 116, 6, C_GREY);
        int w = eg.bk.heat * 70 / BK_HEAT_MAX;
        gfx_rect(134, 5, 72, 7, C_NIGHT);
        int col = eg.bk.heat > BK_HEAT_MAX * 3 / 4 ? ((eg.frame_t / 3) % 2 ? C_RED : C_YELLOW) : eg.bk.heat > BK_HEAT_MAX / 2 ? C_ORANGE : C_JADE;
        if (eg.bk.hot) col = C_RED;
        gfx_rect(135, 6, w, 5, col);
        if (eg.bk.hot && (eg.frame_t / 6) % 2) tiny_draw("LET GO!", 210, 6, C_RED);
        break;
    }
    case MODE_SNEAK: {
        static const int ICON[WP_COUNT] = {SP_IT_DART, SP_IT_AMMO, SP_IT_STUN, SP_IT_GREN};
        spr_draw(&eko_spr[ICON[eg.sn.weapon]], 116, 5, 0);
        snprintf(b, sizeof b, "%s %d", EKO_WP_NAME[eg.sn.weapon], eg.sn.ammo[eg.sn.weapon]);
        tiny_draw(b, 127, 7, C_WHITE);
        if (eg.sn.cards & CARD_GREEN) gfx_rect(190, 5, 6, 8, C_LEAF);
        if (eg.sn.cards & CARD_RED) gfx_rect(198, 5, 6, 8, C_RED);
        if (eg.sn.cards & CARD_BLUE) gfx_rect(206, 5, 6, 8, C_SKY);
        if (eg.sn.alarm || eg.sn.room_spotted) tiny_draw("SEEN", 216, 7, (eg.frame_t / 6) % 2 ? C_RED : C_ORANGE);
        break;
    }
    case MODE_RESCUE: {
        snprintf(b, sizeof b, "JAILBREAK FOR %s", EKO_OWL_NAME[eg.rescue_for]);
        text_draw(b, 120, 2, C_YELLOW);
        if (eg.rs.owl == OWL_VESPER || eg.rs.owl == OWL_SABLE) {
            snprintf(b, sizeof b, "%s %d", eg.rs.owl == OWL_SABLE ? "DARTS" : "SHOTS", eg.rs.ammo);
            tiny_draw(b, 48, 11, C_WHITE);
        }
        if (eg.rs.card) tiny_draw("CARD", 120, 11, C_LEAF);
        break;
    }
    default: break;
    }
}

/* ---- FLINT ------------------------------------------------------------------------------ */

static void brawl_bg(void) {
    const EkoBrawl *b = &eg.br;
    int m = b->map;
    if (m == BM_LAB) {
        gfx_cls(C_NIGHT);
        int off = (b->cam_y / 3) % 40;
        for (int y = -40; y < SCREEN_H; y += 40) {
            gfx_hline(0, SCREEN_W - 1, y + off, C_DUSK);
            for (int x = 20; x < SCREEN_W; x += 60) {
                gfx_rect(x, y + off + 8, 14, 20, C_TEAL);
                gfx_rect(x + 3, y + off + 12 + (eg.frame_t / 8 + x) % 12, 8, 3, C_LIME);
            }
        }
        return;
    }
    if (m == BM_LIFT) {
        gfx_cls(C_DUSK);
        int off = b->lift_y % 48;
        for (int y = -48; y < SCREEN_H; y += 48) {
            gfx_rect(16, y + off, SCREEN_W - 32, 3, C_SLATE);
            gfx_rect(40, y + off + 10, 60, 24, C_NIGHT);
            gfx_rect(220, y + off + 20, 50, 20, C_NIGHT);
        }
        gfx_rect(150, 0, 3, SCREEN_H, C_GREY);
        gfx_rect(170, 0, 3, SCREEN_H, C_GREY);
        return;
    }
    /* the harbour at dusk */
    gfx_cls(m == BM_DOCKS ? C_PURPLE : C_WINE);
    gfx_dither(0, 40, SCREEN_W, 40, C_ORANGE, 5);
    gfx_circ(250, 70, 18, C_AMBER);
    gfx_rect(0, 96, SCREEN_W, 84, C_NAVY);
    gfx_dither(0, 96, SCREEN_W, 8, C_BLUE, 6);
    int off = (b->cam_x / 4) % 120;
    for (int x = -120; x < SCREEN_W + 120; x += 120) {
        int bx = x - off;
        /* cranes, or stacks of containers in the yards */
        if (m == BM_DOCKS) {
            gfx_rect(bx + 30, 30, 4, 70, C_NIGHT);
            gfx_rect(bx + 10, 30, 60, 4, C_NIGHT);
            gfx_line(bx + 32, 30, bx + 60, 36, C_NIGHT);
            gfx_vline(bx + 62, 34, 60, C_NIGHT);
        } else {
            gfx_rect(bx + 10, 60, 40, 16, C_MAROON);
            gfx_rect(bx + 20, 44, 40, 16, C_TEAL);
            gfx_rect(bx + 60, 70, 40, 26, C_NIGHT);
        }
    }
}

static int tile_col(char c) {
    switch (c) {
    case '#': return C_SLATE;
    case 'W': return C_MAROON;
    default: return -1;
    }
}

static void brawl_tiles(void) {
    const EkoBrawl *b = &eg.br;
    int tx0 = b->cam_x / 16, ty0 = b->cam_y / 16;
    for (int ty = ty0; ty <= ty0 + BR_SH + 1 && ty < b->mh; ty++)
        for (int tx = tx0; tx <= tx0 + 21 && tx < b->mw; tx++) {
            char c = b->tile[ty][tx];
            int x = tx * 16 - b->cam_x, y = BR_TOP + ty * 16 - b->cam_y;
            switch (c) {
            case '#': case 'H': case 'h': {
                bool top = ty == 0 || (b->tile[ty - 1][tx] != '#' && b->tile[ty - 1][tx] != 'H' && b->tile[ty - 1][tx] != 'h');
                gfx_rect(x, y, 16, 16, b->map == BM_LAB ? C_DUSK : C_SLATE);
                if (top) gfx_rect(x, y, 16, 3, b->map == BM_LAB ? C_TEAL : C_GREY);
                if (hash2(tx, ty) % 3 == 0) gfx_pset(x + 5, y + 8, C_NIGHT);
                if (c != '#' && b->thp[ty][tx] < 4) gfx_line(x + 3, y + 4, x + 10, y + 12, C_INK);
                break;
            }
            case '=':
                gfx_rect(x, y, 16, 4, C_EARTH);
                gfx_hline(x, x + 15, y, C_TAN);
                gfx_pset(x + 3, y + 4, C_EARTH);
                gfx_pset(x + 12, y + 4, C_EARTH);
                break;
            case 'c': case 'q':
                gfx_rect(x + 1, y + 1, 14, 15, C_TAN);
                gfx_rectb(x + 1, y + 1, 14, 15, C_BROWN);
                gfx_line(x + 1, y + 1, x + 14, y + 15, C_BROWN);
                break;
            case 'W':
                gfx_rect(x, y, 16, 16, C_MAROON);
                gfx_rectb(x, y, 16, 16, C_INK);
                if (ty % 4 == 1 && tx % 2 == 0) { gfx_rect(x + 4, y + 4, 8, 8, C_YELLOW); gfx_rect(x + 6, y + 6, 4, 4, C_INK); }
                break;
            case 'E':
                gfx_rect(x + 2, y - 16, 12, 32, C_INK);
                gfx_rectb(x + 2, y - 16, 12, 32, C_YELLOW);
                break;
            default: (void)tile_col; break;
            }
        }
}

static void brawl_draw(void) {
    const EkoBrawl *b = &eg.br;
    int ox = b->cam_x, oy = b->cam_y - BR_TOP;
    gfx_clip(0, BR_TOP - 1, SCREEN_W, SCREEN_H - BR_TOP + 1);
    brawl_bg();
    brawl_tiles();
    /* objects */
    for (int i = 0; i < BR_MAX_OBJS; i++) {
        const EkoBObj *o = &b->obj[i];
        if (!o->alive) continue;
        int x = EKO_PX(o->x) - ox, y = EKO_PX(o->y) - oy;
        switch (o->kind) {
        case BO_BARREL: spr_draw(&eko_spr[SP_BARREL], x - 6, y - 12, 0); break;
        case BO_MANGO: spr_draw(&eko_spr[SP_MANGO], x - 4, y - 9 - (eg.frame_t / 10) % 2, 0); break;
        case BO_LIFE: spr_draw(&eko_spr[SP_LIFE], x - 4, y - 9, 0); break;
        case BO_KEY: spr_draw(&eko_spr[SP_KEY], x - 4, y - 8 - (eg.frame_t / 8) % 2, 0); break;
        case BO_GRENADE: gfx_circ(x, y - 3, 3, C_FOREST); gfx_pset(x, y - 6, (eg.frame_t / 3) % 2 ? C_RED : C_YELLOW); break;
        case BO_FBULLET: gfx_rect(x - 2, y - 1, 4, 2, C_YELLOW); break;
        case BO_FLAME: gfx_circ(x, y, 3, (eg.frame_t / 2) % 2 ? C_ORANGE : C_YELLOW); break;
        case BO_KI:
            /* the ki blast: a hot ball with a short tail */
            for (int k = 1; k <= 3; k++) gfx_circ(x - isign(o->vx) * k * 5, y, 4 - k, k == 1 ? C_AMBER : C_ORANGE);
            gfx_circ(x, y, 5, (eg.frame_t / 2) % 2 ? C_YELLOW : C_WHITE);
            gfx_circ(x, y, 2, C_WHITE);
            break;
        default: break;
        }
    }
    /* foes */
    for (int i = 0; i < BR_MAX_FOES; i++) {
        const EkoBFoe *f = &b->foe[i];
        if (!f->alive) continue;
        int x = EKO_PX(f->x) - ox, y = EKO_PX(f->y) - oy;
        int fl = f->face < 0 ? SPR_FLIPX : 0;
        if (f->flash && (f->flash / 2) % 2) continue;
        uint8_t map[PAL_COUNT];
        pal_identity(map);
        switch (f->kind) {
        case BF_SCRAPJAW:
            spr_draw(&eko_spr[SP_SCRAPJAW], x - 16, y - 32, fl);
            if (f->state == 2 && f->t < 12) gfx_rect(f->face > 0 ? x + 10 : x - 38, y - 24, 28, 4, C_GREY);
            if (f->state == 3) gfx_circb(x, y - 16, 18 + (f->t % 6), C_RED);
            break;
        case BF_TALPA:
            if (f->state == 2 || (f->state == 3 && f->t < 30)) {
                gfx_rect(x - 10, y - 4, 20, 4, C_EARTH);
                for (int k = 0; k < 3; k++) gfx_pset(x - 8 + k * 8, y - 6 - (eg.frame_t + k) % 3, C_TAN);
            } else spr_draw(&eko_spr[SP_TALPA], x - 16, y - 32, fl);
            break;
        case BF_GRENADIER:
            pal_swap(map, C_NAVY, C_FOREST);
            spr_draw_ex(&eko_spr[f->state == 1 ? SP_GREN_THROW : SP_GREN], x - 8, y - 24, fl, map, -1);
            break;
        case BF_JETPACK:
            pal_swap(map, C_NAVY, C_SLATE);
            spr_draw_ex(&eko_spr[(eg.frame_t / 6) % 2 ? SP_JET1 : SP_JET2], x - 8, y - 24, fl, map, -1);
            gfx_rect(x - (fl ? -4 : 8), y - 18, 4, 9, C_GREY);
            gfx_rect(x - (fl ? -4 : 8), y - 9, 4, 3 + (eg.frame_t % 3), C_ORANGE);
            break;
        case BF_KEYMAN:
            pal_swap(map, C_NAVY, C_RED);
            spr_draw_ex(&eko_spr[SP_KEYMAN], x - 8, y - 24, fl, map, -1);
            break;
        default: {
            int sp = f->state == 1 ? SP_TROOP_PUNCH : f->state == 2 ? SP_TROOP_AIM : (f->vx && (eg.frame_t / 8) % 2) ? SP_TROOP_WALK : SP_TROOP_STAND;
            if (f->state == 99) sp = SP_TROOP_WALK;
            spr_draw(&eko_spr[sp], x - 8, y - 24, fl);
            break;
        }
        }
    }
    /* FLINT */
    const EkoBPlayer *p = &b->p;
    if (!(p->inv && (p->inv / 3) % 2)) {
        int sp = SP_FLINT_STAND;
        if (p->dead_t || p->hurt_t) sp = SP_FLINT_HURT;
        else if (p->act == BRA_KI) sp = SP_FLINT_CHARGE;
        else if (p->act == BRA_AIRKICK) sp = SP_FLINT_AIRKICK;
        else if (!p->ground) sp = SP_FLINT_JUMP;
        else if (p->act == BRA_LOW) sp = SP_FLINT_LOW;
        else if (p->duck) sp = SP_FLINT_DUCK;
        else if (p->act == BRA_KICK) sp = SP_FLINT_KICK;
        else if (p->act == BRA_PUNCH1 || p->act == BRA_PUNCH2 || p->act == BRA_THROW) sp = SP_FLINT_PUNCH;
        else if (p->carry) sp = SP_FLINT_CARRY;
        else if (p->vx) sp = (eg.frame_t / 7) % 2 ? SP_FLINT_WALK1 : SP_FLINT_WALK2;
        int x = EKO_PX(p->x) - ox, y = EKO_PX(p->y) - oy;
        if (p->charge >= BR_CHARGE_T && (eg.frame_t / 3) % 2) {
            uint8_t map[PAL_COUNT];
            pal_identity(map);
            pal_swap(map, C_BROWN, C_YELLOW);
            pal_swap(map, C_HIDE, C_AMBER);
            spr_draw_ex(&eko_spr[sp], x - 8, y - 24, p->face < 0 ? SPR_FLIPX : 0, map, -1);
        } else spr_draw(&eko_spr[sp], x - 8, y - 24, p->face < 0 ? SPR_FLIPX : 0);
    }
    parts_draw(ox, oy);
    gfx_noclip();
}

/* ---- VESPER --------------------------------------------------------------------------------- */

static void gallery_bg(void) {
    const EkoGallery *g = &eg.ga;
    bool fin = g->script == GS_FINALE;
    gfx_cls(fin ? C_MAROON : C_SKY);
    if (fin) {
        gfx_dither(0, 18, SCREEN_W, 40, C_RED, 6);
        for (int x = 0; x < SCREEN_W; x += 40) gfx_line(x, 70, x + 20, 30, C_WINE);
        gfx_rect(0, 60, SCREEN_W, 120, C_DUSK);
        gfx_rect(140, 22, 40, 50, C_SLATE);
        gfx_rect(150, 10, 20, 14, C_GREY);
    } else {
        gfx_dither(0, 18, SCREEN_W, 30, C_ICE, 4);
        /* peaks */
        for (int k = 0; k < 6; k++) {
            int bx = k * 64 - 10, h = 30 + (int)(hash2(k, 3) % 20);
            for (int y = 0; y < h; y++) gfx_hline(bx + y / 1 - h, bx + h - y, 66 - y + 4, y > h - 8 ? C_WHITE : C_SLATE);
        }
        gfx_rect(0, 64, SCREEN_W, 116, C_FOREST);
        gfx_dither(0, 64, SCREEN_W, 30, C_JADE, 6);
    }
    /* the three rows' ground */
    for (int r = 0; r < GA_ROWS; r++) {
        int y = GA_ROW_Y[r];
        gfx_hline(0, SCREEN_W - 1, y, fin ? C_SLATE : C_LEAF);
    }
    gfx_rect(0, 150, SCREEN_W, 30, fin ? C_SLATE : C_EARTH);
    gfx_hline(0, SCREEN_W - 1, 150, fin ? C_GREY : C_TAN);
}

static void cover_draw(int row, int x) {
    int s = GA_ROW_SCALE[row], y = GA_ROW_Y[row];
    int w = 12 * s / 2, h = 9 * s / 2;
    if (eg.ga.script == GS_FINALE) { gfx_rect(x - w / 2, y - h, w, h, C_GREY); gfx_rectb(x - w / 2, y - h, w, h, C_INK); return; }
    gfx_circ(x - w / 4, y - h / 2, h / 2 + 1, C_LEAF);
    gfx_circ(x + w / 4, y - h / 2, h / 2 + 1, C_LEAF);
    gfx_circ(x, y - h + 2, h / 2, C_LIME);
}

static void gfoe_draw(const EkoGFoe *f) {
    int x = EKO_PX(f->x), y = EKO_PX(f->y);
    if (f->flash && (f->flash / 2) % 2) return;
    int s = GA_ROW_SCALE[f->row];
    const Sprite *set = s == 2 ? eko_spr_half : s == 3 ? eko_spr_3q : eko_spr;
    switch (f->kind) {
    case GE_RIFLE: case GE_MORTAR: case GE_RUNNER: {
        int sp = f->kind == GE_RUNNER ? SP_GRUNNER : f->kind == GE_MORTAR ? SP_GMORTAR : (f->state == 1 && f->t > 18 && f->t < 40) ? SP_GTROOP_AIM : SP_GTROOP;
        const Sprite *spr = &set[sp];
        int hh = spr->h;
        int up = f->kind == GE_RUNNER ? hh : f->state == 0 ? f->t * hh / 10 : f->state == 2 ? hh - f->t * hh / 10 : hh;
        up = iclamp(up, 0, hh);
        gfx_clip(0, 0, SCREEN_W, y - 2 * s);
        if (f->kind == GE_RUNNER) gfx_noclip();
        spr_draw(spr, x - spr->w / 2, y - up, f->dir < 0 ? SPR_FLIPX : 0);
        gfx_noclip();
        /* about to fire: the muzzle flashes */
        if (f->state == 1 && ((f->t > 22 && f->t < 34) || (f->t > 72 && f->t < 84 && f->a))) gfx_circ(x, y - up + hh / 2, 2, C_YELLOW);
        break;
    }
    case GE_JEEP: {
        int w = 36 * s / 4, h = 14 * s / 4;
        gfx_rect(x - w / 2, y - h, w, h - 2, C_LEAF);
        gfx_rect(x - w / 4, y - h - 4 * s / 4, w / 2, 5 * s / 4, C_FOREST);
        gfx_circ(x - w / 3, y - 2, 2 * s / 2, C_INK);
        gfx_circ(x + w / 3, y - 2, 2 * s / 2, C_INK);
        break;
    }
    case GE_CHOPPER:
        gfx_circ(x, y, 7, C_FOREST);
        gfx_rect(x - 18 * f->dir, y - 2, 12, 3, C_FOREST);
        gfx_hline(x - 14 + (eg.frame_t % 4) * 2, x + 14 - (eg.frame_t % 4) * 2, y - 9, C_GREY);
        gfx_rect(x + 3 * f->dir - 2, y - 3, 4, 3, C_CYAN);
        break;
    case GE_BARREL: {
        int w = 10 * s / 4, h = 12 * s / 4;
        gfx_rect(x - w / 2, y - h, w, h, C_RED);
        gfx_hline(x - w / 2, x + w / 2 - 1, y - h + 2, C_MAROON);
        gfx_hline(x - w / 2, x + w / 2 - 1, y - 3, C_MAROON);
        break;
    }
    case GE_RAMROD: spr_draw(&eko_spr[SP_RAMROD], x - 16, y - 32, f->dir < 0 ? SPR_FLIPX : 0); break;
    case GE_VULTRA: spr_draw(&eko_spr[SP_VULTRA], x - 16, y - 12, 0); break;
    case GE_POD:
        gfx_rect(x - 12, y - 8, 24, 16, C_SLATE);
        gfx_rectb(x - 12, y - 8, 24, 16, C_INK);
        for (int k = 0; k < 3; k++) gfx_circ(x - 7 + k * 7, y + 4, 2, C_RED);
        break;
    case GE_CANNON:
        gfx_rect(x - 12, y - 7, 24, 14, C_GREY);
        gfx_rect(x - 3, y + 6, 6, 8, C_SLATE);
        break;
    case GE_COIL:
        gfx_rect(x - 60, y - 6, 120, 18, C_DUSK);
        gfx_rectb(x - 60, y - 6, 120, 18, C_INK);
        gfx_rect(x - 14, y - 12, 28, 26, f->a ? C_MAGENTA : C_SLATE);
        spr_draw(&eko_spr[SP_COIL], x - 10, y - 10, 0);
        if (!f->a) gfx_dither(x - 14, y - 12, 28, 26, C_GREY, 8);
        break;
    case GE_MISSILE:
        gfx_rect(x - 2, y - 6, 4, 10, C_LIGHT);
        gfx_rect(x - 2, y + 4, 4, 2 + eg.frame_t % 3, C_ORANGE);
        gfx_pset(x, y - 7, C_RED);
        break;
    default: break;
    }
}

static void gallery_draw(void) {
    const EkoGallery *g = &eg.ga;
    gfx_clip(0, 19, SCREEN_W, SCREEN_H - 19);
    gallery_bg();
    const EkoGScript *sc = &EKO_GSCRIPT[g->script];
    /* back to front: sky, far row, middle, near; each row's cover in front of its troops */
    for (int i = 0; i < GA_MAX_FOES; i++)
        if (g->foe[i].alive && (g->foe[i].kind == GE_CHOPPER || g->foe[i].kind == GE_VULTRA || g->foe[i].kind == GE_COIL)) gfoe_draw(&g->foe[i]);
    for (int i = 0; i < GA_MAX_FOES; i++)
        if (g->foe[i].alive && (g->foe[i].kind == GE_POD || g->foe[i].kind == GE_CANNON)) gfoe_draw(&g->foe[i]);
    for (int r = 0; r < GA_ROWS; r++) {
        for (int i = 0; i < GA_MAX_FOES; i++) {
            const EkoGFoe *f = &g->foe[i];
            if (f->alive && f->row == r && f->kind != GE_MISSILE && f->kind != GE_CHOPPER && f->kind != GE_VULTRA &&
                f->kind != GE_POD && f->kind != GE_CANNON && f->kind != GE_COIL) gfoe_draw(f);
        }
        for (int k = 0; k < 6; k++) if (sc->covers[r][k]) cover_draw(r, sc->covers[r][k]);
    }
    for (int i = 0; i < GA_MAX_FOES; i++) if (g->foe[i].alive && g->foe[i].kind == GE_MISSILE) gfoe_draw(&g->foe[i]);
    /* foe fire: grows as it comes */
    for (int i = 0; i < GA_MAX_SHOTS; i++) {
        const EkoGShot *s = &g->shot[i];
        if (!s->alive) continue;
        if (s->kind == 3) {
            int t = imin(s->t, s->dur);
            int y = s->sy + (s->ty - s->sy) * t / s->dur;
            spr_draw(&eko_spr[SP_MANGO], s->tx - 4, y - 9, 0);
            continue;
        }
        if (s->kind == 1 && s->t >= s->dur) {
            /* a grenade on the ground, its fuse fizzing: a tap of A on it picks it up */
            gfx_circ(s->tx, GA_PY - 4, 3, C_FOREST);
            gfx_pset(s->tx + 1, GA_PY - 8 - (eg.frame_t / 2) % 2, (eg.frame_t / 3) % 2 ? C_RED : C_YELLOW);
            if (s->dur + GA_FUSE_T - s->t < 12 && (eg.frame_t / 2) % 2) gfx_circb(s->tx, GA_PY - 4, 6, C_RED);
            continue;
        }
        int x = s->sx + (s->tx - s->sx) * s->t / s->dur;
        int y = s->sy + (s->ty - s->sy) * s->t / s->dur;
        if (s->kind == 4) {
            /* hers, on its way back: it shrinks into the distance */
            y -= eko_sin(s->t * 32 / s->dur) * 30 / 256;
            gfx_circ(x, y, 3 - s->t * 2 / s->dur, C_FOREST);
            continue;
        }
        if (s->kind == 1) y -= eko_sin(s->t * 32 / s->dur) * 40 / 256;
        int r = 1 + s->t * 3 / s->dur;
        gfx_circ(x, y, r, s->kind == 2 ? C_INK : C_YELLOW);
        gfx_circb(x, y, r + 1, s->kind ? C_ORANGE : C_RED);
        /* where it will land */
        if (s->dur - s->t < 18) gfx_hline(s->tx - 6, s->tx + 6, GA_PY + 1, (eg.frame_t / 2) % 2 ? C_RED : C_ORANGE);
    }
    /* VESPER */
    int px = EKO_PX(g->px);
    if (!(g->inv && (g->inv / 3) % 2)) {
        int sp = g->roll_t ? SP_VESPER_ROLL : g->dead_t ? SP_VESPER_ROLL : g->held_t >= GA_LOCK_T || g->cd > 1 ? SP_VESPER_FIRE :
                 (ek_btn(BTN_LEFT) || ek_btn(BTN_RIGHT)) && (eg.frame_t / 8) % 2 ? SP_VESPER_MOVE : SP_VESPER;
        spr_draw(&eko_spr[sp], px - 8, GA_PY - 24, 0);
        if (g->carry) {
            /* a live grenade held up, ready to go back */
            gfx_circ(px, GA_PY - 28, 3, C_FOREST);
            gfx_pset(px + 1, GA_PY - 32, (eg.frame_t / 3) % 2 ? C_RED : C_YELLOW);
        }
    }
    if (g->held_t >= GA_LOCK_T) tiny_draw("PLANTED", px - 13, GA_PY + 2, C_YELLOW);
    /* the sight */
    int cx = EKO_PX(g->cx), cy = EKO_PX(g->cy);
    int cc = g->flash_t ? C_WHITE : C_RED;
    gfx_circb(cx, cy, 5, cc);
    gfx_hline(cx - 8, cx - 3, cy, cc);
    gfx_hline(cx + 3, cx + 8, cy, cc);
    gfx_vline(cx, cy - 8, cy - 3, cc);
    gfx_vline(cx, cy + 3, cy + 8, cc);
    if (g->flash_t) gfx_circ(cx, cy, 2, C_YELLOW);
    parts_draw(0, 0);
    gfx_noclip();
}

/* ---- DART ------------------------------------------------------------------------------------ */

static void bike_draw(void) {
    const EkoBike *k = &eg.bk;
    bool tun = k->course == BC_TUNNEL;
    gfx_clip(0, 19, SCREEN_W, SCREEN_H - 19);
    gfx_cls(tun ? C_INK : C_NAVY);
    if (tun) {
        gfx_dither(0, 19, SCREEN_W, 60, C_MAROON, 6);
        int off = (k->dist / 2) % 50;
        for (int x = -50; x < SCREEN_W + 50; x += 50) {
            gfx_line(x - off, 19, x - off + 20, 80, C_WINE);
            gfx_rect(x - off + 10, 82, 4, 8, (eg.frame_t / 6 + x) % 2 ? C_ORANGE : C_RED);
        }
    } else {
        /* the city at night: domes and minarets */
        gfx_dither(0, 50, SCREEN_W, 40, C_PURPLE, 6);
        int off = (k->dist / 4) % 90;
        for (int x = -90; x < SCREEN_W + 90; x += 90) {
            int bx = x - off;
            gfx_circ(bx + 30, 74, 14, C_NIGHT);
            gfx_rect(bx + 16, 74, 28, 16, C_NIGHT);
            gfx_rect(bx + 60, 46, 5, 44, C_NIGHT);
            gfx_rect(bx + 58, 46, 9, 3, C_NIGHT);
            gfx_pset(bx + 25, 80, C_YELLOW);
            gfx_pset(bx + 35, 82, C_YELLOW);
        }
        gfx_circ(270, 36, 8, C_CREAM);
    }
    /* the road */
    gfx_rect(0, BK_ROAD_TOP - 4, SCREEN_W, BK_ROAD_BOT - BK_ROAD_TOP + 10, tun ? C_DUSK : C_SLATE);
    gfx_hline(0, SCREEN_W - 1, BK_ROAD_TOP - 4, tun ? C_ORANGE : C_GREY);
    gfx_hline(0, SCREEN_W - 1, BK_ROAD_BOT + 5, tun ? C_ORANGE : C_GREY);
    int off = k->dist % 40;
    for (int x = -40; x < SCREEN_W + 40; x += 40) {
        gfx_rect(x - off, 116, 20, 2, C_LIGHT);
        gfx_rect(x - off + 10, 142, 20, 2, C_LIGHT);
    }
    gfx_rect(0, BK_ROAD_BOT + 6, SCREEN_W, SCREEN_H, tun ? C_MAROON : C_EARTH);
    /* everything on the road, far edge first */
    for (int y = BK_ROAD_TOP - 30; y <= BK_ROAD_BOT + 10; y += 2) {
        for (int i = 0; i < BK_MAX_OBJS; i++) {
            const EkoKObj *o = &k->obj[i];
            if (!o->alive || EKO_PX(o->y) / 2 * 2 != y) continue;
            if (o->flash && (o->flash / 2) % 2) continue;
            int x = EKO_PX(o->x) - k->dist, oy = EKO_PX(o->y);
            switch (o->kind) {
            case OB_CONE: spr_draw(&eko_spr[SP_CONE], x - 4, oy - 8, 0); break;
            case OB_HURDLE: spr_draw(&eko_spr[SP_HURDLE], x - 6, oy - 12, 0); break;
            case OB_RUNNER: spr_draw(&eko_spr[SP_BRUNNER], x - 5, oy - 16, SPR_FLIPX); break;
            case OB_MINE: spr_draw(&eko_spr[SP_MINE], x - 4, oy - 8 - (eg.frame_t / 4) % 2, 0); break;
            case OB_BIKER: spr_draw(&eko_spr[SP_BIKER], x - 12, oy - 16, SPR_FLIPX); break;
            case OB_HOG:
                spr_draw(&eko_spr[SP_HOG], x - 14, oy - 20, SPR_FLIPX);
                if (o->state == 2) gfx_circb(x, oy - 10, 14 + o->t % 6, C_RED);
                break;
            case OB_CAR: {
                int col = o->a ? C_BLUE : C_RED;
                gfx_rect(x - 16, oy - 12, 32, 10, col);
                gfx_rect(x - 8, oy - 18, 16, 7, col);
                gfx_rect(x - 6, oy - 17, 12, 4, C_CYAN);
                gfx_circ(x - 10, oy - 2, 3, C_INK);
                gfx_circ(x + 10, oy - 2, 3, C_INK);
                break;
            }
            case OB_GTRUCK:
                gfx_rect(x - 22, oy - 22, 32, 20, C_LEAF);
                gfx_rect(x + 10, oy - 16, 12, 14, C_FOREST);
                gfx_rect(x + 13, oy - 14, 6, 5, C_CYAN);
                gfx_circ(x - 14, oy - 2, 3, C_INK);
                gfx_circ(x + 14, oy - 2, 3, C_INK);
                break;
            case OB_SEMI:
                gfx_rect(x - 34, oy - 30, 52, 28, o->a ? C_SKY : C_LIGHT);
                gfx_rectb(x - 34, oy - 30, 52, 28, C_INK);
                if (o->a) tiny_draw("V.I.P.E.R.", x - 30, oy - 20, C_NAVY);
                gfx_rect(x + 18, oy - 22, 16, 20, C_RED);
                gfx_rect(x + 23, oy - 20, 8, 6, C_CYAN);
                for (int w = -28; w <= 28; w += 14) gfx_circ(x + w, oy - 2, 3, C_INK);
                break;
            case OB_FIREJET:
                gfx_rect(x - 8, oy - 3, 16, 5, C_INK);
                if (o->b >= 60 && o->b < 90 && (eg.frame_t / 3) % 2) gfx_rect(x - 6, oy - 4, 12, 3, C_ORANGE);
                if (o->b >= 90) for (int h = 0; h < 60; h += 4) gfx_rect(x - 6 + (int)(hash2(h, eg.frame_t) % 4), oy - h - 4, 10, 4, h % 8 ? C_ORANGE : C_YELLOW);
                break;
            case OB_ROCK:
                if (o->z > 0) gfx_circ(x, oy, 4 + (120 - EKO_PX(o->z)) / 20, C_DUSK);
                gfx_circ(x, oy - 6 - EKO_PX(o->z), 7, C_EARTH);
                gfx_circ(x - 2, oy - 8 - EKO_PX(o->z), 3, C_BROWN);
                break;
            case OB_MANGO: spr_draw(&eko_spr[SP_MANGO], x - 4, oy - 12 - (eg.frame_t / 8) % 2, 0); break;
            case OB_LIFE: spr_draw(&eko_spr[SP_LIFE], x - 4, oy - 12, 0); break;
            case OB_CARD: spr_draw(&eko_spr[SP_IT_CARD], x - 4, oy - 12 - (eg.frame_t / 8) % 2, 0); gfx_rect(x - 2, oy - 10, 4, 3, C_SKY); break;
            default: break;
            }
        }
        if (EKO_PX(k->y) / 2 * 2 == y && !(k->inv && (k->inv / 3) % 2)) {
            int bx = EKO_PX(k->x), by = EKO_PX(k->y);
            gfx_rect(bx - 10, by - 1, 20, 3, C_INK);
            if (!k->dead_t && k->hot) {
                /* overheated: the whole bike glows red */
                uint8_t map[PAL_COUNT];
                pal_identity(map);
                for (int c = 0; c < PAL_COUNT; c++) if (c != C_INK) map[c] = (uint8_t)((eg.frame_t / 3) % 2 ? C_RED : C_ORANGE);
                spr_draw_ex(&eko_spr[(eg.frame_t / 4) % 2 ? SP_BIKE1 : SP_BIKE2], bx - 12, by - 16 - EKO_PX(k->z), 0, map, -1);
            } else if (!k->dead_t) spr_draw(&eko_spr[(eg.frame_t / 4) % 2 ? SP_BIKE1 : SP_BIKE2], bx - 12, by - 16 - EKO_PX(k->z), 0);
            if (k->heat > BK_HEAT_MAX * 3 / 4) gfx_pset(bx + 10, by - 18 - EKO_PX(k->z) - (eg.frame_t % 6), C_GREY);
        }
    }
    for (int i = 0; i < BK_MAX_SHOTS; i++) {
        const EkoKShot *s = &k->shot[i];
        if (!s->alive) continue;
        if (s->foe) gfx_circ(EKO_PX(s->x), EKO_PX(s->y), 2, C_YELLOW);
        else gfx_rect(EKO_PX(s->x) - 3, EKO_PX(s->y), 6, 2, C_CYAN);
    }
    parts_draw(0, 0);
    gfx_noclip();
}

/* ---- SABLE ------------------------------------------------------------------------------------ */

static void sneak_draw(void) {
    const EkoSneak *s = &eg.sn;
    int rx0 = s->room_x * SN_RW, ry0 = s->room_y * SN_RH;
    int ox = rx0 * 16, oy = ry0 * 16;
    bool cal = s->map == SM_CALDERA;
    gfx_clip(0, SN_TOP - 1, SCREEN_W, SCREEN_H - SN_TOP + 1);
    gfx_cls(C_INK);
    for (int ty = 0; ty < SN_RH; ty++)
        for (int tx = 0; tx < SN_RW; tx++) {
            char c = s->tile[ry0 + ty][rx0 + tx];
            int x = tx * 16, y = SN_TOP + ty * 16;
            switch (c) {
            case '#':
                gfx_rect(x, y, 16, 16, cal ? C_MAROON : C_NAVY);
                gfx_rect(x, y, 16, 3, cal ? C_RED : C_BLUE);
                break;
            case 'o':
                gfx_rect(x, y, 16, 16, cal ? C_DUSK : C_SLATE);
                gfx_rect(x + 1, y + 1, 14, 13, C_GREY);
                gfx_rectb(x + 1, y + 1, 14, 13, C_INK);
                break;
            case 'w':
                gfx_rect(x, y, 16, 16, cal ? C_MAROON : C_NAVY);
                gfx_line(x + 2, y + 3, x + 9, y + 9, C_INK);
                gfx_line(x + 9, y + 9, x + 13, y + 14, C_INK);
                break;
            case 'G': case 'R': case 'U': {
                int col = c == 'G' ? C_LEAF : c == 'R' ? C_RED : C_SKY;
                bool open = !eko_sneak_wall(rx0 + tx, ry0 + ty);
                gfx_rect(x, y, 16, 16, cal ? C_DUSK : C_NIGHT);
                if (!open) { gfx_rect(x + 1, y + 1, 14, 14, col); gfx_rect(x + 5, y + 6, 6, 4, C_INK); }
                else gfx_rectb(x, y, 16, 16, col);
                break;
            }
            case 'X':
                gfx_rect(x, y, 16, 16, cal ? C_DUSK : C_NIGHT);
                gfx_rectb(x + 2, y + 2, 12, 12, (eg.frame_t / 10) % 2 ? C_YELLOW : C_AMBER);
                break;
            default:
                gfx_rect(x, y, 16, 16, cal ? C_DUSK : C_NIGHT);
                if ((tx + ty) % 2) gfx_rect(x + 7, y + 7, 2, 2, cal ? C_MAROON : C_DUSK);
                break;
            }
        }
    /* items */
    static const int ICON[9] = {SP_IT_DART, SP_IT_AMMO, SP_IT_STUN, SP_IT_GREN, SP_MANGO, SP_LIFE, SP_IT_CARD, SP_IT_CARD, SP_IT_CARD};
    static const int CARD_COL[9] = {0, 0, 0, 0, 0, 0, C_LEAF, C_RED, C_SKY};
    for (int i = 0; i < s->nitem; i++) {
        if (s->item[i].taken) continue;
        int tx = s->item[i].tx - rx0, ty = s->item[i].ty - ry0;
        if (tx < 0 || ty < 0 || tx >= SN_RW || ty >= SN_RH) continue;
        int k = s->item[i].kind;
        spr_draw(&eko_spr[ICON[k]], tx * 16 + 4, SN_TOP + ty * 16 + 4, 0);
        if (CARD_COL[k]) gfx_rect(tx * 16 + 6, SN_TOP + ty * 16 + 6, 4, 3, CARD_COL[k]);
    }
    /* guards, cameras, lights */
    int room = s->room_y * SN_MAX_RX + s->room_x;
    for (int i = 0; i < s->ng; i++) {
        const EkoSGuard *g = &s->g[i];
        if (!g->alive || g->room != room) continue;
        int x = EKO_PX(g->x) - ox, y = EKO_PX(g->y) - oy;
        switch (g->kind) {
        case GK_CAMERA: {
            spr_draw(&eko_spr[SP_CAMERA], x - 4, y - 4, 0);
            int ex = x + eko_cos(g->dir) * 14 / 256, ey = y + eko_sin(g->dir) * 14 / 256;
            gfx_line(x, y, ex, ey, C_RED);
            break;
        }
        case GK_LIGHT:
            gfx_dither_circle(x, y, 15, C_YELLOW, 8);
            gfx_circb(x, y, 15, C_AMBER);
            break;
        case GK_CIPHER:
            if (g->state == GD_INVESTIGATE) {
                if ((eg.frame_t / 2) % 3 == 0) gfx_circb(x, y, 7, C_JADE);
            } else if (!(g->flash && (g->flash / 2) % 2)) spr_draw(&eko_spr[SP_CIPHER], x - 8, y - 8, 0);
            break;
        default: {
            static const int SP[4] = {SP_GUARD_R, SP_GUARD_D, SP_GUARD_L, SP_GUARD_U};
            static const int SP2[4] = {SP_GUARD_R2, SP_GUARD_D2, SP_GUARD_L2, SP_GUARD_U2};
            int f = (g->state == GD_PATROL || g->state == GD_ALERT || g->state == GD_INVESTIGATE || g->state == GD_RETURN) && (eg.frame_t / 10) % 2;
            if (g->b) {
                /* the spider robot */
                gfx_circ(x, y, 5, C_GREY);
                for (int l = 0; l < 4; l++) {
                    int a = l * 16 + 8 + (f ? 4 : 0);
                    gfx_line(x, y, x + eko_cos(a) * 8 / 256, y + eko_sin(a) * 8 / 256, C_LIGHT);
                }
                gfx_pset(x + (g->dir == 0) * 3 - (g->dir == 2) * 3, y + (g->dir == 1) * 3 - (g->dir == 3) * 3, C_RED);
            } else {
                uint8_t map[PAL_COUNT];
                pal_identity(map);
                if (g->state == GD_SLEEP || g->state == GD_STUN) pal_swap(map, C_NAVY, C_SLATE);
                spr_draw_ex(&eko_spr[f ? SP2[g->dir & 3] : SP[g->dir & 3]], x - 8, y - 8, 0, map, -1);
            }
            if (g->state == GD_SLEEP) tiny_draw("Z", x + 4, y - 12 - (eg.frame_t / 20) % 3, C_WHITE);
            if (g->state == GD_STUN) tiny_draw("*", x + 4, y - 12, C_YELLOW);
            if (g->state == GD_ALERT && g->flash) text_draw("!", x - 1, y - 18, C_RED);
            if ((g->state == GD_INVESTIGATE || g->state == GD_LOOK) && g->flash) text_draw("?", x - 2, y - 18, C_YELLOW);
            break;
        }
        }
    }
    for (int i = 0; i < SN_MAX_SHOTS; i++) {
        const EkoSShot *sh = &s->shot[i];
        if (!sh->alive) continue;
        int x = EKO_PX(sh->x) - ox, y = EKO_PX(sh->y) - oy;
        if (sh->foe) gfx_rect(x - 1, y - 1, 3, 3, C_YELLOW);
        else if (sh->kind == WP_DART) gfx_rect(x - 1, y - 1, 3, 3, C_PINK);
        else if (sh->kind == WP_PISTOL) gfx_rect(x - 1, y - 1, 3, 3, C_WHITE);
        else gfx_circ(x, y, 3, sh->kind == WP_STUN ? C_WHITE : C_FOREST);
    }
    if (!(s->inv && (s->inv / 3) % 2)) {
        static const int SP[4] = {SP_SABLE_R, SP_SABLE_D, SP_SABLE_L, SP_SABLE_U};
        static const int SP2[4] = {SP_SABLE_R2, SP_SABLE_D2, SP_SABLE_L2, SP_SABLE_U2};
        int f = s->walk_t && (s->walk_t / 8) % 2;
        spr_draw(&eko_spr[f ? SP2[s->dir] : SP[s->dir]], EKO_PX(s->x) - ox - 8, EKO_PX(s->y) - oy - 8, 0);
    }
    if (s->alarm && (eg.frame_t / 8) % 2) gfx_rectb(0, SN_TOP, SCREEN_W, SCREEN_H - SN_TOP, C_RED);
    parts_draw(ox, oy);
    gfx_noclip();
}

/* ---- the jailbreak ------------------------------------------------------------------------------ */

static void owl_remap(uint8_t *map, int owl) {
    pal_identity(map);
    static const int BODY[OWL_COUNT] = {C_BROWN, C_GREY, C_ORANGE, C_NIGHT};
    static const int EDGE[OWL_COUNT] = {C_EARTH, C_SLATE, C_AMBER, C_DUSK};
    static const int SCARF[OWL_COUNT] = {C_RED, C_CYAN, C_BLUE, C_PURPLE};
    pal_swap(map, C_NIGHT, BODY[owl]);
    pal_swap(map, C_DUSK, EDGE[owl]);
    pal_swap(map, C_PURPLE, SCARF[owl]);
}

static void rescue_draw(void) {
    const EkoRescue *r = &eg.rs;
    gfx_cls(C_INK);
    for (int y = 0; y < RS_H; y++)
        for (int x = 0; x < RS_W; x++) {
            int px = RS_X0 + x * RS_TILE, py = RS_Y0 + y * RS_TILE;
            char c = r->tile[y][x];
            switch (c) {
            case '#': gfx_rect(px, py, 16, 16, C_SLATE); gfx_rect(px, py, 16, 3, C_GREY); break;
            case 'o':
                gfx_rect(px, py, 16, 16, C_NIGHT);
                gfx_rect(px + 1, py + 1, 14, 14, C_TAN);
                gfx_rectb(px + 1, py + 1, 14, 14, C_BROWN);
                gfx_line(px + 2, py + 2, px + 13, py + 13, C_BROWN);
                break;
            case 'C': {
                gfx_rect(px, py, 16, 16, C_NIGHT);
                for (int k = 2; k < 16; k += 4) gfx_vline(px + k, py, py + 15, r->card ? C_YELLOW : C_GREY);
                uint8_t map[PAL_COUNT];
                owl_remap(map, r->captive);
                break;
            }
            default:
                gfx_rect(px, py, 16, 16, C_NIGHT);
                gfx_rectb(px, py, 16, 16, C_INK);
                if (c == 'K') { spr_draw(&eko_spr[SP_IT_CARD], px + 4, py + 4, 0); gfx_rect(px + 6, py + 6, 4, 3, C_YELLOW); }
                break;
            }
        }
    /* the captive, behind the bars */
    {
        uint8_t map[PAL_COUNT];
        owl_remap(map, r->captive);
        int px = RS_X0 + r->cell_x * RS_TILE, py = RS_Y0 + r->cell_y * RS_TILE;
        gfx_clip(px, py, 16, 16);
        spr_draw_ex(&eko_spr[SP_RS_SABLE], px, py, 0, map, -1);
        gfx_noclip();
        for (int k = 2; k < 16; k += 4) gfx_vline(px + k, py, py + 15, r->card ? C_YELLOW : C_GREY);
    }
    /* guards and their lines of sight */
    static const int GDX[4] = {1, 0, -1, 0}, GDY[4] = {0, 1, 0, -1};
    for (int i = 0; i < r->ng; i++) {
        const EkoRGuard *g = &r->g[i];
        if (!g->alive) continue;
        int px = RS_X0 + g->x * RS_TILE, py = RS_Y0 + g->y * RS_TILE;
        if (g->state != RGS_SLEEP) {
            /* the line it watches, dotted */
            int x = g->x, y = g->y;
            for (int k = 0; k < RS_W; k++) {
                x += GDX[(int)g->dir];
                y += GDY[(int)g->dir];
                if (x < 0 || y < 0 || x >= RS_W || y >= RS_H) break;
                char c = r->tile[y][x];
                if (c != '.' && c != 'K') break;
                bool blocked = false;
                for (int j = 0; j < r->ng; j++) if (r->g[j].alive && r->g[j].x == x && r->g[j].y == y) blocked = true;
                if (blocked) break;
                gfx_pset(RS_X0 + x * RS_TILE + 7, RS_Y0 + y * RS_TILE + 7, g->state == RGS_ALERT ? C_RED : C_MAROON);
                gfx_pset(RS_X0 + x * RS_TILE + 8, RS_Y0 + y * RS_TILE + 8, g->state == RGS_ALERT ? C_RED : C_MAROON);
            }
        }
        static const int SP[4] = {SP_GUARD_R, SP_GUARD_D, SP_GUARD_L, SP_GUARD_U};
        uint8_t map[PAL_COUNT];
        pal_identity(map);
        if (g->state == RGS_SLEEP) pal_swap(map, C_NAVY, C_SLATE);
        if (g->role == RG_TURNER) pal_swap(map, C_NAVY, C_PURPLE);
        if (g->role == RG_PATROL) pal_swap(map, C_NAVY, C_TEAL);
        spr_draw_ex(&eko_spr[SP[g->dir & 3]], px, py, 0, map, -1);
        if (g->state == RGS_ALERT) text_draw("!", px + 6, py - 8, C_RED);
        if (g->state == RGS_SEEK) text_draw("?", px + 5, py - 8, C_YELLOW);
        if (g->state == RGS_SLEEP) tiny_draw("Z", px + 11, py - 4, C_WHITE);
    }
    /* the bat */
    {
        static const int SP[5] = {SP_SABLE_R, SP_SABLE_R, SP_SABLE_D, SP_SABLE_L, SP_SABLE_U};
        uint8_t map[PAL_COUNT];
        owl_remap(map, r->owl);
        int px = RS_X0 + r->ox * RS_TILE, py = RS_Y0 + r->oy * RS_TILE;
        spr_draw_ex(&eko_spr[SP[iclamp(r->dir, 0, 4)]], px, py, 0, map, -1);
        if (r->brace) gfx_rectb(px, py, 16, 16, C_YELLOW);
        if (r->card) gfx_rect(px + 11, py + 1, 4, 3, C_YELLOW);
    }
    if (r->shot_t)
        gfx_line(RS_X0 + r->shot_x * RS_TILE + 8, RS_Y0 + r->shot_y * RS_TILE + 8, RS_X0 + r->shot_x2 * RS_TILE + 8,
                 RS_Y0 + r->shot_y2 * RS_TILE + 8, r->owl == OWL_SABLE ? C_PINK : C_YELLOW);
    if (r->noise_t) gfx_circb(RS_X0 + r->noise_x * RS_TILE + 8, RS_Y0 + r->noise_y * RS_TILE + 8, 20 + (12 - r->noise_t) * 4, C_YELLOW);
    static const char *const ACT[OWL_COUNT][2] = {{"PUNCH", "BRACE"}, {"SHOOT", "ROLL"}, {"SHOOT", "DASH"}, {"DART", "WHISTLE"}};
    char b[64];
    snprintf(b, sizeof b, GLYPH_A " %s   " GLYPH_B " %s   TURN %d", ACT[r->owl][0], ACT[r->owl][1], r->turns);
    text_draw(b, 8, SCREEN_H - 9, C_LIGHT);
    if (r->won) { gfx_rect(80, 80, 160, 20, C_INK); text_center("THE CELL IS OPEN!", 160, 86, C_YELLOW); }
    if (r->lost) { gfx_rect(80, 80, 160, 20, C_INK); text_center("CAUGHT!", 160, 86, C_RED); }
}

/* ---- the screens --------------------------------------------------------------------------------- */

static void title_draw(void) {
    gfx_cls(C_NIGHT);
    ui_starfield(eg.frame_t, C_NIGHT);
    gfx_circ(160, 210, 120, C_NAVY);
    /* four bats in a row */
    for (int i = 0; i < OWL_COUNT; i++) spr_draw(&eko_spr[FACE[i]], 92 + i * 36, 82 + ((eg.frame_t / 12 + i) % 2), 0);
    ui_fancy_center("ECHO UNIT", 160, 26, 3, TITLE_GRAD, 3, C_INK, C_MAROON);
    text_center("FOUR BATS AGAINST V.I.P.E.R.", 160, 60, C_LIGHT);
    if ((eg.state_t / 20) % 2 == 0) text_center("PRESS " GLYPH_A " TO START", 160, 112, C_WHITE);
    char b[64];
    snprintf(b, sizeof b, "MARSEILLE %d  ANDES %d  CAIRO %d", (int)egs.best[0], (int)egs.best[1], (int)egs.best[2]);
    tiny_center(b, 160, 140, C_GREY);
    snprintf(b, sizeof b, "SUCCESSFUL RESCUES %d", egs.rescues);
    tiny_center(b, 160, 148, C_GREY);
    tiny_center(GLYPH_B " BACK TO THE LIBRARY", 160, 166, C_DUSK);
}

static void story_draw(void) {
    gfx_cls(C_INK);
    int shown = eg.state_t / 40 + 1;
    for (int i = 0; i < EKO_STORY_LINES && i < shown; i++) text_center(EKO_STORY[i], 160, 30 + i * 11, i == EKO_STORY_LINES - 1 ? C_YELLOW : C_LIGHT);
    tiny_center(GLYPH_A " SKIP", 160, 170, C_DUSK);
}

static const char *owl_status(int i) {
    if (eg.owl[i].captured) return "CAPTURED";
    if (eg.owl[i].cleared) return "DONE";
    return "READY";
}

static void hq_draw(void) {
    gfx_cls(C_NIGHT);
    gfx_rect(0, 0, SCREEN_W, 16, C_INK);
    text_center("THE BELFRY: MISSION BOARD", 160, 4, C_YELLOW);
    for (int i = 0; i < 5; i++) {
        int y = 22 + i * 28;
        bool sel = eg.state == ES_HQ && eg.hq_sel == i;
        bool sel2 = eg.state == ES_PICK_RESCUER && eg.rescue_for == i;
        ui_panel(30, y, 260, 25, sel || sel2 ? C_NAVY : C_INK, sel ? C_YELLOW : sel2 ? C_RED : C_DUSK);
        if (i < 4) {
            spr_draw(&eko_spr[FACE[i]], 34, y + 3, 0);
            if (eg.owl[i].captured) for (int k = 0; k < 20; k += 4) gfx_vline(36 + k, y + 3, y + 22, C_GREY);
            text_draw(EKO_OWL_NAME[i], 60, y + 4, C_WHITE);
            text_draw(EKO_MISSION_NAME[i], 60, y + 14, C_LIGHT);
            const char *st = owl_status(i);
            text_draw(st, 284 - text_width(st), y + 9, eg.owl[i].captured ? C_RED : eg.owl[i].cleared ? C_LEAF : C_YELLOW);
            char b[16];
            snprintf(b, sizeof b, "x%d", eg.owl[i].lives);
            (void)b;
        } else {
            bool open = true;
            for (int k = 0; k < OWL_COUNT; k++) if (!eg.owl[k].cleared || eg.owl[k].captured) open = false;
            spr_draw(&eko_spr[SP_FACE_COIL], 34, y + 3, 0);
            text_draw("ALL FOUR", 60, y + 4, C_WHITE);
            text_draw(EKO_MISSION_NAME[4], 60, y + 14, C_LIGHT);
            text_draw(open ? "OPEN" : "LOCKED", 284 - text_width(open ? "OPEN" : "LOCKED"), y + 9, open ? C_YELLOW : C_GREY);
        }
        if (sel) ui_cursor(20, y + 9, eg.frame_t);
    }
    if (eg.state == ES_PICK_RESCUER) {
        ui_panel(60, 54, 200, 80, C_INK, C_RED);
        char b[48];
        snprintf(b, sizeof b, "WHO BREAKS %s OUT?", EKO_OWL_NAME[eg.rescue_for]);
        text_center(b, 160, 60, C_YELLOW);
        for (int i = 0; i < OWL_COUNT; i++) {
            int y = 74 + i * 12;
            bool free = !eg.owl[i].captured;
            text_draw(EKO_OWL_NAME[i], 120, y, !free ? C_DUSK : eg.pick_sel == i ? C_WHITE : C_GREY);
            if (eg.pick_sel == i) ui_cursor(108, y, eg.frame_t);
        }
        tiny_center(GLYPH_B " BACK", 160, 124, C_GREY);
    }
    msg_draw(166);
}

static void brief_draw(void) {
    gfx_cls(C_INK);
    ui_panel(10, 10, 300, 160, C_NIGHT, C_DUSK);
    spr_draw_scaled(&eko_spr[SP_FACE_HUSH], 22, 24, 2, 0);
    text_draw("COLONEL HUSH", 70, 22, C_YELLOW);
    for (int i = 0; i < 4; i++) text_draw(EKO_BRIEF[eg.mission][i], 70, 40 + i * 12, C_LIGHT);
    char b[48];
    snprintf(b, sizeof b, "MISSION: %s", EKO_MISSION_NAME[eg.mission]);
    text_center(b, 160, 112, C_WHITE);
    if (eg.mission < 4) spr_draw_scaled(&eko_spr[FACE[eg.mission]], 140, 124, 2, 0);
    else for (int i = 0; i < 4; i++) spr_draw(&eko_spr[FACE[i]], 100 + i * 32, 128, 0);
    if ((eg.state_t / 20) % 2 == 0 && eg.state_t > 30) tiny_center(GLYPH_A " GO", 160, 164, C_WHITE);
}

static void card_draw(void) {
    const EkoSection *s = &EKO_MISSION[eg.mission].sec[eg.sec];
    gfx_cls(C_INK);
    ui_fancy_center(EKO_MISSION[eg.mission].name, 160, 50, 2, TITLE_GRAD, 3, C_INK, C_MAROON);
    text_center(s->title, 160, 84, C_WHITE);
    spr_draw_scaled(&eko_spr[FACE[s->owl]], 140, 104, 2, 0);
    char b[32];
    snprintf(b, sizeof b, "LIVES x%d", eg.owl[s->owl].lives);
    text_center(b, 160, 152, C_LIGHT);
}

static void clear_draw(void) {
    gfx_cls(C_NAVY);
    ui_fancy_center(eg.won ? "THE WORLD IS SAFE" : "MISSION COMPLETE", 160, 40, 2, TITLE_GRAD, 3, C_INK, C_MAROON);
    char b[48];
    snprintf(b, sizeof b, "%s  %d POINTS", EKO_MISSION_NAME[eg.mission], eg.score);
    text_center(b, 160, 80, C_WHITE);
    if (eg.mission < 3) {
        snprintf(b, sizeof b, "BEST %d", (int)egs.best[eg.mission]);
        text_center(b, 160, 94, C_LIGHT);
    }
    if (eg.won) text_center(eg.lives_lost == 0 ? "NOT ONE LIFE LOST!" : "LIVES LOST ON THE WAY: SEE YOU AGAIN", 160, 110, C_YELLOW);
    if (eg.state_t > 60) tiny_center(GLYPH_A " ON", 160, 160, C_WHITE);
}

static void captured_draw(void) {
    gfx_cls(C_MAROON);
    int o = eg.state == ES_CAPTURED ? eg.cur_owl : eg.state == ES_RESCUE_WON ? eg.rescue_for : eg.rescue_by;
    spr_draw_scaled(&eko_spr[FACE[o]], 140, 40, 2, 0);
    if (eg.state != ES_RESCUE_WON) for (int k = 0; k < 40; k += 6) gfx_rect(140 + k, 36, 2, 48, C_GREY);
    char b[48];
    if (eg.state == ES_RESCUE_WON) snprintf(b, sizeof b, "%s IS FREE!", EKO_OWL_NAME[o]);
    else snprintf(b, sizeof b, "%s IS CAPTURED", EKO_OWL_NAME[o]);
    text_center(b, 160, 100, eg.state == ES_RESCUE_WON ? C_YELLOW : C_WHITE);
    text_center(eg.state == ES_RESCUE_WON ? "THE MISSION CAN BE TRIED AGAIN" : "ANOTHER BAT MUST BREAK THEM OUT", 160, 116, C_LIGHT);
    if (eg.state_t > 60) tiny_center(GLYPH_A " ON", 160, 160, C_WHITE);
}

static void over_draw(void) {
    gfx_cls(C_INK);
    ui_fancy_center("GAME OVER", 160, 60, 3, TITLE_GRAD, 3, C_INK, C_MAROON);
    text_center(eg.in_finale ? "THE CALDERA GIVES NO SECOND CHANCES" : "ALL OF ECHO UNIT IS LOCKED UP", 160, 104, C_LIGHT);
}

static void ending_draw(void) {
    gfx_cls(C_NIGHT);
    ui_starfield(eg.frame_t, C_NIGHT);
    for (int i = 0; i < OWL_COUNT; i++) spr_draw(&eko_spr[FACE[i]], 92 + i * 36, 120, 0);
    spr_draw(&eko_spr[SP_MANGO], 156, 146, 0);
    int shown = eg.state_t / 60 + 1;
    for (int i = 0; i < EKO_ENDING_LINES && i < shown; i++) text_center(EKO_ENDING[i], 160, 14 + i * 12, C_LIGHT);
    if (eg.state_t > 600) text_center("THANKS FOR PLAYING", 160, 166, C_YELLOW);
}

void eko_draw(void) {
    int sx = eg.shake ? (int)(hash2(eg.frame_t, 1) % 5) - 2 : 0;
    gfx_camera(sx, 0);
    switch (eg.state) {
    case ES_TITLE: title_draw(); break;
    case ES_STORY: story_draw(); break;
    case ES_HQ: case ES_PICK_RESCUER: hq_draw(); break;
    case ES_BRIEF: brief_draw(); break;
    case ES_CARD: card_draw(); break;
    case ES_PLAY:
        switch (eg.mode) {
        case MODE_BRAWL: brawl_draw(); break;
        case MODE_GALLERY: gallery_draw(); break;
        case MODE_BIKE: bike_draw(); break;
        case MODE_SNEAK: sneak_draw(); break;
        case MODE_RESCUE: rescue_draw(); break;
        default: break;
        }
        hud();
        msg_draw(24);
        break;
    case ES_CLEAR: clear_draw(); break;
    case ES_CAPTURED: case ES_RESCUE_WON: case ES_RESCUE_LOST: captured_draw(); break;
    case ES_OVER: over_draw(); break;
    case ES_ENDING: ending_draw(); break;
    default: break;
    }
    gfx_camera(0, 0);
}

void eko_draw_label(int x, int y, int w, int h, int t) {
    gfx_clip(x, y, w, h);
    gfx_rect(x, y, w, h, C_NAVY);
    for (int k = 0; k < h; k += 3) gfx_hline(x, x + w - 1, y + k, k > h / 2 ? C_NIGHT : C_NAVY);
    gfx_circ(x + w - 18, y + 14, 8, C_CREAM);
    for (int i = 0; i < OWL_COUNT; i++) spr_draw(&eko_spr[FACE[i]], x + 4 + i * 23, y + h - 24 + ((t / 10 + i) % 2), 0);
    static const uint8_t grad[2] = {C_YELLOW, C_ORANGE};
    ui_fancy_text("ECHO UNIT", x + 5, y + 4, 1, grad, 2, C_INK, C_MAROON);
    gfx_noclip();
}

/* tests: the screens' words stay inside the screen and clear of each other */
int eko_layout_audit(void) {
    int bad = 0;
    int keep = eg.state, keep_t = eg.state_t;
    static const int SCREENS[] = {ES_TITLE, ES_STORY, ES_HQ, ES_BRIEF, ES_CLEAR, ES_OVER, ES_ENDING};
    for (int i = 0; i < ARRAY_LEN(SCREENS); i++) {
        eg.state = SCREENS[i];
        eg.state_t = 2000;
        ui_audit_begin("echounit", true);
        ui_audit_area("screen", 0, 0, SCREEN_W, SCREEN_H);
        if (SCREENS[i] == ES_STORY)
            for (int k = 0; k < EKO_STORY_LINES; k++)
                if (EKO_STORY[k][0]) ui_audit_text("story", EKO_STORY[k], 160 - text_width(EKO_STORY[k]) / 2, 30 + k * 11);
        if (SCREENS[i] == ES_ENDING)
            for (int k = 0; k < EKO_ENDING_LINES; k++)
                if (EKO_ENDING[k][0]) ui_audit_text("ending", EKO_ENDING[k], 160 - text_width(EKO_ENDING[k]) / 2, 14 + k * 12);
        if (SCREENS[i] == ES_BRIEF)
            for (int m = 0; m < MI_COUNT; m++)
                for (int k = 0; k < 4; k++) {
                    if (70 + text_width(EKO_BRIEF[m][k]) > 306) { bad++; fprintf(stderr, "brief %d line %d too wide\n", m, k); }
                    bad += text_missing(EKO_BRIEF[m][k]) > 0;
                }
        if (SCREENS[i] == ES_TITLE) ui_audit_text("start", "PRESS " GLYPH_A " TO START", 160 - text_width("PRESS " GLYPH_A " TO START") / 2, 112);
        bad += ui_audit_end();
        for (int k = 0; k < EKO_STORY_LINES; k++) bad += text_missing(EKO_STORY[k]) > 0 || text_width(EKO_STORY[k]) > SCREEN_W - 8;
        for (int k = 0; k < EKO_ENDING_LINES; k++) bad += text_missing(EKO_ENDING[k]) > 0 || text_width(EKO_ENDING[k]) > SCREEN_W - 8;
    }
    eg.state = keep;
    eg.state_t = keep_t;
    return bad;
}
