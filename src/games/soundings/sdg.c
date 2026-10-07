/* SOUNDINGS - Moss, Reed and Skip dive from their raft into the dark.
 * Cartridge 27 of UFO 40, a tribute to Divers (UFO 50 #27).
 * See docs/games/27-soundings.md.
 *
 * This file is the screens (title, the raft and its three unlabelled
 * pictures, the shop, the kit, the dive, the item menu, the fight, the wipe,
 * surfacing and the ending), saving, the goals and the test hooks.
 *
 * The log is written only at the raft: when the divers surface, and when
 * something is bought or the kit is changed there. A wipe (or leaving the
 * cartridge mid-dive) goes back to it: every gold piece, relic, pearl, level,
 * chest and lever since the last surfacing is gone. */
#include "sdg.h"

#define P (sdg.prog)

/* ------------------------------------------------------------------ */
/* saving                                                               */

static bool save_exists(void) { return sdg.saved.magic == SDG_MAGIC && sdg.saved.started; }

static void write_save(void) {
    if (sdg.code_on) { sdg.saved = P; return; } /* a code is on: nothing is written */
    sdg.saved = P;
    sdg.saved.magic = SDG_MAGIC;
    sdg.saved.version = SDG_VERSION;
    game_save_write(game_current_index(), &sdg.saved, (int)sizeof sdg.saved);
}

static void load_save(void) {
    static SdgProg tmp;
    if (game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp && tmp.magic == SDG_MAGIC &&
        tmp.version == SDG_VERSION)
        sdg.saved = tmp;
    else memset(&sdg.saved, 0, sizeof sdg.saved);
    P = sdg.saved;
}

static void new_game(void) {
    memset(&P, 0, sizeof P);
    P.magic = SDG_MAGIC;
    P.version = SDG_VERSION;
    P.started = 1;
    P.level = 1;
    P.gold = 500;
    if (sdg.code_on) P.owned[IT_EGG] = 3;
    sdg_refill(&P);
    write_save();
}

static void award(int bit) {
    if (!sdg.code_on) game_award(bit);
}

/* ------------------------------------------------------------------ */
/* scenes                                                               */

void sdg_note(const char *a, const char *b) {
    sdg.note1 = a;
    sdg.note2 = b;
    sdg.note_t = 180;
}

static int dive_music_for(int region) {
    if (region == RG_DEEP) return SDG_MUS_DEEP;
    if (region == RG_STILL) return SDG_MUS_STILL;
    return -1;
}

static void dive_music(void) {
    int r = sdg_region_at((int)(sdg.dive.x >> 8) / SDG_T, (int)(sdg.dive.y >> 8) / SDG_T);
    int m = dive_music_for(r);
    if (m < 0) {
        int now = music_playing();
        if (now >= 0 && now != SDG_MUS_WIN && now != SDG_MUS_LEVEL) music_stop();
    } else if (music_playing() != m) music_play(m);
}

void sdg_goto(int scene) {
    sdg.scene = scene;
    sdg.scene_t = 0;
    input_consume();
    game_set_pausable(scene != SC_TITLE && scene != SC_CODE);
    switch (scene) {
    case SC_TITLE: music_play(SDG_MUS_TITLE); break;
    case SC_RAFT:
    case SC_SHOP:
    case SC_KIT: music_play(SDG_MUS_RAFT); break;
    case SC_DIVE: dive_music(); break;
    case SC_WIPE: music_play(SDG_MUS_WIPE); break;
    case SC_ENDING: music_play(SDG_MUS_END); break;
    default: break;
    }
}

void sdg_surface(void) {
    sdg.nsurf = 0;
    for (int r = 0; r < RL_COUNT; r++) {
        P.relic[r] = (uint8_t)imin(SDG_RELIC_MAX, P.relic[r] + P.carry[r]);
        P.carry[r] = 0;
    }
    for (int i = 0; i < P.npearl; i++) {
        int it = P.pearl[i];
        if (P.owned[it] < SDG_ITEM[it].max) P.owned[it]++;
        if (sdg.nsurf < 6) snprintf(sdg.surf[sdg.nsurf++], sizeof sdg.surf[0], "A PEARL OPENS: %s", SDG_ITEM[it].name);
        if (it == IT_LEECH && !P.gift) {
            P.gift = 1;
            award(GOAL_BEACON);
        }
    }
    P.npearl = 0;
    sdg_refill(&P);
    if (P.dives < 65535) P.dives++;
    write_save();
}

void sdg_wipe(void) {
    P = sdg.saved;
    sdg_refill(&P);
}

void sdg_win_final(void) {
    if (P.wins < 65535) P.wins++;
    bool all = (P.heads & 7) == 7;
    award(GOAL_SAUCER);
    if (all) {
        if (P.cherry_wins < 65535) P.cherry_wins++;
        award(GOAL_ALIEN);
    }
    sdg.ending = all ? 2 : 1;
    sdg.ending_t = 0;
    /* back at the raft afterwards, with everything the dive found */
    sdg_surface();
    sdg_goto(SC_ENDING);
}

/* ------------------------------------------------------------------ */
/* input                                                                */

static unsigned held_buttons(void) {
    unsigned h = 0;
    static const int B[6] = {BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_A, BTN_B};
    for (int k = 0; k < 6; k++)
        if (btn(B[k])) h |= (unsigned)B[k];
    return h;
}

/* fresh presses (with the menus' auto-repeat on the pad) */
static unsigned pressed_buttons(void) {
    unsigned p = 0;
    if (btnp(BTN_A)) p |= BTN_A;
    if (btnp(BTN_B)) p |= BTN_B;
    if (btn_repeat(BTN_UP)) p |= BTN_UP;
    if (btn_repeat(BTN_DOWN)) p |= BTN_DOWN;
    if (btn_repeat(BTN_LEFT)) p |= BTN_LEFT;
    if (btn_repeat(BTN_RIGHT)) p |= BTN_RIGHT;
    return p;
}

