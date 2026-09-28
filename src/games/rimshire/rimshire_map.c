/* RIMSHIRE - the board. The two banners take turns moving along the roads:
 * one space on the very first turn of a war, two after that (three with
 * SCOUTING); every d-pad press moves the banner a space at once, and every
 * move must be used. A banner leaves a trail and can't step back onto it;
 * holding B takes it home at any point and ends the turn, which wipes the
 * trail. Stepping onto the other banner, its home or its trail starts a
 * battle there (a banner caught on its trail is pulled to that point) and
 * the side that stepped acts first. Inns hire out the same three disks all
 * war and close after three visits; tomes teach a skill; seams pay every
 * turn for ten turns; chests pay once. After a battle both trails are wiped,
 * the winner stays, the loser goes home, calls up its reserves and moves
 * next. Beaten at its own home it makes a second stand with its reserves,
 * unless its home is a castle, which falls with it. A war is won when a
 * battle would start and the other side has no disks at all. */
#include "rimshire.h"

RshWar rw;
const int RSH_DX[4] = {1, 0, -1, 0}, RSH_DY[4] = {0, 1, 0, -1};

static int8_t plan_r[4]; /* what each planned step is: 1 a step, 2 a battle, 3 home */

bool rsh_node_ok(int x, int y) {
    return x >= 0 && y >= 0 && x < rw.w && y < rw.h && rw.node[y][x] != N_NONE;
}

bool rsh_road(int x0, int y0, int x1, int y1) {
    if (!rsh_node_ok(x0, y0) || !rsh_node_ok(x1, y1)) return false;
    if (y0 == y1 && iabs(x1 - x0) == 1) return rw.er[y0][imin(x0, x1)] != 0;
    if (x0 == x1 && iabs(y1 - y0) == 1) return rw.ed[imin(y0, y1)][x0] != 0;
    return false;
}

bool rsh_in_trail(int side, int x, int y) {
    const RshSide *s = &rw.s[side];
    for (int i = 0; i < s->trail_n; i++)
        if (s->tx[i] == x && s->ty[i] == y) return true;
    return false;
}

static bool in_plan(int x, int y, int upto) {
    for (int i = 0; i < upto; i++)
        if (rw.plan_x[i] == x && rw.plan_y[i] == y) return true;
    return false;
}

/* One step from (fx, fy): 0 not allowed, 1 a step, 2 it starts a battle,
 * 3 it arrives home. Steps planned ahead (the computer's) count as trail. */
static int step_from(int side, int fx, int fy, int dir, int *nx, int *ny, int planned) {
    int x = fx + RSH_DX[dir], y = fy + RSH_DY[dir];
    *nx = x;
    *ny = y;
    if (!rsh_road(fx, fy, x, y)) return 0;
    const RshSide *s = &rw.s[side], *o = &rw.s[side ^ 1];
    if ((x == o->nx && y == o->ny) || (x == o->bx && y == o->by) || rsh_in_trail(side ^ 1, x, y)) return 2;
    if (rsh_in_trail(side, x, y) || in_plan(x, y, planned)) return 0;
    if (x == s->bx && y == s->by) return 3;
    return 1;
}

int rsh_step_ok(int side, int fx, int fy, int dir, int *nx, int *ny) { return step_from(side, fx, fy, dir, nx, ny, 0); }

int rsh_step_kind(int dir) {
    int nx, ny;
    return step_from(rw.turn, rw.s[rw.turn].nx, rw.s[rw.turn].ny, dir, &nx, &ny, 0);
}

bool rsh_can_step(void) {
    for (int d = 0; d < 4; d++)
        if (rsh_step_kind(d)) return true;
    return false;
}

int rsh_army_size(int side) { return rw.s[side].n_army + rw.s[side].n_reserve; }

