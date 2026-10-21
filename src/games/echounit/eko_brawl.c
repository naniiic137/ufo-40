/* ECHO UNIT - FLINT's brawler: the Marseille docks and yards, the lift ride
 * with Scrapjaw on the roof, and the Caldera's lab climb with Dr. Talpa.
 * One fixed-height jump (he faces the same way the whole jump; LEFT / RIGHT
 * only steer it), a three-hit standing combo, a crouching jab, a charged ki
 * blast (A held standing still until he flashes, then let go: a ball hurled
 * a short way forward, through everything in its path), barrels to lift and
 * throw, and more points for every extra foe one blast or one barrel takes
 * down. The key carrier knocked down off the screen leaves no key at all:
 * the original's bug, kept on purpose. More troopers keep coming while he
 * hangs about, so he has to press on; hazards (a crane load, a crane hook,
 * a lump of slag) show a "!" first. The ki blast wipes out the bullets in
 * its path and carries him forward with it; a jump kick sends a grenade
 * back where it came from. Gulls sit on the bollards: leave them be and
 * they are worth 100 each at the end of the part. Dock rats run low: only
 * a crouching jab (or a barrel) reaches them. */
#include "eko.h"

#define B (&eg.br)

const int EKO_BF_HP[BF_KINDS] = {0, 3, 3, 2, 3, 1, 36, 40};
const int EKO_BF_POINTS[BF_KINDS] = {0, 20, 30, 40, 50, 30, 500, 500};

/* ---- the map ---------------------------------------------------------------- */

static const EkoChunk *chunk_of(char c) {
    for (int i = 0; i < EKO_BCHUNKS; i++)
        if (EKO_BCHUNK[i].code == c) return &EKO_BCHUNK[i];
    return NULL;
}

static char tile_at(int tx, int ty) {
    if (tx < 0 || tx >= B->mw) return '#';
    if (ty < 0) return '.';
    if (ty >= B->mh) return '.';
    return B->tile[ty][tx];
}

bool eko_brawl_solid(int tx, int ty) {
    char c = tile_at(tx, ty);
    return c == '#' || c == 'c' || c == 'q' || c == 'W' || c == 'H' || c == 'h';
}

static bool platform(int tx, int ty) { return tile_at(tx, ty) == '='; }

static bool solid_px(int x, int y) { return eko_brawl_solid(x >> 4, y >> 4); }

static void add_spawn(int tx, int ty, int kind) {
    if (B->nspawn >= ARRAY_LEN(B->spawn)) return;
    B->spawn[B->nspawn].tx = (int16_t)tx;
    B->spawn[B->nspawn].ty = (int16_t)ty;
    B->spawn[B->nspawn].kind = (uint8_t)kind;
    B->spawn[B->nspawn].used = 0;
    B->nspawn++;
}


static void load_map(int m) {
    const EkoBrawlMap *bm = &EKO_BMAP[m];
    memset(B, 0, sizeof *B);
    B->map = m;
    B->lock_x = -1;
    int rows = 0, cols = 0;
    while (rows < BR_MAX_CH && bm->layout[rows]) {
        cols = imax(cols, (int)strlen(bm->layout[rows]));
        rows++;
    }
    B->mw = cols * BR_SW;
    B->mh = rows * BR_SH;
    int px = 32, py = 100;
    for (int cy = 0; cy < rows; cy++)
        for (int cx = 0; cx < cols; cx++) {
            char code = cx < (int)strlen(bm->layout[cy]) ? bm->layout[cy][cx] : '-';
            const EkoChunk *ch = chunk_of(code);
            for (int y = 0; y < BR_SH; y++)
                for (int x = 0; x < BR_SW; x++) {
                    int tx = cx * BR_SW + x, ty = cy * BR_SH + y;
                    char c = ch ? ch->rows[y][x] : '#';
                    char put = c;
                    switch (c) {
                    case 's': add_spawn(tx, ty, BF_SOLDIER); put = '.'; break;
                    case 'g': add_spawn(tx, ty, BF_GRENADIER); put = '.'; break;
                    case 'j': add_spawn(tx, ty, BF_JETPACK); put = '.'; break;
                    case 'k': add_spawn(tx, ty, BF_KEYMAN); put = '.'; break;
                    case 'b': add_spawn(tx, ty, SPK_BARREL); put = '.'; break;
                    case 'm': add_spawn(tx, ty, SPK_MANGO); put = '.'; break;
                    case '1': add_spawn(tx, ty, SPK_LIFE); put = '.'; break;
                    case 'r': add_spawn(tx, ty, BF_RAT); put = '.'; break;
                    case 'u': add_spawn(tx, ty, SPK_GULL); B->gulls++; put = '.'; break;
                    case 'X': add_spawn(tx, ty, SPK_DROP); put = '.'; break;
                    case 'Y': add_spawn(tx, ty, SPK_HOOK); put = '.'; break;
                    case 'P': px = tx * 16 + 8; py = ty * 16 + 16; put = '.'; break;
                    default: break;
                    }
                    B->tile[ty][tx] = put;
                    B->thp[ty][tx] = (int8_t)(put == 'c' || put == 'q' ? 2 : put == 'H' || put == 'h' ? 4 : 0);
                }
        }
    B->p.x = px * EKO_FX;
    B->p.y = py * EKO_FX;
    B->p.face = 1;
    B->p.safe_x = B->p.x;
    B->p.safe_y = B->p.y;
}

/* ---- objects ---------------------------------------------------------------- */

int eko_brawl_add_obj(int kind, int x, int y, int vx, int vy) {
    for (int i = 0; i < BR_MAX_OBJS; i++)
        if (!B->obj[i].alive) {
            EkoBObj *o = &B->obj[i];
            memset(o, 0, sizeof *o);
            o->alive = 1;
            o->kind = (uint8_t)kind;
            o->x = x;
            o->y = y;
            o->vx = vx;
            o->vy = vy;
            o->last_foe = -1;
            return i;
        }
    return -1;
}

int eko_brawl_spawn_foe(int kind, int x, int y) {
    for (int i = 0; i < BR_MAX_FOES; i++)
        if (!B->foe[i].alive) {
            EkoBFoe *f = &B->foe[i];
            memset(f, 0, sizeof *f);
            f->alive = 1;
            f->kind = (uint8_t)kind;
            f->x = x;
            f->y = y;
            f->hp = EKO_BF_HP[kind];
            f->face = -1;
            f->spawn = -1;
            f->last_gx = x;
            f->cd = 40 + rng_range(&eg.rng, 0, 40);
            if (kind == BF_JETPACK) { f->a = y; f->state = 0; }
            return i;
        }
    return -1;
}

static void run_spawns(void) {
    for (int i = 0; i < B->nspawn; i++) {
        if (B->spawn[i].used || B->spawn[i].kind == SPK_DROP || B->spawn[i].kind == SPK_HOOK) continue;
        int x = B->spawn[i].tx * 16 + 8, y = B->spawn[i].ty * 16 + 16;
        if (x < B->cam_x - 16 || x > B->cam_x + SCREEN_W + 24) continue;
        if (y < B->cam_y - 16 || y > B->cam_y + BR_SH * 16 + 32) continue;
        B->spawn[i].used = 1;
        int k = B->spawn[i].kind;
        if (k == SPK_BARREL) eko_brawl_add_obj(BO_BARREL, x * EKO_FX, y * EKO_FX, 0, 0);
        else if (k == SPK_MANGO) eko_brawl_add_obj(BO_MANGO, x * EKO_FX, y * EKO_FX, 0, 0);
        else if (k == SPK_LIFE) eko_brawl_add_obj(BO_LIFE, x * EKO_FX, y * EKO_FX, 0, 0);
        else if (k == SPK_GULL) eko_brawl_add_obj(BO_GULL, x * EKO_FX, y * EKO_FX, 0, 0);
        else {
            int f = eko_brawl_spawn_foe(k, x * EKO_FX, y * EKO_FX);
            if (f >= 0) B->foe[f].spawn = i;
        }
    }
}

/* ---- moving bodies -------------------------------------------------------------- */

