/* SHUTTERBUG - the ships, the gun and the rings, the camera, bulbs and
 * wrenches, scoring and lives, shots, and the flow of a stage.
 * See docs/games/24-shutterbug.md. */
#include "shutterbug.h"

ShbGame sb;
ShbSave sbs;
uint8_t shb_tile[SHB_MAX_COLS][SHB_MAX_ROWS];
uint8_t shb_bhp[SHB_MAX_COLS][SHB_MAX_ROWS];
uint8_t shb_cave[SHB_MAX_COLS];
int shb_cols, shb_rows;

/* lives come from points, and only these eight times */
const uint32_t SHB_EXTEND_AT[SHB_EXTENDS] = {8000, 20000, 36000, 56000, 80000, 108000, 140000, 186000};

#define RING_FAN_DEG 40.0f   /* the outer pair of rings; the inner pair at a third of it */
#define BULB_SLOW 0.35f      /* a bulb drifts this fast toward a ship that is firing... */
#define BULB_FAST 2.8f       /* ...and flies this fast to one that isn't */
#define BULB_LIFE 720
#define WRENCH_POINTS 1000
#define ORB_STEP 20          /* a score orb pays 20, 40, 60 ... up to 200 */
#define ORB_MAX 200

/* ------------------------------------------------------------------------ */
/* small helpers                                                              */

void shb_burst(float x, float y, int col, int n, float sp) {
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < SHB_MAX_PARTS; i++) {
            Part *p = &sb.part[i];
            if (p->life > 0) continue;
            float a = rng_float(&g_rng) * 6.2832f, s = sp * (0.3f + rng_float(&g_rng));
            *p = (Part){x, y, cosf(a) * s, sinf(a) * s, 12 + rng_range(&g_rng, 0, 14), col};
            break;
        }
    }
}

void shb_sfx(const char *name, int gap) {
    static struct { const char *name; uint32_t at; } last[12];
    uint32_t now = engine_frame();
    int slot = -1;
    for (int i = 0; i < 12; i++) {
        if (last[i].name && !strcmp(last[i].name, name)) { slot = i; break; }
        if (!last[i].name && slot < 0) slot = i;
    }
    if (slot < 0) slot = 0;
    if (last[slot].name && !strcmp(last[slot].name, name) && now - last[slot].at < (uint32_t)gap) return;
    last[slot].name = name;
    last[slot].at = now;
    sfx_play_name(name);
}

void shb_save_now(void) {
    sbs.magic = 0x53484201u;
    game_save_write(game_current_index(), &sbs, (int)sizeof sbs);
}

void shb_clear_world(void) {
    memset(sb.foe, 0, sizeof sb.foe);
    memset(sb.es, 0, sizeof sb.es);
    memset(sb.ps, 0, sizeof sb.ps);
    memset(sb.pk, 0, sizeof sb.pk);
    memset(sb.part, 0, sizeof sb.part);
    memset(sb.blast, 0, sizeof sb.blast);
}

int shb_tile_at(float x, float y) {
    if (x < 0 || y < 0) return TL_EMPTY;
    int c = (int)(x / SHB_TILE), r = (int)(y / SHB_TILE);
    if (c >= shb_cols || r >= shb_rows) return TL_EMPTY;
    return shb_tile[c][r];
}

bool shb_solid_at(float x, float y) {
    if (tile_solid(shb_tile_at(x, y))) return true;
    for (int k = 0; k < shb_nmover; k++) {
        const Foe *m = &sb.foe[shb_mover[k]];
        if (m->alive && fabsf(x - m->x) < m->hw && fabsf(y - m->y) < m->hh) return true;
    }
    return false;
}

static bool box_solid(float x, float y, float hw, float hh) {
    for (float yy = y - hh; yy <= y + hh; yy += hh)
        for (float xx = x - hw; xx <= x + hw; xx += hw)
            if (shb_solid_at(xx, yy)) return true;
    return false;
}

int shb_near_ship(float x, float y) {
    int best = -1;
    float bd = 1e9f;
    for (int p = 0; p < 2; p++) {
        Ship *s = &sb.ship[p];
        if (!s->on || !s->alive) continue;
        float d = fabsf(s->x - x) + fabsf(s->y - y);
        if (d < bd) { bd = d; best = p; }
    }
    return best;
}

float shb_aim(float x, float y) {
    int p = shb_near_ship(x, y);
    if (p < 0) return 3.14159f;
    return atan2f(sb.ship[p].y - y, sb.ship[p].x - x);
}

