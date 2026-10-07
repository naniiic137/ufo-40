/* RESTLESS - the map, Old Gaunt, his shots, lifts and traps.
 * Positions are fixed point (1/256 px). Gaunt's x is his middle and his y
 * his feet. */
#include "rsl.h"

const int16_t RSL_SIN[RSL_ANG] = {
    0, 25, 50, 74, 98, 121, 142, 162, 181, 198, 213, 226, 237, 245, 251, 255,
    256, 255, 251, 245, 237, 226, 213, 198, 181, 162, 142, 121, 98, 74, 50, 25,
    0, -25, -50, -74, -98, -121, -142, -162, -181, -198, -213, -226, -237, -245, -251, -255,
    -256, -255, -251, -245, -237, -226, -213, -198, -181, -162, -142, -121, -98, -74, -50, -25};

int rsl_isqrt(int v) {
    if (v <= 0) return 0;
    int r = 0, bit = 1 << 30;
    while (bit > v) bit >>= 2;
    while (bit) {
        if (v >= r + bit) { v -= r + bit; r = (r >> 1) + bit; }
        else r >>= 1;
        bit >>= 2;
    }
    return r;
}

int rsl_dist(int dx, int dy) {
    dx = iabs(dx);
    dy = iabs(dy);
    while (dx > 30000 || dy > 30000) { dx >>= 1; dy >>= 1; }
    return rsl_isqrt(dx * dx + dy * dy);
}

int rsl_dir_to(int dx, int dy) {
    while (iabs(dx) > 30000 || iabs(dy) > 30000) { dx /= 2; dy /= 2; }
    int best = 0;
    long long bd = -((long long)1 << 62);
    for (int a = 0; a < RSL_ANG; a++) {
        long long d = (long long)dx * rsl_cos(a) + (long long)dy * rsl_sin(a);
        if (d > bd) { bd = d; best = a; }
    }
    return best;
}

/* bot hints, pit marks and wall writing: never drawn, never solid */
static char hint[RSL_MH][RSL_MW + 1];
static int main_h; /* rows of the half itself; the pit rooms sit below */

char rsl_tile(int tx, int ty) {
    if (tx < 0 || tx >= rg.mw) return '#';
    if (ty < 0) return '.';
    if (ty >= rg.mh) return '.';
    return rg.map[ty][tx];
}
char rsl_hint(int tx, int ty);
char rsl_hint(int tx, int ty) {
    if (tx < 0 || tx >= rg.mw || ty < 0 || ty >= rg.mh) return 0;
    return hint[ty][tx];
}
int rsl_main_h(void);
int rsl_main_h(void) { return main_h; }

static bool solid_c(char c) { return c == '#' || c == 'B' || c == 'F' || c == 'S' || c == 'D'; }
static bool oneway_c(char c) { return c == '=' || c == '-'; }

bool rsl_solid_px(int x, int y) {
    if (x < 0 || y < 0) return x < 0;
    return solid_c(rsl_tile(x / RSL_TILE, y / RSL_TILE));
}
bool rsl_oneway_px(int x, int y) {
    if (x < 0 || y < 0) return false;
    return oneway_c(rsl_tile(x / RSL_TILE, y / RSL_TILE));
}
bool rsl_floor_px(int x, int y) { return rsl_solid_px(x, y) || rsl_oneway_px(x, y); }

int rsl_ground_below(int x, int y) {
    int ty = y / RSL_TILE;
    if (y < 0) ty = 0;
    for (; ty < main_h; ty++) {
        char c = rsl_tile(x / RSL_TILE, ty);
        if ((solid_c(c) || oneway_c(c)) && !solid_c(rsl_tile(x / RSL_TILE, ty - 1))) return ty * RSL_TILE;
    }
    return -1;
}

bool rsl_in_view(int x, int y, int margin) {
    return x >= rg.cam_x - margin && x < rg.cam_x + SCREEN_W + margin && y >= rg.cam_y - margin &&
           y < rg.cam_y + SCREEN_H + margin;
}

void rsl_burst(int x, int y, int col, int n, int sp) {
    for (int k = 0; k < n; k++)
        for (int i = 0; i < RSL_MAX_PARTS; i++)
            if (rg.part[i].life <= 0) {
                int a = rng_range(&rg.rng, 0, 63), s = rng_range(&rg.rng, sp / 3, sp);
                rg.part[i] = (RslPart){x * RSL_FX, y * RSL_FX, rsl_cos(a) * s / 64, rsl_sin(a) * s / 64,
                                       rng_range(&rg.rng, 12, 28), col};
                break;
            }
}

/* ------------------------------------------------------------------------ */
/* loading a half                                                           */

static const RslScreen *find_screen(const RslHalf *H, char code) {
    for (int i = 0; i < H->nscreens; i++)
        if (H->screens[i].code == code) return &H->screens[i];
    return NULL;
}

static int add_spawn(int type, int tx, int ty) {
    if (rg.nspawn >= RSL_MAX_SPAWNS) return -1;
    RslSpawn *s = &rg.spawn[rg.nspawn];
    memset(s, 0, sizeof *s);
    s->type = (uint8_t)type;
    s->tx = tx;
    s->ty = ty;
    s->child = -1;
    return rg.nspawn++;
}

static int foe_of_char(char c) {
    switch (c) {
    case 'q': return FO_LACKEY;
    case 'p': return FO_BLOOM;
    case 'e': return FO_SPITTER;
    case 'h': return FO_HIVE;
    case 'v': return FO_WISP;
    case 'm': return FO_TUMBLER;
    case 't': return FO_TOAD;
    case 'o': return FO_BOULDER;
    case 'c': return FO_CRAB;
    case 'l': return FO_LEAPER;
    case 'd': return FO_WEEPER;
    case 'k': return FO_SHADE;
    case 'g': return FO_CROW;
    case 'n': return FO_FIREMAW;
    case 'z': return FO_SHIELD;
    case 'u': return FO_HULK;
    case 'a': return FO_GNAT;
    default: return FO_NONE;
    }
}

