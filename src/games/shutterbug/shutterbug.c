/* SHUTTERBUG - Poppy's picture-perfect trip across the stars.
 * Cartridge 24 of UFO 40, a tribute to Caramel Caramel (UFO 50 #24).
 * See docs/games/24-shutterbug.md. This file: the title, the story, the
 * screens between stages, the endings, the save and the test hooks. The
 * play itself is in shutterbug_play.c. */
#include "shutterbug.h"

#define SAVE_MAGIC 0x53484201u

static void set_state(int s) {
    sb.state = s;
    sb.state_t = 0;
}

static void load_save(void) {
    ShbSave tmp;
    memset(&tmp, 0, sizeof tmp);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) { sbs = tmp; return; }
    memset(&sbs, 0, sizeof sbs);
    sbs.magic = SAVE_MAGIC;
}

static int popcount32(uint32_t v) {
    int n = 0;
    while (v) { n += (int)(v & 1); v >>= 1; }
    return n;
}

/* the run is over, won or lost: keep its records */
static void run_over(void) {
    if (sb.best > sbs.best) sbs.best = sb.best;
    int types = popcount32(sb.kinds) + popcount32(sb.kinds2);
    if (types > sbs.most_types) sbs.most_types = (uint8_t)types;
    if (sb.secrets > sbs.most_secrets) sbs.most_secrets = (uint8_t)imin(255, sb.secrets);
    if (sb.stage > sbs.best_stage) sbs.best_stage = (uint8_t)sb.stage;
    sbs.kinds_ever |= sb.kinds;
    sbs.kinds_ever2 |= sb.kinds2;
    if (sb.won && sbs.wins < 65535) sbs.wins++;
    if (sb.true_won && sbs.trues < 65535) sbs.trues++;
    shb_save_now();
}

static void to_title(void) {
    set_state(SS_TITLE);
    game_set_pausable(false);
    input_set_versus(false);
    music_play(SHB_MUS_TITLE);
}

static void start_ending(bool true_end) {
    sb.won = true;
    sb.true_won = true_end;
    game_award(GOAL_SAUCER);
    if (true_end) game_award(GOAL_ALIEN);
    set_state(SS_ENDING);
    game_set_pausable(false);
    music_play(SHB_MUS_ENDING);
}

/* after a stage's clear screen: on to the next, or an ending */
static void next_stage(void) {
    if (sb.stage == 5) {
        if (sb.letters == 7) shb_start_stage(6); /* U, F and O: the true boss waits */
        else start_ending(false);
    } else if (sb.stage == 6) {
        start_ending(true);
    } else {
        shb_start_stage(sb.stage + 1);
    }
}

