/* CUTLASS CUP - the match: players, ball, weapons, laws and the CPU.
 * Every rule is listed with its source in docs/games/14-cutlass-cup.md. */
#include "cutlass.h"

const FighterDef CC_FIGHTER[CC_FIGHTERS] = {
    /* name     title                 secondary        super          S  C  P  rx ry  shirt     dark      pants    sash */
    {"HAMDI", "THE YOUNG CORSAIR", "LUCKY COIN", "COMET", 2, 2, 3, 20, 12, C_RED, C_WINE, C_NAVY, C_YELLOW},
    {"LEILA", "THE DECKHAND", "SEA URCHIN", "WALL RUNNER", 3, 2, 2, 15, 10, C_TEAL, C_NIGHT, C_TAN, C_ORANGE},
    {"NOUR", "THE HARPOONER", "HARPOON", "MIRAGE", 2, 3, 2, 21, 18, C_SKY, C_BLUE, C_CREAM, C_RED},
    {"KARIM", "THE SABRE DANCER", "SIROCCO", "WHIRL", 2, 1, 3, 25, 10, C_WHITE, C_LIGHT, C_WINE, C_AMBER},
    {"ZINA", "THE SHADOW", "DART", "DJINN", 3, 1, 2, 17, 11, C_DUSK, C_NIGHT, C_NIGHT, C_MAGENTA},
    {"OMAR", "THE GUNNER", "POWDER POT", "PLUNGE", 1, 3, 2, 13, 20, C_ORANGE, C_BROWN, C_SLATE, C_LIME},
};

static const float MOVE_SPEED[4] = {0, 1.1f, 1.35f, 1.6f};
static const float POWER[4] = {0, 3.0f, 3.4f, 3.8f};
static const float AIM[4] = {0, 0.36f, 0.62f, 1.0f}; /* tan of 20, 32 and 45 degrees */
static const float SUPER_BASE[CC_FIGHTERS] = {5.6f, 5.0f, 4.6f, 5.0f, 4.8f, 5.2f};

#define SWING_LEN 16
#define SWING_ACT0 3
#define SWING_ACT1 8
#define ROLL_LEN 14
#define STUN_BODY 26
#define STUN_SHORT 24
#define STUN_MED 40
#define STUN_LONG 64
#define STALL_FRAMES 300
#define HIT_Z 14
#define BODY_Z 18

static float frnd(Match *m) { return rng_float(&m->rng); }

int cc_team_size(const Match *m, int team) {
    int n = 0;
    for (int i = 0; i < m->np; i++) n += m->pl[i].team == team;
    return n;
}

float cc_min_x(const Match *m, int i) { return m->pl[i].team == 0 ? 12.0f : (float)(CC_CENTER - CC_REACH); }
float cc_max_x(const Match *m, int i) { return m->pl[i].team == 0 ? (float)(CC_CENTER + CC_REACH) : 308.0f; }

static float home_x(const Player *p) { return p->team == 0 ? CC_CIRCLE_L : CC_CIRCLE_R; }

static void place_players(Match *m) {
    for (int i = 0; i < m->np; i++) {
        Player *p = &m->pl[i];
        p->x = home_x(p);
        p->y = p->home_y;
        p->kx = p->ky = 0;
        p->state = PS_IDLE;
        p->t = 0;
        p->stun = 0;
        p->charge = 0;
        p->face = p->team == 0 ? 1 : -1;
        p->ai_delay = 0;
        p->ai_hold = false;
        p->tap_t = 99;
    }
}

static void clear_field(Match *m) {
    for (int i = 0; i < CC_MAX_PROJ; i++) m->pr[i].live = 0;
    for (int i = 0; i < CC_MAX_FAKES; i++) m->fake[i].live = 0;
    memset(&m->ball, 0, sizeof m->ball);
    m->ball.caught_by = -1;
    m->ball.team = -1;
    m->ball.hitter = -1;
    m->stall_t[0] = m->stall_t[1] = 0;
}

void cc_match_init(Match *m, const int *fighters, int np, const bool *cpu, int goal, int time_min, bool laws,
                   int speed_mode, int ai_level, uint64_t seed) {
    memset(m, 0, sizeof *m);
    rng_seed(&m->rng, seed);
    m->np = np;
    for (int i = 0; i < np; i++) {
        Player *p = &m->pl[i];
        p->fighter = (uint8_t)fighters[i];
        p->cpu = cpu[i];
        /* singles: player 0 left, 1 right. doubles: 0 and 1 left, 2 and 3 right */
        p->team = (uint8_t)(np == 2 ? i : i / 2);
        p->input = (uint8_t)(np == 2 ? i : i);
        bool second = np == 4 && (i % 2 == 1);
        p->home_y = np == 2 ? (float)CC_MID : second ? (float)(CC_BOT - 34) : (float)(CC_TOP + 34);
    }
    m->goal = goal;
    m->time_limit = time_min * 60 * 60;
    m->time_left = m->time_limit;
    m->laws = laws;
    m->speed_mode = speed_mode;
    m->spd = speed_mode == SPEED_HYPER ? 1.5f : speed_mode == SPEED_FAST ? 1.25f : 1.0f;
    m->ai_level = ai_level;
    m->winner = -1;
    m->receiver_team = 0;
    m->state = MS_READY;
    clear_field(m);
    place_players(m);
}

/* ------------------------------------------------------------------ */
/* laws and points                                                      */

static void score_point(Match *m, int team, int why) {
    if (m->state != MS_PLAY && m->state != MS_SERVE) return;
    m->score[team]++;
    m->point_team = team;
    m->point_why = why;
    m->state = MS_POINT;
    m->state_t = 0;
    m->fx |= FX_POINT;
    m->receiver_team = team ^ 1;
    m->ball.live = 0;
    for (int i = 0; i < m->np; i++)
        if (m->pl[i].state == PS_CATCH) m->pl[i].state = PS_IDLE;
}

static void foul(Match *m, int team, int kind) {
    if (!m->laws) return;
    m->fouls[team]++;
    m->last_foul_team = team;
    m->last_foul_kind = kind;
    m->foul_flash = 90;
    m->fx |= FX_FOUL;
    if (m->fouls[team] >= 3) {
        /* the third foul ends the point: a penalty point for the other side */
        m->fouls[team] = 0;
        score_point(m, team ^ 1, kind);
    }
}

