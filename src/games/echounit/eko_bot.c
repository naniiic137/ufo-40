/* ECHO UNIT - the demo player, for the tests. It reads the game the way a
 * player reads the screen and answers with real button presses. In the
 * action pieces it tries a handful of ways to play the next second or so
 * in a copy of the game (the real rules, run ahead) and keeps the one that
 * stays healthiest and gets furthest; in the jailbreak it looks a few turns
 * ahead the same way. On the menus it taps A (and lets go between taps). */
#include "eko.h"

int eko_bot_cherry;
bool eko_sim_done;

static EkoGame bak;
static bool bot_bold; /* stuck a long while: a knock is a fair price for headway */
#define TAPA 0x100

/* ---- running a policy in a copy of the game ---------------------------------------------- */

typedef int (*Policy)(int param, int f);

static int sim_score_mode(void);

static int frame_buttons(int b, int f) {
    int out = b & 0xFF;
    if ((b & TAPA) && (f & 3) < 2) out |= EKO_MAIN;
    return out;
}

static int run_policy_from(Policy pol, int param, int frames, int off) {
    memcpy(&bak, &eg, sizeof eg);
    eko_simulating = true;
    eko_sim_done = false;
    int lost0 = eg.lives_lost, hits0 = eg.hits_taken, hp0 = eko_hp();
    int f = 0;
    for (; f < frames; f++) {
        int b = frame_buttons(pol(param, f + off), f + off);
        eg.prev = eg.in;
        eg.in = (uint16_t)b;
        eko_step_play();
        if (eg.state != ES_PLAY || eko_sim_done) break;
        if (eg.lives_lost != lost0) break;
    }
    int s = sim_score_mode();
    if (eko_sim_done) s += 1000000;
    s -= (eg.lives_lost - lost0) * 2000000;
    s -= (eg.hits_taken - hits0) * (bot_bold && eko_hp() > 1 ? 1200 : eko_bot_cherry ? 9000 : 6000);
    s += (eko_hp() - hp0) * 1500;
    /* nearly out of health: keep well clear */
    if (eko_hp() <= 1 && eko_hp() < hp0 && eg.owl[eg.cur_owl].lives <= 1 && eko_bot_cherry) s -= 200000;
    memcpy(&eg, &bak, sizeof eg);
    eko_simulating = false;
    eko_sim_done = false;
    return s;
}

static int run_policy(Policy pol, int param, int frames) { return run_policy_from(pol, param, frames, 0); }

/* the chosen policy, played for real for a while */
static Policy cur_pol;
static int cur_param, cur_f, cur_left;

int eko_bot_debug;
static int choose(Policy *pols, const int *params, int n, int horizon, int keep) {
    int best = 0, bs = -2147483647;
    /* carrying on with the plan in hand is a candidate too (a plan that
     * walks first and jumps later would never get to its jump otherwise) */
    int cont = -2147483647;
    for (int i = 0; i < n && cur_pol; i++)
        if (pols[i] == cur_pol && params[i] == cur_param && cur_f > 0) { cont = run_policy_from(cur_pol, cur_param, horizon, cur_f) + 300; break; }
    for (int i = 0; i < n; i++) {
        int s = run_policy(pols[i], params[i], horizon);
        if (eko_bot_debug) printf("    plan %d (%d): %d\n", i, params[i], s);
        if (s > bs) { bs = s; best = i; }
    }
    if (eko_bot_debug) printf("    cont %d best %d (%d) cur_f %d\n", cont, bs, best, cur_f);
    if (cont >= bs) { cur_left = keep; return -1; }
    cur_pol = pols[best];
    cur_param = params[best];
    cur_f = 0;
    cur_left = keep;
    return best;
}

static int play_current(void) {
    int b = frame_buttons(cur_pol(cur_param, cur_f), cur_f);
    cur_f++;
    cur_left--;
    return b;
}

/* ==== FLINT ================================================================================= */

static int brawl_target_x(void) {
    const EkoBrawl *b = &eg.br;
    /* a key lying about: go and get it */
    for (int i = 0; i < BR_MAX_OBJS; i++)
        if (b->obj[i].alive && b->obj[i].kind == BO_KEY) return EKO_PX(b->obj[i].x);
    /* the locked wall needs the key: until then, the key carrier */
    if (!b->wall_open_t && !b->p.key) {
        int wall = -1;
        for (int x = 0; x < b->mw && wall < 0; x++)
            for (int y = 0; y < b->mh; y++) if (b->tile[y][x] == 'W') { wall = x * 16; break; }
        if (wall >= 0) {
            for (int i = 0; i < BR_MAX_FOES; i++)
                if (b->foe[i].alive && b->foe[i].kind == BF_KEYMAN) return EKO_PX(b->foe[i].x);
            return wall - 40;
        }
    }
    return b->mw * 16;
}

