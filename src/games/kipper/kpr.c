/* KIPPER - a little yellow sub in a sea gone mean.
 * Cartridge 22 of UFO 40, a tribute to Porgy (UFO 50 #22).
 * See docs/games/22-kipper.md. This file: the title and the story, the
 * dive (docking, the equipment screen, being towed home), Auntie Roe, the
 * beaten creatures, the ending, saving and the test hooks. */
#include "kpr.h"

KprGame kg;
KprSave ksv;
static KprBank bank;               /* what the last dock (or Auntie Roe) kept */

#define SAVE_MAGIC 0x4B505201u

const char *const KPR_EQ_NAME[KPR_EQUIP] = {
    "RAM TORPEDOES", "AUGER", "LONG THRUSTER", "SINKER BOMBS", "UPSHOT ROCKETS",
    "SEEKER FINS", "LAMP", "SONAR", "LEAN MIX", "HULL PLATES",
};
const char *const KPR_EQ_LINE[KPR_EQUIP] = {
    "SHORT, HARD HITS. THEY LIGHT SWITCHES.", "DIGS THROUGH DIRT ON A DASH.", "HOLD THE DASH TO KEEP GOING.",
    "SIT STILL, HOLD THE DASH: A BOMB.", "HOLD FIRE, LET GO: ONE GOES UP.", "TORPEDOES THAT STEER.",
    "LIGHT FOR THE BLACKWATER.", "BIG ONES AND HIDDEN THINGS.", "BURNS A QUARTER LESS FUEL.",
    "THREE PLATES TAKE THREE KNOCKS.",
};

bool kpr_simulating;
bool kpr_demo;
uint32_t kpr_sim_cur, kpr_sim_prev;

void kpr_sfx(const char *name) {
    if (!kpr_simulating) sfx_play_name(name);
}
void kpr_music(int id) {
    if (!kpr_simulating) music_play(id);
}

void kpr_say(const char *m) {
    snprintf(kg.msg, sizeof kg.msg, "%s", m);
    kg.msg_t = 150;
}
void kpr_say_once(int bit, const char *m) {
    if ((kg.said >> bit) & 1) return;
    kg.said |= 1u << bit;
    kpr_say(m);
}

static void set_state(int s) {
    kg.state = s;
    kg.state_t = 0;
}

/* ---- the rules' numbers ------------------------------------------------------ */

bool kpr_on(int eq) { return (kg.cur.on >> eq) & 1; }
int kpr_slots(void) { return imin(4, 2 + kg.cur.lights / 2); }
int kpr_bars_max(void) { return KPR_BARS0 + kg.cur.tanks; }

static int popcount16(unsigned v) {
    int n = 0;
    for (; v; v &= v - 1) n++;
    return n;
}
static int fitted(void) { return popcount16(kg.cur.on); }

/* everything there is to do: 20 tanks, 20 boosters, 20 eggs home, the ten
 * pieces of equipment and the five creatures before Xolloth */
int kpr_parts_done(void) {
    int n = kg.cur.tanks + kg.cur.boosters + kg.cur.eggs_back + popcount16(kg.cur.have);
    for (int b = 0; b < BS_XOLLOTH; b++) n += kg.cur.boss_dead[b];
    return n;
}
int kpr_completion(void) { return kpr_parts_done() * 100 / 75; }

/* ---- saving --------------------------------------------------------------------- */

static void demo_end(void);

void kpr_save_now(void) {
    if (kpr_simulating || kpr_demo) return;
    ksv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &ksv, (int)sizeof ksv);
}

static void load_save(void) {
    KprSave tmp;
    memset(&tmp, 0, sizeof tmp);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) { ksv = tmp; return; }
    memset(&ksv, 0, sizeof ksv);
    ksv.magic = SAVE_MAGIC;
}

static void records(void) {
    if (kpr_simulating) return;
    if (kg.cur.tanks > ksv.best_tanks) ksv.best_tanks = kg.cur.tanks;
    if (kg.cur.boosters > ksv.best_boosters) ksv.best_boosters = kg.cur.boosters;
    if (kg.cur.eggs_back > ksv.best_eggs) ksv.best_eggs = kg.cur.eggs_back;
}

static void bank_now(void) {
    bank = kg.cur;
    if (kpr_simulating) return;
    records();
    ksv.has_run = 1;
    ksv.bank = bank;
    kpr_save_now();
}

/* ---- music ------------------------------------------------------------------------ */

void kpr_area_music(void) {
    if (kg.fight == BS_XOLLOTH) { kpr_music(KPR_MUS_FINAL); return; }
    if (kg.fight >= 0) { kpr_music(KPR_MUS_BOSS); return; }
    static const int *const M[3] = {&KPR_MUS_KELP, &KPR_MUS_ARCADE, &KPR_MUS_BLACK};
    kpr_music(*M[kpr_area_of(kg.y)]);
}

/* ---- the dive ------------------------------------------------------------------------ */

void kpr_reset_creatures(void) {
    kpr_foes_reset();
    memset(kg.bomb, 0, sizeof kg.bomb);
    memset(kg.shot, 0, sizeof kg.shot);
    memset(kg.fuel_drop_t, 0, sizeof kg.fuel_drop_t);
}

/* the sea as the bank left it: every find at home (eggs carried at the
 * last dock are in the trail again), dirt back, gates shut */
static void restore_sea(void) {
    kpr_build_tiles();
    for (int i = 0; i < KPR_NITEMS; i++) kg.where[i] = kg.cur.item[i] == IS_HOME ? IW_HOME : IW_GONE;
    kg.trail_n = 0;
    for (int i = 0; i < KPR_NITEMS; i++)
        if (kg.cur.item[i] == IS_CARRIED) kpr_trail_add(TR_ITEM, i);
    memset(kg.plate, 0, sizeof kg.plate);
    kpr_reset_creatures();
    kpr_bosses_reset();
}

/* out from under a dock: a full tank, the bombs and plates the fitting
 * gives */
