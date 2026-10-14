/* ANTIPODE - the suit: walking, the jump, the flip, the gun, the four mods
 * in their three systems, harm and hazards, things in a room, and walking
 * from room to room. */
#include "apd.h"

int apd_tele_dest(int link, int from_room);

/* a 32-step circle, 256 = 1 */
const int16_t APD_CIRC[32] = {0,   50,  98,  142, 181, 213, 237, 251, 256, 251, 237, 213, 181, 142, 98,  50,
                              0,   -50, -98, -142, -181, -213, -237, -251, -256, -251, -237, -213, -181, -142, -98, -50};


int apd_max_armor(void) { return APD_ARMOR0 + APD_ARMOR_STEP * ag.run.cells; }

void apd_burst(int x, int y, int col, int n, int sp) {
    if (apd_simulating) return;
    for (int k = 0; k < n; k++)
        for (int i = 0; i < APD_MAX_PARTS; i++)
            if (ag.part[i].life <= 0) {
                ApdPart *p = &ag.part[i];
                int a = (k * 32 / imax(1, n) + (int)(ag.frame_t * 7)) & 31;
                int s = sp * (2 + (k % 3)) / 3;
                p->x = x * APD_FX;
                p->y = y * APD_FX;
                p->vx = apd_cos32(a) * s / 2;
                p->vy = apd_sin32(a) * s / 2;
                p->life = 16 + (k % 4) * 4;
                p->col = col;
                break;
            }
}

/* ---- harm ------------------------------------------------------------------ */

static void drain(int n) {
    if (ag.god || n <= 0) return;
    ag.pl.armor -= n;
    if (ag.pl.armor <= 0) {
        ag.pl.armor = 0;
        apd_lose_body();
    }
}

void apd_player_hurt(int dmg, int src, bool spikes) {
    ApdPlayer *p = &ag.pl;
    if (p->inv > 0 || ag.state != AS_PLAY) return;
    bool magma = apd_has(MOD_EMBER, SYS_FRAME), iron = apd_has(MOD_MASS, SYS_FRAME);
    if (spikes) dmg = iron && magma ? 5 : iron ? 10 : magma ? APD_SPIKE_DMG / 2 : APD_SPIKE_DMG;
    else if (magma) dmg = (dmg + 1) / 2;
    if (apd_overload(SYS_FRAME)) dmg = dmg * 3 / 2;
    ag.hits_taken++;
    ag.last_hurt = src;
    p->inv = APD_MERCY;
    apd_sfx("apd_hurt");
    ag.shake = imax(ag.shake, 4);
    drain(dmg);
}

/* SIPHON: harm done gives a little armor back */
static void leech(int dmg) {
    if (!apd_has(MOD_VOLT, SYS_FRAME)) return;
    ag.pl.leech += dmg;
    while (ag.pl.leech >= 10) {
        ag.pl.leech -= 10;
        if (ag.pl.armor < apd_max_armor()) ag.pl.armor++;
    }
}

void apd_hurt_foe_by_player(int i, int dmg);
void apd_hurt_foe_by_player(int i, int dmg) {
    if (i < 0 || i >= APD_MAX_FOES || !ag.foe[i].alive) return;
    int hp0 = ag.foe[i].hp;
    int o = ag.foe[i].owner;
    if (o >= 0 && o < APD_MAX_FOES && ag.foe[o].alive) hp0 = ag.foe[o].hp;
    apd_hurt_foe(i, dmg, true);
    int hp1 = (o >= 0 && o < APD_MAX_FOES) ? (ag.foe[o].alive ? ag.foe[o].hp : 0) : (ag.foe[i].alive ? ag.foe[i].hp : 0);
    if (hp0 > hp1) leech(hp0 - hp1);
}

/* ---- the shots ------------------------------------------------------------- */

int apd_shot_range(void) { return APD_SHOT_LIFE * (apd_has(MOD_MASS, SYS_GUN) ? 2 : 1); }

static int add_shot(int kind, int x, int y, int vx, int vy, int dmg, int life) {
    for (int i = 0; i < APD_MAX_SHOTS; i++)
        if (!ag.shot[i].alive) {
            ApdShot *s = &ag.shot[i];
            memset(s, 0, sizeof *s);
            s->kind = (uint8_t)kind;
            s->alive = 1;
            s->x = x;
            s->y = y;
            s->vx = vx;
            s->vy = vy;
            s->dmg = dmg;
            s->life = life;
            return i;
        }
    return -1;
}

