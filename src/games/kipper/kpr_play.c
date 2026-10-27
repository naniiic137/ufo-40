/* KIPPER - the sub: swimming, dashing and boosting, fuel, torpedoes,
 * missiles and depth charges, the trail of finds, armor plates, the egg
 * companions, steam vents and the camera. */
#include "kpr.h"

int kpr_torp_damage(void) {
    int d = 3 + kg.cur.boosters;
    return kpr_on(EQ_BUSTER) ? d * 2 + 2 : d;
}
int kpr_torp_range(void) {
    if (kpr_on(EQ_BUSTER)) return 32 + kg.cur.boosters * 2;
    return 40 + kg.cur.boosters * 6;
}

/* ---- fuel ------------------------------------------------------------------ */

/* swimming and dashing burn fuel; Efficient Fuel makes it three quarters */
void kpr_burn(int units) {
    if (kg.god) return;
    if (kpr_on(EQ_EFFICIENT)) {
        kg.fuel_frac += units * 3;
        units = kg.fuel_frac / 4;
        kg.fuel_frac %= 4;
    }
    kg.fuel -= units;
    kg.last_cost += units;
}

void kpr_burst(float x, float y, int col, int n, float sp) {
    for (int i = 0, k = 0; i < KPR_MAX_PARTS && k < n; i++) {
        if (kg.part[i].life > 0) continue;
        KprPart *p = &kg.part[i];
        float a = (float)(k * 6.2831853 / n) + (float)(kg.frame_t % 7) * 0.3f;
        p->x = x;
        p->y = y;
        p->vx = cosf(a) * sp;
        p->vy = sinf(a) * sp;
        p->life = 16 + (k % 3) * 4;
        p->col = col;
        k++;
    }
}

/* a knock: half a bar (an armor plate takes it instead), and a push away */
void kpr_sub_hurt(float fx, float fy, int cost) {
    if (kg.inv > 0 || kg.state != KS_PLAY) return;
    float dx = kg.x - fx, dy = kg.y - fy, d = sqrtf(dx * dx + dy * dy);
    if (d < 0.01f) { dx = (float)-kg.face; dy = 0; d = 1; }
    dx /= d;
    dy /= d;
    kg.vx = dx * 2.5f;
    kg.vy = dy * 2.5f;
    kg.dash_t = 0;
    kg.boosting = false;
    kg.inv = KPR_MERCY;
    kg.hits_taken++;
    if (kg.plates > 0) {
        kg.plates--;
        for (int i = 0; i < KPR_PLATES; i++)
            if (!kg.plate[i].alive) {
                kg.plate[i] = (KprPlate){1, kg.x, kg.y, dx * 1.5f, dy * 1.5f - 0.5f};
                break;
            }
        kpr_sfx("kpr_clank");
        return;
    }
    kpr_sfx("kpr_hurt");
    if (!kg.god) {
        kg.fuel -= cost;
        kg.last_cost += cost;
    }
}

/* ---- the trail -------------------------------------------------------------- */

int kpr_trail_count(int kind) {
    int n = 0;
    for (int i = 0; i < kg.trail_n; i++) n += kg.trail[i].kind == kind;
    return n;
}

void kpr_trail_add(int kind, int id) {
    if (kg.trail_n >= KPR_TRAIL) return;
    kg.trail[kg.trail_n++] = (KprTrailObj){(uint8_t)kind, (int16_t)id};
    if (kind == TR_ITEM && id >= 0 && id < KPR_NITEMS) kg.where[id] = IW_TRAIL;
}

static void trail_remove(int slot) {
    for (int i = slot; i < kg.trail_n - 1; i++) kg.trail[i] = kg.trail[i + 1];
    kg.trail_n--;
}

/* where trail slot i swims: on the sub's wake, 12 px apart */
static void trail_pos(int slot, float *x, float *y) {
    int k = (kg.hist_i - 6 * (slot + 1) + KPR_HIST * 4) % KPR_HIST;
    *x = kg.hx[k];
    *y = kg.hy[k];
}

