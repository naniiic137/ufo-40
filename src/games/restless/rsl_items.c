/* RESTLESS - torches, treasure, hidden spots, the bell and Grandmother
 * Ash, the weapon wheel and Blink the owlet. */
#include "rsl.h"

int rsl_main_h(void);
char rsl_hint(int tx, int ty);

const char *const RSL_WEAPON_NAME[WP_COUNT] = {"STAFF", "SEEKER", "EMBER", "SCATTER"};

int rsl_item_value(int kind) {
    switch (kind) {
    case IT_POT: return 100;
    case IT_URN: return 200;
    case IT_JAR: return 500;
    case IT_COIN: return 100;
    case IT_IDOL: return 1000;
    case IT_BEETLE: return 1000;
    default: return 0;
    }
}


int rsl_drop_item(int kind, int x, int y, int var) {
    for (int i = 0; i < RSL_MAX_ITEMS; i++)
        if (!rg.item[i].alive) {
            RslItem *it = &rg.item[i];
            memset(it, 0, sizeof *it);
            it->alive = 1;
            it->kind = (uint8_t)kind;
            it->var = (uint8_t)var;
            it->x = x;
            it->y = y;
            it->vy = -700;
            it->vx = rng_range(&rg.rng, -120, 120);
            it->value = rsl_item_value(kind);
            return i;
        }
    return -1;
}

static void drop_many(int kind, int n, int x, int y) {
    for (int k = 0; k < n; k++) {
        int i = rsl_drop_item(kind, x, y, 0);
        if (i >= 0) rg.item[i].vx = (k - (n - 1) / 2) * 140 + rng_range(&rg.rng, -40, 40);
    }
}

/* treasure, richer with every death */
static int treasure_kind(int roll_hi) {
    static const uint8_t T[5] = {IT_POT, IT_COIN, IT_URN, IT_JAR, IT_IDOL};
    int tier = rng_range(&rg.rng, 0, roll_hi) + (rg.deaths >= 2) + (rg.deaths >= 4) + (rg.deaths >= 6);
    return T[iclamp(tier, 0, 4)];
}

static void drop_treasure(int x, int y, int roll_hi) {
    int k = treasure_kind(roll_hi);
    if (k == IT_COIN) drop_many(IT_COIN, rng_range(&rg.rng, 2, 4), x, y);
    else rsl_drop_item(k, x, y, 0);
}

static void drop_wheel(int x, int y) {
    rg.wheel_id++;
    static const uint8_t W[3] = {WP_SEEKER, WP_EMBER, WP_SCATTER};
    for (int k = 0; k < 3; k++) {
        int i = rsl_drop_item(IT_WEAPON, x, y, W[k]);
        if (i < 0) continue;
        RslItem *it = &rg.item[i];
        it->cx = x;
        it->cy = y - 6 * RSL_FX;
        it->t = k * 63; /* a third of a turn apart */
        it->wheel = rg.wheel_id;
        it->vx = it->vy = 0;
        /* the weapon he already holds turns into a gold jar */
        if (W[k] == rg.pl.weapon) { it->kind = IT_JAR; it->value = rsl_item_value(IT_JAR); }
    }
    rsl_sfx("rsl_wheel");
}

void rsl_light_torch(int s) {
    RslSpawn *sp = &rg.spawn[s];
    if (sp->used) return;
    sp->used = 1;
    int x = (sp->tx * RSL_TILE + 8) * RSL_FX, y = (sp->ty * RSL_TILE + 4) * RSL_FX;
    rsl_burst(sp->tx * RSL_TILE + 8, sp->ty * RSL_TILE, C_ORANGE, 10, 140);
    rsl_sfx("rsl_torch");
    if (sp->a == TORCH_WHEEL) { drop_wheel(x, y); return; }
    if (sp->a == TORCH_EGG) { rsl_drop_item(IT_EGG, x, y, 0); return; }
    if (sp->a == TORCH_JAR) { rsl_drop_item(IT_JAR, x, y, 0); return; }
    if (sp->a == TORCH_URN) { rsl_drop_item(IT_URN, x, y, 0); return; }
    if (sp->a == TORCH_CLOCK) { rsl_drop_item(IT_CLOCK, x, y, 0); return; }
    if (sp->a == TORCH_BELL) { rsl_drop_item(IT_BELL, x, y, 0); return; }
    int r = rng_range(&rg.rng, 0, 99);
    if (r < 12) drop_wheel(x, y);
    else if (r < 22) rsl_drop_item(IT_CLOCK, x, y, 0);
    else if (r < 26) rsl_drop_item(IT_LILY, x, y, 0);
    else if (r < 29 && !rg.owl.on) rsl_drop_item(IT_EGG, x, y, 0);
    else drop_treasure(x, y, 2);
}

