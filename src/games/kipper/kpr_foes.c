/* KIPPER - the sea's creatures. None of them can be killed: enough stun
 * torpedoes pacify one (it turns small and blue and leaves you be) until
 * the sub docks or swims into another area. Every touch and every shot
 * costs half a bar. */
#include "kpr.h"

const char *const KPR_FOE_NAME[FK_KINDS] = {
    "POLYP", "FILCHER", "GLIMMER", "GREENLING", "SHELLBACK", "KITERAY", "BLUEBELL", "REDCAP", "LANTERNMAW",
    "RISER", "PIPEHORSE", "BLOATER", "STILTCRAB", "SUNSTAR", "BILLFISH", "BURR", "SNAPWORM", "FIZZWORM",
};

static const struct { int8_t w, h; int16_t hp; } SPEC[FK_KINDS] = {
    {14, 12, 12}, {10, 8, 4}, {14, 10, 10}, {12, 8, 6}, {14, 12, 14}, {20, 10, 16}, {12, 12, 6}, {14, 14, 30},
    {18, 14, 24}, {10, 16, 12}, {10, 14, 10}, {12, 12, 14}, {16, 12, 16}, {12, 12, 8}, {22, 8, 16}, {12, 12, 10},
    {10, 12, 14}, {10, 10, 12},
};

void kpr_foe_size(int kind, int *w, int *h) {
    *w = SPEC[kind].w;
    *h = SPEC[kind].h;
}

static bool solid_at(float x, float y) { return kpr_shot_solid(x, y); }
static bool box_free(float x, float y, int w, int h) { return !kpr_box_solid(x, y, w, h); }

/* move with walls in the way; returns true if something stopped it */
static bool move_foe(KprFoe *f, float dx, float dy) {
    int w, h;
    kpr_foe_size(f->kind, &w, &h);
    bool hit = false;
    if (dx != 0) {
        if (box_free(f->x + dx, f->y, w, h)) f->x += dx;
        else hit = true;
    }
    if (dy != 0) {
        if (box_free(f->x, f->y + dy, w, h)) f->y += dy;
        else hit = true;
    }
    return hit;
}

static void aim(float fx, float fy, float sp, float *vx, float *vy) {
    float dx = kg.x - fx, dy = kg.y - fy, d = sqrtf(dx * dx + dy * dy);
    if (d < 1) d = 1;
    *vx = dx / d * sp;
    *vy = dy / d * sp;
}

int kpr_add_eshot(int kind, float x, float y, float vx, float vy) {
    for (int i = 0; i < KPR_MAX_ESHOTS; i++)
        if (!kg.es[i].alive) {
            kg.es[i] = (KprEShot){(uint8_t)kind, 1, x, y, vx, vy, kind == ES_CLOUD ? 70 : 150};
            return i;
        }
    return -1;
}

static void shoot_at_sub(const KprFoe *f, float sp) {
    float vx, vy;
    aim(f->x, f->y, sp, &vx, &vy);
    kpr_add_eshot(ES_PELLET, f->x, f->y, vx, vy);
    kpr_sfx("kpr_spit");
}

int kpr_spawn_foe(int kind, float x, float y) {
    for (int i = KPR_NFOES; i < KPR_MAX_FOES; i++)
        if (!kg.foe[i].alive) {
            KprFoe *f = &kg.foe[i];
            memset(f, 0, sizeof *f);
            f->kind = (uint8_t)kind;
            f->alive = 1;
            f->hp = f->maxhp = SPEC[kind].hp;
            f->def = -1;
            f->x = f->hx = x;
            f->y = f->hy = y;
            f->carry = -1;
            f->owner = -1;
            f->child = -1;
            f->dir = 1;
            return i;
        }
    return -1;
}

