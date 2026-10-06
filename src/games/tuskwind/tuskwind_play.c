/* TUSKWIND - the journey: Burl on his islets, the things on them, the
 * items and the duchess's stalls, the terns, the wind, the hall. */
#include "tuskwind.h"

#define PI_F 3.14159265f

const char *const TKW_ITEM_NAME[IT_COUNT] = {"BOBBER", "SPYGLASS", "GRAPNEL", "KITE", "SPINNER", "SPRAT TIN"};
const int TKW_ITEM_COST[IT_COUNT] = {1, 2, 3, 5, 6, 8};

#define HELD(m) ((in & (m)) != 0)
#define PRESSED(m) ((in & (m)) && !(tkg.prev_in & (m)))
#define RELEASED(m) (!(in & (m)) && (tkg.prev_in & (m)))

void tkw_sfx(const char *name) { sfx_play_name(name); }

void tkw_say(const char *msg) {
    tkg.msg = msg;
    tkg.msg_t = 150;
}

void tkw_add_shells(int n) { tkg.shells = imax(0, tkg.shells + n); }

void tkw_set_wind(int dir, int frames) {
    tkg.wind = dir;
    tkg.wind_t = dir ? frames : 0;
}

static bool grounded(const TkwBody *b) { return b->mode == BM_GROUND || b->mode == BM_LUNGE || b->mode == BM_RIDE; }

void tkw_hero_place(int plat, float x) {
    TkwHero *h = &tkg.h;
    TkwBody *b = &h->b;
    const TkwPlat *p = &tkw_w.plat[plat];
    b->x = fclamp(x, p->x + 3.0f, p->x + p->w - 3.0f);
    b->y = (float)p->y;
    b->vx = b->vy = 0;
    b->plat = plat;
    b->ride = -1;
    b->mode = BM_GROUND;
    b->ignore = -1;
    b->kite = false;
    h->charging = false;
    h->charge = 0;
    h->flapping = false;
    h->hurt_t = 0;
}

void tkw_new_journey(uint64_t seed) {
    tkg.seed = seed;
    rng_seed(&tkg.rng, seed ^ 0x5EA5EA5Eull);
    tkg.shells = tkg.keys = tkg.terns = tkg.terns_used = tkg.terns_found = tkg.chests = tkg.spent = 0;
    memset(tkg.inv, 0, sizeof tkg.inv);
    tkg.inv[IT_BOBBER] = 1; /* every journey starts with one */
    tkg.pct = tkg.max_pct = 0;
    tkg.signs_run = 0;
    tkg.sign_near = -1;
    tkg.wind = tkg.wind_t = 0;
    tkg.wind40 = tkg.wind80 = false;
    tkg.msg_t = 0;
    tkg.gift = false;
    tkg.in_hall = false;
    tkg.hall_terns = 0;
    tkg.secret_read = false;
    tkg.rescues = 0;
    tkg.journey_frames = 0;
    tkg.cherry_end = false;
    tkw_gen_journey(seed);
    memset(&tkg.h, 0, sizeof tkg.h);
    TkwHero *h = &tkg.h;
    h->face = 1;
    h->aim = TKW_AIM_START;
    h->red_line = -1;
    h->b.stamina = TKW_STAMINA;
    h->takeoff = tkw_w.start_plat;
    h->ps = PS_PLAY;
    tkw_hero_place(tkw_w.start_plat, (float)tkw_w.start_x);
    h->takeoff_x = h->b.x;
    tkg.prev_in = 0xFFFF; /* nothing held from the title counts as a press */
    tkw_camera(true);
}

/* ---- the camera ------------------------------------------------------------ */

void tkw_camera(bool snap) {
    const TkwHero *h = &tkg.h;
    const TkwBody *b = &h->b;
    float tx = b->x, ty = b->y - 6;
    if (h->ps == PS_LOOK) {
        tx = h->look_x;
        ty = h->look_y;
    } else if (h->ps == PS_BALL) {
        tx = h->ball.x;
        ty = h->ball.y - 4;
    } else if (grounded(b)) {
        /* aiming shows a little more that way */
        float a = h->aim * PI_F / 180.0f;
        tx += cosf(a) * h->face * 44;
        ty -= sinf(a) * 40;
    } else {
        tx += b->vx * 14;
        ty += b->vy * 8;
    }
    float cx = tx - SCREEN_W / 2.0f, cy = ty - (TKW_HUD_H + TKW_VIEW_H / 2.0f);
    cx = fclamp(cx, 0, (float)(tkw_w.w - SCREEN_W));
    cy = fclamp(cy, 0, (float)(tkw_w.h - SCREEN_H));
    if (snap) {
        tkg.cam_x = cx;
        tkg.cam_y = cy;
    } else {
        float k = h->ps == PS_LOOK ? 0.5f : 0.12f;
        tkg.cam_x += (cx - tkg.cam_x) * k;
        tkg.cam_y += (cy - tkg.cam_y) * k;
    }
}