static int stuck_t, best_prog = -1000000, best_map = -1, best_sec = -1;

static int brawl_score(void) {
    const EkoBrawl *b = &eg.br;
    int s = 0;
    int w = stuck_t > 240 ? 120 : 40;
    /* the lab: up, from under the middle where the ledges overlap */
    if (b->boss_on) s += 60000; /* the arena reached: the fight terms below take over */
    else if (b->map == BM_LAB) s -= EKO_PX(b->p.y) * w + iabs(EKO_PX(b->p.x) - 152) * 12;
    else if (b->lift_on) s -= iabs(EKO_PX(b->p.x) - 160) * 4;
    else {
        int tx = brawl_target_x();
        s -= iabs(tx - EKO_PX(b->p.x)) * w;
    }
    s += b->kills * 400 + eg.score;
    s += b->dmg_dealt * (b->lift_on ? 2500 : 500);
    /* in the lift and the boss arenas: close in on the nearest foe */
    if (b->lift_on || b->boss_on) {
        int nd = 1000;
        for (int i = 0; i < BR_MAX_FOES; i++)
            if (b->foe[i].alive && b->foe[i].state != 99) nd = imin(nd, iabs(EKO_PX(b->foe[i].x) - EKO_PX(b->p.x)) + iabs(EKO_PX(b->foe[i].y) - EKO_PX(b->p.y)) / 2);
        if (nd < 1000) s -= imax(0, nd - 20) * 30;
    }
    if (b->boss_on && b->boss_i >= 0 && b->foe[b->boss_i].alive) s -= b->foe[b->boss_i].hp * 600;
    if (b->boss_dead) s += 50000;
    /* the key and the blown wall are worth any walk they lead to */
    if (b->p.key) s += 400000;
    if (b->wall_open_t) s += 800000;
    /* falling is the worst thing */
    if (b->p.dead_t) s -= 3000000;
    return s;
}

static int pol_brawl(int p, int f) {
    /* p: what to do; every plan holds it, a few plans change after 16 frames */
    int a = p & 15, then = (p >> 4) & 15, at = p >> 8;
    if (!at) at = 16;
    if (f >= at && then) { a = then; f -= at; }
    switch (a) {
    case 0: return BTN_RIGHT;
    case 1: return BTN_RIGHT | TAPA;
    case 2: return TAPA;
    case 3: return f < 2 ? (BTN_RIGHT | EKO_ALT) : BTN_RIGHT;
    case 4: return f < 2 ? (BTN_RIGHT | EKO_ALT) : (BTN_RIGHT | TAPA);
    case 5: return BTN_LEFT;
    case 6: return BTN_LEFT | TAPA;
    case 7: return BTN_DOWN | TAPA;
    case 8: return 0;
    case 9: return f < BR_CHARGE_T + 2 ? EKO_MAIN : 0;                 /* charge and let go */
    case 10: return f < 2 ? EKO_ALT : TAPA;                            /* hop on the spot, kick */
    case 11: return f < 2 ? (BTN_LEFT | EKO_ALT) : BTN_LEFT;
    case 12: return f < 3 ? (BTN_DOWN | EKO_MAIN) : f < 10 ? 0 : (f < 12 ? EKO_MAIN : BTN_RIGHT); /* lift a barrel, throw it */
    case 13: return f < 2 ? EKO_ALT : 0;
    case 14: return f < 2 ? (BTN_LEFT | EKO_ALT) : (BTN_LEFT | TAPA);   /* jump left, kicking */
    case 15: return f < 2 ? EKO_ALT : (f > 10 ? TAPA : 0);              /* jump up, kick at the top */
    default: return 0;
    }
}