static void bomb_at(float x, float y, int fuse) {
    for (int i = 0; i < KPR_MAX_BOMBS; i++)
        if (!kg.bomb[i].alive) {
            kg.bomb[i] = (KprBomb){1, x, y, 0, fuse};
            return;
        }
}

void kpr_drop_item_loose(int item, float x, float y) {
    if (item < 0 || item >= KPR_NITEMS) return;
    if (kg.cur.item[item] == IS_CARRIED) kg.cur.item[item] = IS_HOME;   /* an egg towed since a dock, lost again */
    kg.where[item] = IW_LOOSE;
    kg.ix[item] = x;
    kg.iy[item] = y;
}

/* a cloud knocks the last thing in the trail loose (a depth charge goes off) */
int kpr_trail_drop_last(void) {
    if (kg.trail_n <= 0) return -1;
    int s = kg.trail_n - 1;
    KprTrailObj o = kg.trail[s];
    float x, y;
    trail_pos(s, &x, &y);
    trail_remove(s);
    if (o.kind == TR_CHARGE) bomb_at(x, y, 30);
    else kpr_drop_item_loose(o.id, x, y + 4);
    kpr_sfx("kpr_knock");
    return o.kind;
}

/* a filcher swims off with the last find in the trail */
void kpr_steal_from_trail(int f) {
    for (int s = kg.trail_n - 1; s >= 0; s--) {
        KprTrailObj o = kg.trail[s];
        float x, y;
        trail_pos(s, &x, &y);
        trail_remove(s);
        if (o.kind == TR_CHARGE) {
            bomb_at(x, y, 30);       /* a snatched bomb goes off; the filcher swims home empty */
            kg.foe[f].state = 1;
        } else {
            kg.foe[f].carry = o.id;
            if (kg.cur.item[o.id] == IS_CARRIED) kg.cur.item[o.id] = IS_HOME;
            kg.where[o.id] = IW_HELD;
            kg.foe[f].state = 2;
        }
        kpr_sfx("kpr_steal");
        kpr_say_once(1, "HEY! GIVE THAT BACK!");
        return;
    }
}

void kpr_trail_update(void) {
    float lx = kg.hx[kg.hist_i], ly = kg.hy[kg.hist_i];
    float dx = kg.x - lx, dy = kg.y - ly;
    if (dx * dx + dy * dy >= 4.0f) {
        kg.hist_i = (kg.hist_i + 1) % KPR_HIST;
        kg.hx[kg.hist_i] = kg.x;
        kg.hy[kg.hist_i] = kg.y;
    }
}

void kpr_trail_get(int slot, float *x, float *y) { trail_pos(slot, x, y); }

static void reset_history(void) {
    for (int i = 0; i < KPR_HIST; i++) {
        kg.hx[i] = kg.x;
        kg.hy[i] = kg.y - (KPR_HIST - i) * 0.0f;
    }
    kg.hist_i = 0;
}
void kpr_reset_history(void) { reset_history(); }

/* ---- shooting ----------------------------------------------------------------- */

static int add_shot(int kind, float x, float y, float vx, float vy, int life, int dmg) {
    for (int i = 0; i < KPR_MAX_SHOTS; i++)
        if (!kg.shot[i].alive) {
            kg.shot[i] = (KprShot){(uint8_t)kind, 1, 0, x, y, vx, vy, life, dmg};
            return i;
        }
    return -1;
}

static bool nearest_target(float x, float y, float range, float *tx, float *ty, int face) {
    float best = range * range;
    bool found = false;
    for (int i = 0; i < KPR_MAX_FOES; i++) {
        const KprFoe *f = &kg.foe[i];
        if (!f->alive || f->pacified) continue;
        if (f->kind == FK_GLIMMER && f->state == 0) continue;
        float dx = f->x - x, dy = f->y - y;
        if (face && dx * face < 0) continue;
        float d = dx * dx + dy * dy;
        if (d < best) { best = d; *tx = f->x; *ty = f->y; found = true; }
    }
    for (int b = 0; b < KPR_BOSSES; b++) {
        float bx, by;
        int bw, bh;
        if (!kpr_boss_box(b, &bx, &by, &bw, &bh)) continue;
        float dx = bx - x, dy = by - y;
        if (face && dx * face < 0) continue;
        float d = dx * dx + dy * dy;
        if (d < best) { best = d; *tx = bx; *ty = by; found = true; }
    }
    return found;
}