int rsh_strength(int side) {
    const RshSide *s = &rw.s[side];
    int v = 0;
    for (int i = 0; i < s->n_army; i++) v += rsh_kind_value(s->army[i]);
    for (int i = 0; i < s->n_reserve; i++) v += rsh_kind_value(s->reserve[i]) / 2;
    return v;
}

/* ------------------------------------------------------------------ */
/* turns                                                                 */

static void promote(RshSide *s) {
    while (s->n_reserve > 0 && s->n_army < RSH_ARMY) {
        s->army[s->n_army++] = s->reserve[0];
        memmove(s->reserve, s->reserve + 1, (size_t)(s->n_reserve - 1));
        s->n_reserve--;
    }
}

static void send_home(int side) {
    RshSide *s = &rw.s[side];
    s->nx = s->bx;
    s->ny = s->by;
    s->trail_n = 1;
    s->tx[0] = (int8_t)s->bx;
    s->ty[0] = (int8_t)s->by;
}

/* a trail wiped where the banner stands */
static void wipe_trail(int side) {
    RshSide *s = &rw.s[side];
    s->trail_n = 1;
    s->tx[0] = (int8_t)s->nx;
    s->ty[0] = (int8_t)s->ny;
}

static void say(const char *m) {
    snprintf(rw.msg, sizeof rw.msg, "%s", m);
    rw.msg_t = 100;
}

static void begin_turn(void) {
    RshSide *s = &rw.s[rw.turn];
    rw.moves = rw.first_move ? 1 : (s->skills & SK_SCOUTING) ? 3 : 2;
    rw.moves_used = 0;
    /* the seams pay at the start of their owner's turn, ten times each */
    for (int y = 0; y < rw.h; y++)
        for (int x = 0; x < rw.w; x++)
            if (rw.node[y][x] == N_SEAM && rw.owner[y][x] == rw.turn + 1) {
                s->coins += (s->skills & SK_PROSPECT) ? 2 : 1;
                if (rw.seam_left[y][x] > 0 && --rw.seam_left[y][x] == 0) {
                    rw.node[y][x] = N_PLAIN;
                    rw.owner[y][x] = 0;
                }
            }
    rw.plan_n = 0;
    rw.walk_i = 0;
    rw.state = s->cpu >= 0 ? W_CPU : W_PLAN;
    rw.state_t = 0;
    /* a banner at home with nowhere at all to go can only pass */
    if (s->cpu < 0 && !rsh_can_step() && s->nx == s->bx && s->ny == s->by) rw.state = W_WALK;
}

void rsh_end_turn(void) {
    if (rw.state == W_OVER) return;
    rw.first_move = 0;
    rw.turns++;
    rw.turn ^= 1;
    begin_turn();
}

void rsh_go_home(int side) {
    RshSide *s = &rw.s[side];
    if (s->nx == s->bx && s->ny == s->by) return;
    send_home(side);
    rsh_end_turn();
}

static void war_over(int winner) {
    rw.state = W_OVER;
    rw.state_t = 0;
    rw.winner = winner;
}

static void start_battle(int att, int x, int y) {
    int def = att ^ 1;
    /* caught on its trail, an army is pulled to that point */
    RshSide *d = &rw.s[def];
    if (!(x == d->nx && y == d->ny) && !(x == d->bx && y == d->by))
        for (int i = 0; i < d->trail_n; i++)
            if (d->tx[i] == x && d->ty[i] == y) {
                d->trail_n = i + 1;
                d->nx = x;
                d->ny = y;
                break;
            }
    /* the side that turns up with no disks at all loses the war */
    if (rsh_army_size(def) == 0) { war_over(att); return; }
    if (rsh_army_size(att) == 0) { war_over(def); return; }
    if (rw.s[att].n_army == 0) promote(&rw.s[att]);
    if (rw.s[def].n_army == 0) promote(&rw.s[def]);
    rw.bx = x;
    rw.by = y;
    rw.attacker = att;
    rw.state = W_BATTLE;
    rw.state_t = 0;
    rsh_battle_start(att, rng_next(&rw.rng));
}

