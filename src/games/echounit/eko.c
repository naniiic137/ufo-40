/* ECHO UNIT - four bats against V.I.P.E.R.
 * Cartridge 50 of UFO 40, a tribute to Cyber Owls (UFO 50 #50).
 * See docs/games/50-echo-unit.md. This file: the title, the briefing room
 * (the mission board), the run from mission to mission, lives and capture,
 * the jailbreaks, the Caldera finale's handovers, the ending, the records
 * and the test hooks. */
#include "eko.h"

EkoGame eg;
EkoSave egs;
bool eko_simulating;

#define SAVE_MAGIC 0x454B4F01u

const char *const EKO_OWL_NAME[OWL_COUNT] = {"FLINT", "VESPER", "DART", "SABLE"};
const int EKO_OWL_HP[OWL_COUNT] = {8, 6, 6, 4};
const int EKO_OWL_INV[OWL_COUNT] = {60, 60, 60, 20};
const char *const EKO_MISSION_NAME[MI_COUNT] = {"MARSEILLE", "THE ANDES", "CAIRO", "ISTANBUL", "THE CALDERA"};

const EkoMission EKO_MISSION[MI_COUNT] = {
    {"MARSEILLE", 3, {{MODE_BRAWL, OWL_FLINT, BM_DOCKS, "THE DOCKS"},
                      {MODE_BRAWL, OWL_FLINT, BM_YARDS, "THE CONTAINER YARDS"},
                      {MODE_BRAWL, OWL_FLINT, BM_LIFT, "THE CRANE LIFT"}}},
    {"THE ANDES", 1, {{MODE_GALLERY, OWL_VESPER, GS_ANDES, "THE HIGH PASS"}}},
    {"CAIRO", 1, {{MODE_BIKE, OWL_DART, BC_CAIRO, "THE RING ROAD"}}},
    {"ISTANBUL", 1, {{MODE_SNEAK, OWL_SABLE, SM_ISTANBUL, "THE LISTENING POST"}}},
    {"THE CALDERA", 6, {{MODE_SNEAK, OWL_SABLE, 0, "SABLE: THE OUTER HALLS"},
                        {MODE_BRAWL, OWL_FLINT, BM_LAB, "FLINT: THE MAGMA WORKS"},
                        {MODE_SNEAK, OWL_SABLE, 1, "SABLE: THE RED WING"},
                        {MODE_BIKE, OWL_DART, BC_FLIGHT, "DART: OVER THE LAVA SEA"},
                        {MODE_SNEAK, OWL_SABLE, 2, "SABLE: THE BLUE WING"},
                        {MODE_GALLERY, OWL_VESPER, GS_FINALE, "VESPER: THE LAUNCH PAD"}}},
};

const char *const EKO_STORY[] = {
    "SUMMER 1989.",
    "",
    "THE SERPENT SYNDICATE V.I.P.E.R. HAS BUILT",
    "THE DAWN CANNON: A MIRROR IN THE SKY THAT CAN",
    "SET ANY CITY ON EARTH ALIGHT AT NOON.",
    "",
    "THE WORLD'S LAST HOPE HANGS UPSIDE DOWN",
    "IN A BELFRY UNDER LONDON: FOUR BATS, BUILT",
    "BACK BETTER AFTER THE ACCIDENT.",
    "",
    "THEY ARE ECHO UNIT.",
};
const int EKO_STORY_LINES = ARRAY_LEN(EKO_STORY);

const char *const EKO_BRIEF[MI_COUNT][4] = {
    {"FLINT. MARSEILLE'S DOCKS ARE",
     "CRAWLING WITH CRATES OF CANNON",
     "PARTS. CRACK THE CRATES, CRACK",
     "HEADS, AND CORNER SCRAPJAW."},
    {"VESPER. A CONVOY CLIMBS THE",
     "ANDES WITH THE CANNON'S LENS.",
     "HOLD THE HIGH PASS AND PICK",
     "OFF EVERY PEAK POACHER."},
    {"DART. THE SYNDICATE'S GOLD",
     "RIDES CAIRO'S RING ROAD TONIGHT.",
     "RIDE HARD, SHOOT STRAIGHT, AND",
     "KEEP THAT GUN COOL, KID."},
    {"SABLE. CIPHER HAS HACKED OUR",
     "HIDEOUTS FROM AN ISTANBUL",
     "LISTENING POST. SLIP IN SILENT",
     "AND SWITCH HIM OFF."},
    {"ALL OF YOU. THE CANNON SITS IN",
     "A VOLCANO, THE CALDERA. SABLE",
     "LEADS. THERE ARE NO SECOND",
     "CHANCES ON THIS ONE. NONE."},
};

const char *const EKO_ENDING[] = {
    "THE DAWN CANNON FALLS INTO THE SEA.",
    "MADAME COIL IS LED AWAY IN A SACK.",
    "",
    "AT DUSK THE PRIME MINISTER THROWS",
    "A MANGO FEAST FOR ECHO UNIT,",
    "AND FOR ONCE NOBODY HANGS UPSIDE DOWN.",
    "",
    "ECHO UNIT WILL RETURN.",
};
const int EKO_ENDING_LINES = ARRAY_LEN(EKO_ENDING);

/* ---- small services -------------------------------------------------------------- */

static void set_state(int s) {
    eg.state = s;
    eg.state_t = 0;
}

void eko_sfx(const char *name) {
    if (!eko_simulating) sfx_play_name(name);
}

void eko_say(const char *m) {
    snprintf(eg.msg, sizeof eg.msg, "%s", m);
    eg.msg_t = 100;
}

void eko_burst(int x, int y, int col, int n) {
    if (eko_simulating) return;
    for (int k = 0; k < n; k++)
        for (int i = 0; i < EKO_MAX_PARTS; i++)
            if (eg.part[i].life <= 0) {
                int a = rng_range(&eg.rng, 0, 63);
                int sp = rng_range(&eg.rng, 128, 512);
                eg.part[i] = (EkoPart){x * EKO_FX, y * EKO_FX, eko_cos(a) * sp / 256, eko_sin(a) * sp / 256, rng_range(&eg.rng, 12, 28), col};
                break;
            }
}

void eko_parts_update(void) {
    for (int i = 0; i < EKO_MAX_PARTS; i++)
        if (eg.part[i].life > 0) {
            eg.part[i].life--;
            eg.part[i].x += eg.part[i].vx;
            eg.part[i].y += eg.part[i].vy;
            eg.part[i].vy += 16;
        }
}

int eko_hp(void) { return eg.owl[eg.cur_owl].hp; }

void eko_set_hp(int hp) { eg.owl[eg.cur_owl].hp = iclamp(hp, 0, EKO_OWL_HP[eg.cur_owl]); }

void eko_save_now(void) {
    egs.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &egs, (int)sizeof egs);
}

static void load_save(void) {
    EkoSave tmp;
    memset(&tmp, 0, sizeof tmp);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) { egs = tmp; return; }
    memset(&egs, 0, sizeof egs);
    egs.magic = SAVE_MAGIC;
}

static void goals_from_save(void) {
    if (egs.rescues > 0) game_award(GOAL_BEACON);
    if (egs.wins > 0) game_award(GOAL_SAUCER);
    if (egs.cherries > 0) game_award(GOAL_ALIEN);
}

void eko_add_score(int pts) {
    if (pts <= 0 || eko_simulating) {
        if (pts > 0) { eg.score += pts; }
        return;
    }
    eg.score += pts;
    eg.run_score += pts;
    while (eg.score >= eg.next_life) {
        eg.next_life += EKO_LIFE_EVERY;
        EkoOwlState *o = &eg.owl[eg.cur_owl];
        if (o->lives < EKO_MAX_LIVES) {
            o->lives++;
            eko_sfx("eko_1up");
            eko_say("EXTRA LIFE");
        }
    }
}

void eko_hurt(int dmg) {
    if (eg.god) return;
    EkoOwlState *o = &eg.owl[eg.cur_owl];
    if (o->hp <= 0) return;
    eg.hits_taken++;
    o->hp = imax(0, o->hp - dmg);
    if (o->hp == 0) eg.lives_lost++; /* a whole health bar gone: the cherry is lost */
    else eko_sfx("eko_ouch");
}

/* ---- missions and sections ---------------------------------------------------------- */