/* ---- landing, the sea ------------------------------------------------------ */

void tkw_on_land(int plat) {
    TkwPlat *p = &tkw_w.plat[plat];
    if (p->kind == PK_FOAM && p->timer < 0) p->timer = 0;
    if (!tkg.in_hall) {
        tkg.pct = tkw_progress_at(p->x + p->w / 2.0f);
        if (tkg.pct > tkg.max_pct) tkg.max_pct = tkg.pct;
        if (tkg.max_pct >= 50 && !tkg.gift) {
            tkg.gift = true;
            game_award(GOAL_BEACON);
        }
        /* the natural winds: one that dies out at 40 %, one that stays at 80 % */
        if (tkg.max_pct >= 40 && !tkg.wind40) {
            tkg.wind40 = true;
            tkw_set_wind(rng_chance(&tkg.rng, 50) ? 1 : -1, TKW_WIND_FIRST);
            tkw_say("THE WIND IS RISING");
            tkw_sfx("tkw_wind");
        }
        if (tkg.max_pct >= 80 && !tkg.wind80) {
            tkg.wind80 = true;
            tkw_set_wind(rng_chance(&tkg.rng, 50) ? 1 : -1, 0);
            tkw_say("A STORM WIND!");
            tkw_sfx("tkw_wind");
        }
    }
    /* a ram on this islet notices him */
    for (int i = 0; i < tkw_w.nth; i++) {
        TkwThing *t = &tkw_w.th[i];
        if (t->alive && t->kind == TH_RAM && t->plat == plat && t->state == RAM_IDLE) {
            t->state = RAM_ALERT;
            t->t = 0;
            tkw_sfx("tkw_alert");
        }
    }
}

static void fall_in_sea(void) {
    TkwHero *h = &tkg.h;
    tkw_sfx("tkw_splash");
    h->ps_t = 0;
    h->charging = false;
    h->spinner = false;
    if (tkg.god || tkg.terns > 0) h->ps = PS_RESCUE;
    else h->ps = PS_SINK;
}

static void knock(float vx, float vy, int t) {
    TkwHero *h = &tkg.h;
    TkwBody *b = &h->b;
    b->mode = BM_AIR;
    b->plat = -1;
    b->ride = -1;
    b->vx = vx;
    b->vy = vy;
    b->ignore = -1;
    h->charging = false;
    h->charge = 0;
    h->flapping = false;
    h->hurt_t = t;
    tkg.shake = 6;
    tkw_sfx("tkw_bump");
}

/* ---- the mantas ------------------------------------------------------------- */

static void drop_key(float x, float y) {
    int k = tkw_add_thing(&tkw_w, TH_KEY, x, y, -1);
    (void)k;
    tkw_sfx("tkw_pop");
}

static void kill_manta(int k) {
    TkwManta *m = &tkw_w.manta[k];
    if (!m->alive) return;
    m->alive = 0;
    drop_key(m->x, m->y);
    if (tkg.h.b.mode == BM_RIDE && tkg.h.b.ride == k) {
        tkg.h.b.mode = BM_AIR;
        tkg.h.b.ride = -1;
    }
}

static bool in_manta(const TkwManta *m, float x, float y, float r) {
    return m->alive && fabsf(x - m->x) < TKW_MANTA_HW + r && fabsf(y - m->y) < TKW_MANTA_HH + r;
}

static void mantas_update(void) {
    TkwHero *h = &tkg.h;
    TkwBody *b = &h->b;
    for (int k = 0; k < tkw_w.nmanta; k++) {
        TkwManta *m = &tkw_w.manta[k];
        if (!m->alive) continue;
        m->px = m->x;
        float py = m->y;
        tkw_manta_pos(m, tkg.frame_t, &m->x, &m->y);
        if (b->mode == BM_RIDE && b->ride == k) {
            b->x += m->x - m->px;
            b->y = m->y - TKW_MANTA_HH;
            (void)py;
            continue;
        }
        if (h->ps == PS_RESCUE || h->ps == PS_SINK || h->ps == PS_HAUL || b->mode != BM_AIR) continue;
        /* the body against the manta */
        if (fabsf(b->x - m->x) < TKW_MANTA_HW + TKW_HW - 2 && b->y > m->y - TKW_MANTA_HH && b->y - TKW_BH < m->y + TKW_MANTA_HH) {
            if (h->spinner) { kill_manta(k); continue; }
            if (b->vy >= 0 && b->y - b->vy <= m->y - TKW_MANTA_HH + 3) {
                /* on its back: a ride */
                b->mode = BM_RIDE;
                b->ride = k;
                b->plat = -1;
                b->vx = b->vy = 0;
                b->kite = false;
                b->y = m->y - TKW_MANTA_HH;
                tkw_sfx("tkw_land");
            } else {
                float dir = b->x >= m->x ? 1.0f : -1.0f;
                knock(dir * 2.5f, b->y - 6 < m->y ? -1.5f : 1.5f, 20);
            }
        }
    }
}

