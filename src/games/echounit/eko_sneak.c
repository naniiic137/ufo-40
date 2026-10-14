/* ECHO UNIT - SABLE's stealth: the Istanbul listening post (Cipher waits at
 * the bottom of it) and the Caldera's halls, which hold the whole finale
 * together. She is frail (four pips, a blink of safety after a hit) and
 * starts with three tranquiliser darts and three pistol rounds; everything
 * else is found on site. Guards walk their beats, come to look when they
 * hear a shot, and hunt her once they see her; cameras and searchlights set
 * off every guard in the room. Leaving a room makes its guards forget. A
 * room left without being seen is worth 100, less 50 if a noise was heard. */
#include "eko.h"

#define S (&eg.sn)

const char *const EKO_WP_NAME[WP_COUNT] = {"DARTS", "PISTOL", "STUN", "GRENADES"};

/* item kinds */
enum { IK_DARTS, IK_AMMO, IK_STUN, IK_GREN, IK_MANGO, IK_LIFE, IK_GREEN, IK_RED, IK_BLUE };

/* ---- the map -------------------------------------------------------------------- */

static const EkoRoom *room_of(char c) {
    for (int i = 0; i < EKO_ROOMS; i++)
        if (EKO_ROOM[i].code == c) return &EKO_ROOM[i];
    return NULL;
}

static char tile_at(int tx, int ty) {
    if (tx < 0 || ty < 0 || tx >= S->mw || ty >= S->mh) return '#';
    return S->tile[ty][tx];
}

bool eko_sneak_wall(int tx, int ty) {
    char c = tile_at(tx, ty);
    switch (c) {
    case '#': case 'o': case 'w': case ' ': return true;
    case 'G': return !(S->cards & CARD_GREEN);
    case 'R': return !(S->cards & CARD_RED);
    case 'U': return !(S->cards & CARD_BLUE);
    default: return false;
    }
}

bool eko_sneak_solid_px(int x, int y) { return eko_sneak_wall(x >> 4, (y - SN_TOP) >> 4); }

bool eko_sneak_los(int x0, int y0, int x1, int y1) {
    int dx = x1 - x0, dy = y1 - y0;
    int n = imax(iabs(dx), iabs(dy)) / 4;
    for (int i = 1; i < n; i++) {
        int x = x0 + dx * i / n, y = y0 + dy * i / n;
        if (eko_sneak_solid_px(x, y)) return false;
    }
    return true;
}

static int room_index(int rx, int ry) { return ry * SN_MAX_RX + rx; }

static void add_guard(int kind, int tx, int ty, int dir) {
    if (S->ng >= SN_MAX_GUARDS) return;
    EkoSGuard *g = &S->g[S->ng++];
    memset(g, 0, sizeof *g);
    g->alive = 1;
    g->kind = (uint8_t)kind;
    g->room = (uint8_t)room_index(tx / SN_RW, ty / SN_RH);
    g->hx = g->x = (tx * 16 + 8) * EKO_FX;
    g->hy = g->y = (SN_TOP + ty * 16 + 8) * EKO_FX;
    g->hdir = g->dir = dir;
    g->hp = kind == GK_CIPHER ? 26 : 1;
    if (kind == GK_CAMERA) { g->dir = dir * 16; g->hdir = g->dir; }
    if (kind == GK_LIGHT) g->a = 1;
}

static void add_item(int tx, int ty, int kind) {
    if (S->nitem >= ARRAY_LEN(S->item)) return;
    S->item[S->nitem].tx = (int16_t)tx;
    S->item[S->nitem].ty = (int16_t)ty;
    S->item[S->nitem].kind = (uint8_t)kind;
    S->item[S->nitem].taken = 0;
    S->nitem++;
}

/* the way a camera on the wall looks: into the room */
static int camera_dir(int tx, int ty) {
    if (!eko_sneak_wall(tx, ty + 1)) return 1;
    if (!eko_sneak_wall(tx, ty - 1)) return 3;
    if (!eko_sneak_wall(tx + 1, ty)) return 0;
    return 2;
}