/* ------------------------------------------------------------------------ */
/* score and lives                                                            */

uint32_t shb_next_extend(void) {
    return sb.extends < SHB_EXTENDS ? SHB_EXTEND_AT[sb.extends] : 0;
}

void shb_add_score(uint32_t pts) {
    sb.score += pts;
    sb.total += pts;
    if (sb.score > sb.best) sb.best = sb.score;
    while (sb.extends < SHB_EXTENDS && sb.total >= SHB_EXTEND_AT[sb.extends]) {
        sb.extends++;
        sb.lives++;
        sfx_play_name("shb_extend");
        sb.msg = "EXTRA LIFE!";
        sb.msg_t = 90;
    }
}

/* ------------------------------------------------------------------------ */
/* shots                                                                      */

static int add_pshot(int kind, float x, float y, float vx, float vy, int dmg, int owner) {
    for (int i = 0; i < SHB_MAX_PSHOTS; i++) {
        PShot *s = &sb.ps[i];
        if (s->alive) continue;
        *s = (PShot){x, y, vx, vy, (uint8_t)kind, 1, (uint8_t)owner, 0, 0, dmg};
        return i;
    }
    return -1;
}

int shb_add_eshot(int kind, float x, float y, float vx, float vy) {
    static const float R[] = {2.0f, 2.5f, 3.0f, 2.0f, 2.5f, 2.5f, 4.0f, 2.0f};
    for (int i = 0; i < SHB_MAX_ESHOTS; i++) {
        EShot *s = &sb.es[i];
        if (s->alive) continue;
        *s = (EShot){x, y, vx, vy, R[kind], (uint8_t)kind, 1, 0, 0, 1, 0};
        if (kind == ES_BOUNCE || kind == ES_CRUMB) s->bounces = 3;
        return i;
    }
    return -1;
}

void shb_aimed(float x, float y, float speed, int n, float spread_deg) {
    float a = shb_aim(x, y);
    for (int k = 0; k < n; k++) {
        float o = n > 1 ? (k - (n - 1) / 2.0f) * spread_deg * 0.017453f : 0;
        shb_add_eshot(ES_SHOT, x, y, cosf(a + o) * speed, sinf(a + o) * speed);
    }
}

void shb_ring(float x, float y, float speed, int n, float rot, int kind) {
    for (int k = 0; k < n; k++) {
        float a = rot + k * 6.2832f / n;
        shb_add_eshot(kind, x, y, cosf(a) * speed, sinf(a) * speed);
    }
}

void shb_drop(int kind, float x, float y) {
    for (int i = 0; i < SHB_MAX_PICKUPS; i++) {
        Pickup *k = &sb.pk[i];
        if (k->alive) continue;
        *k = (Pickup){x, y, 0, -0.6f, (uint8_t)kind, 1, 0};
        return;
    }
}

/* bounce a moving point off rock: returns true if it bounced */
static bool bounce_off(float *x, float *y, float *vx, float *vy) {
    float nx = *x + *vx, ny = *y + *vy;
    if (!shb_solid_at(nx, ny)) return false;
    bool hx = shb_solid_at(nx, *y), hy = shb_solid_at(*x, ny);
    if (hx || !hy) *vx = -*vx;
    if (hy || !hx) *vy = -*vy;
    return true;
}

void shb_eshot_step(EShot *s) {
    s->t++;
    switch (s->kind) {
    case ES_BOUNCE: case ES_CRUMB:
        if (bounce_off(&s->x, &s->y, &s->vx, &s->vy)) { if (--s->bounces < 0) { s->alive = 0; return; } }
        if (s->kind == ES_CRUMB) s->vy += 0.04f;
        break;
    case ES_FALL:
        s->vy = fminf(s->vy + 0.05f, 2.4f);
        break;
    case ES_ARROW:
        if (s->arg == 0) {
            s->vy = fminf(s->vy + 0.08f, 3.0f);
            if (shb_solid_at(s->x, s->y + s->vy + 2)) {
                /* it lands, and stabs straight back up */
                s->arg = 1;
                s->vx = 0;
                s->vy = -3.0f;
                s->y -= 2;
                return;
            }
        }
        break;
    case ES_RETAL:
        if (s->t < 30) { s->vx = 0; s->vy = 0; }
        else if (s->t == 30) {
            float a = shb_aim(s->x, s->y);
            s->vx = cosf(a) * 1.2f;
            s->vy = sinf(a) * 1.2f;
        }
        break;
    default: break;
    }
    s->x += s->vx;
    s->y += s->vy;
    bool solid_dies = s->kind != ES_BOUNCE && s->kind != ES_CRUMB && s->kind != ES_CRAWL && !(s->kind == ES_ARROW && s->arg == 0);
    if (solid_dies && s->t > 2 && shb_solid_at(s->x, s->y)) s->alive = 0;
    if (s->x < sb.cam_x - 24 || s->x > sb.cam_x + SCREEN_W + 40 || s->y < sb.cam_y - 30 || s->y > sb.cam_y + SHB_PF_H + 30)
        s->alive = 0;
    if (s->kind == ES_CRAWL && s->t > 600) s->alive = 0;
}