/* ---- the things ------------------------------------------------------------- */

static void collect(TkwThing *t) {
    TkwBody *b = &tkg.h.b;
    t->alive = 0;
    switch (t->kind) {
    case TH_COCKLE: tkw_add_shells(1); tkw_sfx("tkw_shell"); break;
    case TH_WHELK: tkw_add_shells(4); tkw_sfx("tkw_whelk"); break;
    case TH_SPIRAL: tkw_add_shells(3); tkw_sfx("tkw_whelk"); break;
    case TH_KEY: tkg.keys++; tkw_sfx("tkw_key"); break;
    case TH_SPRAT: b->stamina = imin(TKW_STAMINA, b->stamina + TKW_SPRAT); tkw_sfx("tkw_sprat"); break;
    default: break;
    }
}

static bool hero_active(void) {
    int ps = tkg.h.ps;
    return ps == PS_PLAY || ps == PS_MENU || ps == PS_THROW || ps == PS_LOOK || ps == PS_ROPE || ps == PS_HAUL;
}

static void rams_update(void) {
    TkwHero *h = &tkg.h;
    TkwBody *b = &h->b;
    for (int i = 0; i < tkw_w.nth; i++) {
        TkwThing *t = &tkw_w.th[i];
        if (!t->alive || t->kind != TH_RAM) continue;
        t->t++;
        bool near = fabsf(b->x - t->x) < TKW_HW + 8 && b->y > t->y - 12 && b->y - TKW_BH < t->y;
        bool on_it = grounded(b) && b->mode != BM_RIDE && b->plat == t->plat;
        switch (t->state) {
        case RAM_IDLE:
            if (on_it && hero_active()) { t->state = RAM_ALERT; t->t = 0; tkw_sfx("tkw_alert"); break; }
            /* flown into before it notices: off it goes */
            if (near && b->mode == BM_AIR && hero_active() && fabsf(b->vx) + fabsf(b->vy) >= 1.2f) {
                t->state = RAM_FALL;
                t->vx = (b->vx >= 0 ? 1 : -1) * 2.2f;
                t->vy = -1.6f;
                t->plat = -1;
                tkw_sfx("tkw_bump");
            }
            break;
        case RAM_ALERT:
            t->dir = b->x >= t->x ? 1 : -1;
            if (t->t >= 40) {
                if (on_it) { t->state = RAM_CHARGE; t->t = 0; tkw_sfx("tkw_charge"); }
                else t->state = RAM_IDLE;
            }
            break;
        case RAM_CHARGE: {
            t->x += t->dir * 2.0f;
            const TkwPlat *p = &tkw_w.plat[t->plat];
            if (near && hero_active() && h->ps != PS_HAUL) {
                knock(t->dir * 3.0f, -2.2f, 30);
                t->state = RAM_REST;
                t->t = 0;
            } else if (t->x < p->x - 2 || t->x > p->x + p->w + 2) {
                /* over the edge: into the sea */
                t->state = RAM_FALL;
                t->vx = t->dir * 2.0f;
                t->vy = 0;
                t->plat = -1;
            }
            break;
        }
        case RAM_REST:
            if (t->t >= 60) t->state = RAM_IDLE;
            break;
        case RAM_FALL:
            t->vy += TKW_G;
            t->x += t->vx;
            t->y += t->vy;
            if (t->y > tkw_w.sea_y + 10) { t->alive = 0; tkw_sfx("tkw_splash"); }
            break;
        }
    }
}

static void foam_update(void) {
    TkwBody *b = &tkg.h.b;
    for (int i = 0; i < tkw_w.nplat; i++) {
        TkwPlat *p = &tkw_w.plat[i];
        if (p->kind != PK_FOAM) continue;
        if (p->gone) {
            /* it forms again once he is clear of it */
            if (p->back > 0 && --p->back == 0) { p->gone = 0; p->timer = -1; }
            continue;
        }
        if (p->timer >= 0 && ++p->timer >= TKW_FOAM_T) {
            p->gone = 1;
            p->back = TKW_FOAM_BACK;
            tkw_sfx("tkw_crumble");
            if (b->plat == i) {
                b->mode = BM_AIR;
                b->plat = -1;
                b->vy = 0;
            }
        }
    }
}