static void copy_screen(const RslScreen *s, int ox, int oy) {
    for (int r = 0; r < RSL_SH; r++) {
        const char *row = s->rows[r];
        for (int c = 0; c < RSL_SW; c++) {
            char ch = row && (int)strlen(row) > c ? row[c] : '#';
            rg.map[oy + r][ox + c] = ch;
        }
    }
}

/* a lift's far end: the ':' in its row (or column) */
static int lift_end(int tx, int ty, bool vertical) {
    for (int d = 1; d < 40; d++) {
        if (vertical) {
            if (rsl_tile(tx, ty - d) == ':') return ty - d;
            if (rsl_tile(tx, ty + d) == ':') return ty + d;
        } else {
            if (rsl_tile(tx + d, ty) == ':') return tx + d;
            if (rsl_tile(tx - d, ty) == ':') return tx - d;
        }
    }
    return vertical ? ty : tx;
}

static void parse_tiles(int x0, int y0, int x1, int y1, bool pit_room) {
    for (int ty = y0; ty < y1; ty++)
        for (int tx = x0; tx < x1; tx++) {
            char c = rg.map[ty][tx];
            int s;
            int fk = foe_of_char(c);
            if (fk != FO_NONE) {
                s = add_spawn(SP_FOE, tx, ty);
                if (s >= 0) { rg.spawn[s].foe = (uint8_t)fk; rg.spawn[s].var = pit_room; }
                rg.map[ty][tx] = c == 'l' ? 'w' : '.';
                continue;
            }
            switch (c) {
            case 'P':
                if (!pit_room) {
                    rg.pl.x = (tx * RSL_TILE + 8) * RSL_FX;
                    rg.pl.y = (ty + 1) * RSL_TILE * RSL_FX;
                }
                s = add_spawn(SP_START, tx, ty);
                if (s >= 0) rg.spawn[s].a = pit_room ? 2 : 1;
                rg.map[ty][tx] = '.';
                break;
            case '|':
                s = add_spawn(SP_START, tx, ty);
                rg.map[ty][tx] = '.';
                break;
            case 'T': case 'W': case 'E':
                s = add_spawn(SP_TORCH, tx, ty);
                if (s >= 0) rg.spawn[s].a = c == 'T' ? TORCH_RANDOM : c == 'W' ? TORCH_WHEEL : TORCH_EGG;
                rg.map[ty][tx] = '.';
                break;
            case 'x':
                add_spawn(SP_SECRET, tx, ty);
                rg.map[ty][tx] = '.';
                break;
            case 'X':
                rg.boss_spawn = add_spawn(SP_BOSS, tx, ty);
                rg.map[ty][tx] = '.';
                break;
            case 'R':
                add_spawn(SP_ROCKFALL, tx, ty);
                rg.map[ty][tx] = '.';
                break;
            case 'F': case 'S':
                s = add_spawn(c == 'F' ? SP_FIREWALL : SP_LAUNCHER, tx, ty);
                if (s >= 0) {
                    /* it faces into its own screen, unless rock is that way */
                    int a = (tx % RSL_SW) < RSL_SW / 2 ? 1 : -1;
                    if (solid_c(rsl_tile(tx + a, ty))) a = -a;
                    rg.spawn[s].a = a;
                }
                break;
            case 'b':
                add_spawn(SP_BUTTON, tx, ty);
                rg.map[ty][tx] = '.';
                break;
            case 'Q': case 'K': case 'Z':
                /* a bell or a clock lying in a treasure room; the way out */
                s = add_spawn(SP_SECRET, tx, ty);
                if (s >= 0) rg.spawn[s].a = c;
                rg.map[ty][tx] = '.';
                break;
            case '!': case '<': case '>': case ',': case '_': case 'Y': case '^':
                hint[ty][tx] = c;
                rg.map[ty][tx] = '.';
                break;
            default: break;
            }
        }
    /* lifts last: their ends are marked with ':' */
    for (int ty = y0; ty < y1; ty++)
        for (int tx = x0; tx < x1; tx++) {
            char c = rg.map[ty][tx];
            if ((c == 'L' || c == 'V') && rg.nlift < RSL_MAX_LIFTS) {
                RslLift *l = &rg.lift[rg.nlift++];
                memset(l, 0, sizeof *l);
                l->vertical = c == 'V';
                l->w = 48;
                l->x0 = l->x1 = tx * RSL_TILE * RSL_FX;
                l->y0 = l->y1 = ty * RSL_TILE * RSL_FX;
                int e = lift_end(tx, ty, l->vertical);
                if (l->vertical) l->y1 = e * RSL_TILE * RSL_FX;
                else l->x1 = e * RSL_TILE * RSL_FX;
                int dist = iabs(l->vertical ? l->y1 - l->y0 : l->x1 - l->x0) / RSL_FX;
                l->period = imax(60, dist * 4 + 60); /* half a pixel a frame, a short rest at each end */
                l->x = l->px = l->x0;
                l->y = l->py = l->y0;
                rg.map[ty][tx] = '.';
            }
        }
    for (int ty = y0; ty < y1; ty++)
        for (int tx = x0; tx < x1; tx++)
            if (rg.map[ty][tx] == ':') rg.map[ty][tx] = '.';
}