static int bot_brawl(void) {
    /* no headway for a while: press on harder */
    {
        const EkoBrawl *b = &eg.br;
        int prog = b->map == BM_LAB ? -EKO_PX(b->p.y) : EKO_PX(b->p.x);
        if (best_map != b->map || best_sec != eg.sec) { best_map = b->map; best_sec = eg.sec; best_prog = prog; stuck_t = 0; }
        if (prog > best_prog + 8 || b->boss_on) { best_prog = prog; stuck_t = 0; }
        else stuck_t++;
        bot_bold = stuck_t > 600;
    }
    if (cur_left > 0 && cur_pol == pol_brawl) return play_current();
    static Policy P[48];
    static int A[48];
    int n = 0;
    static const int plans[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 2 | (3 << 4), 8 | (3 << 4), 5 | (3 << 4), 6 | (1 << 4), 13 | (4 << 4),
                                 0 | (3 << 4) | (4 << 8), 0 | (3 << 4) | (8 << 8), 0 | (3 << 4) | (12 << 8), 0 | (3 << 4) | (20 << 8),
                                 0 | (3 << 4) | (28 << 8), 0 | (4 << 4) | (10 << 8), 1 | (3 << 4) | (10 << 8),
                                 5 | (11 << 4) | (8 << 8), 5 | (11 << 4) | (20 << 8), 5 | (13 << 4) | (8 << 8), 5 | (13 << 4) | (20 << 8),
                                 0 | (13 << 4) | (8 << 8), 0 | (13 << 4) | (20 << 8), 5 | (13 << 4) | (32 << 8), 0 | (13 << 4) | (32 << 8),
                                 14, 15, 5 | (14 << 4) | (8 << 8), 5 | (14 << 4) | (20 << 8), 0 | (15 << 4) | (10 << 8), 5 | (15 << 4) | (10 << 8)};
    for (int i = 0; i < ARRAY_LEN(plans); i++) { P[n] = pol_brawl; A[n++] = plans[i]; }
    if (choose(P, A, n, eg.br.boss_on ? 80 : 56, 8) >= 0) {
        /* a plan that changes course later is played up to its change, at least */
        int then = (cur_param >> 4) & 15, at = cur_param >> 8;
        if (then) cur_left = imax(cur_left, (at ? at : 16) + 6);
    }
    return play_current();
}

/* ==== VESPER ================================================================================== */

static int gal_target(void) {
    const EkoGallery *g = &eg.ga;
    int best = -1, bs = -1000000;
    for (int i = 0; i < GA_MAX_FOES; i++) {
        const EkoGFoe *f = &g->foe[i];
        if (!f->alive || f->kind == GE_BARREL) continue;
        if (f->kind == GE_COIL && !f->a) continue;
        int s = 0;
        if (f->kind == GE_MISSILE) s += 3000 + EKO_PX(f->y) * 10;
        if (f->kind == GE_RIFLE || f->kind == GE_MORTAR) s += 1000 + (f->state == 1 ? 400 : 0) + f->t * 4;
        if (f->kind == GE_RUNNER || f->kind == GE_JEEP || f->kind == GE_CHOPPER) s += 900;
        if (f->kind >= GE_RAMROD) s += 600;
        s -= iabs(EKO_PX(f->x) - EKO_PX(g->px)) * 3;
        if (s > bs) { bs = s; best = i; }
    }
    /* a red barrel next to a crowd or a boss is worth a shot */
    for (int i = 0; i < GA_MAX_FOES; i++)
        if (g->foe[i].alive && g->foe[i].kind == GE_BARREL) {
            int near = 0;
            for (int k = 0; k < GA_MAX_FOES; k++)
                if (k != i && g->foe[k].alive && g->foe[k].kind != GE_BARREL && iabs(EKO_PX(g->foe[k].x) - EKO_PX(g->foe[i].x)) < 40 &&
                    iabs(EKO_PX(g->foe[k].y) - EKO_PX(g->foe[i].y)) < 40) near += g->foe[k].kind == GE_RAMROD ? 3 : 1;
            if (near >= 2) return i;
        }
    return best;
}

static void gal_aim(int fi, int *tx, int *ty) {
    const EkoGFoe *f = &eg.ga.foe[fi];
    int x = EKO_PX(f->x), y = EKO_PX(f->y);
    if (f->kind == GE_MISSILE || f->kind == GE_CHOPPER || f->kind == GE_VULTRA || f->kind == GE_POD || f->kind == GE_CANNON || f->kind == GE_COIL) { *tx = x; *ty = y; return; }
    int s = GA_ROW_SCALE[f->row];
    *tx = x;
    *ty = y - (f->kind == GE_JEEP ? 6 * s / 4 : f->kind == GE_BARREL ? 4 * s / 4 : f->kind == GE_RAMROD ? 16 : 14 * s / 4);
}