static void load_map(int m) {
    const EkoSneakMap *sm = &EKO_SMAP[m];
    memset(S, 0, sizeof *S);
    S->map = m;
    S->boss_g = -1;
    int rows = 0, cols = 0;
    while (rows < SN_MAX_RY && sm->layout[rows]) {
        cols = imax(cols, (int)strlen(sm->layout[rows]));
        rows++;
    }
    S->mw = cols * SN_RW;
    S->mh = rows * SN_RH;
    struct { int tx, ty; } cams[16];
    int ncam = 0;
    for (int ry = 0; ry < rows; ry++)
        for (int rx = 0; rx < cols; rx++) {
            char code = rx < (int)strlen(sm->layout[ry]) ? sm->layout[ry][rx] : '.';
            const EkoRoom *r = code == '.' ? NULL : room_of(code);
            for (int y = 0; y < SN_RH; y++)
                for (int x = 0; x < SN_RW; x++) {
                    int tx = rx * SN_RW + x, ty = ry * SN_RH + y;
                    char c = r ? r->rows[y][x] : ' ';
                    char put = c;
                    switch (c) {
                    case 'h': add_guard(GK_WALKER, tx, ty, 0); put = '.'; break;
                    case 'H': add_guard(GK_WALKER, tx, ty, 2); put = '.'; break;
                    case 'v': add_guard(GK_WALKER, tx, ty, 1); put = '.'; break;
                    case 'V': add_guard(GK_WALKER, tx, ty, 3); put = '.'; break;
                    case 't': add_guard(GK_TURNER, tx, ty, 1); put = '.'; break;
                    case 's': add_guard(GK_WALKER, tx, ty, 1); S->g[S->ng - 1].b = 1; S->g[S->ng - 1].hp = 2; put = '.'; break;
                    case 'c': if (ncam < 16) { cams[ncam].tx = tx; cams[ncam].ty = ty; ncam++; } put = '#'; break;
                    case 'l': add_guard(GK_LIGHT, tx, ty, 0); put = '.'; break;
                    case 'C': S->boss_g = S->ng; add_guard(GK_CIPHER, tx, ty, 1); put = '.'; break;
                    case 'd': add_item(tx, ty, IK_DARTS); put = '.'; break;
                    case 'p': add_item(tx, ty, IK_AMMO); put = '.'; break;
                    case 'n': add_item(tx, ty, IK_STUN); put = '.'; break;
                    case 'e': add_item(tx, ty, IK_GREN); put = '.'; break;
                    case 'm': add_item(tx, ty, IK_MANGO); put = '.'; break;
                    case '1': add_item(tx, ty, IK_LIFE); put = '.'; break;
                    case 'g': add_item(tx, ty, IK_GREEN); put = '.'; break;
                    case 'r': add_item(tx, ty, IK_RED); put = '.'; break;
                    case 'u': add_item(tx, ty, IK_BLUE); put = '.'; break;
                    case 'S': S->start_x = tx; S->start_y = ty; put = '.'; break;
                    default: break;
                    }
                    S->tile[ty][tx] = put;
                }
        }
    /* the walls round every room, then the doorways the map's links open */
    for (int ry = 0; ry < rows; ry++)
        for (int rx = 0; rx < cols; rx++) {
            if (S->tile[ry * SN_RH + 1][rx * SN_RW + 1] == ' ') continue;
            for (int k = 0; k < SN_RW; k++) { S->tile[ry * SN_RH][rx * SN_RW + k] = '#'; S->tile[ry * SN_RH + SN_RH - 1][rx * SN_RW + k] = '#'; }
            for (int k = 0; k < SN_RH; k++) { S->tile[ry * SN_RH + k][rx * SN_RW] = '#'; S->tile[ry * SN_RH + k][rx * SN_RW + SN_RW - 1] = '#'; }
        }
    for (const char *l = sm->links; l && *l;) {
        while (*l == ' ') l++;
        if (!*l) break;
        char a = l[0], b = l[2], door = l[3] == ':' ? l[4] : '.';
        l += l[3] == ':' ? 5 : 3;
        int ax = -1, ay = -1, bx = -1, by = -1;
        for (int ry = 0; ry < rows; ry++)
            for (int rx = 0; sm->layout[ry][rx]; rx++) {
                if (sm->layout[ry][rx] == a) { ax = rx; ay = ry; }
                if (sm->layout[ry][rx] == b) { bx = rx; by = ry; }
            }
        if (ax < 0 || bx < 0) continue;
        if (ay == by && iabs(ax - bx) == 1) {
            int lx = imin(ax, bx);
            for (int k = 4; k <= 5; k++) {
                S->tile[ay * SN_RH + k][lx * SN_RW + SN_RW - 1] = door;
                S->tile[ay * SN_RH + k][(lx + 1) * SN_RW] = door;
            }
        } else if (ax == bx && iabs(ay - by) == 1) {
            int ty = imin(ay, by);
            for (int k = 9; k <= 10; k++) {
                S->tile[ty * SN_RH + SN_RH - 1][ax * SN_RW + k] = door;
                S->tile[(ty + 1) * SN_RH][ax * SN_RW + k] = door;
            }
        }
    }
    for (int i = 0; i < ncam; i++) add_guard(GK_CAMERA, cams[i].tx, cams[i].ty, camera_dir(cams[i].tx, cams[i].ty));
    S->ammo[WP_DART] = 3;
    S->ammo[WP_PISTOL] = 3;
    S->weapon = WP_DART;
    S->x = (S->start_x * 16 + 8) * EKO_FX;
    S->y = (SN_TOP + S->start_y * 16 + 8) * EKO_FX;
    S->dir = 0;
}