void rsl_load_half(int h) {
    const RslHalf *H = &RSL_HALF[h];
    rg.half = h;
    memset(rg.map, 0, sizeof rg.map);
    memset(hint, 0, sizeof hint);
    memset(rg.foe, 0, sizeof rg.foe);
    memset(rg.es, 0, sizeof rg.es);
    memset(rg.shot, 0, sizeof rg.shot);
    memset(rg.item, 0, sizeof rg.item);
    memset(rg.part, 0, sizeof rg.part);
    rg.nspawn = rg.nlift = 0;
    rg.boss_spawn = -1;
    rg.boss_on = rg.boss_dead = false;
    rg.boss_done_t = 0;
    rg.boss_kind = H->boss;
    rg.lock_x = rg.lock_y = -1;
    rg.pit = -1;
    rg.pit_done_t = 0;
    rg.secrets_found = 0;
    rg.lackey_t = 240;
    rg.wisp_t = 900;
    rg.demon_t = 0;
    rg.quiet_t = 0;
    int sy = 0, sx = 0;
    while (sy < RSL_MAX_SY && H->layout[sy]) {
        sx = imax(sx, (int)strlen(H->layout[sy]));
        sy++;
    }
    int pit_rows = 0;
    for (int i = 0; i < 3; i++) if (H->pits[i]) pit_rows = 1;
    if (sy + pit_rows > RSL_MAX_SY) pit_rows = 0;
    rg.mw = imax(sx, 3) * RSL_SW;
    main_h = sy * RSL_SH;
    rg.mh = (sy + pit_rows) * RSL_SH;
    for (int y = 0; y < rg.mh; y++) {
        memset(rg.map[y], '#', (size_t)rg.mw);
        rg.map[y][rg.mw] = 0;
    }
    for (int y = 0; y < sy; y++)
        for (int x = 0; x < (int)strlen(H->layout[y]); x++) {
            const RslScreen *s = find_screen(H, H->layout[y][x]);
            if (s) copy_screen(s, x * RSL_SW, y * RSL_SH);
        }
    parse_tiles(0, 0, rg.mw, main_h, false);
    /* the pit rooms, side by side under the half */
    for (int i = 0; i < 3 && pit_rows; i++)
        if (H->pits[i]) {
            copy_screen(H->pits[i], i * RSL_SW, main_h);
            parse_tiles(i * RSL_SW, main_h, i * RSL_SW + RSL_SW, main_h + RSL_SH, true);
        }
    rg.half_secrets = 0;
    for (int i = 0; i < rg.nspawn; i++)
        if (rg.spawn[i].type == SP_SECRET && !rg.spawn[i].a) rg.half_secrets++;
    /* Gaunt */
    RslPlayer *p = &rg.pl;
    int keep_weapon = p->weapon;
    int px = p->x, py = p->y;
    memset(p, 0, sizeof *p);
    p->x = px;
    p->y = py;
    p->weapon = keep_weapon;
    p->face = 1;
    p->lift = -1;
    p->ground = true;
    p->safe_x = p->x;
    p->safe_y = p->y;
    rg.section_x = p->x;
    rg.section_y = p->y;
    rg.owl.x = p->x - 20 * RSL_FX;
    rg.owl.y = p->y - 40 * RSL_FX;
    rg.owl.target = -1;
    rsl_camera_update(true);
}

/* ------------------------------------------------------------------------ */
/* the camera                                                               */

void rsl_camera_update(bool snap) {
    if (rg.lock_x >= 0) {
        rg.cam_x = rg.lock_x;
        rg.cam_y = rg.lock_y;
        return;
    }
    int px = RSL_PX(rg.pl.x), py = RSL_PX(rg.pl.y);
    int tx = iclamp(px - 150, 0, imax(0, rg.mw * RSL_TILE - SCREEN_W));
    int maxy = imax(0, main_h * RSL_TILE - SCREEN_H);
    int ty = iclamp(py - 112, 0, maxy);
    if (snap) {
        rg.cam_x = tx;
        rg.cam_y = ty;
    } else {
        rg.cam_x = tx;
        if (rg.cam_y < ty) rg.cam_y = imin(ty, rg.cam_y + 3);
        else if (rg.cam_y > ty) rg.cam_y = imax(ty, rg.cam_y - 3);
        /* never let him leave the screen top or bottom */
        if (py - 24 < rg.cam_y) rg.cam_y = imax(0, py - 24);
        if (py > rg.cam_y + SCREEN_H - 4) rg.cam_y = imin(maxy, py - SCREEN_H + 4);
    }
}

/* ------------------------------------------------------------------------ */
/* lifts                                                                    */

static void lifts_update(void) {
    for (int i = 0; i < rg.nlift; i++) {
        RslLift *l = &rg.lift[i];
        l->px = l->x;
        l->py = l->y;
        l->t++;
        int half = l->period / 2;
        int ph = l->t % l->period;
        int f = ph < half ? ph : l->period - ph; /* 0..half */
        /* rest 15 frames at each end */
        int run = imax(1, half - 30);
        int g = iclamp(f - 15, 0, run);
        l->x = l->x0 + (int)((long long)(l->x1 - l->x0) * g / run);
        l->y = l->y0 + (int)((long long)(l->y1 - l->y0) * g / run);
    }
}

int rsl_lift_under(int x, int y) {
    /* x, y in pixels: the feet */
    for (int i = 0; i < rg.nlift; i++) {
        const RslLift *l = &rg.lift[i];
        int lx = RSL_PX(l->x), ly = RSL_PX(l->y);
        if (x >= lx && x < lx + l->w && y >= ly - 1 && y <= ly + 3) return i;
    }
    return -1;
}

/* ------------------------------------------------------------------------ */
/* Gaunt                                                                    */

static int body_h(void) { return rg.pl.duck ? RSL_PH_DUCK : RSL_PH; }

static bool body_hits_solid(int x, int y, int h) {
    /* x middle, y feet, in pixels */
    for (int yy = y - h; yy < y; yy += 7) {
        if (rsl_solid_px(x - RSL_PW / 2, yy) || rsl_solid_px(x + RSL_PW / 2 - 1, yy)) return true;
    }
    return rsl_solid_px(x - RSL_PW / 2, y - 1) || rsl_solid_px(x + RSL_PW / 2 - 1, y - 1);
}