/* the order a half's hidden spots come in, left to right */
static int secret_ordinal(int s) {
    int n = 0;
    const RslSpawn *a = &rg.spawn[s];
    for (int i = 0; i < rg.nspawn; i++) {
        const RslSpawn *b = &rg.spawn[i];
        if (b->type != SP_SECRET || b->a || i == s) continue;
        if (b->tx < a->tx || (b->tx == a->tx && b->ty < a->ty)) n++;
    }
    return n;
}

void rsl_find_secret(int s) {
    RslSpawn *sp = &rg.spawn[s];
    if (sp->used) return;
    sp->used = 1;
    rg.secrets_found++;
    const RslHalf *H = &RSL_HALF[rg.half];
    int k = secret_ordinal(s);
    int x = (sp->tx * RSL_TILE + 8) * RSL_FX, y = (sp->ty * RSL_TILE + 8) * RSL_FX;
    rsl_sfx("rsl_secret");
    rsl_burst(sp->tx * RSL_TILE + 8, sp->ty * RSL_TILE + 8, C_YELLOW, 14, 150);
    if (k >= H->nsecrets) { rsl_drop_item(IT_JAR, x, y, 0); return; }
    drop_many(H->secrets[k].item, imax(1, H->secrets[k].count), x, y);
}

void rsl_kill_drop(int foe_kind, int x, int y, int flags) {
    int dl = rg.deaths;
    int chance = imin(30, 6 + dl * 3);
    bool upgraded = (flags & FF_GREEN) || (foe_kind == FO_GNAT && dl >= 3);
    if (foe_kind == FO_CLOCKFIRE || foe_kind == FO_LEAPER || foe_kind == FO_WISP) chance /= 2;
    if (upgraded) chance = imin(45, chance + 15);
    if (RSL_FOE_HP[foe_kind] >= 40) chance += 25;
    if (rng_range(&rg.rng, 0, 99) >= chance) return;
    drop_treasure(x, y, upgraded || RSL_FOE_HP[foe_kind] >= 40 ? 3 : 1);
}

/* every 5,000: the next kill's gift, fixed by the foe, the mark and the
 * deaths. The cells players have recorded are set by hand; the rest come
 * from a hash of the three. */
static int recorded_gift(int foe_kind, int mark, int deaths) {
    if (foe_kind == FO_TUMBLER && mark == 1 && deaths == 1) return IT_CLOCK;
    if (foe_kind == FO_GNAT && mark == 1 && deaths == 1) return IT_BEETLE;
    if (foe_kind == FO_BLOOM && mark == 2 && deaths <= 1) return deaths ? IT_EGG : IT_BELL;
    if (foe_kind == FO_TOAD && mark == 3 && deaths <= 1) return deaths ? IT_BELL : IT_CLOCK;
    if (foe_kind == FO_TOAD && mark == 4 && deaths <= 1) return deaths ? IT_EGG : IT_BELL;
    return IT_NONE;
}

void rsl_bonus_drop(int foe_kind, int x, int y) {
    static const uint8_t GIFT[8] = {IT_BEETLE, IT_CLOCK, IT_EGG, IT_BELL, IT_JAR, IT_LILY, IT_BEETLE, IT_CHARM};
    int mark = rg.bonus_next / RSL_BONUS_EVERY - 1 - rg.bonus_pending; /* 1 = 5,000 ... */
    int g = recorded_gift(foe_kind, mark, rg.deaths);
    if (g == IT_NONE) g = GIFT[(foe_kind * 5 + mark * 3 + rg.deaths * 2) & 7];
    if (g == IT_EGG && rg.owl.on) g = IT_JAR;
    if (g == IT_BEETLE) drop_many(IT_BEETLE, 3, x, y);
    else rsl_drop_item(g, x, y, 0);
    rsl_sfx("rsl_gift");
}

/* ------------------------------------------------------------------------ */

static bool player_touches(int x, int y, int hw, int hh) {
    const RslPlayer *p = &rg.pl;
    int px = RSL_PX(p->x), py = RSL_PX(p->y) - 11;
    return iabs(px - x) < 5 + hw && iabs(py - y) < 11 + hh;
}