void kpr_launch(int d) {
    kg.cur.dock = (uint8_t)d;
    kg.x = KPR_DOCKS[d].x * KPR_TILE + 8.0f;
    kg.y = (KPR_SKY + 2) * KPR_TILE + 8.0f;
    kg.vx = 0;
    kg.vy = 0.6f;
    kg.face = d == 0 ? -1 : 1;
    kg.fuel = kpr_bars_max() * KPR_BAR;
    kg.fuel_frac = 0;
    kg.inv = 30;
    kg.dash_t = 0;
    kg.boosting = false;
    kg.missile_ready = false;
    kg.b_still = 0;
    kg.rub_t = 0;
    kg.said = 0;
    kg.last_cost = 0;
    kg.plates = kpr_on(EQ_ARMOR) ? KPR_PLATES : 0;
    memset(kg.plate, 0, sizeof kg.plate);
    /* bombs ride in the trail */
    for (int i = kg.trail_n - 1; i >= 0; i--)
        if (kg.trail[i].kind == TR_CHARGE) {
            for (int k = i; k < kg.trail_n - 1; k++) kg.trail[k] = kg.trail[k + 1];
            kg.trail_n--;
        }
    if (kpr_on(EQ_CHARGE))
        for (int k = 0; k < KPR_CHARGES; k++) kpr_trail_add(TR_CHARGE, -1);
    kg.area = kpr_area_of(kg.y);
    kpr_reset_history();
    kpr_camera(true);
    kg.dives++;
    set_state(KS_PLAY);
    kpr_area_music();
}

/* nose up under a dock: everything towed home is installed, the fuel is
 * topped up, the dive is banked and saved, the creatures wake */
void kpr_dock_at(int d) {
    if (kpr_simulating) { set_state(KS_DOCK); return; }   /* the demo player's look-ahead stops here */
    if (kpr_demo) { demo_end(); return; }
    kg.dock_lines = 0;
    char line[40];
    int keep_n = 0;
    KprTrailObj keep[KPR_TRAIL];
    for (int s = 0; s < kg.trail_n; s++) {
        KprTrailObj o = kg.trail[s];
        if (o.kind != TR_ITEM) continue;
        const KprItemDef *it = &KPR_ITEMS[o.id];
        line[0] = 0;
        switch (it->kind) {
        case IK_TANK:
            kg.cur.tanks++;
            snprintf(line, sizeof line, "FUEL TANK: %d BARS", kpr_bars_max());
            break;
        case IK_BOOSTER:
            kg.cur.boosters++;
            snprintf(line, sizeof line, "TORPEDO KIT: LEVEL %d", kg.cur.boosters + 1);
            break;
        case IK_EQUIP:
            kg.cur.have |= (uint16_t)(1 << it->var);
            snprintf(line, sizeof line, "%s!", KPR_EQ_NAME[it->var]);
            break;
        default: break;
        }
        if (it->kind == IK_EGG) {
            kg.cur.item[o.id] = IS_CARRIED;
            keep[keep_n++] = o;
            continue;
        }
        kg.cur.item[o.id] = IS_TAKEN;
        kg.where[o.id] = IW_GONE;
        if (line[0] && kg.dock_lines < 8) snprintf(kg.dock_line[kg.dock_lines++], 40, "%s", line);
    }
    kg.trail_n = 0;
    for (int i = 0; i < keep_n; i++) kg.trail[kg.trail_n++] = keep[i];
    /* anything else loose in the sea goes home */
    for (int i = 0; i < KPR_NITEMS; i++) {
        if (kg.cur.item[i] == IS_CARRIED && kg.where[i] != IW_TRAIL) kg.cur.item[i] = IS_HOME;
        if (kg.cur.item[i] == IS_HOME) kg.where[i] = IW_HOME;
    }
    bool hurt_boss = false;
    for (int b = 0; b < KPR_BOSSES; b++)
        if (!kg.cur.boss_dead[b] && kg.cur.boss_hp[b] < bank.boss_hp[b]) hurt_boss = true;
    if (hurt_boss && kg.dock_lines < 8) snprintf(kg.dock_line[kg.dock_lines++], 40, "THE DAMAGE DONE IS KEPT");
    if (!(kg.cur.docks_known >> d & 1) && kg.dock_lines < 8)
        snprintf(kg.dock_line[kg.dock_lines++], 40, "A NEW DOCK: THE %s", d == 0 ? "PIER" : "LIGHTHOUSE");
    kg.cur.docks_known |= (uint8_t)(1 << d);
    kg.cur.dock = (uint8_t)d;
    /* fitted equipment that is no longer fitted (slots) stays as it was */
    bank_now();
    kpr_build_tiles();
    kpr_reset_creatures();
    kpr_bosses_reset();
    memset(kg.plate, 0, sizeof kg.plate);
    kg.fuel = kpr_bars_max() * KPR_BAR;
    kg.x = KPR_DOCKS[d].x * KPR_TILE + 8.0f;
    kg.y = KPR_SKY * KPR_TILE + KPR_SH / 2.0f;
    kg.vx = kg.vy = 0;
    kg.dock_phase = kg.dock_lines ? 0 : 1;
    kg.dock_cur = 0;
    set_state(KS_DOCK);
    kpr_sfx("kpr_dock");
    kpr_music(kg.dock_lines ? KPR_MUS_ITEM : KPR_MUS_DOCK);
}

/* the tank ran dry: towed back to the last dock, the dive lost */
void kpr_run_dry(void) {
    if (kg.state != KS_PLAY) return;
    if (kpr_demo && !kpr_simulating) { demo_end(); return; }
    kg.fuel = 0;
    set_state(KS_TOWED);
    kpr_sfx("kpr_dry");
    kpr_music(KPR_MUS_TOWED);
}

static void towed_update(void) {
    game_set_pausable(true);
    if (kg.state_t < 150 && !(kg.state_t > 40 && kpr_btnp(BTN_A))) return;
    uint32_t frames = kg.cur.frames;
    kg.cur = bank;
    kg.cur.frames = frames;
    kg.cur.towed++;
    bank.towed++;
    bank.frames = frames;
    if (!kpr_simulating) { ksv.bank = bank; kpr_save_now(); }
    restore_sea();
    int d = kg.cur.dock;
    kg.x = KPR_DOCKS[d].x * KPR_TILE + 8.0f;
    kg.y = KPR_SKY * KPR_TILE + KPR_SH / 2.0f;
    kg.fuel = kpr_bars_max() * KPR_BAR;
    kg.dock_lines = 0;
    kg.dock_phase = 1;
    kg.dock_cur = 0;
    set_state(KS_DOCK);
    kpr_music(KPR_MUS_DOCK);
    if (!kpr_simulating) input_consume();
}

/* ---- the dock: what came home, then the equipment ------------------------------------ */

