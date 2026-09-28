/* HOMESPUN - the action: Wick walking eight ways, her yo-yo and the
 * thunderpipe, the foes and bosses, pots, chests and pickups, rooms and
 * the ways between them, and the trip clock that is also her health. It
 * runs on buttons alone (held and newly pressed), so the demo player plays
 * it the way a person does. Positions are in 1/16 pixel of the field. */
#include "homespun.h"

HsSim hs;
static Rng lr; /* loot and foe dice, seeded per trip */
int hs_hurt_by[16]; /* hits taken, by shot kind; 14 a foe's touch, 15 Mawbo's (for tuning and tests) */

#define PX(v) ((v) / HS_U)
#define SUB(v) ((v) * HS_U)
#define WICK_HALF 5

/* ---- small helpers ------------------------------------------------------------------ */

void hs_sim_seed(uint32_t seed) { rng_seed(&lr, ((uint64_t)seed << 16) ^ 0x5eedu); }

void hs_say(const char *s) {
    snprintf(hs.msg, sizeof hs.msg, "%s", s);
    hs.msg_t = 150 + (int)strlen(s) * 2;
}

static void fx_add(int kind, int x, int y) {
    for (int i = 0; i < ARRAY_LEN(hs.fxs); i++)
        if (!hs.fxs[i].on) { hs.fxs[i] = (HsFx){1, (uint8_t)kind, 0, (int16_t)x, (int16_t)y}; return; }
}

int hs_parts_count(void) { return (sv.parts & 1) + (sv.parts >> 1 & 1) + (sv.parts >> 2 & 1); }

int hs_total(int res) { return sv.res[res] + (sv.on_trip ? sv.gain[res] : 0); }

static void gain(int res, int n) {
    if (!sv.on_trip) { hs_add(res, n); return; }
    int room = hs_cap(res) - sv.res[res] - sv.gain[res];
    if (room < 0) room = 0;
    sv.gain[res] += imin(n, room);
}

bool hs_spend_glint(void) {
    if (sv.on_trip && sv.gain[RES_GLINT] > 0) { sv.gain[RES_GLINT]--; return true; }
    if (sv.res[RES_GLINT] > 0) { sv.res[RES_GLINT]--; return true; }
    return false;
}

static bool tile_solid(int t) {
    return t == T_WALL || t == T_WATER || t == T_LAVA || t == T_BLOCK || t == T_CRACK || t == T_SHUT;
}

static int tile_at_px(int x, int y) {
    if (x < 0 || y < 0 || x >= HS_FW || y >= HS_FH) return T_FLOOR; /* beyond the edge: the next room */
    return hs.tile[y / HS_T][x / HS_T];
}

static bool thing_solid(const HsThing *t) {
    return t->on && t->kind != TH_PAD && t->kind != TH_GEAR && t->kind != TH_PART && !(t->kind == TH_POT && t->open);
}

bool hs_solid_px(int x, int y) {
    if (tile_solid(tile_at_px(x, y))) return true;
    if (hs.area == AR_BASE) {
        for (int p = 0; p < P_COUNT; p++) {
            const HsSpot *s = &HS_SPOT[p];
            if (!s->solid || !hs_spot_shown(p)) continue;
            if (x >= s->x && x < s->x + s->w && y >= s->y && y < s->y + s->h) return true;
        }
        return false;
    }
    for (int i = 0; i < HS_MAX_THINGS; i++) {
        const HsThing *t = &hs.thing[i];
        if (!thing_solid(t)) continue;
        int tx = t->tx * HS_T + 2, ty = t->ty * HS_T + 2;
        if (x >= tx && x < tx + 12 && y >= ty && y < ty + 12) return true;
    }
    return false;
}

static bool box_free(int cx, int cy, int half) {
    int x0 = cx - half, y0 = cy - half, x1 = cx + half - 1, y1 = cy + half - 1;
    for (int y = y0; y <= y1; y += imax(1, (y1 - y0) / 2))
        for (int x = x0; x <= x1; x += imax(1, (x1 - x0) / 2))
            if (hs_solid_px(x, y)) return false;
    return !hs_solid_px(x1, y1) && !hs_solid_px(x0, y1) && !hs_solid_px(x1, y0);
}

bool hs_box_free(int cx, int cy, int half) { return box_free(cx, cy, half); }

/* move a body; returns bit 1 if x was blocked, 2 if y */
static int move_body(int32_t *x, int32_t *y, int32_t vx, int32_t vy, int half, bool slide) {
    int blocked = 0;
    if (vx) {
        int32_t nx = *x + vx;
        if (box_free(PX(nx), PX(*y), half)) *x = nx;
        else {
            blocked |= 1;
            if (slide) /* round a corner: step toward the free side */
                for (int k = 1; k <= 7; k++) {
                    if (box_free(PX(nx), PX(*y) - k, half)) { *y -= SUB(1); break; }
                    if (box_free(PX(nx), PX(*y) + k, half)) { *y += SUB(1); break; }
                }
        }
    }
    if (vy) {
        int32_t ny = *y + vy;
        if (box_free(PX(*x), PX(ny), half)) *y = ny;
        else {
            blocked |= 2;
            if (slide)
                for (int k = 1; k <= 7; k++) {
                    if (box_free(PX(*x) - k, PX(ny), half)) { *x -= SUB(1); break; }
                    if (box_free(PX(*x) + k, PX(ny), half)) { *x += SUB(1); break; }
                }
        }
    }
    return blocked;
}

static int wick_x(void) { return PX(hs.px); }
static int wick_y(void) { return PX(hs.py); }

/* ---- pickups -------------------------------------------------------------------------- */