static void start_section(void) {
    const EkoSection *s = &EKO_MISSION[eg.mission].sec[eg.sec];
    eg.mode = s->mode;
    eg.cur_owl = s->owl;
    eg.msg_t = 0;
    memset(eg.part, 0, sizeof eg.part);
    switch (s->mode) {
    case MODE_BRAWL:
        eko_brawl_start(s->data);
        music_play(s->data == BM_LAB ? EKO_MUS_LAB : s->data == BM_LIFT ? EKO_MUS_LIFT : EKO_MUS_DOCKS);
        break;
    case MODE_GALLERY:
        eko_gallery_start(s->data);
        music_play(s->data == GS_ANDES ? EKO_MUS_ANDES : EKO_MUS_CALDERA);
        break;
    case MODE_BIKE:
        eko_bike_start(s->data);
        music_play(s->data == BC_CAIRO ? EKO_MUS_CAIRO : EKO_MUS_FLIGHT);
        break;
    case MODE_SNEAK:
        if (eg.mission == MI_CALDERA) {
            if (s->data == 0) eko_sneak_start(SM_CALDERA, 0);
            else eko_sneak_resume(s->data);
            music_play(EKO_MUS_CALDERA);
        } else {
            eko_sneak_start(SM_ISTANBUL, 0);
            music_play(EKO_MUS_ISTANBUL);
        }
        break;
    default: break;
    }
    eg.in = eg.prev = 0;
    set_state(ES_PLAY);
    game_set_pausable(true);
}

static void begin_mission(int m) {
    eg.mission = m;
    eg.sec = 0;
    eg.score = 0;
    eg.next_life = EKO_LIFE_EVERY;
    eg.in_finale = m == MI_CALDERA;
    if (m == MI_CALDERA) {
        for (int i = 0; i < OWL_COUNT; i++) { eg.owl[i].lives = EKO_LIVES; eg.owl[i].hp = EKO_OWL_HP[i]; }
    } else {
        eg.owl[m].lives = EKO_LIVES;
        eg.owl[m].hp = EKO_OWL_HP[m];
    }
    set_state(ES_CARD);
}

void eko_section_done(void) {
    if (eg.state != ES_PLAY) return;
    if (eko_simulating) { eko_sim_done = true; return; }
    const EkoSection *s = &EKO_MISSION[eg.mission].sec[eg.sec];
    if (eg.mission == MI_CALDERA) {
        if (s->mode == MODE_BRAWL) eg.sn.cards |= CARD_RED;
        if (s->mode == MODE_BIKE) eg.sn.cards |= CARD_BLUE;
    }
    eg.sec++;
    if (eg.sec < EKO_MISSION[eg.mission].nsec) {
        set_state(ES_CARD);
        game_set_pausable(false);
        return;
    }
    /* the mission is done */
    if (eg.mission < MI_CALDERA) {
        eg.owl[eg.mission].cleared = true;
        if (eg.mission < 3 && (uint32_t)eg.score > egs.best[eg.mission]) egs.best[eg.mission] = (uint32_t)eg.score;
        eko_save_now();
    } else {
        eg.won = true;
        if (egs.wins < 65535) egs.wins++;
        game_award(GOAL_SAUCER);
        if (eg.lives_lost == 0 && !eg.god) {
            if (egs.cherries < 65535) egs.cherries++;
            game_award(GOAL_ALIEN);
        }
        eko_save_now();
    }
    set_state(ES_CLEAR);
    game_set_pausable(false);
    music_play(EKO_MUS_CLEAR);
}

static bool any_free(void) {
    for (int i = 0; i < OWL_COUNT; i++) if (!eg.owl[i].captured) return true;
    return false;
}

void eko_life_lost(void) {
    if (eg.state != ES_PLAY) return;
    EkoOwlState *o = &eg.owl[eg.cur_owl];
    if (eko_simulating && o->lives <= 1) { o->lives = 0; set_state(ES_OVER); return; }
    o->lives--;
    if (o->lives > 0) {
        o->hp = EKO_OWL_HP[eg.cur_owl];
        switch (eg.mode) {
        case MODE_BRAWL: eko_brawl_respawn(); break;
        case MODE_GALLERY: eg.ga.dead_t = 0; eg.ga.inv = 120; break;
        case MODE_BIKE:
            eg.bk.dead_t = 0; eg.bk.inv = 120; eg.bk.heat = 0; eg.bk.held_t = 0; eg.bk.z = 0; eg.bk.vz = 0;
            eg.bk.hot = 0; eg.bk.hot_held = 0;
            for (int i = 0; i < BK_MAX_SHOTS; i++) eg.bk.shot[i].alive = 0;
            break;
        case MODE_SNEAK: eko_sneak_respawn(); break;
        default: break;
        }
        return;
    }
    o->lives = 0;
    game_set_pausable(false);
    if (eg.in_finale) {
        /* the Caldera gives no second chances */
        set_state(ES_OVER);
        music_play(EKO_MUS_OVER);
        return;
    }
    o->captured = true;
    set_state(ES_CAPTURED);
    music_play(EKO_MUS_CAPTURED);
}

/* ---- the jailbreak ----------------------------------------------------------------------- */

static void start_rescue(int captive, int rescuer) {
    eg.rescue_for = captive;
    eg.rescue_by = rescuer;
    eg.mode = MODE_RESCUE;
    eg.cur_owl = rescuer;
    uint32_t seed = rng_next(&eg.rng);
    eko_rescue_gen(&eg.rs, seed & 0xFFFFFF, rescuer, captive);
    eg.in = eg.prev = 0;
    eg.msg_t = 0;
    set_state(ES_PLAY);
    game_set_pausable(true);
    music_play(EKO_MUS_RESCUE);
}

static int rescue_rep;
static void rescue_update(void) {
    EkoRescue *r = &eg.rs;
    if (r->shot_t) r->shot_t--;
    if (r->boom_t) r->boom_t--;
    if (r->noise_t) r->noise_t--;
    if (r->won || r->lost) {
        r->msg_t++;
        if (r->msg_t == 1) eko_sfx(r->won ? "eko_free" : "eko_down");
        if (r->msg_t > 70) {
            game_set_pausable(false);
            if (r->won) {
                eg.owl[eg.rescue_for].captured = false;
                if (egs.rescues < 65535) egs.rescues++;
                game_award(GOAL_BEACON);
                eko_save_now();
                set_state(ES_RESCUE_WON);
                music_play(EKO_MUS_CLEAR);
            } else {
                eg.owl[eg.rescue_by].captured = true;
                eg.lives_lost++;
                set_state(any_free() ? ES_RESCUE_LOST : ES_OVER);
                music_play(any_free() ? EKO_MUS_CAPTURED : EKO_MUS_OVER);
            }
        }
        return;
    }
    int act = RA_NONE;
    static const int DIRB[4] = {BTN_RIGHT, BTN_DOWN, BTN_LEFT, BTN_UP};
    for (int d = 0; d < 4; d++) {
        if (ek_btnp(DIRB[d])) { act = RA_RIGHT + d; rescue_rep = 0; }
        else if (ek_btn(DIRB[d]) && act == RA_NONE) {
            /* held: steps again after a moment */
            if (++rescue_rep > 16 && rescue_rep % 8 == 0) act = RA_RIGHT + d;
        }
    }
    if (ek_btnp(EKO_MAIN)) act = RA_A;
    if (ek_btnp(EKO_ALT)) act = RA_B;
    if (!(eg.in & (EKO_DIRS))) rescue_rep = 0;
    if (act != RA_NONE) {
        int t0 = r->turns, hp0 = r->hp, gs0 = r->gshots, ff0 = r->ff, gr0 = r->grens, st0 = r->strikes, kos0 = r->kos;
        int cloak0 = r->cloak;
        bool card0 = r->card;
        int al0 = 0, al1 = 0;
        for (int i = 0; i < r->ng; i++) al0 += r->g[i].alive && r->g[i].flash;
        eko_rescue_turn(r, act);
        if (r->turns == t0 && act >= RA_A)
            eko_sfx(cloak0 && !r->cloak ? "eko_cloak" : "eko_tink"); /* not ready (nothing happens), or no room to roll or dash */
        if (r->turns != t0) {
            static const char *const SFX[OWL_COUNT][2] = {{"eko_swing", "eko_clang"}, {"eko_gun", "eko_roll"}, {"eko_gun", "eko_rev"}, {"eko_dart", "eko_cloak"}};
            eko_sfx(act >= RA_A ? SFX[r->owl][act - RA_A] : r->strikes != st0 ? "eko_hit" : "eko_step");
        }
        for (int i = 0; i < r->ng; i++) al1 += r->g[i].alive && r->g[i].flash;
        if (r->gshots != gs0) eko_sfx("eko_shot");
        if (r->grens != gr0) eko_sfx("eko_toss");
        if (r->boom_t == 14) { eko_sfx("eko_boom"); eg.shake = 8; }
        if (al1 > al0) eko_sfx("eko_aim");
        if (r->kos != kos0) eko_sfx("eko_ko");
        if (r->hp < hp0) eko_sfx("eko_ouch");
        if (r->ff != ff0) eko_say("THEY HIT THEIR OWN!");
        if (r->card && !card0) { eko_sfx("eko_key"); eko_say("THE CARD: NOW BACK TO THE CELL"); }
    }
}