static void toggle(int eq) {
    uint16_t bit = (uint16_t)(1 << eq);
    if (!(kg.cur.have & bit)) { kpr_sfx("ui_error"); return; }
    if (kg.cur.on & bit) {
        kg.cur.on &= (uint16_t)~bit;
        kpr_sfx("kpr_unfit");
    } else if (fitted() < kpr_slots()) {
        kg.cur.on |= bit;
        kpr_sfx("kpr_fit");
    } else {
        kpr_sfx("ui_error");
        return;
    }
    bank.on = kg.cur.on;
    if (!kpr_simulating) { ksv.bank.on = bank.on; kpr_save_now(); }
}

static void dock_update(void) {
    game_set_pausable(true);
    if (kg.dock_phase == 0) {
        int len = 70 + kg.dock_lines * 20;
        if (kg.state_t >= len || (kg.state_t > 20 && (kpr_btnp(BTN_A) || kpr_btnp(BTN_B)))) {
            kg.dock_phase = 1;
            kg.state_t = 1;
            kpr_music(KPR_MUS_DOCK);
            if (!kpr_simulating) input_consume();
        }
        return;
    }
    if (kg.state_t < 6) return;
    if (kpr_btnp(BTN_LEFT) && kg.dock_cur % 5 > 0) { kg.dock_cur--; kpr_sfx("ui_move"); }
    if (kpr_btnp(BTN_RIGHT) && kg.dock_cur % 5 < 4) { kg.dock_cur++; kpr_sfx("ui_move"); }
    if (kpr_btnp(BTN_UP) && kg.dock_cur >= 5) { kg.dock_cur -= 5; kpr_sfx("ui_move"); }
    if (kpr_btnp(BTN_DOWN) && kg.dock_cur < 5) { kg.dock_cur += 5; kpr_sfx("ui_move"); }
    if (kpr_btnp(BTN_A)) toggle(kg.dock_cur);
    if (kpr_btnp(BTN_B)) {
        if (!kpr_simulating) input_consume();
        kpr_sfx("kpr_launch");
        kpr_launch(kg.cur.dock);
    }
}

/* ---- Auntie Roe --------------------------------------------------------------------- */

void kpr_meet_roe(void) {
    if (kpr_simulating) return;
    if (kpr_demo) { demo_end(); return; }
    int eggs = 0;
    for (int s = 0; s < kg.trail_n; s++)
        if (kg.trail[s].kind == TR_ITEM && KPR_ITEMS[kg.trail[s].id].kind == IK_EGG) eggs++;
    if (!eggs) {
        kpr_say_once(5, "AUNTIE ROE: LOST ANY EGGS, DEAR? I HAVE.");
        return;
    }
    int before = kg.cur.eggs_back;
    for (int s = kg.trail_n - 1; s >= 0; s--) {
        KprTrailObj o = kg.trail[s];
        if (o.kind != TR_ITEM || KPR_ITEMS[o.id].kind != IK_EGG) continue;
        for (int k = s; k < kg.trail_n - 1; k++) kg.trail[k] = kg.trail[k + 1];
        kg.trail_n--;
        kg.cur.item[o.id] = IS_TAKEN;
        kg.where[o.id] = IW_GONE;
        bank.item[o.id] = IS_TAKEN;
        kg.cur.eggs_back++;
    }
    /* an egg handed over is home for good */
    bank.eggs_back = kg.cur.eggs_back;
    if (!kpr_simulating) {
        records();
        ksv.has_run = 1;
        ksv.bank = bank;
        kpr_save_now();
    }
    kg.roe_given = eggs;
    kg.roe_page = (before / 5 != kg.cur.eggs_back / 5 && kg.cur.eggs_back / 5 <= 4) ? 1 : 0;
    kg.vx = kg.vy = 0;
    set_state(KS_ROE);
    kpr_sfx("kpr_roe");
}

static void roe_update(void) {
    game_set_pausable(true);
    if (kg.state_t > 20 && (kpr_btnp(BTN_A) || kpr_btnp(BTN_B))) {
        if (!kpr_simulating) input_consume();
        /* back out of her grotto a little */
        float rx = KPR_ROE.x * KPR_TILE + 8.0f;
        kg.x = rx + 26.0f;
        while (kpr_box_solid(kg.x, kg.y, KPR_SW, KPR_SH) && kg.x < rx + 80) kg.x += 4;
        set_state(KS_PLAY);
        kpr_area_music();
    }
}

/* ---- beaten ------------------------------------------------------------------------------ */

/* a creature beaten flees for good, at once, and its deep light counts */
void kpr_boss_defeated(int b) {
    if (kpr_simulating) { kg.cur.boss_dead[b] = 1; return; }
    if (kpr_demo) { demo_end(); return; }
    kg.cur.boss_dead[b] = 1;
    kg.cur.boss_hp[b] = 0;
    kg.cur.lights++;
    bank.boss_dead[b] = 1;
    bank.boss_hp[b] = 0;
    bank.lights = kg.cur.lights;
    if (b == BS_GNASHER && !kpr_simulating) {
        game_award(GOAL_BEACON);
        ksv.gift = 1;
    }
    if (!kpr_simulating) {
        ksv.has_run = 1;
        ksv.bank = bank;
        kpr_save_now();
    }
    char m[48];
    snprintf(m, sizeof m, "%s FLED! A DEEP LIGHT...", KPR_BOSS_NAME[b]);
    kpr_say(m);
    kpr_burst(kg.boss[b].x, kg.boss[b].y, C_YELLOW, 16, 1.5f);
    kpr_sfx("kpr_light");
}

void kpr_final_defeated(void) {
    if (kpr_simulating) { set_state(KS_ENDING); return; }
    if (kpr_demo) { demo_end(); return; }
    kg.cherry_run = kpr_completion() >= 100;
    if (!kpr_simulating) {
        game_award(GOAL_SAUCER);
        if (kg.cherry_run) {
            game_award(GOAL_ALIEN);
            if (ksv.cherries < 65535) ksv.cherries++;
        }
        if (ksv.wins < 65535) ksv.wins++;
        records();
        kpr_save_now();
    }
    set_state(KS_ENDING);
    kpr_music(KPR_MUS_ENDING);
}

/* ---- one frame of a dive --------------------------------------------------------- */