static int cur_room(void) { return room_index(S->room_x, S->room_y); }

/* ---- rooms ------------------------------------------------------------------------- */

static void reset_room_guards(int room) {
    for (int i = 0; i < S->ng; i++) {
        EkoSGuard *g = &S->g[i];
        if (!g->alive || g->room != room) continue;
        if (g->kind == GK_CIPHER) continue;
        /* they forget: back on their beat (the ones put to sleep wake up) */
        g->x = g->hx;
        g->y = g->hy;
        g->dir = g->hdir;
        g->state = GD_PATROL;
        g->t = 0;
        g->cd = 30;
        g->flash = 0;
    }
}

void eko_sneak_enter_room(int rx, int ry) {
    int old = cur_room();
    /* the room left behind: its bonus, once */
    if (S->room_seen[S->room_y][S->room_x] == 1) {
        S->room_seen[S->room_y][S->room_x] = 2;
        if (!S->room_spotted) {
            int pts = SN_ROOM_BONUS - (S->room_noise ? SN_NOISE_COST : 0);
            eko_add_score(pts);
            S->rooms_clean++;
            char m[32];
            snprintf(m, sizeof m, "UNSEEN +%d", pts);
            eko_say(m);
            eko_sfx("eko_bonus");
        }
    }
    reset_room_guards(old);
    for (int i = 0; i < SN_MAX_SHOTS; i++) S->shot[i].alive = 0;
    S->room_x = rx;
    S->room_y = ry;
    if (!S->room_seen[ry][rx]) S->room_seen[ry][rx] = 1;
    S->room_spotted = 0;
    S->room_noise = 0;
    S->alarm = 0;
    reset_room_guards(cur_room());
    S->entry_x = S->x;
    S->entry_y = S->y;
}

void eko_sneak_noise(int x, int y) {
    if (!S->room_noise) S->noise_total++;
    S->room_noise = 1;
    for (int i = 0; i < S->ng; i++) {
        EkoSGuard *g = &S->g[i];
        if (!g->alive || g->room != cur_room() || (g->kind != GK_WALKER && g->kind != GK_TURNER)) continue;
        if (g->state == GD_PATROL || g->state == GD_LOOK || g->state == GD_RETURN || g->state == GD_INVESTIGATE) {
            g->state = GD_INVESTIGATE;
            g->tx = x * EKO_FX;
            g->ty = y * EKO_FX;
            g->t = 0;
            g->flash = 30; /* "?" */
        }
    }
}

static void spotted(EkoSGuard *g) {
    if (g->state == GD_ALERT) return;
    g->state = GD_ALERT;
    g->t = 0;
    g->cd = 25;
    g->flash = 40; /* "!" */
    if (!S->room_spotted) S->spotted_total++;
    S->room_spotted = 1;
    eko_sfx("eko_spotted");
}

static void alarm_room(void) {
    if (!S->alarm) eko_sfx("eko_alarm");
    S->alarm = 120;
    for (int i = 0; i < S->ng; i++) {
        EkoSGuard *g = &S->g[i];
        if (g->alive && g->room == cur_room() && (g->kind == GK_WALKER || g->kind == GK_TURNER) &&
            g->state != GD_SLEEP && g->state != GD_STUN)
            spotted(g);
    }
    if (!S->room_spotted) S->spotted_total++;
    S->room_spotted = 1;
}

/* does this guard (or camera, or light) see her now? */
int eko_sneak_guard_sees(int gi) {
    const EkoSGuard *g = &S->g[gi];
    if (!g->alive || g->room != cur_room()) return 0;
    int gx = EKO_PX(g->x), gy = EKO_PX(g->y), sx = EKO_PX(S->x), sy = EKO_PX(S->y);
    int dx = sx - gx, dy = sy - gy;
    switch (g->kind) {
    case GK_WALKER: case GK_TURNER: {
        if (g->state == GD_SLEEP || g->state == GD_STUN) return 0;
        int d = eko_dist(dx, dy);
        if (d < 14) return 1; /* bumped into */
        if (d > (g->b ? SN_VIEW + 40 : SN_VIEW)) return 0;
        int face = g->dir * 16;
        if (eko_angdiff(eko_dir_to(dx, dy), face) > SN_VIEW_ANG) return 0;
        return eko_sneak_los(gx, gy, sx, sy);
    }
    case GK_CAMERA: {
        int d = eko_dist(dx, dy);
        if (d > 100 || d < 4) return 0;
        if (eko_angdiff(eko_dir_to(dx, dy), g->dir) > 4) return 0;
        return eko_sneak_los(gx, gy, sx, sy) || d < 20;
    }
    case GK_LIGHT:
        return eko_dist(dx, dy) < 15;
    default: return 0;
    }
}