/* ------------------------------------------------------------------------ */
/* foes: hurt and kill                                                        */

static void add_blast(float x, float y) {
    for (int i = 0; i < SHB_MAX_BLASTS; i++)
        if (!sb.blast[i].alive) { sb.blast[i] = (Blast){x, y, 0, SHB_BLAST_R, 1}; break; }
}

void shb_kill_foe(int i, bool by_blast) {
    Foe *e = &sb.foe[i];
    if (!e->alive) return;
    bool stunned = e->stun > 0 || e->falling;
    e->alive = 0;
    if (e->role == ROLE_PROP) return;
    uint32_t pts = (uint32_t)e->value * (stunned ? 2u : 1u);
    if (e->kind == K_SHARD || e->kind == K_CHUTE) pts = stunned ? 200 : 100;
    shb_add_score(pts);
    int col = e->flags & F_GREEN ? C_LIME : e->flags & F_RED ? C_RED : C_AMBER;
    bool big = e->role == ROLE_MID || e->role == ROLE_BOSS;
    shb_burst(e->x, e->y, col, big ? 50 : 9, big ? 2.6f : 1.5f);
    if (big) { sb.shake = 24; sb.flash = 8; sfx_play_name("shb_bigboom"); sb.big_kills++; }
    else shb_sfx("shb_pop", 3);
    if (e->flags & F_CRYSTAL) shb_drop(PK_CRYSTAL, e->x, e->y);
    if (stunned) {
        /* a photographed foe goes up with a bang that hurts its neighbours,
         * takes every foe of its kind from the same photo with it, and
         * never strikes back */
        add_blast(e->x, e->y);
        for (int j = 0; j < SHB_MAX_FOES; j++) {
            Foe *o = &sb.foe[j];
            if (!o->alive || j == i || o->role == ROLE_PROP) continue;
            if (o->kind == e->kind && o->photo == e->photo && o->stun > 0) { shb_kill_foe(j, true); continue; }
            float dx = o->x - e->x, dy = o->y - e->y;
            if (dx * dx + dy * dy < (float)(SHB_BLAST_R + o->hw) * (SHB_BLAST_R + o->hw)) shb_hurt_foe(j, SHB_BLAST_DMG, false);
        }
    } else if (e->flags & F_GREEN) {
        /* green foes leave a parting shot that drifts after you */
        shb_add_eshot(ES_RETAL, e->x, e->y, 0, 0);
        sb.retaliations++;
    }
    if (e->kind == K_ROCKBIG) {
        for (int k = 0; k < 3; k++) {
            int j = shb_spawn(K_ROCK, e->x, e->y, 0, 0);
            if (j >= 0) {
                float a = 2.4f + k * 0.75f;
                sb.foe[j].vx = cosf(a) * 1.3f - 0.3f;
                sb.foe[j].vy = sinf(a) * 1.3f;
            }
        }
    }
    if (big || e->role == ROLE_PART) shb_boss_dead(i);
    (void)by_blast;
}

void shb_hurt_foe(int i, int dmg, bool ring) {
    Foe *e = &sb.foe[i];
    if (!e->alive || dmg <= 0 || !shb_foe_hittable(e)) return;
    if (e->stun > 0) dmg *= 2;
    e->hp -= dmg;
    e->flash = 3;
    if (e->hp <= 0) shb_kill_foe(i, false);
    (void)ring;
}