void kpr_play_step(void) {
    if (!kpr_simulating && !kpr_demo) game_set_pausable(true);
    kg.cur.frames++;
    if (kg.msg_t > 0) kg.msg_t--;
    kpr_sub_update();
    if (kg.state != KS_PLAY) return;
    kpr_trail_update();
    kpr_shots_update();
    kpr_bombs_update();
    if (kg.state != KS_PLAY) return;
    kpr_foes_update();
    if (kg.state != KS_PLAY) return;
    kpr_eshots_update();
    if (kg.state != KS_PLAY) return;
    int was = kg.fight;
    kpr_bosses_update();
    if (kg.state != KS_PLAY) return;
    kpr_vents_update();
    kpr_items_update();
    if (kg.fuel <= 0) { kpr_run_dry(); return; }
    /* a new area: the creatures wake again */
    int a = kpr_area_of(kg.y);
    if (a != kg.area) {
        kg.area = a;
        kpr_reset_creatures();
        if (a == 2 && !kpr_on(EQ_SPOT)) kpr_say_once(6, "PITCH BLACK. I NEED SOME LIGHT DOWN HERE.");
        kpr_area_music();
    } else if (was != kg.fight) kpr_area_music();
    for (int i = 0; i < KPR_MAX_PARTS; i++)
        if (kg.part[i].life > 0) { kg.part[i].life--; kg.part[i].x += kg.part[i].vx; kg.part[i].y += kg.part[i].vy; }
    if (kg.shake > 0) kg.shake--;
    kpr_camera(false);
    if (kg.fuel < KPR_BAR && kg.fuel > 0) kpr_say_once(7, "FUEL'S LOW! BACK TO A DOCK!");
}

/* ---- the title and the story ---------------------------------------------------- */

static void to_title(void) {
    set_state(KS_TITLE);
    game_set_pausable(false);
    kpr_music(KPR_MUS_TITLE);
    kg.menu = ksv.has_run ? 1 : 0;
}

static void new_run(void) {
    memset(&kg.cur, 0, sizeof kg.cur);
    for (int b = 0; b < KPR_BOSSES; b++) kg.cur.boss_hp[b] = (int16_t)KPR_BOSS_HP[b];
    kg.cur.docks_known = 1;
    kg.cur.dock = 0;
    bank = kg.cur;
    if (!kpr_simulating) {
        ksv.has_run = 1;
        ksv.bank = bank;
        kpr_save_now();
    }
    restore_sea();
    kg.dives = 0;
    kpr_launch(0);
}

static void continue_run(void) {
    kg.cur = ksv.bank;
    bank = kg.cur;
    restore_sea();
    kg.dives = 0;
    kpr_launch(kg.cur.dock);
}

/* left alone on the title, the cartridge shows a dive: the sub on its way
 * home towing three finds with a full tank (any button ends it) */
static void demo_start(void) {
    memset(&kg.cur, 0, sizeof kg.cur);
    for (int b = 0; b < KPR_BOSSES; b++) kg.cur.boss_hp[b] = (int16_t)KPR_BOSS_HP[b];
    kg.cur.docks_known = 1;
    kg.cur.tanks = 6;
    kg.cur.boosters = 4;
    bank = kg.cur;
    kpr_demo = true;
    restore_sea();
    kpr_launch(0);
    kg.x = 70 * KPR_TILE + 8.0f;
    kg.y = 28 * KPR_TILE + 8.0f;
    kpr_reset_history();
    kpr_trail_add(TR_ITEM, 10);
    kpr_trail_add(TR_ITEM, 30);
    kpr_trail_add(TR_ITEM, 50);
    kg.area = kpr_area_of(kg.y);
    kpr_camera(true);
    kg.demo_t = 0;
    kpr_sim_cur = kpr_sim_prev = 0;
    kpr_bot_reset();
}

static void demo_end(void) {
    kpr_demo = false;
    load_save();
    memset(&kg, 0, sizeof kg);
    kg.fight = -1;
    to_title();
}

static void title_update(void) {
    game_set_pausable(false);
    if (input_held()) kg.idle_t = 0;
    else if (++kg.idle_t > 900) { demo_start(); return; }
    if (kpr_btnp(BTN_UP) || kpr_btnp(BTN_DOWN)) { kg.menu ^= 1; kpr_sfx("ui_move"); }
    if (kpr_btnp(BTN_B)) { game_exit_to_library(); return; }
    if ((kpr_btnp(BTN_A) || kpr_btnp(BTN_START)) && kg.state_t > 5) {
        if (!kpr_simulating) input_consume();
        if (kg.menu == 1 && !ksv.has_run) { kpr_sfx("ui_error"); return; }
        kpr_sfx("ui_ok");
        if (kg.menu == 0) {
            set_state(KS_INTRO);
            kpr_music(KPR_MUS_INTRO);
        } else continue_run();
    }
}

static void intro_update(void) {
    game_set_pausable(false);
    if ((kg.state_t > 20 && (kpr_btnp(BTN_A) || kpr_btnp(BTN_B) || kpr_btnp(BTN_START))) || kg.state_t > 1800) {
        if (!kpr_simulating) input_consume();
        new_run();
    }
}

#define ENDING_LEN 1500
#define CREDITS_LEN 1500
static void ending_update(void) {
    game_set_pausable(false);
    if (kpr_btn(BTN_A)) kg.state_t += 2;
    if (kg.state_t > ENDING_LEN) set_state(KS_CREDITS);
}

static void credits_update(void) {
    game_set_pausable(false);
    if (kpr_btn(BTN_A)) kg.state_t += 2;
    if (kg.state_t > CREDITS_LEN) {
        if (!kpr_simulating) input_consume();
        to_title();
    }
}

static void kpr_update(void) {
    kg.state_t++;
    kg.frame_t++;
    if (kpr_demo) {
        /* the demo: the demo player's buttons; a real press or time ends it */
        game_set_pausable(false);
        if (btnp(BTN_ANY) || ++kg.demo_t > 1500 || kg.state != KS_PLAY) {
            input_consume();
            demo_end();
            return;
        }
        uint32_t m = (uint32_t)kpr_bot_buttons();
        kpr_sim_prev = kpr_sim_cur;
        kpr_sim_cur = m;
        kpr_play_step();
        return;
    }
    switch (kg.state) {
    case KS_TITLE: title_update(); break;
    case KS_INTRO: intro_update(); break;
    case KS_PLAY: kpr_play_step(); break;
    case KS_DOCK: dock_update(); break;
    case KS_TOWED: towed_update(); break;
    case KS_ROE: roe_update(); break;
    case KS_ENDING: ending_update(); break;
    case KS_CREDITS: credits_update(); break;
    default: break;
    }
}

/* ---- the cartridge -------------------------------------------------------------- */

static void kpr_load(void) {
    kpr_world_init();
    kpr_art_load();
    kpr_audio_load();
}