/* ---- one frame of play ------------------------------------------------------------------ */

void eko_step_play(void) {
    if (eg.msg_t > 0) eg.msg_t--;
    if (eg.shake > 0) eg.shake--;
    switch (eg.mode) {
    case MODE_BRAWL: eko_brawl_update(); break;
    case MODE_GALLERY: eko_gallery_update(); break;
    case MODE_BIKE: eko_bike_update(); break;
    case MODE_SNEAK: eko_sneak_update(); break;
    case MODE_RESCUE: rescue_update(); break;
    default: break;
    }
    eko_parts_update();
}

/* ---- the run ------------------------------------------------------------------------------ */

static void new_run(void) {
    memset(eg.owl, 0, sizeof eg.owl);
    for (int i = 0; i < OWL_COUNT; i++) { eg.owl[i].lives = EKO_LIVES; eg.owl[i].hp = EKO_OWL_HP[i]; }
    rng_seed(&eg.rng, ((uint64_t)rng_next(&g_rng) << 32) | rng_next(&g_rng));
    eg.lives_lost = 0;
    eg.run_score = 0;
    eg.won = false;
    eg.in_finale = false;
    eg.hits_taken = 0;
    eg.hq_sel = 0;
    if (egs.runs < 65535) egs.runs++;
    eko_save_now();
}

static void to_title(void) {
    set_state(ES_TITLE);
    game_set_pausable(false);
    music_play(EKO_MUS_TITLE);
}

static void to_hq(void) {
    set_state(ES_HQ);
    game_set_pausable(false);
    music_play(EKO_MUS_HQ);
}

static bool caldera_open(void) {
    for (int i = 0; i < OWL_COUNT; i++) if (!eg.owl[i].cleared || eg.owl[i].captured) return false;
    return true;
}

static void hq_update(void) {
    if (btnp(BTN_UP) || btnp(BTN_LEFT)) { eg.hq_sel = (eg.hq_sel + 4) % 5; sfx_play_name("ui_move"); }
    if (btnp(BTN_DOWN) || btnp(BTN_RIGHT)) { eg.hq_sel = (eg.hq_sel + 1) % 5; sfx_play_name("ui_move"); }
    if (!btnp(BTN_A) || eg.state_t < 10) return;
    input_consume();
    int s = eg.hq_sel;
    if (s == 4) {
        if (!caldera_open()) { sfx_play_name("ui_back"); eko_say("ALL FOUR MISSIONS FIRST"); return; }
        sfx_play_name("ui_ok");
        eg.mission = MI_CALDERA;
        set_state(ES_BRIEF);
        music_play(EKO_MUS_BRIEF);
        return;
    }
    EkoOwlState *o = &eg.owl[s];
    if (o->captured) {
        sfx_play_name("ui_ok");
        eg.rescue_for = s;
        eg.pick_sel = 0;
        while (eg.pick_sel < OWL_COUNT && eg.owl[eg.pick_sel].captured) eg.pick_sel++;
        set_state(ES_PICK_RESCUER);
        return;
    }
    if (o->cleared) { sfx_play_name("ui_back"); eko_say("MISSION ALREADY DONE"); return; }
    sfx_play_name("ui_ok");
    eg.mission = s;
    set_state(ES_BRIEF);
    music_play(EKO_MUS_BRIEF);
}

static void eko_update(void) {
    eg.state_t++;
    eg.frame_t++;
    switch (eg.state) {
    case ES_TITLE:
        game_set_pausable(false);
        if (btnp(BTN_B)) { game_exit_to_library(); break; }
        if ((btnp(BTN_A) || btnp(BTN_START)) && eg.state_t > 10) {
            sfx_play_name("ui_ok");
            input_consume();
            set_state(ES_STORY);
            music_play(EKO_MUS_STORY);
        }
        break;
    case ES_STORY:
        if ((eg.state_t > 20 && (btnp(BTN_A) || btnp(BTN_START))) || eg.state_t > 900) {
            input_consume();
            new_run();
            to_hq();
        }
        break;
    case ES_HQ:
        if (eg.msg_t > 0) eg.msg_t--;
        hq_update();
        break;
    case ES_PICK_RESCUER:
        if (btnp(BTN_UP) || btnp(BTN_LEFT) || btnp(BTN_DOWN) || btnp(BTN_RIGHT)) {
            int d = btnp(BTN_UP) || btnp(BTN_LEFT) ? 3 : 1;
            for (int k = 0; k < OWL_COUNT; k++) {
                eg.pick_sel = (eg.pick_sel + d) % OWL_COUNT;
                if (!eg.owl[eg.pick_sel].captured) break;
            }
            sfx_play_name("ui_move");
        }
        if (btnp(BTN_B)) { input_consume(); sfx_play_name("ui_back"); set_state(ES_HQ); break; }
        if (btnp(BTN_A) && eg.state_t > 10 && !eg.owl[eg.pick_sel].captured) {
            input_consume();
            sfx_play_name("ui_ok");
            start_rescue(eg.rescue_for, eg.pick_sel);
        }
        break;
    case ES_BRIEF:
        if (eg.state_t > 30 && (btnp(BTN_A) || btnp(BTN_START))) {
            input_consume();
            begin_mission(eg.mission);
        }
        if (btnp(BTN_B) && eg.mission != MI_CALDERA) { input_consume(); to_hq(); }
        break;
    case ES_CARD:
        game_set_pausable(false);
        if (eg.state_t > 100 || (eg.state_t > 20 && btnp(BTN_A))) {
            input_consume();
            start_section();
        }
        break;
    case ES_PLAY:
        eg.prev = eg.in;
        eg.in = (uint16_t)(input_held() & 0xFF);
        eko_step_play();
        break;
    case ES_CLEAR:
        if (eg.state_t > 60 && btnp(BTN_A)) {
            input_consume();
            if (eg.won) { set_state(ES_ENDING); music_play(EKO_MUS_ENDING); }
            else to_hq();
        }
        break;
    case ES_CAPTURED: case ES_RESCUE_WON: case ES_RESCUE_LOST:
        if (eg.state_t > 60 && btnp(BTN_A)) {
            input_consume();
            if (!any_free()) { set_state(ES_OVER); music_play(EKO_MUS_OVER); }
            else to_hq();
        }
        break;
    case ES_OVER:
        if (eg.state_t > 90 && (btnp(BTN_A) || btnp(BTN_START))) { input_consume(); to_title(); }
        break;
    case ES_ENDING:
        if (btn(BTN_A)) eg.state_t += 2;
        if (eg.state_t > 1500) { input_consume(); to_title(); }
        break;
    default: break;
    }
}

/* ---- cartridge interface ------------------------------------------------------------------- */

static void eko_load(void) {
    eko_art_load();
    eko_audio_load();
}

static void eko_start(void) {
    load_save();
    memset(&eg, 0, sizeof eg);
    rng_seed(&eg.rng, 50);
    goals_from_save();
    to_title();
}

static void eko_quit(void) { eko_save_now(); }

static int count_guards(int state) {
    int n = 0;
    for (int i = 0; i < eg.sn.ng; i++) n += eg.sn.g[i].alive && (state < 0 || eg.sn.g[i].state == state) &&
                                         (eg.sn.g[i].kind == GK_WALKER || eg.sn.g[i].kind == GK_TURNER);
    return n;
}