/* move a body (feet centre, half width hw, height h) with the map; returns
 * bit 1 landed, bit 2 hit a wall */
static int move_body(int *x, int *y, int *vx, int *vy, int hw, int h, bool drop) {
    int res = 0;
    int nx = *x + *vx;
    int dir = isign(*vx);
    if (dir) {
        int edge = EKO_PX(nx) + dir * hw;
        int top = EKO_PX(*y) - h + 1, bot = EKO_PX(*y) - 1;
        bool hit = false;
        for (int yy = top; yy <= bot; yy += 7)
            if (solid_px(edge, yy)) hit = true;
        if (solid_px(edge, bot)) hit = true;
        if (hit) {
            if (dir > 0) nx = (((edge >> 4) * 16) - hw - 1) * EKO_FX + 255;
            else nx = ((((edge >> 4) + 1) * 16) + hw) * EKO_FX;
            *vx = 0;
            res |= 2;
        }
        *x = nx;
    }
    int ny = *y + *vy;
    int lx = EKO_PX(*x) - hw + 1, rx = EKO_PX(*x) + hw - 1;
    if (*vy > 0) {
        int feet_old = EKO_PX(*y), feet_new = EKO_PX(ny);
        for (int fy = feet_old; fy <= feet_new; fy++) {
            bool land = false;
            for (int xx = lx; xx <= rx; xx += 4) {
                if (solid_px(xx, fy)) land = true;
                if (!drop && platform(xx >> 4, fy >> 4) && (fy & 15) == 0 && feet_old <= fy) land = true;
            }
            if (solid_px(rx, fy)) land = true;
            if (!drop && platform(rx >> 4, fy >> 4) && (fy & 15) == 0 && feet_old <= fy) land = true;
            if (land) {
                ny = fy * EKO_FX;
                *vy = 0;
                res |= 1;
                break;
            }
        }
    } else if (*vy < 0) {
        int head = EKO_PX(ny) - h;
        for (int xx = lx; xx <= rx; xx += 4)
            if (solid_px(xx, head)) {
                ny = ((head >> 4) * 16 + 16 + h) * EKO_FX;
                *vy = 0;
                break;
            }
    }
    *y = ny;
    return res;
}

static bool on_ground(int x, int y, int hw) {
    int fy = EKO_PX(y);
    for (int xx = EKO_PX(x) - hw + 1; xx <= EKO_PX(x) + hw - 1; xx += 3)
        if (solid_px(xx, fy) || (platform(xx >> 4, fy >> 4) && (fy & 15) == 0)) return true;
    return false;
}

/* a foe's half width and height (the bosses are big, a rat is low) */
static int foe_hw(const EkoBFoe *f) { return f->kind >= BF_SCRAPJAW ? 14 : 7; }
static int foe_h(const EkoBFoe *f) { return f->kind >= BF_SCRAPJAW ? 30 : f->kind == BF_RAT ? 4 : 22; }

/* a gull frightened off its bollard: no thanks at the end of the part */
static void frighten(EkoBObj *o) {
    if (o->kind != BO_GULL || o->state) return;
    o->state = 1;
    o->t = 0;
    B->gulls_hit++;
    eko_sfx("eko_gull");
}

static void frighten_box(int x0, int y0, int w, int h) {
    for (int i = 0; i < BR_MAX_OBJS; i++) {
        EkoBObj *o = &B->obj[i];
        if (o->alive && o->kind == BO_GULL && !o->state && rects_overlap(x0, y0, w, h, EKO_PX(o->x) - 5, EKO_PX(o->y) - 10, 10, 10)) frighten(o);
    }
}

/* ---- damage ------------------------------------------------------------------------ */

/* the key carrier is down: the key drops where he fell, if that is on the
 * screen. Knocked down off it (past either edge, or down a pit) and the key
 * never appears: Cyber Owls' Chicago softlock, kept as the owner chose. */
static void drop_key(EkoBFoe *f) {
    int x = EKO_PX(f->x), y = EKO_PX(f->y) - 12;
    if (x < B->cam_x || x >= B->cam_x + SCREEN_W || y < B->cam_y - 8 || y >= B->cam_y + BR_SH * 16) {
        B->key_lost = 1;
        return;
    }
    eko_brawl_add_obj(BO_KEY, f->x, f->y - 8 * EKO_FX, 0, -512);
    eko_say("THE WALL KEY!");
}

static void kill_foe(int i, int chain) {
    EkoBFoe *f = &B->foe[i];
    int pts = EKO_BF_POINTS[f->kind];
    int mult = 1;
    if (chain > 0) mult = ++B->chain_kills[chain & 15];
    eko_add_score(pts * mult);
    if (mult > 1) {
        char m[24];
        snprintf(m, sizeof m, "X%d  %d", mult, pts * mult);
        eko_say(m);
    }
    B->kills++;
    eko_burst(EKO_PX(f->x), EKO_PX(f->y) - 12, C_YELLOW, 8);
    if (f->kind == BF_KEYMAN) drop_key(f);
    if (f->kind == BF_SCRAPJAW || f->kind == BF_TALPA) {
        B->boss_dead = 1;
        B->boss_t = 0;
        eko_sfx("eko_bossdie");
        eg.shake = 30;
    } else eko_sfx("eko_ko");
    if (f->spawn >= 0) B->spawn[f->spawn].used = 1;
    f->alive = 0;
}

static void hurt_foe(int i, int dmg, int dir, bool launch, int chain) {
    EkoBFoe *f = &B->foe[i];
    if (!f->alive || f->state == 99) return;
    if (f->kind == BF_TALPA && f->state == 2) return; /* under the floor */
    f->hp -= dmg;
    B->dmg_dealt += dmg;
    f->flash = 8;
    eko_sfx("eko_hit");
    bool boss = f->kind == BF_SCRAPJAW || f->kind == BF_TALPA;
    if (!boss) {
        f->kb = launch ? 20 : 6;
        f->vx = dir * (launch ? 640 : 128); /* a jab barely moves them: the combo stays in reach */
        /* a blow breaks off whatever it was winding up */
        if (f->kind != BF_JETPACK) { f->state = 0; f->t = 0; }
        f->cd = imax(f->cd, 24);
        if (launch) f->vy = -640;
        f->chain = chain;
    }
    if (f->hp <= 0) {
        if (!boss && launch) {
            /* thrown clear: it flies, and the kill counts when it lands */
            f->state = 99;
            f->t = 0;
            f->chain = chain;
            return;
        }
        kill_foe(i, chain);
    }
}

static void hurt_player(int dmg, int dir) {
    EkoBPlayer *p = &B->p;
    if (p->inv > 0 || p->dead_t || eg.god) return;
    eko_hurt(dmg);
    if (eko_hp() <= 0) {
        p->dead_t = 1;
        p->vx = 0;
        p->vy = -768;
        return;
    }
    p->inv = EKO_OWL_INV[OWL_FLINT];
    p->hurt_t = 14;
    p->vx = dir * 320;
    p->vy = -512;
    p->act = BRA_NONE;
    p->charge = 0;
    if (p->carry) {
        EkoBObj *o = &B->obj[p->carry - 1];
        o->state = 0;
        o->vx = 0;
        o->vy = -256;
        p->carry = 0;
    }
}

/* ---- the player -------------------------------------------------------------------- */

static int act_len(int a) {
    switch (a) {
    case BRA_PUNCH1: case BRA_PUNCH2: case BRA_LOW: return 14;
    case BRA_KICK: return 20;
    case BRA_KI: return BR_KI_POSE;
    case BRA_THROW: return 12;
    case BRA_AIRKICK: return 999;
    default: return 0;
    }
}