static void things_update(void) {
    TkwHero *h = &tkg.h;
    TkwBody *b = &h->b;
    float hx = b->x, hy = b->y - 6;
    tkg.sign_near = -1;
    bool active = hero_active();
    for (int i = 0; i < tkw_w.nth; i++) {
        TkwThing *t = &tkw_w.th[i];
        if (!t->alive) continue;
        if (TKW_IS_PICKUP(t->kind)) {
            if (t->kind == TH_SPIRAL && t->state == 0) {
                /* a spiral shell falls to the floor first */
                t->vy = fminf(t->vy + TKW_G, 3.0f);
                t->y += t->vy;
                if (t->y >= 290) { t->y = 290; t->state = 1; }
            }
            if (!active) continue;
            float dx = hx - t->x, dy = hy - t->y, d = sqrtf(dx * dx + dy * dy);
            if (d < 9) { collect(t); continue; }
            if (d < TKW_MAGNET && (t->kind != TH_SPIRAL || t->state)) {
                t->x += dx / d * 2.2f;
                t->y += dy / d * 2.2f;
            }
            continue;
        }
        switch (t->kind) {
        case TH_SIGN:
            if (fabsf(hx - t->x) < 22 && fabsf(b->y - t->y) < 40) {
                tkg.sign_near = t->arg;
                if (!(tkg.signs_run & (1 << t->arg))) {
                    tkg.signs_run |= 1 << t->arg;
                    tks.signs |= 1u << t->arg;
                    tkw_sfx("tkw_sign");
                }
            }
            break;
        case TH_SECRET:
            if (fabsf(hx - t->x) < 26 && fabsf(hy - t->y) < 30) {
                tkg.sign_near = TKW_SIGNS;
                if (!tkg.secret_read) {
                    tkg.secret_read = true;
                    tks.signs |= 1u << TKW_SIGNS;
                    tkw_sfx("tkw_sign");
                }
            }
            break;
        case TH_BELL:
            if (t->state == 0 && active && fabsf(hx - t->x) < 10 && fabsf(hy - (t->y - 10)) < 12) {
                t->state = 1;
                if (tkg.wind) { tkw_set_wind(0, 0); tkw_say("THE WIND DROPS"); }
                else { tkw_set_wind(rng_chance(&tkg.rng, 50) ? 1 : -1, 0); tkw_say("THE WIND RISES"); }
                tkw_sfx("tkw_bell");
            }
            break;
        case TH_PERCH:
            /* walk west and the terns that came along each drop a spiral shell */
            if (t->state == 0 && hx < t->x + 40 && b->mode == BM_GROUND) {
                bool any = false;
                for (int j = 0; j < tkw_w.nth; j++) any |= tkw_w.th[j].alive && tkw_w.th[j].kind == TH_SPIRAL && tkw_w.th[j].state == 0;
                if (!any) {
                    t->state = 1;
                    int s = tkw_add_thing(&tkw_w, TH_SPIRAL, t->x, t->y - 16, -1);
                    (void)s;
                    tkw_sfx("tkw_tern");
                }
            }
            break;
        default: break;
        }
    }
}

/* things that need Burl standing beside them */
static void interact(void) {
    TkwHero *h = &tkg.h;
    TkwBody *b = &h->b;
    if (b->mode != BM_GROUND || h->charging) return;
    for (int i = 0; i < tkw_w.nth; i++) {
        TkwThing *t = &tkw_w.th[i];
        if (!t->alive || t->plat != b->plat) continue;
        float d = fabsf(b->x - t->x);
        switch (t->kind) {
        case TH_LIGHTHOUSE:
            if (t->state == 0 && d < 14) {
                t->state = 1;
                tkg.terns++;
                tkg.terns_found++;
                tkw_say("A TERN WILL WATCH OVER YOU");
                tkw_sfx("tkw_tern");
            }
            break;
        case TH_SHOP:
            if (d < 12) {
                h->ps = PS_SHOP;
                h->ps_t = 0;
                h->shop_th = i;
                h->shop_sel = 0;
                h->shop_cool = 20;
                tkw_sfx("tkw_shopbell");
                return;
            }
            break;
        case TH_CHEST:
            if (t->state == 0 && d < 12) {
                if (tkg.keys > 0) {
                    tkg.keys--;
                    t->state = 1;
                    tkg.chests++;
                    tkw_add_shells(10);
                    tkw_say("TEN SHELLS!");
                    tkw_sfx("tkw_chest");
                } else if (tkg.msg_t <= 0) {
                    tkw_say("LOCKED. IT NEEDS A KEY");
                }
            }
            break;
        case TH_ELDER:
            if (d < 20) {
                tkg.state = TS_TALK;
                tkg.state_t = 0;
                return;
            }
            break;
        default: break;
        }
    }
    /* the door on the last islet */
    if (!tkg.in_hall && b->plat == tkw_w.goal_plat) {
        const TkwPlat *p = &tkw_w.plat[b->plat];
        if (fabsf(b->x - (p->x + p->w - 24)) < 8) {
            h->ps = PS_DOOR;
            h->ps_t = 0;
            tkw_sfx("tkw_door");
        }
    }
}

/* ---- items --------------------------------------------------------------- */

int tkw_menu_items(int *list) {
    int n = 0;
    for (int i = 0; i < IT_COUNT; i++)
        if (tkg.inv[i] > 0) list[n++] = i;
    return n;
}