static int eko_query(const char *key, int *out) {
    const EkoBrawl *b = &eg.br;
    const EkoGallery *g = &eg.ga;
    const EkoBike *k = &eg.bk;
    const EkoSneak *s = &eg.sn;
    const EkoRescue *r = &eg.rs;
    if (!strcmp(key, "bot")) { *out = eko_bot_buttons(); return 1; }
    if (!strcmp(key, "state")) { *out = eg.state; return 1; }
    if (!strcmp(key, "mission")) { *out = eg.mission; return 1; }
    if (!strcmp(key, "sec")) { *out = eg.sec; return 1; }
    if (!strcmp(key, "mode")) { *out = eg.mode; return 1; }
    if (!strcmp(key, "owl")) { *out = eg.cur_owl; return 1; }
    if (!strcmp(key, "lives")) { *out = eg.owl[eg.cur_owl].lives; return 1; }
    if (!strcmp(key, "hp")) { *out = eg.owl[eg.cur_owl].hp; return 1; }
    if (!strcmp(key, "score")) { *out = eg.score; return 1; }
    if (!strcmp(key, "run_score")) { *out = eg.run_score; return 1; }
    if (!strcmp(key, "lives_lost")) { *out = eg.lives_lost; return 1; }
    if (!strcmp(key, "won")) { *out = eg.won; return 1; }
    if (!strcmp(key, "hq_sel")) { *out = eg.hq_sel; return 1; }
    if (!strcmp(key, "finale")) { *out = eg.in_finale; return 1; }
    if (!strncmp(key, "captured", 8)) { *out = eg.owl[iclamp(atoi(key + 8), 0, 3)].captured; return 1; }
    if (!strncmp(key, "cleared", 7)) { *out = eg.owl[iclamp(atoi(key + 7), 0, 3)].cleared; return 1; }
    if (!strncmp(key, "olives", 6)) { *out = eg.owl[iclamp(atoi(key + 6), 0, 3)].lives; return 1; }
    if (!strcmp(key, "save_rescues")) { *out = egs.rescues; return 1; }
    if (!strcmp(key, "save_wins")) { *out = egs.wins; return 1; }
    if (!strcmp(key, "save_cherries")) { *out = egs.cherries; return 1; }
    if (!strcmp(key, "save_runs")) { *out = egs.runs; return 1; }
    if (!strncmp(key, "save_best", 9)) { *out = (int)egs.best[iclamp(atoi(key + 9), 0, 2)]; return 1; }
    if (!strcmp(key, "art_bad")) { *out = eko_art_check(); return 1; }
    if (!strcmp(key, "layout")) { *out = eko_layout_audit(); return 1; }
    /* brawler */
    if (!strcmp(key, "b_px")) { *out = EKO_PX(b->p.x); return 1; }
    if (!strcmp(key, "b_py")) { *out = EKO_PX(b->p.y); return 1; }
    if (!strcmp(key, "b_vy")) { *out = b->p.vy; return 1; }
    if (!strcmp(key, "b_ground")) { *out = b->p.ground; return 1; }
    if (!strcmp(key, "b_act")) { *out = b->p.act; return 1; }
    if (!strcmp(key, "b_charge")) { *out = b->p.charge; return 1; }
    if (!strcmp(key, "b_face")) { *out = b->p.face; return 1; }
    if (!strcmp(key, "b_vx")) { *out = b->p.vx; return 1; }
    if (!strcmp(key, "b_keylost")) { *out = b->key_lost; return 1; }
    if (!strcmp(key, "b_duck")) { *out = b->p.duck; return 1; }
    if (!strcmp(key, "b_carry")) { *out = b->p.carry; return 1; }
    if (!strcmp(key, "b_key")) { *out = b->p.key; return 1; }
    if (!strcmp(key, "b_kills")) { *out = b->kills; return 1; }
    if (!strcmp(key, "b_secrets")) { *out = b->secrets; return 1; }
    if (!strcmp(key, "b_respawns")) { *out = b->respawns; return 1; }
    if (!strcmp(key, "b_hazards")) { *out = b->hazards; return 1; }
    if (!strcmp(key, "b_gulls")) { *out = b->gulls; return 1; }
    if (!strcmp(key, "b_gulls_hit")) { *out = b->gulls_hit; return 1; }
    if (!strcmp(key, "b_kicked")) { int n = 0; for (int i = 0; i < BR_MAX_OBJS; i++) n += b->obj[i].alive && b->obj[i].kind == BO_GRENADE && b->obj[i].state; *out = n; return 1; }
    if (!strcmp(key, "b_rats")) { int n = 0; for (int i = 0; i < BR_MAX_FOES; i++) n += b->foe[i].alive && b->foe[i].kind == BF_RAT; *out = n; return 1; }
    if (!strcmp(key, "b_boss")) { *out = b->boss_on; return 1; }
    if (!strcmp(key, "b_boss_dead")) { *out = b->boss_dead; return 1; }
    if (!strcmp(key, "b_lift_t")) { *out = b->lift_t; return 1; }
    if (!strcmp(key, "b_wall")) { *out = b->wall_open_t; return 1; }
    if (!strcmp(key, "b_cam_x")) { *out = b->cam_x; return 1; }
    if (!strcmp(key, "b_mw")) { *out = b->mw * 16; return 1; }
    if (!strcmp(key, "b_mh")) { *out = b->mh * 16; return 1; }
    if (!strcmp(key, "b_dead")) { *out = b->p.dead_t; return 1; }
    if (!strcmp(key, "b_foes")) { int n = 0; for (int i = 0; i < BR_MAX_FOES; i++) n += b->foe[i].alive; *out = n; return 1; }
    if (!strncmp(key, "b_foe", 5) && key[5] >= '0' && key[5] <= '9') {
        int i = atoi(key + 5);
        const char *u = strchr(key + 5, '_');
        if (i < 0 || i >= BR_MAX_FOES || !u) return 0;
        const EkoBFoe *f = &b->foe[i];
        if (!strcmp(u, "_alive")) { *out = f->alive; return 1; }
        if (!strcmp(u, "_hp")) { *out = f->hp; return 1; }
        if (!strcmp(u, "_x")) { *out = EKO_PX(f->x); return 1; }
        if (!strcmp(u, "_state")) { *out = f->state; return 1; }
        return 0;
    }
    if (!strncmp(key, "b_objs", 6)) {
        int kind = key[6] == '_' ? atoi(key + 7) : -1, n = 0;
        for (int i = 0; i < BR_MAX_OBJS; i++) n += b->obj[i].alive && (kind < 0 || b->obj[i].kind == kind);
        *out = n;
        return 1;
    }
    if (!strcmp(key, "b_key_x")) {
        *out = -1;
        for (int i = 0; i < BR_MAX_OBJS; i++) if (b->obj[i].alive && b->obj[i].kind == BO_KEY) { *out = EKO_PX(b->obj[i].x); break; }
        return 1;
    }
    if (!strcmp(key, "b_key_y")) {
        *out = -1;
        for (int i = 0; i < BR_MAX_OBJS; i++) if (b->obj[i].alive && b->obj[i].kind == BO_KEY) { *out = EKO_PX(b->obj[i].y); break; }
        return 1;
    }
    if (!strcmp(key, "b_gren_x") || !strcmp(key, "b_gren_y")) {
        *out = -1;
        for (int i = 0; i < BR_MAX_OBJS; i++) if (b->obj[i].alive && b->obj[i].kind == BO_GRENADE) { *out = EKO_PX(key[7] == 'x' ? b->obj[i].x : b->obj[i].y); break; }
        return 1;
    }
    if (!strcmp(key, "b_barrel_state")) {
        *out = -1;
        for (int i = 0; i < BR_MAX_OBJS; i++) if (b->obj[i].alive && b->obj[i].kind == BO_BARREL) { *out = b->obj[i].state; break; }
        return 1;
    }
    /* gallery */
    if (!strcmp(key, "g_px")) { *out = EKO_PX(g->px); return 1; }
    if (!strcmp(key, "g_cx")) { *out = EKO_PX(g->cx); return 1; }
    if (!strcmp(key, "g_cy")) { *out = EKO_PX(g->cy); return 1; }
    if (!strcmp(key, "g_locked")) { *out = g->held_t >= GA_LOCK_T; return 1; }
    if (!strcmp(key, "g_shots")) { *out = g->shots_fired; return 1; }
    if (!strcmp(key, "g_hits")) { *out = g->hits; return 1; }
    if (!strcmp(key, "g_combo")) { *out = g->combo; return 1; }
    if (!strcmp(key, "g_kills")) { *out = g->kills; return 1; }
    if (!strcmp(key, "g_roll")) { *out = g->roll_t; return 1; }
    if (!strcmp(key, "g_t")) { *out = g->t; return 1; }
    if (!strcmp(key, "g_boss")) { *out = g->boss_on; return 1; }
    if (!strcmp(key, "g_boss_dead")) { *out = g->boss_dead; return 1; }
    if (!strcmp(key, "g_foes")) { int n = 0; for (int i = 0; i < GA_MAX_FOES; i++) n += g->foe[i].alive; *out = n; return 1; }
    if (!strncmp(key, "g_kind", 6)) {
        int kd = atoi(key + 6), n = 0;
        for (int i = 0; i < GA_MAX_FOES; i++) n += g->foe[i].alive && g->foe[i].kind == kd;
        *out = n;
        return 1;
    }
    if (!strncmp(key, "g_hpk", 5)) {
        int kd = atoi(key + 5);
        *out = -1;
        for (int i = 0; i < GA_MAX_FOES; i++) if (g->foe[i].alive && g->foe[i].kind == kd) { *out = g->foe[i].hp; break; }
        return 1;
    }
    if (!strcmp(key, "g_eshots")) { int n = 0; for (int i = 0; i < GA_MAX_SHOTS; i++) n += g->shot[i].alive && g->shot[i].kind != 3; *out = n; return 1; }
    if (!strcmp(key, "g_carry")) { *out = g->carry; return 1; }
    if (!strcmp(key, "g_grabs")) { *out = g->grabs; return 1; }
    if (!strcmp(key, "g_throws")) { *out = g->throws; return 1; }
    if (!strcmp(key, "g_grenades")) { int n = 0; for (int i = 0; i < GA_MAX_SHOTS; i++) n += g->shot[i].alive && g->shot[i].kind == 1 && g->shot[i].t >= g->shot[i].dur; *out = n; return 1; }
    if (!strcmp(key, "g_mangos")) { int n = 0; for (int i = 0; i < GA_MAX_SHOTS; i++) n += g->shot[i].alive && g->shot[i].kind == 3; *out = n; return 1; }
    /* bike */
    if (!strcmp(key, "k_x")) { *out = EKO_PX(k->x); return 1; }
    if (!strcmp(key, "k_y")) { *out = EKO_PX(k->y); return 1; }
    if (!strcmp(key, "k_z")) { *out = EKO_PX(k->z); return 1; }
    if (!strcmp(key, "k_heat")) { *out = k->heat; return 1; }
    if (!strcmp(key, "k_dist")) { *out = k->dist; return 1; }
    if (!strcmp(key, "k_shots")) { *out = k->shots_fired; return 1; }
    if (!strcmp(key, "k_kills")) { *out = k->kills; return 1; }
    if (!strcmp(key, "k_overheat")) { *out = k->overheat; return 1; }
    if (!strcmp(key, "k_hot")) { *out = k->hot; return 1; }
    if (!strcmp(key, "k_dead")) { *out = k->dead_t; return 1; }
    if (!strcmp(key, "k_boss")) { *out = k->boss_on; return 1; }
    if (!strcmp(key, "k_boss_dead")) { *out = k->boss_dead; return 1; }
    if (!strcmp(key, "k_card")) { *out = k->card; return 1; }
    if (!strcmp(key, "k_length")) { *out = EKO_KCOURSE[k->course].length; return 1; }
    if (!strncmp(key, "k_kind", 6)) {
        int kd = atoi(key + 6), n = 0;
        for (int i = 0; i < BK_MAX_OBJS; i++) n += k->obj[i].alive && k->obj[i].kind == kd;
        *out = n;
        return 1;
    }
    /* stealth */
    if (!strcmp(key, "s_x")) { *out = EKO_PX(s->x); return 1; }
    if (!strcmp(key, "s_y")) { *out = EKO_PX(s->y); return 1; }
    if (!strcmp(key, "s_dir")) { *out = s->dir; return 1; }
    if (!strcmp(key, "s_room_x")) { *out = s->room_x; return 1; }
    if (!strcmp(key, "s_room_y")) { *out = s->room_y; return 1; }
    if (!strcmp(key, "s_weapon")) { *out = s->weapon; return 1; }
    if (!strcmp(key, "s_next")) {
        /* the item B picks next, as the kit at the bottom left marks it (-1 none) */
        *out = -1;
        for (int k = 1; k < WP_COUNT && *out < 0; k++) if (s->ammo[(s->weapon + k) % WP_COUNT] > 0) *out = (s->weapon + k) % WP_COUNT;
        return 1;
    }
    if (!strcmp(key, "s_shotvx") || !strcmp(key, "s_shotvy")) {
        *out = 0;
        for (int i = 0; i < SN_MAX_SHOTS; i++)
            if (s->shot[i].alive && !s->shot[i].foe) { *out = key[7] == 'x' ? s->shot[i].vx : s->shot[i].vy; break; }
        return 1;
    }
    if (!strncmp(key, "s_ammo", 6)) { *out = s->ammo[iclamp(atoi(key + 6), 0, 3)]; return 1; }
    if (!strcmp(key, "s_cards")) { *out = s->cards; return 1; }
    if (!strcmp(key, "s_spotted")) { *out = s->spotted_total; return 1; }
    if (!strcmp(key, "s_room_spotted")) { *out = s->room_spotted; return 1; }
    if (!strcmp(key, "s_room_noise")) { *out = s->room_noise; return 1; }
    if (!strcmp(key, "s_clean")) { *out = s->rooms_clean; return 1; }
    if (!strcmp(key, "s_boss_dead")) { *out = s->boss_dead; return 1; }
    if (!strcmp(key, "s_relays")) { *out = s->used_relays; return 1; }
    if (!strcmp(key, "s_alarm")) { *out = s->alarm; return 1; }
    if (!strcmp(key, "s_dead")) { *out = s->dead_t; return 1; }
    if (!strcmp(key, "s_alert")) { *out = count_guards(GD_ALERT); return 1; }
    if (!strcmp(key, "s_asleep")) { *out = count_guards(GD_SLEEP); return 1; }
    if (!strcmp(key, "s_stunned")) { *out = count_guards(GD_STUN); return 1; }
    if (!strcmp(key, "s_looking")) { *out = count_guards(GD_INVESTIGATE) + count_guards(GD_LOOK); return 1; }
    if (!strcmp(key, "s_guards")) { *out = count_guards(-1); return 1; }
    if (!strcmp(key, "s_boss_hp")) { *out = s->boss_g >= 0 ? s->g[s->boss_g].hp : -1; return 1; }
    if (!strcmp(key, "s_boss_state")) { *out = s->boss_g >= 0 ? s->g[s->boss_g].state : -1; return 1; }
    if (!strncmp(key, "s_tile_", 7)) {
        int tx = atoi(key + 7);
        const char *u = strchr(key + 7, '_');
        int ty = u ? atoi(u + 1) : 0;
        *out = tx >= 0 && ty >= 0 && tx < s->mw && ty < s->mh ? s->tile[ty][tx] : 0;
        return 1;
    }
    if (!strcmp(key, "s_items_left")) { int n = 0; for (int i = 0; i < s->nitem; i++) n += !s->item[i].taken; *out = n; return 1; }
    if (!strcmp(key, "s_rooms")) {
        int n = 0;
        for (int y = 0; y < SN_MAX_RY; y++) for (int x = 0; x < SN_MAX_RX; x++) n += s->tile[y * SN_RH + 1][x * SN_RW + 1] != ' ' && x * SN_RW < s->mw && y * SN_RH < s->mh;
        *out = n;
        return 1;
    }
    if (!strncmp(key, "s_g", 3) && key[3] >= '0' && key[3] <= '9') {
        int i = atoi(key + 3);
        const char *u = strchr(key + 3, '_');
        if (i < 0 || i >= SN_MAX_GUARDS || !u) return 0;
        const EkoSGuard *q = &s->g[i];
        if (!strcmp(u, "_state")) { *out = q->alive ? q->state : GD_DEAD; return 1; }
        if (!strcmp(u, "_kind")) { *out = q->kind; return 1; }
        if (!strcmp(u, "_x")) { *out = EKO_PX(q->x); return 1; }
        if (!strcmp(u, "_y")) { *out = EKO_PX(q->y); return 1; }
        if (!strcmp(u, "_hp")) { *out = q->hp; return 1; }
        return 0;
    }
    /* jailbreak */
    if (!strcmp(key, "r_x")) { *out = r->ox; return 1; }
    if (!strcmp(key, "r_y")) { *out = r->oy; return 1; }
    if (!strcmp(key, "r_hp")) { *out = r->hp; return 1; }
    if (!strcmp(key, "r_cd0")) { *out = r->cd[0]; return 1; }
    if (!strcmp(key, "r_cd1")) { *out = r->cd[1]; return 1; }
    if (!strcmp(key, "r_gshots")) { *out = r->gshots; return 1; }
    if (!strcmp(key, "r_ff")) { *out = r->ff; return 1; }
    if (!strcmp(key, "r_spinning")) { int n = 0; for (int i = 0; i < r->ng; i++) n += r->g[i].alive && r->g[i].state == RGS_SPIN; *out = n; return 1; }
    if (!strcmp(key, "r_firing")) { int n = 0; for (int i = 0; i < r->ng; i++) n += r->g[i].alive && r->g[i].state == RGS_FIRE; *out = n; return 1; }
    if (!strcmp(key, "r_shots")) { int n = 0; for (int i = 0; i < RS_MAX_SHOTS; i++) n += r->shot[i].alive; *out = n; return 1; }
    if (!strcmp(key, "r_grens")) { int n = 0; for (int i = 0; i < RS_MAX_GRENS; i++) n += r->gren[i].alive; *out = n; return 1; }
    if (!strcmp(key, "r_thrown")) { *out = r->grens; return 1; }
    if (!strcmp(key, "r_cloak")) { *out = r->cloak; return 1; }
    if (!strcmp(key, "r_card_hp")) { *out = r->card_hp; return 1; }
    if (!strcmp(key, "r_hits")) { *out = r->hits; return 1; }
    if (!strcmp(key, "r_stabs")) { *out = r->stabs; return 1; }
    if (!strcmp(key, "r_strikes")) { *out = r->strikes; return 1; }
    if (!strcmp(key, "r_rooms")) { *out = r->rooms; return 1; }
    if (!strncmp(key, "r_shot0_", 8)) {
        /* the first shot in flight: its x, y */
        *out = -1;
        for (int i = 0; i < RS_MAX_SHOTS; i++) if (r->shot[i].alive) { *out = key[8] == 'x' ? r->shot[i].x : r->shot[i].y; break; }
        return 1;
    }
    if (!strncmp(key, "r_g", 3) && key[3] >= '0' && key[3] <= '9') {
        int i = atoi(key + 3);
        const char *u = strchr(key + 3, '_');
        if (i < 0 || i >= RS_MAX_GUARDS || !u) return 0;
        const EkoRGuard *q = &r->g[i];
        if (!strcmp(u, "_alive")) { *out = q->alive; return 1; }
        if (!strcmp(u, "_x")) { *out = q->x; return 1; }
        if (!strcmp(u, "_y")) { *out = q->y; return 1; }
        if (!strcmp(u, "_hp")) { *out = q->alive ? q->hp : 0; return 1; }
        if (!strcmp(u, "_state")) { *out = q->state; return 1; }
        if (!strcmp(u, "_sleep")) { *out = q->sleep; return 1; }
        if (!strcmp(u, "_dir")) { *out = q->dir; return 1; }
        return 0;
    }
    if (!strcmp(key, "r_dir")) { *out = r->dir; return 1; }
    if (!strcmp(key, "r_card")) { *out = r->card; return 1; }
    if (!strcmp(key, "r_won")) { *out = r->won; return 1; }
    if (!strcmp(key, "r_lost")) { *out = r->lost; return 1; }
    if (!strcmp(key, "r_turns")) { *out = r->turns; return 1; }
    if (!strncmp(key, "r_tile_", 7)) {
        int tx = atoi(key + 7);
        const char *u = strchr(key + 7, '_');
        int ty = u ? atoi(u + 1) : 0;
        *out = tx >= 0 && ty >= 0 && tx < RS_W && ty < RS_H ? r->tile[ty][tx] : 0;
        return 1;
    }
    if (!strcmp(key, "r_kos")) { *out = r->kos; return 1; }
    if (!strcmp(key, "r_owl")) { *out = r->owl; return 1; }
    if (!strcmp(key, "r_guards")) { int n = 0; for (int i = 0; i < r->ng; i++) n += r->g[i].alive; *out = n; return 1; }
    if (!strcmp(key, "r_alert")) { int n = 0; for (int i = 0; i < r->ng; i++) n += r->g[i].alive && r->g[i].state == RGS_ALERT; *out = n; return 1; }
    if (!strcmp(key, "r_asleep")) { int n = 0; for (int i = 0; i < r->ng; i++) n += r->g[i].alive && r->g[i].state == RGS_SLEEP; *out = n; return 1; }
    if (!strcmp(key, "r_seeking")) { int n = 0; for (int i = 0; i < r->ng; i++) n += r->g[i].alive && r->g[i].state == RGS_SEEK; *out = n; return 1; }
    if (!strncmp(key, "r_botwins_", 10)) {
        extern int eko_bot_debug;
        /* r_botwins_<owl>_<first seed>_<n>: the demo player plays n rolled
         * jails as that bat, turn by turn with the real rules; how many it wins */
        int owl = 0, first = 1, n = 1;
        if (sscanf(key + 10, "%d_%d_%d", &owl, &first, &n) != 3) return 0;
        owl = iclamp(owl, 0, 3);
        int wins = 0;
        static EkoRescue t;
        for (int sd = first; sd < first + n; sd++) {
            if (eko_rescue_gen(&t, (uint32_t)sd, owl, (owl + 1) % 4) < 0) continue;
            for (int turn = 0; turn < 400 && !t.won && !t.lost; turn++) {
                int a = eko_rescue_bot_action(&t);
                if (a == RA_NONE) break;
                eko_rescue_turn(&t, a);
                if (eko_bot_debug > 2 && turn < 60) {
                    printf("    t%d a%d -> (%d,%d) d%d hp %d cd %d %d |", t.turns, a, t.ox, t.oy, t.dir, t.hp, t.cd[0], t.cd[1]);
                    for (int i = 0; i < t.ng; i++) if (t.g[i].alive) printf(" g%d(%d,%d)s%dd%d", i, t.g[i].x, t.g[i].y, t.g[i].state, t.g[i].dir);
                    printf("\n");
                }
            }
            wins += t.won;
            if (eko_bot_debug == 1)
                printf("    jail %d bat %d: %s in %d turns, hits %d, kos %d, ff %d, shots %d, grenades %d, rooms %d, guards %d\n", sd, owl,
                       t.won ? "won" : "lost", t.turns, t.hits, t.kos, t.ff, t.gshots, t.grens, t.rooms, t.ng);
            if (!t.won && eko_bot_debug > 1) {
                /* the board where it gave up: # wall o crate C cell K card, guards by state, @ the bat */
                for (int y = 0; y < RS_H; y++) {
                    char line[RS_W + 1];
                    for (int x = 0; x < RS_W; x++) line[x] = t.tile[y][x];
                    line[RS_W] = 0;
                    for (int i = 0; i < t.ng; i++)
                        if (t.g[i].alive && t.g[i].y == y) line[(int)t.g[i].x] = "PASZXF"[t.g[i].state];
                    if (t.oy == y) line[t.ox] = '@';
                    printf("    %s\n", line);
                }
                printf("    hp %d cd %d %d card %d dir %d\n", t.hp, t.cd[0], t.cd[1], t.card, t.dir);
            }
            if (!t.won && eko_bot_debug) printf("  bat %d lost jail %d (%s, %d turns)\n", owl, sd, t.lost ? "caught" : "stuck", t.turns);
        }
        *out = wins;
        return 1;
    }
    if (!strcmp(key, "r_layouts_bad")) {
        /* 500 rolled jails: every one fair, and no two alike in a row */
        EkoRescue t, prev;
        int bad = 0;
        memset(&prev, 0, sizeof prev);
        for (int sd = 1; sd <= 500; sd++) {
            if (eko_rescue_gen(&t, (uint32_t)sd, sd % 4, (sd + 1) % 4) < 0 || eko_rescue_check(&t)) bad++;
            if (sd > 1 && !memcmp(t.tile, prev.tile, sizeof t.tile)) bad++;
            prev = t;
        }
        *out = bad;
        return 1;
    }
    if (!strcmp(key, "r_layout_variety")) {
        /* 500 rolled jails: which room counts (bits 0-2: one, two, three
         * rooms) and which walls the cell sat in (bits 3-6: left, right,
         * top, bottom) came up, and the five kinds of guard (bits 7-12) */
        EkoRescue t;
        int seen = 0;
        for (int sd = 1; sd <= 500; sd++) {
            if (eko_rescue_gen(&t, (uint32_t)sd, sd % 4, (sd + 1) % 4) < 0) continue;
            seen |= 1 << (iclamp(t.rooms, 1, 3) - 1);
            seen |= 1 << (t.cell_x == 0 ? 3 : t.cell_x == RS_W - 1 ? 4 : t.cell_y == 0 ? 5 : 6);
            for (int i = 0; i < t.ng; i++) seen |= 1 << (7 + t.g[i].kind);
        }
        *out = seen;
        return 1;
    }
    return 0;
}

