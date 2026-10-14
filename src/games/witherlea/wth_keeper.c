/* WITHERLEA - the Keeper. He comes by himself when enough time has gone by
 * (longer after every escape), or 2.5 s after a noise (6.5 s after a wrong
 * turn in the hedges). That countdown stops while the rooms change and while
 * you hold something. In your room he walks up to you; leave and he follows
 * a room behind; four rooms in a row and he loses you. Hide (the breathing
 * game: eight marks, three misses and he finds you), or strike him with the
 * knife or the poker (each works twice; the third blow with the same one he
 * blocks). Caught, you wake in the cold store with everything kept. */
#include "wth.h"

int wth_rand(int lo, int hi) { return rng_range(&wth.rng, lo, hi); }

void wth_keeper_reset(void) {
    memset(&wth.k, 0, sizeof wth.k);
    wth.k.hide_spot = -1;
}

bool wth_scared(void) {
    int s = wth.k.state;
    return s == E_HERE || s == E_FOLLOW || s == E_STUN;
}

bool wth_keeper_visible(void) {
    int s = wth.k.state;
    return (s == E_HERE || s == E_STUN || s == E_CAUGHT) && wth.k.room == wth.s.room;
}

/* he grows as he comes closer: 0 at the door, 256 at your face */
static int closeness(void) {
    if (wth.k.state == E_STUN || wth.k.state == E_CAUGHT) return 256;
    int c = (WTH_CATCH_T - wth.k.t) * 256 / WTH_CATCH_T;
    return iclamp(c, 0, 256);
}

void wth_keeper_rect(int *x, int *y, int *w, int *h) {
    int c = closeness();
    int hh = 56 + c * 60 / 256; /* 56 .. 116 px tall */
    int ww = hh * 2 / 5;
    *w = ww;
    *h = hh;
    *x = 160 - ww / 2;
    *y = 128 - hh;
    if (wth.k.state == E_STUN) { *y += hh / 3; *h -= hh / 3; }
}

static void arrive(void) {
    WthKeeper *k = &wth.k;
    k->state = E_HERE;
    k->t = WTH_CATCH_T;
    k->room = wth.s.room;
    k->chain = 0;
    k->arrive = 0;
    music_play(WTH_MUS_CHASE);
    sfx_play_name("wth_sting");
    gfx_set_flash(3);
}

void wth_escaped(void) {
    WthKeeper *k = &wth.k;
    int spot = k->state == E_HIDING ? k->hide_spot : -1;
    k->state = E_NONE;
    k->hide_spot = -1;
    if (spot >= 0) {
        int f = wth_hide_one_use_flag(spot);
        if (f >= 0) wth_set(f);
    }
    if (wth.s.escapes < 65535) wth.s.escapes++;
    wth.s.interval = imin(wth.s.interval + WTH_WAIT_STEP, WTH_WAIT_MAX);
    wth.s.spawn_left = wth.s.interval;
    if (!wth_flag(F_GIFT)) {
        wth_set(F_GIFT);
        wth_award(GOAL_BEACON);
    }
    wth_room_music();
    wth_save_now();
}

void wth_caught_now(void) {
    WthKeeper *k = &wth.k;
    if (k->state == E_HIDING && k->hide_spot >= 0) {
        int f = wth_hide_one_use_flag(k->hide_spot);
        if (f >= 0) wth_set(f);
    }
    k->state = E_CAUGHT;
    k->t = 0;
    k->room = wth.s.room;
    k->hide_spot = -1;
    wth.held = 0;
    wth.msg_n = wth.msg_page = 0;
    wth_set(F_EVER_CAUGHT);
    if (wth.s.caught < 65535) wth.s.caught++;
    music_play(WTH_MUS_CAUGHT);
    sfx_play_name("wth_swing");
    wth_save_now();
}