static void stun(Match *m, Player *p, int frames, float push) {
    if (p->state == PS_CATCH) return;
    p->state = PS_STUN;
    p->stun = frames;
    p->t = 0;
    p->charge = 0;
    p->kx = -p->face * push;
    m->fx |= FX_STUN;
}

static void lose_half(Player *p) {
    if (p->meter > 0) p->meter--;
}

/* ------------------------------------------------------------------ */
/* the serve                                                            */

static void start_serve(Match *m) {
    clear_field(m);
    place_players(m);
    m->state = MS_SERVE;
    m->state_t = 0;
    m->serve_live = true;
}

static void toss(Match *m) {
    Ball *b = &m->ball;
    memset(b, 0, sizeof *b);
    b->live = 1;
    b->caught_by = -1;
    b->team = -1;
    b->hitter = -1;
    b->x = CC_CENTER;
    b->y = CC_TOP + 4;
    b->z = 24;
    int dir = m->receiver_team == 0 ? -1 : 1;
    b->dir = (int8_t)dir;
    /* aim for the receiver (the first of its team) */
    float ty = CC_MID;
    for (int i = 0; i < m->np; i++)
        if (m->pl[i].team == m->receiver_team) { ty = m->pl[i].y; break; }
    b->vx = dir * 1.7f * m->spd;
    float frames = (CC_CENTER - (m->receiver_team == 0 ? CC_CIRCLE_L : CC_CIRCLE_R)) / (1.7f * m->spd);
    if (frames < 0) frames = -frames;
    b->vy = (ty - b->y) / imax(20, (int)frames);
    b->vz = 1.0f * m->spd;
    b->speed = 1.7f * m->spd;
    m->fx |= FX_SERVE;
}

/* ------------------------------------------------------------------ */
/* striking                                                             */

static void set_ball_normal(Ball *b) {
    b->kind = BK_NORMAL;
    b->phase = 0;
    b->curve = 0;
    b->level = 0;
}

static void give_meter(Match *m, Player *p) {
    p->strikes++;
    if (p->strikes % 2 == 0 && p->meter < 6) {
        p->meter++;
        m->fx |= FX_METER;
    }
}

static int held_aim(const Player *p, const Pad *pad) {
    int back = p->team == 0 ? -1 : 1;
    if (pad->dx == back) return 2;       /* a lob */
    return pad->dy;                      /* -1 up, 0 straight, 1 down */
}

static void spawn_fakes(Match *m, int n, float speed, int dir) {
    static const float ANG[4] = {-0.35f, 0.35f, -0.7f, 0.7f};
    for (int k = 0; k < n && k < CC_MAX_FAKES; k++) {
        Ball *f = &m->fake[k];
        *f = m->ball;
        f->kind = BK_NORMAL;
        f->live = 1;
        f->vx = dir * speed * cosf(ANG[k]);
        f->vy = speed * sinf(ANG[k]) + m->ball.vy * 0.5f;
    }
}

static void strike_ball(Match *m, int pi, const Pad *pad, int kind, bool sweet) {
    Player *p = &m->pl[pi];
    Ball *b = &m->ball;
    const FighterDef *f = &CC_FIGHTER[p->fighter];
    /* touching the other side's serve first is a foul */
    if (m->serve_live && b->team == -1 && p->team != m->receiver_team && m->laws) {
        foul(m, p->team, FOUL_SERVE);
        if (m->state == MS_PLAY) start_serve(m);
        return;
    }
    bool reflect = b->kind != BK_NORMAL && b->team != p->team;
    int dir = p->team == 0 ? 1 : -1;
    int aim = held_aim(p, pad);
    float spd = m->spd;
    b->team = (int8_t)p->team;
    b->hitter = (int8_t)pi;
    b->dir = (int8_t)dir;
    b->t = 0;
    b->no_body = 10;
    b->caught_by = -1;
    m->serve_live = false;
    set_ball_normal(b);
    m->fx |= FX_STRIKE;
    m->fx_x = b->x;
    m->fx_y = b->y;
    if (reflect) m->fx |= FX_REFLECT;
    give_meter(m, p);
    if (kind == SW_SUPER && !reflect) {
        int level = iclamp(p->meter / 2, 1, 3);
        p->meter = imax(0, p->meter - 2);
        float s = (SUPER_BASE[p->fighter] + 0.8f * (level - 1)) * spd;
        b->speed = s;
        b->level = (uint8_t)level;
        b->z = imin((int)b->z, 6);
        b->vz = 0;
        b->turn = (int8_t)(aim == 2 ? 0 : aim);
        m->fx |= FX_SUPER;
        switch (p->fighter) {
        case F_HAMDI:
            b->kind = BK_COMET;
            b->vx = dir * s * cosf(aim == 2 ? 0 : aim * 0.44f);
            b->vy = s * sinf(aim == 2 ? 0 : aim * 0.44f);
            break;
        case F_LEILA:
            b->kind = BK_WALL;
            b->turn = (int8_t)(aim < 0 ? -1 : 1);
            b->vx = dir * 1.2f * spd;
            b->vy = b->turn * s;
            break;
        case F_NOUR:
            b->kind = BK_MIRAGE;
            b->vx = dir * s * cosf(aim == 2 ? 0 : aim * 0.3f);
            b->vy = s * sinf(aim == 2 ? 0 : aim * 0.3f);
            spawn_fakes(m, imin(CC_MAX_FAKES, 1 + level), s, dir);
            break;
        case F_KARIM:
            b->kind = BK_WHIRL;
            /* forward held: it will curve down; up/down: it turns that way */
            b->turn = (int8_t)(pad->dy != 0 ? pad->dy : (pad->dx == dir ? 2 : 0));
            b->vx = dir * s;
            b->vy = 0;
            break;
        case F_ZINA:
            b->kind = BK_DJINN;
            b->vx = dir * s;
            b->vy = 0;
            b->curve = (aim == 2 ? 0 : aim) * 0.07f * spd * spd;
            break;
        default:
            b->kind = BK_PLUNGE;
            b->vx = 0;
            b->vy = (b->y < CC_MID ? 1.0f : -1.0f) * s;
            if (fabsf(b->y - CC_MID) < 4) { b->phase = 1; b->vx = dir * s; b->vy = 0; }
            break;
        }
        return;
    }
    float s = POWER[f->power] * spd;
    if (p->roll_boost) s *= 1.45f;
    if (sweet) { s *= 1.25f; m->fx |= FX_SWEET; }
    if (reflect) s = imax((int)s, 4) * 1.0f;
    b->speed = s;
    if (aim == 2) {
        /* back + strike: into the air */
        b->vx = dir * 2.3f * spd;
        b->vy = pad->dy * 0.4f * spd;
        b->z = imax((int)b->z, 2);
        b->vz = 3.4f * spd;
        return;
    }
    float t = AIM[f->control] * aim;
    float len = sqrtf(1 + t * t);
    b->vx = dir * s / len;
    b->vy = s * t / len;
    b->vz = 0;
    if (b->z > 4) b->z = 4;
    /* a rolling strike up or down curves */
    if (p->roll_boost && p->roll_dy != 0 && p->fighter != F_ZINA) {
        b->vy = 0;
        b->curve = p->roll_dy * 0.07f * spd * spd;
        p->curve = true;
    }
}