static void companion_pos(int k, float *x, float *y) {
    /* blue fish (1st and 3rd) circle the sub; red ones (2nd, 4th) swim behind */
    if (k % 2 == 0) {
        float a = kg.comp_ang + (k == 2 ? 3.14159f : 0.0f);
        *x = kg.x + cosf(a) * 18.0f;
        *y = kg.y + sinf(a) * 14.0f;
    } else {
        *x = kg.x - kg.face * (14.0f + (k == 3 ? 6.0f : 0.0f));
        *y = kg.y - 10.0f + (k == 3 ? 18.0f : 0.0f) + sinf(kg.comp_ang * 2 + k) * 2.0f;
    }
}
void kpr_companion_pos(int k, float *x, float *y) { companion_pos(k, x, y); }
int kpr_companions(void) { return imin(4, kg.cur.eggs_back / 5); }

static void fire_torpedo(void) {
    bool buster = kpr_on(EQ_BUSTER);
    int range = kpr_torp_range();
    int life = (int)(range / KPR_TORP_SPEED);
    int s = add_shot(buster ? SH_BUSTER : SH_TORP, kg.x + kg.face * 8.0f, kg.y + 1.0f, kg.face * KPR_TORP_SPEED, 0,
                     life, kpr_torp_damage());
    if (s >= 0 && kpr_on(EQ_TARGET)) kg.shot[s].homing = 1;
    kg.fire_t = 0;
    kpr_sfx(buster ? "kpr_buster" : "kpr_torp");
    /* each red fish puffs a bubble at the nearest creature */
    int n = kpr_companions();
    for (int k = 1; k < n; k += 2) {
        float cx, cy, tx = 0, ty = 0;
        companion_pos(k, &cx, &cy);
        float vx = kg.face * 2.5f, vy = 0;
        if (nearest_target(cx, cy, 110, &tx, &ty, 0)) {
            float dx = tx - cx, dy = ty - cy, d = sqrtf(dx * dx + dy * dy);
            if (d > 1) { vx = dx / d * 2.5f; vy = dy / d * 2.5f; }
        }
        add_shot(SH_BUBBLE, cx, cy, vx, vy, 44, 2 + kg.cur.boosters / 3);
    }
}

static bool missile_alive(void) {
    for (int i = 0; i < KPR_MAX_SHOTS; i++)
        if (kg.shot[i].alive && kg.shot[i].kind == SH_MISSILE) return true;
    return false;
}

static void blast_damage(float x, float y, int radius, int foe_dmg, int boss_dmg, bool hurts_sub) {
    for (int i = 0; i < KPR_MAX_FOES; i++) {
        KprFoe *f = &kg.foe[i];
        if (!f->alive || f->pacified) continue;
        float dx = f->x - x, dy = f->y - y;
        if (dx * dx + dy * dy <= (float)((radius + 6) * (radius + 6))) kpr_hurt_foe(i, foe_dmg, true);
    }
    for (int b = 0; b < KPR_BOSSES; b++) {
        float bx, by;
        int bw, bh;
        if (!kpr_boss_box(b, &bx, &by, &bw, &bh)) continue;
        float dx = fabsf(bx - x) - bw / 2.0f, dy = fabsf(by - y) - bh / 2.0f;
        if (dx < 0) dx = 0;
        if (dy < 0) dy = 0;
        if (dx * dx + dy * dy <= (float)(radius * radius)) kpr_boss_hurt(b, boss_dmg);
    }
    if (hurts_sub) {
        float dx = kg.x - x, dy = kg.y - y;
        if (dx * dx + dy * dy <= (float)(radius * radius)) kpr_sub_hurt(x, y, KPR_HIT);
    }
}

static void explode_missile(KprShot *s) {
    s->alive = 0;
    kpr_blast(s->x, s->y, 28);
    blast_damage(s->x, s->y, 28, 20, 60, false);
    kpr_burst(s->x, s->y, C_ORANGE, 12, 2.0f);
    kpr_burst(s->x, s->y, C_YELLOW, 8, 1.2f);
    kg.shake = imax(kg.shake, 6);
    kpr_sfx("kpr_boom");
}