static void kpr_start(void) {
    kpr_demo = false;
    load_save();
    memset(&kg, 0, sizeof kg);
    kg.fight = -1;
    if (ksv.gift) game_award(GOAL_BEACON);
    if (ksv.wins > 0) game_award(GOAL_SAUCER);
    if (ksv.cherries > 0) game_award(GOAL_ALIEN);
    to_title();
}

static void kpr_quit(void) { kpr_save_now(); }

/* ---- test hooks ------------------------------------------------------------------ */

static int where_count(int w) {
    int n = 0;
    for (int i = 0; i < KPR_NITEMS; i++) n += kg.where[i] == w;
    return n;
}
static int items_kind(int k) {
    int n = 0;
    for (int i = 0; i < KPR_NITEMS; i++) n += KPR_ITEMS[i].kind == k;
    return n;
}
static int trail_items(int k) {
    int n = 0;
    for (int s = 0; s < kg.trail_n; s++) n += kg.trail[s].kind == TR_ITEM && KPR_ITEMS[kg.trail[s].id].kind == k;
    return n;
}
static int find_item(int kind, int var) {
    for (int i = 0; i < KPR_NITEMS; i++)
        if (KPR_ITEMS[i].kind == kind && (kind != IK_EQUIP || KPR_ITEMS[i].var == var) &&
            (kind == IK_EQUIP || kg.cur.item[i] == IS_HOME))
            return i;
    return -1;
}

int kpr_reach_check(void);