static void take(int i) {
    RslItem *it = &rg.item[i];
    int x = RSL_PX(it->x), y = RSL_PX(it->y);
    it->alive = 0;
    switch (it->kind) {
    case IT_POT: case IT_URN: case IT_JAR: case IT_COIN: case IT_IDOL: case IT_BEETLE:
        rsl_add_score(it->value);
        rsl_sfx(it->value >= 500 ? "rsl_bigcoin" : "rsl_coin");
        break;
    case IT_CLOCK:
        rg.time += RSL_CLOCK_ADD;
        rsl_say("+30 SECONDS");
        rsl_sfx("rsl_clock");
        break;
    case IT_LILY:
        rsl_clear_screen(true);
        rsl_sfx("rsl_lily");
        rg.shake = 10;
        break;
    case IT_CHARM: {
        int f = rsl_spawn_foe(FO_CLOUD, it->x, (rg.cam_y - 10) * RSL_FX, 0);
        (void)f;
        rsl_say("A STORM GATHERS");
        rsl_sfx("rsl_zap");
        break;
    }
    case IT_EGG:
        if (rg.owl.on) { rsl_add_score(500); rsl_sfx("rsl_bigcoin"); }
        else {
            rg.owl.on = true;
            rg.owl.x = it->x;
            rg.owl.y = it->y;
            rg.owl.target = -1;
            rsl_say("BLINK THE OWLET HATCHES");
            rsl_sfx("rsl_hatch");
        }
        break;
    case IT_BELL: {
        int a = rsl_drop_item(IT_ASH, rg.pl.x, (rg.cam_y + SCREEN_H + 16) * RSL_FX, 0);
        if (a >= 0) { rg.item[a].vx = 0; rg.item[a].vy = 0; }
        rsl_sfx("rsl_bell");
        rsl_music(RSL_MUS_RISE);
        break;
    }
    case IT_ASH:
        if (rg.deaths > 0) { rg.deaths--; rsl_say("ONE DEATH FORGIVEN"); }
        else { rsl_add_score(1000); }
        rsl_sfx("rsl_bless");
        break;
    case IT_WEAPON:
        rg.pl.weapon = it->var;
        rsl_say(it->var == WP_EMBER ? "EMBER: STRONG AND FAR" : it->var == WP_SEEKER ? "SEEKER: IT FINDS THEM" : "SCATTER: FIVE AT ONCE");
        rsl_sfx("rsl_power");
        break;
    default: break;
    }
    /* one pick from a wheel: the rest of it goes */
    if (it->wheel)
        for (int j = 0; j < RSL_MAX_ITEMS; j++)
            if (rg.item[j].alive && rg.item[j].wheel == it->wheel) rg.item[j].alive = 0;
    (void)x;
    (void)y;
}