/* a torpedo (or bubble) meets a wall tile: cracked walls crack, a buster
 * torpedo lights a switch */
static void shot_hits_tile(KprShot *s) {
    int tx = (int)floorf(s->x / KPR_TILE), ty = (int)floorf(s->y / KPR_TILE);
    int c = kpr_tile(tx, ty);
    if (c == '?') {
        int b = kpr_brk_index(tx, ty);
        if (b >= 0 && ++kg.crack[b] >= KPR_CRACK_HITS) kpr_break_tile(tx, ty);
        kpr_sfx("kpr_crack");
    } else if (c >= 'a' && c <= 'h') {
        if (s->kind == SH_BUSTER) {
            int gate = c - 'a';
            if (!((kg.gates >> gate) & 1)) {
                kg.gates |= (uint8_t)(1 << gate);
                kpr_sfx("kpr_switch");
                kpr_say("A SWITCH! SOMETHING OPENED.");
            }
        } else {
            kpr_sfx("kpr_tink");
            kpr_say_once(2, "IT WON'T BUDGE. IT NEEDS A HARDER HIT.");
        }
    } else {
        kpr_sfx("kpr_tink");
    }
    kpr_burst(s->x, s->y, C_WHITE, 3, 0.8f);
    s->alive = 0;
}

void kpr_shots_update(void) {
    for (int i = 0; i < KPR_MAX_SHOTS; i++) {
        KprShot *s = &kg.shot[i];
        if (!s->alive) continue;
        if (s->kind == SH_MISSILE) {
            s->vy = fmaxf(s->vy - 0.08f, -4.0f);
            s->y += s->vy;
            if (--s->life <= 0 || kpr_shot_solid(s->x, s->y - 3)) { explode_missile(s); continue; }
        } else {
            if (s->homing) {
                float tx, ty;
                if (nearest_target(s->x, s->y, 96, &tx, &ty, s->vx > 0 ? 1 : -1)) {
                    float dy = ty - s->y;
                    s->vy = fclamp(s->vy + (dy > 0 ? 0.18f : -0.18f), -2.2f, 2.2f);
                }
            }
            s->x += s->vx;
            s->y += s->vy;
            if (--s->life <= 0) { s->alive = 0; continue; }
            if (kpr_shot_solid(s->x, s->y)) { shot_hits_tile(s); continue; }
        }
        /* creatures */
        for (int f = 0; f < KPR_MAX_FOES && s->alive; f++) {
            KprFoe *o = &kg.foe[f];
            if (!o->alive || o->pacified) continue;
            int w, h;
            kpr_foe_size(o->kind, &w, &h);
            if (fabsf(o->x - s->x) * 2 > w + 4 || fabsf(o->y - s->y) * 2 > h + 4) continue;
            if (s->kind == SH_MISSILE) { explode_missile(s); break; }
            kpr_hurt_foe(f, s->dmg, s->kind == SH_BUSTER);
            s->alive = 0;
        }
        for (int b = 0; b < KPR_BOSSES && s->alive; b++) {
            float bx, by;
            int bw, bh;
            if (!kpr_boss_box(b, &bx, &by, &bw, &bh)) continue;
            if (fabsf(bx - s->x) * 2 > bw + 2 || fabsf(by - s->y) * 2 > bh + 2) continue;
            if (s->kind == SH_MISSILE) { explode_missile(s); break; }
            if (kpr_boss_vulnerable(b)) kpr_boss_hurt(b, s->dmg);
            else kpr_sfx("kpr_tink");
            kpr_burst(s->x, s->y, C_WHITE, 3, 0.8f);
            s->alive = 0;
        }
    }
}

/* ---- depth charges ------------------------------------------------------------ */

static void drop_charge(void) {
    for (int s = kg.trail_n - 1; s >= 0; s--)
        if (kg.trail[s].kind == TR_CHARGE) {
            float x, y;
            trail_pos(s, &x, &y);
            trail_remove(s);
            bomb_at(x, y + 2, 150);
            kpr_sfx("kpr_drop");
            return;
        }
}