/* ------------------------------------------------------------------ */
/* the title                                                            */

static void update_title(unsigned pr) {
    bool has = save_exists();
    int n = has ? 3 : 2; /* (CONTINUE) NEW GAME CODE */
    if (pr & BTN_UP) { sdg.title_sel = (sdg.title_sel + n - 1) % n; sdg.title_erase = 0; sfx_play_name("sdg_tick"); }
    if (pr & BTN_DOWN) { sdg.title_sel = (sdg.title_sel + 1) % n; sdg.title_erase = 0; sfx_play_name("sdg_tick"); }
    if (sdg.title_sel >= n) sdg.title_sel = 0;
    if ((pr & BTN_B) && sdg.scene_t > 10) { game_exit_to_library(); return; }
    if ((pr & BTN_A) && sdg.scene_t > 15) {
        int pick = has ? sdg.title_sel : sdg.title_sel + 1;
        if (pick == 0) {
            P = sdg.saved;
            sdg_refill(&P);
            sfx_play_name("ui_ok");
            sdg_goto(SC_RAFT);
        } else if (pick == 2) {
            sfx_play_name("ui_ok");
            sdg.code_pos = 0;
            sdg.code_msg_t = 0;
            sdg_goto(SC_CODE);
        } else if (has && !sdg.title_erase) {
            sdg.title_erase = 1;
            sfx_play_name("sdg_nope");
        } else {
            new_game();
            sfx_play_name("ui_ok");
            sdg.raft_sel = 0;
            sdg_goto(SC_RAFT);
        }
    }
}

static void update_code(unsigned pr) {
    if (sdg.code_msg_t > 0) sdg.code_msg_t--;
    char *c = &sdg.code[sdg.code_pos];
    if (pr & BTN_LEFT) { sdg.code_pos = (sdg.code_pos + 7) % 8; sfx_play_name("sdg_tick"); }
    if (pr & BTN_RIGHT) { sdg.code_pos = (sdg.code_pos + 1) % 8; sfx_play_name("sdg_tick"); }
    if (pr & BTN_UP) { *c = (char)(*c >= 'Z' ? 'A' : *c + 1); sfx_play_name("sdg_tick"); }
    if (pr & BTN_DOWN) { *c = (char)(*c <= 'A' ? 'Z' : *c - 1); sfx_play_name("sdg_tick"); }
    if (pr & BTN_B) { sfx_play_name("ui_back"); sdg_goto(SC_TITLE); return; }
    if ((pr & BTN_A) && sdg.scene_t > 5) {
        if (!strncmp(sdg.code, SDG_CODE, 8)) {
            sdg.code_on = 1;
            sfx_play_name("sdg_head");
        } else sfx_play_name("sdg_nope");
        sdg.code_msg_t = 120;
    }
}

/* ------------------------------------------------------------------ */
/* the raft, the shop and the kit                                       */

static void update_raft(unsigned pr) {
    if (pr & BTN_LEFT) { sdg.raft_sel = (sdg.raft_sel + 2) % 3; sfx_play_name("sdg_tick"); }
    if (pr & BTN_RIGHT) { sdg.raft_sel = (sdg.raft_sel + 1) % 3; sfx_play_name("sdg_tick"); }
    if (pr & BTN_B) { sfx_play_name("ui_back"); sdg_goto(SC_TITLE); return; }
    if (pr & BTN_A) {
        sfx_play_name("ui_ok");
        if (sdg.raft_sel == 0) { sdg.shop_sel = 0; sdg_goto(SC_SHOP); }
        else if (sdg.raft_sel == 1) { sdg.kit_col = 1; sdg.kit_sel = 0; sdg.kit_held = 0; sdg.kit_scroll = 0; sdg_goto(SC_KIT); }
        else {
            sdg_refill(&P);
            sdg_dive_begin();
            sdg.note_t = 0;
            sdg_goto(SC_DIVE);
            sfx_play_name("sdg_splash");
        }
    }
}

static void update_shop(unsigned pr) {
    uint8_t list[16];
    int n = sdg_shop_list(sdg.shop_page, list, 16);
    if (pr & BTN_LEFT) { sdg.shop_page = (sdg.shop_page + SHOP_PAGES - 1) % SHOP_PAGES; sdg.shop_sel = 0; sfx_play_name("sdg_tick"); }
    if (pr & BTN_RIGHT) { sdg.shop_page = (sdg.shop_page + 1) % SHOP_PAGES; sdg.shop_sel = 0; sfx_play_name("sdg_tick"); }
    n = sdg_shop_list(sdg.shop_page, list, 16);
    if (pr & BTN_UP) { sdg.shop_sel = (sdg.shop_sel + n) % (n + 1); sfx_play_name("sdg_tick"); }
    if (pr & BTN_DOWN) { sdg.shop_sel = (sdg.shop_sel + 1) % (n + 1); sfx_play_name("sdg_tick"); }
    if (pr & BTN_B) { sfx_play_name("ui_back"); sdg_goto(SC_RAFT); return; }
    if (pr & BTN_A) {
        if (sdg.shop_sel >= n) { sfx_play_name("ui_back"); sdg_goto(SC_RAFT); return; }
        if (sdg_buy(&P, list[sdg.shop_sel])) {
            sfx_play_name("sdg_buy");
            write_save();
        } else sfx_play_name("sdg_nope");
    }
}