/* the attack's box this frame (px), false when none */
static bool attack_box(int *x0, int *y0, int *x1, int *y1, int *dmg, bool *launch) {
    EkoBPlayer *p = &B->p;
    int px = EKO_PX(p->x), py = EKO_PX(p->y), f = p->face;
    *launch = false;
    switch (p->act) {
    case BRA_PUNCH1: case BRA_PUNCH2:
        if (p->act_t < 3 || p->act_t > 8) return false;
        *x0 = f > 0 ? px + 2 : px - 20; *x1 = *x0 + 18; *y0 = py - 20; *y1 = py - 8; *dmg = 1;
        return true;
    case BRA_KICK:
        if (p->act_t < 4 || p->act_t > 11) return false;
        *x0 = f > 0 ? px + 2 : px - 22; *x1 = *x0 + 20; *y0 = py - 16; *y1 = py - 4; *dmg = 2;
        return true;
    case BRA_LOW:
        if (p->act_t < 3 || p->act_t > 8) return false;
        *x0 = f > 0 ? px + 2 : px - 20; *x1 = *x0 + 18; *y0 = py - 9; *y1 = py; *dmg = 1;
        return true;
    case BRA_AIRKICK:
        *x0 = f > 0 ? px : px - 18; *x1 = *x0 + 18; *y0 = py - 14; *y1 = py + 2; *dmg = 2;
        return true;
    default: return false;
    }
}

static void begin_act(int a) {
    EkoBPlayer *p = &B->p;
    p->act = a;
    p->act_t = 0;
    p->atk_sn++;
    if (a == BRA_KI) {
        /* the ki blast: hurled a short way forward; it scores as one chain */
        int o = eko_brawl_add_obj(BO_KI, p->x + p->face * 10 * EKO_FX, p->y - 14 * EKO_FX, p->face * BR_KI_VX, 0);
        if (o >= 0) {
            B->obj[o].chain = ++B->chain_next;
            B->chain_kills[B->obj[o].chain & 15] = 0;
            B->obj[o].hits = 0; /* the foes it has gone through, one bit each */
        }
        eko_sfx("eko_ki");
    } else if (a != BRA_THROW) eko_sfx("eko_swing");
}

static int barrel_near(void) {
    EkoBPlayer *p = &B->p;
    for (int i = 0; i < BR_MAX_OBJS; i++) {
        EkoBObj *o = &B->obj[i];
        if (!o->alive || o->kind != BO_BARREL || o->state != 0) continue;
        if (iabs(EKO_PX(o->x) - EKO_PX(p->x)) < 16 && iabs(EKO_PX(o->y) - EKO_PX(p->y)) < 10) return i;
    }
    return -1;
}

/* a blow on a crate or a cracked wall: it gives once its knocks run out */
static void knock_tile(int tx, int ty, int dmg) {
    char c = tile_at(tx, ty);
    B->thp[ty][tx] = (int8_t)(B->thp[ty][tx] - dmg);
    eko_sfx("eko_clang");
    if (B->thp[ty][tx] > 0) return;
    B->tile[ty][tx] = '.';
    eko_burst(tx * 16 + 8, ty * 16 + 8, c == 'c' || c == 'q' ? C_TAN : C_GREY, 10);
    if (c == 'c' || c == 'q') eko_add_score(10);
    if (c == 'q' || c == 'H') { eko_brawl_add_obj(BO_MANGO, (tx * 16 + 8) * EKO_FX, (ty * 16 + 16) * EKO_FX, 0, -384); B->secrets++; }
    if (c == 'h') { eko_brawl_add_obj(BO_LIFE, (tx * 16 + 8) * EKO_FX, (ty * 16 + 16) * EKO_FX, 0, -384); B->secrets++; }
    if (c == 'H' || c == 'h') eko_sfx("eko_secret");
}

static void attack_hits(void) {
    int x0, y0, x1, y1, dmg;
    bool launch;
    EkoBPlayer *p = &B->p;
    if (!attack_box(&x0, &y0, &x1, &y1, &dmg, &launch)) return;
    for (int i = 0; i < BR_MAX_FOES; i++) {
        EkoBFoe *f = &B->foe[i];
        if (!f->alive || f->state == 99 || f->hitsn == p->atk_sn) continue;
        int fx = EKO_PX(f->x), fy = EKO_PX(f->y);
        int hw = f->kind >= BF_SCRAPJAW ? 14 : 6, h = foe_h(f);
        if (rects_overlap(x0, y0, x1 - x0, y1 - y0, fx - hw, fy - h, hw * 2, h)) {
            f->hitsn = p->atk_sn;
            hurt_foe(i, dmg, p->face, launch || p->act == BRA_KICK, 0);
        }
    }
    /* barrels in the air: a kick (or any blow) sends them on again */
    for (int i = 0; i < BR_MAX_OBJS; i++) {
        EkoBObj *o = &B->obj[i];
        if (!o->alive || o->kind != BO_BARREL || o->state != 3 || o->hits == p->atk_sn) continue;
        int ox = EKO_PX(o->x), oy = EKO_PX(o->y);
        if (rects_overlap(x0, y0, x1 - x0, y1 - y0, ox - 7, oy - 14, 14, 14)) {
            o->hits = p->atk_sn;
            o->last_foe = -1;
            o->state = 2;
            o->vx = p->face * 1024;
            o->vy = -320;
            eko_sfx("eko_clang");
        }
    }
    /* a grenade in the air meets a jump kick: back it goes, at them */
    if (p->act == BRA_AIRKICK)
        for (int i = 0; i < BR_MAX_OBJS; i++) {
            EkoBObj *o = &B->obj[i];
            if (!o->alive || o->kind != BO_GRENADE || o->state) continue;
            if (!rects_overlap(x0 - 2, y0 - 2, x1 - x0 + 4, y1 - y0 + 4, EKO_PX(o->x) - 4, EKO_PX(o->y) - 7, 8, 8)) continue;
            /* back the way it came, at the nearest foe ahead */
            int gx = EKO_PX(o->x), gy = EKO_PX(o->y) - 3, best = -1, bd = 220;
            for (int k = 0; k < BR_MAX_FOES; k++) {
                const EkoBFoe *f = &B->foe[k];
                int d = (EKO_PX(f->x) - gx) * p->face;
                if (f->alive && f->state != 99 && d > 0 && d < bd) { bd = d; best = k; }
            }
            int a = best >= 0 ? eko_dir_to(EKO_PX(B->foe[best].x) - gx, EKO_PX(B->foe[best].y) - 12 - gy) : (p->face > 0 ? 0 : 32);
            o->state = 1;
            o->vx = eko_cos(a) * 1024 / 256;
            o->vy = eko_sin(a) * 1024 / 256;
            o->t = 0;
            eko_sfx("eko_clang");
        }
    frighten_box(x0, y0, x1 - x0, y1 - y0);
    /* crates and cracked walls */
    for (int ty = y0 >> 4; ty <= y1 >> 4; ty++)
        for (int tx = x0 >> 4; tx <= x1 >> 4; tx++) {
            char c = tile_at(tx, ty);
            if (c != 'c' && c != 'q' && c != 'H' && c != 'h') continue;
            if (B->thp[ty][tx] <= 0) continue;
            /* one knock per blow */
            if (B->tile_sn == p->atk_sn) continue;
            B->tile_sn = p->atk_sn;
            knock_tile(tx, ty, imax(1, dmg / 2 + (launch ? 2 : 0)));
        }
}