void kpr_bombs_update(void) {
    for (int i = 0; i < KPR_MAX_BOMBS; i++) {
        KprBomb *b = &kg.bomb[i];
        if (!b->alive) continue;
        if (!kpr_shot_solid(b->x, b->y + 5)) b->y += 0.45f;
        if (--b->fuse <= 0) {
            b->alive = 0;
            kpr_blast(b->x, b->y, 44);
            blast_damage(b->x, b->y, 44, 40, 350, true);
            kpr_burst(b->x, b->y, C_ORANGE, 16, 2.6f);
            kpr_burst(b->x, b->y, C_YELLOW, 12, 1.6f);
            kpr_burst(b->x, b->y, C_WHITE, 8, 0.8f);
            kg.shake = imax(kg.shake, 12);
            kpr_sfx("kpr_bigboom");
            continue;
        }
    }
}

/* ---- steam vents ------------------------------------------------------------------ */

bool kpr_vent_on(int v) { return (kg.frame_t + v * 37) % 180 >= 120; }

void kpr_vents_update(void) {
    for (int v = 0; v < KPR_NVENTS; v++) {
        if (!kpr_vent_on(v)) continue;
        float vx = KPR_VENTS[v].x * KPR_TILE + 8.0f, vy = (KPR_VENTS[v].y + 1) * KPR_TILE;
        if (fabsf(kg.x - vx) < 6 + KPR_SW / 2 && kg.y > vy - 56 && kg.y < vy) kpr_sub_hurt(vx, kg.y + 6, KPR_HIT);
    }
}

/* ---- the pick-ups ----------------------------------------------------------------- */

static const char *const PICK_LINE[3] = {"A FUEL TANK! GET IT HOME.", "A TORPEDO BOOSTER! GET IT HOME.",
                                         "AN EGG! AUNTIE ROE WILL WANT THIS."};

void kpr_items_update(void) {
    for (int i = 0; i < KPR_NITEMS; i++) {
        if (kg.cur.item[i] != IS_HOME) continue;
        int w = kg.where[i];
        if (w != IW_HOME && w != IW_LOOSE) continue;
        float x = w == IW_HOME ? KPR_ITEMS[i].x * KPR_TILE + 8.0f : kg.ix[i];
        float y = w == IW_HOME ? KPR_ITEMS[i].y * KPR_TILE + 8.0f : kg.iy[i];
        float dx = x - kg.x, dy = y - kg.y;
        if (dx * dx + dy * dy > 13.0f * 13.0f) continue;
        if (kg.trail_n >= KPR_TRAIL) {
            kpr_say_once(3, "NO ROOM! THREE THINGS IS ALL I CAN TOW.");
            continue;
        }
        kpr_trail_add(TR_ITEM, i);
        kpr_sfx("kpr_pick");
        const KprItemDef *d = &KPR_ITEMS[i];
        if (d->kind == IK_EQUIP) {
            char m[48];
            snprintf(m, sizeof m, "%s! GET IT HOME.", KPR_EQ_NAME[d->var]);
            kpr_say(m);
        } else kpr_say(PICK_LINE[d->kind]);
    }
    /* knocked-off armor plates */
    for (int i = 0; i < KPR_PLATES; i++) {
        KprPlate *p = &kg.plate[i];
        if (!p->alive) continue;
        p->x += p->vx;
        p->y += p->vy;
        p->vx *= 0.95f;
        p->vy = p->vy * 0.95f + 0.01f;
        if (kpr_shot_solid(p->x, p->y)) { p->x -= p->vx; p->y -= p->vy; p->vx = p->vy = 0; }
        float dx = p->x - kg.x, dy = p->y - kg.y;
        if (kg.inv < KPR_MERCY - 20 && dx * dx + dy * dy < 144.0f && kg.plates < KPR_PLATES) {
            p->alive = 0;
            kg.plates++;
            kpr_sfx("kpr_clank");
        }
    }
    /* fuel left by a pacified creature */
    for (int i = 0; i < 8; i++) {
        if (kg.fuel_drop_t[i] <= 0) continue;
        kg.fuel_drop_t[i]--;
        kg.fuel_drop_y[i] -= 0.1f;
        float dx = kg.fuel_drop_x[i] - kg.x, dy = kg.fuel_drop_y[i] - kg.y;
        if (dx * dx + dy * dy < 144.0f) {
            kg.fuel_drop_t[i] = 0;
            kg.fuel = imin(kpr_bars_max() * KPR_BAR, kg.fuel + KPR_BAR / 2);
            kpr_sfx("kpr_fuel");
        }
    }
}