/* the kit list: every item owned */
static int kit_list(uint8_t *out) {
    int n = 0;
    for (int it = 1; it < IT_COUNT; it++)
        if (P.owned[it] > 0) out[n++] = (uint8_t)it;
    return n;
}

static void kit_leave(void) {
    if (sdg.kit_held) {
        if (sdg.kit_from >= 0) P.equip[sdg.kit_from / 2][sdg.kit_from % 2] = (uint8_t)sdg.kit_held;
        sdg.kit_held = 0;
    }
    sdg_refill(&P);
    write_save();
    sdg_goto(SC_RAFT);
}

static void update_kit(unsigned pr) {
    uint8_t list[IT_COUNT];
    int n = kit_list(list);
    if (sdg.kit_col == 0) {
        if (n == 0) { sdg.kit_col = 1; sdg.kit_sel = 0; return; }
        sdg.kit_sel = iclamp(sdg.kit_sel, 0, n - 1);
        if (pr & BTN_UP) { sdg.kit_sel = (sdg.kit_sel + n - 1) % n; sfx_play_name("sdg_tick"); }
        if (pr & BTN_DOWN) { sdg.kit_sel = (sdg.kit_sel + 1) % n; sfx_play_name("sdg_tick"); }
        if (pr & BTN_RIGHT) { sdg.kit_col = 1; sdg.kit_sel = 0; sfx_play_name("sdg_tick"); return; }
        if (pr & BTN_A) {
            int it = list[sdg.kit_sel];
            if (sdg.kit_held) {
                /* back into storage */
                sdg.kit_held = 0;
                sfx_play_name("sdg_tick");
            } else if (sdg_stored(&P, it) > 0) {
                sdg.kit_held = it;
                sdg.kit_from = -1;
                sdg.kit_col = 1;
                sdg.kit_sel = 0;
                sfx_play_name("sdg_tick");
            } else sfx_play_name("sdg_nope");
        }
    } else {
        if (pr & BTN_UP) { sdg.kit_sel = sdg.kit_sel >= 6 ? 4 : sdg.kit_sel >= 2 ? sdg.kit_sel - 2 : 6; sfx_play_name("sdg_tick"); }
        else if (pr & BTN_DOWN) { sdg.kit_sel = sdg.kit_sel >= 6 ? 0 : sdg.kit_sel + 2 >= 6 ? 6 : sdg.kit_sel + 2; sfx_play_name("sdg_tick"); }
        if (pr & BTN_RIGHT) { if (sdg.kit_sel < 6 && (sdg.kit_sel & 1) == 0) sdg.kit_sel++; sfx_play_name("sdg_tick"); }
        if (pr & BTN_LEFT) {
            if (sdg.kit_sel < 6 && (sdg.kit_sel & 1)) sdg.kit_sel--;
            else if (n > 0) { sdg.kit_col = 0; sdg.kit_sel = 0; }
            sfx_play_name("sdg_tick");
        }
        if (pr & BTN_A) {
            if (sdg.kit_sel >= 6) { sfx_play_name("ui_back"); kit_leave(); return; }
            int d = sdg.kit_sel / 2, s = sdg.kit_sel % 2;
            if (!sdg.kit_held) {
                if (P.equip[d][s]) {
                    sdg.kit_held = P.equip[d][s];
                    sdg.kit_from = sdg.kit_sel;
                    P.equip[d][s] = 0;
                    sfx_play_name("sdg_tick");
                } else sfx_play_name("sdg_nope");
            } else {
                int old = P.equip[d][s];
                if (sdg.kit_from == sdg.kit_sel) {
                    /* into itself: back to storage */
                } else {
                    P.equip[d][s] = (uint8_t)sdg.kit_held;
                    if (sdg.kit_from >= 0) P.equip[sdg.kit_from / 2][sdg.kit_from % 2] = (uint8_t)old;
                }
                sdg.kit_held = 0;
                sfx_play_name("sdg_tick");
            }
        }
    }
    if (pr & BTN_B) {
        if (sdg.kit_held) {
            if (sdg.kit_from >= 0) P.equip[sdg.kit_from / 2][sdg.kit_from % 2] = (uint8_t)sdg.kit_held;
            sdg.kit_held = 0;
            sfx_play_name("ui_back");
        } else { sfx_play_name("ui_back"); kit_leave(); }
    }
}

/* ------------------------------------------------------------------ */
/* the dive and the item menu                                           */

static void begin_fight(int mob) {
    SdgMob *m = &sdg.dive.mob[mob];
    uint8_t kinds[SDG_FOES];
    int n = 0, boss = 0;
    sdg_group_for(m->kind, m->region, kinds, &n);
    if (m->kind == MK_WARDEN) boss = 1;
    if (m->kind == MK_GLOAM) boss = 2;
    sdg_battle_start(mob, kinds, n, boss);
    sdg_goto(SC_BATTLE);
    /* the red deep's own music carries on through its ordinary fights */
    music_play(boss == 1 ? SDG_MUS_WARDEN : boss == 2 ? SDG_MUS_GLOAM : m->region == RG_DEEP ? SDG_MUS_DEEP : SDG_MUS_BATTLE);
    sfx_play_name("sdg_fight");
    gfx_set_flash(6);
}

static void update_dive(unsigned held, unsigned pr) {
    if (sdg.note_t > 0) sdg.note_t--;
    if (pr & BTN_B) {
        sdg.menu_sel = 0;
        sdg.menu_pick = 0;
        sfx_play_name("sdg_tick");
        sdg_goto(SC_MENU);
        return;
    }
    sdg_dive_step(held, pr);
    if (sdg_dive_news == DV_SURFACE) {
        sdg_surface();
        sfx_play_name("sdg_splash");
        sdg_goto(SC_SURFACE);
        music_play(SDG_MUS_RAFT);
        return;
    }
    if (sdg_dive_news == DV_BATTLE) { begin_fight(sdg_dive_news_mob); return; }
    if ((sdg.frame & 15) == 0) dive_music();
}