static void player_update(void) {
    EkoBPlayer *p = &B->p;
    if (p->dead_t) {
        p->dead_t++;
        p->vy = imin(p->vy + BR_GRAV, BR_MAX_FALL);
        p->y += p->vy;
        if (p->dead_t > 80) eko_life_lost();
        return;
    }
    if (p->inv > 0) p->inv--;
    bool L = ek_btn(BTN_LEFT), R = ek_btn(BTN_RIGHT), D = ek_btn(BTN_DOWN);
    bool A = ek_btn(EKO_MAIN), Ap = ek_btnp(EKO_MAIN), Bp = ek_btnp(EKO_ALT);
    p->ground = on_ground(p->x, p->y, BR_PW / 2) && p->vy >= 0;
    if (p->hurt_t > 0) {
        p->hurt_t--;
    } else {
        if (p->act && p->act != BRA_AIRKICK) {
            p->act_t++;
            if (p->act_t >= act_len(p->act)) p->act = BRA_NONE;
        }
        if (p->act == BRA_AIRKICK && p->ground) p->act = BRA_NONE;
        p->duck = p->ground && D && !p->carry && (p->act == BRA_NONE || p->act == BRA_LOW);
        /* the charge: A held, standing still on the ground, nothing else going on */
        bool still = p->ground && !L && !R && !D && !p->carry;
        if (A && still && p->act == BRA_NONE) {
            p->charge++;
            if (p->charge == BR_CHARGE_T) eko_sfx("eko_charged");
        } else if (!A) {
            if (p->charge >= BR_CHARGE_T && still) begin_act(BRA_KI);
            p->charge = 0;
        } else if (!still) p->charge = 0;
        /* blows */
        if (Ap) {
            if (p->carry) {
                EkoBObj *o = &B->obj[p->carry - 1];
                o->state = 2;
                o->vx = p->face * 1024;
                o->vy = -256;
                o->x = p->x + p->face * 8 * EKO_FX;
                o->y = p->y - 4 * EKO_FX; /* thrown low, at a trooper's chest */
                o->chain = ++B->chain_next;
                B->chain_kills[o->chain & 15] = 0;
                o->hits = -1;
                o->last_foe = -1;
                p->carry = 0;
                begin_act(BRA_THROW);
                eko_sfx("eko_throw");
            } else if (!p->ground) {
                if (p->act != BRA_AIRKICK) begin_act(BRA_AIRKICK);
            } else if (D) {
                int b = barrel_near();
                if (b >= 0) {
                    B->obj[b].state = 1;
                    p->carry = b + 1;
                    p->act = BRA_NONE;
                    eko_sfx("eko_lift");
                } else if (p->act == BRA_NONE) begin_act(BRA_LOW);
            } else if (p->act == BRA_NONE) begin_act(BRA_PUNCH1);
            else if (p->act == BRA_PUNCH1 && p->act_t >= 5) begin_act(BRA_PUNCH2);
            else if (p->act == BRA_PUNCH2 && p->act_t >= 5) begin_act(BRA_KICK);
        }
        bool rooted = p->ground && (p->act == BRA_PUNCH1 || p->act == BRA_PUNCH2 || p->act == BRA_KICK ||
                                    p->act == BRA_LOW || p->act == BRA_THROW);
        /* the ki blast carries him forward with it (off a ledge, too) */
        if (p->act == BRA_KI) p->vx = p->face * BR_KI_LUNGE;
        else if (rooted || p->duck) p->vx = 0;
        else {
            int dir = (R ? 1 : 0) - (L ? 1 : 0);
            p->vx = dir * (p->carry ? BR_CARRY_WALK : BR_WALK);
            /* in the air the pad only steers: he faces the same way the whole jump */
            if (dir && p->act == BRA_NONE && p->ground) p->face = dir;
        }
        /* the one jump: the same height every time */
        if (Bp && p->ground && !p->duck) {
            p->vy = BR_JUMP_VY;
            p->ground = false;
            p->charge = 0;
            eko_sfx("eko_jump");
        }
    }
    p->vy = imin(p->vy + BR_GRAV, BR_MAX_FALL);
    int r = move_body(&p->x, &p->y, &p->vx, &p->vy, BR_PW / 2, p->duck ? BR_PH_DUCK : BR_PH, false);
    if (r & 1) {
        if (p->act == BRA_AIRKICK) p->act = BRA_NONE;
    }
    /* keep inside the map and the boss arena */
    int minx = (B->lock_x >= 0 ? B->lock_x : 0) + 6, maxx = B->lock_x >= 0 ? B->lock_x + SCREEN_W - 6 : B->mw * 16 - 6;
    if (B->map != BM_LAB && B->lock_x < 0) minx = imax(minx, B->cam_x + 6);
    if (EKO_PX(p->x) < minx) p->x = minx * EKO_FX;
    if (EKO_PX(p->x) > maxx) p->x = maxx * EKO_FX;
    if (on_ground(p->x, p->y, BR_PW / 2) && p->vy >= 0) {
        int tx = EKO_PX(p->x) >> 4, ty = EKO_PX(p->y) >> 4;
        /* a safe spot: firm ground under both neighbours too */
        bool firm = (eko_brawl_solid(tx - 1, ty) || platform(tx - 1, ty)) && (eko_brawl_solid(tx + 1, ty) || platform(tx + 1, ty));
        if (firm) { p->safe_x = p->x; p->safe_y = p->y; }
    }
    attack_hits();
    /* down a pit */
    if (EKO_PX(p->y) > B->mh * 16 + 24) {
        eko_hurt(99);
        p->dead_t = 30;
        p->vy = 0;
        eko_sfx("eko_fall");
    }
    /* the key in the lock */
    if (p->key && !B->wall_open_t) {
        int px = EKO_PX(p->x), py = EKO_PX(p->y) - 10;
        for (int dx = -14; dx <= 14; dx += 28)
            if (tile_at((px + dx) >> 4, py >> 4) == 'W') {
                B->wall_open_t = 1;
                p->key = false;
                eko_say("CHARGE SET: STAND BACK");
                eko_sfx("eko_beep");
            }
    }
    /* the way out */
    if (tile_at(EKO_PX(p->x) >> 4, (EKO_PX(p->y) - 8) >> 4) == 'E' && !B->done_t) {
        B->done_t = 1;
        eko_sfx("eko_door");
    }
}

/* ---- objects ------------------------------------------------------------------------- */

static void explode(int x, int y, int r, int dmg, bool from_foe) {
    eko_burst(x, y, C_ORANGE, 14);
    eko_sfx("eko_boom");
    eg.shake = imax(eg.shake, 6);
    EkoBPlayer *p = &B->p;
    if (from_foe && eko_dist(EKO_PX(p->x) - x, EKO_PX(p->y) - 12 - y) < r + 6) hurt_player(dmg, EKO_PX(p->x) < x ? -1 : 1);
    if (!from_foe) {
        /* his (a grenade kicked back): it takes the foes round it, as one chain */
        int chain = ++B->chain_next;
        B->chain_kills[chain & 15] = 0;
        for (int k = 0; k < BR_MAX_FOES; k++) {
            EkoBFoe *f = &B->foe[k];
            if (f->alive && f->state != 99 && eko_dist(EKO_PX(f->x) - x, EKO_PX(f->y) - 12 - y) < r + 10)
                hurt_foe(k, 3, EKO_PX(f->x) < x ? -1 : 1, true, chain);
        }
        frighten_box(x - r, y - r, r * 2, r * 2);
    }
    /* a grenade spoils any fruit lying about */
    for (int i = 0; i < BR_MAX_OBJS; i++) {
        EkoBObj *o = &B->obj[i];
        if (o->alive && (o->kind == BO_MANGO || o->kind == BO_LIFE) && eko_dist(EKO_PX(o->x) - x, EKO_PX(o->y) - y) < r + 8) {
            o->alive = 0;
            eko_burst(EKO_PX(o->x), EKO_PX(o->y) - 4, C_LIME, 6);
        }
    }
}