/* ---- Sable --------------------------------------------------------------------------- */

static bool box_free(int x, int y, int hw) {
    return !eko_sneak_solid_px(x - hw, y - hw) && !eko_sneak_solid_px(x + hw - 1, y - hw) &&
           !eko_sneak_solid_px(x - hw, y + hw - 1) && !eko_sneak_solid_px(x + hw - 1, y + hw - 1);
}

static const int DX4[4] = {1, 0, -1, 0}, DY4[4] = {0, 1, 0, -1};

static void hurt_sable(int dmg) {
    if (S->inv > 0 || S->dead_t || eg.god) return;
    eko_hurt(dmg);
    if (eko_hp() <= 0) { S->dead_t = 1; eko_sfx("eko_down"); return; }
    S->inv = EKO_OWL_INV[OWL_SABLE];
    S->hurt_t = 10;
    eko_sfx("eko_ouch");
}

static void throw_weapon(void) {
    int w = S->weapon;
    if (S->ammo[w] <= 0) { eko_sfx("eko_tink"); return; }
    S->ammo[w]--;
    for (int i = 0; i < SN_MAX_SHOTS; i++)
        if (!S->shot[i].alive) {
            EkoSShot *s = &S->shot[i];
            int sp = w == WP_DART ? 3 : w == WP_PISTOL ? 4 : 3;
            *s = (EkoSShot){1, 0, S->x + DX4[S->dir] * 6 * EKO_FX, S->y + DY4[S->dir] * 6 * EKO_FX,
                            DX4[S->dir] * sp * EKO_FX, DY4[S->dir] * sp * EKO_FX, 0, w};
            break;
        }
    if (w == WP_PISTOL) { eko_sfx("eko_gun"); eko_sneak_noise(EKO_PX(S->x), EKO_PX(S->y)); }
    else if (w == WP_DART) eko_sfx("eko_dart");
    else eko_sfx("eko_toss");
    /* out of this one: on to the next that has any */
    if (S->ammo[w] == 0)
        for (int k = 1; k < WP_COUNT; k++)
            if (S->ammo[(w + k) % WP_COUNT] > 0) { S->weapon = (w + k) % WP_COUNT; break; }
}

static void take_items(void) {
    int sx = EKO_PX(S->x), sy = EKO_PX(S->y);
    for (int i = 0; i < S->nitem; i++) {
        if (S->item[i].taken) continue;
        int ix = S->item[i].tx * 16 + 8, iy = SN_TOP + S->item[i].ty * 16 + 8;
        if (iabs(ix - sx) > 10 || iabs(iy - sy) > 10) continue;
        S->item[i].taken = 1;
        switch (S->item[i].kind) {
        case IK_DARTS: S->ammo[WP_DART] += 3; eko_say("DARTS +3"); break;
        case IK_AMMO: S->ammo[WP_PISTOL] += 3; eko_say("ROUNDS +3"); break;
        case IK_STUN: S->ammo[WP_STUN] += 2; eko_say("STUN GRENADES +2"); break;
        case IK_GREN: S->ammo[WP_GRENADE] += 2; eko_say("GRENADES +2"); break;
        case IK_MANGO: eko_set_hp(eko_hp() + EKO_MANGO_HP); eko_say("MANGO!"); break;
        case IK_LIFE: eg.owl[eg.cur_owl].lives = imin(EKO_MAX_LIVES, eg.owl[eg.cur_owl].lives + 1); eko_say("EXTRA LIFE"); break;
        case IK_GREEN: S->cards |= CARD_GREEN; eko_say("GREEN CARD"); break;
        case IK_RED: S->cards |= CARD_RED; eko_say("RED CARD"); break;
        case IK_BLUE: S->cards |= CARD_BLUE; eko_say("BLUE CARD"); break;
        }
        for (int k = 0; k < WP_COUNT; k++) S->ammo[k] = imin(S->ammo[k], 9);
        if (S->ammo[S->weapon] == 0)
            for (int k = 0; k < WP_COUNT; k++) if (S->ammo[k] > 0) { S->weapon = k; break; }
        eko_sfx(S->item[i].kind == IK_LIFE ? "eko_1up" : S->item[i].kind >= IK_GREEN ? "eko_key" : "eko_pickup");
    }
}