/* ---- the sub ----------------------------------------------------------------- */

bool kpr_drilling(void) { return kpr_on(EQ_DRILL) && (kg.dash_t > 0 || kg.boosting); }
static bool drilling(void) { return kpr_on(EQ_DRILL) && (kg.dash_t > 0 || kg.boosting); }

/* move along one axis; dirt gives way to a drilling dash, a cracked wall
 * to rubbing */
static void move_axis(float dx, float dy, bool *rubbed) {
    float nx = kg.x + dx, ny = kg.y + dy;
    if (!kpr_box_solid(nx, ny, KPR_SW, KPR_SH)) { kg.x = nx; kg.y = ny; return; }
    int x0 = (int)floorf((nx - KPR_SW / 2.0f) / KPR_TILE), x1 = (int)floorf((nx + KPR_SW / 2.0f - 0.01f) / KPR_TILE);
    int y0 = (int)floorf((ny - KPR_SH / 2.0f) / KPR_TILE), y1 = (int)floorf((ny + KPR_SH / 2.0f - 0.01f) / KPR_TILE);
    bool dirt_only = true;
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            if (!kpr_solid_tile(x, y)) continue;
            int c = kpr_tile(x, y);
            if (c != 'd') dirt_only = false;
            if (c == '?') *rubbed = true;
        }
    if (dirt_only && drilling()) {
        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++)
                if (kpr_tile(x, y) == 'd') kpr_break_tile(x, y);
        kpr_sfx("kpr_drill");
        kg.x = nx;
        kg.y = ny;
        return;
    }
    /* a corner just clipped: slide round it (up to 6 px either way) */
    for (int k = 1; k <= 6; k++) {
        for (int sgn = -1; sgn <= 1; sgn += 2) {
            float ox = dx == 0 ? sgn * (float)k : 0, oy = dy == 0 ? sgn * (float)k : 0;
            if (!kpr_box_solid(nx + ox, ny + oy, KPR_SW, KPR_SH) && !kpr_box_solid(kg.x + ox, kg.y + oy, KPR_SW, KPR_SH)) {
                float sx = ox > 0 ? 0.5f : ox < 0 ? -0.5f : 0, sy = oy > 0 ? 0.5f : oy < 0 ? -0.5f : 0;
                if (!kpr_box_solid(kg.x + sx, kg.y + sy, KPR_SW, KPR_SH)) {
                    kg.x += sx;
                    kg.y += sy;
                    return;
                }
            }
        }
    }
    if (dx != 0) kg.vx = 0;
    if (dy != 0) kg.vy = 0;
    if (kg.dash_t > 0 && fabsf(dx) + fabsf(dy) > 1.5f) kpr_sfx("kpr_bump");
}

static void rub_walls(void) {
    /* every cracked tile the sub's box presses on gives */
    int x0 = (int)floorf((kg.x - KPR_SW / 2.0f - 1) / KPR_TILE), x1 = (int)floorf((kg.x + KPR_SW / 2.0f + 1) / KPR_TILE);
    int y0 = (int)floorf((kg.y - KPR_SH / 2.0f - 1) / KPR_TILE), y1 = (int)floorf((kg.y + KPR_SH / 2.0f + 1) / KPR_TILE);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            if (kpr_tile(x, y) == '?') kpr_break_tile(x, y);
    kpr_sfx("kpr_crack");
}