/* every creature back where it lives, awake: at a dock and on changing area */
void kpr_foes_reset(void) {
    memset(kg.foe, 0, sizeof kg.foe);
    memset(kg.es, 0, sizeof kg.es);
    for (int i = 0; i < KPR_NFOES && i < KPR_MAX_FOES; i++) {
        const KprFoeDef *d = &KPR_FOES[i];
        KprFoe *f = &kg.foe[i];
        f->kind = d->kind;
        f->alive = 1;
        f->hp = f->maxhp = SPEC[d->kind].hp;
        f->def = (int16_t)i;
        f->x = f->hx = d->x * KPR_TILE + 8.0f;
        f->y = f->hy = d->y * KPR_TILE + 8.0f;
        f->carry = -1;
        f->owner = -1;
        f->child = -1;
        f->dir = (i & 1) ? 1 : -1;
        f->t = i * 13;
        if (d->kind == FK_SNAPWORM) {
            /* it hides in the nearest wall: which way out of it */
            static const int D[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            for (int k = 0; k < 4; k++)
                if (kpr_solid_tile(d->x + D[k][0], d->y + D[k][1])) { f->a = -D[k][0]; f->b = -D[k][1]; break; }
        }
    }
    /* stolen things go home with them */
    for (int i = 0; i < KPR_NITEMS; i++)
        if (kg.where[i] == IW_HELD) kg.where[i] = IW_HOME;
}

int kpr_foes_awake(void) {
    int n = 0;
    for (int i = 0; i < KPR_MAX_FOES; i++) n += kg.foe[i].alive && !kg.foe[i].pacified;
    return n;
}
int kpr_count_foes(int kind, bool pacified) {
    int n = 0;
    for (int i = 0; i < KPR_MAX_FOES; i++)
        n += kg.foe[i].alive && kg.foe[i].kind == kind && (pacified ? kg.foe[i].pacified : !kg.foe[i].pacified);
    return n;
}

static void pacify(int i) {
    KprFoe *f = &kg.foe[i];
    f->pacified = 1;
    f->vx = f->vy = 0;
    f->state = 0;
    if (f->carry >= 0) {
        kpr_drop_item_loose(f->carry, f->x, f->y);
        f->carry = -1;
    }
    kpr_burst(f->x, f->y, C_SKY, 8, 1.0f);
    kpr_sfx("kpr_calm");
    uint32_t h = (uint32_t)kg.frame_t * 2654435761u ^ (uint32_t)(i * 40503);
    if ((h >> 7) % 100 < KPR_FOE_DROP) {
        for (int k = 0; k < 8; k++)
            if (kg.fuel_drop_t[k] <= 0) {
                kg.fuel_drop_x[k] = f->x;
                kg.fuel_drop_y[k] = f->y;
                kg.fuel_drop_t[k] = 600;
                break;
            }
    }
}

void kpr_hurt_foe(int i, int dmg, bool buster) {
    KprFoe *f = &kg.foe[i];
    if (!f->alive || f->pacified) return;
    if (f->kind == FK_SNAPWORM && f->state < 2) return;         /* tucked in the wall */
    if (f->kind == FK_SHELLBACK && f->state == 1 && !buster) {  /* in its shell */
        kpr_sfx("kpr_tink");
        return;
    }
    f->hp -= (int16_t)dmg;
    f->flash = 6;
    kpr_sfx("kpr_hit");
    if (f->kind == FK_SHELLBACK && f->state == 0 && !buster && f->hp > 0) {
        /* stung: into the shell, and if the sub is close it comes flying */
        f->state = 1;
        f->t = 0;
    }
    if (f->kind == FK_BLOATER && f->state == 0) { f->state = 1; f->t = 0; }
    if (f->kind == FK_GLIMMER) f->state = imax(f->state, 30);
    if (f->hp <= 0) pacify(i);
}

/* ---- each kind ------------------------------------------------------------ */

static float sub_dist2(const KprFoe *f) {
    float dx = kg.x - f->x, dy = kg.y - f->y;
    return dx * dx + dy * dy;
}

static void foe_update(int i) {
    KprFoe *f = &kg.foe[i];
    f->t++;
    if (f->flash > 0) f->flash--;
    if (f->pacified) {
        f->y += sinf(f->t * 0.05f) * 0.1f;
        return;
    }
    float d2 = sub_dist2(f);
    float dx = kg.x - f->x, dy = kg.y - f->y;
    switch (f->kind) {
    case FK_POLYP:
        if (f->t % 200 == 0 && d2 < 112.0f * 112.0f) shoot_at_sub(f, 0.9f);
        if (f->child >= 0 && (!kg.foe[f->child].alive || kg.foe[f->child].owner != i)) f->child = -1;
        if (f->child < 0 && kg.trail_n > 0 && d2 < 170.0f * 170.0f && f->t % 200 == 100) {
            int c = kpr_spawn_foe(FK_FILCHER, f->x, f->y - 6);
            if (c >= 0) {
                kg.foe[c].owner = (int16_t)i;
                f->child = (int16_t)c;
            }
        }
        break;
    case FK_FILCHER: {
        if (f->state == 0) {
            /* after the last thing in the trail */
            if (kg.trail_n <= 0) { f->state = 1; break; }
            float tx, ty;
            kpr_trail_get(kg.trail_n - 1, &tx, &ty);
            float ex = tx - f->x, ey = ty - f->y, e = sqrtf(ex * ex + ey * ey);
            if (e < 9) kpr_steal_from_trail(i);
            else move_foe(f, ex / e * 1.35f, ey / e * 1.35f);
        } else {
            /* home to its polyp, with or without the loot */
            const KprFoe *o = f->owner >= 0 ? &kg.foe[f->owner] : NULL;
            float hx = o ? o->x : f->hx, hy = o ? o->y - 6 : f->hy;
            float ex = hx - f->x, ey = hy - f->y, e = sqrtf(ex * ex + ey * ey);
            if (e > 2) {
                f->x += ex / e * 1.1f;
                f->y += ey / e * 1.1f;
            } else if (f->carry < 0 && f->state == 1) {
                f->alive = 0;                 /* back inside, empty-handed */
            }
        }
        break;
    }
    case FK_GLIMMER:
        if (f->state > 0) f->state--;
        if (move_foe(f, f->dir * 0.8f, 0) || fabsf(f->x - f->hx) > 90) {
            f->dir = -f->dir;
            f->state = 40;                    /* seen only against the wall */
            if (fabsf(f->x - f->hx) > 90) f->state = 0;
        }
        break;
    case FK_GREENLING:
        if (move_foe(f, f->dir * 0.7f, 0)) f->dir = -f->dir;
        move_foe(f, 0, sinf(f->t * 0.03f) * 0.3f);
        break;
    case FK_SHELLBACK:
        if (f->state == 0) {
            /* back and forth along its floor, shooting at the sub */
            if (!solid_at(f->x, f->y + 7)) move_foe(f, 0, 0.8f);
            if (move_foe(f, f->dir * 0.4f, 0) || fabsf(f->x - f->hx) > 64 || !solid_at(f->x + f->dir * 8, f->y + 9))
                f->dir = -f->dir;
            if (f->t % 160 == 60 && d2 < 140.0f * 140.0f) shoot_at_sub(f, 1.1f);
            if (f->t % 330 == 300) { f->state = 1; f->t = 0; }
        } else if (f->state == 1) {
            if (f->t == 1 && d2 < 52.0f * 52.0f) {
                f->state = 2;
                f->t = 0;
                aim(f->x, f->y, 3.0f, &f->vx, &f->vy);
            } else if (f->t > 120) { f->state = 0; f->t = 1; }
        } else if (f->state == 2) {
            /* flung at the sub, off the walls */
            if (move_foe(f, f->vx, 0)) f->vx = -f->vx;
            if (move_foe(f, 0, f->vy)) f->vy = -f->vy;
            if (f->t > 90) { f->state = 3; f->t = 0; }
        } else {
            if (move_foe(f, 0, 0.7f) || f->t > 120) { f->state = 0; f->t = 1; }
        }
        break;
    case FK_KITERAY:
        if (move_foe(f, f->dir * 0.9f, 0) || fabsf(f->x + f->dir * 0.9f - f->hx) > 96) f->dir = -f->dir;
        move_foe(f, 0, cosf(f->t * 0.025f) * 0.45f);
        break;
    case FK_BLUEBELL:
    case FK_REDCAP:
        if (d2 < 160.0f * 160.0f) {
            float vx, vy;
            aim(f->x, f->y, 0.3f, &vx, &vy);
            move_foe(f, vx, vy);
        } else move_foe(f, 0, sinf(f->t * 0.04f) * 0.2f);
        break;
    case FK_LANTERNMAW:
        if (f->state == 0) {
            f->dir = dx > 0 ? 1 : -1;
            move_foe(f, 0, sinf(f->t * 0.03f) * 0.15f);
            if (d2 < 72.0f * 72.0f) { f->state = 1; f->t = 0; aim(f->x, f->y, 2.6f, &f->vx, &f->vy); kpr_sfx("kpr_snap"); }
        } else if (f->state == 1) {
            move_foe(f, f->vx, f->vy);
            if (f->t > 28) { f->state = 2; f->t = 0; }
        } else {
            float ex = f->hx - f->x, ey = f->hy - f->y, e = sqrtf(ex * ex + ey * ey);
            if (e > 1.5f) { f->x += ex / e * 0.8f; f->y += ey / e * 0.8f; }
            else if (f->t > 60) f->state = 0;
        }
        break;
    case FK_RISER:
        if (f->state == 0) {
            if (fabsf(dx) < 26 && dy < 0 && dy > -140) { f->state = 1; f->t = 0; kpr_sfx("kpr_whoosh"); }
            else if (f->y < f->hy) move_foe(f, 0, 0.5f);
        } else {
            if (move_foe(f, 0, -3.0f) || f->t > 40) { f->state = 0; f->t = 0; }
        }
        break;
    case FK_PIPEHORSE: {
        float d = sqrtf(d2);
        if (d < 300) {
            float k = d < 76 ? -0.8f : d > 110 ? 0.5f : 0.0f;
            if (d > 1) move_foe(f, dx / d * k, dy / d * k + sinf(f->t * 0.07f) * 0.5f);
            f->dir = dx > 0 ? 1 : -1;
            if (f->t % 160 == 0 && d < 150) shoot_at_sub(f, 1.0f);
        }
        break;
    }
    case FK_BLOATER:
        if (f->state == 0) {
            move_foe(f, 0, sinf(f->t * 0.03f) * 0.2f);
            if (d2 < 44.0f * 44.0f) { f->state = 1; f->t = 0; }
        } else if (f->state == 1) {
            if (f->t == 1) {
                /* puffed: shots and clouds every way */
                for (int k = 0; k < 8; k++) {
                    float a = k * 0.785398f, c = cosf(a), s = sinf(a);
                    kpr_add_eshot(ES_PELLET, f->x, f->y, c * 1.0f, s * 1.0f);
                    kpr_add_eshot(ES_CLOUD, f->x, f->y, c * 0.7f, s * 0.7f);
                }
                kpr_sfx("kpr_puff");
            }
            if (f->t > 60) { f->state = 2; f->t = 0; }
        } else if (f->t > 120) f->state = 0;
        break;
    case FK_STILTCRAB:
        if (!solid_at(f->x, f->y + 7) && f->state == 0) f->vy = fminf(f->vy + 0.12f, 2.5f);
        if (f->state == 0 && solid_at(f->x, f->y + 7)) {
            f->vy = 0;
            if (d2 < 160.0f * 160.0f) {
                f->dir = dx > 0 ? 1 : -1;
                if (move_foe(f, f->dir * 1.4f, 0)) f->dir = -f->dir;
                if (dy < -20 && fabsf(dx) < 60 && f->t % 50 == 0) { f->vy = -3.0f; kpr_sfx("kpr_whoosh"); }
            }
        } else {
            move_foe(f, f->dir * 0.8f, 0);
        }
        if (f->vy != 0 && move_foe(f, 0, f->vy)) f->vy = 0;
        f->vy = fminf(f->vy + 0.12f, 2.5f);
        break;
    case FK_SUNSTAR:
        if (f->state == 0) {
            if (d2 < 64.0f * 64.0f && f->t > 30) { f->state = 1; f->t = 0; aim(f->x, f->y, 3.0f, &f->vx, &f->vy); kpr_sfx("kpr_whoosh"); }
        } else if (f->state == 1) {
            if (move_foe(f, f->vx, f->vy)) { f->state = 0; f->t = 0; }      /* stuck in the wall again */
            else if (f->t > 50) { f->state = 2; f->t = 0; }
        } else {
            if (move_foe(f, 0, 0.6f) || move_foe(f, 0, 0) || f->t > 240) { f->state = 0; f->t = 0; }
        }
        break;
    case FK_BILLFISH:
        if (f->state == 0) {
            if (move_foe(f, f->dir * 0.5f, 0)) f->dir = -f->dir;
            if (fabsf(dy) < 14 && dx * f->dir > 0 && fabsf(dx) < 170) { f->state = 1; f->t = 0; }
            else if (fabsf(dy) < 14 && fabsf(dx) < 170 && f->t % 60 == 0) f->dir = dx > 0 ? 1 : -1;
        } else if (f->state == 1) {
            if (f->t > 20) { f->state = 2; f->t = 0; kpr_sfx("kpr_whoosh"); }
        } else if (f->state == 2) {
            if (move_foe(f, f->dir * 4.0f, 0)) { f->state = 3; f->t = 0; kpr_sfx("kpr_bump"); }
            else if (f->t > 70) { f->state = 0; f->t = 0; }
        } else if (f->t > 60) { f->state = 0; f->t = 0; f->dir = -f->dir; }
        break;
    case FK_BURR:
        f->state = d2 < 40.0f * 40.0f ? 1 : 0;   /* angry: spines out */
        break;
    case FK_SNAPWORM:
        if (f->state == 0) {
            if (d2 < 44.0f * 44.0f && f->t > 60) { f->state = 1; f->t = 0; }
        } else if (f->state == 1) {
            if (f->t > 30) { f->state = 2; f->t = 0; kpr_sfx("kpr_snap"); }
        } else if (f->state == 2) {
            float k = fminf(1.0f, f->t / 10.0f) * 26.0f;
            f->x = f->hx + f->a * k;
            f->y = f->hy + f->b * k;
            if (f->t > 40) { f->state = 3; f->t = 0; }
        } else {
            float k = fmaxf(0.0f, 1.0f - f->t / 20.0f) * 26.0f;
            f->x = f->hx + f->a * k;
            f->y = f->hy + f->b * k;
            if (f->t > 20) { f->state = 0; f->t = 0; f->x = f->hx; f->y = f->hy; }
        }
        break;
    case FK_FIZZWORM:
        if (d2 < 180.0f * 180.0f) {
            float vx, vy;
            aim(f->x, f->y, 0.45f, &vx, &vy);
            f->vx += (vx - f->vx) * 0.05f;
            f->vy += (vy - f->vy) * 0.05f;
            move_foe(f, f->vx, f->vy + sinf(f->t * 0.1f) * 0.2f);
        }
        break;
    default: break;
    }
}

static bool touches_sub(const KprFoe *f) {
    int w, h;
    kpr_foe_size(f->kind, &w, &h);
    if (f->kind == FK_BURR && f->state) { w += 8; h += 8; }
    if (f->kind == FK_BLOATER && f->state == 1) { w += 8; h += 8; }
    if (f->kind == FK_SNAPWORM && f->state < 2) return false;
    if (f->kind == FK_FILCHER) return false;   /* it only steals */
    return fabsf(f->x - kg.x) * 2 < w + KPR_SW - 2 && fabsf(f->y - kg.y) * 2 < h + KPR_SH - 2;
}

void kpr_foes_update(void) {
    if (kg.no_foes) return;
    for (int i = 0; i < KPR_MAX_FOES; i++) {
        KprFoe *f = &kg.foe[i];
        if (!f->alive) continue;
        /* only the creatures near the sub stir */
        if (fabsf(f->x - kg.x) > 300 || fabsf(f->y - kg.y) > 200) continue;
        foe_update(i);
        if (kg.state != KS_PLAY) return;
        if (f->alive && !f->pacified && touches_sub(f)) {
            /* the Auger on a dash or boost runs creatures through */
            if (kpr_drilling()) kpr_hurt_foe(i, 40, true);
            else kpr_sub_hurt(f->x, f->y, KPR_HIT);
        }
        if (kg.state != KS_PLAY) return;
    }
}

void kpr_eshots_update(void) {
    for (int i = 0; i < KPR_MAX_ESHOTS; i++) {
        KprEShot *e = &kg.es[i];
        if (!e->alive) continue;
        e->x += e->vx;
        e->y += e->vy;
        if (e->kind == ES_CLOUD) { e->vx *= 0.985f; e->vy *= 0.985f; }
        if (--e->life <= 0 || (e->kind != ES_CLOUD && kpr_shot_solid(e->x, e->y))) { e->alive = 0; continue; }
        if (e->kind == ES_CLOUD) {
            /* a cloud knocks the trail loose: touching the sub or what it tows */
            bool hit = fabsf(e->x - kg.x) < 10 && fabsf(e->y - kg.y) < 9;
            for (int s = 0; s < kg.trail_n && !hit; s++) {
                float tx, ty;
                kpr_trail_get(s, &tx, &ty);
                hit = fabsf(e->x - tx) < 8 && fabsf(e->y - ty) < 8;
            }
            if (hit && kg.trail_n > 0) { kpr_trail_drop_last(); e->alive = 0; }
            continue;
        }
        if (fabsf(e->x - kg.x) * 2 < KPR_SW + 3 && fabsf(e->y - kg.y) * 2 < KPR_SH + 3) {
            e->alive = 0;
            kpr_sub_hurt(e->x - e->vx * 4, e->y - e->vy * 4, KPR_HIT);
            if (kg.state != KS_PLAY) return;
        }
    }
}