static int pol_gallery(int p, int f) {
    /* p: 0 aim on the move; 1 / 2 aim offset left / right (to dodge); 3 planted;
     * 4 / 5 roll left / right; 6 / 7 run left / right with the sight on */
    const EkoGallery *g = &eg.ga;
    int t = gal_target();
    int px = EKO_PX(g->px), cx = EKO_PX(g->cx), cy = EKO_PX(g->cy);
    int b = 0;
    if (p == 4 || p == 5) return f < 2 ? (EKO_ALT | (p == 4 ? BTN_LEFT : BTN_RIGHT)) : 0;
    if (t < 0) {
        if (p == 6) return BTN_LEFT;
        if (p == 7) return BTN_RIGHT;
        return px < 150 ? BTN_RIGHT : px > 170 ? BTN_LEFT : 0;
    }
    int tx, ty;
    gal_aim(t, &tx, &ty);
    if (p == 3) {
        /* planted: hold A, the pad steers the sight */
        b = EKO_MAIN;
        if (cx < tx - 2) b |= BTN_RIGHT;
        if (cx > tx + 2) b |= BTN_LEFT;
        if (cy < ty - 2) b |= BTN_DOWN;
        if (cy > ty + 2) b |= BTN_UP;
        return b;
    }
    int want = tx + (p == 1 ? -28 : p == 2 ? 28 : 0);
    if (p == 6) want = px - 40;
    if (p == 7) want = px + 40;
    if (px < want - 2) b |= BTN_RIGHT;
    if (px > want + 2) b |= BTN_LEFT;
    if (cy < ty - 2) b |= BTN_DOWN;
    if (cy > ty + 2) b |= BTN_UP;
    if (iabs(cx - tx) < 10 && iabs(cy - ty) < 10) b |= TAPA;
    return b;
}

static int gallery_score(void) {
    const EkoGallery *g = &eg.ga;
    int s = g->kills * 300 + eg.score;
    for (int i = 0; i < GA_MAX_FOES; i++)
        if (g->foe[i].alive && g->foe[i].kind >= GE_RAMROD && g->foe[i].kind != GE_MISSILE) s -= g->foe[i].hp * 200;
    /* stay away from where shots will land */
    for (int i = 0; i < GA_MAX_SHOTS; i++)
        if (g->shot[i].alive && g->shot[i].kind != 3 && iabs(g->shot[i].tx - EKO_PX(g->px)) < 14) s -= 4000;
    for (int i = 0; i < GA_MAX_FOES; i++)
        if (g->foe[i].alive && g->foe[i].kind == GE_MISSILE && iabs(EKO_PX(g->foe[i].x) - EKO_PX(g->px)) < 18) s -= 3000;
    for (int i = 0; i < GA_MAX_SHOTS; i++)
        if (g->shot[i].alive && g->shot[i].kind == 3) s -= iabs(g->shot[i].tx - EKO_PX(g->px)) * (eko_hp() < EKO_OWL_HP[OWL_VESPER] ? 20 : 0);
    if (g->boss_dead) s += 100000;
    return s;
}

static int bot_gallery(void) {
    if (cur_left > 0 && cur_pol == pol_gallery) return play_current();
    static Policy P[8];
    static int A[8];
    for (int i = 0; i < 8; i++) { P[i] = pol_gallery; A[i] = i; }
    choose(P, A, 8, 50, 6);
    return play_current();
}

/* ==== DART ===================================================================================== */

static int pol_bike(int p, int f) {
    /* p: lane offset in 12s (-4..4) + 9 * (jump now) ; the gun taps all the while */
    const EkoBike *k = &eg.bk;
    int lane = (p % 9) - 4, jump = p / 9;
    int ty = EKO_PX(k->y) + lane * 12;
    /* line up with the nearest thing worth shooting in that lane band */
    int b = TAPA;
    int y = EKO_PX(k->y);
    if (y < ty - 1) b |= BTN_DOWN;
    if (y > ty + 1) b |= BTN_UP;
    int x = EKO_PX(k->x);
    int wantx = k->boss_on ? 70 : 80;
    if (x < wantx - 2) b |= BTN_RIGHT;
    if (x > wantx + 2) b |= BTN_LEFT;
    if (jump && f < 2) b |= EKO_ALT;
    return b;
}