void hs_drop(int kind, int amount, int32_t x, int32_t y) {
    for (int i = 0; i < HS_MAX_ITEMS; i++)
        if (!hs.item[i].on) {
            int32_t ox = SUB(rng_range(&lr, -8, 8)), oy = SUB(rng_range(&lr, -6, 6));
            if (kind == IT_IDOL) ox = oy = 0; /* the idol falls where Mawbo stood */
            int32_t nx = x + ox, ny = y + oy;
            if (hs_solid_px(PX(nx), PX(ny))) { nx = x; ny = y; }
            /* a fish's loot washes up on the nearest dry spot */
            for (int r = 1; r <= 3 && !box_free(PX(nx), PX(ny), 5); r++)
                for (int d = 0; d < 8; d++) {
                    static const int D8[8][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
                    int32_t tx = x + SUB(D8[d][0] * 12 * r), ty = y + SUB(D8[d][1] * 12 * r);
                    if (box_free(PX(tx), PX(ty), 5)) { nx = tx; ny = ty; break; }
                }
            hs.item[i] = (HsItem){1, (uint8_t)kind, (int16_t)amount, 0, nx, ny};
            return;
        }
}

static void loot(int tier, int32_t x, int32_t y) {
    int r = rng_range(&lr, 0, 999);
    switch (tier) {
    case 0:
        if (r < 990) hs_drop(IT_GLINT, 3, x, y);
        else hs_drop(IT_BAR, 1, x, y);
        break;
    case 1:
        if (r < 700) hs_drop(IT_GLINT, 5, x, y);
        else if (r < 750) hs_drop(IT_BAR, 1, x, y);
        else if (r < 800) hs_drop(IT_ODD, 1, x, y);
        else if (r < 900) hs_drop(IT_JERKY, 1, x, y);
        else hs_drop(IT_TONIC, 1, x, y);
        break;
    case 2:
        if (r < 500) hs_drop(IT_GLINT, 10, x, y);
        else if (r < 750) hs_drop(IT_BAR, 1, x, y);
        else if (r < 875) { hs_drop(IT_GLINT, 5, x, y); hs_drop(IT_ODD, 1, x, y); hs_drop(IT_BAR, 1, x, y); }
        else hs_drop(IT_TONIC, 1, x, y);
        break;
    default:
        break;
    }
}

static void take_item(HsItem *it) {
    switch (it->kind) {
    case IT_GLINT: gain(RES_GLINT, it->amount); sfx_play_name("hs_glint"); break;
    case IT_BAR: gain(RES_BAR, it->amount); sfx_play_name("hs_bar"); break;
    case IT_JERKY: gain(RES_JERKY, it->amount); sfx_play_name("hs_bar"); break;
    case IT_DATA: gain(RES_DATA, it->amount); sfx_play_name("hs_bar"); break;
    case IT_THREAD: gain(RES_THREAD, it->amount); sfx_play_name("hs_bar"); break;
    case IT_ODD: sv.odds = (uint16_t)imin(999, sv.odds + it->amount); sfx_play_name("hs_odd"); break;
    case IT_TONIC: sv.time_f += TONIC_SMALL_S * 60 * it->amount; sfx_play_name("hs_tonic"); break;
    case IT_BIGTONIC: sv.time_f += TONIC_BIG_S * 60; sfx_play_name("hs_tonic"); break;
    case IT_IDOL:
        sv.carry |= 16;
        hs_say("THE GRINNING IDOL! TAKE IT HOME.");
        hs.events |= EV_IDOL;
        sfx_play_name("hs_part");
        break;
    default: break;
    }
    hs.events |= EV_PICK;
    fx_add(FX_SPARK, PX(it->x), PX(it->y));
    it->on = 0;
}

/* ---- rooms ------------------------------------------------------------------------------ */

bool hs_mawbo_here(void) {
    return sv.on_trip && sv.parts == 7 && !sv.idol && !(sv.carry & 16) && hs.room == sv.mawbo_room && sv.mawbo_wins < 6;
}

int hs_add_mob(int kind, int tx, int ty, int spawn) {
    for (int i = 0; i < HS_MAX_MOBS; i++)
        if (!hs.mob[i].on) {
            HsMob *m = &hs.mob[i];
            memset(m, 0, sizeof *m);
            m->on = 1;
            m->kind = (uint8_t)kind;
            m->spawn = (uint8_t)spawn;
            m->x = SUB(tx * HS_T + HS_T / 2);
            m->y = SUB(ty * HS_T + HS_T / 2);
            m->hp = (int16_t)HS_FOE[kind].hp;
            m->t = (int16_t)rng_range(&lr, 0, 60);
            m->dx = 1;
            m->up = 1;
            m->face = 1;
            m->cool = (int16_t)rng_range(&lr, 30, 90);
            if (kind == E_GRUBLET || kind == E_SPLOSH) m->up = 0;
            if (HS_FOE[kind].tier == 3) hs.boss = i;
            return i;
        }
    return -1;
}

static void set_shutters(bool shut) {
    for (int y = 0; y < HS_TH; y++)
        for (int x = 0; x < HS_TW; x++) {
            if (shut && hs.tile[y][x] == T_DOOR) hs.tile[y][x] = T_SHUT;
            if (!shut && hs.tile[y][x] == T_SHUT && hs.area == AR_DUN) hs.tile[y][x] = T_DOOR;
        }
    hs.shut = shut;
}

void hs_sim_enter(int room, int32_t px, int32_t py) {
    int biome = 0;
    hs.room = room;
    hs.area = hs_room_area(room);
    hs_room_tiles(room, hs.tile, &biome);
    hs.biome = (uint8_t)biome;
    memset(hs.mob, 0, sizeof hs.mob);
    memset(hs.shot, 0, sizeof hs.shot);
    memset(hs.item, 0, sizeof hs.item);
    memset(hs.thing, 0, sizeof hs.thing);
    memset(hs.fxs, 0, sizeof hs.fxs);
    hs.boss = -1;
    hs.shut = 0;
    hs.shot_seen = 0;
    hs.px = px;
    hs.py = py;
    hs.yo_t = 0;
    hs.knock_t = 0;
    hs.muzzle_t = 0;
    if (sv.on_trip) sv.room = (uint8_t)room;
    if (hs.area != AR_BASE) {
        HsSpawn sp[HS_MAX_SPAWN];
        int n = hs_room_spawns(room, sp, HS_MAX_SPAWN);
        for (int i = 0; i < n; i++) {
            if (sv.dead[room] >> i & 1) continue;
            int m = hs_add_mob(sp[i].kind, sp[i].tx, sp[i].ty, i);
            if (m >= 0 && sp[i].kind == E_MAWBO) hs.mob[m].hp = sv.mawbo_hp;
        }
        if (hs_mawbo_here()) {
            int m = hs_add_mob(E_MAWBO, 10, 4, 0xFF);
            if (m >= 0) hs.mob[m].hp = (int16_t)(sv.mawbo_hp > 0 ? sv.mawbo_hp : HS_FOE[E_MAWBO].hp);
        }
        HsThingSpawn ts[HS_MAX_THINGS];
        int nt = hs_room_things(room, ts, HS_MAX_THINGS);
        for (int i = 0; i < nt && i < 15; i++) {
            HsThing *t = &hs.thing[i];
            *t = (HsThing){1, ts[i].kind, ts[i].arg, (uint8_t)i, ts[i].tx, ts[i].ty, 0};
            bool used = (sv.opened[room] >> i & 1) != 0;
            if (t->kind == TH_POT && used) t->on = 0;
            if (t->kind == TH_CHEST && used) t->open = 1;
            if (t->kind == TH_CACHE && used) t->open = 1;
            if (t->kind == TH_GEAR && (sv.lab_fixed || (sv.carry & 8))) t->on = 0;
            if (t->kind == TH_PART && ((sv.parts | sv.carry) >> t->arg & 1)) t->on = 0;
        }
    }
    hs.events |= 0;
}

/* ---- trips --------------------------------------------------------------------------------- */

static int random_screen(void) {
    for (;;) {
        int s = rng_range(&lr, 0, HS_SCREENS - 1);
        if (s != hw.lair && s != HS_START_SCREEN && s != hs.room && s != sv.mawbo_room) return s;
    }
}

void hs_start_trip(int room, int32_t px, int32_t py) {
    sv.excursion++;
    sv.trips++;
    rng_seed(&lr, ((uint64_t)hw.seed << 20) ^ sv.excursion * 2654435761u);
    sv.on_trip = 1;
    sv.time_f = sv.res[RES_JERKY] * hs_jerky_seconds() * 60;
    sv.res[RES_JERKY] = 0;
    memset(sv.gain, 0, sizeof sv.gain);
    memset(sv.dead, 0, sizeof sv.dead);
    memset(sv.opened, 0, sizeof sv.opened);
    sv.carry = 0;
    sv.volt_down = 0;
    sv.mawbo_wins = 0;
    sv.mawbo_hp = (int16_t)HS_FOE[E_MAWBO].hp;
    hs.room = -1;
    sv.mawbo_room = (uint8_t)random_screen();
    hs_sim_enter(room, px, py);
}

static void end_trip(void) {
    sv.on_trip = 0;
    memset(sv.gain, 0, sizeof sv.gain);
    memset(sv.dead, 0, sizeof sv.dead);
    memset(sv.opened, 0, sizeof sv.opened);
    sv.carry = 0;
    sv.volt_down = 0;
    sv.mawbo_wins = 0;
    sv.time_f = 0;
}

void hs_bank_trip(void) {
    for (int r = 0; r < RES_COUNT; r++) hs_add(r, sv.gain[r]);
    sv.parts |= (uint8_t)(sv.carry & 7);
    if (sv.carry & 8) sv.lab_fixed = 1;
    if (sv.carry & 16) sv.idol = 1;
    end_trip();
}

void hs_lose_trip(void) {
    sv.fadeouts++;
    sv.odds = 0;
    end_trip();
}

/* ---- Wick is hit ------------------------------------------------------------------------------ */

static void hurt(int base_s, int32_t fromx, int32_t fromy) {
    if (hs.inv > 0 || !sv.on_trip) return;
    int s = hs_hit_cost_s(base_s);
    sv.time_f -= s * 60;
    sv.hits++;
    hs.last_hit_s = s;
    hs.inv = HIT_INV;
    int dx = PX(hs.px - fromx), dy = PX(hs.py - fromy);
    int d = imax(1, iabs(dx) + iabs(dy));
    hs.kx = dx * 48 / d;
    hs.ky = dy * 48 / d;
    hs.knock_t = 8;
    hs.events |= EV_HURT;
    sfx_play_name("hs_ouch");
}

/* ---- striking foes ------------------------------------------------------------------------------ */

static bool mob_hittable(const HsMob *m) {
    if (!m->on) return false;
    if ((m->kind == E_GRUBLET || m->kind == E_SPLOSH) && !m->up) return false;
    if (m->kind == E_MAWBO && m->st == 9) return false; /* vanishing */
    return true;
}

static bool mob_harmful(const HsMob *m) {
    if (!m->on) return false;
    if ((m->kind == E_GRUBLET || m->kind == E_SPLOSH) && !m->up) return false;
    if (m->kind == E_MAWBO && m->st == 9) return false;
    return true;
}

static void open_shutters_if_clear(void) {
    if (hs.area != AR_DUN) return;
    for (int i = 0; i < HS_MAX_MOBS; i++)
        if (hs.mob[i].on && HS_FOE[hs.mob[i].kind].tier == 3) return;
    set_shutters(false);
}

static void kill_mob(int i) {
    HsMob *m = &hs.mob[i];
    int kind = m->kind, tier = HS_FOE[kind].tier;
    m->on = 0;
    if (m->spawn != 0xFF) sv.dead[hs.room] |= (uint16_t)(1 << m->spawn);
    fx_add(tier == 3 ? FX_BOOM : FX_PUFF, PX(m->x), PX(m->y));
    sfx_play_name(tier == 3 ? "hs_bossdown" : "hs_pop");
    if (tier < 3) { loot(tier, m->x, m->y); return; }
    if (hs.boss == i) hs.boss = -1;
    switch (kind) {
    case E_MAWBO:
        sv.mawbo_wins++;
        sv.mawbo_hp = (int16_t)HS_FOE[E_MAWBO].hp;
        for (int k = 0; k < HS_MAX_MOBS; k++) /* its brood goes with it */
            if (hs.mob[k].on && hs.mob[k].spawn == 0xFF) hs.mob[k].on = 0;
        if (sv.mawbo_wins >= 6) {
            hs_drop(IT_IDOL, 1, m->x, m->y);
            hs_say("MAWBO IS BEATEN FOR GOOD! IT LEFT SOMETHING.");
        } else {
            sv.mawbo_room = (uint8_t)random_screen();
            char b[64];
            snprintf(b, sizeof b, "MAWBO FLEES! (%d OF 6)", sv.mawbo_wins);
            hs_say(b);
            hs.events |= EV_MAWBO_FLED;
        }
        break;
    case E_VOLTHOG:
        sv.volt_down = 1;
        hs_drop(IT_BIGTONIC, 1, m->x, m->y);
        hs_room_tiles(hs.room, hs.tile, NULL); /* the gate to the lair opens */
        hs_say("THE VOLTHOG FIZZLES OUT. A GATE GRINDS OPEN.");
        break;
    case E_LOOM:
        sv.loom_home = 1;
        hs_drop(IT_BIGTONIC, 1, m->x, m->y);
        hs_say("MOTHER LOOM YIELDS. SHE'LL SPIN THREAD AT CAMP.");
        hs.events |= EV_LOOM;
        break;
    default:
        hs_drop(IT_BIGTONIC, 1, m->x, m->y);
        hs_say("THE GUARDIAN FALLS. THE SHIP PART IS FREE!");
        break;
    }
    for (int k = 0; k < HS_MAX_MOBS; k++) /* minions melt away */
        if (hs.mob[k].on && hs.mob[k].spawn == 0xFF && hs.mob[k].kind != E_MAWBO && kind != E_MAWBO) hs.mob[k].on = 0;
    hs.events |= EV_BOSSDOWN;
    open_shutters_if_clear();
}

void hs_kill_all(void) {
    for (int i = 0; i < HS_MAX_MOBS; i++)
        if (hs.mob[i].on) kill_mob(i);
}

static void strike(int i, int dmg, int stun) {
    HsMob *m = &hs.mob[i];
    m->hp = (int16_t)(m->hp - dmg);
    m->flash = 8;
    if (HS_FOE[m->kind].tier < 3) m->stun = (int16_t)imax(m->stun, stun);
    else m->stun = (int16_t)imax(m->stun, stun / 3);
    sfx_play_name("hs_hit");
    if (m->kind == E_MAWBO) sv.mawbo_hp = (int16_t)imax(0, m->hp);
    if (m->hp <= 0) kill_mob(i);
}

static bool overlap(int ax, int ay, int ah, int bx, int by, int bh) {
    return iabs(ax - bx) < ah + bh && iabs(ay - by) < ah + bh;
}

static void break_pot(HsThing *t) {
    t->on = 0;
    sv.opened[hs.room] |= (uint16_t)(1 << t->idx);
    int32_t x = SUB(t->tx * HS_T + 8), y = SUB(t->ty * HS_T + 8);
    fx_add(FX_PUFF, PX(x), PX(y));
    sfx_play_name("hs_pot");
    int r = rng_range(&lr, 0, 99);
    if (r < 50) hs_drop(IT_GLINT, 3, x, y);
    else if (r < 80) hs_drop(IT_ODD, 1, x, y);
}

/* a weapon's head at (x, y) pixels with half-size h: pots, cracks, the Glowstone */
static void weapon_world(int x, int y, int h, bool *stone_once) {
    for (int i = 0; i < HS_MAX_THINGS; i++) {
        HsThing *t = &hs.thing[i];
        if (!t->on || t->kind != TH_POT) continue;
        if (overlap(x, y, h, t->tx * HS_T + 8, t->ty * HS_T + 8, 6)) break_pot(t);
    }
    int tt = tile_at_px(x, y);
    if (tt == T_CRACK) {
        for (int ty = 0; ty < HS_TH; ty++)
            for (int tx = 0; tx < HS_TW; tx++)
                if (hs.tile[ty][tx] == T_CRACK) { hs.tile[ty][tx] = T_FLOOR; fx_add(FX_PUFF, tx * HS_T + 8, ty * HS_T + 8); }
        sv.opened[hs.room] |= 0x8000;
        sfx_play_name("hs_crack");
    }
    if (hs.area == AR_BASE && stone_once && !*stone_once) {
        const HsSpot *s = &HS_SPOT[P_STONE];
        if (x + h > s->x && x - h < s->x + s->w && y + h > s->y && y - h < s->y + s->h) {
            *stone_once = true;
            int n = hs_yoyo_dmg();
            int32_t cx = SUB(s->x + s->w / 2), cy = SUB(s->y + s->h + 8);
            if (rng_range(&lr, 0, 299) == 0) hs_drop(IT_BAR, 1, cx, cy); /* 1 in 300: a bar */
            else hs_drop(IT_GLINT, n, cx, cy);
            fx_add(FX_SPARK, x, y);
            sfx_play_name("hs_stone");
        }
    }
}

/* ---- Wick's weapons ------------------------------------------------------------------------------ */

static void yoyo_update(void) {
    if (hs.yo_t <= 0) return;
    hs.yo_t++;
    int t = hs.yo_t;
    int reach = t <= YOYO_OUT ? t * YOYO_REACH / YOYO_OUT : (2 * YOYO_OUT - t) * YOYO_REACH / YOYO_OUT;
    if (reach < 0) reach = 0;
    int fx = hs.dirx, fy = hs.diry;
    int k = (fx && fy) ? 181 : 256; /* diagonals: 1/sqrt 2 */
    int x = wick_x() + fx * reach * k / 256, y = wick_y() + fy * reach * k / 256;
    hs.yo_x = SUB(x);
    hs.yo_y = SUB(y);
    /* a wall turns it round */
    if (t <= YOYO_OUT && tile_solid(tile_at_px(x, y))) {
        weapon_world(x, y, 3, (bool *)&hs.yo_stone);
        hs.yo_t = 2 * YOYO_OUT - t;
    }
    bool stone = hs.yo_stone != 0;
    weapon_world(x, y, 4, &stone);
    hs.yo_stone = stone;
    for (int i = 0; i < HS_MAX_MOBS; i++) {
        HsMob *m = &hs.mob[i];
        if (!mob_hittable(m) || (hs.yo_hit >> i & 1)) continue;
        if (overlap(x, y, 4, PX(m->x), PX(m->y), HS_FOE[m->kind].size / 2)) {
            hs.yo_hit |= 1u << i;
            strike(i, hs_yoyo_dmg(), 12);
        }
    }
    if (hs.yo_t >= 2 * YOYO_OUT) hs.yo_t = 0;
}

static void pipe_update(void) {
    if (hs.muzzle_t <= 0) return;
    hs.muzzle_t--;
    int x = wick_x() + hs.dirx * 11, y = wick_y() + hs.diry * 11;
    weapon_world(x, y, 7, NULL);
    for (int i = 0; i < HS_MAX_MOBS; i++) {
        HsMob *m = &hs.mob[i];
        if (!mob_hittable(m) || (hs.bump_hit >> i & 1)) continue;
        if (overlap(x, y, 7, PX(m->x), PX(m->y), HS_FOE[m->kind].size / 2)) {
            hs.bump_hit |= 1u << i;
            strike(i, 1, 20);
        }
    }
}

static void shot_add(int kind, bool mine, int32_t x, int32_t y, int32_t vx, int32_t vy, int life) {
    hs.shot_seen |= 1u << kind;
    for (int i = 0; i < HS_MAX_SHOTS; i++)
        if (!hs.shot[i].on) {
            hs.shot[i] = (HsShot){1, (uint8_t)kind, (uint8_t)mine, x, y, vx, vy, (int16_t)life, 0};
            return;
        }
}

static void attack(void) {
    if (sv.pipe && sv.weapon == 1) {
        if (hs.shoot_cool > 0) return;
        hs.shoot_cool = PIPE_COOL;
        hs.muzzle_t = 6;
        hs.bump_hit = 0;
        if (hs_spend_glint()) {
            int k = (hs.dirx && hs.diry) ? 181 : 256;
            shot_add(SH_SLUG, true, hs.px + SUB(hs.dirx * 8), hs.py + SUB(hs.diry * 8), hs.dirx * PIPE_SPEED * k / 256,
                     hs.diry * PIPE_SPEED * k / 256, 48);
            sfx_play_name("hs_pipe");
        } else {
            sfx_play_name("hs_click");
        }
        return;
    }
    if (hs.yo_t > 0) return;
    hs.yo_t = 1;
    hs.yo_hit = 0;
    hs.yo_stone = 0;
    sfx_play_name("hs_yoyo");
}

/* ---- foes ------------------------------------------------------------------------------------ */

static void aim(const HsMob *m, int speed, int32_t *vx, int32_t *vy) {
    float dx = (float)(hs.px - m->x), dy = (float)(hs.py - m->y);
    float d = sqrtf(dx * dx + dy * dy);
    if (d < 1) d = 1;
    *vx = (int32_t)(dx / d * (float)speed);
    *vy = (int32_t)(dy / d * (float)speed);
}

static void fire_at(const HsMob *m, int kind, int speed, float spread, int life) {
    float a = atan2f((float)(hs.py - m->y), (float)(hs.px - m->x)) + spread;
    shot_add(kind, false, m->x, m->y, (int32_t)(cosf(a) * (float)speed), (int32_t)(sinf(a) * (float)speed), life);
}

static void ring(const HsMob *m, int kind, int n, int speed, int life, float off) {
    for (int k = 0; k < n; k++) {
        float a = (float)k * 6.2831853f / (float)n + off;
        shot_add(kind, false, m->x, m->y, (int32_t)(cosf(a) * (float)speed), (int32_t)(sinf(a) * (float)speed), life);
    }
}

static void lob(const HsMob *m) {
    /* a bomb flies to where Wick stands and sits a moment before it bursts */
    int32_t vx = (hs.px - m->x) / 30, vy = (hs.py - m->y) / 30;
    hs.shot_seen |= 1u << SH_BOMB;
    for (int i = 0; i < HS_MAX_SHOTS; i++)
        if (!hs.shot[i].on) {
            hs.shot[i] = (HsShot){1, SH_BOMB, 0, m->x, m->y, vx, vy, 30, 36};
            return;
        }
}

static int minions(int kind) {
    int n = 0;
    for (int i = 0; i < HS_MAX_MOBS; i++) n += hs.mob[i].on && hs.mob[i].kind == kind && hs.mob[i].spawn == 0xFF;
    return n;
}

static void summon(const HsMob *m, int kind, int max) {
    if (minions(kind) >= max) return;
    int tx = iclamp(PX(m->x) / HS_T + rng_range(&lr, -2, 2), 1, HS_TW - 2);
    int ty = iclamp(PX(m->y) / HS_T + rng_range(&lr, -1, 2), 1, HS_TH - 2);
    if (tile_solid(hs.tile[ty][tx])) return;
    int boss = hs.boss;
    int i = hs_add_mob(kind, tx, ty, 0xFF);
    hs.boss = boss;
    if (i >= 0) { fx_add(FX_PUFF, tx * HS_T + 8, ty * HS_T + 8); if (kind == E_GRUBLET) hs.mob[i].up = 1; }
}

static void pick_dir8(HsMob *m) {
    m->dx = (int8_t)rng_range(&lr, -1, 1);
    m->dy = (int8_t)rng_range(&lr, -1, 1);
    if (!m->dx && !m->dy) m->dx = 1;
}

static void pick_dir4(HsMob *m) {
    int d = rng_range(&lr, 0, 3);
    m->dx = (int8_t)HS_DX[d];
    m->dy = (int8_t)HS_DY[d];
}

static void toward4(HsMob *m) {
    int dx = PX(hs.px - m->x), dy = PX(hs.py - m->y);
    if (iabs(dx) > iabs(dy)) { m->dx = (int8_t)isign(dx); m->dy = 0; }
    else { m->dx = 0; m->dy = (int8_t)isign(dy); }
}

static int dist_px(const HsMob *m) {
    int dx = PX(hs.px - m->x), dy = PX(hs.py - m->y);
    return (int)sqrtf((float)(dx * dx + dy * dy));
}

/* fliers keep inside the walls of the field */
static void fly(HsMob *m, int32_t vx, int32_t vy) {
    m->x = iclamp(m->x + vx, SUB(20), SUB(HS_FW - 20));
    m->y = iclamp(m->y + vy, SUB(20), SUB(HS_FH - 20));
}

/* A leap or a dive is coming: the boss shakes for a third of a second first. */
bool hs_mob_tell(const HsMob *m) {
    if (!m->on || m->st != 0) return false;
    int phase = m->kind == E_MAWBO ? (m->t / 360) % 5 : -1;
    int t = m->kind == E_MAWBO ? m->t / 2 : m->t; /* Mawbo's borrowed moves run at half pace */
    if (m->kind == E_GRUMMKING || phase == 0) return t % 160 >= 60 && t % 160 < 80;
    if (m->kind == E_ZIZZIK || phase == 2) return t % 150 >= 80 && t % 150 < 100;
    return false;
}

/* A boss's moves. `own` is false when Mawbo borrows them: then it keeps its
 * own slow walk and summons only its worms, but leaps, dives and fires as
 * the owner does. */
static void boss_pattern(HsMob *m, int which, bool own) {
    /* borrowed moves come half as often: Mawbo has all of them to get through */
    int t = own ? m->t : m->t / 2;
    bool tick = own || (m->t & 1) == 0; /* each value of t happens once */
    if (!tick) return; /* (its leaps and dives go at half speed too) */
    switch (which) {
    case 0: /* the Grumm King: a leap, a slam of rocks */
        if (m->st == 0) {
            if (own) {
                int32_t vx, vy;
                aim(m, 10, &vx, &vy);
                move_body(&m->x, &m->y, vx, vy, 10, true);
            }
            if (t % 160 == 80) { m->st = 1; m->cool2 = 30; m->tx = (hs.px - m->x) / 30; m->ty = (hs.py - m->y) / 30; sfx_play_name("hs_leap"); }
        } else {
            move_body(&m->x, &m->y, m->tx, m->ty, 10, false);
            if (--m->cool2 <= 0) { m->st = 0; ring(m, SH_ROCK, 8, 28, 60, 0.2f); fx_add(FX_BOOM, PX(m->x), PX(m->y) + 8); sfx_play_name("hs_slam"); }
        }
        if (t % 400 == 399 && own) summon(m, E_GRUMMLE, 2);
        break;
    case 1: /* the Thornmother: fans and rings of seeds */
        if (t % 90 == 45) {
            if ((t / 90) % 4 == 3) ring(m, SH_SEED, 12, 22, 120, 0);
            else for (int k = -2; k <= 2; k++) fire_at(m, SH_SEED, 26, (float)k * 0.26f, 120);
            sfx_play_name("hs_spit");
        }
        if (t % 360 == 300 && own) summon(m, E_SPITBLOOM, 2);
        break;
    case 2: /* Zizzik: circles, dives */
        if (m->st == 0) {
            /* its own: a slow, weaving flight after Wick over walls and pits;
             * every two and a half seconds it shakes and dives */
            if (own) {
                int32_t vx, vy;
                aim(m, 12, &vx, &vy);
                vx += (int32_t)(sinf((float)t * 0.05f) * 10.0f);
                vy += (int32_t)(cosf((float)t * 0.04f) * 8.0f);
                fly(m, vx, vy);
            }
            if (t % 150 == 100) { m->st = 1; m->cool2 = 40; aim(m, 44, &m->vx, &m->vy); sfx_play_name("hs_dive"); }
        } else {
            if (own) fly(m, m->vx, m->vy);
            else move_body(&m->x, &m->y, m->vx, m->vy, 10, false);
            if (--m->cool2 <= 0) m->st = 0;
        }
        if (t % 420 == 150 && own) summon(m, E_GNATTER, 2);
        break;
    case 3: /* the Volthog: bolts and bombs */
        if (m->st == 0 && own) {
            if (m->dx == 0) m->dx = 1;
            int b = move_body(&m->x, &m->y, m->dx * 14, (SUB(56) - m->y) / 16, 12, false);
            if ((b & 1) || PX(m->x) < 40 || PX(m->x) > HS_FW - 40) m->dx = (int8_t)-m->dx;
        }
        if (t % 70 == 35) { fire_at(m, SH_BOLT, 56, 0, 70); sfx_play_name("hs_zap"); }
        if (t % 160 == 120) lob(m);
        break;
    case 4: /* Mother Loom: webs, and her brood */
        if (own) {
            if (m->dx == 0) m->dx = 1;
            int b = move_body(&m->x, &m->y, m->dx * 14, (SUB(44) - m->y) / 16, 12, false);
            if ((b & 1) || PX(m->x) < 40 || PX(m->x) > HS_FW - 40) m->dx = (int8_t)-m->dx;
        }
        if (t % 90 == 60) { for (int k = -1; k <= 1; k++) fire_at(m, SH_WEB, 30, (float)k * 0.3f, 100); sfx_play_name("hs_spit"); }
        if (t % 220 == 200 && own) summon(m, E_SKITTER, 3);
        break;
    default: break;
    }
}

static void mob_update(int i) {
    HsMob *m = &hs.mob[i];
    const HsFoeDef *d = &HS_FOE[m->kind];
    if (m->flash > 0) m->flash--;
    m->t++;
    if (m->stun > 0) { m->stun--; return; }
    int sp = d->speed, dist = dist_px(m);
    int32_t vx = 0, vy = 0;
    switch (m->kind) {
    case E_SPINNER:
        if (m->t % 30 == 0) pick_dir8(m);
        if (move_body(&m->x, &m->y, m->dx * sp, m->dy * sp, 5, false)) pick_dir8(m);
        break;
    case E_GRUBLET:
        if (!m->up) {
            aim(m, sp, &vx, &vy);
            move_body(&m->x, &m->y, vx, vy, 5, true);
            if (m->t > 90) { m->up = 1; m->t = 0; }
        } else if (m->t > 80) { m->up = 0; m->t = 0; }
        break;
    case E_SPLOSH:
        if (!m->up && m->t > 110) { m->up = 1; m->t = 0; }
        if (m->up && m->t == 14) fire_at(m, SH_PELLET, 30, 0, 90);
        if (m->up && m->t > 60) { m->up = 0; m->t = 0; }
        break;
    case E_SKITTER: case E_BROODER:
        if (m->st == 0 && m->t > 40) { m->st = 1; m->t = 0; aim(m, sp, &m->vx, &m->vy); fx_add(FX_HOP, PX(m->x), PX(m->y)); }
        if (m->st == 1) { move_body(&m->x, &m->y, m->vx, m->vy, 6, false); if (m->t > 16) { m->st = 0; m->t = 0; } }
        if (m->kind == E_BROODER && --m->cool <= 0) { m->cool = 110; for (int k = -1; k <= 1; k++) fire_at(m, SH_WEB, 26, (float)k * 0.3f, 90); }
        break;
    case E_STINGLE:
        if (m->st == 0) {
            if (m->t % 60 == 0) pick_dir4(m);
            if (move_body(&m->x, &m->y, m->dx * sp, m->dy * sp, 6, false)) pick_dir4(m);
            int dx = PX(hs.px - m->x), dy = PX(hs.py - m->y);
            if (dist < 140 && (iabs(dx) < 8 || iabs(dy) < 8)) { m->st = 1; toward4(m); m->t = 0; sfx_play_name("hs_dive"); }
        } else if (m->st == 1) {
            if (move_body(&m->x, &m->y, m->dx * 44, m->dy * 44, 6, false) || m->t > 50) { m->st = 2; m->t = 0; }
        } else if (m->t > 30) { m->st = 0; m->t = 0; }
        break;
    case E_GNATTER: {
        aim(m, sp, &vx, &vy);
        vx += (int32_t)(sinf((float)m->t * 0.12f) * 14.0f);
        vy += (int32_t)(cosf((float)m->t * 0.09f) * 14.0f);
        fly(m, vx, vy);
        break;
    }
    case E_SPITBLOOM:
        if (--m->cool <= 0) {
            m->cool = 100;
            if ((m->t / 100) % 2) fire_at(m, SH_SEED, 30, 0, 100);
            else for (int k = 0; k < 4; k++) shot_add(SH_SEED, false, m->x, m->y, HS_DX[k] * 28, HS_DY[k] * 28, 100);
        }
        break;
    case E_GRUMMLE: case E_OOZER:
        aim(m, sp, &vx, &vy);
        move_body(&m->x, &m->y, vx, vy, 7, true);
        if (m->kind == E_OOZER && --m->cool <= 0) { m->cool = 180; summon(m, E_DRIPLET, 3); }
        break;
    case E_LOBBER:
        if (dist < 64) aim(m, -sp, &vx, &vy);
        else if (dist > 110) aim(m, sp, &vx, &vy);
        move_body(&m->x, &m->y, vx, vy, 6, true);
        if (--m->cool <= 0) { m->cool = 120; lob(m); }
        break;
    case E_SHELLBACK: case E_TREADLE:
        if (m->dx == 0 && m->dy == 0) pick_dir4(m);
        if (m->dx != 0 && m->dy != 0) m->dy = 0;
        if (move_body(&m->x, &m->y, m->dx * sp, m->dy * sp, 7, false)) {
            if (rng_chance(&lr, 50)) toward4(m); else pick_dir4(m);
        }
        if (m->kind == E_TREADLE) {
            int dx = PX(hs.px - m->x), dy = PX(hs.py - m->y);
            if (--m->cool <= 0 && (iabs(dx) < 8 || iabs(dy) < 8)) {
                m->cool = 90;
                toward4(m);
                shot_add(SH_SHELL, false, m->x, m->y, m->dx * 40, m->dy * 40, 90);
                sfx_play_name("hs_boom");
            }
        }
        break;
    case E_DRIPLET:
        if (m->st == 0 && m->t > 40) { m->st = 1; m->t = 0; pick_dir8(m); }
        if (m->st == 1) { move_body(&m->x, &m->y, m->dx * sp, m->dy * sp, 5, false); if (m->t > 12) { m->st = 0; m->t = 0; } }
        break;
    case E_CLACKER: case E_REDCLACKER: {
        int lunge = m->kind == E_CLACKER ? 40 : 56;
        int dx = PX(hs.px - m->x), dy = PX(hs.py - m->y);
        if (m->st == 0) {
            move_body(&m->x, &m->y, isign(dx) * sp, 0, 7, false);
            if (iabs(dx) < 10 && iabs(dy) < 80) { m->st = 1; m->t = 0; m->dy = (int8_t)isign(dy); }
        } else if (m->st == 1) {
            if (move_body(&m->x, &m->y, 0, m->dy * lunge, 7, false) || m->t > 14) { m->st = 2; m->t = 0; }
        } else {
            move_body(&m->x, &m->y, 0, -m->dy * sp, 7, false);
            if (m->t > 20) { m->st = 0; m->t = 0; }
        }
        break;
    }
    case E_PUFFCAP:
        if (--m->cool <= 0) { m->cool = 150; ring(m, SH_SPORE, 8, 20, 80, (float)(m->t % 2) * 0.39f); sfx_play_name("hs_puff"); }
        break;
    case E_ZIPSNAKE:
        if (m->st == 0) {
            if (m->t > 40) { m->st = 1; m->t = 0; toward4(m); }
        } else if (move_body(&m->x, &m->y, m->dx * sp, m->dy * sp, 6, false) || m->t > 30) { m->st = 0; m->t = 0; }
        break;
    case E_LEECHLING:
        if (m->st == 0) {
            if (m->t % 50 == 0) pick_dir8(m);
            move_body(&m->x, &m->y, m->dx * sp / 2, m->dy * sp / 2, 5, false);
            if (dist < 80 && --m->cool <= 0) { m->st = 1; m->t = 0; aim(m, 44, &m->vx, &m->vy); m->cool = 60; fx_add(FX_HOP, PX(m->x), PX(m->y)); }
        } else {
            move_body(&m->x, &m->y, m->vx, m->vy, 5, false);
            if (m->t > 22) { m->st = 0; m->t = 1; }
        }
        break;
    case E_GRUMMKING: boss_pattern(m, 0, true); break;
    case E_THORNMOTHER: boss_pattern(m, 1, true); break;
    case E_ZIZZIK: boss_pattern(m, 2, true); break;
    case E_VOLTHOG: boss_pattern(m, 3, true); break;
    case E_LOOM: boss_pattern(m, 4, true); break;
    case E_MAWBO: {
        /* it walks after Wick, and every six seconds takes up another
         * guardian's attacks: leap and rocks, seeds, a dive, bolts and bombs,
         * webs; its own are worms and four-way pellets */
        int phase = (m->t / 360) % 5;
        if (m->st == 0) {
            aim(m, sp, &vx, &vy);
            move_body(&m->x, &m->y, vx, vy, 10, true);
        }
        boss_pattern(m, phase, false);
        if (m->t % 300 == 200) summon(m, E_GRUBLET, 2);
        if (m->t % 300 == 50) for (int k = 0; k < 4; k++) shot_add(SH_PELLET, false, m->x, m->y, HS_DX[k] * 30, HS_DY[k] * 30, 100);
        break;
    }
    default: break;
    }
    if (mob_harmful(m) && overlap(wick_x(), wick_y(), WICK_HALF, PX(m->x), PX(m->y), d->size / 2 - 1)) {
        if (hs.inv == 0) hs_hurt_by[m->kind == E_MAWBO ? 15 : 14]++;
        hurt(d->hit_s, m->x, m->y);
    }
}

static void shots_update(void) {
    for (int i = 0; i < HS_MAX_SHOTS; i++) {
        HsShot *s = &hs.shot[i];
        if (!s->on) continue;
        if (s->kind == SH_BLAST) {
            if (overlap(wick_x(), wick_y(), WICK_HALF, PX(s->x), PX(s->y), 14)) {
                if (hs.inv == 0) hs_hurt_by[SH_BLAST]++;
                hurt(30, s->x, s->y);
            }
            if (--s->life <= 0) s->on = 0;
            continue;
        }
        if (s->kind == SH_BOMB) {
            if (s->life > 0) { s->x += s->vx; s->y += s->vy; s->life--; }
            else if (--s->fuse <= 0) {
                s->kind = SH_BLAST;
                s->life = 12;
                fx_add(FX_BOOM, PX(s->x), PX(s->y));
                sfx_play_name("hs_boom");
            }
            continue;
        }
        s->x += s->vx;
        s->y += s->vy;
        int x = PX(s->x), y = PX(s->y);
        if (--s->life <= 0 || x < 0 || y < 0 || x >= HS_FW || y >= HS_FH || tile_solid(tile_at_px(x, y))) {
            if (s->mine) weapon_world(x, y, 3, NULL);
            s->on = 0;
            continue;
        }
        if (s->mine) {
            weapon_world(x, y, 3, NULL);
            for (int k = 0; k < HS_MAX_MOBS; k++) {
                HsMob *m = &hs.mob[k];
                if (!mob_hittable(m)) continue;
                if (overlap(x, y, 3, PX(m->x), PX(m->y), HS_FOE[m->kind].size / 2)) {
                    strike(k, 1, 30);
                    s->on = 0;
                    break;
                }
            }
        } else if (overlap(wick_x(), wick_y(), WICK_HALF, x, y, 3)) {
            if (hs.inv == 0) hs_hurt_by[s->kind]++;
            hurt(30, s->x, s->y);
            s->on = 0;
        }
    }
}

/* ---- things -------------------------------------------------------------------------------------- */

int hs_front_thing(void) {
    int x = wick_x() + hs.dirx * 12, y = wick_y() + hs.diry * 12;
    int best = -1, bd = 999;
    for (int i = 0; i < HS_MAX_THINGS; i++) {
        const HsThing *t = &hs.thing[i];
        if (!t->on) continue;
        int cx = t->tx * HS_T + 8, cy = t->ty * HS_T + 8;
        int d = iabs(cx - x) + iabs(cy - y);
        if (t->kind == TH_PAD) d = iabs(cx - wick_x()) + iabs(cy - wick_y()) - 6; /* on or beside a hopstone */
        if (d < 14 && d < bd) { bd = d; best = i; }
    }
    return best;
}

int hs_front_spot(void) {
    int x = wick_x() + hs.dirx * 10, y = wick_y() + hs.diry * 10;
    for (int p = 0; p < P_COUNT; p++) {
        const HsSpot *s = &HS_SPOT[p];
        if (!hs_spot_shown(p)) continue;
        int m = 3;
        if (p == P_PAD) {
            int cx = s->x + s->w / 2, cy = s->y + s->h / 2;
            if (iabs(wick_x() - cx) < 9 && iabs(wick_y() - cy) < 9) return p;
            continue;
        }
        if (x >= s->x - m && x < s->x + s->w + m && y >= s->y - m && y < s->y + s->h + m) return p;
    }
    return -1;
}

static void things_touch(void) {
    for (int i = 0; i < HS_MAX_THINGS; i++) {
        HsThing *t = &hs.thing[i];
        if (!t->on) continue;
        int cx = t->tx * HS_T + 8, cy = t->ty * HS_T + 8;
        if (iabs(cx - wick_x()) > 10 || iabs(cy - wick_y()) > 10) continue;
        if (t->kind == TH_GEAR) {
            t->on = 0;
            sv.carry |= 8;
            hs_say("A GEAR! THE THINKER AT CAMP NEEDS ONE.");
            sfx_play_name("hs_part");
        } else if (t->kind == TH_PART && hs.boss < 0) {
            t->on = 0;
            sv.carry |= (uint8_t)(1 << t->arg);
            hs_say("A SHIP PART! GET IT BACK TO THE TUMBLEWEED.");
            sfx_play_name("hs_part");
        }
    }
}

/* open a chest (A in front of it) */
void hs_open_chest(int i) {
    HsThing *t = &hs.thing[i];
    if (!t->on || t->open) return;
    t->open = 1;
    sv.opened[hs.room] |= (uint16_t)(1 << t->idx);
    int32_t x = SUB(t->tx * HS_T + 8), y = SUB(t->ty * HS_T + 18);
    sfx_play_name("hs_chest");
    if (t->kind == TH_CACHE) { hs_drop(IT_JERKY, 1, x, y); hs_drop(IT_JERKY, 1, x, y); return; }
    switch (t->arg) {
    case 0:
        hs_drop(IT_ODD, rng_range(&lr, 2, 4), x, y);
        if (rng_chance(&lr, 30)) hs_drop(IT_BAR, 1, x, y);
        break;
    case 1:
        hs_drop(IT_ODD, rng_range(&lr, 3, 6), x, y);
        hs_drop(IT_BAR, rng_range(&lr, 1, 2), x, y);
        break;
    default: /* Kit's */
        hs_drop(IT_ODD, 5, x, y);
        hs_drop(IT_BAR, 3, x, y);
        break;
    }
}

/* ---- ways between rooms ------------------------------------------------------------------------ */

static void go(int room, int x, int y) {
    hs_sim_enter(room, SUB(x), SUB(y));
    hs.inv = imax(hs.inv, 10);
}

static void edges(void) {
    int x = wick_x(), y = wick_y();
    int dir = -1;
    if (x < 3) dir = D_LEFT;
    else if (x > HS_FW - 4) dir = D_RIGHT;
    else if (y < 3) dir = D_UP;
    else if (y > HS_FH - 4) dir = D_DOWN;
    /* a cave mouth: walking up into it */
    if (hs.area == AR_OVER && y < 14 && tile_at_px(x, y - 6) == T_CAVE) {
        int c = hw.cave_of[hs.room];
        if (c >= 0) {
            int kind = hw.cave_kind[c];
            if (kind <= K_DUN3) go(HS_R_DUN0 + kind * HS_DROOMS, HS_FW / 2, HS_FH - 12);
            else go(HS_R_CAVE0 + c, HS_FW / 2, HS_FH - 12);
            sfx_play_name("hs_door");
        }
        return;
    }
    if (dir < 0) return;
    if (hs.area == AR_BASE) {
        if (dir == D_UP) { hs.py = SUB(8); hs.events |= EV_GATE; }
        return;
    }
    if (hs.area == AR_OVER) {
        if (hs.room == HS_START_SCREEN && dir == D_DOWN) { hs.events |= EV_HOME; return; }
        int n = hs_neighbour(hs.room, dir);
        if (n < 0) return;
        int nx = dir == D_LEFT ? HS_FW - 6 : dir == D_RIGHT ? 5 : x;
        int ny = dir == D_UP ? HS_FH - 6 : dir == D_DOWN ? 5 : y;
        go(n, nx, ny);
        return;
    }
    /* caves and dungeon ways-in lead back out below the cave mouth */
    bool way_in = hs.area == AR_CAVE || (hs.area == AR_DUN && (hs.room - HS_R_DUN0) % HS_DROOMS == 0);
    if (dir == D_DOWN && way_in) {
        int c = hs.area == AR_CAVE ? hs.room - HS_R_CAVE0 : (hs.room - HS_R_DUN0) / HS_DROOMS;
        int s = hw.cave_screen[c];
        go(s, hw.cave_x[s] * HS_T + 8, HS_T + 12);
        hs.dirx = 0;
        hs.diry = 1;
        sfx_play_name("hs_door");
        return;
    }
    if (hs.area == AR_DUN) {
        int n = hs_dun_neighbour(hs.room, dir);
        if (n < 0) return;
        int nx = dir == D_LEFT ? HS_FW - 6 : dir == D_RIGHT ? 5 : x;
        int ny = dir == D_UP ? HS_FH - 6 : dir == D_DOWN ? 5 : y;
        /* the far side may be a crack not yet broken: then it stays shut */
        uint8_t t[HS_TH][HS_TW];
        hs_room_tiles(n, t, NULL);
        int tx = iclamp(nx / HS_T, 0, HS_TW - 1), ty = iclamp(ny / HS_T, 0, HS_TH - 1);
        if (tile_solid(t[ty][tx])) {
            hs.px = SUB(iclamp(x, 6, HS_FW - 7));
            hs.py = SUB(iclamp(y, 6, HS_FH - 7));
            return;
        }
        go(n, nx, ny);
    }
}

/* ---- the frame ---------------------------------------------------------------------------------- */

void hs_sim_update(uint32_t pad, uint32_t press) {
    hs.frame++;
    if (hs.msg_t > 0) hs.msg_t--;
    if (hs.inv > 0) hs.inv--;
    if (hs.shoot_cool > 0) hs.shoot_cool--;
    for (int i = 0; i < ARRAY_LEN(hs.fxs); i++)
        if (hs.fxs[i].on && ++hs.fxs[i].t > 20) hs.fxs[i].on = 0;
    if (sv.on_trip) {
        sv.time_f--;
        if (sv.time_f <= 0) { sv.time_f = 0; hs.events |= EV_FADED; return; }
    }
    /* Wick */
    if (hs.knock_t > 0) {
        hs.knock_t--;
        move_body(&hs.px, &hs.py, hs.kx, hs.ky, WICK_HALF, false);
        /* a knock never throws her out of the room */
        hs.px = iclamp(hs.px, SUB(8), SUB(HS_FW - 8));
        hs.py = iclamp(hs.py, SUB(8), SUB(HS_FH - 8));
    } else if (hs.yo_t == 0 && hs.muzzle_t == 0) {
        int dx = ((pad & BTN_RIGHT) != 0) - ((pad & BTN_LEFT) != 0);
        int dy = ((pad & BTN_DOWN) != 0) - ((pad & BTN_UP) != 0);
        if (dx || dy) {
            hs.dirx = dx;
            hs.diry = dy;
            int sp = (dx && dy) ? WICK_SPEED * 181 / 256 : WICK_SPEED;
            move_body(&hs.px, &hs.py, dx * sp, dy * sp, WICK_HALF, true);
            hs.walk_t++;
            if (hs.walk_t % 16 == 0) sfx_play_name("hs_step");
        } else {
            hs.walk_t = 0;
        }
    }
    if (press & BTN_B) attack();
    yoyo_update();
    pipe_update();
    /* the boss room shuts once Wick is inside */
    if (hs.area == AR_DUN && hs.boss >= 0 && !hs.shut) {
        int x = wick_x(), y = wick_y();
        if (x > 26 && x < HS_FW - 26 && y > 26 && y < HS_FH - 26) { set_shutters(true); sfx_play_name("hs_shut"); }
    }
    for (int i = 0; i < HS_MAX_MOBS; i++)
        if (hs.mob[i].on) mob_update(i);
    shots_update();
    /* pickups */
    for (int i = 0; i < HS_MAX_ITEMS; i++) {
        HsItem *it = &hs.item[i];
        if (!it->on) continue;
        it->t++;
        if (it->t > 8 && overlap(wick_x(), wick_y(), WICK_HALF + 1, PX(it->x), PX(it->y), 4)) take_item(it);
    }
    things_touch();
    edges();
}