static void update_menu(unsigned pr) {
    if (!sdg.menu_pick) {
        if (pr & BTN_UP) { sdg.menu_sel = (sdg.menu_sel + 4) % 6; sfx_play_name("sdg_tick"); }
        if (pr & BTN_DOWN) { sdg.menu_sel = (sdg.menu_sel + 2) % 6; sfx_play_name("sdg_tick"); }
        if (pr & (BTN_LEFT | BTN_RIGHT)) { sdg.menu_sel ^= 1; sfx_play_name("sdg_tick"); }
        if (pr & BTN_B) { sfx_play_name("ui_back"); sdg_goto(SC_DIVE); return; }
        if (pr & BTN_A) {
            int d = sdg.menu_sel / 2, s = sdg.menu_sel % 2, it = P.equip[d][s];
            if (!sdg_dive_usable(it) || P.hp[d] <= 0 || P.uses[d][s] == 0) { sfx_play_name("sdg_nope"); return; }
            int k = SDG_ITEM[it].kind;
            if (k == K_BOMB || k == K_MIST) {
                sdg_use_item_dive(d, s, d);
                sdg_goto(SC_DIVE);
                return;
            }
            sdg.menu_pick = 1;
            sdg.menu_target = d;
            if (k == K_EGG)
                for (int t = 0; t < 3; t++)
                    if (P.hp[t] <= 0) { sdg.menu_target = t; break; }
        }
    } else {
        if (pr & BTN_LEFT) { sdg.menu_target = (sdg.menu_target + 2) % 3; sfx_play_name("sdg_tick"); }
        if (pr & BTN_RIGHT) { sdg.menu_target = (sdg.menu_target + 1) % 3; sfx_play_name("sdg_tick"); }
        if (pr & BTN_B) { sdg.menu_pick = 0; sfx_play_name("ui_back"); return; }
        if (pr & BTN_A) {
            sdg_use_item_dive(sdg.menu_sel / 2, sdg.menu_sel % 2, sdg.menu_target);
            sdg.menu_pick = 0;
        }
    }
}

static void update_battle(unsigned pr) {
    sdg_battle_step(pr);
    SdgBattle *b = &sdg.bat;
    if (b->phase == BP_WIN && b->nmsg == 0 && sdg.scene_t > 1 && !sdg.snd_ev) {
        sdg.snd_ev = 1;
        music_play(b->levelled ? SDG_MUS_LEVEL : SDG_MUS_WIN);
    }
    if (b->phase != BP_DONE) return;
    sdg.snd_ev = 0;
    if (b->wipe) { sdg_goto(SC_WIPE); return; }
    if (b->mob >= 0) {
        SdgMob *m = &sdg.dive.mob[b->mob];
        bool won = sdg_alive_count(&P) > 0 && b->nmsg == 0 && b->xp >= 0;
        m->state = MS_GONE;
        m->gone_at = sdg.dive.t;
        if (b->boss == 1) {
            bool beat = true;
            for (int i = 0; i < b->nfoe; i++)
                if (b->foe[i].alive) beat = false;
            if (beat) {
                sdg_open_door(DOOR_WARDEN);
                sdg_note("THE SHRINE'S DOORS GRIND OPEN.", NULL);
            } else m->state = MS_HOME;
        }
        if (b->boss == 2) {
            bool beat = true;
            for (int i = 0; i < b->nfoe; i++)
                if (b->foe[i].alive) beat = false;
            if (beat) { sdg_win_final(); return; }
            m->state = MS_HOME;
        }
        (void)won;
    }
    sdg.dive.inv = 90;
    sdg_goto(SC_DIVE);
}

/* ------------------------------------------------------------------ */
/* the cartridge                                                        */

static void sdg_load(void) {
    sdg_art_load();
    sdg_audio_load();
    sdg_map_index();
}

static void sdg_start(void) {
    memset(&sdg, 0, sizeof sdg);
    rng_seed(&sdg.rng, rng_next(&g_rng));
    memcpy(sdg.code, "AAAAAAAA", 9);
    load_save();
    sdg_dive_begin();
    sdg_goto(SC_TITLE);
}

static void sdg_quit(void) {
    /* the log only holds what was written at the raft */
}

static void update(void) {
    sdg.frame++;
    sdg.scene_t++;
    unsigned held = held_buttons(), pr = pressed_buttons();
    switch (sdg.scene) {
    case SC_TITLE: update_title(pr); break;
    case SC_CODE: update_code(pr); break;
    case SC_RAFT: update_raft(pr); break;
    case SC_SHOP: update_shop(pr); break;
    case SC_KIT: update_kit(pr); break;
    case SC_DIVE: update_dive(held, pr); break;
    case SC_MENU: update_menu(pr); break;
    case SC_BATTLE: update_battle(pr); break;
    case SC_WIPE:
        if ((pr & BTN_A) && sdg.scene_t > 60) { sdg_wipe(); sdg_goto(SC_RAFT); }
        break;
    case SC_SURFACE:
        if ((pr & BTN_A) && sdg.scene_t > 20) sdg_goto(SC_RAFT);
        break;
    case SC_ENDING:
        sdg.ending_t++;
        if ((pr & BTN_A) && sdg.ending_t > 600) sdg_goto(SC_RAFT);
        break;
    default: break;
    }
}