/* one trigger pull from (x, y): the main shot and, with FANSHOT, two more */
static void volley(int kind, int x, int y, int face, int aim, int grav) {
    int dx = 0, dy = 0;
    if (aim == 1) dy = -grav;
    else if (aim == 2) dy = grav;
    else dx = face;
    bool hot = apd_has(MOD_EMBER, SYS_GUN), zap = apd_has(MOD_VOLT, SYS_GUN);
    int dmg = APD_SHOT_DMG * (hot ? 2 : 1), life = apd_shot_range();
    int sp = APD_SHOT_SPEED;
    int i = add_shot(kind, x, y, dx * sp, dy * sp, dmg, life);
    if (i >= 0) { ag.shot[i].hot = hot; ag.shot[i].zap = zap; }
    if (apd_has(MOD_TWIN, SYS_GUN)) {
        int d = sp * 181 / 256; /* 45 degrees */
        for (int s = -1; s <= 1; s += 2) {
            int vx, vy;
            if (dx) { vx = dx * d; vy = s * d; }
            else { vx = s * d; vy = dy * d; }
            int k = add_shot(kind, x, y, vx, vy, dmg, life);
            if (k >= 0) { ag.shot[k].hot = hot; ag.shot[k].zap = zap; }
        }
    }
}

static void fire(void) {
    ApdPlayer *p = &ag.pl;
    int aim = 0;
    if (apd_btn(BTN_UP)) aim = 1;
    else if (apd_btn(BTN_DOWN) && !p->ground) aim = 2;
    p->aim = aim;
    int x = p->x, y = p->y;
    if (aim == 0) {
        x += p->face * 8 * APD_FX;
        y += p->grav * (p->crouch ? 5 : -2) * APD_FX;
    } else {
        y += (aim == 1 ? -p->grav : p->grav) * 10 * APD_FX;
        x += p->face * 2 * APD_FX;
    }
    volley(PS_GUN, x, y, p->face, aim, p->grav);
    if (ag.shadow.on) {
        ApdShadow *s = &ag.shadow;
        int sx = s->x, sy = s->y;
        if (s->aim == 0) { sx += s->face * 8 * APD_FX; sy += s->grav * -2 * APD_FX; }
        else sy += (s->aim == 1 ? -s->grav : s->grav) * 10 * APD_FX;
        volley(PS_SHADOW, sx, sy, s->face, s->aim, s->grav);
    }
    if (ag.drone.on && ag.drone.latch < 0) {
        int vx = aim == 0 ? p->face * APD_SHOT_SPEED : 0;
        int vy = aim == 0 ? 0 : (aim == 1 ? -p->grav : p->grav) * APD_SHOT_SPEED;
        add_shot(PS_DRONE, ag.drone.x, ag.drone.y, vx, vy, 3, apd_shot_range());
    }
    p->shots_fired++;
    apd_sfx(apd_has(MOD_EMBER, SYS_GUN) ? "apd_hotshot" : "apd_shot");
    if (apd_overload(SYS_GUN)) drain(1); /* an overloaded gun costs 1 a shot */
}

static bool shot_hits_box(const ApdShot *s, int cx, int cy, int w, int h) {
    int sx = APD_PX(s->x), sy = APD_PX(s->y);
    int r = s->kind == PS_WAVE ? 6 : 2;
    return rects_overlap(sx - r, sy - r, r * 2, r * 2, cx - w / 2, cy - h / 2, w, h);
}

static void burn_tile(int tx, int ty) {
    int d = ag.tdoor[ty][tx] - 1;
    if (d >= 0) {
        apd_open_gate(d);
        apd_sfx("apd_boom");
        apd_burst(tx * APD_TILE + 8, ty * APD_TILE + 8, C_ORANGE, 12, 3);
        /* the blast hurts foes near by, not the suit */
        for (int i = 0; i < APD_MAX_FOES; i++)
            if (ag.foe[i].alive && iabs(APD_PX(ag.foe[i].x) - (tx * APD_TILE + 8)) < 40 &&
                iabs(APD_PX(ag.foe[i].y) - (ty * APD_TILE + 8)) < 40)
                apd_hurt_foe(i, 30, true);
    }
}