void wth_keeper_step(bool trans) {
    WthKeeper *k = &wth.k;
    if (wth_flag(F_KEEPER_DEAD)) {
        if (k->state != E_NONE) k->state = E_NONE;
        return;
    }
    switch (k->state) {
    case E_NONE:
        if (!trans && wth_flag(F_LEFT_CELL) && wth.s.spawn_left > 0 && --wth.s.spawn_left == 0) {
            k->state = E_COMING;
            k->t = WTH_NOISE_T;
            k->noise_maze = 0;
        }
        break;
    case E_COMING:
        /* the countdown stops between rooms and while you hold something */
        if (!trans && !wth.held && --k->t <= 0) arrive();
        break;
    case E_HERE:
        if (k->arrive < 10000) k->arrive++;
        if (!trans && --k->t <= 0) wth_caught_now();
        break;
    case E_FOLLOW:
        if (!trans && --k->t <= 0) arrive();
        break;
    case E_STUN:
        if (!trans && --k->t <= 0) {
            k->state = E_HERE;
            k->t = WTH_CATCH_T;
            wth_say("HE'S GETTING UP.");
        }
        break;
    case E_CAUGHT:
        if (++k->t >= WTH_CAUGHT_T) wth_goto(SC_WAKE);
        break;
    default: break;
    }
}

void wth_keeper_room_changed(int from, int to) {
    WthKeeper *k = &wth.k;
    (void)to;
    switch (k->state) {
    case E_HERE:
        /* he stays in the room you left, one step behind you */
        if (from == k->room) {
            k->state = E_FOLLOW;
            k->t = WTH_FOLLOW_T;
            k->chain = 1;
        }
        break;
    case E_STUN:
        if (from == k->room) {
            wth_escaped();
            wth_say("YOU LEAVE HIM ON HIS KNEES. BEHIND YOU HE BELLOWS, BUT HE DOESN'T FOLLOW.");
        }
        break;
    case E_FOLLOW:
        if (to == k->room) {
            /* doubled back, straight into him */
            k->state = E_HERE;
            k->t = WTH_CATCH_T / 2;
            k->chain = 0;
            k->arrive = 0;
            sfx_play_name("wth_sting");
            break;
        }
        k->room = (uint8_t)from; /* he comes on into the room you just left */
        k->chain++;
        if (k->chain >= WTH_OUTRUN) {
            wth_escaped();
            wth_say("HIS FOOTSTEPS SLOW, AND STOP, AND GO ANOTHER WAY. YOU'VE LOST HIM. FOR NOW.");
        } else k->t = WTH_FOLLOW_T;
        break;
    default: break;
    }
}

static void floor_drop(int room, int it) {
    for (int i = 0; i < WTH_FLOOR; i++)
        if (!wth.s.floor[room][i]) { wth.s.floor[room][i] = (uint8_t)it; return; }
}

void wth_keeper_click(int held) {
    WthKeeper *k = &wth.k;
    if (k->state != E_HERE && k->state != E_STUN) return;
    if (held == I_CROSSBOW) {
        if (!wth_flag(F_BOW_LOADED)) { wth_say("THE CROSSBOW ISN'T LOADED!"); return; }
        wth_set(F_KEEPER_DEAD);
        wth_clear(F_BOW_LOADED);
        wth_take_away(I_CROSSBOW);
        floor_drop(wth.s.room, I_BLACKKEY);
        wth.s.body_room = wth.s.room;
        k->state = E_NONE;
        sfx_play_name("wth_shot");
        gfx_set_flash(4);
        wth_room_music();
        wth_say("THE BOLT TAKES HIM IN THE CHEST. HE SITS DOWN SLOWLY, LIKE A TIRED MAN, AND THE VEIL FALLS FORWARD. HE "
                "DOESN'T MOVE AGAIN. SOMETHING CLATTERS OUT OF HIS APRON POCKET ONTO THE FLOOR.");
        wth_save_now();
        return;
    }
    if (held == I_KNIFE || held == I_POKER) {
        if (k->state == E_STUN) { wth_say("HE'S DOWN. DON'T WAIT FOR HIM TO GET UP. GO!"); return; }
        uint8_t *n = held == I_KNIFE ? &wth.s.stun_knife : &wth.s.stun_poker;
        if (*n >= WTH_STUNS) {
            sfx_play_name("wth_block");
            wth_say("HE'S READY FOR IT THIS TIME. THE BILLHOOK'S HAFT CATCHES YOUR BLOW, AND THE BLADE COMES ROUND.");
            wth_caught_now();
            return;
        }
        (*n)++;
        if (wth.s.stuns < 65535) wth.s.stuns++;
        k->state = E_STUN;
        k->t = WTH_STUN_T;
        sfx_play_name("wth_hit");
        gfx_set_flash(2);
        wth_say(held == I_KNIFE ? "YOU STAB AT HIM. HE STAGGERS BACK AND DROPS TO ONE KNEE, HUGGING HIS ARM. GO, NOW!"
                                : "YOU SWING THE POKER INTO HIS KNEE. HE GOES DOWN WITH A ROAR. GO, NOW!");
        return;
    }
    if (held) { wth_say("THAT WON'T STOP HIM!"); return; }
    wth_say("HE'S RIGHT THERE! GET OUT, OR HIDE!");
}