bool tkw_can_use(int it) {
    const TkwHero *h = &tkg.h;
    const TkwBody *b = &h->b;
    bool ground = b->mode == BM_GROUND || b->mode == BM_RIDE;
    switch (it) {
    case IT_BOBBER: case IT_SPYGLASS: case IT_GRAPNEL: return ground;
    case IT_KITE: return !b->kite;
    case IT_SPINNER: return !h->spinner;
    case IT_TIN: return b->stamina < TKW_STAMINA;
    default: return false;
    }
}

static void use_item(int it) {
    TkwHero *h = &tkg.h;
    TkwBody *b = &h->b;
    if (!tkw_can_use(it)) { tkw_sfx("tkw_nope"); h->ps = PS_PLAY; return; }
    h->ps = PS_PLAY;
    h->ps_t = 0;
    switch (it) {
    case IT_BOBBER:
        h->ps = PS_THROW;
        h->charging = false;
        h->charge = 0;
        break;
    case IT_SPYGLASS:
        h->ps = PS_LOOK;
        h->look_x = tkg.cam_x + SCREEN_W / 2.0f;
        h->look_y = tkg.cam_y + TKW_HUD_H + TKW_VIEW_H / 2.0f;
        break;
    case IT_GRAPNEL: {
        float a = h->aim * PI_F / 180.0f;
        tkg.inv[it]--;
        h->ps = PS_ROPE;
        h->rope_x = b->x;
        h->rope_y = b->y - 6;
        h->rope_dx = cosf(a) * h->face;
        h->rope_dy = -sinf(a);
        h->rope_len = 0;
        h->rope_plat = -1;
        tkw_sfx("tkw_rope");
        break;
    }
    case IT_KITE:
        tkg.inv[it]--;
        b->kite = true;
        tkw_sfx("tkw_item");
        break;
    case IT_SPINNER:
        tkg.inv[it]--;
        h->spinner = true;
        h->spin_fuel = TKW_SPIN_FUEL;
        if (b->mode != BM_AIR) {
            b->mode = BM_AIR;
            b->plat = -1;
            b->ride = -1;
            b->vy = 0;
            b->y -= 2; /* up off the ground, hovering */
        }
        tkw_sfx("tkw_item");
        break;
    case IT_TIN:
        tkg.inv[it]--;
        b->stamina = TKW_STAMINA;
        tkw_sfx("tkw_sprat");
        break;
    }
}

/* ---- Burl ----------------------------------------------------------------- */

static int dpad_x(uint16_t in) { return (HELD(BTN_RIGHT) ? 1 : 0) - (HELD(BTN_LEFT) ? 1 : 0); }
static int dpad_y(uint16_t in) { return (HELD(BTN_DOWN) ? 1 : 0) - (HELD(BTN_UP) ? 1 : 0); }

static void aim_input(uint16_t in) {
    TkwHero *h = &tkg.h;
    if (HELD(BTN_UP)) h->aim = fminf(TKW_AIM_MAX, h->aim + TKW_AIM_STEP);
    if (HELD(BTN_DOWN)) h->aim = fmaxf(TKW_AIM_MIN, h->aim - TKW_AIM_STEP);
}

static void launch(void) {
    TkwHero *h = &tkg.h;
    TkwBody *b = &h->b;
    if (b->mode == BM_GROUND) {
        h->takeoff = b->plat;
        h->takeoff_x = b->x;
    }
    bool ride = b->mode == BM_RIDE;
    if (ride) b->plat = -1;
    tkw_body_launch(b, &tkw_w, h->face, h->aim, h->charge);
    if (ride && b->mode == BM_LUNGE) { b->mode = BM_AIR; b->vy = 0; } /* no ground to slide along */
    b->ride = -1;
    h->charging = false;
    h->flap_ok = true;
    h->charge = 0;
    h->red_line = -1;
    tkw_sfx(b->mode == BM_LUNGE ? "tkw_lunge" : "tkw_jump");
}

static void hero_air(uint16_t in, bool control) {
    TkwHero *h = &tkg.h;
    TkwBody *b = &h->b;
    float wind = (float)tkg.wind;
    if (h->spinner) {
        /* the spinner: straight where the pad points, no gravity, its own bar */
        int dx = control ? dpad_x(in) : 0, dy = control ? dpad_y(in) : 0;
        float len = (dx || dy) ? sqrtf((float)(dx * dx + dy * dy)) : 1.0f;
        b->vx = dx / len * TKW_SPIN_SPEED - wind * TKW_WIND;
        b->vy = dy / len * TKW_SPIN_SPEED - (b->kite ? TKW_G_KITE : TKW_G);
        tkw_body_air(b, &tkw_w, 0, 0, false, wind);
        if (--h->spin_fuel <= 0) { h->spinner = false; tkw_say("THE SPINNER IS SPENT"); }
        h->flapping = false;
    } else {
        h->flapping = control && h->flap_ok && HELD(BTN_A) && b->stamina > 0 && h->hurt_t <= 0;
        int dx = dpad_x(in), dy = dpad_y(in);
        tkw_body_air(b, &tkw_w, (float)dx, (float)dy, h->flapping, wind);
        if (h->flapping && (tkg.frame_t % 8) == 0) tkw_sfx("tkw_flap");
    }
    if (b->bumped) tkw_sfx("tkw_bump");
    if (b->landed >= 0) {
        tkw_sfx("tkw_land");
        h->hurt_t = 0;
        tkw_on_land(b->landed);
    }
}