static void sable_update(void) {
    if (S->dead_t) {
        S->dead_t++;
        if (S->dead_t > 70) eko_life_lost();
        return;
    }
    if (S->inv) S->inv--;
    if (S->hurt_t) S->hurt_t--;
    bool held[4] = {ek_btn(BTN_RIGHT), ek_btn(BTN_DOWN), ek_btn(BTN_LEFT), ek_btn(BTN_UP)};
    bool press[4] = {ek_btnp(BTN_RIGHT), ek_btnp(BTN_DOWN), ek_btnp(BTN_LEFT), ek_btnp(BTN_UP)};
    int mdir = -1;
    for (int k = 0; k < 4; k++) if (press[k]) mdir = k;
    if (mdir < 0 && held[S->dir]) mdir = S->dir;
    if (mdir < 0) for (int k = 0; k < 4; k++) if (held[k]) { mdir = k; break; }
    if (mdir >= 0) {
        S->dir = mdir;
        S->walk_t++;
        int nx = S->x + DX4[mdir] * SN_WALK, ny = S->y + DY4[mdir] * SN_WALK;
        if (box_free(EKO_PX(nx), EKO_PX(ny), SN_PW / 2)) { S->x = nx; S->y = ny; }
        else {
            /* round the corner: slide toward the open lane */
            int px = EKO_PX(S->x), py = EKO_PX(S->y);
            if (DX4[mdir]) {
                int cy = ((py - SN_TOP) & ~15) + SN_TOP + 8;
                int off = py - cy;
                if (iabs(off) <= 6 && off != 0 && box_free(px + DX4[mdir] * 2, cy, SN_PW / 2)) S->y -= isign(off) * EKO_FX;
            } else {
                int cx = (px & ~15) + 8;
                int off = px - cx;
                if (iabs(off) <= 6 && off != 0 && box_free(cx, py + DY4[mdir] * 2, SN_PW / 2)) S->x -= isign(off) * EKO_FX;
            }
        }
    } else S->walk_t = 0;
    if (ek_btnp(EKO_MAIN)) throw_weapon();
    if (ek_btnp(EKO_ALT)) {
        /* the next weapon she has any of */
        for (int k = 1; k <= WP_COUNT; k++) {
            int w = (S->weapon + k) % WP_COUNT;
            if (S->ammo[w] > 0) { if (w != S->weapon) eko_sfx("eko_select"); S->weapon = w; break; }
        }
    }
    take_items();
    /* into the next room */
    int px = EKO_PX(S->x), py = EKO_PX(S->y) - SN_TOP;
    int rx = px / (SN_RW * 16), ry = py / (SN_RH * 16);
    if (rx != S->room_x || ry != S->room_y) eko_sneak_enter_room(rx, ry);
    /* the handover points in the Caldera */
    int tx = px >> 4, ty = py >> 4;
    if (tile_at(tx, ty) == 'X' && !S->relay) {
        S->tile[ty][tx] = '.';
        S->relay = 1;
        S->used_relays++;
        eko_sfx("eko_door");
    }
}

/* ---- guards ---------------------------------------------------------------------------- */

static bool guard_free(const EkoSGuard *g, int x, int y) {
    (void)g;
    int px = EKO_PX(x), py = EKO_PX(y);
    if (!box_free(px, py, 5)) return false;
    /* never out of their own room */
    int rx = px / (SN_RW * 16), ry = (py - SN_TOP) / (SN_RH * 16);
    return room_index(rx, ry) == g->room;
}

/* a step toward (tx, ty), the longer way first, the other if blocked */
static void step_toward(EkoSGuard *g, int tx, int ty, int sp) {
    int dx = tx - g->x, dy = ty - g->y;
    int ax = iabs(dx), ay = iabs(dy);
    int order[2] = {ax >= ay ? 0 : 1, ax >= ay ? 1 : 0};
    for (int k = 0; k < 2; k++) {
        int axis = order[k];
        int d = axis == 0 ? dx : dy;
        if (iabs(d) < 64) continue;
        int nx = g->x + (axis == 0 ? isign(d) * sp : 0), ny = g->y + (axis == 1 ? isign(d) * sp : 0);
        if (guard_free(g, nx, ny)) {
            g->x = nx;
            g->y = ny;
            g->dir = axis == 0 ? (d > 0 ? 0 : 2) : (d > 0 ? 1 : 3);
            return;
        }
    }
    /* snap onto the lane if stuck on a corner */
    int cx = ((EKO_PX(g->x) & ~15) + 8) * EKO_FX, cy = ((((EKO_PX(g->y) - SN_TOP) & ~15) + SN_TOP + 8)) * EKO_FX;
    g->x += iclamp(cx - g->x, -sp, sp);
    g->y += iclamp(cy - g->y, -sp, sp);
}