static void shb_update(void) {
    sb.state_t++;
    switch (sb.state) {
    case SS_TITLE:
        sb.frame_t++;
        game_set_pausable(false);
        if (btnp(BTN_UP) || btnp(BTN_DOWN)) { sb.menu ^= 1; sfx_play_name("ui_move"); }
        if (btnp(BTN_B)) { game_exit_to_library(); break; }
        if ((btnp(BTN_A) || btnp(BTN_START)) && sb.state_t > 6) {
            sfx_play_name("ui_ok");
            sb.players = sb.menu + 1;
            input_consume();
            set_state(SS_STORY);
            music_play(SHB_MUS_STORY);
        }
        break;
    case SS_STORY:
        sb.frame_t++;
        if (sb.state_t > 640 || ((btnp(BTN_A) || btnp(BTN_START)) && sb.state_t > 10)) {
            input_consume();
            game_set_pausable(true);
            shb_new_run(sb.players);
        }
        break;
    case SS_BANNER:
        sb.frame_t++;
        game_set_pausable(true);
        if (sb.state_t >= 100) set_state(SS_PLAY);
        break;
    case SS_PLAY: {
        game_set_pausable(true);
        shb_play_update();
        if (sb.state != SS_PLAY) break;
        /* the boss tune while a big foe is out */
        bool big = false;
        for (int i = 0; i < SHB_MAX_FOES; i++)
            if (sb.foe[i].alive && (sb.foe[i].role == ROLE_MID || sb.foe[i].role == ROLE_BOSS) && sb.foe[i].t >= 0) big = true;
        if (sb.stage != 6 && !sb.boss_dead) music_play(big ? SHB_MUS_BOSS : SHB_MUS_STAGE[sb.stage]);
        break;
    }
    case SS_LOST:
        sb.frame_t++;
        game_set_pausable(false);
        if (sb.state_t > 180 || (sb.state_t > 60 && btnp(BTN_A))) {
            input_consume();
            sb.lives--;
            sb.score = 0; /* the shown score starts again with the stage */
            shb_start_stage(sb.stage);
        }
        break;
    case SS_OVER:
        sb.frame_t++;
        game_set_pausable(false);
        if (sb.state_t > 240 || (sb.state_t > 60 && btnp(BTN_A))) { input_consume(); run_over(); to_title(); }
        break;
    case SS_CLEAR:
        sb.frame_t++;
        game_set_pausable(false);
        if (sb.state_t > 220 || (sb.state_t > 60 && btnp(BTN_A))) { input_consume(); next_stage(); }
        break;
    case SS_ENDING:
        sb.frame_t++;
        if (sb.state_t > 900 || (sb.state_t > 320 && btnp(BTN_A))) {
            input_consume();
            set_state(SS_CREDITS);
            music_play(SHB_MUS_CREDITS);
        }
        break;
    case SS_CREDITS:
        sb.frame_t++;
        if (btn(BTN_A)) sb.state_t += 3; /* hold A to hurry them along */
        if (sb.state_t > 1600) { input_consume(); run_over(); to_title(); }
        break;
    }
}

/* ------------------------------------------------------------------------ */
/* cartridge interface                                                      */

static void shb_load(void) {
    shb_art_load();
    shb_audio_load();
}

static void shb_start(void) {
    load_save();
    memset(&sb, 0, sizeof sb);
    sb.players = 1;
    to_title();
}

static void shb_quit(void) {
    input_set_versus(false);
    shb_save_now();
}

static int count_foes(int kind) {
    int n = 0;
    for (int i = 0; i < SHB_MAX_FOES; i++)
        if (sb.foe[i].alive && sb.foe[i].t >= 0 && (kind < 0 ? sb.foe[i].role != ROLE_PROP : sb.foe[i].kind == kind)) n++;
    return n;
}

static int first_of(int kind) {
    for (int i = 0; i < SHB_MAX_FOES; i++)
        if (sb.foe[i].alive && sb.foe[i].kind == kind) return i;
    return -1;
}