/* a shot reaches a foe: true if the shot is used up */
static bool shot_hits_foe(int i, int dmg, float x, float y) {
    Foe *e = &sb.foe[i];
    if (e->kind == K_LASER) {
        /* the gate swallows the shot and answers it */
        if (e->state == 1 && (sb.frame_t % 10) == 0) shb_aimed(e->x, y, 1.6f, 1, 0);
        return e->state == 1;
    }
    if (e->kind == K_ORB) {
        e->alive = 0;
        int v = imin(ORB_MAX, ORB_STEP * (sb.orbs_shot + 1));
        sb.orbs_shot++;
        shb_add_score((uint32_t)v);
        shb_burst(e->x, e->y, C_YELLOW, 6, 1.0f);
        shb_sfx("shb_orb", 2);
        return true;
    }
    if (e->role == ROLE_PROP) return false;
    if (e->role == ROLE_MID || e->role == ROLE_BOSS || e->role == ROLE_PART) {
        if (!shb_boss_hit(i, dmg, x, y)) { shb_sfx("shb_clink", 6); return true; }
    }
    if (!shb_foe_hittable(e)) return e->kind != K_GHOST;
    shb_hurt_foe(i, dmg, false);
    return true;
}

static bool foe_box_hit(const Foe *e, float x, float y, float r) {
    return fabsf(e->x - x) < e->hw + r && fabsf(e->y - y) < e->hh + r;
}

/* ------------------------------------------------------------------------ */
/* the camera                                                                 */

void shb_take_photo(int p) {
    Ship *s = &sb.ship[p];
    s->flash = 0;
    s->snap_t = 14;
    float cx = s->x + SHB_CURSOR_DX, cy = s->y;
    s->snap_x = cx;
    s->snap_y = cy;
    sb.photo_seq++;
    sb.photos++;
    sfx_play_name("shb_snap");
    float hw = SHB_PHOTO_W / 2.0f, hh = SHB_PHOTO_H / 2.0f;
    for (int i = 0; i < SHB_MAX_FOES; i++) {
        Foe *e = &sb.foe[i];
        if (!e->alive || e->t < 0) continue;
        if (fabsf(e->x - cx) < hw + e->hw && fabsf(e->y - cy) < hh + e->hh) shb_foe_photographed(i, sb.photo_seq);
    }
    /* crumbly rock in the picture is weakened */
    for (int c = (int)((cx - hw) / SHB_TILE); c <= (int)((cx + hw) / SHB_TILE); c++)
        for (int r = (int)((cy - hh) / SHB_TILE); r <= (int)((cy + hh) / SHB_TILE); r++)
            if (c >= 0 && r >= 0 && c < shb_cols && r < shb_rows && shb_tile[c][r] == TL_BREAK) shb_bhp[c][r] = 1;
}

/* ------------------------------------------------------------------------ */
/* the ships                                                                  */

void shb_hurt_ship(int p) {
    Ship *s = &sb.ship[p];
    if (!s->alive || s->inv > 0 || sb.god) return;
    if (s->armour) {
        s->armour = false;
        s->inv = SHB_INV_T;
        sb.shake = 8;
        sfx_play_name("shb_armour");
        shb_burst(s->x, s->y, C_RED, 12, 1.6f);
        return;
    }
    s->alive = false;
    s->dead_t = 0;
    sb.deaths++;
    sb.shake = 20;
    sfx_play_name("shb_die");
    shb_burst(s->x, s->y, p ? C_LIME : C_RED, 40, 2.2f);
    shb_burst(s->x, s->y, C_WHITE, 16, 1.2f);
}

static int pad_of(int p) {
    if (p == 0) {
        return (btn(BTN_LEFT) ? BTN_LEFT : 0) | (btn(BTN_RIGHT) ? BTN_RIGHT : 0) | (btn(BTN_UP) ? BTN_UP : 0) |
               (btn(BTN_DOWN) ? BTN_DOWN : 0) | (btn(BTN_A) ? BTN_A : 0) | (btn(BTN_B) ? BTN_B : 0) |
               (btnp(BTN_A) ? 0x100 : 0);
    }
    return (btn2(BTN_LEFT) ? BTN_LEFT : 0) | (btn2(BTN_RIGHT) ? BTN_RIGHT : 0) | (btn2(BTN_UP) ? BTN_UP : 0) |
           (btn2(BTN_DOWN) ? BTN_DOWN : 0) | (btn2(BTN_A) ? BTN_A : 0) | (btn2(BTN_B) ? BTN_B : 0) |
           (btnp2(BTN_A) ? 0x100 : 0);
}