/* ------------------------------------------------------------------ */
/* inns, tomes, seams, chests                                            */

static void roll_offers(uint8_t out[3]) {
    int kinds[K_COUNT], n = 0;
    for (int k = 0; k < K_COUNT; k++)
        if ((rw.pool >> k) & 1 && RSH_KIND[k].cost > 0) kinds[n++] = k;
    for (int i = 0; i < 3; i++) {
        out[i] = n ? (uint8_t)kinds[rng_range(&rw.rng, 0, n - 1)] : K_COUNT;
        for (int j = 0; j < i && n >= 3; j++)
            if (out[j] == out[i]) { i--; break; } /* three different disks */
    }
}

static void roll_tome(int side) {
    int left[RSH_SKILLS], n = 0;
    for (int k = 0; k < RSH_SKILLS; k++)
        if (!(rw.s[side].skills >> k & 1)) left[n++] = k;
    for (int i = 0; i < 3; i++) {
        if (n == 0) { rw.tome[i] = 0xFF; continue; }
        int j = rng_range(&rw.rng, 0, n - 1);
        rw.tome[i] = (uint8_t)left[j];
        left[j] = left[--n];
    }
}

static void after_step(void);

/* buy disk i of the inn: into the field army, or, with that full, a field
 * disk of your choice goes to the reserve to make room */
void rsh_inn_buy(int i) {
    RshSide *s = &rw.s[rw.turn];
    if (i < 0 || i > 2 || rw.offer[i] >= K_COUNT) return;
    int k = rw.offer[i];
    if (s->coins < RSH_KIND[k].cost) return;
    if (s->n_army < RSH_ARMY) {
        s->coins -= RSH_KIND[k].cost;
        s->army[s->n_army++] = (uint8_t)k;
        return;
    }
    if (s->n_reserve >= RSH_RESERVE) return;
    rw.swap_kind = k;
    rw.menu_sel = 0;
    rw.state = W_SWAP;
}

void rsh_swap_pick(int i) {
    RshSide *s = &rw.s[rw.turn];
    int k = rw.swap_kind;
    rw.state = W_INN;
    rw.menu_sel = 0;
    if (i < 0 || i >= s->n_army || k < 0 || k >= K_COUNT) return;
    s->coins -= RSH_KIND[k].cost;
    s->reserve[s->n_reserve++] = s->army[i];
    memmove(s->army + i, s->army + i + 1, (size_t)(s->n_army - i - 1));
    s->army[s->n_army - 1] = (uint8_t)k;
}

void rsh_inn_leave(void) {
    RshSide *s = &rw.s[rw.turn];
    if (rw.visits[s->ny][s->nx] >= 3) rw.node[s->ny][s->nx] = N_PLAIN;
    after_step();
}

void rsh_tome_pick(int i) {
    RshSide *s = &rw.s[rw.turn];
    if (i >= 0 && i < 3 && rw.tome[i] < RSH_SKILLS) s->skills |= 1 << rw.tome[i];
    rw.node[s->ny][s->nx] = N_PLAIN;
    after_step();
}

/* the computer at an inn: the best disks it can pay for; with a full field
 * army it benches its weakest disk for a better one */
static void cpu_shop(void) {
    RshSide *s = &rw.s[rw.turn];
    for (int pass = 0; pass < 8; pass++) {
        int best = -1, bv = 0;
        for (int i = 0; i < 3; i++) {
            int k = rw.offer[i];
            if (k >= K_COUNT || RSH_KIND[k].cost > s->coins) continue;
            int v = rsh_kind_value(k);
            if (v > bv) { bv = v; best = i; }
        }
        if (best < 0) break;
        if (s->n_army < RSH_ARMY) { rsh_inn_buy(best); continue; }
        if (s->n_reserve >= RSH_RESERVE) break;
        int weak = 0;
        for (int i = 1; i < s->n_army; i++)
            if (rsh_kind_value(s->army[i]) < rsh_kind_value(s->army[weak])) weak = i;
        if (rsh_kind_value(s->army[weak]) >= bv) break;
        rsh_inn_buy(best);
        rsh_swap_pick(weak);
    }
}