void rsl_items_update(void) {
    int mh = rsl_main_h();
    for (int i = 0; i < RSL_MAX_ITEMS; i++) {
        RslItem *it = &rg.item[i];
        if (!it->alive) continue;
        it->t++;
        int x = RSL_PX(it->x), y = RSL_PX(it->y);
        if (it->wheel) {
            /* the wheel turns slowly about the torch */
            int a = it->t / 3;
            it->x = it->cx + rsl_cos(a) * 16;
            it->y = it->cy + rsl_sin(a) * 12;
            if (!rsl_in_view(RSL_PX(it->x), RSL_PX(it->y), 260)) { it->alive = 0; continue; }
        } else if (it->kind == IT_ASH) {
            it->y -= 96;
            if (RSL_PX(it->y) < rg.cam_y - 30) { it->alive = 0; continue; }
        } else {
            /* falls and settles */
            if (!it->landed) {
                it->vy = imin(it->vy + 40, 1024);
                int nx = it->x + it->vx;
                if (!rsl_solid_px(RSL_PX(nx), y)) it->x = nx;
                int oy = RSL_PX(it->y);
                it->y += it->vy;
                int ny = RSL_PX(it->y);
                for (int yy = oy + 1; yy <= ny + 5; yy++)
                    if (yy % RSL_TILE == 0 && it->vy > 0 && rsl_floor_px(RSL_PX(it->x), yy) && yy - 5 <= ny) {
                        it->y = (yy - 5) * RSL_FX;
                        it->landed = 1;
                        break;
                    }
                if (RSL_PX(it->y) > rg.mh * RSL_TILE + 20 || (RSL_PX(it->y) < mh * RSL_TILE && RSL_PX(it->y) + 6 >= mh * RSL_TILE)) { it->alive = 0; continue; }
            }
            if (it->t > 900 || !rsl_in_view(RSL_PX(it->x), RSL_PX(it->y), 260)) { it->alive = 0; continue; }
            /* a gold beetle runs from him, hop by hop, until a wall boxes it in */
            if (it->kind == IT_BEETLE && it->landed) {
                int dx = RSL_PX(it->x) - RSL_PX(rg.pl.x), dy = RSL_PX(it->y) - (RSL_PX(rg.pl.y) - 8);
                int away = dx >= 0 ? 1 : -1;
                if (iabs(dx) < 60 && iabs(dy) < 30 && !rsl_solid_px(RSL_PX(it->x) + away * 10, RSL_PX(it->y))) {
                    it->landed = 0;
                    it->vy = -520;
                    it->vx = away * 260;
                }
            }
        }
        x = RSL_PX(it->x);
        y = RSL_PX(it->y);
        int reach = it->kind == IT_ASH ? 10 : 6;
        if (rg.state == RS_PLAY && it->t > 8 && player_touches(x, y, reach, reach)) take(i);
    }
    /* torches spit fire from 4 deaths; hidden spots wait to be touched */
    for (int s = 0; s < rg.nspawn; s++) {
        RslSpawn *sp = &rg.spawn[s];
        int sx = sp->tx * RSL_TILE + 8, sy = sp->ty * RSL_TILE + 8;
        if (sp->type == SP_TORCH && !sp->used && rg.deaths >= 4 && rsl_in_view(sx, sy, -8)) {
            if (++sp->b >= 150) {
                sp->b = 0;
                int a = rsl_dir_to(rg.pl.x - sx * RSL_FX, rg.pl.y - 12 * RSL_FX - sy * RSL_FX);
                rsl_add_eshot(ES_FIRE, sx * RSL_FX, (sy - 6) * RSL_FX, rsl_cos(a) * 360 / 256, rsl_sin(a) * 360 / 256);
                rsl_sfx("rsl_fireball");
            }
        }
        if (sp->type != SP_SECRET || sp->used) continue;
        if (sp->a == 'Q' || sp->a == 'K') {
            if (rg.pit >= 0 && sp->ty >= mh && sp->tx / RSL_SW == rg.pit) {
                rsl_drop_item(sp->a == 'Q' ? IT_BELL : IT_CLOCK, sx * RSL_FX, sy * RSL_FX, 0);
                sp->used = 1;
            }
            continue;
        }
        if (sp->a == 'Z') {
            if (rg.pit >= 0 && sp->tx / RSL_SW == rg.pit && rg.state == RS_PLAY && player_touches(sx, sy, 8, 8)) {
                rsl_leave_pit();
                return;
            }
            continue;
        }
        if (rg.state == RS_PLAY && player_touches(sx, sy, 7, 7)) rsl_find_secret(s);
    }
}

/* ------------------------------------------------------------------------ */
/* Blink the owlet: follows, takes one hit, flies to hidden spots            */

void rsl_owlet_update(void) {
    RslOwlet *o = &rg.owl;
    if (o->flag_t > 0) {
        o->flag_t--;
        o->y -= 64;
    }
    if (!o->on) return;
    o->t++;
    const RslPlayer *p = &rg.pl;
    int tx = p->x - p->face * 16 * RSL_FX, ty = p->y - 34 * RSL_FX;
    o->target = -1;
    int best = 1 << 30;
    for (int s = 0; s < rg.nspawn; s++) {
        const RslSpawn *sp = &rg.spawn[s];
        if (sp->type != SP_SECRET || sp->used || sp->a) continue;
        int sx = sp->tx * RSL_TILE + 8, sy = sp->ty * RSL_TILE + 8;
        int dx = sx - RSL_PX(p->x), dy = sy - RSL_PX(p->y);
        if (iabs(dx) < 150 && iabs(dy) < 100 && iabs(dx) + iabs(dy) < best) {
            best = iabs(dx) + iabs(dy);
            o->target = s;
            tx = sx * RSL_FX;
            ty = (sy - 12) * RSL_FX;
        }
    }
    int dx = tx - o->x, dy = ty - o->y;
    o->x += dx / 10;
    o->y += dy / 10 + rsl_sin(o->t * 2) * 40 / 256;
}