static void throw_rings(int p) {
    Ship *s = &sb.ship[p];
    static const float FAN[4] = {-1.0f, -1.0f / 3.0f, 1.0f / 3.0f, 1.0f};
    for (int k = 0; k < 4; k++) {
        float a = FAN[k] * RING_FAN_DEG * 0.017453f;
        int j = add_pshot(PS_RING, s->x + 6, s->y, cosf(a) * SHB_RING_SPEED, sinf(a) * SHB_RING_SPEED, SHB_RING_DMG, p);
        if (j >= 0) sb.ps[j].bounces = SHB_RING_BOUNCES;
    }
    sb.rings_thrown++;
    sfx_play_name("shb_rings");
}

static void ship_update(int p) {
    Ship *s = &sb.ship[p];
    if (!s->on) return;
    if (s->snap_t > 0) s->snap_t--;
    if (!s->alive) { s->dead_t++; return; }
    int pad = pad_of(p);
    int dx = (pad & BTN_RIGHT ? 1 : 0) - (pad & BTN_LEFT ? 1 : 0);
    int dy = (pad & BTN_DOWN ? 1 : 0) - (pad & BTN_UP ? 1 : 0);
    s->vx = dx;
    s->vy = dy;
    float sp = SHB_SPEED * (dx && dy ? 0.7071f : 1.0f);
    float nx = s->x + dx * sp, ny = s->y + dy * sp;
    float minx = sb.cam_x + 8, maxx = sb.cam_x + SCREEN_W - 10;
    float miny = sb.cam_y + 6, maxy = sb.cam_y + SHB_PF_H - 6;
    nx = fclamp(nx, minx, maxx);
    ny = fclamp(ny, miny, maxy);
    bool inside = box_solid(s->x, s->y, SHB_BOX_W, SHB_BOX_H);
    if (inside && s->ghost > 0) {
        /* pushed into the rock: slip through until clear */
        s->x = nx;
        s->y = ny;
    } else {
        s->ghost = 0;
        if (!box_solid(nx, s->y, SHB_BOX_W, SHB_BOX_H)) s->x = nx;
        if (!box_solid(s->x, ny, SHB_BOX_W, SHB_BOX_H)) s->y = ny;
    }
    if (s->ghost > 0 && !box_solid(s->x, s->y, SHB_BOX_W, SHB_BOX_H)) s->ghost = 0;
    /* the scroll pushes from the left: against rock, that crushes */
    if (s->x < minx) {
        s->x = minx;
        if (box_solid(s->x, s->y, SHB_BOX_W, SHB_BOX_H) && s->ghost == 0) {
            shb_hurt_ship(p);
            s->ghost = 1;
        }
    }
    if (s->y < miny) s->y = miny;
    if (s->y > maxy) s->y = maxy;
    if (s->ghost > 0) s->ghost++;
    /* the gun, and the rings it charges */
    s->firing = (pad & BTN_B) != 0;
    if (s->firing) {
        s->hold_t++;
        if (s->hold_t == SHB_CHARGE_T) sfx_play_name("shb_ready");
        if (--s->fire_cd <= 0) {
            add_pshot(PS_GUN, s->x + 8, s->y + 1, SHB_SHOT_SPEED, 0, 1, p);
            s->fire_cd = SHB_FIRE_GAP;
            shb_sfx(p ? "shb_shot2" : "shb_shot", 4);
        }
    } else {
        if (s->hold_t >= SHB_CHARGE_T) throw_rings(p);
        s->hold_t = 0;
        s->fire_cd = 0;
    }
    /* the camera */
    if ((pad & 0x100) && s->flash >= SHB_FLASH_MAX) shb_take_photo(p);
    else if ((pad & 0x100)) shb_sfx("shb_empty", 10);
    if (s->flash < SHB_FLASH_MAX) s->flash = imin(SHB_FLASH_MAX, s->flash + SHB_FLASH_RATE);
    if (s->inv > 0) s->inv--;
}

/* ------------------------------------------------------------------------ */
/* the world, a frame at a time                                               */