static int shb_query(const char *key, int *out) {
    const Ship *s0 = &sb.ship[0], *s1 = &sb.ship[1];
    if (!strcmp(key, "bot")) { *out = shb_bot_buttons(); return 1; }
    if (!strcmp(key, "state")) { *out = sb.state; return 1; }
    if (!strcmp(key, "menu")) { *out = sb.menu; return 1; }
    if (!strcmp(key, "players")) { *out = sb.players; return 1; }
    if (!strcmp(key, "stage")) { *out = sb.stage; return 1; }
    if (!strcmp(key, "score")) { *out = (int)sb.score; return 1; }
    if (!strcmp(key, "best")) { *out = (int)sb.best; return 1; }
    if (!strcmp(key, "total")) { *out = (int)sb.total; return 1; }
    if (!strcmp(key, "lives")) { *out = sb.lives; return 1; }
    if (!strcmp(key, "extends")) { *out = sb.extends; return 1; }
    if (!strcmp(key, "next_extend")) { *out = (int)shb_next_extend(); return 1; }
    if (!strcmp(key, "deaths")) { *out = sb.deaths; return 1; }
    if (!strcmp(key, "letters")) { *out = sb.letters; return 1; }
    if (!strcmp(key, "secrets")) { *out = sb.secrets; return 1; }
    if (!strcmp(key, "types")) { *out = popcount32(sb.kinds) + popcount32(sb.kinds2); return 1; }
    if (!strcmp(key, "won")) { *out = sb.won; return 1; }
    if (!strcmp(key, "true_won")) { *out = sb.true_won; return 1; }
    if (!strcmp(key, "zero_prologue")) { *out = sb.zero_prologue; return 1; }
    if (!strcmp(key, "tip")) { *out = sb.tip; return 1; }
    if (!strcmp(key, "cam_x")) { *out = (int)sb.cam_x; return 1; }
    if (!strcmp(key, "cam_y")) { *out = (int)sb.cam_y; return 1; }
    if (!strcmp(key, "hold")) { *out = sb.hold; return 1; }
    if (!strcmp(key, "boss_dead")) { *out = sb.boss_dead; return 1; }
    if (!strcmp(key, "stage_t")) { *out = sb.stage_t; return 1; }
    if (!strcmp(key, "photos")) { *out = sb.photos; return 1; }
    if (!strcmp(key, "rings_thrown")) { *out = sb.rings_thrown; return 1; }
    if (!strcmp(key, "crystals_got")) { *out = sb.crystals_got; return 1; }
    if (!strcmp(key, "wrenches")) { *out = sb.wrenches; return 1; }
    if (!strcmp(key, "retaliations")) { *out = sb.retaliations; return 1; }
    if (!strcmp(key, "big_kills")) { *out = sb.big_kills; return 1; }
    if (!strcmp(key, "orbs_shot")) { *out = sb.orbs_shot; return 1; }
    /* the ships */
    if (!strcmp(key, "alive")) { *out = s0->alive; return 1; }
    if (!strcmp(key, "alive2")) { *out = s1->alive; return 1; }
    if (!strcmp(key, "on2")) { *out = s1->on; return 1; }
    if (!strcmp(key, "armour")) { *out = s0->armour; return 1; }
    if (!strcmp(key, "armour2")) { *out = s1->armour; return 1; }
    if (!strcmp(key, "inv")) { *out = s0->inv; return 1; }
    if (!strcmp(key, "flash")) { *out = s0->flash; return 1; }
    if (!strcmp(key, "flash2")) { *out = s1->flash; return 1; }
    if (!strcmp(key, "hold_t")) { *out = s0->hold_t; return 1; }
    if (!strcmp(key, "firing")) { *out = s0->firing; return 1; }
    if (!strcmp(key, "px")) { *out = (int)lroundf(s0->x - sb.cam_x); return 1; }
    if (!strcmp(key, "py")) { *out = (int)lroundf(s0->y - sb.cam_y); return 1; }
    if (!strcmp(key, "wy")) { *out = (int)lroundf(s0->y); return 1; }
    if (!strcmp(key, "px2")) { *out = (int)lroundf(s1->x - sb.cam_x); return 1; }
    if (!strcmp(key, "py2")) { *out = (int)lroundf(s1->y - sb.cam_y); return 1; }
    /* the world */
    if (!strcmp(key, "foes")) { *out = count_foes(-1); return 1; }
    if (!strncmp(key, "foes_", 5)) { *out = count_foes(atoi(key + 5)); return 1; }
    if (!strcmp(key, "stunned")) {
        int n = 0;
        for (int i = 0; i < SHB_MAX_FOES; i++) n += sb.foe[i].alive && (sb.foe[i].stun > 0 || sb.foe[i].falling);
        *out = n;
        return 1;
    }
    if (!strncmp(key, "hp_", 3)) { int i = first_of(atoi(key + 3)); *out = i >= 0 ? sb.foe[i].hp : -1; return 1; }
    if (!strncmp(key, "stun_", 5)) { int i = first_of(atoi(key + 5)); *out = i >= 0 ? sb.foe[i].stun : -1; return 1; }
    if (!strncmp(key, "open_", 5)) { int i = first_of(atoi(key + 5)); *out = i >= 0 ? sb.foe[i].open : -1; return 1; }
    if (!strncmp(key, "fstate_", 7)) { int i = first_of(atoi(key + 7)); *out = i >= 0 ? sb.foe[i].state : -1; return 1; }
    if (!strncmp(key, "fx_", 3)) { int i = first_of(atoi(key + 3)); *out = i >= 0 ? (int)lroundf(sb.foe[i].x - sb.cam_x) : -999; return 1; }
    if (!strncmp(key, "fy_", 3)) { int i = first_of(atoi(key + 3)); *out = i >= 0 ? (int)lroundf(sb.foe[i].y - sb.cam_y) : -999; return 1; }
    if (!strcmp(key, "eshots")) { int n = 0; for (int i = 0; i < SHB_MAX_ESHOTS; i++) n += sb.es[i].alive; *out = n; return 1; }
    if (!strncmp(key, "eshots_", 7)) {
        int k = atoi(key + 7), n = 0;
        for (int i = 0; i < SHB_MAX_ESHOTS; i++) n += sb.es[i].alive && sb.es[i].kind == k;
        *out = n;
        return 1;
    }
    if (!strncmp(key, "pshots_", 7)) {
        int k = atoi(key + 7), n = 0;
        for (int i = 0; i < SHB_MAX_PSHOTS; i++) n += sb.ps[i].alive && sb.ps[i].kind == k;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "arrows_up")) { int n = 0; for (int i = 0; i < SHB_MAX_ESHOTS; i++) n += sb.es[i].alive && sb.es[i].kind == ES_ARROW && sb.es[i].arg == 1; *out = n; return 1; }
    if (!strcmp(key, "crumbs_bounced")) { int n = 0; for (int i = 0; i < SHB_MAX_ESHOTS; i++) n += sb.es[i].alive && sb.es[i].kind == ES_CRUMB && sb.es[i].bounces < 3; *out = n; return 1; }
    if (!strcmp(key, "ring_bounced")) {
        int n = 0;
        for (int i = 0; i < SHB_MAX_PSHOTS; i++) n += sb.ps[i].alive && sb.ps[i].kind == PS_RING && sb.ps[i].bounces < SHB_RING_BOUNCES;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "ring_back")) {
        /* a ring flying back to the left: it bounced off rock */
        int n = 0;
        for (int i = 0; i < SHB_MAX_PSHOTS; i++) n += sb.ps[i].alive && sb.ps[i].kind == PS_RING && sb.ps[i].vx < 0;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "crystals")) { int n = 0; for (int i = 0; i < SHB_MAX_PICKUPS; i++) n += sb.pk[i].alive && sb.pk[i].kind == PK_CRYSTAL; *out = n; return 1; }
    if (!strcmp(key, "wrench_out")) { int n = 0; for (int i = 0; i < SHB_MAX_PICKUPS; i++) n += sb.pk[i].alive && sb.pk[i].kind == PK_WRENCH; *out = n; return 1; }
    if (!strcmp(key, "crystal_dx")) {
        *out = 999;
        for (int i = 0; i < SHB_MAX_PICKUPS; i++)
            if (sb.pk[i].alive && sb.pk[i].kind == PK_CRYSTAL) { *out = (int)lroundf(sb.pk[i].x - s0->x); break; }
        return 1;
    }
    if (!strcmp(key, "tile_here")) { *out = shb_tile_at(s0->x, s0->y); return 1; }
    if (!strncmp(key, "spawn_count_", 12)) {
        /* how many of a kind every stage's list brings, all told */
        int k = atoi(key + 12), n = 0;
        for (int st = 0; st < SHB_STAGES; st++)
            for (int i = 0; i < SHB_STAGE[st].nspawns; i++)
                if (SHB_STAGE[st].spawns[i].kind == k) n += imax(1, SHB_STAGE[st].spawns[i].n);
        *out = n;
        return 1;
    }
    if (!strncmp(key, "red_in_", 7)) {
        /* red repair foes on one stage's list */
        int st = iclamp(atoi(key + 7), 0, SHB_STAGES - 1), n = 0;
        for (int i = 0; i < SHB_STAGE[st].nspawns; i++) n += (SHB_STAGE[st].spawns[i].flags & F_RED) != 0;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "kalei_phase")) { int i = first_of(K_KALEI); *out = i >= 0 ? sb.foe[i].phase : -1; return 1; }
    if (!strcmp(key, "falling")) { int n = 0; for (int i = 0; i < SHB_MAX_FOES; i++) n += sb.foe[i].alive && sb.foe[i].falling; *out = n; return 1; }
    if (!strcmp(key, "frozen")) { int n = 0; for (int i = 0; i < SHB_MAX_FOES; i++) n += sb.foe[i].alive && sb.foe[i].frozen; *out = n; return 1; }
    if (!strcmp(key, "bhp_min")) {
        /* the weakest crumbly rock on screen */
        int m = 99, c0 = (int)(sb.cam_x / SHB_TILE);
        for (int c = c0; c < c0 + 41 && c < shb_cols; c++)
            for (int r = 0; r < shb_rows; r++) if (shb_tile[c][r] == TL_BREAK && shb_bhp[c][r] < m) m = shb_bhp[c][r];
        *out = m;
        return 1;
    }
    if (!strcmp(key, "spawns_sorted")) {
        *out = 1;
        for (int s = 0; s < SHB_STAGES; s++)
            for (int i = 1; i < SHB_STAGE[s].nspawns; i++)
                if (SHB_STAGE[s].spawns[i].x < SHB_STAGE[s].spawns[i - 1].x) *out = 0;
        return 1;
    }
    /* the save */
    if (!strcmp(key, "save_best")) { *out = (int)sbs.best; return 1; }
    if (!strcmp(key, "save_runs")) { *out = sbs.runs; return 1; }
    if (!strcmp(key, "save_wins")) { *out = sbs.wins; return 1; }
    if (!strcmp(key, "save_trues")) { *out = sbs.trues; return 1; }
    if (!strcmp(key, "save_types")) { *out = sbs.most_types; return 1; }
    if (!strcmp(key, "save_secrets")) { *out = sbs.most_secrets; return 1; }
    if (!strcmp(key, "save_stage")) { *out = sbs.best_stage; return 1; }
    return 0;
}

