/* FULL PEAL - the balloon round. It follows a stage's boss whenever one of
 * the stage's waves was graded 100 or 0. Balloons drift up out of the
 * distance down the lanes: red ones are worth 1, orange ones 3, and every
 * 100 in the stage turns more of them orange. Fifty points in the time
 * buys a continue. */
#include "fpl.h"

#define SPAWN_EVERY 36
#define RISE 0.0075f
#define APPROACH (1.0f / 500)

void fpl_bonus_start(void) {
    fpl_clear_world();
    memset(fpg.bal, 0, sizeof fpg.bal);
    fpg.bonus_pts = 0;
    fpg.bonus_t = 0;
    fpg.bonus_spawned = 0;
    fpg.bonus_won = false;
    fpg.bonus_orange = imin(FPL_BALLOONS, FPL_BALLOON_ORANGE0 + FPL_BALLOON_ORANGE_PER * fpg.perfect_stage);
}

static bool is_orange(int i) {
    int o = fpg.bonus_orange;
    return (i + 1) * o / FPL_BALLOONS != i * o / FPL_BALLOONS;
}

static void pop(int i) {
    FplBalloon *b = &fpg.bal[i];
    b->alive = 0;
    b->popped = 1;
    fpg.bonus_pts += b->orange ? 3 : 1;
    fpl_burst(b->x, b->y, b->z, b->orange ? C_ORANGE : C_RED, 8);
    fpl_sfx(b->orange ? "fpl_pop3" : "fpl_pop", 2);
    if (!fpg.bonus_won && fpg.bonus_pts >= FPL_BONUS_NEED) {
        fpg.bonus_won = true;
        fpl_sfx("fpl_extra", 0);
    }
}

/* a player shot meeting the balloons (called from fpl_shots_update) */
void fpl_bonus_shot(FplPShot *s, float z0) {
    for (int i = 0; i < fpg.bonus_spawned && s->alive; i++) {
        FplBalloon *b = &fpg.bal[i];
        if (!b->alive) continue;
        if (s->side) {
            if (b->z > FPL_PLANE_Z) continue;
            float dx = b->x - s->x, dy = b->y - s->y;
            if (dx * dx + dy * dy < 0.45f * 0.45f) { s->alive = 0; pop(i); }
        } else {
            float lx = fpl_lane_x(s->lane_c), ly = fpl_lane_y(s->lane_r);
            if (b->z <= FPL_PLANE_Z || b->z < z0 - 0.02f || b->z > s->z + 0.02f) continue;
            if (fabsf(b->x - lx) < 0.65f && fabsf(b->y - ly) < 0.65f) { s->alive = 0; pop(i); }
        }
    }
}

void fpl_bonus_update(uint8_t in) {
    fpg.bonus_t++;
    fpl_ship_update(in);
    if (fpg.bonus_spawned < FPL_BALLOONS && fpg.bonus_t % SPAWN_EVERY == 1 && fpg.bonus_t < FPL_BONUS_T - 60) {
        int i = fpg.bonus_spawned++;
        FplBalloon *b = &fpg.bal[i];
        b->alive = 1;
        b->orange = is_orange(i);
        /* a fixed scatter over the six columns */
        static const int8_t COL[12] = {2, 4, 0, 3, 5, 1, 3, 0, 4, 2, 5, 1};
        b->x = fpl_lane_x(COL[i % 12]) + ((i / 12) % 2 ? 0.25f : -0.25f);
        b->y = 1.6f;
        b->z = 0.75f;
        b->t = 0;
    }
    for (int i = 0; i < fpg.bonus_spawned; i++) {
        FplBalloon *b = &fpg.bal[i];
        if (!b->alive) continue;
        b->t++;
        b->y -= RISE;
        b->x += sinf(b->t * 0.05f + i) * 0.004f;
        if (b->z > 0) b->z -= APPROACH;
        if (b->z < 0) b->z = 0;
        if (b->y < -2.5f) b->alive = 0; /* drifted off the top */
    }
    fpl_shots_update();
    fpl_parts_update();
}

bool fpl_bonus_over(void) {
    if (fpg.bonus_t >= FPL_BONUS_T) return true;
    if (fpg.bonus_spawned < FPL_BALLOONS) return false;
    for (int i = 0; i < FPL_BALLOONS; i++)
        if (fpg.bal[i].alive) return false;
    return true;
}