static void cpu_tome(void) {
    static const int PREF[RSH_SKILLS] = {2, 0, 7, 1, 4, 3, 6, 5}; /* scouting, command, remedy, ... */
    int pick = 0;
    for (int p = 0; p < RSH_SKILLS; p++) {
        bool found = false;
        for (int i = 0; i < 3; i++)
            if (rw.tome[i] == PREF[p]) { pick = i; found = true; }
        if (found) break;
    }
    rsh_tome_pick(pick);
}

/* arriving on a node: what is there */
static void visit(int side, int x, int y) {
    RshSide *s = &rw.s[side];
    switch (rw.node[y][x]) {
    case N_INN:
        if (rw.visits[y][x] < 3) {
            rw.visits[y][x]++;
            if (s->skills & SK_HAGGLING) { s->coins += 4; say("HAGGLING: +4 COINS"); }
            memcpy(rw.offer, rw.inn_offer[y][x], 3);
            rw.menu_sel = 0;
            rw.state = W_INN;
            rw.state_t = 0;
            if (s->cpu >= 0) { cpu_shop(); rsh_inn_leave(); }
        }
        break;
    case N_TOME:
        roll_tome(side);
        rw.menu_sel = 0;
        rw.state = W_TOME;
        rw.state_t = 0;
        if (s->cpu >= 0) cpu_tome();
        break;
    case N_SEAM:
        if (rw.owner[y][x] != side + 1) say((s->skills & SK_PROSPECT) ? "SEAM CLAIMED: 2 COINS A TURN" : "SEAM CLAIMED: 1 COIN A TURN");
        rw.owner[y][x] = (uint8_t)(side + 1);
        break;
    case N_CHEST: {
        int c = 4 + (int)(rng_next(&rw.rng) % 2);
        char m[24];
        snprintf(m, sizeof m, "A CHEST: +%d COINS", c);
        say(m);
        s->coins += c;
        rw.node[y][x] = N_PLAIN;
        break;
    }
    }
}

/* after a step (and whatever happened there): walk on, wait for the next
 * press, or end the turn once every move is used */
static void after_step(void) {
    RshSide *s = &rw.s[rw.turn];
    rw.state = W_WALK;
    rw.walk_t = 0;
    if (rw.walk_i < rw.plan_n) return;
    if (rw.moves_used >= rw.moves) { rsh_end_turn(); return; }
    rw.plan_n = rw.walk_i = 0;
    rw.state = s->cpu >= 0 ? W_CPU : W_PLAN;
    rw.state_t = s->cpu >= 0 ? 16 : 0;
}

/* one step of the walk */
static void next_step(void) {
    RshSide *s = &rw.s[rw.turn];
    if (rw.walk_i >= rw.plan_n) { after_step(); return; }
    int x = rw.plan_x[rw.walk_i], y = rw.plan_y[rw.walk_i], r = plan_r[rw.walk_i];
    rw.walk_i++;
    rw.moves_used++;
    if (r == 3) { send_home(rw.turn); rsh_end_turn(); return; }
    s->nx = x;
    s->ny = y;
    if (s->trail_n < RSH_TRAIL) {
        s->tx[s->trail_n] = (int8_t)x;
        s->ty[s->trail_n] = (int8_t)y;
        s->trail_n++;
    }
    sfx_play_name("rsh_step");
    if (r == 2) { start_battle(rw.turn, x, y); return; }
    rw.state = W_WALK;
    visit(rw.turn, x, y);
    if (rw.state == W_WALK) after_step();
}