static int shb_cheat(const char *cmd) {
    int a, b;
    float x, y;
    if (sscanf(cmd, "run %d", &a) == 1) { sb.players = iclamp(a, 1, 2); shb_new_run(sb.players); sb.state = SS_PLAY; game_set_pausable(true); return 1; }
    if (sscanf(cmd, "stage %d", &a) == 1) { shb_start_stage(iclamp(a, 0, SHB_STAGES - 1)); sb.state = SS_PLAY; return 1; }
    if (sscanf(cmd, "goto %d", &a) == 1) {
        /* jump the view along the stage, running every spawn on the way */
        const StageDef *st = &SHB_STAGE[sb.stage];
        sb.cam_x = (float)a;
        while (sb.scroll_i + 1 < st->nscroll && st->scroll[sb.scroll_i + 1].x <= a) sb.scroll_i++;
        sb.hold = HOLD_NONE;
        if (st->scroll[sb.scroll_i].x == a) sb.hold = st->scroll[sb.scroll_i].hold;
        while (sb.spawn_i < st->nspawns && st->spawns[sb.spawn_i].x < a + SCREEN_W - 40) sb.spawn_i++;
        sb.cam_y = shb_band_lo(sb.cam_x + SCREEN_W / 2);
        shb_clear_world();
        for (int p = 0; p < 2; p++) if (sb.ship[p].on) { sb.ship[p].x = sb.cam_x + 48; sb.ship[p].y = sb.cam_y + 84 + p * 20; }
        return 1;
    }
    if (!strcmp(cmd, "god")) { sb.god = !sb.god; return 1; }
    if (!strcmp(cmd, "quiet")) { sb.spawn_i = SHB_STAGE[sb.stage].nspawns; return 1; }
    if (!strcmp(cmd, "clear")) { shb_clear_world(); return 1; }
    if (!strcmp(cmd, "stop")) { sb.hold = HOLD_FOREVER; return 1; } /* the view stays put */
    if (!strcmp(cmd, "go")) { sb.hold = HOLD_NONE; return 1; }
    if (sscanf(cmd, "score %d", &a) == 1) { sb.score = (uint32_t)a; if (sb.score > sb.best) sb.best = sb.score; return 1; }
    if (sscanf(cmd, "addscore %d", &a) == 1) { shb_add_score((uint32_t)a); return 1; }
    if (sscanf(cmd, "lives %d", &a) == 1) { sb.lives = a; return 1; }
    if (sscanf(cmd, "letters %d", &a) == 1) { sb.letters = (uint8_t)a; return 1; }
    /* (the player 2 forms first: "pos %f" would read "pos2 ..." too) */
    if (sscanf(cmd, "flash2 %d", &a) == 1) { sb.ship[1].flash = a; return 1; }
    if (sscanf(cmd, "flash %d", &a) == 1) { sb.ship[0].flash = a; return 1; }
    if (sscanf(cmd, "pos2 %f %f", &x, &y) == 2) { sb.ship[1].x = sb.cam_x + x; sb.ship[1].y = sb.cam_y + y; return 1; }
    if (sscanf(cmd, "pos %f %f", &x, &y) == 2) { sb.ship[0].x = sb.cam_x + x; sb.ship[0].y = sb.cam_y + y; return 1; }
    if (sscanf(cmd, "foe %d %f %f %d", &a, &x, &y, &b) == 4) {
        /* a foe at a screen spot (flags b) */
        int i = shb_spawn(a, sb.cam_x + x, sb.cam_y + y, b, 0);
        (void)i;
        return 1;
    }
    if (!strncmp(cmd, "foea ", 5)) {
        /* a foe at a screen spot, with flags and an arg (a block's swing) */
        int c = 0;
        if (sscanf(cmd, "foea %d %f %f %d %d", &a, &x, &y, &b, &c) == 5) shb_spawn(a, sb.cam_x + x, sb.cam_y + y, b, c);
        return 1;
    }
    if (sscanf(cmd, "dummy %d %f %f %d", &a, &x, &y, &b) == 4) {
        /* a foe that is held still (it never moves or fires) */
        int i = shb_spawn(a, sb.cam_x + x, sb.cam_y + y, b, 0);
        if (i >= 0) sb.foe[i].still = true;
        return 1;
    }
    if (sscanf(cmd, "wake %d", &a) == 1) { for (int i = 0; i < SHB_MAX_FOES; i++) if (sb.foe[i].alive && sb.foe[i].kind == a) sb.foe[i].still = false; return 1; }
    if (sscanf(cmd, "hp %d %d", &a, &b) == 2) { for (int i = 0; i < SHB_MAX_FOES; i++) if (sb.foe[i].alive && sb.foe[i].kind == a) sb.foe[i].hp = b; return 1; }
    if (!strncmp(cmd, "eshot ", 6)) {
        float vx = 0, vy = 0;
        if (sscanf(cmd, "eshot %d %f %f %f %f", &a, &x, &y, &vx, &vy) >= 3) shb_add_eshot(a, sb.cam_x + x, sb.cam_y + y, vx, vy);
        return 1;
    }
    if (sscanf(cmd, "drop %d %f %f", &a, &x, &y) == 3) { shb_drop(a, sb.cam_x + x, sb.cam_y + y); return 1; }
    if (sscanf(cmd, "hurt %d", &a) == 1) { sb.ship[iclamp(a, 0, 1)].inv = 0; shb_hurt_ship(iclamp(a, 0, 1)); return 1; }
    if (sscanf(cmd, "letterbot %d", &a) == 1) { shb_bot_letters = a != 0; return 1; }
    if (!strcmp(cmd, "win")) { sb.stage = 5; sb.state = SS_CLEAR; sb.state_t = 200; return 1; }
    return 0;
}

const GameDef GAME_SHUTTERBUG = {
    "shutterbug",
    "SHUTTERBUG",
    "1986",
    "ARCADE SHOOTER",
    "A PHOTO FREEZES A FOE: IT PAYS DOUBLE AND GOES BANG.",
    {"BEAT THE TEAPOT", "BEAT KING THUNDERJAW", "SNAP U, F AND O; BEAT THE TRUE BOSS"},
    "D-PAD\tFLY\n"
    "HOLD " GLYPH_B "\tGUN, AND CHARGE THE RINGS\n"
    "LET GO " GLYPH_B "\tTHROW FOUR RINGS\n"
    "TAP " GLYPH_A "\tTAKE A PHOTO\n"
    "START\tPAUSE\n"
    "\n"
    "NOT FIRING? THE BULBS FLY TO YOU.",
    C_PINK, C_NAVY,
    shb_load, shb_start, shb_update, shb_draw, shb_quit, shb_draw_label, shb_query, shb_cheat,
    "CARAMEL CARAMEL", 24,
    NULL,
};