static void hero_ground(uint16_t in, bool control) {
    TkwHero *h = &tkg.h;
    TkwBody *b = &h->b;
    int dir = 0;
    if (control && !h->charging && b->mode != BM_LUNGE) {
        dir = dpad_x(in);
        /* a press the other way turns him first, on the spot */
        if (dir && dir != h->face) {
            h->face = dir;
            dir = 0;
        }
    }
    if (b->mode == BM_RIDE) {
        if (b->ride < 0 || !tkw_w.manta[b->ride].alive) { b->mode = BM_AIR; b->ride = -1; return; }
        const TkwManta *m = &tkw_w.manta[b->ride];
        if (dir) b->x = fclamp(b->x + dir * TKW_WALK, m->x - TKW_MANTA_HW + 2, m->x + TKW_MANTA_HW - 2);
        return;
    }
    tkw_body_ground(b, &tkw_w, dir);
    if (dir && (tkg.frame_t % 16) == 0) tkw_sfx("tkw_scoot");
}

static void play_update(uint16_t in) {
    TkwHero *h = &tkg.h;
    TkwBody *b = &h->b;
    bool control = h->hurt_t <= 0;
    if (h->hurt_t > 0) h->hurt_t--;
    if (grounded(b)) {
        if (control && b->mode != BM_LUNGE) {
            if (h->charging) {
                aim_input(in);
                if (HELD(BTN_A)) h->charge = imin(TKW_CHARGE_T, h->charge + 1);
                else { launch(); return; }
            } else if (PRESSED(BTN_A)) {
                h->charging = true;
                h->charge = 0;
                tkw_sfx("tkw_charge0");
            } else if (PRESSED(BTN_B)) {
                h->ps = PS_MENU;
                h->menu_sel = 0;
                return;
            } else {
                aim_input(in);
            }
        }
        hero_ground(in, control);
        if (b->mode == BM_AIR && h->charging) {
            /* the islet went from under him mid-charge */
            h->charging = false;
            h->charge = 0;
            h->flap_ok = false;
        }
        interact();
        return;
    }
    /* in the air */
    if (!HELD(BTN_A)) h->flap_ok = true;
    if (control && PRESSED(BTN_B)) {
        h->ps = PS_MENU;
        h->menu_sel = 0;
    }
    hero_air(in, control);
}

static void menu_update(uint16_t in) {
    TkwHero *h = &tkg.h;
    TkwBody *b = &h->b;
    int list[IT_COUNT], n = tkw_menu_items(list);
    if (PRESSED(BTN_RIGHT)) { h->menu_sel = (h->menu_sel + 1) % (n + 1); tkw_sfx("ui_move"); }
    if (PRESSED(BTN_LEFT)) { h->menu_sel = (h->menu_sel + n) % (n + 1); tkw_sfx("ui_move"); }
    /* the world doesn't wait */
    if (grounded(b)) hero_ground(0, false);
    else hero_air(0, false);
    if (h->ps != PS_MENU) return;
    if (!HELD(BTN_B)) {
        if (h->menu_sel == 0 || h->menu_sel > n) { h->ps = PS_PLAY; tkw_sfx("ui_back"); }
        else use_item(list[h->menu_sel - 1]);
    }
}

static void throw_update(uint16_t in) {
    TkwHero *h = &tkg.h;
    TkwBody *b = &h->b;
    if (!grounded(b)) { h->ps = PS_PLAY; return; }
    if (PRESSED(BTN_B) && !h->charging) { h->ps = PS_PLAY; tkw_sfx("ui_back"); return; }
    aim_input(in);
    if (!h->charging) {
        int d = dpad_x(in);
        if (d) h->face = d;
        if (PRESSED(BTN_A)) { h->charging = true; h->charge = 0; }
        hero_ground(0, false);
        return;
    }
    if (HELD(BTN_A)) { h->charge = imin(TKW_CHARGE_T, h->charge + 1); return; }
    /* thrown: it flies as Burl would */
    tkg.inv[IT_BOBBER]--;
    memset(&h->ball, 0, sizeof h->ball);
    h->ball.x = b->x;
    h->ball.y = b->y;
    h->ball.plat = b->mode == BM_RIDE ? -1 : b->plat;
    h->ball.ride = -1;
    h->ball.mode = BM_GROUND;
    tkw_body_launch(&h->ball, &tkw_w, h->face, h->aim, h->charge);
    if (h->ball.mode == BM_LUNGE && h->ball.plat < 0) { h->ball.mode = BM_AIR; h->ball.vy = 0; }
    h->ball_charge = h->charge;
    h->red_line = h->charge;
    h->ball_t = 0;
    h->charging = false;
    h->charge = 0;
    h->ps = PS_BALL;
    tkw_sfx("tkw_throw");
}