int rsh_try_step(int dir) {
    if (rw.state != W_PLAN) return 0;
    int nx, ny, r = step_from(rw.turn, rw.s[rw.turn].nx, rw.s[rw.turn].ny, dir, &nx, &ny, 0);
    if (!r) return 0;
    rw.plan_x[0] = (int8_t)nx;
    rw.plan_y[0] = (int8_t)ny;
    plan_r[0] = (int8_t)r;
    rw.plan_n = 1;
    rw.walk_i = 0;
    rw.walk_t = 0;
    rw.state = W_WALK;
    return 1;
}

void rsh_war_update(void) {
    rw.state_t++;
    if (rw.msg_t > 0) rw.msg_t--;
    switch (rw.state) {
    case W_WALK:
        if (++rw.walk_t >= 10) {
            rw.walk_t = 0;
            if (rw.walk_i < rw.plan_n) next_step();
            else if (rw.plan_n == 0) rsh_end_turn(); /* a turn with nowhere to go */
        }
        break;
    case W_CPU:
        if (rw.state_t >= 24) {
            int8_t px[4], py[4];
            int n = rsh_cpu_map_plan(rw.turn, px, py);
            if (n <= 0) {
                if (rw.s[rw.turn].nx == rw.s[rw.turn].bx && rw.s[rw.turn].ny == rw.s[rw.turn].by) rsh_end_turn();
                else rsh_go_home(rw.turn);
                break;
            }
            rw.plan_n = n;
            rw.walk_i = 0;
            rw.walk_t = 0;
            rw.state = W_WALK;
        }
        break;
    default: break;
    }
}

/* ------------------------------------------------------------------ */
/* after a battle                                                        */

void rsh_battle_over(int result) {
    /* the survivors, in their queue order, and what the field paid */
    for (int s = 0; s < 2; s++) {
        RshSide *sd = &rw.s[s];
        uint8_t keep[RSH_ARMY];
        int n = 0;
        for (int q = 0; q < rb.qn[s]; q++) {
            const RshDisk *d = &rb.p.d[rb.queue[s][q]];
            if (d->on && !d->proj && n < RSH_ARMY) keep[n++] = d->kind;
        }
        memcpy(sd->army, keep, (size_t)n);
        sd->n_army = n;
        sd->coins += rb.p.coins[s];
    }
    int att = rw.attacker;
    rw.regroup = 0;
    if (result == B_STALE || result == B_BOTH) {
        /* both go home; a stalemate calls up no reserves; the side that
         * didn't start the battle moves next */
        send_home(0);
        send_home(1);
        if (result == B_BOTH) {
            rw.s[0].losses++;
            rw.s[1].losses++;
            promote(&rw.s[0]);
            promote(&rw.s[1]);
            if (rsh_army_size(0) == 0 && rsh_army_size(1) == 0) { war_over(2); return; }
        }
        rw.first_move = 0;
        rw.turns++;
        rw.turn = att ^ 1;
        begin_turn();
        return;
    }
    int winner = result == B_WIN1, lose = winner ^ 1;
    RshSide *L = &rw.s[lose];
    rw.s[winner].wins++;
    L->losses++;
    bool at_home = rw.bx == L->bx && rw.by == L->by;
    send_home(lose);
    promote(L);
    wipe_trail(winner);
    if (at_home) {
        /* a castle falls with its army; a plain home makes a second stand */
        if (L->castle || rsh_army_size(lose) == 0) { war_over(winner); return; }
        rw.regroup = 1;
        start_battle(winner, rw.bx, rw.by);
        return;
    }
    /* the loser starts a full turn from home */
    rw.first_move = 0;
    rw.turns++;
    rw.turn = lose;
    begin_turn();
}

/* ------------------------------------------------------------------ */
/* the computer's route                                                   */