void apd_shots_update(void) {
    for (int i = 0; i < APD_MAX_SHOTS; i++) {
        ApdShot *s = &ag.shot[i];
        if (!s->alive) continue;
        s->x += s->vx;
        s->y += s->vy;
        if (--s->life <= 0) { s->alive = 0; continue; }
        int px = APD_PX(s->x), py = APD_PX(s->y);
        /* the Crown's lightning deletes shots */
        if (ag.lightning_t > 0) {
            extern int apd_lightning_x[4];
            for (int k = 0; k < 4; k++)
                if (apd_lightning_x[k] >= 0 && iabs(px - apd_lightning_x[k]) < 10) s->alive = 0;
            if (!s->alive) continue;
        }
        int t = apd_tile_at(px, py);
        if (s->kind != PS_WAVE && !s->zap) {
            if (t == T_BARREL || t == T_FAKE) {
                if (s->hot) burn_tile(px / APD_TILE, py / APD_TILE);
                else apd_sfx("apd_tink");
                s->alive = 0;
                continue;
            }
            if (t == T_WALL || t == T_SPUP || t == T_SPDN || t == T_CRACK || t == T_DOOR || t == T_LOCK || t == T_SHUT ||
                t == T_GREY || px < 0 || py < 0 || px >= ag.rw * APD_TILE || py >= ag.rh * APD_TILE) {
                s->alive = 0;
                apd_burst(px, py, C_GREY, 2, 1);
                continue;
            }
        } else if (s->zap && (t == T_BARREL || t == T_FAKE) && s->hot) {
            burn_tile(px / APD_TILE, py / APD_TILE);
        }
        if (s->kind == PS_WAVE && (t == T_WALL || t == T_GREY)) { s->alive = 0; continue; }
        /* targets in walls */
        for (int k = 0; k < ag.nth && s->alive; k++) {
            ApdThing *th = &ag.th[k];
            if (!th->alive) continue;
            int tx = th->tx * APD_TILE + 8, ty = th->ty * APD_TILE + 8;
            if (th->kind == TH_TARGET && iabs(px - tx) < 8 && iabs(py - ty) < 8) {
                apd_open_gate(th->id);
                apd_sfx("apd_unlock");
                apd_burst(tx, ty, C_CYAN, 8, 2);
                s->alive = 0;
            }
        }
        if (!s->alive) continue;
        /* foes */
        for (int f = 0; f < APD_MAX_FOES; f++) {
            ApdFoe *fo = &ag.foe[f];
            if (!fo->alive || (fo->flags & FF_NOHIT)) continue;
            if (!shot_hits_box(s, APD_PX(fo->x), APD_PX(fo->y), fo->w, fo->h)) continue;
            if (fo->flags & FF_REFLECT) {
                /* the shot bounces back */
                s->vx = -s->vx;
                s->vy = -s->vy;
                s->kind = PS_WAVE; /* can't hit again */
                s->life = 6;
                apd_sfx("apd_tink");
                break;
            }
            apd_hurt_foe_by_player(f, s->dmg);
            s->alive = 0;
            break;
        }
    }
}

/* The quiet vault's two pads, one in the floor and one in the ceiling right
 * above it: each is pressed by the suit or by its shadow touching it. Both
 * at once, and the grey wall slides away. */
static bool box_on_pad(int x, int y, const ApdThing *t, int reach) {
    int px = APD_PX(x), py = APD_PX(y);
    int tx = t->tx * APD_TILE + 8;
    if (iabs(px - tx) > 10) return false;
    if (t->var == 0) { /* floor pad: the box's bottom at the tile's bottom, or a little over it */
        int floor = (t->ty + 1) * APD_TILE, bottom = py + APD_PH / 2;
        return bottom <= floor + 1 && bottom >= floor - reach;
    }
    int ceil = t->ty * APD_TILE, top = py - APD_PH / 2;
    return top >= ceil - 1 && top <= ceil + reach;
}

static void pads_update(void) {
    int on = 0, n = 0;
    for (int i = 0; i < ag.nth; i++) {
        ApdThing *t = &ag.th[i];
        if (t->kind != TH_BUTTON) continue;
        n++;
        bool pressed = box_on_pad(ag.pl.x, ag.pl.y, t, 3) || (ag.shadow.on && box_on_pad(ag.shadow.x, ag.shadow.y, t, 24));
        if (pressed && !t->flag) apd_sfx("apd_click");
        t->flag = pressed;
        t->t = pressed ? 12 : 0;
        on += pressed;
    }
    ag.button_t[0] = on;
    if (n == 2 && on == 2 && !ag.run.secret_open) {
        ag.run.secret_open = 1;
        for (int y = 0; y < ag.rh; y++)
            for (int x = 0; x < ag.rw; x++)
                if (ag.tile[y][x] == T_GREY) ag.tile[y][x] = T_AIR;
        apd_sfx("apd_unlock");
        apd_say("THE GREY WALL SLIDES AWAY");
    }
}

/* ---- the flip ---------------------------------------------------------------- */