static void draw(void) {
    switch (sdg.scene) {
    case SC_TITLE: sdg_draw_title(); break;
    case SC_CODE: sdg_draw_code(); break;
    case SC_RAFT: sdg_draw_raft(); break;
    case SC_SHOP: sdg_draw_shop(); break;
    case SC_KIT: sdg_draw_kit(); break;
    case SC_DIVE: sdg_draw_dive(); break;
    case SC_MENU: sdg_draw_dive(); sdg_draw_menu(); break;
    case SC_BATTLE: sdg_draw_battle(); break;
    case SC_WIPE: sdg_draw_wipe(); break;
    case SC_SURFACE: sdg_draw_surface(); break;
    case SC_ENDING: sdg_draw_ending(); break;
    default: gfx_cls(C_INK); break;
    }
}

/* ------------------------------------------------------------------ */
/* test hooks                                                           */

static int num_key(const char *key, const char *name, int *n) {
    size_t len = strlen(name);
    if (strncmp(key, name, len) || key[len] < '0' || key[len] > '9') return 0;
    *n = atoi(key + len);
    return 1;
}

/* the map's own checks: every chest, lever, head and both guardians can be
 * reached from the surface with every door and wall open; with them shut the
 * deep caves can't (the Warden keeps them) */
static int map_bad(void) {
    static uint8_t seen[SDG_MH][SDG_MW];
    static int16_t qc[SDG_MW * SDG_MH], qr[SDG_MW * SDG_MH];
    int bad = 0;
    for (int pass = 0; pass < 2; pass++) {
        uint8_t keep_doors = P.doors, keep_walls = P.walls;
        P.doors = pass ? 0xFF : (uint8_t)((1 << DOOR_WEST) | (1 << DOOR_SHORT));
        P.walls = pass ? 0xFF : 0;
        memset(seen, 0, sizeof seen);
        int h = 0, t = 0;
        for (int c = SDG_SURF_C0; c <= SDG_SURF_C1; c++)
            if (sdg_cell_open(c, SDG_SURF_ROW)) { seen[SDG_SURF_ROW][c] = 1; qc[t] = (int16_t)c; qr[t++] = SDG_SURF_ROW; }
        while (h < t) {
            int c = qc[h], r = qr[h++];
            static const int DC[4] = {1, -1, 0, 0}, DR[4] = {0, 0, 1, -1};
            for (int k = 0; k < 4; k++) {
                int nc = c + DC[k], nr = r + DR[k];
                if (nc < 0 || nr < 0 || nc >= SDG_MW || nr >= SDG_MH || seen[nr][nc] || !sdg_cell_open(nc, nr)) continue;
                seen[nr][nc] = 1;
                qc[t] = (int16_t)nc;
                qr[t++] = (int16_t)nr;
            }
        }
        P.doors = keep_doors;
        P.walls = keep_walls;
        if (pass) {
            for (int i = 0; i < sdg_mi.nchest; i++) bad += !seen[sdg_mi.chest[i].r][sdg_mi.chest[i].c];
            for (int i = 0; i < sdg_mi.nhead; i++) bad += !seen[sdg_mi.head[i].r][sdg_mi.head[i].c];
            for (int i = 0; i < LV_COUNT; i++) bad += !seen[sdg_mi.lever[i].r][sdg_mi.lever[i].c];
            bad += !seen[sdg_mi.warden.r][sdg_mi.warden.c];
            bad += !seen[sdg_mi.gloam.r][sdg_mi.gloam.c];
            for (int i = 0; i < sdg_mi.nspawn; i++) bad += !seen[sdg_mi.spawn[i].r][sdg_mi.spawn[i].c];
        } else {
            /* before the Warden: the levers of the deep and the last door stay out of reach */
            bad += seen[sdg_mi.lever[LV_FINAL_W].r][sdg_mi.lever[LV_FINAL_W].c];
            bad += seen[sdg_mi.lever[LV_FINAL_E].r][sdg_mi.lever[LV_FINAL_E].c];
            bad += seen[sdg_mi.gloam.r][sdg_mi.gloam.c];
            bad += seen[sdg_mi.head[2].r][sdg_mi.head[2].c];
            bad += !seen[sdg_mi.warden.r][sdg_mi.warden.c];
        }
    }
    bad += sdg_mi.nchest != SDG_CHESTS;
    bad += sdg_mi.nhead != SDG_HEADS;
    bad += sdg_mi.nlore != SDG_LORES;
    bad += sdg_mi.nwall != SDG_WALLS;
    return bad;
}

static int calc_out;

static int count_mobs(int kind) {
    int n = 0;
    for (int i = 0; i < sdg.dive.nmob; i++) n += sdg.dive.mob[i].kind == kind;
    return n;
}