static int node_value(int side, int x, int y, int *battle) {
    const RshSide *s = &rw.s[side], *o = &rw.s[side ^ 1];
    *battle = 0;
    if ((x == o->nx && y == o->ny) || (x == o->bx && y == o->by) || rsh_in_trail(side ^ 1, x, y)) {
        *battle = 1;
        int mine = rsh_strength(side), theirs = rsh_strength(side ^ 1);
        if (rsh_army_size(side ^ 1) == 0) return 400;   /* nothing left to fight: the war */
        if (rsh_army_size(side) == 0) return -1000;
        int base = x == o->bx && y == o->by;
        /* the computer is bolder; the demo player waits to be stronger */
        int need = s->cpu >= 0 ? 85 : 100;
        if (mine * 100 >= theirs * need) return base ? 90 : 70;
        return -1000;
    }
    int army = rsh_army_size(side);
    switch (rw.node[y][x]) {
    case N_INN: {
        if (rw.visits[y][x] >= 3 || army >= RSH_ARMY + RSH_RESERVE) return 0;
        int best = 0, coins = s->coins + ((s->skills & SK_HAGGLING) ? 4 : 0);
        for (int i = 0; i < 3; i++) {
            int k = rw.inn_offer[y][x][i];
            if (k < K_COUNT && RSH_KIND[k].cost <= coins) best = imax(best, rsh_kind_value(k));
        }
        return best ? 20 + best * 2 : 2;
    }
    case N_TOME: return 45;
    case N_CHEST: return 26;
    case N_SEAM: return rw.owner[y][x] == side + 1 ? 0 : 12 + rw.seam_left[y][x] * (rw.owner[y][x] ? 3 : 2);
    default: return 0;
    }
}

/* The steps the computer (or the demo player) takes this turn toward the
 * most worthwhile place it can reach, in rw.plan_*; 0 or -1: go home. */