static void flip(void) {
    ApdPlayer *p = &ag.pl;
    p->grav = -p->grav;
    p->vy = 0;
    p->jumping = false;
    p->flip_id++;
    p->flips++;
    p->air_flips++;
    apd_sfx("apd_flip");
    if (apd_has(MOD_EMBER, SYS_FLIP)) {
        /* FLARE: a short heat wave both ways */
        for (int d = -1; d <= 1; d += 2) {
            int k = add_shot(PS_WAVE, p->x, p->y, d * 3 * APD_FX, 0, 15, 16);
            if (k >= 0) ag.shot[k].hot = 1;
        }
    }
    if (apd_has(MOD_TWIN, SYS_FLIP)) {
        ag.shadow.on = true;
        ag.shadow.x = p->x;
        ag.shadow.y = p->y;
        ag.shadow.face = p->face;
        ag.shadow.aim = apd_btn(BTN_UP) ? 1 : apd_btn(BTN_DOWN) ? 2 : 0;
        ag.shadow.grav = -p->grav; /* it stays as he was */
        ag.shadow.t = APD_SHADOW_LIFE;
    }
    /* STORMBALL: the ball holds from the first flip of a jump to the landing,
     * through up to three flips; a fourth puts it out */
    if (apd_has(MOD_VOLT, SYS_FLIP)) p->thunder_t = p->air_flips <= APD_THUNDER_FLIPS ? 1 : 0;
    if (apd_has(MOD_MASS, SYS_FLIP)) {
        p->stomp = true;
        p->vy = p->grav * APD_STOMP;
    }
    if (apd_overload(SYS_FLIP)) drain(2); /* an overloaded FLIP costs 2 a flip */
}

/* ---- moving ------------------------------------------------------------------- */

static bool solid_at(int x, int y, int vy) {
    return apd_box_solid(APD_PX(x), APD_PX(y), APD_PW, APD_PH, ag.pl.grav, vy);
}

/* ANVIL breaks the cracked concrete it lands on */
static void stomp_break(void) {
    ApdPlayer *p = &ag.pl;
    int px = APD_PX(p->x), py = APD_PX(p->y) + p->grav * (APD_PH / 2 + 2);
    for (int dx = -APD_PW / 2; dx <= APD_PW / 2; dx += APD_PW / 2) {
        int tx = (px + dx) / APD_TILE, ty = py / APD_TILE;
        if (tx >= 0 && ty >= 0 && tx < ag.rw && ty < ag.rh && ag.tile[ty][tx] == T_CRACK) {
            int d = ag.tdoor[ty][tx] - 1;
            if (d >= 0) apd_open_gate(d);
            else ag.tile[ty][tx] = T_AIR;
            apd_sfx("apd_crumble");
            apd_burst(tx * APD_TILE + 8, ty * APD_TILE + 8, C_TAN, 10, 3);
        }
    }
}

static void touch_doors(int dir) {
    /* walking into a door opens it (a pass door only with the pass) */
    ApdPlayer *p = &ag.pl;
    int px = APD_PX(p->x) + dir * (APD_PW / 2 + 1);
    for (int dy = -APD_PH / 2; dy < APD_PH / 2; dy += 4) {
        int py = APD_PX(p->y) + dy;
        int tx = px / APD_TILE, ty = py / APD_TILE;
        if (px < 0 || tx >= ag.rw || py < 0 || ty >= ag.rh) continue;
        int t = ag.tile[ty][tx], d = ag.tdoor[ty][tx] - 1;
        if (d < 0) continue;
        if (t == T_DOOR || (t == T_LOCK && ag.run.pass >= apd_doors[d].type - E_PASS1 + 1)) {
            for (int y = 0; y < ag.rh; y++)
                for (int x = 0; x < ag.rw; x++)
                    if (ag.tdoor[y][x] == d + 1 && (ag.tile[y][x] == T_DOOR || ag.tile[y][x] == T_LOCK)) ag.tile[y][x] = T_AIR;
            apd_sfx("apd_door");
        } else if (t == T_LOCK && ag.msg_t <= 0) {
            static char m[32];
            snprintf(m, sizeof m, "PASS %d NEEDED", apd_doors[d].type - E_PASS1 + 1);
            apd_say(m);
            apd_sfx("apd_deny");
        }
    }
}