static void ball_update(void) {
    TkwHero *h = &tkg.h;
    TkwBody *ball = &h->ball;
    h->ball_t++;
    if (ball->mode == BM_AIR) {
        tkw_body_air(ball, &tkw_w, 0, 0, false, (float)tkg.wind);
        if (ball->landed >= 0) tkw_sfx("tkw_ballbump");
        /* a hard throw bursts a manta */
        if (h->ball_charge >= 45)
            for (int k = 0; k < tkw_w.nmanta; k++)
                if (in_manta(&tkw_w.manta[k], ball->x, ball->y - 4, 4)) kill_manta(k);
    } else if (ball->mode == BM_LUNGE || ball->mode == BM_GROUND) {
        tkw_body_ground(ball, &tkw_w, 0);
    }
    bool settled = ball->mode == BM_GROUND && h->ps_t == 0;
    if (settled) h->ps_t = h->ball_t;
    if ((h->ps_t && h->ball_t - h->ps_t > 40) || ball->y > tkw_w.sea_y + 20 || h->ball_t > 360) {
        if (ball->y > tkw_w.sea_y + 20) tkw_sfx("tkw_splash");
        h->ps = PS_PLAY;
        h->ps_t = 0;
    }
    hero_ground(0, false);
}

static void look_update(uint16_t in) {
    TkwHero *h = &tkg.h;
    h->look_x += dpad_x(in) * 4.0f;
    h->look_y += dpad_y(in) * 4.0f;
    float minx = tkg.in_hall ? TKW_HALL_EDGE - 16.0f + SCREEN_W / 2.0f : SCREEN_W / 2.0f; /* the shelf stays hidden */
    h->look_x = fclamp(h->look_x, minx, tkw_w.w - SCREEN_W / 2.0f);
    h->look_y = fclamp(h->look_y, TKW_HUD_H + TKW_VIEW_H / 2.0f, tkw_w.h - TKW_VIEW_H / 2.0f);
    if (h->ps_t > 4 && (PRESSED(BTN_A) || PRESSED(BTN_B))) {
        tkg.inv[IT_SPYGLASS]--;
        h->ps = PS_PLAY;
        tkw_sfx("ui_back");
    }
    hero_ground(0, false);
}

static int plat_at_point(float x, float y, int skip) {
    for (int i = 0; i < tkw_w.nplat; i++) {
        const TkwPlat *p = &tkw_w.plat[i];
        if (i == skip || p->gone) continue;
        if (x >= p->x && x <= p->x + p->w && y >= p->y - 2 && y <= p->y + p->h) return i;
    }
    return -1;
}

static void rope_update(void) {
    TkwHero *h = &tkg.h;
    TkwBody *b = &h->b;
    float sx = b->x, sy = b->y - 6;
    for (int s = 0; s < 7; s++) {
        h->rope_len += 1.0f;
        float x = sx + h->rope_dx * h->rope_len, y = sy + h->rope_dy * h->rope_len;
        h->rope_x = x;
        h->rope_y = y;
        for (int k = 0; k < tkw_w.nmanta; k++)
            if (in_manta(&tkw_w.manta[k], x, y, 1)) {
                kill_manta(k);
                h->ps = PS_PLAY;
                return;
            }
        int p = plat_at_point(x, y, b->mode == BM_RIDE ? -1 : b->plat);
        if (p >= 0) {
            h->rope_plat = p;
            h->ps = PS_HAUL;
            b->mode = BM_AIR;
            b->plat = -1;
            b->ride = -1;
            tkw_sfx("tkw_hook");
            return;
        }
        if (h->rope_len >= 240 || x < 0 || x > tkw_w.w || y < 0 || y > tkw_w.h) {
            h->ps = PS_PLAY;
            tkw_sfx("tkw_nope");
            return;
        }
    }
}

static void haul_update(void) {
    TkwHero *h = &tkg.h;
    TkwBody *b = &h->b;
    const TkwPlat *p = &tkw_w.plat[h->rope_plat];
    float tx = fclamp(h->rope_x, p->x + 3.0f, p->x + p->w - 3.0f), ty = (float)p->y;
    float dx = tx - b->x, dy = ty - b->y, d = sqrtf(dx * dx + dy * dy);
    if (p->gone || d <= 5.0f) {
        if (p->gone) { b->mode = BM_AIR; b->vx = b->vy = 0; h->ps = PS_PLAY; return; }
        tkw_hero_place(h->rope_plat, tx);
        h->ps = PS_PLAY;
        tkw_sfx("tkw_land");
        tkw_on_land(h->rope_plat);
        return;
    }
    b->x += dx / d * 5.0f;
    b->y += dy / d * 5.0f;
}