static int sdg_query(const char *key, int *out) {
    int i;
    SdgBattle *b = &sdg.bat;
    if (!strcmp(key, "bot")) { *out = sdg.bot_on ? (int)sdg_bot() : 0; return 1; }
    if (!strcmp(key, "scene")) { *out = sdg.scene; return 1; }
    if (!strcmp(key, "level")) { *out = P.level; return 1; }
    if (!strcmp(key, "xp")) { *out = (int)P.xp; return 1; }
    if (!strcmp(key, "gold")) { *out = (int)P.gold; return 1; }
    if (num_key(key, "hp", &i)) { *out = i < 3 ? P.hp[i] : -1; return 1; }
    if (num_key(key, "maxhp", &i)) { *out = i < 3 ? sdg_diver_maxhp(&P, i) : -1; return 1; }
    if (num_key(key, "dlevel", &i)) { *out = i < 3 ? sdg_diver_level(&P, i) : -1; return 1; }
    if (num_key(key, "relic", &i)) { *out = i < RL_COUNT ? P.relic[i] : -1; return 1; }
    if (num_key(key, "carry", &i)) { *out = i < RL_COUNT ? P.carry[i] : -1; return 1; }
    if (num_key(key, "owned", &i)) { *out = i < IT_COUNT ? P.owned[i] : -1; return 1; }
    if (num_key(key, "eq", &i)) { *out = i < 6 ? P.equip[i / 2][i % 2] : -1; return 1; }
    if (num_key(key, "uses", &i)) { *out = i < 6 ? P.uses[i / 2][i % 2] : -1; return 1; }
    if (!strcmp(key, "doors")) { *out = P.doors; return 1; }
    if (!strcmp(key, "levers")) { *out = P.levers; return 1; }
    if (!strcmp(key, "walls")) { *out = P.walls; return 1; }
    if (!strcmp(key, "heads")) { *out = P.heads; return 1; }
    if (!strcmp(key, "chests")) { *out = (int)P.chests; return 1; }
    if (!strcmp(key, "npearl")) { *out = P.npearl; return 1; }
    if (!strcmp(key, "deepest")) { *out = P.deepest; return 1; }
    if (!strcmp(key, "depth")) { *out = sdg_depth(); return 1; }
    if (!strcmp(key, "x")) { *out = (int)(sdg.dive.x >> 8); return 1; }
    if (!strcmp(key, "y")) { *out = (int)(sdg.dive.y >> 8); return 1; }
    if (!strcmp(key, "region")) { *out = sdg_region_at((int)(sdg.dive.x >> 8) / SDG_T, (int)(sdg.dive.y >> 8) / SDG_T); return 1; }
    if (!strcmp(key, "mist")) { *out = (int)(sdg.dive.mist >> 8); return 1; }
    if (!strcmp(key, "wins")) { *out = P.wins; return 1; }
    if (!strcmp(key, "cherry_wins")) { *out = P.cherry_wins; return 1; }
    if (!strcmp(key, "gift")) { *out = P.gift; return 1; }
    if (!strcmp(key, "dives")) { *out = P.dives; return 1; }
    if (!strcmp(key, "doors_open")) { *out = P.doors_open; return 1; }
    if (!strcmp(key, "chests_open")) { *out = P.chests_open; return 1; }
    if (!strcmp(key, "saved_level")) { *out = sdg.saved.level; return 1; }
    if (!strcmp(key, "saved_gold")) { *out = (int)sdg.saved.gold; return 1; }
    if (!strcmp(key, "saved_chests")) { *out = (int)sdg.saved.chests; return 1; }
    if (num_key(key, "saved_relic", &i)) { *out = i < RL_COUNT ? sdg.saved.relic[i] : -1; return 1; }
    if (!strcmp(key, "map_bad")) { *out = map_bad(); return 1; }
    if (!strcmp(key, "art_bad")) { *out = sdg_art_bad(); return 1; }
    if (!strcmp(key, "nmob")) { *out = sdg.dive.nmob; return 1; }
    if (num_key(key, "mobkind", &i)) { *out = count_mobs(i); return 1; }
    if (num_key(key, "mobstate", &i)) { *out = i < sdg.dive.nmob ? sdg.dive.mob[i].state : -1; return 1; }
    if (!strcmp(key, "bphase")) { *out = b->phase; return 1; }
    if (!strcmp(key, "bcur")) { *out = b->cur; return 1; }
    if (!strcmp(key, "bround")) { *out = b->round; return 1; }
    if (!strcmp(key, "bmsgs")) { *out = b->nmsg; return 1; }
    if (!strcmp(key, "nfoe")) { *out = b->nfoe; return 1; }
    if (!strcmp(key, "boss")) { *out = b->boss; return 1; }
    if (!strcmp(key, "bxp")) { *out = (int)b->xp; return 1; }
    if (!strcmp(key, "foes_alive")) { int n = 0; for (int k = 0; k < b->nfoe; k++) n += b->foe[k].alive; *out = n; return 1; }
    if (num_key(key, "foehp", &i)) { *out = i < b->nfoe ? b->foe[i].hp : -1; return 1; }
    if (num_key(key, "foekind", &i)) { *out = i < b->nfoe ? b->foe[i].kind : -1; return 1; }
    if (num_key(key, "foemark", &i)) { *out = i < b->nfoe ? b->foe[i].mark : -1; return 1; }
    if (num_key(key, "foeweak", &i)) { *out = i < b->nfoe ? b->foe[i].weak : -1; return 1; }
    if (num_key(key, "foealive", &i)) { *out = i < b->nfoe ? b->foe[i].alive : -1; return 1; }
    if (num_key(key, "guarding", &i)) { *out = i < 3 ? b->guarding[i] : -1; return 1; }
    if (!strcmp(key, "save_bytes")) { *out = (int)sizeof(SdgProg); return 1; }
    if (!strcmp(key, "bot_task")) { *out = sdg_bot_task; return 1; }
    if (!strcmp(key, "ending")) { *out = sdg.ending; return 1; }
    if (!strcmp(key, "music")) { *out = music_playing(); return 1; }
    if (!strcmp(key, "mus_battle")) { *out = SDG_MUS_BATTLE; return 1; }
    if (!strcmp(key, "mus_deep")) { *out = SDG_MUS_DEEP; return 1; }
    if (!strcmp(key, "mus_still")) { *out = SDG_MUS_STILL; return 1; }
    if (!strcmp(key, "frame")) { *out = (int)sdg.frame; return 1; }
    if (!strcmp(key, "code_on")) { *out = sdg.code_on; return 1; }
    if (!strcmp(key, "calc")) { *out = calc_out; return 1; }
    if (!strcmp(key, "music_kind")) {
        int m = music_playing();
        const int ids[10] = {SDG_MUS_TITLE, SDG_MUS_RAFT, SDG_MUS_BATTLE, SDG_MUS_WARDEN, SDG_MUS_GLOAM, SDG_MUS_DEEP, SDG_MUS_STILL, SDG_MUS_WIN, SDG_MUS_WIPE, SDG_MUS_END};
        *out = 0;
        for (int k = 0; k < 10; k++)
            if (m >= 0 && m == ids[k]) *out = k + 1;
        return 1;
    }
    if (!strcmp(key, "thorns_taken")) { *out = b->thorns_taken; return 1; }
    if (!strcmp(key, "leech_healed")) { *out = b->leech_healed; return 1; }
    if (!strcmp(key, "covered")) { *out = b->covered; return 1; }
    if (!strcmp(key, "last_dmg")) { *out = b->last_dmg; return 1; }
    if (!strcmp(key, "last_target")) { *out = b->last_target; return 1; }
    if (!strcmp(key, "last_heavy")) { *out = b->last_heavy; return 1; }
    if (!strcmp(key, "foe_heals")) { *out = b->heals; return 1; }
    if (!strcmp(key, "foe_revives")) { *out = b->revives; return 1; }
    if (!strcmp(key, "bmenu")) { *out = b->menu; return 1; }
    if (!strcmp(key, "tmode")) { *out = b->tmode; return 1; }
    if (!strcmp(key, "tsel")) { *out = b->tsel; return 1; }
    if (!strcmp(key, "imenu_pick")) { *out = sdg.menu_pick; return 1; }
    if (!strcmp(key, "imenu_sel")) { *out = sdg.menu_sel; return 1; }
    if (!strcmp(key, "kit_held")) { *out = sdg.kit_held; return 1; }
    if (!strcmp(key, "kit_sel")) { *out = sdg.kit_sel; return 1; }
    if (!strcmp(key, "kit_col")) { *out = sdg.kit_col; return 1; }
    if (!strcmp(key, "shop_page")) { *out = sdg.shop_page; return 1; }
    if (!strcmp(key, "shop_sel")) { *out = sdg.shop_sel; return 1; }
    if (!strcmp(key, "raft_sel")) { *out = sdg.raft_sel; return 1; }
    if (num_key(key, "stored", &i)) { *out = i < IT_COUNT ? sdg_stored(&P, i) : -1; return 1; }
    if (!strcmp(key, "nshots")) { int n = 0; for (int k = 0; k < SDG_SHOTS; k++) n += sdg.dive.shot[k].on; *out = n; return 1; }
    if (!strcmp(key, "title_sel")) { *out = sdg.title_sel; return 1; }
    if (!strcmp(key, "code_pos")) { *out = sdg.code_pos; return 1; }
    if (num_key(key, "code_ch", &i)) { *out = i < 8 ? sdg.code[i] : -1; return 1; }
    return 0;
}