/* is there rock at the leading edge of a body moving dir? */
static bool wall_ahead_of(int x, int y, int h, int dir) {
    int ex = dir > 0 ? x + RSL_PW / 2 - 1 : x - RSL_PW / 2;
    for (int yy = y - h; yy < y; yy += 7)
        if (rsl_solid_px(ex, yy)) return true;
    return rsl_solid_px(ex, y - 1);
}

static bool standing_on(int x, int y, bool allow_oneway) {
    for (int dx = -RSL_PW / 2 + 1; dx <= RSL_PW / 2 - 2; dx += 3) {
        if (rsl_solid_px(x + dx, y)) return true;
        if (allow_oneway && rsl_oneway_px(x + dx, y) && !rsl_oneway_px(x + dx, y - 1) && (y % RSL_TILE) == 0)
            return true;
    }
    return false;
}

void rsl_player_hurt(int src) {
    RslPlayer *p = &rg.pl;
    if (rg.state != RS_PLAY || p->inv > 0 || rg.god) return;
    if (rg.owl.on) {
        rg.owl.on = false;
        rg.owl.flag_t = 70;
        p->inv = RSL_SHIELD_INV;
        rsl_sfx("rsl_flag");
        rg.last_hurt = src;
        return;
    }
    rsl_kill_player(src);
}

void rsl_kill_player(int src) {
    if (rg.state != RS_PLAY) return;
    rg.last_hurt = src;
    rg.deaths++;
    if (rg.deaths > rgs.most_deaths) rgs.most_deaths = (uint16_t)imin(65535, rg.deaths);
    rg.state = RS_DYING;
    rg.state_t = 0;
    rg.pl.charging = false;
    rg.pl.charge = 0;
    rsl_burst(RSL_PX(rg.pl.x), RSL_PX(rg.pl.y) - 12, C_WHITE, 18, 160);
    rsl_sfx("rsl_die");
    rsl_music_stop();
}

/* every foe, shot and loose item in view goes (revival, the lily) */
void rsl_clear_screen(bool drops) {
    for (int i = 0; i < RSL_MAX_FOES; i++) {
        RslFoe *f = &rg.foe[i];
        if (!f->alive || RSL_IS_BOSS(f->kind)) continue;
        if (!rsl_in_view(RSL_PX(f->x), RSL_PX(f->y), 8)) continue;
        if (drops) rsl_kill_foe(i, false);
        else {
            rsl_burst(RSL_PX(f->x), RSL_PX(f->y), C_LIGHT, 6, 120);
            if (f->spawn >= 0) { rg.spawn[f->spawn].child = -1; rg.spawn[f->spawn].t = 0; }
            f->alive = 0;
        }
    }
    for (int i = 0; i < RSL_MAX_ESHOTS; i++)
        if (rg.es[i].alive && rg.es[i].kind != ES_TONGUE && rg.es[i].kind != ES_BLADE) rg.es[i].alive = 0;
}

static void reset_pit_spawns(void) {
    for (int i = 0; i < rg.nspawn; i++)
        if (rg.spawn[i].type == SP_FOE && rg.spawn[i].ty >= main_h) {
            rg.spawn[i].active = 0;
            rg.spawn[i].child = -1;
        }
}

void rsl_enter_pit(int idx) {
    const RslHalf *H = &RSL_HALF[rg.half];
    reset_pit_spawns();
    if (idx < 0 || idx >= 3 || !H->pits[idx]) {
        /* no room: back to the section start */
        rg.pl.x = rg.section_x;
        rg.pl.y = rg.section_y;
        rg.pl.vx = rg.pl.vy = 0;
        rsl_camera_update(true);
        return;
    }
    rg.pit = idx;
    rg.pit_done_t = 0;
    rg.lock_x = idx * RSL_SW * RSL_TILE;
    rg.lock_y = main_h * RSL_TILE + RSL_SH * RSL_TILE - SCREEN_H;
    /* drop in from the top middle of the room */
    rg.pl.x = (rg.lock_x + 160) * RSL_FX;
    rg.pl.y = (main_h * RSL_TILE + 40) * RSL_FX;
    for (int i = 0; i < rg.nspawn; i++)
        if (rg.spawn[i].type == SP_START && rg.spawn[i].a == 2 && rg.spawn[i].tx / RSL_SW == idx &&
            rg.spawn[i].ty >= main_h) {
            rg.pl.x = (rg.spawn[i].tx * RSL_TILE + 8) * RSL_FX;
            rg.pl.y = (rg.spawn[i].ty * RSL_TILE + 4) * RSL_FX;
        }
    rg.pl.vx = rg.pl.vy = 0;
    rg.pl.lift = -1;
    /* everything up top goes */
    for (int i = 0; i < RSL_MAX_FOES; i++)
        if (rg.foe[i].alive && !(rg.foe[i].flags & FF_PIT)) {
            if (rg.foe[i].spawn >= 0) rg.spawn[rg.foe[i].spawn].child = -1;
            rg.foe[i].alive = 0;
        }
    memset(rg.es, 0, sizeof rg.es);
    memset(rg.shot, 0, sizeof rg.shot);
    rsl_camera_update(true);
    if (H->treasure_pit == idx) rsl_music(RSL_MUS_SPECIAL);
    rsl_sfx("rsl_fall");
}