int rsh_cpu_map_plan(int side, int8_t *px, int8_t *py) {
    const RshSide *s = &rw.s[side];
    int moves = rw.moves - rw.moves_used;
    if (moves <= 0) return 0;
    int16_t dist[RSH_MH][RSH_MW];
    int8_t parx[RSH_MH][RSH_MW], pary[RSH_MH][RSH_MW];
    for (int y = 0; y < RSH_MH; y++)
        for (int x = 0; x < RSH_MW; x++) dist[y][x] = -1;
    int qx[RSH_MW * RSH_MH], qy[RSH_MW * RSH_MH], qh = 0, qt = 0;
    dist[s->ny][s->nx] = 0;
    qx[qt] = s->nx;
    qy[qt++] = s->ny;
    int bestv = -100000, tx = -1, ty = -1;
    /* breadth-first along roads off our own trail; a node that would start
     * a battle ends a path */
    while (qh < qt) {
        int x = qx[qh], y = qy[qh++];
        int battle;
        if (dist[y][x] > 0) {
            int v = node_value(side, x, y, &battle);
            int sc = v * 10 - dist[y][x] * 45;
            if (v > 0 && sc > bestv) { bestv = sc; tx = x; ty = y; }
            if (battle) continue;
        }
        for (int d = 0; d < 4; d++) {
            int nx = x + RSH_DX[d], ny = y + RSH_DY[d];
            if (!rsh_road(x, y, nx, ny) || dist[ny][nx] >= 0) continue;
            if (nx == s->bx && ny == s->by) continue;
            if (rsh_in_trail(side, nx, ny)) continue;
            int b;
            if (node_value(side, nx, ny, &b) < -100 && b) continue; /* a fight it doesn't want */
            dist[ny][nx] = (int16_t)(dist[y][x] + 1);
            parx[ny][nx] = (int8_t)x;
            pary[ny][nx] = (int8_t)y;
            qx[qt] = nx;
            qy[qt++] = ny;
        }
    }
    if (tx < 0) {
        /* nothing worth going for: toward the other home */
        int ob = 1000;
        for (int y = 0; y < rw.h; y++)
            for (int x = 0; x < rw.w; x++) {
                if (dist[y][x] <= 0) continue;
                int b;
                node_value(side, x, y, &b);
                if (b) continue;
                int dd = iabs(x - rw.s[side ^ 1].bx) + iabs(y - rw.s[side ^ 1].by);
                if (dd < ob) { ob = dd; tx = x; ty = y; }
            }
    }
    bool home = s->nx == s->bx && s->ny == s->by;
    if (tx < 0) {
        /* boxed in: away from home it retreats; at home every move must
         * still be used, so it takes whatever road is left, fight or not */
        if (!home) return -1;
        for (int d = 0; d < 4 && tx < 0; d++) {
            int nx, ny;
            if (step_from(side, s->nx, s->ny, d, &nx, &ny, 0)) { tx = nx; ty = ny; parx[ty][tx] = (int8_t)s->nx; pary[ty][tx] = (int8_t)s->ny; }
        }
        if (tx < 0) return 0;
    }
    int8_t pathx[64], pathy[64];
    int n = 0, x = tx, y = ty;
    while (!(x == s->nx && y == s->ny) && n < 64) {
        pathx[n] = (int8_t)x;
        pathy[n] = (int8_t)y;
        n++;
        int ppx = parx[y][x], ppy = pary[y][x];
        x = ppx;
        y = ppy;
    }
    int steps = 0;
    for (int i = n - 1; i >= 0 && steps < moves; i--) {
        int b;
        rw.plan_x[steps] = px[steps] = pathx[i];
        rw.plan_y[steps] = py[steps] = pathy[i];
        node_value(side, pathx[i], pathy[i], &b);
        plan_r[steps] = (int8_t)(b ? 2 : 1);
        steps++;
        if (b) break;
    }
    /* moves can't be skipped: go on past the target where a road allows */
    while (steps < moves && plan_r[steps - 1] == 1) {
        int ex = px[steps - 1], ey = py[steps - 1], pick = -1, pv = -100000;
        for (int d = 0; d < 4; d++) {
            int nx, ny;
            int r = step_from(side, ex, ey, d, &nx, &ny, steps);
            if (!r) continue;
            int b, v = node_value(side, nx, ny, &b);
            if (r == 2 && v < 0) v = -5000;
            if (r == 3) v = -50;
            if (v > pv) { pv = v; pick = d; }
        }
        if (pick < 0) break;
        if (pv <= -5000 && !home) return -1;
        int nx, ny;
        int r = step_from(side, ex, ey, pick, &nx, &ny, steps);
        rw.plan_x[steps] = px[steps] = (int8_t)nx;
        rw.plan_y[steps] = py[steps] = (int8_t)ny;
        plan_r[steps] = (int8_t)r;
        steps++;
        if (r >= 2) break;
    }
    return steps;
}

/* ------------------------------------------------------------------ */
/* starting a war                                                        */

static void place_sides(void) {
    for (int y = 0; y < rw.h; y++)
        for (int x = 0; x < rw.w; x++) {
            if (rw.node[y][x] == N_BASE0) { rw.s[0].bx = x; rw.s[0].by = y; }
            if (rw.node[y][x] == N_BASE1) { rw.s[1].bx = x; rw.s[1].by = y; }
            if (rw.node[y][x] == N_INN) roll_offers(rw.inn_offer[y][x]);
            if (rw.node[y][x] == N_SEAM) rw.seam_left[y][x] = RSH_SEAM_TURNS;
        }
    send_home(0);
    send_home(1);
}

static void fill_army(RshSide *s, const char *army, const char *res) {
    s->n_army = s->n_reserve = 0;
    for (const char *c = army; c && *c && s->n_army < RSH_ARMY; c++) {
        int k = rsh_kind_of_letter(*c);
        if (k >= 0) s->army[s->n_army++] = (uint8_t)k;
    }
    for (const char *c = res; c && *c && s->n_reserve < RSH_RESERVE; c++) {
        int k = rsh_kind_of_letter(*c);
        if (k >= 0) s->reserve[s->n_reserve++] = (uint8_t)k;
    }
}

