/* TUSKWIND - the brawl: two walruses on one screen that wraps at the
 * sides, the same aimed and charged jumps, and the islets crumbling away
 * one by one. Bump the other one into the sea. Best of 1 to 9 rounds. */
#include "tuskwind.h"

static uint16_t prev_in[2];
static int end_t, bump_cool;

void tkw_brawl_start_round(void) {
    tkg.round_no++;
    uint64_t seed = (uint64_t)rng_next(&tkg.rng) << 32 | rng_next(&tkg.rng);
    tkw_gen_brawl(seed);
    for (int p = 0; p < 2; p++) {
        TkwBrawler *br = &tkg.br[p];
        int wins = br->wins;
        memset(br, 0, sizeof *br);
        br->wins = wins;
        const TkwPlat *pl = &tkw_w.plat[p];
        br->b.x = pl->x + pl->w / 2.0f;
        br->b.y = (float)pl->y;
        br->b.plat = p;
        br->b.ride = -1;
        br->b.mode = BM_GROUND;
        br->b.ignore = -1;
        br->b.stamina = TKW_BRAWL_STAMINA;
        br->face = p == 0 ? 1 : -1;
        br->aim = TKW_AIM_START;
    }
    tkg.crumble_t = 0;
    tkg.sprat_t = 0;
    tkg.round_winner = -1;
    end_t = 0;
    bump_cool = 0;
    prev_in[0] = prev_in[1] = 0xFFFF;
    tkg.cam_x = tkg.cam_y = 0;
}

void tkw_brawl_start_match(void) {
    rng_seed(&tkg.rng, (uint64_t)rng_next(&g_rng) << 32 | rng_next(&g_rng));
    tkg.br[0].wins = tkg.br[1].wins = 0;
    tkg.round_no = 0;
    tkw_brawl_start_round();
}

static bool on_ground(const TkwBody *b) { return b->mode == BM_GROUND || b->mode == BM_LUNGE; }

static void brawler_step(TkwBrawler *p, uint16_t in, uint16_t prev) {
    TkwBody *b = &p->b;
    if (p->out) return;
    bool control = p->hurt_t <= 0;
    if (p->hurt_t > 0) p->hurt_t--;
    p->anim++;
    int dx = ((in & BTN_RIGHT) ? 1 : 0) - ((in & BTN_LEFT) ? 1 : 0);
    int dy = ((in & BTN_DOWN) ? 1 : 0) - ((in & BTN_UP) ? 1 : 0);
    bool a_held = (in & BTN_A) != 0, a_pressed = a_held && !(prev & BTN_A);
    if (on_ground(b)) {
        int walk = 0;
        if (control && b->mode != BM_LUNGE) {
            if (in & BTN_UP) p->aim = fminf(TKW_AIM_MAX, p->aim + TKW_AIM_STEP);
            if (in & BTN_DOWN) p->aim = fmaxf(TKW_AIM_MIN, p->aim - TKW_AIM_STEP);
            if (p->charging) {
                if ((in & BTN_B) && !(prev & BTN_B)) {
                    p->charging = false; /* B calls it off */
                    p->charge = 0;
                } else if (a_held) p->charge = imin(TKW_CHARGE_T, p->charge + 1);
                else {
                    tkw_body_launch(b, &tkw_w, p->face, p->aim, p->charge);
                    p->charging = false;
                    p->charge = 0;
                    p->flap_ok = true;
                    sfx_play_name(b->mode == BM_LUNGE ? "tkw_lunge" : "tkw_jump");
                    return;
                }
            } else if (a_pressed) {
                p->charging = true;
                p->charge = 0;
            } else if (dx) {
                /* as on the journey: a press the other way turns first */
                if (dx != p->face) p->face = dx;
                else walk = dx;
            }
        }
        tkw_body_ground(b, &tkw_w, walk);
        if (b->mode == BM_AIR) p->flap_ok = false;
        return;
    }
    if (!a_held) p->flap_ok = true;
    p->flapping = control && p->flap_ok && a_held && b->stamina > 0;
    tkw_body_air(b, &tkw_w, (float)dx, (float)dy, p->flapping, 0);
    if (b->landed >= 0) { p->hurt_t = 0; sfx_play_name("tkw_land"); }
    if (b->bumped) sfx_play_name("tkw_bump");
}