/* ------------------------------------------------------------------ */
/* secondary weapons                                                    */

static Proj *new_proj(Match *m, int kind, int pi) {
    for (int i = 0; i < CC_MAX_PROJ; i++)
        if (!m->pr[i].live) {
            Proj *r = &m->pr[i];
            memset(r, 0, sizeof *r);
            r->kind = (uint8_t)kind;
            r->owner = (uint8_t)pi;
            r->team = m->pl[pi].team;
            r->live = 1;
            r->x = m->pl[pi].x + m->pl[pi].face * 8;
            r->y = m->pl[pi].y;
            r->z = 8;
            return r;
        }
    return NULL;
}

static void limit_traps(Match *m, int kind, int pi, int max) {
    int n = 0, oldest = -1, oldest_t = -1;
    for (int i = 0; i < CC_MAX_PROJ; i++) {
        Proj *r = &m->pr[i];
        if (r->live && r->kind == kind && r->owner == pi) {
            n++;
            if (r->t > oldest_t) { oldest_t = r->t; oldest = i; }
        }
    }
    if (n >= max && oldest >= 0) m->pr[oldest].live = 0;
}

static void fire_secondary(Match *m, int pi) {
    Player *p = &m->pl[pi];
    int bars = p->meter / 2;
    if (p->fighter == F_NOUR)
        for (int i = 0; i < CC_MAX_PROJ; i++)
            if (m->pr[i].live && m->pr[i].kind == PR_HOOK && m->pr[i].owner == pi) return; /* one line at a time */
    lose_half(p);
    m->fx |= FX_SECOND;
    float spd = m->spd;
    Proj *r = NULL;
    switch (p->fighter) {
    case F_HAMDI:
        r = new_proj(m, PR_COIN, pi);
        if (r) r->vx = p->face * 3.2f * spd;
        break;
    case F_LEILA:
        limit_traps(m, PR_URCHIN, pi, 2);
        r = new_proj(m, PR_URCHIN, pi);
        if (r) { r->z = 10; r->vx = p->face * 1.6f * spd; r->vz = 2.2f * spd; }
        break;
    case F_NOUR:
        r = new_proj(m, PR_HOOK, pi);
        if (r) { r->vx = p->face * 5.0f * spd; r->range = 120; r->z = 8; }
        break;
    case F_KARIM:
        r = new_proj(m, PR_SLASH, pi);
        if (r) { r->vx = p->face * 3.4f * spd; r->range = 72.0f + 36.0f * bars; }
        break;
    case F_ZINA:
        r = new_proj(m, PR_DART, pi);
        if (r) r->vx = p->face * 4.0f * spd;
        break;
    default:
        limit_traps(m, PR_BOMB, pi, 2);
        r = new_proj(m, PR_BOMB, pi);
        if (r) { r->z = 10; r->vx = p->face * 1.5f * spd; r->vz = 2.0f * spd; r->life = 110; }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* players                                                              */

static bool body_at(const Player *p, float x, float y, float pad) {
    return fabsf(x - p->x) <= 6 + pad && y >= p->y - 6 - pad && y <= p->y + 4 + pad;
}

static bool in_swing(const Player *p, float x, float y, float z) {
    const FighterDef *f = &CC_FIGHTER[p->fighter];
    if (z >= HIT_Z) return false;
    if (p->swing_kind == SW_THRUST) {
        /* the trident thrust reaches up or down */
        float dy = (y - p->y) * p->roll_dy;
        return fabsf(x - p->x) <= 9 && dy >= -3 && dy <= 30;
    }
    float dx = (x - p->x) * p->face;
    return dx >= -3 && dx <= f->reach_x && fabsf(y - p->y) <= f->reach_y;
}

static void blast(Match *m, Proj *r) {
    r->kind = PR_BLAST;
    r->t = 0;
    r->life = 18;
    r->vx = r->vy = r->vz = 0;
    r->z = 0;
    m->fx |= FX_BLAST;
    m->fx_x = r->x;
    m->fx_y = r->y;
    for (int i = 0; i < m->np; i++) {
        Player *p = &m->pl[i];
        if (p->team == r->team) continue;
        float dx = p->x - r->x, dy = p->y - r->y;
        if (dx * dx + dy * dy <= 22 * 22) { stun(m, p, STUN_LONG, 1.2f); lose_half(p); }
    }
    Ball *b = &m->ball;
    float dx = b->x - r->x, dy = b->y - r->y;
    if (b->live && b->caught_by < 0 && dx * dx + dy * dy <= 22 * 22) {
        /* the blast launches the ball, whatever its height, in a high curve */
        int dir = r->team == 0 ? 1 : -1;
        if (m->serve_live && b->team == -1 && r->team != m->receiver_team) { foul(m, r->team, FOUL_SERVE); return; }
        set_ball_normal(b);
        b->team = (int8_t)r->team;
        b->hitter = (int8_t)r->owner;
        b->dir = (int8_t)dir;
        b->vx = dir * 2.6f * m->spd;
        b->vy = 0;
        b->vz = 3.0f * m->spd;
        b->curve = (rng_chance(&m->rng, 50) ? 1 : -1) * 0.05f * m->spd * m->spd;
        b->t = 0;
        m->serve_live = false;
    }
}

static void start_swing(Player *p, int kind) {
    p->state = PS_SWING;
    p->t = 0;
    p->swing_kind = kind;
    p->swing_hit = false;
    p->charge = 0;
}

static void update_player(Match *m, int pi, const Pad *pad) {
    Player *p = &m->pl[pi];
    const FighterDef *f = &CC_FIGHTER[p->fighter];
    float spd = m->spd;
    p->t++;
    p->tap_t++;
    if (p->roll_cd > 0) p->roll_cd--;
    p->face = p->team == 0 ? 1 : -1;
    p->hold_dy = pad->dy;
    /* knockback */
    p->x += p->kx;
    p->y += p->ky;
    p->kx *= 0.8f;
    p->ky *= 0.8f;
    if (p->state == PS_WIN || p->state == PS_LOSE) return;
    if (p->state == PS_STUN) {
        if (--p->stun <= 0) { p->state = PS_IDLE; p->t = 0; }
        goto clamp;
    }
    if (p->state == PS_CATCH) {
        Ball *b = &m->ball;
        if (pad->strike) p->mash++;
        b->x = p->x + p->face * 7;
        b->y = p->y;
        b->z = 8;
        p->kx = -p->face * (p->t < 20 ? 1.0f : 0.0f);
        if (p->mash >= 6) {
            /* mashed in time: the super shot flies back */
            p->state = PS_IDLE;
            p->t = 0;
            int dir = p->face;
            set_ball_normal(b);
            b->kind = BK_COMET;
            b->level = 1;
            b->speed = 5.0f * spd;
            b->vx = dir * b->speed;
            b->vy = 0;
            b->z = 6;
            b->team = (int8_t)p->team;
            b->hitter = (int8_t)pi;
            b->dir = (int8_t)dir;
            b->caught_by = -1;
            b->no_body = 12;
            b->t = 0;
            m->fx |= FX_REFLECT | FX_SUPER;
        } else if (p->t >= 48) {
            /* too slow: it slips out and over their head */
            p->state = PS_STUN;
            p->stun = 20;
            b->caught_by = -1;
            b->vx = -p->face * 2.6f * spd;
            b->vy = 0;
            b->vz = 2.4f * spd;
            b->z = 14;
            b->no_body = 60;
            b->hitter = (int8_t)pi;
            set_ball_normal(b);
            m->fx |= FX_SLIP;
        }
        goto clamp;
    }
    /* strike presses: a double tap throws the secondary weapon */
    if (pad->strike) {
        if (p->tap_t <= 12 && p->meter >= 1 && p->state != PS_ROLL) {
            fire_secondary(m, pi);
            p->tap_t = 99;
        } else {
            p->tap_t = 0;
            if (p->state == PS_ROLL) {
                p->roll_boost = true;
                start_swing(p, SW_NORMAL);
            } else if (p->state != PS_SWING) {
                p->roll_boost = false;
                start_swing(p, SW_NORMAL);
            }
        }
    }
    /* holding strike with a full bar charges a super shot */
    if (pad->held && p->meter >= 2 && p->state != PS_SWING && p->state != PS_ROLL) {
        p->state = PS_CHARGE;
        p->charge++;
    }
    if (p->state == PS_CHARGE && !pad->held) {
        if (p->charge >= 12 && p->meter >= 2) { p->roll_boost = false; start_swing(p, SW_SUPER); }
        else { p->state = PS_IDLE; p->charge = 0; }
    }
    /* roll (Zina blinks, Omar only rolls forward and back or thrusts) */
    if (pad->roll && p->roll_cd == 0 && (p->state == PS_IDLE || p->state == PS_RUN)) {
        int dx = pad->dx, dy = pad->dy;
        if (p->fighter == F_ZINA) {
            if (!dx && !dy) dx = p->face;
            float len = sqrtf((float)(dx * dx + dy * dy));
            for (int i = 0; i < CC_MAX_PROJ; i++)
                if (m->pr[i].live && m->pr[i].kind == PR_DECOY && m->pr[i].owner == pi) m->pr[i].live = 0;
            Proj *d = new_proj(m, PR_DECOY, pi);
            if (d) { d->x = p->x; d->y = p->y; d->z = 0; d->life = 90; }
            p->x += dx / len * 44;
            p->y += dy / len * 44;
            p->roll_cd = 24;
            m->fx |= FX_BLINK;
        } else if (p->fighter == F_OMAR && dy != 0 && dx == 0) {
            p->roll_dy = (int8_t)dy;
            start_swing(p, SW_THRUST);
            p->roll_cd = 12;
        } else {
            if (p->fighter == F_OMAR) dy = 0;
            if (!dx && !dy) dx = p->face;
            p->state = PS_ROLL;
            p->t = 0;
            p->roll_dx = (int8_t)dx;
            p->roll_dy = (int8_t)dy;
            p->roll_cd = ROLL_LEN + 8;
            m->fx |= FX_ROLL;
        }
    }
    switch (p->state) {
    case PS_ROLL: {
        if (pad->dx || pad->dy) {
            p->roll_dx = (int8_t)pad->dx;
            p->roll_dy = (int8_t)(p->fighter == F_OMAR ? 0 : pad->dy);
            if (!p->roll_dx && !p->roll_dy) p->roll_dx = (int8_t)p->face;
        }
        float len = sqrtf((float)(p->roll_dx * p->roll_dx + p->roll_dy * p->roll_dy));
        float s = MOVE_SPEED[f->speed] * 2.4f * spd;
        p->x += p->roll_dx / len * s;
        p->y += p->roll_dy / len * s;
        if (p->t >= ROLL_LEN) { p->state = PS_IDLE; p->t = 0; }
        break;
    }
    case PS_SWING: {
        float s = MOVE_SPEED[f->speed] * spd * 0.35f;
        if (p->roll_boost && p->t < 6) s = MOVE_SPEED[f->speed] * spd * 1.6f;
        p->x += pad->dx * s;
        p->y += pad->dy * s;
        if (p->t >= SWING_ACT0 && p->t <= SWING_ACT1) {
            Ball *b = &m->ball;
            if (!p->swing_hit && b->live && b->caught_by < 0 && in_swing(p, b->x, b->y, b->z) &&
                (m->state == MS_PLAY)) {
                p->swing_hit = true;
                strike_ball(m, pi, pad, p->swing_kind == SW_SUPER ? SW_SUPER : SW_NORMAL, p->t == SWING_ACT0);
            }
            /* fake balls pop when struck */
            for (int k = 0; k < CC_MAX_FAKES; k++)
                if (m->fake[k].live && m->fake[k].team != p->team && in_swing(p, m->fake[k].x, m->fake[k].y, m->fake[k].z))
                    m->fake[k].live = 0;
            /* weapons knock back coins and darts, set off powder pots, move urchins */
            for (int i = 0; i < CC_MAX_PROJ; i++) {
                Proj *r = &m->pr[i];
                if (!r->live || !in_swing(p, r->x, r->y, 0)) continue;
                if ((r->kind == PR_COIN || r->kind == PR_DART) && r->team != p->team) {
                    r->vx = -r->vx;
                    r->team = p->team;
                    r->owner = (uint8_t)pi;
                    r->reflected = true;
                    m->fx |= FX_REFLECT;
                } else if (r->kind == PR_BOMB && r->z < 4) {
                    blast(m, r);
                } else if (r->kind == PR_URCHIN && r->z < 2 && r->team != p->team) {
                    r->team = p->team;
                    r->owner = (uint8_t)pi;
                    r->vx = p->face * 2.4f * spd;
                }
            }
            /* striking the other side's body with your weapon is a foul */
            if (!p->swing_hit && p->t == SWING_ACT1)
                for (int j = 0; j < m->np; j++) {
                    Player *o = &m->pl[j];
                    if (o->team == p->team || o->state == PS_STUN) continue;
                    if (in_swing(p, o->x, o->y, 0) || in_swing(p, o->x, o->y - 6, 0)) {
                        stun(m, o, 20, 0.8f);
                        foul(m, p->team, FOUL_WEAPON);
                        break;
                    }
                }
        }
        if (p->t >= SWING_LEN) { p->state = PS_IDLE; p->t = 0; p->roll_boost = false; }
        break;
    }
    case PS_CHARGE: {
        float s = MOVE_SPEED[f->speed] * spd * 0.6f;
        p->x += pad->dx * s;
        p->y += pad->dy * s;
        break;
    }
    default: {
        float s = MOVE_SPEED[f->speed] * spd;
        float len = (pad->dx && pad->dy) ? 0.7071f : 1.0f;
        p->x += pad->dx * s * len;
        p->y += pad->dy * s * len;
        int want = (pad->dx || pad->dy) ? PS_RUN : PS_IDLE;
        if (want != p->state) { p->state = (uint8_t)want; p->t = 0; }
        break;
    }
    }
clamp:
    p->x = fclamp(p->x, cc_min_x(m, pi), cc_max_x(m, pi));
    p->y = fclamp(p->y, CC_TOP + 2, CC_BOT);
}

/* ------------------------------------------------------------------ */
/* the ball                                                             */

static void wall_bounce(Match *m, Ball *b) {
    if (b->y < CC_TOP) { b->y = CC_TOP; b->vy = fabsf(b->vy); b->curve = -fabsf(b->curve) * 0.5f; m->fx |= FX_WALL; }
    if (b->y > CC_BOT) { b->y = CC_BOT; b->vy = -fabsf(b->vy); b->curve = fabsf(b->curve) * 0.5f; m->fx |= FX_WALL; }
}

static void ball_air(Match *m, Ball *b) {
    if (b->z > 0 || b->vz > 0) {
        b->z += b->vz;
        b->vz -= 0.16f * m->spd * m->spd;
        if (b->z <= 0) {
            b->z = 0;
            if (b->vz < -1.2f) {
                b->vz = -b->vz * 0.3f;
                b->vx *= 0.8f;
            } else {
                b->vz = 0;
            }
        }
    }
}

static void move_super(Match *m, Ball *b) {
    int dir = b->dir;
    switch (b->kind) {
    case BK_WALL:
        if (b->phase == 0 && (b->y <= CC_TOP || b->y >= CC_BOT)) {
            b->phase = 1;
            b->y = b->turn < 0 ? CC_TOP : CC_BOT;
            b->vy = 0;
            b->vx = dir * b->speed;
        }
        break;
    case BK_WHIRL:
        if (b->phase == 0 && b->t >= 12) {
            b->phase = 1;
            b->ax = b->x;
            b->ay = b->y - 12;
            b->ang = 1.5708f;
        } else if (b->phase == 1) {
            b->ang += 0.3f * m->spd * dir;
            b->ax += dir * 0.6f * m->spd;
            b->x = b->ax + cosf(b->ang) * 12;
            b->y = b->ay + sinf(b->ang) * 12;
            b->vx = b->vy = 0;
            if (fabsf(b->ang - 1.5708f) >= 4 * 3.14159f) {
                b->phase = 2;
                if (b->turn == -1 || b->turn == 1) {
                    b->vx = dir * b->speed * 0.72f;
                    b->vy = b->turn * b->speed * 0.72f;
                } else {
                    b->vx = dir * b->speed;
                    b->vy = 0;
                    if (b->turn == 2) b->curve = 0.09f * m->spd * m->spd; /* forward held: it dives down */
                }
            }
        }
        break;
    case BK_PLUNGE:
        if (b->phase == 0 && ((b->vy > 0 && b->y >= CC_MID) || (b->vy < 0 && b->y <= CC_MID))) {
            b->phase = 1;
            b->y = CC_MID;
            b->vy = 0;
            b->vx = dir * b->speed;
        }
        break;
    default: break;
    }
}

static void catch_ball(Match *m, int pi) {
    Player *p = &m->pl[pi];
    p->state = PS_CATCH;
    p->t = 0;
    p->mash = 0;
    p->charge = 0;
    m->ball.caught_by = pi;
    m->ball.vx = m->ball.vy = m->ball.vz = 0;
    m->fx |= FX_CATCH;
}

static void update_ball(Match *m) {
    Ball *b = &m->ball;
    if (!b->live || b->caught_by >= 0) return;
    b->t++;
    if (b->no_body > 0) b->no_body--;
    if (b->kind != BK_NORMAL) move_super(m, b);
    if (!(b->kind == BK_WHIRL && b->phase == 1)) {
        b->vy += b->curve;
        b->x += b->vx;
        b->y += b->vy;
    }
    if (!(b->kind == BK_WALL && b->phase == 1)) wall_bounce(m, b);
    else b->y = b->turn < 0 ? CC_TOP : CC_BOT;
    ball_air(m, b);
    /* a ball left on the deck slowly rolls to a stop */
    if (b->kind == BK_NORMAL && b->z <= 0 && b->t > 90) { b->vx *= 0.995f; b->vy *= 0.995f; }
    /* bodies */
    for (int i = 0; i < m->np; i++) {
        Player *p = &m->pl[i];
        if (b->z >= BODY_Z || !body_at(p, b->x, b->y, 0)) continue;
        if (i == b->hitter && b->no_body > 0) continue;
        if (p->state == PS_SWING && p->t <= SWING_ACT1) continue; /* the swing gets the first chance */
        if (b->kind != BK_NORMAL && b->team != p->team && p->state != PS_STUN) {
            catch_ball(m, i);
            return;
        }
        if (b->team == -1 && p->team == m->receiver_team) continue; /* the serve waits for a strike */
        /* it bounces off, and that player is stunned for a moment */
        b->vx = -b->vx * 0.55f;
        b->vy = b->vy * 0.5f + (b->y - p->y) * 0.15f;
        b->x += b->vx * 2;
        b->no_body = 12;
        b->hitter = (int8_t)i;
        if (p->state != PS_STUN) stun(m, p, STUN_BODY, 0.5f);
        m->fx |= FX_BODY;
        m->fx_x = b->x;
        m->fx_y = b->y;
    }
    /* decoy barrels */
    for (int i = 0; i < CC_MAX_PROJ; i++) {
        Proj *r = &m->pr[i];
        if (!r->live || b->z >= 12) continue;
        if (r->kind == PR_DECOY && fabsf(b->x - r->x) < 7 && fabsf(b->y - r->y) < 6) {
            b->vx = -b->vx;
            b->x += b->vx * 2;
            r->live = 0;
            m->fx |= FX_BODY;
        } else if (r->kind == PR_URCHIN && r->z < 2 && fabsf(b->x - r->x) < 6 && fabsf(b->y - r->y) < 6 && b->t > 2) {
            /* the urchin sends the ball toward its owner's foe, downward unless up is held */
            int dir = r->team == 0 ? 1 : -1;
            float s = fmaxf(3.0f * m->spd, sqrtf(b->vx * b->vx + b->vy * b->vy));
            int up = m->pl[r->owner].team == r->team && m->pl[r->owner].hold_dy < 0 ? -1 : 1;
            set_ball_normal(b);
            b->vx = dir * s * 0.72f;
            b->vy = up * s * 0.72f;
            b->dir = (int8_t)dir;
            b->team = (int8_t)r->team;
            b->x += b->vx * 2;
            b->t = 0;
            m->fx |= FX_WALL;
        }
    }
    /* out over the side: a point */
    if (b->x < 6) score_point(m, 1, 0);
    else if (b->x > 314) score_point(m, 0, 0);
}

static void update_fakes(Match *m) {
    for (int k = 0; k < CC_MAX_FAKES; k++) {
        Ball *f = &m->fake[k];
        if (!f->live) continue;
        f->x += f->vx;
        f->y += f->vy;
        wall_bounce(m, f);
        if (f->x < 0 || f->x > 320) f->live = 0;
        for (int i = 0; i < m->np; i++)
            if (m->pl[i].team != f->team && body_at(&m->pl[i], f->x, f->y, 0)) f->live = 0;
    }
}

/* ------------------------------------------------------------------ */
/* projectiles                                                          */

static void hit_player(Match *m, Proj *r, Player *p, int frames) {
    stun(m, p, frames, 1.6f);
    lose_half(p);
    m->fx_x = p->x;
    m->fx_y = p->y;
}

static void proj_ball(Match *m, Proj *r, float push) {
    Ball *b = &m->ball;
    if (!b->live || b->caught_by >= 0 || b->z >= 10) return;
    if (fabsf(b->x - r->x) > 7 || fabsf(b->y - r->y) > 7) return;
    if (m->serve_live && b->team == -1 && r->team != m->receiver_team) {
        if (m->laws) { foul(m, r->team, FOUL_SERVE); if (m->state == MS_PLAY) start_serve(m); return; }
    }
    int dir = r->team == 0 ? 1 : -1;
    if (b->kind != BK_NORMAL && b->team != r->team) m->fx |= FX_REFLECT;
    set_ball_normal(b);
    b->vx = dir * push * m->spd;
    b->vy *= 0.5f;
    b->dir = (int8_t)dir;
    b->team = (int8_t)r->team;
    b->hitter = (int8_t)r->owner;
    b->t = 0;
    b->no_body = 6;
    if (r->team == m->receiver_team) m->serve_live = false;
    m->fx |= FX_STRIKE;
}

static void update_proj(Match *m) {
    for (int i = 0; i < CC_MAX_PROJ; i++) {
        Proj *r = &m->pr[i];
        if (!r->live) continue;
        r->t++;
        Player *own = &m->pl[r->owner];
        switch (r->kind) {
        case PR_COIN:
        case PR_DART:
            r->x += r->vx;
            if (r->x < -8 || r->x > 328) { r->live = 0; break; }
            proj_ball(m, r, 2.8f);
            for (int j = 0; j < m->np; j++) {
                Player *p = &m->pl[j];
                if (p->team == r->team || p->state == PS_STUN || !body_at(p, r->x, r->y, 1)) continue;
                hit_player(m, r, p, r->reflected ? STUN_SHORT : STUN_MED);
                r->live = 0;
                break;
            }
            break;
        case PR_URCHIN:
        case PR_BOMB:
            if (r->z > 0 || r->vz > 0) {
                r->x += r->vx;
                r->z += r->vz;
                r->vz -= 0.16f * m->spd * m->spd;
                if (r->z <= 0) { r->z = 0; r->vz = 0; r->vx = 0; }
            } else if (r->vx != 0) {
                r->x += r->vx; /* an urchin knocked across the deck */
                r->vx *= 0.9f;
                if (fabsf(r->vx) < 0.1f) r->vx = 0;
            }
            if (r->x < 4 || r->x > 316) { r->live = 0; break; }
            if (r->kind == PR_URCHIN && r->z < 2) {
                for (int j = 0; j < m->np; j++) {
                    Player *p = &m->pl[j];
                    if (p->team == r->team || p->state == PS_STUN || !body_at(p, r->x, r->y, 0)) continue;
                    hit_player(m, r, p, STUN_LONG);
                    r->live = 0;
                    break;
                }
            }
            if (r->kind == PR_BOMB && r->z <= 0 && --r->life <= 0) blast(m, r);
            break;
        case PR_HOOK: {
            Ball *b = &m->ball;
            if (!r->hooked) {
                r->dist += fabsf(r->vx);
                r->x += r->vx;
                if (r->dist >= r->range) r->vx = -r->vx * 1.2f;
                float bs = sqrtf(b->vx * b->vx + b->vy * b->vy);
                /* it can't catch a ball that is airborne, too fast or too far */
                if (b->live && b->caught_by < 0 && b->z < 10 && bs < 5.0f * m->spd && fabsf(b->x - r->x) < 7 &&
                    fabsf(b->y - r->y) < 8 && r->dist <= r->range + 2) {
                    if (m->serve_live && b->team == -1 && r->team != m->receiver_team && m->laws) {
                        foul(m, r->team, FOUL_SERVE);
                        if (m->state == MS_PLAY) start_serve(m);
                        break;
                    }
                    r->hooked = true;
                    r->full_range = r->dist >= r->range - 10;
                    set_ball_normal(b);
                    b->team = (int8_t)r->team;
                    b->vz = 0;
                    b->z = 4;
                }
                for (int j = 0; j < m->np; j++) {
                    Player *p = &m->pl[j];
                    if (p->team == r->team || p->state == PS_STUN || !body_at(p, r->x, r->y, 1)) continue;
                    hit_player(m, r, p, STUN_MED);
                    r->vx = -own->face * fabsf(r->vx);
                    break;
                }
                /* back home without a catch */
                if ((r->x - own->x) * own->face < 4 && r->vx * own->face < 0) r->live = 0;
            } else {
                /* reel the ball in */
                float dx = own->x + own->face * 10 - r->x;
                r->x += dx * 0.2f + (dx > 0 ? 1 : -1);
                r->y += (own->y - r->y) * 0.2f;
                b->x = r->x;
                b->y = r->y;
                b->vx = b->vy = 0;
                if (fabsf(own->x + own->face * 10 - r->x) < 4) {
                    r->live = 0;
                    if (!r->full_range) {
                        /* it strikes the ball as soon as it arrives */
                        Pad p0 = {0, 0, false, false, false, false};
                        strike_ball(m, r->owner, &p0, SW_NORMAL, false);
                    } else {
                        b->vx = 0;
                        b->vy = 0;
                    }
                }
            }
            if (r->t > 200) r->live = 0;
            break;
        }
        case PR_SLASH:
            r->x += r->vx;
            r->dist += fabsf(r->vx);
            if (r->dist >= r->range) { r->live = 0; break; }
            if (r->t % 6 == 1) proj_ball(m, r, 3.6f);
            for (int k = 0; k < CC_MAX_PROJ; k++) {
                Proj *o = &m->pr[k];
                if (k == i || !o->live || o->team == r->team || o->kind == PR_BLAST || o->kind == PR_SLASH) continue;
                if (fabsf(o->x - r->x) < 10 && fabsf(o->y - r->y) < 10) o->live = 0; /* cuts through weapons */
            }
            for (int j = 0; j < m->np; j++) {
                Player *p = &m->pl[j];
                if (p->team == r->team || p->state == PS_STUN || r->hooked || !body_at(p, r->x, r->y, 3)) continue;
                hit_player(m, r, p, STUN_MED);
                r->hooked = true; /* only once */
            }
            break;
        case PR_DECOY:
            if (--r->life <= 0) r->live = 0;
            break;
        case PR_BLAST:
            if (--r->life <= 0) r->live = 0;
            break;
        default: r->live = 0; break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* the match                                                            */

static void end_match(Match *m, int winner) {
    m->state = MS_OVER;
    m->state_t = 0;
    m->winner = winner;
    for (int i = 0; i < m->np; i++) {
        m->pl[i].state = m->pl[i].team == winner ? PS_WIN : PS_LOSE;
        m->pl[i].t = 0;
    }
}

void cc_match_update(Match *m, const Pad pads[CC_MAX_PLAYERS]) {
    m->frame++;
    m->state_t++;
    if (m->foul_flash > 0) m->foul_flash--;
    switch (m->state) {
    case MS_READY:
        if (m->state_t >= 80) start_serve(m);
        return;
    case MS_SERVE:
        for (int i = 0; i < m->np; i++) {
            Pad none = {0, 0, false, false, false, false};
            update_player(m, i, m->pl[i].team == m->receiver_team || m->state_t > 20 ? &pads[i] : &none);
        }
        if (m->state_t == 40) { toss(m); m->state = MS_PLAY; m->state_t = 0; }
        break;
    case MS_PLAY:
        for (int i = 0; i < m->np; i++) update_player(m, i, &pads[i]);
        if (m->state != MS_PLAY) break;
        update_ball(m);
        update_fakes(m);
        update_proj(m);
        /* stalling: the ball may not stay on one side too long */
        if (m->state == MS_PLAY && m->ball.live && m->ball.caught_by < 0) {
            int side = m->ball.x < CC_CENTER ? 0 : 1;
            m->stall_t[side]++;
            m->stall_t[side ^ 1] = 0;
            if (m->stall_t[side] > STALL_FRAMES && !m->serve_live) {
                m->stall_t[side] = 0;
                foul(m, side, FOUL_STALL);
            }
        }
        break;
    case MS_POINT:
        for (int i = 0; i < m->np; i++) {
            Pad none = {0, 0, false, false, false, false};
            update_player(m, i, &none);
        }
        update_proj(m);
        if (m->state_t >= 100) {
            int w = -1;
            if (m->score[0] >= m->goal) w = 0;
            if (m->score[1] >= m->goal) w = 1;
            if (m->sudden && m->score[0] != m->score[1]) w = m->score[0] > m->score[1] ? 0 : 1;
            if (w >= 0) end_match(m, w);
            else start_serve(m);
        }
        return;
    case MS_OVER:
        for (int i = 0; i < m->np; i++) m->pl[i].t++;
        return;
    }
    /* the clock */
    if (m->time_limit > 0 && m->time_left > 0 && (m->state == MS_PLAY || m->state == MS_SERVE)) {
        if (--m->time_left == 0) {
            if (m->score[0] != m->score[1]) end_match(m, m->score[0] > m->score[1] ? 0 : 1);
            else m->sudden = true;
        }
    }
}

/* ------------------------------------------------------------------ */
/* the CPU                                                              */

/* where will the ball cross x = px (ignoring curves), bouncing off the rails */
static float predict_y(const Ball *b, float px) {
    if (fabsf(b->vx) < 0.05f) return b->y;
    float t = (px - b->x) / b->vx;
    if (t < 0) return b->y;
    float y = b->y + b->vy * t;
    float h = CC_BOT - CC_TOP;
    float r = fmodf(y - CC_TOP, 2 * h);
    if (r < 0) r += 2 * h;
    return CC_TOP + (r > h ? 2 * h - r : r);
}

static const Player *nearest_foe(const Match *m, const Player *p) {
    const Player *best = NULL;
    float bd = 1e9f;
    for (int i = 0; i < m->np; i++) {
        const Player *o = &m->pl[i];
        if (o->team == p->team) continue;
        float d = fabsf(o->y - p->y);
        if (d < bd) { bd = d; best = o; }
    }
    return best;
}

void cc_ai_pad(Match *m, int i, Pad *out) {
    Player *p = &m->pl[i];
    Ball *b = &m->ball;
    const FighterDef *f = &CC_FIGHTER[p->fighter];
    memset(out, 0, sizeof *out);
    int lvl = iclamp(m->ai_level, 0, 4);
    if (p->state == PS_CATCH) {
        /* mash to throw it back */
        out->strike = (m->frame % 2 == 0) && rng_chance(&m->rng, 30 + lvl * 14);
        return;
    }
    if (p->state == PS_STUN || m->state == MS_OVER || m->state == MS_READY) return;
    if (p->ai_cool > 0) p->ai_cool--;
    const Player *foe = nearest_foe(m, p);
    int dir = p->team == 0 ? 1 : -1;
    bool live = b->live && b->caught_by < 0 && m->state == MS_PLAY;
    bool coming = live && b->vx * dir < -0.1f;
    bool slow = live && fabsf(b->vx) < 0.6f && fabsf(b->vy) < 0.6f;
    bool mine_side = live && (b->x - CC_CENTER) * dir < 0;
    bool others_serve = m->serve_live && p->team != m->receiver_team;
    /* a new trajectory: think again after a moment */
    if (b->t == 1 || b->t == 0) {
        p->ai_delay = imax(3, 16 - lvl * 3);
        p->ai_err = (frnd(m) * 2 - 1) * (14 - lvl * 2.6f);
    }
    float tx = home_x(p), ty = p->home_y;
    if (live && !others_serve) {
        if (b->z > 16 && b->vz != 0) {
            /* a high ball: it chases the ball itself, not where it will land */
            ty = b->y;
            tx = b->x;
        } else if (coming) {
            ty = predict_y(b, p->x) + p->ai_err;
            if (m->np == 4) {
                /* doubles: cover your own half */
                bool top = p->home_y < CC_MID;
                if ((top && ty > CC_MID + 12) || (!top && ty < CC_MID - 12)) ty = p->home_y;
            }
            tx = home_x(p) + dir * 10;
        } else if (slow && mine_side) {
            tx = b->x - dir * 10;
            ty = b->y;
        } else {
            ty = CC_MID + (p->home_y - CC_MID) + (b->y - CC_MID) * 0.3f;
        }
    }
    if (p->ai_delay > 0) {
        p->ai_delay--;
        tx = p->ai_tx;
        ty = p->ai_ty;
    } else {
        p->ai_tx = tx;
        p->ai_ty = ty;
    }
    float ddx = tx - p->x, ddy = ty - p->y;
    if (ddx > 3) out->dx = 1;
    if (ddx < -3) out->dx = -1;
    if (ddy > 2) out->dy = 1;
    if (ddy < -2) out->dy = -1;
    /* a quick roll when the ball is about to get past */
    if (coming && fabsf(ddy) > 26 && fabsf(b->x - p->x) < 70 && p->roll_cd == 0 && rng_chance(&m->rng, 10 + lvl * 8)) {
        out->roll = true;
        out->dx = 0;
    }
    /* super shot: charge while the ball is on its way */
    if (p->meter >= 2 && p->ai_cool == 0 && coming && fabsf(b->x - p->x) > 50 && fabsf(b->x - p->x) < 150 &&
        !p->ai_hold && rng_chance(&m->rng, 4 + lvl * 2)) {
        p->ai_hold = true;
    }
    float bx = (b->x - p->x) * dir;
    bool reach = live && b->z < HIT_Z - 2 && bx >= -2 && bx <= f->reach_x + 2 && fabsf(b->y - p->y) <= f->reach_y;
    if (p->ai_hold) {
        out->held = true;
        if (reach || !live || !coming) {
            out->held = false;
            p->ai_hold = false;
            p->ai_cool = 60;
        }
        if (p->state == PS_CHARGE) goto aim;
    }
    /* strike when it can reach the ball (never the other side's serve, never a foe's body) */
    if (reach && !others_serve && p->state != PS_SWING && (b->team != p->team || slow || b->team == -1)) {
        bool body_in = false;
        for (int j = 0; j < m->np; j++) {
            const Player *o = &m->pl[j];
            if (o->team != p->team && fabsf(o->x - p->x) < f->reach_x + 6 && fabsf(o->y - p->y) < f->reach_y + 6) body_in = true;
        }
        if (!body_in || fabsf(b->x - p->x) < 10) out->strike = true;
    }
aim:
    if (out->strike || (p->state == PS_CHARGE && !out->held)) {
        /* aim away from the foe; lob over one who comes forward */
        out->dx = 0;
        if (foe) {
            bool forward = (foe->x - CC_CENTER) * dir < 30;
            int r = rng_range(&m->rng, 0, 99);
            if (forward && r < 45) out->dx = (int8_t)-dir;
            else if (r < 30) out->dy = 0;
            else out->dy = foe->y < CC_MID ? 1 : -1;
        }
    }
    /* the secondary weapon: when the foe is lined up and the ball is away */
    if (p->meter >= 1 && p->ai_cool == 0 && !out->strike && !p->ai_hold && foe && live && !others_serve) {
        bool lined = fabsf(foe->y - p->y) < 12;
        bool trap = p->fighter == F_LEILA || p->fighter == F_OMAR;
        bool hook = p->fighter == F_NOUR && coming && fabsf(b->y - p->y) < 8 && fabsf(b->x - p->x) < 110;
        if ((lined && !coming && rng_chance(&m->rng, 3 + lvl)) || (trap && !coming && rng_chance(&m->rng, 1 + lvl)) ||
            (hook && rng_chance(&m->rng, 4))) {
            p->ai_plan = 1;
        }
    }
    if (p->ai_plan == 1) { out->strike = true; p->ai_plan = 2; }
    else if (p->ai_plan >= 2 && p->ai_plan < 5) p->ai_plan++;
    else if (p->ai_plan == 5) { out->strike = true; p->ai_plan = 0; p->ai_cool = 90; }
}