/* ------------------------------------------------------------------ */
/* the breathing game                                                   */

static const int16_t MINI_SPEED[WTH_BREATHS] = {256, 300, 352, 416, 480, 560, 640, 736}; /* 1/256 px a frame */
static const int16_t MINI_ZONE[WTH_BREATHS] = {28, 24, 20, 17, 14, 12, 10, 8};

static void mini_prompt(void) {
    WthMini *m = &wth.k.mini;
    int i = imin(m->n, WTH_BREATHS - 1);
    m->zw = MINI_ZONE[i];
    m->zone = (int16_t)wth_rand(30, WTH_BAR_W - m->zw - 4);
    m->pos = 0;
    m->vel = MINI_SPEED[i];
    m->passes = 0;
}

void wth_hide_begin(int spot) {
    WthKeeper *k = &wth.k;
    k->state = E_HIDING;
    k->hide_spot = (int8_t)spot;
    k->room = wth.s.room;
    memset(&k->mini, 0, sizeof k->mini);
    k->mini.pause = 60;
    mini_prompt();
    wth.held = 0;
    music_play(WTH_MUS_HIDE);
    sfx_play_name("wth_hide");
}

static void mini_next(void) {
    WthMini *m = &wth.k.mini;
    m->n++;
    if (m->miss >= WTH_MISSES) {
        sfx_play_name("wth_found");
        wth_caught_now();
        return;
    }
    if (m->n >= WTH_BREATHS) {
        wth_escaped();
        wth_say("HE STANDS THERE A LONG TIME, BREATHING THROUGH THE VEIL. THEN THE FLOORBOARDS CREAK AWAY. YOU LET "
                "YOUR BREATH GO.");
        return;
    }
    m->pause = 24;
    mini_prompt();
}

void wth_mini_step(bool press) {
    WthMini *m = &wth.k.mini;
    if (m->flash > 0) m->flash--;
    if (m->pause > 0) { m->pause--; return; }
    m->pos += m->vel;
    int lim = WTH_BAR_W * 256;
    if (m->pos >= lim) { m->pos = 2 * lim - m->pos; m->vel = -m->vel; m->passes++; }
    if (m->pos < 0) { m->pos = -m->pos; m->vel = -m->vel; m->passes++; }
    if (press) {
        int x = (int)(m->pos >> 8);
        bool ok = x >= m->zone && x < m->zone + m->zw;
        if (ok) { m->hit++; sfx_play_name("wth_breath"); }
        else { m->miss++; sfx_play_name("wth_gasp"); }
        m->flash = 20;
        m->flash_ok = ok;
        mini_next();
        return;
    }
    if (m->passes >= 2) { /* across and back with no press */
        m->miss++;
        m->flash = 20;
        m->flash_ok = 0;
        sfx_play_name("wth_gasp");
        mini_next();
    }
}