static int terrain_char(char c) {
    switch (c) {
    case ':': return T_STONE;
    case 's': return T_SAND;
    case '~': return T_WATER;
    default: return T_GRASS;
    }
}

static int node_char(char c) {
    switch (c) {
    case '+': return N_PLAIN;
    case '1': return N_BASE0;
    case '2': return N_BASE1;
    case 'I': return N_INN;
    case 'T': return N_TOME;
    case '$': return N_SEAM;
    case 'C': return N_CHEST;
    default: return N_NONE;
    }
}

/* the board from its drawing: nodes and roads on the even rows, the
 * vertical roads and the tiles between on the odd ones */
static void parse_rows(const RshScenario *sc) {
    rw.w = sc->w;
    rw.h = sc->h;
    rw.border = sc->border;
    int bt = terrain_char(sc->border);
    for (int y = 0; y <= RSH_MH; y++)
        for (int x = 0; x <= RSH_MW; x++) rw.tile[y][x] = (uint8_t)bt;
    for (int r = 0; r < 2 * sc->h - 1; r++) {
        const char *row = sc->rows[r];
        int len = (int)strlen(row);
        for (int c = 0; c < 2 * sc->w - 1 && c < len; c++) {
            char ch = row[c];
            if (r % 2 == 0) {
                if (c % 2 == 0) rw.node[r / 2][c / 2] = (uint8_t)node_char(ch);
                else if (ch == '-') rw.er[r / 2][c / 2] = 1;
            } else {
                if (c % 2 == 0) { if (ch == '|') rw.ed[r / 2][c / 2] = 1; }
                else rw.tile[(r + 1) / 2][(c + 1) / 2] = (uint8_t)terrain_char(ch);
            }
        }
    }
}

static void war_common(int cpu0, int cpu1) {
    rw.s[0].cpu = cpu0;
    rw.s[1].cpu = cpu1;
    place_sides();
    rw.first_move = 1;
    rw.turns = 0;
    rw.winner = -1;
    begin_turn();
}

void rsh_war_start(const RshScenario *sc, int scen, int cpu0, int cpu1) {
    memset(&rw, 0, sizeof rw);
    rng_seed(&rw.rng, sc->seed);
    rw.scen = scen;
    parse_rows(sc);
    rw.pool = sc->pool;
    rw.battle_coins = sc->battle_coins;
    for (int s = 0; s < 2; s++) {
        rw.s[s].coins = sc->coins[s];
        fill_army(&rw.s[s], sc->army[s], sc->reserve[s]);
        rw.s[s].castle = (sc->castle >> s) & 1;
    }
    rw.turn = sc->first; /* in the campaign Brass always moves first */
    war_common(cpu0, cpu1);
}

/* a random war: its own board; the streak's Plum banner brings more every
 * war won; two players toss for who moves first */
void rsh_war_streak(uint32_t seed, int streak, int versus) {
    memset(&rw, 0, sizeof rw);
    rng_seed(&rw.rng, seed);
    rw.scen = -1;
    rsh_make_streak_map(&rw, &rw.rng);
    rw.pool = 0x7FFF; /* every disk but the empress */
    static const char *const EXTRA = "SWFBLTOHMARPEDY";
    for (int s = 0; s < 2; s++) {
        RshSide *sd = &rw.s[s];
        fill_army(sd, "SS", "WSS");
        sd->coins = 6;
    }
    if (!versus) {
        RshSide *e = &rw.s[1];
        for (int i = 0; i < streak && e->n_army < RSH_ARMY; i++)
            e->army[e->n_army++] = (uint8_t)rsh_kind_of_letter(EXTRA[rng_range(&rw.rng, 0, 14)]);
        e->coins += 2 * streak;
    }
    rw.turn = versus ? (int)(rng_next(&rw.rng) & 1) : 0;
    war_common(-1, versus ? -1 : 10);
}