void rsl_leave_pit(void) {
    rg.pit = -1;
    rg.lock_x = rg.lock_y = -1;
    rg.pl.x = rg.section_x;
    rg.pl.y = rg.section_y;
    rg.pl.vx = rg.pl.vy = 0;
    rg.pl.lift = -1;
    rg.pl.ground = true;
    rg.pl.safe_x = rg.pl.x;
    rg.pl.safe_y = rg.pl.y;
    for (int i = 0; i < RSL_MAX_FOES; i++)
        if (rg.foe[i].alive && (rg.foe[i].flags & FF_PIT)) rg.foe[i].alive = 0;
    reset_pit_spawns();
    memset(rg.es, 0, sizeof rg.es);
    for (int i = 0; i < RSL_MAX_ITEMS; i++)
        if (rg.item[i].alive && RSL_PX(rg.item[i].y) >= main_h * RSL_TILE) rg.item[i].alive = 0;
    rg.quiet_t = 90;
    rsl_camera_update(true);
    const int mus[RSL_HALVES] = {RSL_MUS_S1A, RSL_MUS_S1B, RSL_MUS_S2A, RSL_MUS_S2B, RSL_MUS_S3A, RSL_MUS_S3B};
    if (!rg.boss_on) rsl_music(mus[rg.half]);
}

/* which pit he fell down: the run of '_' marks under him (left to right) */
static int pit_index_at(int px) {
    int row = main_h - 1, idx = -1, best = -1, bestd = 1 << 30;
    bool in = false;
    for (int tx = 0; tx < rg.mw; tx++) {
        bool m = hint[row][tx] == '_';
        if (m && !in) idx++;
        in = m;
        if (m) {
            int d = iabs(tx * RSL_TILE + 8 - px);
            if (d < bestd) { bestd = d; best = idx; }
        }
    }
    return best;
}

static void update_section(void) {
    /* the section start: the last '|' (or the start) he has walked past */
    int px = RSL_PX(rg.pl.x);
    int bx = -1, by = 0;
    for (int i = 0; i < rg.nspawn; i++) {
        const RslSpawn *s = &rg.spawn[i];
        if (s->type != SP_START || s->a == 2 || s->ty >= main_h) continue;
        int sx = s->tx * RSL_TILE + 8;
        if (sx <= px + 4 && sx > bx) {
            int g = rsl_ground_below(sx, s->ty * RSL_TILE);
            if (g < 0) continue;
            bx = sx;
            by = g;
        }
    }
    if (bx >= 0 && bx * RSL_FX > rg.section_x - 1) {
        rg.section_x = bx * RSL_FX;
        rg.section_y = by * RSL_FX;
    }
}

static void player_update(void) {
    RslPlayer *p = &rg.pl;
    bool L = rsl_btn(BTN_LEFT), R = rsl_btn(BTN_RIGHT), D = rsl_btn(BTN_DOWN), U = rsl_btn(BTN_UP);
    int dir = R && !L ? 1 : L && !R ? -1 : 0;
    if (p->inv > 0) p->inv--;
    if (p->lag > 0) p->lag--;
    if (p->fire_t > 0) p->fire_t--;
    p->anim++;

    /* riding a lift */
    if (p->lift >= 0) {
        const RslLift *l = &rg.lift[p->lift];
        int nx = p->x + (l->x - l->px);
        if (!body_hits_solid(RSL_PX(nx), RSL_PX(p->y), body_h())) p->x = nx;
        p->y = l->y;
    }

    if (p->ground) {
        p->duck = D && p->lag == 0;
        if (p->lag == 0 && rsl_btnp(BTN_B)) {
            bool on_oneway = !standing_on(RSL_PX(p->x), RSL_PX(p->y), false) && p->lift < 0;
            if (D && on_oneway) {
                p->dropping = true;
                p->y += 3 * RSL_FX;
                p->ground = false;
                p->vy = 0;
                p->jump_dir = 0;
                p->air_t = 0;
                p->duck = false;
            } else {
                p->vy = RSL_JUMP_VY;
                p->ground = false;
                p->jump_dir = dir;
                if (dir) p->face = dir;
                p->air_t = 0;
                p->duck = false;
                p->lift = -1;
                rsl_sfx("rsl_jump");
            }
        } else if (p->lag == 0 && !p->duck && dir) {
            p->face = dir;
            int nx = p->x + dir * RSL_WALK;
            if (!wall_ahead_of(RSL_PX(nx), RSL_PX(p->y), RSL_PH, dir)) p->x = nx;
            p->jump_dir = dir;
        } else {
            p->jump_dir = 0;
        }
        if (!p->duck && !dir && p->lag == 0) p->jump_dir = 0;
        /* still standing? */
        int lf = rsl_lift_under(RSL_PX(p->x), RSL_PX(p->y));
        if (p->ground && lf < 0 && !standing_on(RSL_PX(p->x), RSL_PX(p->y), !p->dropping)) {
            p->ground = false;
            p->vy = 0;
            p->air_t = 0;
            p->lift = -1;
        } else if (p->ground) {
            p->lift = lf;
            if (lf < 0) {
                p->safe_x = p->x;
                p->safe_y = p->y;
            }
        }
    }
    if (!p->ground) {
        p->duck = false;
        if (dir) p->face = dir; /* he can turn in the air, never steer */
        p->air_t++;
        int nx = p->x + p->jump_dir * RSL_WALK;
        if (p->jump_dir && !wall_ahead_of(RSL_PX(nx), RSL_PX(p->y), RSL_PH, p->jump_dir)) p->x = nx;
        p->vy = imin(p->vy + RSL_GRAV, RSL_MAX_FALL);
        int ny = p->y + p->vy;
        int oldf = RSL_PX(p->y), newf = RSL_PX(ny);
        bool landed = false;
        if (p->vy > 0) {
            /* rock and planks under the feet */
            for (int f = oldf; f <= newf && !landed; f++) {
                if (f % RSL_TILE) continue;
                bool solid = standing_on(RSL_PX(p->x), f, false);
                bool plank = !p->dropping && standing_on(RSL_PX(p->x), f, true) && f >= oldf;
                if (solid || plank) { ny = f * RSL_FX; landed = true; }
            }
            if (!landed) {
                for (int i = 0; i < rg.nlift; i++) {
                    const RslLift *l = &rg.lift[i];
                    int lx = RSL_PX(l->x), ly = RSL_PX(l->y);
                    int x = RSL_PX(p->x);
                    if (x >= lx && x < lx + l->w && oldf <= ly + 2 && newf >= ly) {
                        ny = l->y;
                        landed = true;
                        p->lift = i;
                        break;
                    }
                }
            }
        } else if (p->vy < 0) {
            int head = RSL_PX(ny) - RSL_PH;
            if (rsl_solid_px(RSL_PX(p->x) - 4, head) || rsl_solid_px(RSL_PX(p->x) + 3, head)) {
                ny = ((head / RSL_TILE) + 1) * RSL_TILE * RSL_FX + RSL_PH * RSL_FX;
                p->vy = 0;
            }
        }
        p->y = ny;
        if (p->dropping && p->air_t > 10) p->dropping = false;
        if (landed) {
            p->ground = true;
            p->vy = 0;
            p->dropping = false;
            if (p->air_t >= 4) {
                p->lag = RSL_LAND_LAG;
                rsl_sfx("rsl_land");
            }
            p->air_t = 0;
            p->jump_dir = 0;
        }
    }
    /* the walls of a locked screen */
    if (rg.lock_x >= 0) {
        int lo = (rg.lock_x + 6) * RSL_FX, hi = (rg.lock_x + SCREEN_W - 6) * RSL_FX;
        p->x = iclamp(p->x, lo, hi);
    } else {
        p->x = iclamp(p->x, 6 * RSL_FX, (rg.mw * RSL_TILE - 6) * RSL_FX);
    }

    /* down a pit */
    if (rg.pit < 0 && RSL_PX(p->y) >= main_h * RSL_TILE) {
        int idx = pit_index_at(RSL_PX(p->x));
        rsl_enter_pit(idx);
        return;
    }
    if (rg.pit >= 0 && RSL_PX(p->y) > rg.mh * RSL_TILE + 20) {
        /* can't happen in a closed room, but never fall forever */
        rg.pl.x = rg.pl.safe_x;
        rg.pl.y = rg.pl.safe_y;
    }

    /* the attack: a tap shoots at once, a held button charges */
    if (rsl_btnp(BTN_A)) {
        rsl_fire(false);
        p->charging = true;
        p->charge = 0;
    } else if (p->charging && rsl_btn(BTN_A)) {
        p->charge++;
        if (p->charge == RSL_CHARGE_T) rsl_sfx("rsl_charged");
    } else if (p->charging) {
        if (p->charge >= RSL_CHARGE_T) rsl_fire(true);
        p->charging = false;
        p->charge = 0;
    }
    (void)U;
    if (rg.pit < 0) update_section();
}