static int kpr_query(const char *key, int *out) {
    if (!strcmp(key, "bot")) { *out = kpr_bot_buttons(); return 1; }
    if (!strcmp(key, "bot_idle")) { *out = kpr_bot_idle(); return 1; }
    if (!strcmp(key, "bot_goal")) { *out = kpr_bot_goal; return 1; }
    if (!strcmp(key, "state")) { *out = kg.state; return 1; }
    if (!strcmp(key, "demo")) { *out = kpr_demo; return 1; }
    if (!strcmp(key, "menu")) { *out = kg.menu; return 1; }
    if (!strcmp(key, "x")) { *out = (int)kg.x; return 1; }
    if (!strcmp(key, "y")) { *out = (int)kg.y; return 1; }
    if (!strcmp(key, "tx")) { *out = (int)(kg.x / KPR_TILE); return 1; }
    if (!strcmp(key, "ty")) { *out = (int)(kg.y / KPR_TILE); return 1; }
    if (!strcmp(key, "vx100")) { *out = (int)(kg.vx * 100); return 1; }
    if (!strcmp(key, "vy100")) { *out = (int)(kg.vy * 100); return 1; }
    if (!strcmp(key, "face")) { *out = kg.face; return 1; }
    if (!strcmp(key, "area")) { *out = kpr_area_of(kg.y); return 1; }
    if (!strcmp(key, "fuel")) { *out = kg.fuel; return 1; }
    if (!strcmp(key, "fuel_max")) { *out = kpr_bars_max() * KPR_BAR; return 1; }
    if (!strcmp(key, "bars_max")) { *out = kpr_bars_max(); return 1; }
    if (!strcmp(key, "last_cost")) { *out = kg.last_cost; return 1; }
    if (!strcmp(key, "inv")) { *out = kg.inv; return 1; }
    if (!strcmp(key, "dash")) { *out = kg.dash_t; return 1; }
    if (!strcmp(key, "boosting")) { *out = kg.boosting; return 1; }
    if (!strcmp(key, "ready")) { *out = kg.missile_ready; return 1; }
    if (!strcmp(key, "eshot_vy100")) { *out = 0; for (int i = 0; i < KPR_MAX_ESHOTS; i++) if (kg.es[i].alive && kg.es[i].kind == ES_PELLET) { *out = (int)(kg.es[i].vy * 100); break; } return 1; }
    if (!strcmp(key, "ready_bomb")) { *out = kg.charge_ready; return 1; }
    if (!strcmp(key, "fire_hold")) { *out = kg.fire_hold; return 1; }
    if (!strcmp(key, "missile_y")) { *out = -1; for (int i = 0; i < KPR_MAX_SHOTS; i++) if (kg.shot[i].alive && kg.shot[i].kind == SH_MISSILE) *out = (int)kg.shot[i].y; return 1; }
    if (!strcmp(key, "missile_life")) { *out = -1; for (int i = 0; i < KPR_MAX_SHOTS; i++) if (kg.shot[i].alive && kg.shot[i].kind == SH_MISSILE) *out = kg.shot[i].life; return 1; }
    if (!strcmp(key, "plates")) { *out = kg.plates; return 1; }
    if (!strcmp(key, "plate_x")) { *out = -1; for (int i = 0; i < KPR_PLATES; i++) if (kg.plate[i].alive) *out = (int)kg.plate[i].x; return 1; }
    if (!strcmp(key, "plate_y")) { *out = -1; for (int i = 0; i < KPR_PLATES; i++) if (kg.plate[i].alive) *out = (int)kg.plate[i].y; return 1; }
    if (!strcmp(key, "loose_plates")) { int n = 0; for (int i = 0; i < KPR_PLATES; i++) n += kg.plate[i].alive; *out = n; return 1; }
    if (!strcmp(key, "trail")) { *out = kg.trail_n; return 1; }
    if (!strcmp(key, "trail_charges")) { *out = kpr_trail_count(TR_CHARGE); return 1; }
    if (!strcmp(key, "trail_eggs")) { *out = trail_items(IK_EGG); return 1; }
    if (!strcmp(key, "trail_tanks")) { *out = trail_items(IK_TANK); return 1; }
    if (!strcmp(key, "loose")) { *out = where_count(IW_LOOSE); return 1; }
    if (!strcmp(key, "held")) { *out = where_count(IW_HELD); return 1; }
    if (!strcmp(key, "tanks")) { *out = kg.cur.tanks; return 1; }
    if (!strcmp(key, "boosters")) { *out = kg.cur.boosters; return 1; }
    if (!strcmp(key, "eggs_back")) { *out = kg.cur.eggs_back; return 1; }
    if (!strcmp(key, "lights")) { *out = kg.cur.lights; return 1; }
    if (!strcmp(key, "slots")) { *out = kpr_slots(); return 1; }
    if (!strcmp(key, "have")) { *out = kg.cur.have; return 1; }
    if (!strcmp(key, "on")) { *out = kg.cur.on; return 1; }
    if (!strcmp(key, "companions")) { *out = kpr_companions(); return 1; }
    if (!strcmp(key, "completion")) { *out = kpr_completion(); return 1; }
    if (!strcmp(key, "parts")) { *out = kpr_parts_done(); return 1; }
    if (!strcmp(key, "dock")) { *out = kg.cur.dock; return 1; }
    if (!strcmp(key, "docks_known")) { *out = kg.cur.docks_known; return 1; }
    if (!strcmp(key, "dock_phase")) { *out = kg.dock_phase; return 1; }
    if (!strcmp(key, "dock_cur")) { *out = kg.dock_cur; return 1; }
    if (!strcmp(key, "dock_lines")) { *out = kg.dock_lines; return 1; }
    if (!strcmp(key, "towed")) { *out = (int)kg.cur.towed; return 1; }
    if (!strcmp(key, "frames")) { *out = (int)kg.cur.frames; return 1; }
    if (!strcmp(key, "dives")) { *out = kg.dives; return 1; }
    if (!strcmp(key, "hits")) { *out = kg.hits_taken; return 1; }
    if (!strcmp(key, "gates")) { *out = kg.gates; return 1; }
    if (!strcmp(key, "fight")) { *out = kg.fight; return 1; }
    if (!strcmp(key, "msg_t")) { *out = kg.msg_t; return 1; }
    if (!strcmp(key, "cherry_run")) { *out = kg.cherry_run; return 1; }
    if (!strcmp(key, "roe_page")) { *out = kg.roe_page; return 1; }
    if (!strcmp(key, "roe_given")) { *out = kg.roe_given; return 1; }
    if (!strcmp(key, "foes_awake")) { *out = kpr_foes_awake(); return 1; }
    if (!strcmp(key, "eshots")) { int n = 0; for (int i = 0; i < KPR_MAX_ESHOTS; i++) n += kg.es[i].alive; *out = n; return 1; }
    if (!strncmp(key, "eshots_", 7)) { int k = atoi(key + 7), n = 0; for (int i = 0; i < KPR_MAX_ESHOTS; i++) n += kg.es[i].alive && kg.es[i].kind == k; *out = n; return 1; }
    if (!strcmp(key, "shots")) { int n = 0; for (int i = 0; i < KPR_MAX_SHOTS; i++) n += kg.shot[i].alive; *out = n; return 1; }
    if (!strncmp(key, "shots_", 6)) { int k = atoi(key + 6), n = 0; for (int i = 0; i < KPR_MAX_SHOTS; i++) n += kg.shot[i].alive && kg.shot[i].kind == k; *out = n; return 1; }
    if (!strcmp(key, "shot0_life")) { *out = -1; for (int i = 0; i < KPR_MAX_SHOTS; i++) if (kg.shot[i].alive && kg.shot[i].kind <= SH_BUSTER) { *out = kg.shot[i].life; break; } return 1; }
    if (!strcmp(key, "shot0_dmg")) { *out = -1; for (int i = 0; i < KPR_MAX_SHOTS; i++) if (kg.shot[i].alive && kg.shot[i].kind <= SH_BUSTER) { *out = kg.shot[i].dmg; break; } return 1; }
    if (!strcmp(key, "shot0_vy100")) { *out = 0; for (int i = 0; i < KPR_MAX_SHOTS; i++) if (kg.shot[i].alive && kg.shot[i].kind <= SH_BUSTER) { *out = (int)(kg.shot[i].vy * 100); break; } return 1; }
    if (!strcmp(key, "torp_dmg")) { *out = kpr_torp_damage(); return 1; }
    if (!strcmp(key, "torp_range")) { *out = kpr_torp_range(); return 1; }
    if (!strcmp(key, "bombs")) { int n = 0; for (int i = 0; i < KPR_MAX_BOMBS; i++) n += kg.bomb[i].alive; *out = n; return 1; }
    if (!strncmp(key, "foes_", 5)) { *out = kpr_count_foes(atoi(key + 5), false); return 1; }
    if (!strncmp(key, "calm_", 5)) { *out = kpr_count_foes(atoi(key + 5), true); return 1; }
    if (!strncmp(key, "foe", 3) && isdigit((unsigned char)key[3])) {
        int i = atoi(key + 3);
        const char *u = strchr(key + 3, '_');
        if (i < 0 || i >= KPR_MAX_FOES || !u) return 0;
        const KprFoe *f = &kg.foe[i];
        if (!strcmp(u, "_alive")) { *out = f->alive; return 1; }
        if (!strcmp(u, "_kind")) { *out = f->kind; return 1; }
        if (!strcmp(u, "_hp")) { *out = f->hp; return 1; }
        if (!strcmp(u, "_calm")) { *out = f->pacified; return 1; }
        if (!strcmp(u, "_state")) { *out = f->state; return 1; }
        if (!strcmp(u, "_x")) { *out = (int)f->x; return 1; }
        if (!strcmp(u, "_y")) { *out = (int)f->y; return 1; }
        if (!strcmp(u, "_carry")) { *out = f->carry; return 1; }
        return 0;
    }
    if (!strncmp(key, "boss", 4) && isdigit((unsigned char)key[4])) {
        int b = key[4] - '0';
        const char *u = key + 5;
        if (b < 0 || b >= KPR_BOSSES) return 0;
        const KprBoss *o = &kg.boss[b];
        if (!strcmp(u, "_on")) { *out = o->on; return 1; }
        if (!strcmp(u, "_hp")) { *out = o->hp; return 1; }
        if (!strcmp(u, "_dead")) { *out = kg.cur.boss_dead[b]; return 1; }
        if (!strcmp(u, "_banked")) { *out = bank.boss_hp[b]; return 1; }
        if (!strcmp(u, "_state")) { *out = o->state; return 1; }
        if (!strcmp(u, "_x")) { *out = (int)o->x; return 1; }
        if (!strcmp(u, "_y")) { *out = (int)o->y; return 1; }
        if (!strcmp(u, "_beam")) { *out = o->beam_t; return 1; }
        if (!strcmp(u, "_tent")) { *out = o->tent_t; return 1; }
        return 0;
    }
    if (!strncmp(key, "item", 4) && isdigit((unsigned char)key[4])) {
        int i = atoi(key + 4);
        const char *u = strchr(key + 4, '_');
        if (i < 0 || i >= KPR_NITEMS || !u) return 0;
        if (!strcmp(u, "_state")) { *out = kg.cur.item[i]; return 1; }
        if (!strcmp(u, "_where")) { *out = kg.where[i]; return 1; }
        if (!strcmp(u, "_banked")) { *out = bank.item[i]; return 1; }
        return 0;
    }
    if (!strncmp(key, "tile_", 5)) {
        int x = atoi(key + 5);
        const char *u = strchr(key + 5, '_');
        *out = u ? kpr_tile(x, atoi(u + 1)) : -1;
        return 1;
    }
    if (!strncmp(key, "find_", 5)) {
        /* find_K_V: the first item of kind K (equipment V) still in the sea */
        int k = atoi(key + 5);
        const char *u = strchr(key + 5, '_');
        *out = find_item(k, u ? atoi(u + 1) : 0);
        return 1;
    }
    if (!strncmp(key, "nitems_", 7)) { *out = items_kind(atoi(key + 7)); return 1; }
    if (!strcmp(key, "nfoes")) { *out = KPR_NFOES; return 1; }
    if (!strcmp(key, "nbrk")) { *out = kpr_nbrk(); return 1; }
    if (!strcmp(key, "save_has")) { *out = ksv.has_run; return 1; }
    if (!strcmp(key, "save_tanks")) { *out = ksv.bank.tanks; return 1; }
    if (!strcmp(key, "save_eggs")) { *out = ksv.bank.eggs_back; return 1; }
    if (!strcmp(key, "save_dock")) { *out = ksv.bank.dock; return 1; }
    if (!strcmp(key, "save_on")) { *out = ksv.bank.on; return 1; }
    if (!strcmp(key, "bank_tanks")) { *out = bank.tanks; return 1; }
    if (!strcmp(key, "bank_brk")) { int n = 0; for (int i = 0; i < kpr_nbrk(); i++) n += (bank.broken[i >> 3] >> (i & 7)) & 1; *out = n; return 1; }
    if (!strcmp(key, "brk")) { int n = 0; for (int i = 0; i < kpr_nbrk(); i++) n += kpr_brk_banked(i); *out = n; return 1; }
    if (!strcmp(key, "rec_tanks")) { *out = ksv.best_tanks; return 1; }
    if (!strcmp(key, "rec_boosters")) { *out = ksv.best_boosters; return 1; }
    if (!strcmp(key, "rec_eggs")) { *out = ksv.best_eggs; return 1; }
    if (!strcmp(key, "rec_wins")) { *out = ksv.wins; return 1; }
    if (!strcmp(key, "rec_cherries")) { *out = ksv.cherries; return 1; }
    if (!strcmp(key, "reach")) { *out = kpr_reach_check(); return 1; }
    if (!strcmp(key, "layout")) { *out = kpr_layout_audit(); return 1; }
    if (!strcmp(key, "art_bad")) { *out = kpr_art_check(); return 1; }
    if (!strcmp(key, "vent0")) { *out = kpr_vent_on(0); return 1; }
    if (!strcmp(key, "ink_view") || !strcmp(key, "radar_yellow") || !strcmp(key, "radar_red")) {
        /* draw the screen and count: black pixels in the view, yellow or
         * red ones in the radar */
        kpr_draw();
        int n = 0;
        if (key[0] == 'i') {
            for (int y = 0; y < SCREEN_H; y++)
                for (int x = KPR_HUD_W; x < SCREEN_W; x++) n += g_screen.px[y * SCREEN_W + x] == C_INK;
        } else {
            int col = key[6] == 'y' ? C_YELLOW : C_RED;
            for (int y = 4; y < 54; y++)
                for (int x = 8; x < 58; x++) n += g_screen.px[y * SCREEN_W + x] == col;
        }
        *out = n;
        return 1;
    }
    if (!strcmp(key, "sx")) { *out = (int)kg.x - kg.cam_x + KPR_HUD_W; return 1; }
    return 0;
}