static int sdg_cheat(const char *cmd) {
    int a, b2, c, d, e;
    if (sscanf(cmd, "calc_atk %d %d %d %d %d", &a, &b2, &c, &d, &e) == 5) { calc_out = sdg_attack_damage(a, b2, c, d, e); return 1; }
    if (sscanf(cmd, "calc_reduce %d %d", &a, &b2) == 2) { calc_out = sdg_reduce(a, b2); return 1; }
    if (sscanf(cmd, "run_rate %d", &a) == 1) {
        calc_out = 0;
        for (int k = 0; k < a; k++) calc_out += sdg_chance(SDG_ESCAPE);
        return 1;
    }
    if (sscanf(cmd, "shield_mul %d", &a) == 1) { calc_out = sdg_shield_mul(&P, a % 3); return 1; }
    if (sscanf(cmd, "give %d %d", &a, &b2) == 2) { if (a > 0 && a < IT_COUNT) P.owned[a] = (uint8_t)b2; return 1; }
    if (sscanf(cmd, "give %d", &a) == 1) { if (a > 0 && a < IT_COUNT) P.owned[a]++; return 1; }
    if (sscanf(cmd, "equip %d %d %d", &a, &b2, &c) == 3) {
        if (a >= 0 && a < 3 && b2 >= 0 && b2 < 2 && c >= 0 && c < IT_COUNT) {
            if (c && P.owned[c] == 0) P.owned[c] = 1;
            P.equip[a][b2] = (uint8_t)c;
            P.uses[a][b2] = c ? SDG_ITEM[c].uses : 0;
        }
        return 1;
    }
    if (sscanf(cmd, "uses %d %d %d", &a, &b2, &c) == 3) { P.uses[a % 3][b2 & 1] = (uint8_t)c; return 1; }
    if (sscanf(cmd, "gold %d", &a) == 1) { P.gold = sdg_cap_gold(a); return 1; }
    if (sscanf(cmd, "relic %d %d", &a, &b2) == 2) { if (a >= 0 && a < RL_COUNT) P.relic[a] = (uint8_t)b2; return 1; }
    if (sscanf(cmd, "carry %d %d", &a, &b2) == 2) { if (a >= 0 && a < RL_COUNT) P.carry[a] = (uint8_t)b2; return 1; }
    if (sscanf(cmd, "level %d", &a) == 1) {
        P.level = (uint8_t)iclamp(a, 1, SDG_MAX_LEVEL);
        P.xp = SDG_XP_AT[P.level];
        for (int k = 0; k < 3; k++) P.hp[k] = (int16_t)sdg_diver_maxhp(&P, k);
        return 1;
    }
    if (sscanf(cmd, "xp %d", &a) == 1) { sdg_add_xp(&P, a); return 1; }
    if (sscanf(cmd, "hp %d %d", &a, &b2) == 2) { if (a >= 0 && a < 3) P.hp[a] = (int16_t)b2; return 1; }
    if (sscanf(cmd, "at %d %d", &a, &b2) == 2) {
        sdg.dive.x = (a * SDG_T + SDG_T / 2) * 256;
        sdg.dive.y = (b2 * SDG_T + SDG_T / 2) * 256;
        sdg.dive.left_surface = 1;
        return 1;
    }
    if (sscanf(cmd, "fight %d %d %d %d", &a, &b2, &c, &d) >= 1) {
        uint8_t k[4];
        int n = sscanf(cmd, "fight %d %d %d %d", &a, &b2, &c, &d);
        int v[4] = {a, b2, c, d};
        for (int j = 0; j < n; j++) k[j] = (uint8_t)iclamp(v[j], 0, EN_COUNT - 1);
        sdg_battle_start(-1, k, n, k[0] == EN_ARM || k[0] == EN_EYE ? 2 : (n > 1 && k[1] == EN_WARDEN) ? 1 : 0);
        sdg_goto(SC_BATTLE);
        music_play(SDG_MUS_BATTLE);
        return 1;
    }
    if (sscanf(cmd, "foehp %d %d", &a, &b2) == 2) { if (a >= 0 && a < sdg.bat.nfoe) sdg.bat.foe[a].hp = (int16_t)b2; return 1; }
    if (sscanf(cmd, "foehide %d", &a) == 1) { if (a >= 0 && a < sdg.bat.nfoe) sdg.bat.foe[a].hidden = 1; return 1; }
    if (sscanf(cmd, "foeweak %d %d", &a, &b2) == 2) { if (a >= 0 && a < sdg.bat.nfoe) sdg.bat.foe[a].weak = (uint8_t)b2; return 1; }
    if (sscanf(cmd, "seed %d", &a) == 1) { rng_seed(&sdg.rng, (uint64_t)a); return 1; }
    if (sscanf(cmd, "door %d", &a) == 1) { sdg_open_door(a); return 1; }
    if (sscanf(cmd, "wall %d", &a) == 1) { P.walls |= (uint8_t)(1 << a); return 1; }
    if (sscanf(cmd, "head %d", &a) == 1) { P.heads |= (uint8_t)(1 << a); return 1; }
    if (sscanf(cmd, "pearl %d", &a) == 1) { if (P.npearl < SDG_PEARLS) P.pearl[P.npearl++] = (uint8_t)a; return 1; }
    if (sscanf(cmd, "bot %d", &a) == 1) { sdg.bot_on = 1; sdg.bot_goal = (uint8_t)a; sdg_bot_reset(); return 1; }
    if (sscanf(cmd, "bot_debug %d", &a) == 1) { sdg_bot_debug = a; return 1; }
    if (!strcmp(cmd, "bot_off")) { sdg.bot_on = 0; return 1; }
    if (!strcmp(cmd, "nomobs")) {
        for (int k = 0; k < sdg.dive.nmob; k++) { sdg.dive.mob[k].state = MS_GONE; sdg.dive.mob[k].gone_at = 0xF0000000u; }
        return 1;
    }
    if (sscanf(cmd, "onlymob %d %d", &a, &b2) == 2) {
        /* every creature away but the one that starts at cell a, b2 */
        for (int k = 0; k < sdg.dive.nmob; k++) {
            SdgMob *m = &sdg.dive.mob[k];
            if (m->hx / SDG_T == a && m->hy / SDG_T == b2) continue;
            m->state = MS_GONE;
            m->gone_at = 0xF0000000u;
        }
        return 1;
    }
    if (!strcmp(cmd, "surface")) { sdg_surface(); sdg_goto(SC_RAFT); return 1; }
    if (!strcmp(cmd, "dive")) { sdg_refill(&P); sdg_dive_begin(); sdg_goto(SC_DIVE); return 1; }
    if (!strcmp(cmd, "new")) { new_game(); sdg_goto(SC_RAFT); return 1; }
    if (!strcmp(cmd, "save")) { write_save(); return 1; }
    if (sscanf(cmd, "ending %d", &a) == 1) { sdg.ending = a; sdg.ending_t = 0; sdg_goto(SC_ENDING); return 1; }
    if (sscanf(cmd, "scene %d", &a) == 1) { sdg_goto(a); return 1; }
    if (sscanf(cmd, "kit %d %d", &a, &b2) == 2) { sdg.kit_col = a; sdg.kit_sel = b2; return 1; }
    if (sscanf(cmd, "shop %d %d", &a, &b2) == 2) { sdg.shop_page = a; sdg.shop_sel = b2; return 1; }
    if (sscanf(cmd, "note %d", &a) == 1) { if (a >= 0 && a < SDG_LORES) sdg_note(SDG_LORE_TEXT[a][0], SDG_LORE_TEXT[a][1]); return 1; }
    return 0;
}

const GameDef GAME_SOUNDINGS = {
    "soundings",
    "SOUNDINGS",
    "1986",
    "RPG",
    "THREE AXOLOTLS DIVE FROM ONE LIT RAFT INTO A DARK, SILENT SEA.",
    {"BRING HOME THE LEECH CLUB", "STILL THE GLOAMHEART", "THREE HEADS, THEN STILL IT"},
    GLYPH_DPAD "\tSWIM / MOVE\n"
    GLYPH_A "\tOPEN, PULL, LOOK / CHOOSE\n"
    GLYPH_B "\tITEMS / BACK\n"
    "START\tPAUSE",
    C_NAVY, C_CYAN,
    sdg_load, sdg_start, update, draw, sdg_quit, sdg_draw_label, sdg_query, sdg_cheat,
    "DIVERS", 27,
    NULL,
};