static void objs_update(void) {
    EkoBPlayer *p = &B->p;
    int px = EKO_PX(p->x), py = EKO_PX(p->y);
    for (int i = 0; i < BR_MAX_OBJS; i++) {
        EkoBObj *o = &B->obj[i];
        if (!o->alive) continue;
        o->t++;
        int ox = EKO_PX(o->x), oy = EKO_PX(o->y);
        switch (o->kind) {
        case BO_BARREL:
            if (o->state == 1) {
                o->x = p->x;
                o->y = p->y - 22 * EKO_FX;
                break;
            }
            if (o->state == 2 || o->state == 3) {
                o->vy = imin(o->vy + (o->state == 2 ? 24 : 40), BR_MAX_FALL);
                int vx = o->vx, vy = o->vy;
                int r = move_body(&o->x, &o->y, &vx, &vy, 6, 12, true);
                o->vy = vy;
                if (r) {
                    /* it breaks on the ground or a wall */
                    eko_burst(EKO_PX(o->x), EKO_PX(o->y) - 6, C_BROWN, 8);
                    eko_sfx("eko_clang");
                    o->alive = 0;
                    break;
                }
                for (int k = 0; k < BR_MAX_FOES; k++) {
                    EkoBFoe *f = &B->foe[k];
                    if (!f->alive || f->state == 99 || k == o->last_foe) continue;
                    int hw = foe_hw(f), h = f->kind == BF_RAT ? 14 : foe_h(f); /* a barrel bowls low enough for a rat */
                    if (rects_overlap(EKO_PX(o->x) - 6, EKO_PX(o->y) - 12, 12, 12, EKO_PX(f->x) - hw, EKO_PX(f->y) - h, hw * 2, h) &&
                        (o->state == 2 || o->vy > 0)) {
                        o->last_foe = k;
                        hurt_foe(k, 3, isign(o->vx) ? isign(o->vx) : 1, true, o->chain);
                        /* the ricochet: up and back, for a kick to send it on */
                        o->state = 3;
                        o->vx = -o->vx / 3;
                        o->vy = -896;
                        break;
                    }
                }
                frighten_box(EKO_PX(o->x) - 6, EKO_PX(o->y) - 12, 12, 12);
                if (EKO_PX(o->y) > B->mh * 16 + 32) o->alive = 0;
                break;
            }
            /* resting: falls */
            {
                int vx = 0;
                o->vy = imin(o->vy + BR_GRAV, BR_MAX_FALL);
                move_body(&o->x, &o->y, &vx, &o->vy, 6, 12, false);
                if (EKO_PX(o->y) > B->mh * 16 + 32) o->alive = 0;
            }
            break;
        case BO_MANGO: case BO_LIFE: case BO_KEY: {
            int vx = 0;
            o->vy = imin(o->vy + BR_GRAV / 2, BR_MAX_FALL);
            move_body(&o->x, &o->y, &vx, &o->vy, 5, 10, false);
            if (EKO_PX(o->y) > B->mh * 16 + 32) {
                /* a key never falls out of the world: it comes back up on the ledge */
                if (o->kind == BO_KEY) { o->y = (B->mh * 16 - 40) * EKO_FX; o->vy = 0; o->x = imax(o->x, (B->cam_x + 24) * EKO_FX); }
                else o->alive = 0;
            }
            if (p->dead_t == 0 && iabs(ox - px) < 12 && oy > py - 26 && oy < py + 6 && o->t > 10) {
                if (o->kind == BO_MANGO) { eko_set_hp(eko_hp() + EKO_MANGO_HP); eko_sfx("eko_mango"); eko_say("MANGO!"); }
                else if (o->kind == BO_LIFE) {
                    eg.owl[eg.cur_owl].lives = imin(EKO_MAX_LIVES, eg.owl[eg.cur_owl].lives + 1);
                    eko_sfx("eko_1up");
                    eko_say("EXTRA LIFE");
                } else { p->key = true; eko_sfx("eko_key"); eko_say("GOT THE KEY: TO THE LOCKED WALL"); }
                o->alive = 0;
            }
            break;
        }
        case BO_GRENADE: {
            if (!o->state) o->vy = imin(o->vy + 40, BR_MAX_FALL); /* kicked back, it flies straight */
            int vx = o->vx, vy = o->vy;
            int r = move_body(&o->x, &o->y, &vx, &vy, 3, 6, false);
            o->vx = vx;
            o->vy = vy;
            if (o->state && o->t > 4) {
                /* kicked back: it goes off on the first foe it meets */
                for (int k = 0; k < BR_MAX_FOES; k++) {
                    EkoBFoe *f = &B->foe[k];
                    if (f->alive && f->state != 99 &&
                        rects_overlap(EKO_PX(o->x) - 3, EKO_PX(o->y) - 6, 6, 6, EKO_PX(f->x) - foe_hw(f), EKO_PX(f->y) - foe_h(f), foe_hw(f) * 2, foe_h(f)))
                        r = 1;
                }
            }
            if (r || o->t > 150) { explode(EKO_PX(o->x), EKO_PX(o->y) - 4, 18, 1, o->state == 0); o->alive = 0; }
            break;
        }
        case BO_KI: {
            /* straight ahead, through every foe in its path, until it has flown its way */
            o->x += o->vx;
            int kx = EKO_PX(o->x), ky = EKO_PX(o->y);
            for (int k = 0; k < BR_MAX_FOES; k++) {
                EkoBFoe *f = &B->foe[k];
                if (!f->alive || f->state == 99 || (o->hits & (1 << k))) continue;
                int hw = foe_hw(f), h = foe_h(f);
                if (rects_overlap(kx - 6, ky - 6, 12, 12, EKO_PX(f->x) - hw, EKO_PX(f->y) - h, hw * 2, h)) {
                    o->hits |= 1 << k;
                    hurt_foe(k, 4, isign(o->vx), true, o->chain);
                }
            }
            /* it wipes out the bullets in its path */
            for (int k = 0; k < BR_MAX_OBJS; k++) {
                EkoBObj *b2 = &B->obj[k];
                if (b2->alive && b2->kind == BO_FBULLET && iabs(EKO_PX(b2->x) - kx) < 9 && iabs(EKO_PX(b2->y) - ky) < 10) {
                    b2->alive = 0;
                    eko_burst(EKO_PX(b2->x), EKO_PX(b2->y), C_WHITE, 3);
                }
            }
            frighten_box(kx - 6, ky - 6, 12, 12);
            char c = tile_at(kx >> 4, ky >> 4);
            if (eko_brawl_solid(kx >> 4, ky >> 4)) {
                /* a crate or a cracked wall takes the blow */
                if ((c == 'c' || c == 'q' || c == 'H' || c == 'h') && B->thp[ky >> 4][kx >> 4] > 0) knock_tile(kx >> 4, ky >> 4, 3);
                eko_burst(kx, ky, C_YELLOW, 6);
                o->alive = 0;
                break;
            }
            if (o->t >= BR_KI_T) { eko_burst(kx, ky, C_YELLOW, 4); o->alive = 0; }
            break;
        }
        case BO_GULL:
            if (o->state) {
                /* off it flies, complaining */
                o->x += 384;
                o->y -= 320;
                if (EKO_PX(o->y) < B->cam_y - 20) o->alive = 0;
            }
            break;
        case BO_DROP: {
            /* "!" up at the top of the screen, then the load comes down */
            if (o->state == 0) {
                if (o->t >= BR_WARN_T) { o->state = 1; o->y = (B->cam_y - 8) * EKO_FX; o->vy = 0; eko_sfx("eko_whoosh"); }
                break;
            }
            o->vy = imin(o->vy + 40, BR_MAX_FALL);
            o->y += o->vy;
            int dx0 = EKO_PX(o->x) - 8, dy0 = EKO_PX(o->y) - 14;
            if (p->dead_t == 0 && rects_overlap(dx0, dy0, 16, 14, px - 5, py - (p->duck ? BR_PH_DUCK : BR_PH), 10, p->duck ? BR_PH_DUCK : BR_PH))
                hurt_player(2, px < EKO_PX(o->x) ? -1 : 1);
            for (int k = 0; k < BR_MAX_FOES; k++) {
                EkoBFoe *f = &B->foe[k];
                if (f->alive && f->state != 99 && f->kind < BF_SCRAPJAW &&
                    rects_overlap(dx0, dy0, 16, 14, EKO_PX(f->x) - foe_hw(f), EKO_PX(f->y) - foe_h(f), foe_hw(f) * 2, foe_h(f)))
                    hurt_foe(k, 3, EKO_PX(f->x) < EKO_PX(o->x) ? -1 : 1, true, 0);
            }
            int fx = EKO_PX(o->x) >> 4, fy = EKO_PX(o->y) >> 4;
            if (tile_at(fx, fy) == '#' || EKO_PX(o->y) > B->mh * 16 + 32) {
                eko_burst(EKO_PX(o->x), EKO_PX(o->y) - 6, C_TAN, 10);
                eko_sfx("eko_thud");
                eg.shake = imax(eg.shake, 5);
                o->alive = 0;
            }
            break;
        }
        case BO_HOOK: {
            /* "!" at the right edge, then the hook sweeps across at chest height */
            if (o->state == 0) {
                if (o->t >= BR_WARN_T) { o->state = 1; o->x = (B->cam_x + SCREEN_W + 16) * EKO_FX; eko_sfx("eko_whoosh"); }
                break;
            }
            o->x -= 4 * EKO_FX;
            int hx0 = EKO_PX(o->x) - 9, hy0 = EKO_PX(o->y) - 4;
            if (p->dead_t == 0 && rects_overlap(hx0, hy0, 18, 8, px - 5, py - (p->duck ? BR_PH_DUCK : BR_PH), 10, p->duck ? BR_PH_DUCK : BR_PH))
                hurt_player(1, -1);
            for (int k = 0; k < BR_MAX_FOES; k++) {
                EkoBFoe *f = &B->foe[k];
                if (f->alive && f->state != 99 && f->kind < BF_SCRAPJAW &&
                    rects_overlap(hx0, hy0, 18, 8, EKO_PX(f->x) - foe_hw(f), EKO_PX(f->y) - foe_h(f), foe_hw(f) * 2, foe_h(f)))
                    hurt_foe(k, 3, -1, true, 0);
            }
            if (EKO_PX(o->x) < B->cam_x - 30) o->alive = 0;
            break;
        }
        case BO_FBULLET: case BO_FLAME:
            o->x += o->vx;
            o->y += o->vy;
            if (o->kind == BO_FLAME) o->vy += 12;
            if (solid_px(EKO_PX(o->x), EKO_PX(o->y)) || o->t > 160) { o->alive = 0; break; }
            if (p->dead_t == 0 && rects_overlap(EKO_PX(o->x) - 2, EKO_PX(o->y) - 2, 4, 4, px - 6, py - (p->duck ? BR_PH_DUCK : BR_PH), 12,
                                                p->duck ? BR_PH_DUCK : BR_PH)) {
                hurt_player(o->kind == BO_FLAME ? 2 : 1, isign(o->vx) ? isign(o->vx) : 1);
                o->alive = 0;
            }
            break;
        default: break;
        }
    }
}