/* ------------------------------------------------------------------------ */
/* shots                                                                    */

int rsl_shots_alive(int kind) {
    int n = 0;
    for (int i = 0; i < RSL_MAX_SHOTS; i++) n += rg.shot[i].alive && (kind < 0 || rg.shot[i].kind == kind);
    return n;
}

static int new_shot(int kind, int x, int y, int ang, int speed, int life, int dmg, bool charged) {
    for (int i = 0; i < RSL_MAX_SHOTS; i++)
        if (!rg.shot[i].alive) {
            RslShot *s = &rg.shot[i];
            memset(s, 0, sizeof *s);
            s->alive = 1;
            s->kind = (uint8_t)kind;
            s->charged = charged;
            s->x = x;
            s->y = y;
            s->ang = ang & 63;
            s->vx = rsl_cos(ang) * speed / 256;
            s->vy = rsl_sin(ang) * speed / 256;
            s->life = life;
            s->dmg = dmg;
            s->r = charged ? 4 : 3;
            return i;
        }
    return -1;
}

void rsl_fire(bool charged) {
    RslPlayer *p = &rg.pl;
    if (rg.state != RS_PLAY) return;
    bool up = rsl_btn(BTN_UP), down = rsl_btn(BTN_DOWN) && !p->ground;
    int ang = p->face > 0 ? 0 : 32;
    if (up) ang = p->face > 0 ? 56 : 40;
    else if (down) ang = p->face > 0 ? 8 : 24;
    int ox = p->x + p->face * 9 * RSL_FX;
    int oy = p->y - (p->duck ? 8 : 17) * RSL_FX;
    int k = p->weapon;
    switch (k) {
    case WP_STAFF:
        if (!charged && rsl_shots_alive(WP_STAFF) >= 3) return;
        new_shot(WP_STAFF, ox, oy, ang, charged ? 1536 : 1024, charged ? 24 : 18, 8, charged);
        break;
    case WP_EMBER:
        if (!charged && rsl_shots_alive(WP_EMBER) >= 3) return;
        if (charged) {
            int a = new_shot(WP_EMBER, ox, oy, ang, 1408, 40, 10, true);
            int b = new_shot(WP_EMBER, ox, oy, ang, 1408, 40, 10, true);
            if (a >= 0) rg.shot[a].phase = 0;
            if (b >= 0) rg.shot[b].phase = 32;
        } else {
            new_shot(WP_EMBER, ox, oy, ang, 1280, 34, 10, false);
        }
        break;
    case WP_SEEKER: {
        int n = rsl_shots_alive(WP_SEEKER);
        if (!charged && n >= 6) return;
        /* only the first two track all the way; more fall short */
        int life = charged ? 110 : n >= 2 ? 22 : 80;
        new_shot(WP_SEEKER, ox, oy, ang, charged ? 1152 : 768, life, 6, charged);
        break;
    }
    case WP_SCATTER:
        if (!charged && rsl_shots_alive(WP_SCATTER) >= 10) return;
        for (int j = -2; j <= 2; j++) new_shot(WP_SCATTER, ox, oy, ang + j * 2, 896, 16, charged ? 4 : 3, charged);
        break;
    }
    p->fire_t = 10;
    p->shots_fired++;
    rsl_sfx(charged ? "rsl_bigshot" : "rsl_shot");
}