static int bike_score(void) {
    const EkoBike *k = &eg.bk;
    int s = k->kills * 300 + eg.score;
    for (int i = 0; i < BK_MAX_OBJS; i++)
        if (k->obj[i].alive && k->obj[i].kind == OB_HOG) s -= k->obj[i].hp * 400;
    if (k->boss_dead) s += 100000;
    if (k->card) s += 100000;
    s -= k->heat * 10;
    /* the middle of the road leaves room both ways */
    s -= iabs(EKO_PX(k->y) - 130) * 4;
    /* line up with something to shoot */
    int best = 1000;
    for (int i = 0; i < BK_MAX_OBJS; i++) {
        const EkoKObj *o = &k->obj[i];
        if (!o->alive || o->hp <= 0 || o->kind == OB_CONE || o->kind == OB_MINE) continue;
        int sx = EKO_PX(o->x) - k->dist;
        if (sx < EKO_PX(k->x) || sx > 320) continue;
        best = imin(best, iabs(EKO_PX(o->y) - EKO_PX(k->y)));
        if (o->kind == OB_CARD || o->kind == OB_MANGO) best = imin(best, iabs(EKO_PX(o->y) - EKO_PX(k->y)));
    }
    if (best < 1000) s -= best * 15;
    return s;
}

static int bot_bike(void) {
    if (cur_left > 0 && cur_pol == pol_bike) return play_current();
    static Policy P[18];
    static int A[18];
    int n = 0;
    for (int j = 0; j < 2; j++)
        for (int l = 0; l < 9; l++) { P[n] = pol_bike; A[n++] = l + 9 * j; }
    choose(P, A, n, 60, 6);
    return play_current();
}

/* ==== SABLE ======================================================================================= */

static int16_t sdist[SN_MH][SN_MW];
static int sd_tx = -1, sd_ty = -1, sd_cards = -1, sd_map = -1;

static void sneak_field(int tx, int ty) {
    const EkoSneak *s = &eg.sn;
    if (tx == sd_tx && ty == sd_ty && s->cards == sd_cards && s->map == sd_map) return;
    sd_tx = tx; sd_ty = ty; sd_cards = s->cards; sd_map = s->map;
    for (int y = 0; y < SN_MH; y++) for (int x = 0; x < SN_MW; x++) sdist[y][x] = 30000;
    static int16_t qx[SN_MW * SN_MH], qy[SN_MW * SN_MH];
    int h = 0, t = 0;
    sdist[ty][tx] = 0;
    qx[t] = (int16_t)tx; qy[t] = (int16_t)ty; t++;
    while (h < t) {
        int x = qx[h], y = qy[h];
        h++;
        static const int DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (nx < 0 || ny < 0 || nx >= s->mw || ny >= s->mh || sdist[ny][nx] != 30000) continue;
            if (eko_sneak_wall(nx, ny)) continue;
            sdist[ny][nx] = (int16_t)(sdist[y][x] + 1);
            qx[t] = (int16_t)nx; qy[t] = (int16_t)ny; t++;
        }
    }
}

/* where Sable is headed now (a tile of the whole map) */
static void sneak_target(int *tx, int *ty) {
    const EkoSneak *s = &eg.sn;
    int px = EKO_PX(s->x) >> 4, py = (EKO_PX(s->y) - SN_TOP) >> 4;
    int rx0 = s->room_x * SN_RW, ry0 = s->room_y * SN_RH;
    /* anything lying in this room that can be reached */
    int best = -1, bd = 1 << 30;
    sneak_field(px, py);
    for (int i = 0; i < s->nitem; i++) {
        if (s->item[i].taken || sdist[s->item[i].ty][s->item[i].tx] >= 30000) continue;
        int ix = s->item[i].tx, iy = s->item[i].ty;
        if (ix < rx0 || iy < ry0 || ix >= rx0 + SN_RW || iy >= ry0 + SN_RH) continue;
        int d = iabs(ix - px) + iabs(iy - py);
        if (d < bd) { bd = d; best = i; }
    }
    if (best >= 0) { *tx = s->item[best].tx; *ty = s->item[best].ty; return; }
    /* the green card first */
    for (int i = 0; i < s->nitem; i++)
        if (!s->item[i].taken && s->item[i].kind == 6) { *tx = s->item[i].tx; *ty = s->item[i].ty; return; }
    if (s->map == SM_ISTANBUL) {
        /* before Cipher: everything worth having that can be reached */
        sneak_field(px, py);
        int bi = -1;
        bd = 1 << 30;
        for (int i = 0; i < s->nitem; i++) {
            if (s->item[i].taken || s->item[i].kind == 0) continue;
            if (s->item[i].kind == 4 && eko_hp() >= EKO_OWL_HP[OWL_SABLE]) continue;
            int d = sdist[s->item[i].ty][s->item[i].tx];
            if (d < bd) { bd = d; bi = i; }
        }
        if (bi >= 0 && bd < 30000) { *tx = s->item[bi].tx; *ty = s->item[bi].ty; return; }
    }
    if (s->map == SM_ISTANBUL && s->boss_g >= 0) {
        const EkoSGuard *c = &s->g[s->boss_g];
        *tx = (c->hx >> 8) >> 4;
        *ty = (((c->hy >> 8) - SN_TOP) >> 4) - 2;
        return;
    }
    /* the next handover point that can be reached */
    int bx = -1, by = -1;
    bd = 1 << 30;
    sneak_field(px, py);
    for (int y = 0; y < s->mh; y++)
        for (int x = 0; x < s->mw; x++)
            if (s->tile[y][x] == 'X' && sdist[y][x] < bd) { bd = sdist[y][x]; bx = x; by = y; }
    *tx = bx < 0 ? px : bx;
    *ty = by < 0 ? py : by;
}