static void shop_update(uint16_t in) {
    TkwHero *h = &tkg.h;
    TkwThing *t = &tkw_w.th[h->shop_th];
    int ware[2] = {t->arg & 15, t->arg >> 4};
    h->ps_t++;
    if (h->shop_cool > 0) h->shop_cool--;
    if (PRESSED(BTN_LEFT) || PRESSED(BTN_UP)) { h->shop_sel = (h->shop_sel + 2) % 3; tkw_sfx("ui_move"); }
    if (PRESSED(BTN_RIGHT) || PRESSED(BTN_DOWN)) { h->shop_sel = (h->shop_sel + 1) % 3; tkw_sfx("ui_move"); }
    bool leave = PRESSED(BTN_B) || (PRESSED(BTN_A) && h->shop_sel == 2);
    if (!leave && PRESSED(BTN_A) && h->shop_cool == 0) {
        /* one press, one purchase */
        int it = ware[h->shop_sel], cost = TKW_ITEM_COST[it];
        if (tkg.shells >= cost && tkg.inv[it] < TKW_ITEM_MAX) {
            tkg.shells -= cost;
            tkg.spent += cost;
            tkg.inv[it]++;
            h->shop_cool = 15;
            tkw_sfx("tkw_buy");
        } else {
            tkw_sfx("tkw_nope");
        }
    }
    if (leave) {
        /* she won't be here again */
        t->alive = 0;
        h->ps = PS_PLAY;
        tkw_sfx("tkw_poof");
    }
}

static void rescue_update(void) {
    TkwHero *h = &tkg.h;
    if (++h->ps_t < 80) return;
    int p = h->takeoff;
    TkwPlat *pl = &tkw_w.plat[p];
    if (pl->gone) { pl->gone = 0; pl->back = 0; }
    pl->timer = -1;
    if (!tkg.god) {
        tkg.terns--;
        tkg.terns_used++;
    }
    tkg.rescues++;
    tkw_hero_place(p, h->takeoff_x);
    h->spinner = false;
    h->ps = PS_PLAY;
    h->ps_t = 0;
    h->flap_ok = false;
    tkw_on_land(p);
    tkw_camera(false);
}

void tkw_enter_hall(void) {
    tkg.in_hall = true;
    tkg.hall_terns = tkg.terns; /* they fly ahead and perch */
    tkg.terns = 0;
    tkw_set_wind(0, 0);
    tkw_gen_hall(tkg.hall_terns);
    TkwHero *h = &tkg.h;
    h->ps = PS_PLAY;
    h->spinner = false;
    h->red_line = -1;
    tkw_hero_place(tkw_w.start_plat, TKW_HALL_DOOR);
    h->takeoff = tkw_w.start_plat;
    h->takeoff_x = TKW_HALL_DOOR;
    h->face = -1;
    tkg.state = TS_HALL;
    tkg.state_t = 0;
    music_play(TKW_MUS_HALL);
    tkw_camera(true);
}

void tkw_journey_update(uint16_t in) {
    TkwHero *h = &tkg.h;
    TkwBody *b = &h->b;
    tkg.journey_frames++;
    if (tkg.msg_t > 0) tkg.msg_t--;
    if (tkg.shake > 0) tkg.shake--;
    if (tkg.wind_t > 0 && --tkg.wind_t == 0) {
        tkg.wind = 0;
        tkw_say("THE WIND DIES DOWN");
    }
    if (h->ps != PS_SHOP) {
        tkg.frame_t++;
        mantas_update();
        foam_update();
        rams_update();
    }
    switch (h->ps) {
    case PS_PLAY: play_update(in); break;
    case PS_MENU: menu_update(in); break;
    case PS_THROW: throw_update(in); break;
    case PS_BALL: ball_update(); break;
    case PS_LOOK: h->ps_t++; look_update(in); break;
    case PS_ROPE: rope_update(); break;
    case PS_HAUL: haul_update(); break;
    case PS_SHOP: shop_update(in); break;
    case PS_RESCUE: rescue_update(); break;
    case PS_SINK:
        if (++h->ps_t >= 100) { tkg.state = TS_WAKE; tkg.state_t = 0; }
        break;
    case PS_DOOR:
        if (++h->ps_t >= 50) tkw_enter_hall();
        break;
    }
    if (h->ps != PS_SHOP) things_update();
    /* into the sea */
    if ((h->ps == PS_PLAY || h->ps == PS_MENU) && b->y > tkw_w.sea_y) fall_in_sea();
    h->anim++;
    /* rain once the storm wind has come */
    tkw_camera(false);
}