static void place(float x, float y) {
    kg.x = x;
    kg.y = y;
    kg.vx = kg.vy = 0;
    int a = kpr_area_of(y);
    if (a != kg.area) { kg.area = a; kpr_reset_creatures(); }
    kpr_reset_history();
    kpr_camera(true);
}

static int kpr_cheat(const char *cmd) {
    int a, b;
    if (!strcmp(cmd, "newgame")) { new_run(); return 1; }
    if (sscanf(cmd, "tile %d %d", &a, &b) == 2) { place(a * KPR_TILE + 8.0f, b * KPR_TILE + 8.0f); return 1; }
    if (sscanf(cmd, "pos %d %d", &a, &b) == 2) { place((float)a, (float)b); return 1; }
    if (sscanf(cmd, "face %d", &a) == 1) { kg.face = a < 0 ? -1 : 1; return 1; }
    if (sscanf(cmd, "fuel %d", &a) == 1) { kg.fuel = a; return 1; }
    if (sscanf(cmd, "tanks %d", &a) == 1) { kg.cur.tanks = (uint8_t)a; bank.tanks = (uint8_t)a; kg.fuel = kpr_bars_max() * KPR_BAR; return 1; }
    if (sscanf(cmd, "boosters %d", &a) == 1) { kg.cur.boosters = (uint8_t)a; bank.boosters = (uint8_t)a; return 1; }
    if (sscanf(cmd, "lights %d", &a) == 1) { kg.cur.lights = (uint8_t)a; bank.lights = (uint8_t)a; return 1; }
    if (sscanf(cmd, "eggsback %d", &a) == 1) { kg.cur.eggs_back = (uint8_t)a; bank.eggs_back = (uint8_t)a; return 1; }
    if (sscanf(cmd, "have %d", &a) == 1) { kg.cur.have |= (uint16_t)(1 << a); bank.have = kg.cur.have; return 1; }
    if (sscanf(cmd, "fit %d", &a) == 1) {
        kg.cur.have |= (uint16_t)(1 << a);
        kg.cur.on |= (uint16_t)(1 << a);
        bank.have = kg.cur.have;
        bank.on = kg.cur.on;
        if (a == EQ_ARMOR) kg.plates = KPR_PLATES;
        if (a == EQ_CHARGE) while (kg.trail_n < KPR_TRAIL && kpr_trail_count(TR_CHARGE) < KPR_CHARGES) kpr_trail_add(TR_CHARGE, -1);
        return 1;
    }
    if (sscanf(cmd, "unfit %d", &a) == 1) { kg.cur.on &= (uint16_t)~(1 << a); bank.on = kg.cur.on; return 1; }
    if (sscanf(cmd, "take %d", &a) == 1) {
        /* an item straight into the trail */
        if (a >= 0 && a < KPR_NITEMS && kg.cur.item[a] == IS_HOME && kg.trail_n < KPR_TRAIL) kpr_trail_add(TR_ITEM, a);
        return 1;
    }
    if (sscanf(cmd, "dock %d", &a) == 1) { kpr_dock_at(a); return 1; }
    if (sscanf(cmd, "launch %d", &a) == 1) { kpr_launch(a); return 1; }
    if (sscanf(cmd, "foe %d %d %d", &a, &b, &(int){0}) == 3) {
        int k, x, y;
        sscanf(cmd, "foe %d %d %d", &k, &x, &y);
        kpr_spawn_foe(iclamp(k, 0, FK_KINDS - 1), (float)x, (float)y);
        return 1;
    }
    if (sscanf(cmd, "hurtfoe %d %d", &a, &b) == 2) { kpr_hurt_foe(a, b, false); return 1; }
    if (sscanf(cmd, "bosshp %d %d", &a, &b) == 2) { if (a >= 0 && a < KPR_BOSSES) { kg.boss[a].hp = b; kg.cur.boss_hp[a] = (int16_t)b; } return 1; }
    if (sscanf(cmd, "bosspos %d %d %d", &a, &b, &(int){0}) == 3) {
        int k, x, y;
        sscanf(cmd, "bosspos %d %d %d", &k, &x, &y);
        if (k >= 0 && k < KPR_BOSSES) { kg.boss[k].x = (float)x; kg.boss[k].y = (float)y; }
        return 1;
    }
    if (sscanf(cmd, "hurtboss %d %d", &a, &b) == 2) { kpr_boss_hurt(a, b); return 1; }
    if (sscanf(cmd, "killboss %d", &a) == 1) { if (a >= 0 && a < KPR_BOSSES) { kg.boss[a].hp = 1; kpr_boss_hurt(a, 9999); } return 1; }
    if (sscanf(cmd, "bossstate %d %d", &a, &b) == 2) { if (a >= 0 && a < KPR_BOSSES) { kg.boss[a].state = b; kg.boss[a].t = 0; } return 1; }
    if (sscanf(cmd, "bosst %d %d", &a, &b) == 2) { if (a >= 0 && a < KPR_BOSSES) kg.boss[a].t = b; return 1; }
    if (!strcmp(cmd, "god")) { kg.god = true; return 1; }
    if (!strcmp(cmd, "mortal")) { kg.god = false; return 1; }
    if (!strcmp(cmd, "nofoes")) { kg.no_foes = true; return 1; }
    if (!strcmp(cmd, "foes")) { kg.no_foes = false; return 1; }
    if (!strcmp(cmd, "calm")) { kg.calm = true; return 1; }
    if (!strcmp(cmd, "stormy")) { kg.calm = false; return 1; }
    if (!strcmp(cmd, "clear")) { memset(kg.foe, 0, sizeof kg.foe); memset(kg.es, 0, sizeof kg.es); return 1; }
    if (!strcmp(cmd, "resetfoes")) { kpr_reset_creatures(); return 1; }
    if (sscanf(cmd, "eshot %d %d %d", &a, &b, &(int){0}) == 3) {
        int k, x, y, vx = 0, vy = 0;
        sscanf(cmd, "eshot %d %d %d %d %d", &k, &x, &y, &vx, &vy);
        kpr_add_eshot(k, (float)x, (float)y, vx / 10.0f, vy / 10.0f);
        return 1;
    }
    if (!strcmp(cmd, "allitems")) {
        for (int i = 0; i < KPR_NITEMS; i++) {
            const KprItemDef *d = &KPR_ITEMS[i];
            if (kg.cur.item[i] == IS_TAKEN) continue;
            kg.cur.item[i] = IS_TAKEN;
            kg.where[i] = IW_GONE;
            if (d->kind == IK_TANK) kg.cur.tanks++;
            else if (d->kind == IK_BOOSTER) kg.cur.boosters++;
            else if (d->kind == IK_EGG) kg.cur.eggs_back++;
            else kg.cur.have |= (uint16_t)(1 << d->var);
        }
        kg.trail_n = 0;
        bank = kg.cur;
        kg.fuel = kpr_bars_max() * KPR_BAR;
        return 1;
    }
    if (!strcmp(cmd, "allbosses")) {
        for (int b2 = 0; b2 < BS_XOLLOTH; b2++) {
            kg.cur.boss_dead[b2] = 1;
            kg.cur.boss_hp[b2] = 0;
            kg.boss[b2].on = 0;
        }
        kg.cur.lights = 5;
        bank = kg.cur;
        return 1;
    }
    if (!strcmp(cmd, "openall")) {
        /* every blast-opened wall opened and banked */
        for (int y = 0; y < KPR_H; y++)
            for (int x = 0; x < KPR_W; x++) {
                int c = kg.tile[y][x];
                if (c == 'B' || c == 'X' || c == '?') kpr_break_tile(x, y);
            }
        bank = kg.cur;
        return 1;
    }
    if (!strcmp(cmd, "bank")) { bank_now(); return 1; }
    if (!strcmp(cmd, "dry")) { kg.fuel = 0; kpr_run_dry(); return 1; }
    if (!strcmp(cmd, "win")) { kpr_final_defeated(); return 1; }
    if (sscanf(cmd, "botlog %d", &a) == 1) { kpr_bot_log = a; return 1; }
    if (sscanf(cmd, "menu %d", &a) == 1) { kg.menu = a; return 1; }
    if (sscanf(cmd, "statet %d", &a) == 1) { kg.state_t = a; return 1; }
    return 0;
}

const GameDef GAME_KIPPER = {
    "kipper",
    "KIPPER",
    "1986",
    "UNDERSEA ADVENTURE",
    "A LITTLE SUB IN A SEA GONE MEAN. FUEL IS LIFE: TOW YOUR FINDS HOME.",
    {"DRIVE OFF GNASHER THE SHARK", "SEE WHAT WOKE IN THE DEEP", "EVERYTHING FOUND, THEN XOLLOTH"},
    GLYPH_DPAD "\tSWIM, EIGHT WAYS\n"
    GLYPH_A " (HOLD)\tTORPEDOES (FACING LOCKS)\n"
    GLYPH_A " (TAP)\tTORPEDOES, QUICKER\n"
    GLYPH_B "\tDASH (COSTS FUEL)\n"
    "NOSE UP\tDOCK UNDER A DOCK\n"
    "START\tPAUSE",
    C_YELLOW, C_NAVY,
    kpr_load, kpr_start, kpr_update, kpr_draw, kpr_quit, kpr_draw_label, kpr_query, kpr_cheat,
    "PORGY", 22,
    NULL,
};