static int s_goal_x, s_goal_y;

/* the direction (bits) that walks Sable down the field */
static int sneak_step_dir(void) {
    const EkoSneak *s = &eg.sn;
    int x = EKO_PX(s->x), y = EKO_PX(s->y) - SN_TOP;
    int tx = x >> 4, ty = y >> 4;
    int cx = tx * 16 + 8, cy = ty * 16 + 8;
    static const int DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};
    static const int BTN[4] = {BTN_RIGHT, BTN_DOWN, BTN_LEFT, BTN_UP};
    int best = -1, bd = sdist[ty][tx];
    for (int d = 0; d < 4; d++) {
        int nx = tx + DX[d], ny = ty + DY[d];
        if (nx < 0 || ny < 0 || nx >= SN_MW || ny >= SN_MH) continue;
        if (sdist[ny][nx] < bd) { bd = sdist[ny][nx]; best = d; }
    }
    if (best < 0) {
        /* on the goal tile: centre on it */
        if (x < cx - 1) return BTN_RIGHT;
        if (x > cx + 1) return BTN_LEFT;
        if (y < cy - 1) return BTN_DOWN;
        if (y > cy + 1) return BTN_UP;
        return 0;
    }
    /* line up on the lane first */
    if (DX[best] && iabs(y - cy) > 2) return y < cy ? BTN_DOWN : BTN_UP;
    if (DY[best] && iabs(x - cx) > 2) return x < cx ? BTN_RIGHT : BTN_LEFT;
    return BTN[best];
}

/* a guard (or Cipher) lined up with her within reach: the way to face it */
static int lined_up(int *gi_out) {
    const EkoSneak *s = &eg.sn;
    int room = s->room_y * SN_MAX_RX + s->room_x;
    int sx = EKO_PX(s->x), sy = EKO_PX(s->y);
    for (int i = 0; i < s->ng; i++) {
        const EkoSGuard *g = &s->g[i];
        if (!g->alive || g->room != room) continue;
        if (g->kind == GK_CAMERA || g->kind == GK_LIGHT) continue;
        if (g->state == GD_SLEEP || g->state == GD_STUN) continue;
        if (g->kind == GK_CIPHER && g->state == GD_INVESTIGATE) continue;
        int gx = EKO_PX(g->x), gy = EKO_PX(g->y);
        int dx = gx - sx, dy = gy - sy;
        if (iabs(dy) <= 4 && iabs(dx) < 150 && eko_sneak_los(sx, sy, gx, gy)) { *gi_out = i; return dx > 0 ? 0 : 2; }
        if (iabs(dx) <= 4 && iabs(dy) < 150 && eko_sneak_los(sx, sy, gx, gy)) { *gi_out = i; return dy > 0 ? 1 : 3; }
    }
    return -1;
}