static float speed_of(const TkwBody *b) { return sqrtf(b->vx * b->vx + b->vy * b->vy); }

static void bump(void) {
    TkwBrawler *p0 = &tkg.br[0], *p1 = &tkg.br[1];
    if (p0->out || p1->out) return;
    if (bump_cool > 0) { bump_cool--; return; }
    float dx = p1->b.x - p0->b.x, dy = p1->b.y - p0->b.y;
    if (dx > TKW_BRAWL_W / 2.0f) dx -= TKW_BRAWL_W;
    if (dx < -TKW_BRAWL_W / 2.0f) dx += TKW_BRAWL_W;
    if (fabsf(dx) >= 2 * TKW_HW + 1 || fabsf(dy) >= TKW_BH) return;
    float s0 = speed_of(&p0->b), s1 = speed_of(&p1->b);
    int att = s0 >= s1 ? 0 : 1;
    TkwBrawler *a = &tkg.br[att], *v = &tkg.br[1 - att];
    float dir = (att == 0 ? dx : -dx) >= 0 ? 1.0f : -1.0f; /* from the bumper to the bumped */
    if (fmaxf(s0, s1) >= 1.2f) {
        /* a real bump: the one that came in faster sends the other flying */
        v->b.mode = BM_AIR;
        v->b.plat = -1;
        v->b.ignore = -1;
        v->b.vx = a->b.vx * 0.9f + dir * 1.6f;
        v->b.vy = fminf(a->b.vy, 0) * 0.5f - 1.8f;
        v->hurt_t = 24;
        v->charging = false;
        v->charge = 0;
        a->b.vx *= -0.25f;
        a->b.vy *= 0.5f;
        if (a->b.mode == BM_LUNGE) { a->b.mode = BM_GROUND; a->b.vx = 0; }
        bump_cool = 12;
        tkg.shake = 6;
        sfx_play_name("tkw_bump");
    } else if (on_ground(&p0->b) && on_ground(&p1->b)) {
        /* shoulder to shoulder: they just don't pass through each other */
        float push = (2 * TKW_HW + 1 - fabsf(dx)) / 2.0f;
        float s = dx >= 0 ? 1.0f : -1.0f;
        for (int k = 0; k < 2; k++) {
            TkwBody *b = &tkg.br[k].b;
            const TkwPlat *pl = &tkw_w.plat[b->plat];
            b->x = fclamp(b->x + (k == 0 ? -s : s) * push, pl->x + 2.0f, pl->x + pl->w - 2.0f);
        }
    }
}

static void crumble(void) {
    tkg.crumble_t++;
    /* after ten seconds an islet starts to go every two and a half */
    if (tkg.crumble_t > 600 && (tkg.crumble_t - 600) % 150 == 1) {
        int pick[TKW_MAX_PLATS], n = 0;
        for (int i = 0; i < tkw_w.nplat; i++)
            if (!tkw_w.plat[i].gone && tkw_w.plat[i].brawl_t == 0) pick[n++] = i;
        if (n) {
            tkw_w.plat[pick[rng_range(&tkg.rng, 0, n - 1)]].brawl_t = 90;
            sfx_play_name("tkw_crumble");
        }
    }
    for (int i = 0; i < tkw_w.nplat; i++) {
        TkwPlat *p = &tkw_w.plat[i];
        if (p->gone || p->brawl_t == 0) continue;
        if (--p->brawl_t == 0) p->gone = 1;
    }
}

static void sprats(void) {
    if (++tkg.sprat_t % 420 == 0 && tkw_count_things(TH_SPRAT) < 2)
        tkw_add_thing(&tkw_w, TH_SPRAT, (float)rng_range(&tkg.rng, 20, TKW_BRAWL_W - 20), (float)rng_range(&tkg.rng, 30, 90), -1);
    for (int i = 0; i < tkw_w.nth; i++) {
        TkwThing *t = &tkw_w.th[i];
        if (!t->alive || t->kind != TH_SPRAT) continue;
        for (int p = 0; p < 2; p++) {
            TkwBody *b = &tkg.br[p].b;
            if (tkg.br[p].out || fabsf(b->x - t->x) > 9 || fabsf(b->y - 6 - t->y) > 9) continue;
            t->alive = 0;
            b->stamina = imin(TKW_BRAWL_STAMINA, b->stamina + TKW_BRAWL_STAMINA / 4);
            sfx_play_name("tkw_sprat");
            break;
        }
    }
}