static void touch_doors_v(int dir) {
    ApdPlayer *p = &ag.pl;
    int py = APD_PX(p->y) + dir * (APD_PH / 2 + 1);
    for (int dx = -APD_PW / 2; dx <= APD_PW / 2; dx += APD_PW / 2) {
        int px = APD_PX(p->x) + dx;
        int tx = px / APD_TILE, ty = py / APD_TILE;
        if (px < 0 || tx >= ag.rw || py < 0 || ty >= ag.rh) continue;
        int t = ag.tile[ty][tx], d = ag.tdoor[ty][tx] - 1;
        if (d < 0) continue;
        if (t == T_LOCK && ag.run.pass >= apd_doors[d].type - E_PASS1 + 1) {
            for (int y = 0; y < ag.rh; y++)
                for (int x = 0; x < ag.rw; x++)
                    if (ag.tdoor[y][x] == d + 1 && ag.tile[y][x] == T_LOCK) ag.tile[y][x] = T_AIR;
            apd_sfx("apd_door");
        } else if (t == T_LOCK && ag.msg_t <= 0) {
            static char m[32];
            snprintf(m, sizeof m, "PASS %d NEEDED", apd_doors[d].type - E_PASS1 + 1);
            apd_say(m);
            apd_sfx("apd_deny");
        }
    }
}

static void move_x(int vx) {
    ApdPlayer *p = &ag.pl;
    if (!vx) return;
    int step = isign(vx) * APD_FX;
    int left = iabs(vx);
    while (left > 0) {
        int d = left >= APD_FX ? step : isign(vx) * left;
        if (solid_at(p->x + d, p->y, 0)) {
            touch_doors(isign(vx));
            /* a door just opened: keep going next frame */
            p->x = (APD_PX(p->x) * APD_FX) + (vx > 0 ? APD_FX - 1 : 0);
            return;
        }
        p->x += d;
        left -= APD_FX;
    }
}

static void move_y(int vy) {
    ApdPlayer *p = &ag.pl;
    if (!vy) return;
    int step = isign(vy) * APD_FX;
    int left = iabs(vy);
    while (left > 0) {
        int d = left >= APD_FX ? step : isign(vy) * left;
        if (solid_at(p->x, p->y + d, d)) {
            touch_doors_v(isign(vy)); /* a hatch he has the pass for opens, floor or ceiling */
            if (!solid_at(p->x, p->y + d, d)) { p->y += d; left -= APD_FX; continue; }
            if (isign(vy) == p->grav) {
                if (p->stomp) stomp_break();
                if (!solid_at(p->x, p->y + d, d)) { p->y += d; left -= APD_FX; continue; }
                if (!p->ground) {
                    if (p->stomp) { ag.shake = 6; apd_sfx("apd_stomp"); }
                    else apd_sfx("apd_land");
                }
                p->ground = true;
                p->stomp = false;
                p->air_flips = 0;
                p->thunder_t = 0;
            } else p->jumping = false;
            p->vy = 0;
            return;
        }
        p->y += d;
        left -= APD_FX;
    }
}