static void guard_shoot(EkoSGuard *g) {
    for (int i = 0; i < SN_MAX_SHOTS; i++)
        if (!S->shot[i].alive) {
            S->shot[i] = (EkoSShot){1, 1, g->x + DX4[g->dir] * 6 * EKO_FX, g->y + DY4[g->dir] * 6 * EKO_FX,
                                    DX4[g->dir] * 640, DY4[g->dir] * 640, 0, 0};
            eko_sfx("eko_shot");
            return;
        }
}

static void cipher_update(EkoSGuard *g) {
    /* the hacker blinks from terminal to terminal; he can only be hurt while he shows */
    static const int TX[4] = {4, 15, 15, 4}, TY[4] = {2, 2, 7, 7};
    int rx = (g->room % SN_MAX_RX) * SN_RW, ry = (g->room / SN_MAX_RX) * SN_RH;
    g->t++;
    if (g->flash) g->flash--;
    if (g->state == GD_STUN) {
        if (g->t > 120) { g->state = GD_PATROL; g->t = 0; }
        return;
    }
    if (g->state == GD_PATROL) {          /* showing */
        if (g->t % 40 == 20) {
            int sx = EKO_PX(S->x), sy = EKO_PX(S->y);
            int a = eko_dir_to(sx - EKO_PX(g->x), sy - EKO_PX(g->y));
            for (int k = -1; k <= 1; k++)
                for (int i = 0; i < SN_MAX_SHOTS; i++)
                    if (!S->shot[i].alive) {
                        int aa = a + k * 4;
                        S->shot[i] = (EkoSShot){1, 1, g->x, g->y, eko_cos(aa) * 2 + eko_cos(aa) / 2, eko_sin(aa) * 2 + eko_sin(aa) / 2, 0, 0};
                        break;
                    }
            eko_sfx("eko_zap");
        }
        if (g->t > 130) { g->state = GD_INVESTIGATE; g->t = 0; g->a = (g->a + 1 + rng_range(&eg.rng, 0, 2)) % 4; }
    } else {                              /* gone: on his way to the next terminal */
        int tx = ((rx + TX[g->a]) * 16 + 8) * EKO_FX, ty = (SN_TOP + (ry + TY[g->a]) * 16 + 8) * EKO_FX;
        g->x += iclamp(tx - g->x, -512, 512);
        g->y += iclamp(ty - g->y, -512, 512);
        if (g->x == tx && g->y == ty && g->t > 40) { g->state = GD_PATROL; g->t = 0; }
    }
}