static void pshots_update(void) {
    for (int i = 0; i < SHB_MAX_PSHOTS; i++) {
        PShot *s = &sb.ps[i];
        if (!s->alive) continue;
        s->t++;
        if (s->kind == PS_RING) {
            if (bounce_off(&s->x, &s->y, &s->vx, &s->vy) && --s->bounces < 0) { s->alive = 0; continue; }
            if (s->t > SHB_RING_LIFE) { s->alive = 0; continue; }
        }
        s->x += s->vx;
        s->y += s->vy;
        if (s->x > sb.cam_x + SCREEN_W + 8 || s->x < sb.cam_x - 8 || s->y < sb.cam_y - 8 || s->y > sb.cam_y + SHB_PF_H + 8) {
            s->alive = 0;
            continue;
        }
        if (s->kind == PS_GUN) {
            int c = (int)(s->x / SHB_TILE), r = (int)(s->y / SHB_TILE);
            int tl = shb_tile_at(s->x, s->y);
            if (tile_solid(tl)) {
                if (tl == TL_BREAK) {
                    if (--shb_bhp[c][r] <= 0) {
                        shb_tile[c][r] = TL_EMPTY;
                        shb_burst(c * SHB_TILE + 4.0f, r * SHB_TILE + 4.0f, C_TAN, 5, 1.0f);
                        shb_sfx("shb_crumble", 4);
                    }
                }
                s->alive = 0;
                continue;
            }
        }
        /* enemy shots that can be shot down */
        for (int j = 0; j < SHB_MAX_ESHOTS; j++) {
            EShot *e = &sb.es[j];
            if (!e->alive || e->kind != ES_FALL) continue;
            if (fabsf(e->x - s->x) < e->r + 3 && fabsf(e->y - s->y) < e->r + 3) {
                e->alive = 0;
                if (e->arg) shb_drop(PK_CRYSTAL, e->x, e->y);
                shb_burst(e->x, e->y, C_TAN, 4, 0.8f);
                shb_add_score(10);
                if (s->kind == PS_GUN) s->alive = 0;
                break;
            }
        }
        if (!s->alive) continue;
        float r = s->kind == PS_RING ? 4.0f : 2.0f;
        for (int j = 0; j < SHB_MAX_FOES; j++) {
            Foe *e = &sb.foe[j];
            if (!e->alive || e->t < 0) continue;
            if (!foe_box_hit(e, s->x, s->y, r)) continue;
            if (shot_hits_foe(j, s->dmg, s->x, s->y)) { s->alive = 0; break; }
        }
    }
}

static void eshots_update(void) {
    for (int i = 0; i < SHB_MAX_ESHOTS; i++) {
        EShot *s = &sb.es[i];
        if (!s->alive) continue;
        shb_eshot_step(s);
        if (!s->alive) continue;
        for (int p = 0; p < 2; p++) {
            Ship *sh = &sb.ship[p];
            if (!sh->on || !sh->alive) continue;
            float dx = s->x - sh->x, dy = s->y - sh->y, rr = s->r + SHB_HIT_R;
            if (dx * dx + dy * dy < rr * rr) {
                if (sh->inv <= 0 && !sb.god) s->alive = 0;
                shb_hurt_ship(p);
            }
        }
    }
}

static void contact_update(void) {
    for (int p = 0; p < 2; p++) {
        Ship *sh = &sb.ship[p];
        if (!sh->on || !sh->alive) continue;
        for (int i = 0; i < SHB_MAX_FOES; i++) {
            Foe *e = &sb.foe[i];
            if (!e->alive || e->t < 0 || !shb_foe_harmful(e)) continue;
            if (fabsf(e->x - sh->x) < e->hw + SHB_HIT_R && fabsf(e->y - sh->y) < e->hh + SHB_HIT_R) shb_hurt_ship(p);
        }
    }
}