static int nearest_foe(int x, int y, int ang, bool ahead_only) {
    int best = -1, bd = 1 << 30;
    for (int i = 0; i < RSL_MAX_FOES; i++) {
        const RslFoe *f = &rg.foe[i];
        if (!f->alive || (f->flags & FF_HARMLESS)) continue;
        if (f->kind == FO_CRAB && f->state == 2) continue;
        int dx = f->x - x, dy = f->y - y;
        if (ahead_only && (dx * rsl_cos(ang) + dy * rsl_sin(ang)) < 0) continue;
        int d = rsl_dist(dx / RSL_FX, dy / RSL_FX);
        if (d < bd && d < 260) { bd = d; best = i; }
    }
    return best;
}

static bool shot_hits_foe(RslShot *s, int i) {
    const RslFoe *f = &rg.foe[i];
    int sx = RSL_PX(s->x), sy = RSL_PX(s->y);
    int fx = RSL_PX(f->x), fy = RSL_PX(f->y);
    return sx + s->r > fx - f->w / 2 && sx - s->r < fx + f->w / 2 && sy + s->r > fy - f->h / 2 &&
           sy - s->r < fy + f->h / 2;
}

void rsl_shots_update(void) {
    for (int i = 0; i < RSL_MAX_SHOTS; i++) {
        RslShot *s = &rg.shot[i];
        if (!s->alive) continue;
        s->t++;
        if (s->hover > 0) {
            /* a charged scatter pellet hangs in the air and blocks shots */
            s->hover--;
            if (s->hover == 0) s->alive = 0;
        } else {
            if (s->kind == WP_SEEKER && s->t > 4) {
                int every = s->charged ? 2 : 3;
                int tgt = nearest_foe(s->x, s->y, s->ang, true);
                if (tgt >= 0 && s->t % every == 0) {
                    int want = rsl_dir_to(rg.foe[tgt].x - s->x, rg.foe[tgt].y - s->y);
                    int d = ((want - s->ang + 96) & 63) - 32;
                    if (d > 0) s->ang = (s->ang + 1) & 63;
                    else if (d < 0) s->ang = (s->ang + 63) & 63;
                    int sp = s->charged ? 1152 : 768;
                    s->vx = rsl_cos(s->ang) * sp / 256;
                    s->vy = rsl_sin(s->ang) * sp / 256;
                }
            }
            s->x += s->vx;
            s->y += s->vy;
            s->life--;
            if (s->life <= 0) {
                if (s->kind == WP_SCATTER && s->charged) { s->hover = 60; s->vx = s->vy = 0; s->life = 1; }
                else { s->alive = 0; continue; }
            }
        }
        /* the helix of a charged ember */
        int dx = 0, dy = 0;
        if (s->kind == WP_EMBER && s->charged) {
            int o = rsl_sin(s->t * 3 + s->phase) * 7;
            dx = -rsl_sin(s->ang) * o / 256;
            dy = rsl_cos(s->ang) * o / 256;
        }
        int sx = RSL_PX(s->x + dx), sy = RSL_PX(s->y + dy);
        if (rsl_solid_px(sx, sy)) {
            rsl_burst(sx, sy, C_LIGHT, 2, 60);
            s->alive = 0;
            continue;
        }
        if (!rsl_in_view(sx, sy, 24)) { s->alive = 0; continue; }
        int ox = s->x, oy = s->y;
        s->x += dx;
        s->y += dy;
        bool hit = false;
        /* foes */
        for (int f = 0; f < RSL_MAX_FOES && !hit; f++) {
            RslFoe *fo = &rg.foe[f];
            if (!fo->alive || (fo->flags & FF_HARMLESS) || fo->kind == FO_KING) continue;
            if (!shot_hits_foe(s, f)) continue;
            hit = true;
            rsl_hurt_foe(f, s->dmg, s->vx >= 0 ? 1 : -1);
            if (s->kind == WP_SEEKER && s->charged && !s->split) {
                for (int j = -1; j <= 1; j += 2) {
                    int n = new_shot(WP_SEEKER, s->x, s->y, s->ang + j * 6, 768, 40, 6, false);
                    if (n >= 0) { rg.shot[n].split = 1; rg.shot[n].t = 5; }
                }
            }
        }
        /* the Hollow King's eyes are his only weak spots */
        if (!hit) {
            for (int f = 0; f < RSL_MAX_FOES && !hit; f++) {
                RslFoe *fo = &rg.foe[f];
                if (!fo->alive || fo->kind != FO_KING) continue;
                int ex[3] = {fo->x - 22 * RSL_FX, fo->x + 22 * RSL_FX, fo->x};
                int ey[3] = {fo->y - 10 * RSL_FX, fo->y - 10 * RSL_FX, fo->y - 30 * RSL_FX};
                int n = fo->a ? 3 : 2; /* the third eye only while open */
                for (int e = 0; e < n && !hit; e++)
                    if (iabs(s->x - ex[e]) < (7 + s->r) * RSL_FX && iabs(s->y - ey[e]) < (6 + s->r) * RSL_FX) {
                        hit = true;
                        rsl_hurt_foe(f, s->dmg, 0);
                    }

            }
        }
        /* shootable foe shots: bombs, fireballs */
        for (int e = 0; e < RSL_MAX_ESHOTS && !hit; e++) {
            RslEShot *q = &rg.es[e];
            if (!q->alive) continue;
            if (!q->shootable && !(s->kind == WP_SCATTER && s->charged && s->hover > 0)) continue;
            if (q->kind == ES_TRAP || q->kind == ES_FLAME || q->kind == ES_TONGUE || q->kind == ES_BLADE ||
                q->kind == ES_BLAZE)
                continue;
            if (iabs(q->x - s->x) < (q->w / 2 + s->r) * RSL_FX && iabs(q->y - s->y) < (q->h / 2 + s->r) * RSL_FX) {
                q->hp -= (int16_t)s->dmg;
                rsl_burst(RSL_PX(q->x), RSL_PX(q->y), C_ORANGE, 4, 100);
                if (q->hp <= 0 || !q->shootable) q->alive = 0;
                if (!(s->kind == WP_SCATTER && s->hover > 0)) hit = true;
                rsl_sfx("rsl_pop");
            }
        }
        /* torches */
        for (int t = 0; t < rg.nspawn && !hit; t++) {
            RslSpawn *sp = &rg.spawn[t];
            if (sp->type != SP_TORCH || sp->used) continue;
            int tx = sp->tx * RSL_TILE + 8, ty = sp->ty * RSL_TILE + 4;
            if (iabs(sx - tx) < 7 + s->r && iabs(sy - ty) < 12 + s->r) {
                hit = true;
                rsl_light_torch(t);
            }
        }
        s->x = ox;
        s->y = oy;
        if (hit && s->hover == 0) {
            rsl_burst(sx, sy, s->kind == WP_EMBER ? C_ORANGE : C_ICE, 3, 80);
            s->alive = 0;
        }
    }
}