void apd_player_update(void) {
    ApdPlayer *p = &ag.pl;
    bool left = apd_btn(BTN_LEFT), right = apd_btn(BTN_RIGHT), down = apd_btn(BTN_DOWN);
    bool fire_held = apd_btn(BTN_A);
    if (p->inv > 0) p->inv--;
    if (!apd_has(MOD_VOLT, SYS_FLIP)) p->thunder_t = 0;
    /* the facing locks while the gun fires */
    if (!fire_held) {
        if (left && !right) p->face = -1;
        if (right && !left) p->face = 1;
    }
    p->firing = fire_held;
    p->crouch = p->ground && down;
    int vx = 0;
    if (!p->crouch) {
        if (left && !right) vx = -APD_WALK;
        if (right && !left) vx = APD_WALK;
    }
    p->vx = vx;
    /* jump, or in the air flip */
    if (apd_btnp(BTN_B)) {
        if (p->ground) {
            p->vy = -p->grav * APD_JUMP;
            p->ground = false;
            p->jumping = true;
            apd_sfx("apd_jump");
        } else flip();
    }
    if (p->jumping && !apd_btn(BTN_B) && p->vy * p->grav < -APD_JUMP_CUT) {
        p->vy = -p->grav * APD_JUMP_CUT;
        p->jumping = false;
    }
    if (p->stomp) p->vy = p->grav * APD_STOMP;
    else {
        p->vy += p->grav * APD_GRAV;
        if (p->vy * p->grav > APD_FALL) p->vy = p->grav * APD_FALL;
    }
    if (p->vy * p->grav >= 0) p->jumping = p->jumping && p->vy * p->grav < 0;
    move_x(p->vx);
    bool was_ground = p->ground;
    p->ground = false;
    move_y(p->vy);
    if (!p->ground && was_ground && p->vy == 0) {
        /* standing still: check the floor is still there */
        if (solid_at(p->x, p->y + p->grav * APD_FX, p->grav)) p->ground = true;
    }
    if (!p->ground && solid_at(p->x, p->y + p->grav * APD_FX, p->grav) && p->vy * p->grav >= 0) {
        p->ground = true;
        p->air_flips = 0;
        p->thunder_t = 0;
        p->stomp = false;
    }
    /* the gun */
    if (fire_held) {
        if (p->fire_t <= 0) {
            fire();
            p->fire_t = APD_FIRE_EVERY;
        }
    } else p->fire_t = 0;
    if (p->fire_t > 0) p->fire_t--;
    if (!fire_held) p->aim = apd_btn(BTN_UP) ? 1 : (down && !p->ground ? 2 : 0);
    if (p->vx) p->anim++;
    /* the shadow and the drone */
    if (ag.shadow.on && --ag.shadow.t <= 0) ag.shadow.on = false;
    ag.drone.on = apd_has(MOD_TWIN, SYS_FRAME);
    if (ag.drone.on) {
        ApdDrone *d = &ag.drone;
        d->t++;
        if (d->latch >= 0 && (!ag.foe[d->latch].alive || iabs(APD_PX(ag.foe[d->latch].x) - APD_PX(p->x)) > 120 ||
                              iabs(APD_PX(ag.foe[d->latch].y) - APD_PX(p->y)) > 120))
            d->latch = -1;
        if (d->latch < 0) {
            int best = -1, bd = 56;
            for (int i = 0; i < APD_MAX_FOES; i++) {
                const ApdFoe *f = &ag.foe[i];
                if (!f->alive || (f->flags & FF_NOHIT)) continue;
                int dd = imax(iabs(APD_PX(f->x) - APD_PX(p->x)), iabs(APD_PX(f->y) - APD_PX(p->y)));
                if (dd < bd) { bd = dd; best = i; }
            }
            d->latch = best;
        }
        int tx, ty;
        if (d->latch >= 0) {
            tx = ag.foe[d->latch].x;
            ty = ag.foe[d->latch].y;
            if (d->t % 8 == 0 && iabs(d->x - tx) < 8 * APD_FX && iabs(d->y - ty) < 8 * APD_FX)
                apd_hurt_foe_by_player(d->latch, 1);
        } else {
            d->ang = (d->ang + 1) & 31;
            tx = p->x + apd_cos32(d->ang) * 22;
            ty = p->y + apd_sin32(d->ang) * 14;
        }
        d->x += (tx - d->x) / 4;
        d->y += (ty - d->y) / 4;
    } else ag.drone.latch = -1;
}

/* ---- hazards ------------------------------------------------------------------- */

static bool box_touches(int t) {
    ApdPlayer *p = &ag.pl;
    /* the box, one pixel bigger all round: its edges, corners included */
    int x0 = APD_PX(p->x) - APD_PW / 2 - 1, x1 = APD_PX(p->x) - APD_PW / 2 + APD_PW;
    int y0 = APD_PX(p->y) - APD_PH / 2 - 1, y1 = APD_PX(p->y) - APD_PH / 2 + APD_PH;
    for (int k = 0; k <= 4; k++) {
        int y = y0 + (y1 - y0) * k / 4;
        if (apd_tile_at(x0, y) == t || apd_tile_at(x1, y) == t) return true;
    }
    for (int k = 1; k < 4; k++) {
        int x = x0 + (x1 - x0) * k / 4;
        if (apd_tile_at(x, y0) == t || apd_tile_at(x, y1) == t) return true;
    }
    return false;
}

static bool box_in(int t) {
    ApdPlayer *p = &ag.pl;
    int x0 = APD_PX(p->x) - APD_PW / 2 + 1, x1 = x0 + APD_PW - 2;
    int y0 = APD_PX(p->y) - APD_PH / 2 + 1, y1 = y0 + APD_PH - 2;
    for (int y = y0; y <= y1; y += (y1 - y0) / 3)
        for (int x = x0; x <= x1; x += (x1 - x0))
            if (apd_tile_at(x, y) == t) return true;
    return false;
}