static int pol_sneak(int p, int f) {
    /* 0 walk on; 1 wait; 2 back off; 3 shoot what is lined up (darts or what she holds);
     * 4 walk on, shooting anything lined up; 5 pistol at what is lined up; 6 grenade */
    const EkoSneak *s = &eg.sn;
    static const int BTN[4] = {BTN_RIGHT, BTN_DOWN, BTN_LEFT, BTN_UP};
    int gi = -1;
    int d = lined_up(&gi);
    switch (p) {
    case 0: return sneak_step_dir();
    case 1: return 0;
    case 2: {
        int b = sneak_step_dir();
        if (b & BTN_RIGHT) return BTN_LEFT;
        if (b & BTN_LEFT) return BTN_RIGHT;
        if (b & BTN_UP) return BTN_DOWN;
        if (b & BTN_DOWN) return BTN_UP;
        return 0;
    }
    case 8: return BTN_RIGHT;
    case 9: return BTN_DOWN;
    case 10: return BTN_LEFT;
    case 11: return BTN_UP;
    case 7: {
        /* hunt: line up with the nearest guard on its feet, then shoot it */
        if (d >= 0) { p = 3; break; }
        int room = s->room_y * SN_MAX_RX + s->room_x, best = -1, bd = 1 << 30;
        for (int i = 0; i < s->ng; i++) {
            const EkoSGuard *g = &s->g[i];
            if (!g->alive || g->room != room || (g->kind != GK_WALKER && g->kind != GK_TURNER)) continue;
            if (g->state == GD_SLEEP || g->state == GD_STUN) continue;
            int dd = iabs(EKO_PX(g->x) - EKO_PX(s->x)) + iabs(EKO_PX(g->y) - EKO_PX(s->y));
            if (dd < bd) { bd = dd; best = i; }
        }
        if (best < 0) return 0;
        int dx = EKO_PX(s->g[best].x) - EKO_PX(s->x), dy = EKO_PX(s->g[best].y) - EKO_PX(s->y);
        if (iabs(dx) < iabs(dy)) return dx > 0 ? BTN_RIGHT : BTN_LEFT;
        return dy > 0 ? BTN_DOWN : BTN_UP;
    }
    default: break;
    }
    switch (p) {
    case 3: case 4: case 5: case 6: {
        if (d < 0) return p == 4 ? sneak_step_dir() : 0;
        int want = p == 5 ? WP_PISTOL : p == 6 ? WP_GRENADE : (s->g[gi].b || s->g[gi].kind == GK_CIPHER) ? WP_PISTOL : WP_DART;
        if (s->ammo[want] <= 0) { want = WP_PISTOL; if (s->ammo[want] <= 0) return p == 4 ? sneak_step_dir() : 0; }
        if (s->weapon != want) return (f & 3) < 2 ? EKO_ALT : 0;
        if (s->dir != d) return BTN[d];
        return (f & 7) < 2 ? EKO_MAIN : 0;
    }
    default: return 0;
    }
}

static int sn_stuck, sn_best = 1 << 30, sn_gx = -1, sn_gy = -1;

static int sneak_score(void) {
    const EkoSneak *s = &eg.sn;
    int x = EKO_PX(s->x) >> 4, y = (EKO_PX(s->y) - SN_TOP) >> 4;
    int d = sdist[iclamp(y, 0, SN_MH - 1)][iclamp(x, 0, SN_MW - 1)];
    int sc = -d * (sn_stuck > 600 ? 1500 : 200);
    /* in Cipher's room: keep a fair distance and work him down */
    if (s->boss_g >= 0 && s->g[s->boss_g].alive && s->g[s->boss_g].room == s->room_y * SN_MAX_RX + s->room_x) {
        const EkoSGuard *c = &s->g[s->boss_g];
        int cd = eko_dist(EKO_PX(c->x) - EKO_PX(s->x), EKO_PX(c->y) - EKO_PX(s->y));
        sc = 20000 - iabs(cd - 70) * 30 + (26 - c->hp) * 6000;
    }
    bool bold = sn_stuck > 300; /* waited long enough: get on with it */
    sc -= s->spotted_total * (bold ? 0 : 50000);
    sc -= s->noise_total * 2000;
    if (s->boss_g >= 0 && s->g[s->boss_g].alive) sc -= s->g[s->boss_g].hp * 3000;
    if (s->boss_dead) sc += 200000;
    sc += s->used_relays * 200000;
    for (int i = 0; i < s->nitem; i++) sc += s->item[i].taken * 3000;
    sc += eg.score;
    /* alert guards close by are trouble */
    int room = s->room_y * SN_MAX_RX + s->room_x;
    for (int i = 0; i < s->ng; i++) {
        const EkoSGuard *g = &s->g[i];
        if (g->alive && g->room == room && g->state == GD_ALERT && !bold) sc -= 8000;
    }
    for (int i = 0; i < SN_MAX_SHOTS; i++)
        if (!bold && s->shot[i].alive && s->shot[i].foe && eko_dist(EKO_PX(s->shot[i].x) - EKO_PX(s->x), EKO_PX(s->shot[i].y) - EKO_PX(s->y)) < 20) sc -= 5000;
    return sc;
}