/* ---- foes ------------------------------------------------------------------------------- */

static void foe_shoot(EkoBFoe *f, int speed, int dy) {
    int i = eko_brawl_add_obj(BO_FBULLET, f->x + f->face * 10 * EKO_FX, f->y - dy * EKO_FX, f->face * speed, 0);
    (void)i;
    eko_sfx("eko_shot");
}

static void touch_damage(EkoBFoe *f, int hw, int h, int dmg) {
    EkoBPlayer *p = &B->p;
    if (p->dead_t || f->state == 99) return;
    int px = EKO_PX(p->x), py = EKO_PX(p->y);
    int ph = p->duck ? BR_PH_DUCK : BR_PH;
    if (rects_overlap(EKO_PX(f->x) - hw, EKO_PX(f->y) - h, hw * 2, h, px - 5, py - ph, 10, ph))
        hurt_player(dmg, px < EKO_PX(f->x) ? -1 : 1);
}

static void boss_update(int i);

static void foes_update(void) {
    EkoBPlayer *p = &B->p;
    int px = EKO_PX(p->x), py = EKO_PX(p->y);
    for (int i = 0; i < BR_MAX_FOES; i++) {
        EkoBFoe *f = &B->foe[i];
        if (!f->alive) continue;
        f->t++;
        if (f->flash) f->flash--;
        if (f->cd) f->cd--;
        int fx = EKO_PX(f->x), fy = EKO_PX(f->y);
        int dx = px - fx;
        /* knocked flying: it scores when it lands */
        if (f->state == 99) {
            f->vy = imin(f->vy + BR_GRAV, BR_MAX_FALL);
            int r = move_body(&f->x, &f->y, &f->vx, &f->vy, 6, 20, true);
            /* bowling: a flying body knocks the next one down too */
            for (int k = 0; k < BR_MAX_FOES; k++) {
                EkoBFoe *g = &B->foe[k];
                if (k == i || !g->alive || g->state == 99 || g->kind >= BF_SCRAPJAW) continue;
                if (iabs(EKO_PX(g->x) - EKO_PX(f->x)) < 12 && iabs(EKO_PX(g->y) - EKO_PX(f->y)) < (g->kind == BF_RAT ? 24 : 18) && f->chain > 0)
                    hurt_foe(k, 4, isign(f->vx) ? isign(f->vx) : 1, true, f->chain);
            }
            if ((r & 1) || f->t > 40 || EKO_PX(f->y) > B->mh * 16 + 24) kill_foe(i, f->chain);
            continue;
        }
        if (f->kind == BF_SCRAPJAW || f->kind == BF_TALPA) { boss_update(i); continue; }
        /* far behind the camera: gone (a key carrier always stays) */
        if (fx < B->cam_x - 120 && f->kind != BF_KEYMAN) {
            f->alive = 0;
            continue;
        }
        if (f->kb > 0) {
            f->kb--;
            f->vx = f->vx * 7 / 8;
        } else if (f->kind == BF_JETPACK) {
            /* hovers above him, then dives */
            f->face = dx < 0 ? -1 : 1;
            if (f->state == 0) {
                int tx = (px - f->face * 40) * EKO_FX, ty = (py - 56 + eko_sin(f->t * 2) * 6 / 256) * EKO_FX;
                f->vx = iclamp((tx - f->x) / 24, -384, 384);
                f->vy = iclamp((ty - f->y) / 24, -320, 320);
                /* a slow shot down at him every so often */
                if (++f->b >= 120 && iabs(dx) < 150 && fy < py - 20) {
                    f->b = 0;
                    int a = eko_dir_to(px - fx, py - 12 - (fy - 12));
                    eko_brawl_add_obj(BO_FBULLET, f->x, f->y - 12 * EKO_FX, eko_cos(a) * 384 / 256, eko_sin(a) * 384 / 256);
                    eko_sfx("eko_shot");
                }
                if (!f->cd && iabs(dx) < 70) { f->state = 1; f->t = 0; f->vx = f->face * 512; f->vy = 640; }
            } else {
                if (f->t > 30) { f->state = 0; f->cd = 110; }
            }
            f->x += f->vx;
            f->y += f->vy;
            if (EKO_PX(f->y) < B->cam_y + 26) f->y = (B->cam_y + 26) * EKO_FX;
            touch_damage(f, 6, 18, 1);
            continue;
        } else {
            f->face = dx < 0 ? -1 : 1;
            int adx = iabs(dx);
            bool level = iabs(py - fy) < 14;
            if (f->kind == BF_RAT) {
                /* runs at him low and fast; a bite, then it backs off */
                if (f->state == 1) {
                    f->vx = -f->face * 256;
                    if (f->t > 24) f->state = 0;
                } else {
                    f->vx = f->face * 448;
                    EkoBPlayer *pl = &B->p;
                    if (!pl->dead_t && pl->inv == 0 && rects_overlap(fx - 6, fy - 5, 12, 5, px - 5, py - (pl->duck ? BR_PH_DUCK : BR_PH), 10, pl->duck ? BR_PH_DUCK : BR_PH)) {
                        hurt_player(1, f->face);
                        f->state = 1;
                        f->t = 0;
                    }
                }
            } else if (f->kind == BF_GRENADIER) {
                /* keeps its distance and lobs */
                if (adx < 70) f->vx = -f->face * 192;
                else if (adx > 130) f->vx = f->face * 192;
                else f->vx = 0;
                if (!f->cd && adx < 170) {
                    f->state = 1;
                    f->t = 0;
                    f->cd = 100 + rng_range(&eg.rng, 0, 50);
                    int frames = 50;
                    int vx = dx * EKO_FX / frames;
                    eko_brawl_add_obj(BO_GRENADE, f->x, f->y - 24 * EKO_FX, iclamp(vx, -900, 900), -1000);
                    eko_sfx("eko_toss");
                }
                if (f->state == 1 && f->t > 14) f->state = 0;
            } else {
                /* soldiers and the key carrier: close in and punch, or stop and shoot */
                if (f->state == 1) {
                    /* the wind-up of a punch */
                    f->vx = 0;
                    if (f->t == 16) {
                        if (adx < 24 && level) hurt_player(1, f->face);
                    }
                    if (f->t > 26) { f->state = 0; f->cd = 50; }
                } else if (f->state == 2) {
                    f->vx = 0;
                    if (f->t == 18) foe_shoot(f, 640, 16);
                    if (f->t > 34) { f->state = 0; f->cd = 90 + rng_range(&eg.rng, 0, 60); }
                } else {
                    if (adx > 20) f->vx = f->face * (f->kind == BF_KEYMAN ? 176 : 160);
                    else f->vx = 0;
                    if (!f->cd && adx < 24 && level) { f->state = 1; f->t = 0; }
                    else if (!f->cd && adx > 60 && adx < 180 && level && f->t % 7 == 0 && rng_range(&eg.rng, 0, 3) == 0) {
                        f->state = 2;
                        f->t = 0;
                    }
                }
            }
            /* no walking off ledges */
            if (f->vx && on_ground(f->x, f->y, 5)) {
                int ahead = fx + isign(f->vx) * 10;
                if (!eko_brawl_solid(ahead >> 4, fy >> 4) && !platform(ahead >> 4, fy >> 4)) f->vx = 0;
            }
        }
        f->vy = imin(f->vy + BR_GRAV, BR_MAX_FALL);
        int vx = f->vx;
        int r = move_body(&f->x, &f->y, &vx, &f->vy, 6, f->kind == BF_RAT ? 6 : 22, false);
        (void)r;
        if (on_ground(f->x, f->y, 5)) { f->last_gx = f->x; f->a = f->y; }
        if (EKO_PX(f->y) > B->mh * 16 + 24) { kill_foe(i, f->chain); continue; }
        if (f->kb == 0 && f->kind != BF_RAT) touch_damage(f, 5, 20, 1);
    }
}