void apd_hazards_update(void) {
    ApdPlayer *p = &ag.pl;
    const ApdSpec *sp = &APD_SPECS[apd_rooms[ag.room].spec];
    if (box_touches(T_SPUP) || box_touches(T_SPDN)) apd_player_hurt(APD_SPIKE_DMG, 100, true);
    if (box_in(T_ACID)) {
        if (++p->acid_t >= 20) { p->acid_t = 0; drain(5); apd_sfx("apd_sizzle"); }
    } else p->acid_t = 0;
    if ((sp->flags & RF_HEAT) && !apd_has(MOD_EMBER, SYS_FRAME)) {
        if (++p->heat_t >= 6) { p->heat_t = 0; drain(1); }
    }
    if (box_in(T_ZAP) && p->thunder_t <= 0 && p->inv <= 0) {
        apd_player_hurt(30, 101, false);
        p->vx = 0;
        /* thrown back the way he came */
        int push = APD_PX(p->x) % APD_CPW < APD_CPW / 2 ? 1 : -1;
        for (int k = 0; k < 12; k++)
            if (!solid_at(p->x + push * APD_FX, p->y, 0)) p->x += push * APD_FX;
    }
    /* gravity balls: red while gravity points down, purple while it points up */
    for (int i = 0; i < ag.nth; i++) {
        const ApdThing *t = &ag.th[i];
        if (!t->alive || t->kind != TH_BALL) continue;
        bool live = t->var == 0 ? p->grav > 0 : p->grav < 0;
        if (!live) continue;
        int bx = t->tx * APD_TILE + 8, by = t->ty * APD_TILE + 8;
        if (iabs(bx - APD_PX(p->x)) < 5 + APD_PW / 2 && iabs(by - APD_PX(p->y)) < 5 + APD_PH / 2)
            apd_player_hurt(APD_SPIKE_DMG, 102, true);
    }
}

/* ---- things -------------------------------------------------------------------- */

void apd_things_update(void) {
    ApdPlayer *p = &ag.pl;
    pads_update();
    int px = APD_PX(p->x), py = APD_PX(p->y);
    int zone = -1;
    for (int i = 0; i < ag.nth; i++) {
        ApdThing *t = &ag.th[i];
        if (!t->alive) continue;
        if (t->t > 0) t->t--;
        int tx = t->tx * APD_TILE + 8, ty = t->ty * APD_TILE + 8;
        int dx = iabs(px - tx), dy = iabs(py - ty);
        switch (t->kind) {
        case TH_ITEM:
            if (dx < 12 && dy < 16) {
                t->alive = 0;
                apd_pickup(t->id);
            }
            break;
        case TH_POD:
            if (dx < 26 && dy < 24) {
                zone = t->id;
                if (apd_btnp(BTN_UP) && p->ground) apd_enter_pod();
            }
            break;
        case TH_TERM:
            if (dx < 14 && dy < 20 && apd_btnp(BTN_UP) && p->ground) {
                if (t->var >= 1 && t->var <= 3) {
                    if (ag.run.pass < t->var) {
                        ag.run.pass = t->var;
                        apd_sfx("apd_unlock");
                    }
                    apd_read_terminal(100 + t->var);
                } else apd_read_terminal(t->id);
            }
            break;
        case TH_TELE:
            if (dx < 14 && dy < 20 && apd_btnp(BTN_UP) && p->ground) {
                if (t->id == 9 && ag.room != apd_room_find(SEC_CROWN, 'T') && ag.run.ciphers != 15) {
                    apd_say("FOUR CIPHERS NEEDED");
                    apd_sfx("apd_deny");
                } else apd_start_teleport(t->id);
            }
            break;
        default: break;
        }
    }
    /* walking up to a cradle saves and fills the armor and the spare bodies */
    if (zone >= 0 && zone != ag.pod_zone) apd_pod_save(zone);
    ag.pod_zone = zone;
}

/* ---- the camera and leaving the room ---------------------------------------------- */

void apd_camera(bool snap) {
    int tx = iclamp(APD_PX(ag.pl.x) - SCREEN_W / 2, 0, ag.rw * APD_TILE - SCREEN_W);
    int ty = iclamp(APD_PX(ag.pl.y) - APD_CPH / 2, 0, ag.rh * APD_TILE - APD_CPH);
    (void)snap;
    ag.cam_x = tx;
    ag.cam_y = ty;
}

static void enter_room(int nr, int x, int y) {
    ApdPlayer *p = &ag.pl;
    apd_build_room(nr);
    p->x = x;
    p->y = y;
    p->entry_x = x;
    p->entry_y = y;
    p->entry_grav = p->grav;
    p->entry_face = p->face;
    ag.run.visited[nr] = 1;
    ag.pod_zone = -1;
    /* the door he came through stands open */
    int px = APD_PX(x), py = APD_PX(y);
    for (int ty = py / APD_TILE - 2; ty <= py / APD_TILE + 2; ty++)
        for (int tx = px / APD_TILE - 2; tx <= px / APD_TILE + 2; tx++)
            if (tx >= 0 && ty >= 0 && tx < ag.rw && ty < ag.rh && ag.tdoor[ty][tx] &&
                (ag.tile[ty][tx] == T_DOOR || ag.tile[ty][tx] == T_LOCK))
                ag.tile[ty][tx] = T_AIR;
    apd_camera(true);
    extern void apd_room_music(void);
    apd_room_music();
}