void kpr_sub_update(void) {
    int dx = (kpr_btn(BTN_RIGHT) ? 1 : 0) - (kpr_btn(BTN_LEFT) ? 1 : 0);
    int dy = (kpr_btn(BTN_DOWN) ? 1 : 0) - (kpr_btn(BTN_UP) ? 1 : 0);
    bool a = kpr_btn(BTN_A), b = kpr_btn(BTN_B);
    if (kg.inv > 0) kg.inv--;
    if (dx || dy) { kg.last_dir_x = dx; kg.last_dir_y = dy; }

    /* the fire button: held fire keeps the facing; tapping is quicker */
    kg.fire_t++;
    if (!a && dx) kg.face = dx;
    if (kpr_btnp(BTN_A)) {
        kg.fire_hold = 0;
        if (kg.fire_t >= KPR_FIRE_TAP) fire_torpedo();
    } else if (a) {
        kg.fire_hold++;
        if (kg.fire_t >= KPR_FIRE_HOLD) fire_torpedo();
        if (kpr_on(EQ_MISSILE) && kg.fire_hold >= KPR_MISSILE_HOLD && !missile_alive() && !kg.missile_ready) {
            kg.missile_ready = true;
            kpr_sfx("kpr_ready");
        }
    }
    if (kpr_btnr(BTN_A) && kg.missile_ready) {
        kg.missile_ready = false;
        add_shot(SH_MISSILE, kg.x, kg.y - 6, 0, -1.5f, 150, 20);
        kpr_sfx("kpr_missile");
    }
    if (!a) { kg.fire_hold = 0; kg.missile_ready = false; }
    kg.firing = a;

    /* the dash button: a dash; held, the super booster; sitting still with
     * depth charges, a charge */
    bool still = !dx && !dy;
    bool can_charge = kpr_on(EQ_CHARGE) && kpr_trail_count(TR_CHARGE) > 0;
    if (kpr_btnp(BTN_B)) {
        kg.b_hold = 0;
        if (still && can_charge) kg.b_still = 1;
        else {
            kg.b_still = 0;
            float ddx = dx, ddy = dy;
            if (still) ddx = (float)kg.face;
            float l = sqrtf(ddx * ddx + ddy * ddy);
            kg.dash_dx = ddx / l;
            kg.dash_dy = ddy / l;
            kg.dash_t = KPR_DASH_T;
            kpr_burn(KPR_DASH_COST);
            kpr_sfx("kpr_dash");
        }
    } else if (b) {
        kg.b_hold++;
        if (kg.b_still > 0) {
            /* held sitting still: a bomb readies; touching the pad calls it off */
            if (!still) {
                kg.b_still = -1;
                kg.charge_ready = false;
            } else if (++kg.b_still > KPR_CHARGE_HOLD && !kg.charge_ready) {
                kg.charge_ready = true;
                kpr_sfx("kpr_ready");
            }
        }
    }
    if (kpr_btnr(BTN_B) && kg.b_still > 0) {
        if (kg.charge_ready) drop_charge();      /* let go: the bomb drops */
        else {
            /* let go before it readied: a dash after all */
            kg.dash_dx = (float)kg.face;
            kg.dash_dy = 0;
            kg.dash_t = KPR_DASH_T;
            kpr_burn(KPR_DASH_COST);
            kpr_sfx("kpr_dash");
        }
        kg.b_still = 0;
        kg.charge_ready = false;
    }
    if (!b) { kg.b_still = 0; kg.charge_ready = false; }
    kg.boosting = b && kpr_on(EQ_BOOSTER) && kg.b_still == 0 && kg.dash_t == 0 && kg.b_hold >= KPR_DASH_T;

    /* swimming */
    float tx = 0, ty = 0;
    if (dx || dy) {
        float l = (dx && dy) ? 0.7071f : 1.0f;
        tx = dx * l * KPR_SPEED;
        ty = dy * l * KPR_SPEED;
    }
    if (kg.dash_t > 0) {
        float k = (float)kg.dash_t / KPR_DASH_T;
        kg.vx = kg.dash_dx * (KPR_SPEED + (KPR_DASH_SPEED - KPR_SPEED) * k);
        kg.vy = kg.dash_dy * (KPR_SPEED + (KPR_DASH_SPEED - KPR_SPEED) * k);
        kg.dash_t--;
    } else if (kg.boosting) {
        float bx = dx, by = dy;
        if (!dx && !dy) { bx = kg.dash_dx; by = kg.dash_dy; }
        float l = sqrtf(bx * bx + by * by);
        if (l > 0) {
            kg.dash_dx = bx / l;
            kg.dash_dy = by / l;
        }
        kg.vx = kg.dash_dx * KPR_BOOST_SPEED;
        kg.vy = kg.dash_dy * KPR_BOOST_SPEED;
        kpr_burn(KPR_BOOST_COST);
    } else {
        kg.vx = fapproach(kg.vx, tx, (dx ? KPR_ACCEL : KPR_DRAG));
        kg.vy = fapproach(kg.vy, ty, (dy ? KPR_ACCEL : KPR_DRAG));
    }
    kg.moving = dx || dy || kg.dash_t > 0 || kg.boosting;

    bool rubbed = false;
    float ox = kg.x, oy = kg.y;
    move_axis(kg.vx, 0, &rubbed);
    move_axis(0, kg.vy, &rubbed);
    /* the surface: the sub can't leave the water */
    float top = KPR_SKY * KPR_TILE + KPR_SH / 2.0f;
    if (kg.y < top) { kg.y = top; if (kg.vy < 0) kg.vy = 0; }
    /* swimming burns fuel; pushing on a wall or sitting still doesn't */
    if ((dx || dy) && kg.dash_t == 0 && !kg.boosting && (fabsf(kg.x - ox) > 0.05f || fabsf(kg.y - oy) > 0.05f)) kpr_burn(1);
    if (rubbed && (dx || dy)) {
        if (++kg.rub_t >= KPR_RUB) { rub_walls(); kg.rub_t = 0; }
    } else if (!rubbed) kg.rub_t = 0;

    /* docking: nose up against a dock */
    if (dy < 0 && kg.y <= top + 1) {
        for (int d = 0; d < KPR_NDOCKS; d++) {
            float cx = KPR_DOCKS[d].x * KPR_TILE + 8.0f;
            if (fabsf(kg.x - cx) <= 16) { kpr_dock_at(d); return; }
        }
    }
    /* Auntie Roe */
    {
        float rx = KPR_ROE.x * KPR_TILE + 8.0f, ry = KPR_ROE.y * KPR_TILE + 8.0f;
        float ddx = kg.x - rx, ddy = kg.y - ry;
        if (ddx * ddx + ddy * ddy < 20.0f * 20.0f) kpr_meet_roe();
    }
    if (kg.state != KS_PLAY) return;
    kg.comp_ang += 0.06f;

    /* blue fish stop shots */
    int n = kpr_companions();
    for (int k = 0; k < n; k += 2) {
        float cx, cy;
        companion_pos(k, &cx, &cy);
        for (int i = 0; i < KPR_MAX_ESHOTS; i++) {
            KprEShot *e = &kg.es[i];
            if (!e->alive) continue;
            if (fabsf(e->x - cx) < 7 && fabsf(e->y - cy) < 7) { e->alive = 0; kpr_burst(e->x, e->y, C_SKY, 3, 0.6f); }
        }
    }
    if (kg.fuel <= 0) kpr_run_dry();
}

/* ---- the camera ------------------------------------------------------------------- */

void kpr_camera(bool snap) {
    int tx = (int)kg.x - KPR_VIEW_W / 2, ty = (int)kg.y - KPR_VIEW_H / 2;
    tx = iclamp(tx, 0, KPR_PW - KPR_VIEW_W);
    ty = iclamp(ty, 0, KPR_PH - KPR_VIEW_H);
    if (snap) { kg.cam_x = tx; kg.cam_y = ty; return; }
    kg.cam_x += (tx - kg.cam_x) / 4 + isign(tx - kg.cam_x);
    kg.cam_y += (ty - kg.cam_y) / 4 + isign(ty - kg.cam_y);
    if (iabs(tx - kg.cam_x) <= 1) kg.cam_x = tx;
    if (iabs(ty - kg.cam_y) <= 1) kg.cam_y = ty;
}