void tkw_brawl_update(uint16_t in0, uint16_t in1) {
    tkg.frame_t++;
    if (tkg.shake > 0) tkg.shake--;
    uint16_t in[2] = {in0, in1};
    for (int p = 0; p < 2; p++) {
        brawler_step(&tkg.br[p], in[p], prev_in[p]);
        prev_in[p] = in[p];
    }
    bump();
    crumble();
    sprats();
    for (int p = 0; p < 2; p++) {
        TkwBrawler *br = &tkg.br[p];
        if (!br->out && br->b.y > TKW_BRAWL_SEA + 8) {
            br->out = true;
            sfx_play_name("tkw_splash");
            if (!end_t) end_t = 1;
        }
    }
    /* a moment to see whether both went in */
    if (end_t && ++end_t > 30) {
        bool o0 = tkg.br[0].out, o1 = tkg.br[1].out;
        tkg.round_winner = o0 && o1 ? -1 : o0 ? 1 : 0;
        if (tkg.round_winner >= 0) tkg.br[tkg.round_winner].wins++;
        else tkg.round_no--; /* a draw is played again */
        tkg.state = TS_BRAWL_ROUND;
        tkg.state_t = 0;
        music_play(TKW_MUS_ROUND);
    }
}

/* ---- a simple opponent, for the tests and the demo ------------------------------ */

static int plan_aim = -1, plan_charge, plan_face;

int tkw_brawl_bot(void) {
    TkwBrawler *me = &tkg.br[1], *op = &tkg.br[0];
    if (me->out || tkg.state != TS_BRAWL) return 0;
    int m = 0;
    if (on_ground(&me->b)) {
        if (me->b.mode == BM_LUNGE) return 0;
        if (me->charging) {
            if (me->charge < plan_charge) m |= BTN_A;
            else plan_aim = -1;
            return m;
        }
        if (plan_aim < 0) {
            /* aim to land on the other one */
            float best = 1e9f;
            for (int ai = 4; ai <= 15; ai++)
                for (int c = 10; c <= TKW_CHARGE_T; c += 5)
                    for (int f = -1; f <= 1; f += 2) {
                        TkwBody s = me->b;
                        tkw_body_launch(&s, &tkw_w, f, ai * 6.0f, c);
                        for (int k = 0; k < 240 && s.mode == BM_AIR; k++) tkw_body_air(&s, &tkw_w, 0, 0, false, 0);
                        if (s.mode == BM_AIR || s.y > TKW_BRAWL_SEA) continue;
                        float d = fabsf(s.x - op->b.x) + fabsf(s.y - op->b.y);
                        if (d < best) { best = d; plan_aim = ai * 6; plan_charge = c; plan_face = f; }
                    }
            if (plan_aim < 0) { plan_aim = 45; plan_charge = 20; plan_face = me->face; }
        }
        if (me->face != plan_face) return plan_face > 0 ? BTN_RIGHT : BTN_LEFT;
        if (me->aim < plan_aim - 0.1f) return BTN_UP;
        if (me->aim > plan_aim + 0.1f) return BTN_DOWN;
        if (tkg.frame_t % 40 < 20) return 0; /* a breath between jumps */
        return BTN_A;
    }
    /* in the air: flap back over the nearest islet when sinking */
    plan_aim = -1;
    if (me->b.vy > 0 && me->b.y > 120 && me->b.stamina > 0) {
        float bd = 1e9f, tx = me->b.x;
        for (int i = 0; i < tkw_w.nplat; i++) {
            const TkwPlat *p = &tkw_w.plat[i];
            if (p->gone) continue;
            float cx = p->x + p->w / 2.0f, d = fabsf(cx - me->b.x);
            if (d < bd) { bd = d; tx = cx; }
        }
        m |= BTN_A | BTN_UP;
        if (tx > me->b.x + 4) m |= BTN_RIGHT;
        if (tx < me->b.x - 4) m |= BTN_LEFT;
    }
    return m;
}