static void guards_update(void) {
    int room = cur_room();
    int sx = EKO_PX(S->x), sy = EKO_PX(S->y);
    if (S->alarm) S->alarm--;
    for (int i = 0; i < S->ng; i++) {
        EkoSGuard *g = &S->g[i];
        if (!g->alive || g->room != room) continue;
        if (g->kind == GK_CIPHER) { cipher_update(g); continue; }
        if (g->kind == GK_CAMERA) {
            g->t++;
            g->dir = g->hdir + eko_sin(g->t) * 9 / 256;
            if (!S->dead_t && eko_sneak_guard_sees(i)) alarm_room();
            continue;
        }
        if (g->kind == GK_LIGHT) {
            /* sweeps along its row */
            int nx = g->x + g->a * 160;
            if (!guard_free(g, nx, g->y) || iabs(EKO_PX(nx) - EKO_PX(g->hx)) > 72) g->a = -g->a;
            else g->x = nx;
            if (!S->dead_t && eko_sneak_guard_sees(i)) alarm_room();
            continue;
        }
        g->t++;
        if (g->flash) g->flash--;
        if (g->cd) g->cd--;
        switch (g->state) {
        case GD_PATROL:
            if (g->kind == GK_TURNER) {
                if (g->t % 120 == 0) g->dir = (g->dir + 1) & 3;
            } else {
                int sp = g->b ? 320 : 176; /* a spider robot scuttles */
                int nx = g->x + DX4[g->dir] * sp, ny = g->y + DY4[g->dir] * sp;
                int ax = EKO_PX(g->x) + DX4[g->dir] * 9, ay = EKO_PX(g->y) + DY4[g->dir] * 9;
                if (!guard_free(g, nx, ny) || eko_sneak_solid_px(ax, ay)) { g->dir = (g->dir + 2) & 3; g->t = 0; }
                else { g->x = nx; g->y = ny; }
            }
            break;
        case GD_INVESTIGATE:
            step_toward(g, g->tx, g->ty, 224);
            if ((iabs(g->tx - g->x) < 6 * EKO_FX && iabs(g->ty - g->y) < 6 * EKO_FX) || g->t > 300) { g->state = GD_LOOK; g->t = 0; }
            break;
        case GD_LOOK:
            if (g->t % 30 == 0) g->dir = (g->dir + 1) & 3;
            if (g->t > SN_LOOK_T) { g->state = GD_RETURN; g->t = 0; }
            break;
        case GD_RETURN:
            step_toward(g, g->hx, g->hy, 176);
            if ((iabs(g->hx - g->x) < 2 * EKO_FX && iabs(g->hy - g->y) < 2 * EKO_FX) || g->t > 400) {
                g->x = g->hx; g->y = g->hy; g->dir = g->hdir; g->state = GD_PATROL; g->t = 0;
            }
            break;
        case GD_ALERT: {
            int gx = EKO_PX(g->x), gy = EKO_PX(g->y);
            int dx = sx - gx, dy = sy - gy;
            bool lined = (iabs(dx) < 6 || iabs(dy) < 6) && eko_sneak_los(gx, gy, sx, sy);
            if (lined) {
                g->dir = iabs(dx) < 6 ? (dy > 0 ? 1 : 3) : (dx > 0 ? 0 : 2);
                if (!g->cd && !S->dead_t) { guard_shoot(g); g->cd = 50; }
                if (eko_dist(dx, dy) > 24) step_toward(g, S->x, S->y, 200);
            } else step_toward(g, S->x, S->y, 288);
            break;
        }
        case GD_SLEEP: case GD_STUN:
            if (g->t > (g->state == GD_SLEEP ? SN_SLEEP_T : SN_STUN_T)) { g->state = GD_RETURN; g->t = 0; }
            break;
        default: break;
        }
        if (g->state != GD_ALERT && g->state != GD_SLEEP && g->state != GD_STUN && !S->dead_t && eko_sneak_guard_sees(i)) spotted(g);
        /* a touch hurts once they are onto her */
        if (g->state == GD_ALERT && !S->dead_t && iabs(EKO_PX(g->x) - sx) < 10 && iabs(EKO_PX(g->y) - sy) < 10) hurt_sable(1);
    }
}

/* ---- shots and blasts -------------------------------------------------------------------- */

static void blast(int x, int y, bool stun) {
    eko_burst(x, y, stun ? C_WHITE : C_ORANGE, 14);
    eko_sfx(stun ? "eko_flash" : "eko_boom");
    eg.shake = imax(eg.shake, stun ? 4 : 8);
    eko_sneak_noise(x, y);
    int r = stun ? 36 : 30;
    for (int i = 0; i < S->ng; i++) {
        EkoSGuard *g = &S->g[i];
        if (!g->alive || g->room != cur_room()) continue;
        if (g->kind == GK_CAMERA || g->kind == GK_LIGHT) continue;
        if (eko_dist(EKO_PX(g->x) - x, EKO_PX(g->y) - y) > r + (g->kind == GK_CIPHER ? 8 : 0)) continue;
        if (g->kind == GK_CIPHER) {
            if (g->state == GD_INVESTIGATE) continue; /* not there to hit */
            if (stun) { g->state = GD_STUN; g->t = 0; }
            else { g->hp -= 10; g->flash = 10; eko_sfx("eko_hit"); }
            continue;
        }
        if (stun && !g->b) { g->state = GD_STUN; g->t = 0; }
        else if (stun) continue;
        else { g->alive = 0; g->state = GD_DEAD; eko_burst(EKO_PX(g->x), EKO_PX(g->y), C_RED, 6); }
    }
    if (!stun) {
        /* cracked walls give */
        int tx0 = (x - r) >> 4, tx1 = (x + r) >> 4, ty0 = (y - SN_TOP - r) >> 4, ty1 = (y - SN_TOP + r) >> 4;
        for (int ty = ty0; ty <= ty1; ty++)
            for (int tx = tx0; tx <= tx1; tx++)
                if (tile_at(tx, ty) == 'w' && eko_dist(tx * 16 + 8 - x, SN_TOP + ty * 16 + 8 - y) < r + 10) {
                    S->tile[ty][tx] = '.';
                    eko_burst(tx * 16 + 8, SN_TOP + ty * 16 + 8, C_GREY, 6);
                }
        if (eko_dist(EKO_PX(S->x) - x, EKO_PX(S->y) - y) < r) hurt_sable(2);
    }
}