static int eko_cheat(const char *cmd) {
    int a, b, c, d;
    if (!strcmp(cmd, "run")) { new_run(); to_hq(); return 1; }
    if (sscanf(cmd, "mission %d", &a) == 1) {
        /* straight into a mission (its owl ready), past the briefing */
        if (eg.state == ES_TITLE || eg.state == ES_STORY) new_run();
        eg.owl[iclamp(a, 0, 3)].captured = false;
        begin_mission(iclamp(a, 0, MI_CALDERA));
        start_section();
        return 1;
    }
    if (sscanf(cmd, "section %d", &a) == 1) {
        eg.sec = iclamp(a, 0, EKO_MISSION[eg.mission].nsec - 1);
        if (eg.mission == MI_CALDERA && EKO_MISSION[eg.mission].sec[eg.sec].mode == MODE_SNEAK && eg.sec > 0 && eg.sn.map != SM_CALDERA)
            eko_sneak_start(SM_CALDERA, 0);
        start_section();
        return 1;
    }
    if (sscanf(cmd, "finished %d", &a) == 1) { eg.owl[iclamp(a, 0, 3)].cleared = true; return 1; }
    if (sscanf(cmd, "capture %d", &a) == 1) { eg.owl[iclamp(a, 0, 3)].captured = true; return 1; }
    if (sscanf(cmd, "hp %d", &a) == 1) { eko_set_hp(a); return 1; }
    if (sscanf(cmd, "lives %d", &a) == 1) { eg.owl[eg.cur_owl].lives = a; return 1; }
    if (sscanf(cmd, "score %d", &a) == 1) { eg.score = a; eg.next_life = (a / EKO_LIFE_EVERY + 1) * EKO_LIFE_EVERY; return 1; }
    if (sscanf(cmd, "addscore %d", &a) == 1) { eko_add_score(a); return 1; }
    if (sscanf(cmd, "hqsel %d", &a) == 1) { eg.hq_sel = a; return 1; }
    if (!strcmp(cmd, "hq")) { to_hq(); return 1; }
    if (!strcmp(cmd, "god")) { eg.god = true; return 1; }
    if (!strcmp(cmd, "mortal")) { eg.god = false; return 1; }
    if (!strcmp(cmd, "die")) {
        /* the current bat's health bar emptied, as by a blow */
        eg.god = false;
        eko_hurt(99);
        if (eg.mode == MODE_BRAWL) eg.br.p.dead_t = 1;
        if (eg.mode == MODE_GALLERY) eg.ga.dead_t = 1;
        if (eg.mode == MODE_BIKE) eg.bk.dead_t = 1;
        if (eg.mode == MODE_SNEAK) eg.sn.dead_t = 1;
        return 1;
    }
    if (!strcmp(cmd, "done")) { eko_section_done(); return 1; }
    /* brawler */
    if (sscanf(cmd, "bpos %d %d", &a, &b) == 2) {
        eg.br.p.x = a * EKO_FX; eg.br.p.y = b * EKO_FX; eg.br.p.vy = 0;
        eg.br.p.safe_x = eg.br.p.x; eg.br.p.safe_y = eg.br.p.y;
        eg.br.cam_x = iclamp(a - 140, 0, imax(0, eg.br.mw * 16 - SCREEN_W));
        eg.br.cam_y = iclamp(b - 110, 0, imax(0, eg.br.mh * 16 - BR_SH * 16));
        return 1;
    }
    if (sscanf(cmd, "bfoe %d %d %d", &a, &b, &c) == 3) {
        int f = eko_brawl_spawn_foe(iclamp(a, 1, BF_KINDS - 1), b * EKO_FX, c * EKO_FX);
        if (f >= 0) eg.br.foe[f].cd = 9999;
        return 1;
    }
    if (sscanf(cmd, "bfoeactive %d", &a) == 1) { if (a >= 0 && a < BR_MAX_FOES) eg.br.foe[a].cd = 0; return 1; }
    if (sscanf(cmd, "bobj %d %d %d", &a, &b, &c) == 3) { eko_brawl_add_obj(a, b * EKO_FX, c * EKO_FX, 0, 0); return 1; }
    if (!strcmp(cmd, "bclear")) {
        for (int i = 0; i < BR_MAX_FOES; i++) eg.br.foe[i].alive = 0;
        for (int i = 0; i < eg.br.nspawn; i++) if (eg.br.spawn[i].kind != SPK_DROP && eg.br.spawn[i].kind != SPK_HOOK) eg.br.spawn[i].used = 1;
        for (int i = 0; i < BR_MAX_OBJS; i++) if (eg.br.obj[i].kind != BO_KEY) eg.br.obj[i].alive = 0;
        eg.br.calm = 3; /* and nobody else comes, nothing falls */
        return 1;
    }
    if (!strcmp(cmd, "bwaves")) { eg.br.calm &= ~1; eg.br.respawn_t = BR_RESPAWN_T; return 1; }
    if (!strcmp(cmd, "bhazards")) { eg.br.calm &= ~2; return 1; }
    if (!strcmp(cmd, "bkey")) { eg.br.p.key = true; return 1; }
    if (!strcmp(cmd, "bboss")) { eko_brawl_boss_start(); return 1; }
    if (sscanf(cmd, "bbosshp %d", &a) == 1) { if (eg.br.boss_i >= 0 && eg.br.boss_on) eg.br.foe[eg.br.boss_i].hp = a; return 1; }
    if (sscanf(cmd, "blift %d", &a) == 1) { eg.br.lift_t = a; return 1; }
    /* gallery */
    if (sscanf(cmd, "gfoe %d %d %d", &a, &b, &c) == 3) {
        int f = eko_gallery_spawn(a, b, c, 0);
        if (f >= 0) { eg.ga.foe[f].x = c * EKO_FX; eg.ga.foe[f].state = 1; eg.ga.foe[f].t = 0; }
        return 1;
    }
    if (sscanf(cmd, "gfoestill %d", &a) == 1) { if (a >= 0 && a < GA_MAX_FOES) { eg.ga.foe[a].t = -100000; } return 1; }
    if (!strcmp(cmd, "gclear")) { memset(eg.ga.foe, 0, sizeof eg.ga.foe); memset(eg.ga.shot, 0, sizeof eg.ga.shot); return 1; }
    if (!strcmp(cmd, "gnospawn")) { eg.ga.spawn_i = EKO_GSCRIPT[eg.ga.script].n; return 1; }
    if (sscanf(cmd, "gt %d", &a) == 1) { eg.ga.t = a; return 1; }
    if (!strcmp(cmd, "gboss")) { eko_gallery_boss_start(); return 1; }
    if (sscanf(cmd, "gaim %d %d", &a, &b) == 2) { eg.ga.px = a * EKO_FX; eg.ga.cx = a * EKO_FX; eg.ga.cy = b * EKO_FX; return 1; }
    if (sscanf(cmd, "gpos %d", &a) == 1) { eg.ga.px = a * EKO_FX; eg.ga.cx = a * EKO_FX; return 1; }
    if (sscanf(cmd, "gshot %d %d", &a, &b) == 2) { /* a bullet landing at x = a in b frames */
        for (int i = 0; i < GA_MAX_SHOTS; i++) if (!eg.ga.shot[i].alive) { eg.ga.shot[i] = (EkoGShot){1, 0, a, 60, a, GA_PY - 10, 0, b}; break; }
        return 1;
    }
    if (sscanf(cmd, "ggrenade %d %d", &a, &b) == 2) { /* a lobbed grenade landing at x = a in b frames */
        for (int i = 0; i < GA_MAX_SHOTS; i++) if (!eg.ga.shot[i].alive) { eg.ga.shot[i] = (EkoGShot){1, 1, a, 60, a, GA_PY - 10, 0, b}; break; }
        return 1;
    }
    if (sscanf(cmd, "gmango %d", &a) == 1) { /* a mango lying at x = a */
        for (int i = 0; i < GA_MAX_SHOTS; i++) if (!eg.ga.shot[i].alive) { eg.ga.shot[i] = (EkoGShot){1, 3, a, GA_PY, a, GA_PY, 1, 1}; break; }
        return 1;
    }
    if (sscanf(cmd, "gbosshp %d %d", &a, &b) == 2) {
        for (int i = 0; i < GA_MAX_FOES; i++) if (eg.ga.foe[i].alive && eg.ga.foe[i].kind == a) eg.ga.foe[i].hp = b;
        return 1;
    }
    /* bike */
    if (sscanf(cmd, "kobj %d %d %d", &a, &b, &c) == 3) { eko_bike_spawn(a, eg.bk.dist + b, c, 0); return 1; }
    if (!strcmp(cmd, "kclear")) { memset(eg.bk.obj, 0, sizeof eg.bk.obj); memset(eg.bk.shot, 0, sizeof eg.bk.shot); return 1; }
    if (!strcmp(cmd, "knospawn")) { eg.bk.spawn_i = EKO_KCOURSE[eg.bk.course].n; return 1; }
    if (sscanf(cmd, "kdist %d", &a) == 1) {
        eg.bk.dist = a;
        const EkoKCourse *kc = &EKO_KCOURSE[eg.bk.course];
        while (eg.bk.spawn_i < kc->n && kc->spawns[eg.bk.spawn_i].at <= a) eg.bk.spawn_i++;
        return 1;
    }
    if (sscanf(cmd, "kpos %d %d", &a, &b) == 2) { eg.bk.x = a * EKO_FX; eg.bk.y = b * EKO_FX; return 1; }
    if (sscanf(cmd, "kbosshp %d", &a) == 1) {
        for (int i = 0; i < BK_MAX_OBJS; i++) if (eg.bk.obj[i].alive && eg.bk.obj[i].kind == OB_HOG) eg.bk.obj[i].hp = a;
        return 1;
    }
    /* stealth */
    if (sscanf(cmd, "spos %d %d", &a, &b) == 2) {
        /* tile (a, b) of the whole map */
        eg.sn.x = (a * 16 + 8) * EKO_FX;
        eg.sn.y = (SN_TOP + b * 16 + 8) * EKO_FX;
        eko_sneak_enter_room(a / SN_RW, b / SN_RH);
        return 1;
    }
    if (sscanf(cmd, "sammo %d %d", &a, &b) == 2) { eg.sn.ammo[iclamp(a, 0, 3)] = b; return 1; }
    if (sscanf(cmd, "sweapon %d", &a) == 1) { eg.sn.weapon = iclamp(a, 0, 3); return 1; }
    if (sscanf(cmd, "scards %d", &a) == 1) { eg.sn.cards = a; return 1; }
    if (sscanf(cmd, "sdir %d", &a) == 1) { eg.sn.dir = a & 3; return 1; }
    if (sscanf(cmd, "sguard %d %d %d %d", &a, &b, &c, &d) == 4) {
        /* guard a to tile (b, c), facing d, on its beat there */
        if (a >= 0 && a < eg.sn.ng) {
            EkoSGuard *q = &eg.sn.g[a];
            q->x = q->hx = (b * 16 + 8) * EKO_FX;
            q->y = q->hy = (SN_TOP + c * 16 + 8) * EKO_FX;
            q->dir = q->hdir = d;
            q->room = (uint8_t)((c / SN_RH) * SN_MAX_RX + b / SN_RW);
            q->state = GD_PATROL;
            q->kind = GK_TURNER;
            q->alive = 1;
            q->t = 1;
        }
        return 1;
    }
    if (sscanf(cmd, "sgoff %d", &a) == 1) { if (a >= 0 && a < eg.sn.ng) eg.sn.g[a].alive = 0; return 1; }
    if (!strcmp(cmd, "sroomoff")) {
        /* every guard, camera and light in this room gone */
        int room = eg.sn.room_y * SN_MAX_RX + eg.sn.room_x;
        for (int i = 0; i < eg.sn.ng; i++) if (eg.sn.g[i].room == room && eg.sn.g[i].kind != GK_CIPHER) eg.sn.g[i].alive = 0;
        return 1;
    }
    if (sscanf(cmd, "snoise %d %d", &a, &b) == 2) { eko_sneak_noise(a, b); return 1; }
    if (sscanf(cmd, "sbosshp %d", &a) == 1) { if (eg.sn.boss_g >= 0) eg.sn.g[eg.sn.boss_g].hp = a; return 1; }
    /* jailbreak */
    if (sscanf(cmd, "rescue %d %d %d", &a, &b, &c) == 3) {
        /* a jailbreak of seed c: owl b goes in for owl a */
        if (eg.state == ES_TITLE || eg.state == ES_STORY) new_run();
        eg.owl[iclamp(a, 0, 3)].captured = true;
        eg.rescue_for = iclamp(a, 0, 3);
        eg.rescue_by = iclamp(b, 0, 3);
        eg.mode = MODE_RESCUE;
        eg.cur_owl = eg.rescue_by;
        eko_rescue_gen(&eg.rs, (uint32_t)c, eg.rescue_by, eg.rescue_for);
        eg.in = eg.prev = 0;
        eg.msg_t = 0;
        set_state(ES_PLAY);
        game_set_pausable(true);
        music_play(EKO_MUS_RESCUE);
        return 1;
    }
    if (!strcmp(cmd, "rblank")) {
        /* an empty jail for rule tests (15 x 10, walls round it): the bat at
         * (2, 4) facing right, the cell door at (0, 4), the card at (13, 1) */
        EkoRescue *r = &eg.rs;
        int owl = r->owl, captive = r->captive, seed = r->seed;
        memset(r, 0, sizeof *r);
        r->owl = owl; r->captive = captive; r->seed = seed;
        for (int y = 0; y < RS_H; y++) for (int x = 0; x < RS_W; x++) r->tile[y][x] = (x == 0 || y == 0 || x == RS_W - 1 || y == RS_H - 1) ? '#' : '.';
        r->ox = r->start_x = 2; r->oy = r->start_y = 4; r->dir = RA_RIGHT;
        r->cell_x = 0; r->cell_y = 4; r->tile[4][0] = 'C';
        r->key_x = 13; r->key_y = 1; r->tile[1][13] = 'K';
        r->card_hp = RS_CARD_HP;
        r->rooms = 1;
        r->hp = EKO_RS_HP[r->owl];
        return 1;
    }
    if (sscanf(cmd, "rguard %d %d %d %d", &a, &b, &c, &d) == 4) {
        /* a guard of kind a (RK_*) at (b, c) facing d */
        EkoRescue *r = &eg.rs;
        if (r->ng < RS_MAX_GUARDS) {
            EkoRGuard *q = &r->g[r->ng++];
            memset(q, 0, sizeof *q);
            q->alive = 1; q->kind = (uint8_t)iclamp(a, 0, RK_COUNT - 1); q->state = RGS_POST; q->hp = (uint8_t)EKO_RK_HP[q->kind];
            q->x = q->hx = (int8_t)b; q->y = q->hy = (int8_t)c; q->dir = q->hdir = (int8_t)d;
        }
        return 1;
    }
    if (sscanf(cmd, "rgstate %d %d %d", &a, &b, &c) == 3) {
        /* guard a's state (RGS_*), and its sleep turns */
        if (a >= 0 && a < eg.rs.ng) { eg.rs.g[a].state = (uint8_t)iclamp(b, 0, RGS_FIRE); eg.rs.g[a].sleep = (int8_t)c; }
        return 1;
    }
    if (sscanf(cmd, "rtile %d %d %d", &a, &b, &c) == 3) { eg.rs.tile[b][a] = (char)c; return 1; }
    if (sscanf(cmd, "rpos %d %d", &a, &b) == 2) { eg.rs.ox = a; eg.rs.oy = b; return 1; }
    if (!strcmp(cmd, "rcard")) { eg.rs.card = true; return 1; }
    if (sscanf(cmd, "botcherry %d", &a) == 1) { eko_bot_cherry = a; return 1; }
    if (sscanf(cmd, "botdebug %d", &a) == 1) { extern int eko_bot_debug; eko_bot_debug = a; return 1; }
    return 0;
}