/* ------------------------------------------------------------------------ */
/* traps                                                                    */

static bool spawn_on_screen(const RslSpawn *s, int margin) {
    return rsl_in_view(s->tx * RSL_TILE + 8, s->ty * RSL_TILE + 8, margin);
}

static void traps_update(void) {
    int px = RSL_PX(rg.pl.x), py = RSL_PX(rg.pl.y);
    for (int i = 0; i < rg.nspawn; i++) {
        RslSpawn *s = &rg.spawn[i];
        if (s->t > 0) s->t--;
        switch (s->type) {
        case SP_ROCKFALL: {
            if (s->t > 0 || !spawn_on_screen(s, 0)) break;
            int rx = s->tx * RSL_TILE + 8;
            if (s->b > 0) {
                /* it shakes loose, then drops */
                if (--s->b == 0) {
                    int e = rsl_add_eshot(ES_ROCK, rx * RSL_FX, (s->ty * RSL_TILE + 6) * RSL_FX, 0, 0);
                    if (e >= 0) rg.es[e].grav = 30;
                    s->t = 200;
                }
            } else if (iabs(px - rx) < 40 && py > s->ty * RSL_TILE) {
                s->b = 30;
                rsl_sfx("rsl_rumble");
            }
            break;
        }
        case SP_FIREWALL: {
            if (!spawn_on_screen(s, 16)) { s->b = 0; break; }
            s->b = (s->b + 1) % 200;
            if (s->b == 70) rsl_sfx("rsl_roar");
            if (s->b >= 70 && s->b < 130 && s->b % 6 == 0) {
                int x = (s->tx * RSL_TILE + 8 + s->a * 14) * RSL_FX;
                int e = rsl_add_eshot(ES_FLAME, x, (s->ty * RSL_TILE + 8) * RSL_FX, s->a * 640, 0);
                if (e >= 0) rg.es[e].life = 18;
            }
            break;
        }
        case SP_BUTTON: {
            if (s->t > 0) break;
            int bx = s->tx * RSL_TILE, by = (s->ty + 1) * RSL_TILE;
            bool pressed = rg.pl.ground && px + 4 > bx + 2 && px - 4 < bx + 14 && iabs(py - by) < 3;
            for (int f = 0; f < RSL_MAX_FOES && !pressed; f++) {
                const RslFoe *fo = &rg.foe[f];
                if (!fo->alive || !(fo->flags & FF_GROUNDED)) continue;
                int fx = RSL_PX(fo->x), ff = RSL_PX(fo->y) + fo->h / 2;
                pressed = fx + 4 > bx + 2 && fx - 4 < bx + 14 && iabs(ff - by) < 4;
            }
            if (!pressed) break;
            s->t = 120;
            rsl_sfx("rsl_click");
            /* every launcher on this screen fires */
            int scr = s->tx / RSL_SW, scy = s->ty / RSL_SH;
            for (int j = 0; j < rg.nspawn; j++) {
                const RslSpawn *l = &rg.spawn[j];
                if (l->type != SP_LAUNCHER || l->tx / RSL_SW != scr || l->ty / RSL_SH != scy) continue;
                int x = (l->tx * RSL_TILE + 8 + l->a * 12) * RSL_FX;
                rsl_add_eshot(ES_TRAP, x, (l->ty * RSL_TILE + 8) * RSL_FX, l->a * 1100, 0);
            }
            break;
        }
        default: break;
        }
    }
}

/* ------------------------------------------------------------------------ */

static void parts_update(void) {
    for (int i = 0; i < RSL_MAX_PARTS; i++) {
        RslPart *q = &rg.part[i];
        if (q->life <= 0) continue;
        q->life--;
        q->x += q->vx;
        q->y += q->vy;
        q->vy += 12;
    }
}

void rsl_world_update(void) {
    lifts_update();
    player_update();
    if (rg.state != RS_PLAY) return;
    rsl_camera_update(false);
    traps_update();
    rsl_shots_update();
    rsl_spawners_update();
    rsl_foes_update();
    rsl_eshots_update();
    rsl_items_update();
    rsl_owlet_update();
    parts_update();
    if (rg.shake > 0) rg.shake--;
}