/* ---- the bosses ------------------------------------------------------------------------------- */

void eko_brawl_boss_start(void) {
    if (B->boss_on) return;
    const EkoBrawlMap *bm = &EKO_BMAP[B->map];
    B->boss_on = 1;
    B->lock_x = B->cam_x;
    int kind = bm->boss == BRB_SCRAPJAW ? BF_SCRAPJAW : BF_TALPA;
    int x = (B->lock_x + 250) * EKO_FX;
    int y = (B->cam_y + 40) * EKO_FX;
    B->boss_i = eko_brawl_spawn_foe(kind, x, y);
    if (B->boss_i >= 0) { B->foe[B->boss_i].cd = 90; B->foe[B->boss_i].state = 0; }
    eko_say(kind == BF_SCRAPJAW ? "SCRAPJAW, THE DOCK CROC" : "DR. TALPA AND HIS DRILL");
    eko_sfx("eko_bossin");
    if (!eko_simulating) music_play(EKO_MUS_BOSS);
}

static void boss_update(int i) {
    EkoBFoe *f = &B->foe[i];
    EkoBPlayer *p = &B->p;
    int px = EKO_PX(p->x), fx = EKO_PX(f->x);
    int dx = px - fx;
    if (f->kind == BF_SCRAPJAW) {
        /* states: 0 walk in, 1 hook wind-up, 2 hook swing, 3 dash wind-up, 4 dash, 5 toss */
        switch (f->state) {
        case 0:
            f->face = dx < 0 ? -1 : 1;
            f->vx = f->face * 200;
            if (!f->cd) {
                int r = rng_range(&eg.rng, 0, 2);
                if (iabs(dx) < 40) r = 0;
                f->state = r == 0 ? 1 : r == 1 ? 3 : 5;
                f->t = 0;
                f->vx = 0;
            }
            break;
        case 1:
            f->vx = 0;
            if (f->t > 26) { f->state = 2; f->t = 0; eko_sfx("eko_swing"); }
            break;
        case 2:
            if (f->t < 12) {
                int hx = f->face > 0 ? fx + 8 : fx - 38;
                if (rects_overlap(hx, EKO_PX(f->y) - 26, 30, 20, px - 5, EKO_PX(p->y) - (p->duck ? BR_PH_DUCK : BR_PH), 10, p->duck ? BR_PH_DUCK : BR_PH))
                    hurt_player(1, f->face);
            }
            if (f->t > 30) { f->state = 0; f->cd = 50 + rng_range(&eg.rng, 0, 40); }
            break;
        case 3:
            f->vx = 0;
            f->face = dx < 0 ? -1 : 1;
            if (f->t > 40) { f->state = 4; f->t = 0; eko_sfx("eko_lunge"); }
            break;
        case 4:
            f->vx = f->face * 900;
            if (fx < B->lock_x + 20 || fx > B->lock_x + SCREEN_W - 20 || f->t > 60) {
                f->vx = 0;
                f->state = 0;
                f->cd = 60;
                eg.shake = 8;
                eko_sfx("eko_thud");
            }
            break;
        case 5:
            f->vx = 0;
            if (f->t == 20 || f->t == 40) {
                int frames = 48 + (f->t == 40 ? 16 : 0);
                eko_brawl_add_obj(BO_GRENADE, f->x, f->y - 30 * EKO_FX, iclamp(dx * EKO_FX / frames, -1000, 1000), -1100);
                eko_sfx("eko_toss");
            }
            if (f->t > 60) { f->state = 0; f->cd = 60; }
            break;
        }
    } else {
        /* Dr. Talpa: 0 stalk, 1 burrow (into the floor), 2 under (moves to him), 3 erupt, 4 bolts, 5 hop */
        switch (f->state) {
        case 0:
            f->face = dx < 0 ? -1 : 1;
            f->vx = f->face * 160;
            if (!f->cd) {
                int r = rng_range(&eg.rng, 0, 2);
                f->state = r == 0 ? 1 : r == 1 ? 4 : 5;
                f->t = 0;
                f->vx = 0;
            }
            break;
        case 1:
            f->vx = 0;
            if (f->t > 24) { f->state = 2; f->t = 0; eko_sfx("eko_drill"); }
            break;
        case 2:
            f->face = dx < 0 ? -1 : 1;
            f->vx = f->face * 512;
            if (iabs(dx) < 6 || f->t > 120) { f->vx = 0; f->state = 3; f->t = 0; }
            break;
        case 3:
            if (f->t == 30) {
                eko_sfx("eko_boom");
                eg.shake = 10;
                if (iabs(px - fx) < 22) hurt_player(2, dx < 0 ? -1 : 1);
            }
            if (f->t > 50) { f->state = 0; f->cd = 50; }
            break;
        case 4:
            f->vx = 0;
            if (f->t == 20 || f->t == 32 || f->t == 44) {
                int k = (f->t - 20) / 12;
                eko_brawl_add_obj(BO_FLAME, f->x + f->face * 14 * EKO_FX, f->y - 24 * EKO_FX, f->face * 700, -400 + k * 260);
                eko_sfx("eko_zap");
            }
            if (f->t > 64) { f->state = 0; f->cd = 60; }
            break;
        case 5:
            if (f->t == 1) { f->vy = -1300; f->vx = iclamp(dx * EKO_FX / 50, -700, 700); }
            if (f->t > 6 && on_ground(f->x, f->y, 12)) { f->vx = 0; f->state = 0; f->cd = 40; eg.shake = 6; eko_sfx("eko_thud"); }
            break;
        }
    }
    f->vy = imin(f->vy + BR_GRAV, BR_MAX_FALL);
    int vx = f->vx;
    move_body(&f->x, &f->y, &vx, &f->vy, 13, 30, false);
    int lo = (B->lock_x + 16) * EKO_FX, hi = (B->lock_x + SCREEN_W - 16) * EKO_FX;
    f->x = iclamp(f->x, lo, hi);
    if (!(f->kind == BF_TALPA && (f->state == 2 || (f->state == 3 && f->t < 30)))) touch_damage(f, 12, 28, 1);
}

/* ---- the lift ride ----------------------------------------------------------------------------- */