void apd_room_change_check(void) {
    ApdPlayer *p = &ag.pl;
    const ApdRoom *r = &apd_rooms[ag.room];
    int px = APD_PX(p->x), py = APD_PX(p->y);
    int W = ag.rw * APD_TILE, H = ag.rh * APD_TILE;
    int dx = 0, dy = 0;
    if (px < 0) dx = -1;
    else if (px >= W) dx = 1;
    else if (py < 0) dy = -1;
    else if (py >= H) dy = 1;
    else return;
    int cx = r->gx + iclamp(px, 0, W - 1) / APD_CPW, cy = r->gy + iclamp(py, 0, H - 1) / APD_CPH;
    int nx = cx + dx, ny = cy + dy;
    if (nx < 0 || ny < 0 || nx >= APD_GW || ny >= APD_GH) return;
    int nr = apd_cell[r->sector][ny][nx];
    if (nr < 0) return;
    const ApdRoom *n = &apd_rooms[nr];
    int x, y;
    if (dx) {
        x = dx > 0 ? (APD_PW / 2 + 1) : n->w * APD_CPW - APD_PW / 2 - 2;
        y = (ny - n->gy) * APD_CPH + py % APD_CPH;
        x = x * APD_FX;
        y = y * APD_FX;
    } else {
        x = ((nx - n->gx) * APD_CPW + px % APD_CPW) * APD_FX;
        y = (dy > 0 ? APD_PH / 2 + 1 : n->h * APD_CPH - APD_PH / 2 - 2) * APD_FX;
    }
    enter_room(nr, x, y);
}

/* a spare body takes over at the room's entrance, as the room is now */
void apd_respawn_here(void) {
    ApdPlayer *p = &ag.pl;
    p->x = p->entry_x;
    p->y = p->entry_y;
    p->grav = p->entry_grav;
    p->face = p->entry_face;
    p->vx = p->vy = 0;
    p->ground = false;
    p->stomp = false;
    p->jumping = false;
    p->thunder_t = 0;
    p->air_flips = 0;
    p->inv = APD_MERCY;
    p->armor = apd_max_armor();
    ag.shadow.on = false;
    memset(ag.shot, 0, sizeof ag.shot);
    memset(ag.es, 0, sizeof ag.es);
    apd_camera(true);
}

/* ---- the pick-ups ---------------------------------------------------------------- */

void apd_pickup(int item) {
    if (item < 0 || item >= apd_nitems || ag.run.item_got[item]) return;
    ag.run.item_got[item] = 1;
    const ApdItemDef *it = &apd_items[item];
    char m[40];
    switch (it->kind) {
    case IT_ARMOR:
        ag.run.cells++;
        ag.pl.armor = apd_max_armor();
        snprintf(m, sizeof m, "ARMOR CELL: ARMOR %d", apd_max_armor());
        break;
    case IT_SPARE:
        ag.run.spares_max++;
        ag.spares = ag.run.spares_max;
        snprintf(m, sizeof m, "SPARE BODY: %d SPARES", ag.run.spares_max);
        break;
    case IT_DAMPER:
        ag.run.dampers++;
        snprintf(m, sizeof m, "DAMPER: %d MODS A SYSTEM", apd_safe_slots());
        break;
    default: {
        int mod = it->var;
        /* a new mod goes into the first empty system (GUN, FRAME, FLIP); with
         * none empty, into the GUN */
        int sys = SYS_GUN;
        static const int ORDER[3] = {SYS_GUN, SYS_FRAME, SYS_FLIP};
        for (int k = 2; k >= 0; k--)
            if (apd_sys_count(ORDER[k]) == 0) sys = ORDER[k];
        ag.run.mod_sys[mod] = (uint8_t)sys;
        snprintf(m, sizeof m, "%s MOD", APD_MOD_NAME[mod]);
        if (!apd_simulating && !ag.run.code_on) {
            game_award(GOAL_BEACON); /* the gift: get a mod */
            ags.gift = 1;
        }
        break;
    }
    }
    apd_say(m);
    apd_sfx("apd_item");
    if (!apd_simulating) music_play(APD_MUS_ITEM);
    ag.msg_t = 150;
}