static int bot_sneak(void) {
    sneak_target(&s_goal_x, &s_goal_y);
    {
        const EkoSneak *s = &eg.sn;
        (void)s;
        sneak_field(s_goal_x, s_goal_y);
    }
    {
        const EkoSneak *s = &eg.sn;
        int x = EKO_PX(s->x) >> 4, y = (EKO_PX(s->y) - SN_TOP) >> 4;
        int d = sdist[iclamp(y, 0, SN_MH - 1)][iclamp(x, 0, SN_MW - 1)];
        if (s_goal_x != sn_gx || s_goal_y != sn_gy) { sn_gx = s_goal_x; sn_gy = s_goal_y; sn_best = d; sn_stuck = 0; }
        if (d < sn_best) { sn_best = d; sn_stuck = 0; }
        else sn_stuck++;
        if (s->boss_g >= 0 && s->g[s->boss_g].alive && s->g[s->boss_g].room == s->room_y * SN_MAX_RX + s->room_x) sn_stuck = 0;
        bot_bold = sn_stuck > 600;
    }
    if (cur_left > 0 && cur_pol == pol_sneak) return play_current();
    static Policy P[12];
    static int A[12];
    for (int i = 0; i < 12; i++) { P[i] = pol_sneak; A[i] = i; }
    const EkoSneak *sn = &eg.sn;
    bool boss = sn->boss_g >= 0 && sn->g[sn->boss_g].alive && sn->g[sn->boss_g].room == sn->room_y * SN_MAX_RX + sn->room_x;
    choose(P, A, 12, boss ? 90 : 70, 6);
    return play_current();
}

/* ==== the menus and the run ======================================================================= */

static int sim_score_mode(void) {
    switch (eg.mode) {
    case MODE_BRAWL: return brawl_score();
    case MODE_GALLERY: return gallery_score();
    case MODE_BIKE: return bike_score();
    case MODE_SNEAK: return sneak_score();
    default: return 0;
    }
}

static int tap(int b) { return (eg.frame_t & 3) < 2 ? b : 0; }

static int rescuer_for(void) {
    /* the best jailbreaker first: Dart, then Flint, Vesper, Sable */
    static const int ORDER[OWL_COUNT] = {OWL_DART, OWL_FLINT, OWL_VESPER, OWL_SABLE};
    for (int i = 0; i < OWL_COUNT; i++) if (!eg.owl[ORDER[i]].captured) return ORDER[i];
    return 0;
}

static int bot_rescue(void) {
    if ((eg.frame_t & 3) >= 2) return 0;
    int a = eko_rescue_bot_action(&eg.rs);
    switch (a) {
    case RA_RIGHT: return BTN_RIGHT;
    case RA_DOWN: return BTN_DOWN;
    case RA_LEFT: return BTN_LEFT;
    case RA_UP: return BTN_UP;
    case RA_A: return EKO_MAIN;
    case RA_B: return EKO_ALT;
    default: return 0;
    }
}

int eko_bot_buttons(void) {
    switch (eg.state) {
    case ES_TITLE: case ES_STORY: case ES_BRIEF: case ES_CARD: case ES_CLEAR: case ES_CAPTURED:
    case ES_RESCUE_WON: case ES_RESCUE_LOST: case ES_OVER: case ES_ENDING:
        cur_left = 0;
        return tap(BTN_A);
    case ES_HQ: {
        int want = -1;
        for (int i = 0; i < OWL_COUNT && want < 0; i++) if (eg.owl[i].captured) want = i;
        for (int i = 0; i < OWL_COUNT && want < 0; i++) if (!eg.owl[i].cleared) want = i;
        if (want < 0) want = 4;
        if (eg.hq_sel != want) return tap(BTN_DOWN);
        return eg.state_t > 12 ? tap(BTN_A) : 0;
    }
    case ES_PICK_RESCUER: {
        int want = rescuer_for();
        if (eg.pick_sel != want) return tap(BTN_DOWN);
        return eg.state_t > 12 ? tap(BTN_A) : 0;
    }
    case ES_PLAY:
        switch (eg.mode) {
        case MODE_BRAWL: return bot_brawl();
        case MODE_GALLERY: return bot_gallery();
        case MODE_BIKE: return bot_bike();
        case MODE_SNEAK: return bot_sneak();
        case MODE_RESCUE: return bot_rescue();
        default: return 0;
        }
    default: return 0;
    }
}