static void pickups_update(void) {
    for (int i = 0; i < SHB_MAX_PICKUPS; i++) {
        Pickup *k = &sb.pk[i];
        if (!k->alive) continue;
        k->t++;
        int p = shb_near_ship(k->x, k->y);
        if (k->kind == PK_CRYSTAL && p >= 0) {
            /* bulbs drift toward you; stop firing and they fly to you */
            int q = -1;
            float bd = 1e9f;
            for (int o = 0; o < 2; o++) {
                Ship *s = &sb.ship[o];
                if (!s->on || !s->alive || s->firing) continue;
                float d = fabsf(s->x - k->x) + fabsf(s->y - k->y);
                if (d < bd) { bd = d; q = o; }
            }
            int to = q >= 0 ? q : p;
            float sp = q >= 0 ? BULB_FAST : BULB_SLOW;
            float a = atan2f(sb.ship[to].y - k->y, sb.ship[to].x - k->x);
            k->vx = fapproach(k->vx, cosf(a) * sp, 0.25f);
            k->vy = fapproach(k->vy, sinf(a) * sp, 0.25f);
        } else if (k->kind == PK_WRENCH) {
            k->vx = -0.25f;
            k->vy = sinf(k->t * 0.08f) * 0.3f;
        }
        k->x += k->vx;
        k->y += k->vy;
        if (k->t > BULB_LIFE || k->x < sb.cam_x - 12) { k->alive = 0; continue; }
        for (int o = 0; o < 2; o++) {
            Ship *s = &sb.ship[o];
            if (!s->on || !s->alive) continue;
            if (fabsf(s->x - k->x) < 10 && fabsf(s->y - k->y) < 9) {
                k->alive = 0;
                if (k->kind == PK_CRYSTAL) {
                    s->flash = imin(SHB_FLASH_MAX, s->flash + SHB_CRYSTAL);
                    sb.crystals_got++;
                    shb_sfx("shb_bulb", 3);
                } else {
                    sb.wrenches++;
                    if (!s->armour) s->armour = true;
                    else shb_add_score(WRENCH_POINTS);
                    sfx_play_name("shb_wrench");
                }
                break;
            }
        }
    }
}