static void shots_update(void) {
    for (int i = 0; i < SN_MAX_SHOTS; i++) {
        EkoSShot *s = &S->shot[i];
        if (!s->alive) continue;
        s->t++;
        bool thrown = !s->foe && (s->kind == WP_STUN || s->kind == WP_GRENADE);
        int nx = s->x + s->vx, ny = s->y + s->vy;
        if (eko_sneak_solid_px(EKO_PX(nx), EKO_PX(ny)) || (thrown && s->t >= 18)) {
            if (thrown) blast(EKO_PX(s->x), EKO_PX(s->y), s->kind == WP_STUN);
            s->alive = 0;
            continue;
        }
        s->x = nx;
        s->y = ny;
        int x = EKO_PX(s->x), y = EKO_PX(s->y);
        if (s->foe) {
            if (!S->dead_t && iabs(x - EKO_PX(S->x)) < 6 && iabs(y - EKO_PX(S->y)) < 6) { hurt_sable(1); s->alive = 0; }
            continue;
        }
        if (thrown) continue;
        for (int k = 0; k < S->ng; k++) {
            EkoSGuard *g = &S->g[k];
            if (!g->alive || g->room != cur_room() || g->kind == GK_CAMERA || g->kind == GK_LIGHT) continue;
            if (g->state == GD_SLEEP && s->kind == WP_DART) continue;
            if (iabs(x - EKO_PX(g->x)) > 7 || iabs(y - EKO_PX(g->y)) > 7) continue;
            s->alive = 0;
            if (g->kind == GK_CIPHER) {
                if (g->state == GD_INVESTIGATE) { s->alive = 1; continue; }
                if (s->kind == WP_PISTOL) { g->hp -= 3; g->flash = 8; eko_sfx("eko_hit"); }
                else eko_sfx("eko_tink");
                break;
            }
            if (s->kind == WP_DART && g->b) { eko_sfx("eko_tink"); break; } /* darts bounce off a robot */
            if (s->kind == WP_PISTOL && g->b && --g->hp > 0) { g->flash = 6; eko_sfx("eko_hit"); break; }
            if (s->kind == WP_DART) {
                g->state = GD_SLEEP;
                g->t = 0;
                eko_sfx("eko_zzz");
            } else {
                g->alive = 0;
                g->state = GD_DEAD;
                eko_burst(x, y, C_RED, 6);
                eko_sfx("eko_ko");
            }
            break;
        }
    }
}

/* ---- the section ------------------------------------------------------------------------- */

void eko_sneak_start(int map, int zone) {
    load_map(map);
    S->zone = zone;
    int px = EKO_PX(S->x), py = EKO_PX(S->y) - SN_TOP;
    S->room_x = px / (SN_RW * 16);
    S->room_y = py / (SN_RH * 16);
    S->room_seen[S->room_y][S->room_x] = 1;
    S->entry_x = S->x;
    S->entry_y = S->y;
    reset_room_guards(cur_room());
}

/* back to Sable in the Caldera after another bat's part: she carries on */
void eko_sneak_resume(int zone) {
    S->zone = zone;
    S->relay = 0;
    S->done_t = 0;
    S->dead_t = 0;
    S->inv = 60;
    for (int i = 0; i < SN_MAX_SHOTS; i++) S->shot[i].alive = 0;
    reset_room_guards(cur_room());
    S->entry_x = S->x;
    S->entry_y = S->y;
}

void eko_sneak_respawn(void) {
    S->dead_t = 0;
    S->inv = 120;
    S->x = S->entry_x;
    S->y = S->entry_y;
    for (int i = 0; i < SN_MAX_SHOTS; i++) S->shot[i].alive = 0;
    reset_room_guards(cur_room());
    S->alarm = 0;
}

void eko_sneak_update(void) {
    sable_update();
    if (S->dead_t > 70) return;
    guards_update();
    shots_update();
    if (S->boss_g >= 0 && !S->boss_dead) {
        EkoSGuard *c = &S->g[S->boss_g];
        if (c->hp <= 0 && c->alive) {
            c->alive = 0;
            S->boss_dead = 1;
            eko_add_score(500);
            eko_burst(EKO_PX(c->x), EKO_PX(c->y), C_LIME, 20);
            eko_sfx("eko_bossdie");
            eg.shake = 20;
        }
        if (c->alive && c->room == cur_room() && !eko_simulating && music_playing() != EKO_MUS_BOSS) music_play(EKO_MUS_BOSS);
    }
    if (S->boss_dead || S->relay) {
        if (++S->done_t > (S->relay ? 20 : 150)) eko_section_done();
    }
}