static void lift_update(void) {
    if (!B->lift_on) return;
    if (B->lift_t < BR_LIFT_T) {
        B->lift_t++;
        B->lift_y += 1;
        while (B->lift_wave < EKO_LIFT_NWAVES && EKO_LIFT_WAVES[B->lift_wave].t <= B->lift_t) {
            const EkoWave *w = &EKO_LIFT_WAVES[B->lift_wave++];
            for (int k = 0; k < w->n; k++) {
                int x = w->x + k * 30;
                if (x > 296) x -= 270;
                if (w->kind == BF_NONE) eko_brawl_add_obj(BO_MANGO, x * EKO_FX, (BR_TOP + 4) * EKO_FX, 0, 0); /* a crate of fruit dropped in */
                else eko_brawl_spawn_foe(w->kind, x * EKO_FX, (BR_TOP + 4) * EKO_FX);
            }
        }
    } else if (!B->boss_on) {
        bool any = false;
        for (int i = 0; i < BR_MAX_FOES; i++) any |= B->foe[i].alive;
        if (!any) {
            eko_say("THE ROOF");
            eko_brawl_boss_start();
        }
    }
}

/* ---- they keep coming, and the stage turns on him ------------------------------------------------- */

/* the ground in column x on the screen (its top, px), or -1 */
static int ground_in(int x) {
    int tx = x >> 4;
    for (int ty = (B->cam_y >> 4) + 1; ty < (B->cam_y >> 4) + BR_SH; ty++) {
        bool floor = eko_brawl_solid(tx, ty) || platform(tx, ty);
        if (floor && !eko_brawl_solid(tx, ty - 1) && !eko_brawl_solid(tx, ty - 2)) return ty * 16;
    }
    return -1;
}

static void respawn_update(void) {
    if ((B->calm & 1) || B->boss_on || B->lift_on || B->map == BM_LIFT || B->done_t || B->wall_open_t) return;
    if (B->respawn_t < BR_RESPAWN_T) { B->respawn_t++; return; }
    int n = 0;
    for (int i = 0; i < BR_MAX_FOES; i++) n += B->foe[i].alive && B->foe[i].kind < BF_SCRAPJAW;
    if (n >= BR_RESPAWN_CAP) return;
    B->respawn_t = 0;
    int k = B->respawns++;
    if (B->map == BM_LAB || k % 3 == 2) {
        /* a jetpacker drops in from above */
        int x = B->cam_x + 40 + (k * 97) % (SCREEN_W - 80);
        eko_brawl_spawn_foe(BF_JETPACK, x * EKO_FX, (B->cam_y + 24) * EKO_FX);
        return;
    }
    /* a trooper walks on, from ahead or from behind, where there is ground */
    int x = k % 2 ? B->cam_x + SCREEN_W + 8 : B->cam_x + 4;
    int y = ground_in(x);
    if (y < 0) { x = B->cam_x + SCREEN_W - 12; y = ground_in(x); }
    if (y < 0) return;
    eko_brawl_spawn_foe(k % 4 == 1 ? BF_GRENADIER : BF_SOLDIER, x * EKO_FX, y * EKO_FX);
}

static void hazards_update(void) {
    if ((B->calm & 2) || B->boss_on) return;
    int px = EKO_PX(B->p.x), py = EKO_PX(B->p.y);
    for (int i = 0; i < B->nspawn; i++) {
        int k = B->spawn[i].kind;
        if (B->spawn[i].used || (k != SPK_DROP && k != SPK_HOOK)) continue;
        int x = B->spawn[i].tx * 16 + 8, y = B->spawn[i].ty * 16 + 16;
        if (y < B->cam_y || y > B->cam_y + BR_SH * 16) continue;
        if (k == SPK_DROP ? iabs(px - x) >= 56 : (x - px >= 140 || x < px || iabs(py - y) > 24)) continue;
        B->spawn[i].used = 1;
        B->hazards++;
        int o = eko_brawl_add_obj(k == SPK_DROP ? BO_DROP : BO_HOOK, x * EKO_FX, (k == SPK_DROP ? B->cam_y : y - 20) * EKO_FX, 0, 0);
        (void)o;
        eko_sfx("eko_warn");
    }
}

/* ---- the camera and the section ------------------------------------------------------------------- */

static void camera_update(bool snap) {
    EkoBPlayer *p = &B->p;
    if (B->lock_x >= 0) { B->cam_x = B->lock_x; return; }
    int tx = EKO_PX(p->x) - 140;
    int ty = EKO_PX(p->y) - 110;
    tx = iclamp(tx, 0, imax(0, B->mw * 16 - SCREEN_W));
    ty = iclamp(ty, 0, imax(0, B->mh * 16 - BR_SH * 16));
    if (B->map != BM_LAB && !snap) tx = imax(tx, B->cam_x); /* no going back */
    if (snap) { B->cam_x = tx; B->cam_y = ty; return; }
    B->cam_x += iclamp(tx - B->cam_x, -6, 6);
    B->cam_y += iclamp(ty - B->cam_y, -4, 4);
}

void eko_brawl_start(int map) {
    load_map(map);
    B->lift_on = EKO_BMAP[map].lift;
    camera_update(true);
    run_spawns();
}

void eko_brawl_respawn(void) {
    EkoBPlayer *p = &B->p;
    int keep_key = p->key;
    int sn = p->atk_sn;
    int sx = p->safe_x, sy = p->safe_y;
    memset(p, 0, sizeof *p);
    p->atk_sn = sn + 1;
    p->key = keep_key;
    p->x = sx;
    p->y = sy;
    p->face = 1;
    p->inv = 120;
    p->safe_x = p->x;
    p->safe_y = p->y;
    /* the barrel he was carrying drops */
    for (int i = 0; i < BR_MAX_OBJS; i++)
        if (B->obj[i].alive && B->obj[i].kind == BO_BARREL && B->obj[i].state == 1) B->obj[i].state = 0;
    camera_update(true);
}

void eko_brawl_update(void) {
    player_update();
    if (B->p.dead_t && B->p.dead_t > 80) return;
    camera_update(false);
    run_spawns();
    respawn_update();
    hazards_update();
    foes_update();
    objs_update();
    lift_update();
    /* the boss's mark */
    if (!B->boss_on && EKO_BMAP[B->map].boss && !B->lift_on) {
        int px = EKO_PX(B->p.x) >> 4, py = (EKO_PX(B->p.y) - 8) >> 4;
        for (int ty = py - 3; ty <= py + 3; ty++)
            for (int tx = (px / BR_SW) * BR_SW; tx < (px / BR_SW + 1) * BR_SW; tx++) /* anywhere in the arena's chunk */
                if (tile_at(tx, ty) == 'B') {
                    /* the arena: the chunk he is in */
                    B->cam_x = (EKO_PX(B->p.x) / (BR_SW * 16)) * BR_SW * 16;
                    B->cam_y = (EKO_PX(B->p.y) - 8) / (BR_SH * 16) * BR_SH * 16;
                    eko_brawl_boss_start();
                    ty = py + 4;
                    break;
                }
    }
    if (B->wall_open_t) {
        B->wall_open_t++;
        if (B->wall_open_t == 60) {
            for (int y = 0; y < B->mh; y++)
                for (int x = 0; x < B->mw; x++)
                    if (B->tile[y][x] == 'W') {
                        B->tile[y][x] = '.';
                        eko_burst(x * 16 + 8, y * 16 + 8, C_ORANGE, 6);
                    }
            eko_sfx("eko_boom");
            eg.shake = 20;
        }
    }
    if (B->boss_dead) {
        B->boss_t++;
        if (B->boss_t > 150 && !B->done_t) B->done_t = 1;
    }
    if (B->done_t) {
        B->done_t++;
        if (B->done_t == 2 && B->gulls > B->gulls_hit) {
            /* every gull left in peace: 100 */
            char m[32];
            snprintf(m, sizeof m, "GULLS LEFT IN PEACE +%d", (B->gulls - B->gulls_hit) * 100);
            eko_add_score((B->gulls - B->gulls_hit) * 100);
            eko_say(m);
            eko_sfx("eko_bonus");
        }
        if (B->done_t > 40) eko_section_done();
    }
}