static void fx_update(void) {
    for (int i = 0; i < SHB_MAX_PARTS; i++) {
        Part *p = &sb.part[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        p->vx *= 0.94f;
        p->vy *= 0.94f;
    }
    for (int i = 0; i < SHB_MAX_BLASTS; i++)
        if (sb.blast[i].alive && ++sb.blast[i].t > 16) sb.blast[i].alive = 0;
    if (sb.shake > 0) sb.shake--;
    if (sb.flash > 0) sb.flash--;
    if (sb.msg_t > 0) sb.msg_t--;
    if (sb.warn_t > 0) sb.warn_t--;
}

/* ------------------------------------------------------------------------ */
/* the view                                                                   */

static bool hold_done(void) {
    if (sb.hold == HOLD_FOREVER) return false;
    if (sb.hold == HOLD_GENS) {
        for (int i = 0; i < SHB_MAX_FOES; i++)
            if (sb.foe[i].alive && sb.foe[i].kind == K_GEN) return false;
        return true;
    }
    if (sb.hold == HOLD_BIG) {
        if (!sb.big_seen) return false;
        for (int i = 0; i < SHB_MAX_FOES; i++)
            if (sb.foe[i].alive && (sb.foe[i].role == ROLE_MID || sb.foe[i].role == ROLE_BOSS)) return false;
        return true;
    }
    return true;
}

static void view_update(void) {
    const StageDef *st = &SHB_STAGE[sb.stage];
    if (sb.hold != HOLD_NONE) {
        for (int i = 0; i < SHB_MAX_FOES; i++)
            if (sb.foe[i].alive && (sb.foe[i].role == ROLE_MID || sb.foe[i].role == ROLE_BOSS)) sb.big_seen = true;
        if (hold_done()) sb.hold = HOLD_NONE;
    }
    if (sb.hold == HOLD_NONE && sb.cam_x < st->end_x) {
        float sp = st->scroll[sb.scroll_i].speed / 4.0f;
        float nx = sb.cam_x + sp;
        if (sb.scroll_i + 1 < st->nscroll && nx >= st->scroll[sb.scroll_i + 1].x) {
            nx = st->scroll[sb.scroll_i + 1].x;
            sb.scroll_i++;
            sb.hold = st->scroll[sb.scroll_i].hold;
            sb.big_seen = false;
        }
        sb.cam_x = fminf(nx, (float)st->end_x);
    }
    /* up and down: within the band the view follows the ships */
    float lo = shb_band_lo(sb.cam_x + SCREEN_W / 2), hi = shb_band_hi(sb.cam_x + SCREEN_W / 2);
    float ys = 0;
    int n = 0;
    for (int p = 0; p < 2; p++)
        if (sb.ship[p].on && sb.ship[p].alive) { ys += sb.ship[p].y; n++; }
    float want = n ? ys / n - SHB_PF_H / 2 : sb.cam_y;
    want = fclamp(want, lo, hi);
    sb.cam_y = fapproach(sb.cam_y, want, 1.4f);
}

/* ------------------------------------------------------------------------ */
/* the run and the stage                                                      */

static void place_ships(void) {
    float y0 = sb.cam_y + SHB_PF_H / 2;
    for (int p = 0; p < 2; p++) {
        Ship *s = &sb.ship[p];
        bool on = p == 0 || sb.players == 2;
        memset(s, 0, sizeof *s);
        s->on = on;
        s->alive = on;
        s->armour = true;
        s->x = sb.cam_x + 48;
        s->y = y0 + (sb.players == 2 ? (p ? 18 : -18) : 0);
        s->flash = SHB_FLASH_MAX;
    }
}

void shb_start_stage(int s) {
    sb.stage = s;
    shb_clear_world();
    shb_build_terrain(s);
    sb.cam_x = 0;
    sb.cam_y = shb_band_lo(SCREEN_W / 2);
    sb.scroll_i = 0;
    sb.hold = SHB_STAGE[s].scroll[0].hold;
    sb.big_seen = false;
    sb.spawn_i = 0;
    sb.stage_t = 0;
    sb.boss_dead = false;
    sb.clear_t = 0;
    sb.orbs_shot = 0;
    sb.warn_t = 0;
    sb.msg_t = 0;
    sb.tip = -1;
    sb.tip_t = 0;
    place_ships();
    sb.state = SS_BANNER;
    sb.state_t = 0;
    music_play(SHB_MUS_STAGE[s]);
}

void shb_new_run(int players) {
    sb.players = players;
    sb.score = sb.best = sb.total = 0;
    sb.extends = 0;
    sb.lives = 0;          /* no spare lives: they are earned */
    sb.deaths = 0;
    sb.letters = 0;
    sb.kinds = sb.kinds2 = 0;
    sb.secrets = 0;
    sb.won = sb.true_won = false;
    sb.beacon_given = false;
    sb.zero_prologue = false;
    sb.photos = sb.rings_thrown = sb.crystals_got = sb.wrenches = sb.retaliations = sb.big_kills = 0;
    if (sbs.runs < 65535) sbs.runs++;
    shb_save_now();
    shb_start_stage(0);
    input_set_versus(players == 2);
}

/* the stage's last foe is down */
void shb_stage_won(void) {
    if (sb.boss_dead) return;
    sb.boss_dead = true;
    sb.clear_t = 0;
    for (int k = 0; k < SHB_MAX_ESHOTS; k++)
        if (sb.es[k].alive) { sb.es[k].alive = 0; shb_burst(sb.es[k].x, sb.es[k].y, C_GREY, 1, 0.5f); }
}

static bool any_ship_down(void) {
    for (int p = 0; p < 2; p++)
        if (sb.ship[p].on && !sb.ship[p].alive) return true;
    return false;
}

static int ship_dead_t(void) {
    int t = 0;
    for (int p = 0; p < 2; p++)
        if (sb.ship[p].on && !sb.ship[p].alive) t = imax(t, sb.ship[p].dead_t);
    return t;
}

void shb_play_update(void) {
    sb.frame_t++;
    sb.stage_t++;
    view_update();
    shb_run_spawns();
    for (int p = 0; p < 2; p++) ship_update(p);
    pshots_update();
    shb_foes_update();
    eshots_update();
    contact_update();
    pickups_update();
    fx_update();
    /* a ship is lost: after a moment, a life or the end */
    if (any_ship_down() && ship_dead_t() >= SHB_DEAD_T) {
        input_consume();
        if (sb.lives > 0) {
            sb.state = SS_LOST;
            sb.state_t = 0;
            music_play(SHB_MUS_LOST);
        } else {
            sb.state = SS_OVER;
            sb.state_t = 0;
            music_play(SHB_MUS_OVER);
        }
        return;
    }
    /* the stage is over: its boss fell, or the way ran out */
    const StageDef *st = &SHB_STAGE[sb.stage];
    if (!sb.boss_dead && sb.cam_x >= st->end_x && sb.spawn_i >= st->nspawns && sb.hold == HOLD_NONE) {
        bool any = false;
        for (int i = 0; i < SHB_MAX_FOES; i++) {
            Foe *e = &sb.foe[i];
            if (e->alive && e->role != ROLE_PROP && e->t >= 0) { any = true; break; }
        }
        if (!any) shb_stage_won();
    }
    if (sb.boss_dead && !any_ship_down()) {
        sb.clear_t++;
        if (sb.clear_t >= 150) {
            input_consume();
            if (sb.stage == 0 && sb.best == 0) sb.zero_prologue = true;
            sb.state = SS_CLEAR;
            sb.state_t = 0;
            music_play(SHB_MUS_CLEAR);
        }
    }
}