const GameDef GAME_ECHOUNIT = {
    "echounit",
    "ECHO UNIT",
    "1989",
    "ACTION / SHOOTER / STRATEGY",
    "FOUR CYBER BATS, FOUR WAYS TO FIGHT. LOSE A BAT, BREAK IT OUT OF JAIL.",
    {"WIN A JAILBREAK", "BRING DOWN MADAME COIL", "SAVE THE WORLD WITHOUT LOSING A LIFE"},
    "FLINT\t" GLYPH_A " HIT, HOLD: KI BLAST " GLYPH_B " JUMP\n"
    "\t" GLYPH_DOWN "+" GLYPH_A " JAB, OR LIFT A BARREL\n"
    "VESPER\t" GLYPH_A " TAP: SHOOT, HOLD: PLANT\n"
    "\t" GLYPH_UP GLYPH_DOWN " SIGHT " GLYPH_B " DIVE\n"
    "\t" GLYPH_A " ON AN ITEM: PICK IT UP\n"
    "DART\t" GLYPH_A " SHOOT (HOLD: IT HEATS) " GLYPH_B " JUMP\n"
    "SABLE\t" GLYPH_DPAD " 8 WAYS " GLYPH_A " USE " GLYPH_B " NEXT ITEM\n"
    "JAILBREAK\t" GLYPH_DPAD " STEP " GLYPH_A GLYPH_B " SKILLS\n"
    "START\tPAUSE",
    C_NAVY, C_RED,
    eko_load, eko_start, eko_update, eko_draw, eko_quit, eko_draw_label, eko_query, eko_cheat,
    "CYBER OWLS", 50,
    NULL,
};
