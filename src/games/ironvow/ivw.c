/* IRONVOW - a knight's descent through Blackspire Keep.
 * Cartridge 37 of UFO 40, a tribute to Valbrace (UFO 50 #37).
 * See docs/games/37-ironvow.md. This file: the title, the screens the
 * context menu opens (pack, item, picker, pedlar, throne, gem panel, well,
 * glyph, chart, talk), the floors one after another, saving on each new
 * floor, death and the reload, the escape, goals, records and test hooks. */
#include "ivw.h"
#include <stdarg.h>

IvwGame ig;
IvwSave igs;
bool ivw_simulating;

#define SAVE_MAGIC 0x49565701u

/* ---- small services ----------------------------------------------------------------------- */

void ivw_sfx(const char *name) {
    if (!ivw_simulating) sfx_play_name(name);
}

void ivw_say(const char *fmt, ...) {
    char buf[96];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    /* the log is 184 pixels wide: a long message takes two lines */
    char lines[2][UI_WRAP_LEN];
    int n = imin(2, ui_wrap(buf, IVW_LOG_W, true, lines, 2));
    for (int i = 0; i < n; i++) {
        memmove(ig.msg[0], ig.msg[1], sizeof ig.msg[0] * (IVW_MSG_LINES - 1));
        snprintf(ig.msg[IVW_MSG_LINES - 1], sizeof ig.msg[0], "%.63s", lines[i]);
    }
    ig.msg_t = 600;
}

void ivw_set_state(int s) {
    ig.state = s;
    ig.state_t = 0;
}

int ivw_max_hp(const IvwHero *h) { return 50 + 10 * (h->stat[STAT_GRIT] / 3) + h->hp_bonus; }
int ivw_max_mp(const IvwHero *h) { return 10 + 4 * h->stat[STAT_LORE]; }
int ivw_max_st(const IvwHero *h) { return (40 + 6 * h->stat[STAT_GRIT]) * 16; }
int ivw_stat_total(const IvwHero *h) { return h->stat[0] + h->stat[1] + h->stat[2]; }

/* ---- the pack (kept in order: no gaps) ------------------------------------------------------ */
int ivw_count_item(int item) {
    int n = 0;
    for (int i = 0; i < ig.hero.inv_cap; i++) n += ig.hero.inv[i] == item;
    return n;
}

int ivw_find_item(int item) {
    for (int i = 0; i < ig.hero.inv_cap; i++)
        if (ig.hero.inv[i] == item) return i;
    return -1;
}

static int pack_count(void) {
    int n = 0;
    while (n < ig.hero.inv_cap && ig.hero.inv[n]) n++;
    return n;
}

int ivw_free_slots(void) { return ig.hero.inv_cap - pack_count(); }

bool ivw_give_item(int item) {
    int n = pack_count();
    if (n >= ig.hero.inv_cap) return false;
    ig.hero.inv[n] = (uint8_t)item;
    ig.hero.ready[n] = 0;
    return true;
}

void ivw_remove_slot(int slot) {
    IvwHero *h = &ig.hero;
    if (slot < 0 || slot >= IVW_INV_MAX) return;
    int n = pack_count();
    for (int i = slot; i < IVW_INV_MAX - 1; i++) { h->inv[i] = h->inv[i + 1]; h->ready[i] = h->ready[i + 1]; }
    h->inv[IVW_INV_MAX - 1] = 0;
    h->ready[IVW_INV_MAX - 1] = 0;
    int8_t *gear[3] = {&h->wpn, &h->arm, &h->shd};
    for (int g = 0; g < 3; g++) {
        if (*gear[g] == slot) *gear[g] = -1;
        else if (*gear[g] > slot) (*gear[g])--;
    }
    (void)n;
}

void ivw_add_secret(int bit) {
    if (!(ig.hero.secrets & bit)) ivw_say("(A SECRET IS FOUND.)");
    ig.hero.secrets |= (uint8_t)bit;
    if (!ig.demo) igs.secrets_seen |= (uint8_t)bit;
}

void ivw_check_gift(void) {
    if (ig.demo) return;
    for (int s = 0; s < 3; s++)
        if (ig.hero.stat[s] >= 8) {
            if (!igs.gifts) igs.gifts = 1;
            game_award(GOAL_BEACON);
            return;
        }
}

/* ---- saving ------------------------------------------------------------------------------------- */
void ivw_save_now(void) {
    if (ig.demo) return;
    igs.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &igs, (int)sizeof igs);
}

static void load_save(void) {
    IvwSave tmp;
    memset(&igs, 0, sizeof igs);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) igs = tmp;
}

static void goals_from_save(void) {
    if (igs.gifts) game_award(GOAL_BEACON);
    if (igs.wins) game_award(GOAL_SAUCER);
    if (igs.cherries) game_award(GOAL_ALIEN);
}

/* ---- music ---------------------------------------------------------------------------------------- */
void ivw_floor_music(void) {
    int b = IVW_FLOOR[ig.hero.floor].biome;
    music_play(b == BIO_GAOL ? IVW_MUS_GAOL : b == BIO_WARREN ? IVW_MUS_WARREN : IVW_MUS_STAR);
}

/* ---- the run ---------------------------------------------------------------------------------------- */
static void new_hero(void) {
    IvwHero *h = &ig.hero;
    memset(h, 0, sizeof *h);
    h->stat[0] = h->stat[1] = h->stat[2] = 5;
    h->inv_cap = 10;
    h->inv[0] = IT_DIRK;
    h->inv[1] = IT_RAGS;
    h->inv[2] = IT_BUCKLER;
    h->wpn = 0; h->arm = 1; h->shd = 2;
    h->hp = (int16_t)ivw_max_hp(h);
    h->mp = (int16_t)ivw_max_mp(h);
    h->st = (int16_t)ivw_max_st(h);
}

static void clear_view(void) {
    memset(ig.msg, 0, sizeof ig.msg);
    ig.anim_t = ig.anim_kind = 0;
    memset(&ig.cast, 0, sizeof ig.cast);
    memset(&ig.f, 0, sizeof ig.f);
    ig.buff = 0;
}

/* a new floor: the knight arrives, the game saves */
void ivw_floor_arrive(int floor) {
    IvwHero *h = &ig.hero;
    int x, y, d;
    h->floor = (uint8_t)floor;
    ivw_floor_start(floor, &x, &y, &d);
    h->x = (int8_t)x; h->y = (int8_t)y; h->dir = (int8_t)d;
    h->poison = h->burn = 0;
    clear_view();
    igs.hero = *h;
    igs.rng_state = ig.rng.state;
    igs.rng_inc = ig.rng.inc;
    igs.has_run = 1;
    if (floor > igs.best_floor) igs.best_floor = (uint8_t)floor;
    ivw_save_now();
    ig.floor_start = *h;
    ivw_floor_load(floor);
    ivw_reveal_around();
    ivw_set_state(VS_CARD);
    game_set_pausable(true);
    ivw_floor_music();
}

/* back to the start of the floor, as it was saved */
static void reload_floor(void) {
    ig.hero = igs.hero;
    ig.rng.state = igs.rng_state;
    ig.rng.inc = igs.rng_inc;
    clear_view();
    ivw_floor_load(ig.hero.floor);
    ivw_reveal_around();
    ivw_set_state(VS_CARD);
    game_set_pausable(true);
    ivw_floor_music();
}

void ivw_game_over(void) {
    if (ig.state == VS_OVER) return;
    if (igs.deaths < 0xFFFFFFF0u && !ig.demo) igs.deaths++;
    ig.f.active = 0;
    ig.cast.on = 0;
    ivw_save_now();
    ivw_set_state(VS_OVER);
    game_set_pausable(false);
    music_play(IVW_MUS_OVER);
}

static void descend(void) {
    IvwHero *h = &ig.hero;
    if (h->floor <= 2) {
        int lit;
        h->torch_all[h->floor] = (uint8_t)ivw_torches(&lit);
        h->torch_lit[h->floor] = (uint8_t)lit;
    }
    ivw_sfx("ivw_stairs");
    ivw_floor_arrive(h->floor + 1);
}

void ivw_win_escape(void) {
    ig.won = true;
    ig.ending_kind = ig.dawn_kill ? 1 : 0;
    game_award(GOAL_SAUCER);
    if (igs.wins < 255) igs.wins++;
    if (ig.dawn_kill) {
        game_award(GOAL_ALIEN);
        if (igs.cherries < 255) igs.cherries++;
    }
    igs.has_run = 0;
    ivw_save_now();
    ivw_set_state(VS_ENDING);
    game_set_pausable(false);
    music_play(IVW_MUS_ENDING);
}

static void new_game(void) {
    memset(&ig.hero, 0, sizeof ig.hero);
    ig.won = false;
    ig.dawn_kill = 0;
    ig.easter = 0;
    rng_seed(&ig.rng, ((uint64_t)rng_next(&g_rng) << 32) | rng_next(&g_rng));
    new_hero();
    ivw_set_state(VS_STORY);
    music_play(IVW_MUS_TITLE);
}

/* ---- the attract mode: left on the title, the demo player shows a later floor ---- */
#define DEMO_FRAMES 1800
static int demo_t;

static void start_demo(void) {
    IvwHero *h = &ig.hero;
    ig.demo = true;
    demo_t = 0;
    rng_seed(&ig.rng, (uint64_t)(1000 + ig.frame_t));
    memset(h, 0, sizeof *h);
    h->stat[0] = 12; h->stat[1] = 12; h->stat[2] = 12;
    h->inv_cap = 10;
    static const uint8_t KIT[6] = {IT_STAR, IT_IRON, IT_WYRM, IT_RED, IT_RED, IT_BLUE};
    memcpy(h->inv, KIT, sizeof KIT);
    h->wpn = 0; h->arm = 1; h->shd = 2;
    h->thrones_used = 0x7F;
    h->floor = 5;
    h->x = 20; h->y = 14; h->dir = 3;
    h->hp = (int16_t)ivw_max_hp(h);
    h->mp = (int16_t)ivw_max_mp(h);
    h->st = (int16_t)ivw_max_st(h);
    memset(ig.msg, 0, sizeof ig.msg);
    ivw_floor_load(5);
    for (int i = 0; i < 3; i++) ig.w.flame[i] = (IvwFlame){1, (uint8_t)(16 - i * 2), 14, 0, 0};
    ivw_reveal_around();
    ivw_bot_reset();
    ivw_set_state(VS_EXPLORE);
    ivw_floor_music();
}

static void to_title(void);

/* true while the demo runs (this frame's buttons are the demo player's) */
static bool demo_frame(void) {
    if (!ig.demo) return false;
    if ((input_down() & 0xFF) || ++demo_t > DEMO_FRAMES || ig.state == VS_OVER || ig.state == VS_ENDING ||
        ig.state == VS_CONFIRM || ig.hero.floor != 5) {
        ig.demo = false;
        input_consume();
        memset(&ig.hero, 0, sizeof ig.hero);
        memset(&ig.f, 0, sizeof ig.f);
        to_title();
        return false;
    }
    input_set_raw((uint32_t)ivw_bot_buttons());
    input_update();
    return true;
}

static void to_title(void) {
    ivw_set_state(VS_TITLE);
    ig.title_sel = igs.has_run ? 1 : 0;
    game_set_pausable(false);
    music_play(IVW_MUS_TITLE);
}

/* ---- the screens' input ------------------------------------------------------------------------------- */

static int menu_move(int sel, int n) {
    if (n <= 0) return 0;
    if (btn_repeat(BTN_UP)) { sel = (sel + n - 1) % n; ivw_sfx("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { sel = (sel + 1) % n; ivw_sfx("ui_move"); }
    return sel;
}

static bool pressed_a(void) {
    if (!btnp(BTN_A)) return false;
    input_consume();
    return true;
}

static bool pressed_b(void) {
    if (!btnp(BTN_B)) return false;
    input_consume();
    return true;
}

/* the item screen's choices for a slot */
enum { IA_EQUIP, IA_REMOVE, IA_DRINK, IA_READY, IA_UNREADY, IA_USE, IA_DROP, IA_BACK };
const char *const IVW_IA_NAME[8] = {"EQUIP", "TAKE OFF", "DRINK", "READY", "UNREADY", "USE", "DROP", "BACK"};

static void build_item_menu(int slot) {
    IvwHero *h = &ig.hero;
    int it = h->inv[slot];
    const IvwItem *d = &IVW_ITEM[it];
    ig.nitem_menu = 0;
    bool worn = slot == h->wpn || slot == h->arm || slot == h->shd;
    if (d->kind == IK_WEAPON || d->kind == IK_ARMOUR || d->kind == IK_SHIELD)
        ig.item_menu[ig.nitem_menu++] = (uint8_t)(worn ? IA_REMOVE : IA_EQUIP);
    if (d->kind == IK_POTION) ig.item_menu[ig.nitem_menu++] = IA_DRINK;
    if ((d->kind == IK_PART && it != IT_BONE && it != IT_HEAD) || it == IT_BEETLE)
        ig.item_menu[ig.nitem_menu++] = (uint8_t)(h->ready[slot] ? IA_UNREADY : IA_READY);
    if (it == IT_BONE || it == IT_HEAD || it == IT_COIN) ig.item_menu[ig.nitem_menu++] = IA_USE;
    ig.item_menu[ig.nitem_menu++] = IA_DROP;
    ig.item_menu[ig.nitem_menu++] = IA_BACK;
}

static void item_do(int slot, int a) {
    IvwHero *h = &ig.hero;
    int it = h->inv[slot];
    const IvwItem *d = &IVW_ITEM[it];
    switch (a) {
    case IA_EQUIP:
        if (d->kind == IK_WEAPON) h->wpn = (int8_t)slot;
        if (d->kind == IK_ARMOUR) h->arm = (int8_t)slot;
        if (d->kind == IK_SHIELD) h->shd = (int8_t)slot;
        if (d->kind == IK_WEAPON && h->stat[STAT_MIGHT] < d->req) ivw_say("%s: IT NEEDS %d MIGHT. IT FEELS CLUMSY.", d->name, d->req);
        else ivw_say("YOU TAKE UP THE %s.", d->name);
        ivw_sfx("ivw_equip");
        ivw_set_state(VS_INV);
        return;
    case IA_REMOVE:
        if (slot == h->wpn) h->wpn = -1;
        if (slot == h->arm) h->arm = -1;
        if (slot == h->shd) h->shd = -1;
        ivw_set_state(VS_INV);
        return;
    case IA_READY: h->ready[slot] = 1; ivw_say("%s: READY FOR THE NEXT FIGHT.", d->name); ivw_set_state(VS_INV); return;
    case IA_UNREADY: h->ready[slot] = 0; ivw_set_state(VS_INV); return;
    case IA_DRINK: ivw_set_state(VS_EXPLORE); ivw_use_item_world(slot); return;
    case IA_USE: ivw_set_state(VS_EXPLORE); ivw_use_item_world(slot); return;
    case IA_DROP:
        if (slot == h->wpn || slot == h->arm || slot == h->shd) { ivw_say("TAKE IT OFF FIRST."); ivw_set_state(VS_INV); return; }
        ivw_remove_slot(slot);
        ivw_drop_item(it, h->x, h->y);
        ivw_say("YOU SET DOWN THE %s.", d->name);
        ivw_set_state(VS_EXPLORE);
        ivw_pass_turn();
        return;
    default: ivw_set_state(VS_INV); return;
    }
}

/* the gem panel: no two of a colour side by side */
static bool gems_solved(const uint8_t *b) {
    for (int i = 0; i < 9; i++) {
        int x = i % 3, y = i / 3;
        if (x < 2 && b[i] == b[i + 1]) return false;
        if (y < 2 && b[i] == b[i + 3]) return false;
    }
    return true;
}

static void gems_update(void) {
    int x = ig.gem_sel % 3, y = ig.gem_sel / 3;
    if (btnp(BTN_LEFT) && x > 0) ig.gem_sel--;
    if (btnp(BTN_RIGHT) && x < 2) ig.gem_sel++;
    if (btnp(BTN_UP) && y > 0) ig.gem_sel -= 3;
    if (btnp(BTN_DOWN) && y < 2) ig.gem_sel += 3;
    if (pressed_b()) { ivw_set_state(VS_EXPLORE); return; }
    if (!pressed_a()) return;
    if (ig.gem_held < 0) { ig.gem_held = ig.gem_sel; ivw_sfx("ivw_gem"); return; }
    uint8_t t = ig.gem_board[ig.gem_held];
    ig.gem_board[ig.gem_held] = ig.gem_board[ig.gem_sel];
    ig.gem_board[ig.gem_sel] = t;
    ig.gem_held = -1;
    ivw_sfx("ivw_gem");
    if (gems_solved(ig.gem_board)) {
        int n = rng_range(&ig.rng, 4, 8);
        ig.hero.shards = (int16_t)(ig.hero.shards + n);
        ig.w.st[ig.gem_panel_y][ig.gem_panel_x] = 1;
        ivw_say("THE GEMS CLICK. %d SHARDS FALL OUT.", n);
        ivw_sfx("ivw_coin");
        ivw_set_state(VS_EXPLORE);
        ivw_pass_turn();
    }
}

static int shop_price(int item) { return imax(2, IVW_ITEM[item].value * 2); }

static void shop_update(void) {
    IvwHero *h = &ig.hero;
    if (btnp(BTN_LEFT) || btnp(BTN_RIGHT)) { ig.shop_buy ^= 1; ig.shop_sel = 0; ivw_sfx("ui_move"); }
    int n = ig.shop_buy ? 4 : pack_count();
    ig.shop_sel = menu_move(ig.shop_sel, imax(1, n));
    if (pressed_b()) { ivw_set_state(VS_EXPLORE); return; }
    if (!pressed_a() || n == 0) return;
    if (ig.shop_buy) {
        int it = ig.w.shop[ig.shop_sel];
        if (!it) return;
        int p = shop_price(it);
        if (h->shards < p) { ivw_say("THAT'S %d SHARDS, DEARIE.", p); ivw_sfx("ui_error"); return; }
        if (!ivw_give_item(it)) { ivw_say("YOUR PACK IS FULL, DEARIE."); ivw_sfx("ui_error"); return; }
        h->shards = (int16_t)(h->shards - p);
        ig.w.shop[ig.shop_sel] = 0;
        ivw_say("BOUGHT: %s.", IVW_ITEM[it].name);
        ivw_sfx("ivw_coin");
    } else {
        int s = ig.shop_sel, it = h->inv[s];
        if (s == h->wpn || s == h->arm || s == h->shd || IVW_ITEM[it].kind == IK_KEY) { ivw_say("NOT THAT ONE."); return; }
        if (!IVW_ITEM[it].value) { ivw_say("WORTHLESS TO ME, DEARIE."); return; }
        h->shards = (int16_t)(h->shards + IVW_ITEM[it].value);
        ivw_remove_slot(s);
        if (ig.shop_sel >= pack_count()) ig.shop_sel = imax(0, pack_count() - 1);
        ivw_say("SOLD FOR %d SHARDS.", IVW_ITEM[it].value);
        ivw_sfx("ivw_coin");
    }
}

static void throne_update(void) {
    IvwHero *h = &ig.hero;
    ig.throne_sel = menu_move(ig.throne_sel, 4);
    int s = ig.throne_sel;
    if (s < 3 && (btnp(BTN_RIGHT) || pressed_a()) && ig.throne_pts > 0 && h->stat[s] < 20) {
        h->stat[s]++;
        ig.throne_pts--;
        ivw_sfx("ivw_point");
        return;
    }
    if (s < 3 && btnp(BTN_LEFT) && h->stat[s] > ig.stat_before[s]) { h->stat[s]--; ig.throne_pts++; ivw_sfx("ui_back"); return; }
    if (s == 3 && pressed_a() && ig.throne_pts == 0) {
        /* health, mana and stamina grow with the new numbers */
        h->hp = (int16_t)imin(ivw_max_hp(h), h->hp + imax(0, ivw_max_hp(h) - h->hp) / 2);
        h->mp = (int16_t)imin(ivw_max_mp(h), h->mp);
        h->st = (int16_t)ivw_max_st(h);
        ivw_check_gift();
        ivw_say("THE THRONE'S POWER SETTLES IN YOU.");
        ivw_set_state(VS_EXPLORE);
        ivw_floor_music();
        ivw_pass_turn();
    }
}

static void teleport_update(void) {
    if (btnp(BTN_LEFT)) ig.tele_x = imax(0, ig.tele_x - 1);
    if (btnp(BTN_RIGHT)) ig.tele_x = imin(IVW_W - 1, ig.tele_x + 1);
    if (btnp(BTN_UP)) ig.tele_y = imax(0, ig.tele_y - 1);
    if (btnp(BTN_DOWN)) ig.tele_y = imin(IVW_H - 1, ig.tele_y + 1);
    if (pressed_b()) { ivw_set_state(VS_EXPLORE); return; }
    if (pressed_a()) ivw_teleport_to(ig.tele_x, ig.tele_y);
}

static int pick_count(void) {
    if (ig.pick_mode == 2) {
        uint8_t items[16];
        return ivw_drops_here(ig.hero.x, ig.hero.y, items, 16);
    }
    if (ig.pick_mode == 3) return 4;
    return pack_count();
}

static void ivw_update(void) {
    ig.state_t++;
    ig.frame_t++;
    if (ig.msg_t > 0) ig.msg_t--;
    if (ig.clock_t > 0) ig.clock_t--;
    if (ig.cast.result_t > 0) ig.cast.result_t--;
    if (ig.demo && !demo_frame()) return;
    switch (ig.state) {
    case VS_TITLE:
        game_set_pausable(false);
        if (input_down() & 0xFF) ig.state_t = imin(ig.state_t, 20);
        if (ig.state_t > 900) { start_demo(); break; }
        if (pressed_b()) { game_exit_to_library(); break; }
        if (igs.has_run) ig.title_sel = menu_move(ig.title_sel, 2);
        if (ig.state_t > 10 && (pressed_a() || btnp(BTN_START))) {
            ivw_sfx("ui_ok");
            input_consume();
            if (ig.title_sel == 1 && igs.has_run) reload_floor();
            else new_game();
        }
        break;
    case VS_STORY:
        if ((ig.state_t > 20 && (pressed_a() || btnp(BTN_START))) || ig.state_t > 1200) {
            input_consume();
            ivw_floor_arrive(0);
        }
        break;
    case VS_CARD:
        if (ig.state_t > 120 || (ig.state_t > 15 && pressed_a())) ivw_set_state(VS_EXPLORE);
        break;
    case VS_EXPLORE:
        if (ig.cast.on) ivw_cast_update();
        else ivw_explore_update();
        break;
    case VS_MENU:
        ig.menu_sel = menu_move(ig.menu_sel, ig.nmenu);
        if (pressed_b()) { ivw_set_state(VS_EXPLORE); break; }
        if (pressed_a()) { ivw_sfx("ui_ok"); ivw_menu_do(ig.menu[ig.menu_sel]); }
        break;
    case VS_INV: {
        int n = pack_count();
        ig.inv_sel = menu_move(ig.inv_sel, imax(1, n));
        if (pressed_b()) { ivw_set_state(VS_EXPLORE); break; }
        if (pressed_a() && n) { build_item_menu(ig.inv_sel); ig.item_sel = 0; ivw_set_state(VS_ITEM); }
        break;
    }
    case VS_ITEM:
        ig.item_sel = menu_move(ig.item_sel, ig.nitem_menu);
        if (pressed_b()) { ivw_set_state(VS_INV); break; }
        if (pressed_a()) item_do(ig.inv_sel, ig.item_menu[ig.item_sel]);
        break;
    case VS_PICK: {
        int n = pick_count();
        ig.pick_sel = menu_move(ig.pick_sel, imax(1, n));
        if (pressed_b()) { ivw_set_state(VS_EXPLORE); break; }
        if (pressed_a() && n) ivw_pick_do(ig.pick_sel);
        break;
    }
    case VS_SHOP: shop_update(); break;
    case VS_THRONE: throne_update(); break;
    case VS_GEMS: gems_update(); break;
    case VS_WISH:
        ig.wish_sel = menu_move(ig.wish_sel, 3);
        if (pressed_a()) ivw_wish(ig.wish_sel);
        break;
    case VS_GLYPH: case VS_MAP:
        if (ig.state_t > 10 && (pressed_a() || pressed_b())) ivw_set_state(VS_EXPLORE);
        break;
    case VS_TALK:
        if (ig.state_t > 8 && (pressed_a() || pressed_b())) {
            if (++ig.talk_page >= ig.ntalk) ivw_set_state(VS_EXPLORE);
            else ig.state_t = 0;
        }
        break;
    case VS_FIGHT: ivw_fight_update(); break;
    case VS_LOOT:
        if (ig.state_t > 30 && pressed_a()) {
            IvwFight *f = &ig.f;
            for (int i = 0; i < f->nloot; i++) {
                int it = f->loot[i];
                if (ivw_give_item(it)) ivw_say("%s.", IVW_ITEM[it].name);
                else { ivw_drop_item(it, ig.hero.x, ig.hero.y); ivw_say("%s FALLS TO THE FLOOR.", IVW_ITEM[it].name); }
            }
            ivw_fight_done();
        }
        break;
    case VS_OVER:
        if (ig.state_t > 60 && pressed_a()) reload_floor();
        else if (ig.state_t > 60 && pressed_b()) to_title();
        break;
    case VS_ENDING:
        if (btn(BTN_A)) ig.state_t += 2;
        if (ig.state_t > 1500) { input_consume(); to_title(); }
        break;
    case VS_CONFIRM:
        if (btnp(BTN_LEFT) || btnp(BTN_RIGHT) || btnp(BTN_UP) || btnp(BTN_DOWN)) { ig.confirm_sel ^= 1; ivw_sfx("ui_move"); }
        if (pressed_b()) { ivw_set_state(VS_EXPLORE); break; }
        if (pressed_a()) {
            if (ig.confirm_sel == 1) descend();
            else ivw_set_state(VS_EXPLORE);
        }
        break;
    case VS_TELEPORT: teleport_update(); break;
    default: break;
    }
}

/* ---- cartridge interface ------------------------------------------------------------------------------------- */

static void ivw_load(void) {
    ivw_art_load();
    ivw_audio_load();
}

static void ivw_start(void) {
    extern int ivw_no_loot;
    ivw_no_loot = 0;
    load_save();
    memset(&ig, 0, sizeof ig);
    rng_seed(&ig.rng, 37);
    goals_from_save();
    ivw_bot_reset();
    to_title();
}

static void ivw_quit(void) { ivw_save_now(); }

static int popcount8(int v) {
    int n = 0;
    for (; v; v >>= 1) n += v & 1;
    return n;
}

static int foe_index(const char *key, int skip) {
    int i = atoi(key + skip);
    return iclamp(i, 0, IVW_MAX_FOES - 1);
}

static int ivw_query(const char *key, int *out) {
    IvwHero *h = &ig.hero;
    IvwWorld *w = &ig.w;
    IvwFight *f = &ig.f;
    if (!strcmp(key, "bot")) { *out = ivw_bot_buttons(); return 1; }
    if (!strcmp(key, "state")) { *out = ig.state; return 1; }
    if (!strcmp(key, "floor")) { *out = h->floor; return 1; }
    if (!strcmp(key, "x")) { *out = h->x; return 1; }
    if (!strcmp(key, "y")) { *out = h->y; return 1; }
    if (!strcmp(key, "dir")) { *out = h->dir; return 1; }
    if (!strcmp(key, "hp")) { *out = h->hp; return 1; }
    if (!strcmp(key, "hp_max")) { *out = ivw_max_hp(h); return 1; }
    if (!strcmp(key, "mp")) { *out = h->mp; return 1; }
    if (!strcmp(key, "mp_max")) { *out = ivw_max_mp(h); return 1; }
    if (!strcmp(key, "st")) { *out = h->st / 16; return 1; }
    if (!strcmp(key, "st_max")) { *out = ivw_max_st(h) / 16; return 1; }
    if (!strcmp(key, "might")) { *out = h->stat[0]; return 1; }
    if (!strcmp(key, "grit")) { *out = h->stat[1]; return 1; }
    if (!strcmp(key, "lore")) { *out = h->stat[2]; return 1; }
    if (!strcmp(key, "essence")) { *out = h->essence; return 1; }
    if (!strcmp(key, "shards")) { *out = h->shards; return 1; }
    if (!strcmp(key, "keys")) { *out = h->keys; return 1; }
    if (!strcmp(key, "poison")) { *out = h->poison; return 1; }
    if (!strcmp(key, "burn")) { *out = h->burn; return 1; }
    if (!strcmp(key, "turn")) { *out = w->turn; return 1; }
    if (!strcmp(key, "veil")) { *out = w->veil; return 1; }
    if (!strcmp(key, "fly")) { *out = w->fly; return 1; }
    if (!strcmp(key, "shift")) { *out = w->shift; return 1; }
    if (!strcmp(key, "slime_b")) { *out = w->slime_b; return 1; }
    if (!strcmp(key, "spikes_up")) { *out = ivw_spikes_up(); return 1; }
    if (!strcmp(key, "battles")) { *out = w->battles; return 1; }
    if (!strcmp(key, "searches")) { *out = w->searches; return 1; }
    if (!strcmp(key, "escape")) { *out = w->escape; return 1; }
    if (!strcmp(key, "boss_dead")) { *out = w->boss_dead; return 1; }
    if (!strcmp(key, "pack")) { *out = pack_count(); return 1; }
    if (!strcmp(key, "pack_cap")) { *out = h->inv_cap; return 1; }
    if (!strcmp(key, "wpn")) { *out = ivw_weapon_item(); return 1; }
    if (!strcmp(key, "arm")) { *out = ivw_armour_item(); return 1; }
    if (!strcmp(key, "shd")) { *out = ivw_shield_item(); return 1; }
    if (!strncmp(key, "has", 3)) { *out = ivw_count_item(atoi(key + 3)); return 1; }
    if (!strncmp(key, "drops", 5)) { uint8_t it[16]; *out = ivw_drops_here(h->x, h->y, it, 16); return 1; }
    if (!strcmp(key, "flames")) { int n = 0; for (int i = 0; i < IVW_MAX_FLAME; i++) n += w->flame[i].alive; *out = n; return 1; }
    if (!strcmp(key, "menu_n")) { *out = ig.nmenu; return 1; }
    if (!strncmp(key, "menu", 4) && key[4] >= '0' && key[4] <= '9') { int i = atoi(key + 4); *out = i < ig.nmenu ? ig.menu[i] : -1; return 1; }
    if (!strcmp(key, "vmenu_sel")) { *out = ig.menu_sel; return 1; }
    if (!strcmp(key, "inv_sel")) { *out = ig.inv_sel; return 1; }
    if (!strcmp(key, "msg_t")) { *out = ig.msg_t; return 1; }
    if (!strcmp(key, "knight_favour")) { *out = h->knight_favour; return 1; }
    if (!strcmp(key, "knight_gone")) { *out = h->knight_gone; return 1; }
    if (!strcmp(key, "honor")) { *out = h->honor; return 1; }
    if (!strcmp(key, "dawn_given")) { *out = h->dawn_given; return 1; }
    if (!strcmp(key, "coin_given")) { *out = h->coin_given; return 1; }
    if (!strcmp(key, "secrets")) { *out = h->secrets; return 1; }
    if (!strcmp(key, "hp_bonus")) { *out = h->hp_bonus; return 1; }
    if (!strcmp(key, "throne_pts")) { *out = ig.throne_pts; return 1; }
    if (!strcmp(key, "dawn_kill")) { *out = ig.dawn_kill; return 1; }
    if (!strcmp(key, "ending")) { *out = ig.ending_kind; return 1; }
    if (!strcmp(key, "easter")) { *out = ig.easter; return 1; }
    if (!strcmp(key, "buff")) { *out = ig.buff; return 1; }
    if (!strcmp(key, "egg_fights")) { *out = h->egg_fights; return 1; }
    if (!strcmp(key, "bosses")) { *out = h->bosses; return 1; }
    /* the tile ahead and here */
    if (!strcmp(key, "ahead")) { *out = ivw_tile(h->x + IVW_DX[h->dir], h->y + IVW_DY[h->dir]); return 1; }
    if (!strcmp(key, "ahead_st")) {
        int ax = h->x + IVW_DX[h->dir], ay = h->y + IVW_DY[h->dir];
        *out = (ax >= 0 && ay >= 0 && ax < IVW_W && ay < IVW_H) ? w->st[ay][ax] : 0;
        return 1;
    }
    if (!strcmp(key, "here")) { *out = ivw_tile(h->x, h->y); return 1; }
    if (!strncmp(key, "tile_", 5)) { int x = 0, y = 0; sscanf(key + 5, "%d_%d", &x, &y); *out = ivw_tile(x, y); return 1; }
    if (!strncmp(key, "st_", 3)) { int x = 0, y = 0; sscanf(key + 3, "%d_%d", &x, &y); *out = (x >= 0 && y >= 0 && x < IVW_W && y < IVW_H) ? w->st[y][x] : 0; return 1; }
    /* casting */
    if (!strcmp(key, "casting")) { *out = ig.cast.on; return 1; }
    if (!strcmp(key, "cast_result")) { *out = ig.cast.result; return 1; }
    if (!strcmp(key, "cast_window")) { *out = ivw_cast_window(); return 1; }
    /* the fight */
    if (!strcmp(key, "fight")) { *out = ig.state == VS_FIGHT; return 1; }
    if (!strcmp(key, "f_px")) { *out = f->px / 16; return 1; }
    if (!strcmp(key, "f_pstate")) { *out = f->pstate; return 1; }
    if (!strcmp(key, "f_combo")) { *out = f->combo; return 1; }
    if (!strcmp(key, "f_exhausted")) { *out = f->exhausted; return 1; }
    if (!strcmp(key, "f_blocks")) { *out = f->blocks_left; return 1; }
    if (!strcmp(key, "f_foes")) { *out = ivw_fight_foes_alive(); return 1; }
    if (!strcmp(key, "f_nfoe")) { *out = f->nfoe; return 1; }
    if (!strcmp(key, "f_result")) { *out = f->result; return 1; }
    if (!strcmp(key, "f_essence")) { *out = f->essence_won; return 1; }
    if (!strcmp(key, "f_nloot")) { *out = f->nloot; return 1; }
    if (!strcmp(key, "f_loot0")) { *out = f->nloot ? f->loot[0] : 0; return 1; }
    if (!strcmp(key, "f_invis")) { *out = f->invis; return 1; }
    if (!strcmp(key, "f_haste")) { *out = f->haste; return 1; }
    if (!strcmp(key, "f_ward")) { *out = f->ward; return 1; }
    if (!strcmp(key, "f_twin")) { *out = f->twin; return 1; }
    if (!strcmp(key, "f_silence")) { *out = f->silence; return 1; }
    if (!strcmp(key, "f_beetle")) { *out = f->beetle && !f->beetle_out; return 1; }
    if (!strcmp(key, "f_ball")) { *out = f->ball_kind; return 1; }
    if (!strcmp(key, "f_shots")) { int n = 0; for (int i = 0; i < IVW_MAX_SHOTS; i++) n += f->shot[i].alive; *out = n; return 1; }
    if (!strcmp(key, "f_phase")) { *out = f->regent_phase; return 1; }
    if (!strcmp(key, "f_swaps")) { *out = f->swaps; return 1; }
    if (!strcmp(key, "f_t")) { *out = (int)f->t; return 1; }
    if (!strcmp(key, "f_front")) { int n = 0; for (int i = 0; i < f->nfoe; i++) n += f->foe[i].alive && f->foe[i].row == 0; *out = n; return 1; }
    if (!strcmp(key, "f_recover")) { *out = f->recover_hp; return 1; }
    if (!strncmp(key, "fk", 2)) { *out = f->foe[foe_index(key, 2)].kind; return 1; }
    if (!strncmp(key, "fhp", 3)) { *out = f->foe[foe_index(key, 3)].hp; return 1; }
    if (!strncmp(key, "fmax", 4)) { *out = f->foe[foe_index(key, 4)].hpmax; return 1; }
    if (!strncmp(key, "fx", 2)) { *out = f->foe[foe_index(key, 2)].x / 16; return 1; }
    if (!strncmp(key, "frow", 4)) { *out = f->foe[foe_index(key, 4)].row; return 1; }
    if (!strncmp(key, "fstate", 6)) { *out = f->foe[foe_index(key, 6)].state; return 1; }
    if (!strncmp(key, "falive", 6)) { *out = f->foe[foe_index(key, 6)].alive; return 1; }
    if (!strncmp(key, "fslow", 5)) { *out = f->foe[foe_index(key, 5)].slow; return 1; }
    if (!strncmp(key, "ffreeze", 7)) { *out = f->foe[foe_index(key, 7)].freeze; return 1; }
    if (!strncmp(key, "fburn", 5)) { *out = f->foe[foe_index(key, 5)].burn; return 1; }
    if (!strncmp(key, "fpoison", 7)) { *out = f->foe[foe_index(key, 7)].poison; return 1; }
    if (!strncmp(key, "fstun", 5)) { *out = f->foe[foe_index(key, 5)].stun; return 1; }
    if (!strncmp(key, "fcd", 3)) { *out = f->foe[foe_index(key, 3)].cd; return 1; }
    if (!strncmp(key, "fstrikes", 8)) { *out = f->foe[foe_index(key, 8)].strikes; return 1; }
    if (!strncmp(key, "falt", 4)) { *out = f->foe[foe_index(key, 4)].alt; return 1; }
    /* the records */
    if (!strcmp(key, "save_battles")) { *out = (int)igs.battles; return 1; }
    if (!strcmp(key, "save_deaths")) { *out = (int)igs.deaths; return 1; }
    if (!strcmp(key, "save_secrets")) { *out = popcount8(igs.secrets_seen); return 1; }
    if (!strcmp(key, "save_has_run")) { *out = igs.has_run; return 1; }
    if (!strcmp(key, "save_floor")) { *out = igs.hero.floor; return 1; }
    if (!strcmp(key, "save_wins")) { *out = igs.wins; return 1; }
    if (!strcmp(key, "save_cherries")) { *out = igs.cherries; return 1; }
    if (!strcmp(key, "title_sel")) { *out = ig.title_sel; return 1; }
    if (!strcmp(key, "demo")) { *out = ig.demo; return 1; }
    if (!strcmp(key, "layout")) { *out = ivw_layout_audit(); return 1; }
    if (!strcmp(key, "glyphs_ok")) {
        /* every glyph is its own: none equals another's or its mirror */
        int bad = 0;
        for (int s = 0; s < SP_COUNT; s++) bad += ivw_glyph_spell(IVW_SPELL[s].glyph) != s;
        *out = bad;
        return 1;
    }
    return 0;
}

static int item_by_name(const char *s) {
    for (int i = 1; i < IT_COUNT; i++) {
        char nm[32];
        snprintf(nm, sizeof nm, "%s", IVW_ITEM[i].name);
        for (char *p = nm; *p; p++) if (*p == ' ' || *p == '\'') *p = '_';
        if (!strcmp(nm, s)) return i;
    }
    return atoi(s);
}

static int ivw_cheat(const char *cmd) {
    IvwHero *h = &ig.hero;
    IvwWorld *w = &ig.w;
    int a, b, c, d;
    char word[40];
    if (sscanf(cmd, "newgame %d", &a) == 1) {
        /* a fresh run with this seed, standing on floor 0 */
        memset(&ig.hero, 0, sizeof ig.hero);
        rng_seed(&ig.rng, (uint64_t)a);
        new_hero();
        ivw_floor_arrive(0);
        ivw_set_state(VS_EXPLORE);
        return 1;
    }
    if (sscanf(cmd, "floor %d", &a) == 1) { ivw_floor_arrive(iclamp(a, 0, IVW_FLOORS - 1)); ivw_set_state(VS_EXPLORE); return 1; }
    if (sscanf(cmd, "pos %d %d %d", &a, &b, &c) == 3) { h->x = (int8_t)a; h->y = (int8_t)b; h->dir = (int8_t)(c & 3); ivw_reveal_around(); return 1; }
    if (sscanf(cmd, "stats %d %d %d", &a, &b, &c) == 3) {
        h->stat[0] = (uint8_t)a; h->stat[1] = (uint8_t)b; h->stat[2] = (uint8_t)c;
        h->hp = (int16_t)ivw_max_hp(h); h->mp = (int16_t)ivw_max_mp(h); h->st = (int16_t)ivw_max_st(h);
        return 1;
    }
    if (sscanf(cmd, "give %39s", word) == 1) { ivw_give_item(item_by_name(word)); return 1; }
    if (sscanf(cmd, "drop %39s", word) == 1) { ivw_drop_item(item_by_name(word), h->x, h->y); return 1; }
    if (sscanf(cmd, "equip %d", &a) == 1) {
        int it = h->inv[a];
        if (IVW_ITEM[it].kind == IK_WEAPON) h->wpn = (int8_t)a;
        if (IVW_ITEM[it].kind == IK_ARMOUR) h->arm = (int8_t)a;
        if (IVW_ITEM[it].kind == IK_SHIELD) h->shd = (int8_t)a;
        return 1;
    }
    if (sscanf(cmd, "ready %d", &a) == 1) { h->ready[a] = 1; return 1; }
    if (sscanf(cmd, "essence %d", &a) == 1) { h->essence = a; return 1; }
    if (sscanf(cmd, "shards %d", &a) == 1) { h->shards = (int16_t)a; return 1; }
    if (sscanf(cmd, "keys %d", &a) == 1) { h->keys = (uint8_t)a; return 1; }
    if (sscanf(cmd, "hp %d", &a) == 1) { h->hp = (int16_t)a; return 1; }
    if (sscanf(cmd, "mp %d", &a) == 1) { h->mp = (int16_t)a; return 1; }
    if (sscanf(cmd, "st %d", &a) == 1) { h->st = (int16_t)(a * 16); return 1; }
    if (sscanf(cmd, "poison %d", &a) == 1) { h->poison = (uint8_t)a; return 1; }
    if (sscanf(cmd, "honor %d", &a) == 1) { h->honor = (uint8_t)a; return 1; }
    if (sscanf(cmd, "turn %d", &a) == 1) { w->turn = a; return 1; }
    if (sscanf(cmd, "battles %d", &a) == 1) { w->battles = (uint8_t)a; return 1; }
    if (sscanf(cmd, "tile %d %d %d", &a, &b, &c) == 3) { w->tile[b][a] = (uint8_t)c; return 1; }
    if (sscanf(cmd, "tilest %d %d %d", &a, &b, &c) == 3) { w->st[b][a] = (uint8_t)c; return 1; }
    if (sscanf(cmd, "flame %d %d", &a, &b) == 2) {
        for (int i = 0; i < IVW_MAX_FLAME; i++)
            if (!w->flame[i].alive) { w->flame[i] = (IvwFlame){1, (uint8_t)a, (uint8_t)b, 0, 0}; break; }
        return 1;
    }
    if (!strcmp(cmd, "noflames")) { memset(w->flame, 0, sizeof w->flame); w->nspawn = 0; return 1; }
    if (sscanf(cmd, "fight %d %d %d %d", &a, &b, &c, &d) >= 1) {
        uint8_t k[4];
        int n = sscanf(cmd, "fight %d %d %d %d", &a, &b, &c, &d);
        int v[4] = {a, b, c, d};
        for (int i = 0; i < n; i++) k[i] = (uint8_t)v[i];
        ivw_fight_begin(k, n, BOSS_NONE);
        return 1;
    }
    if (sscanf(cmd, "boss %d", &a) == 1) {
        if (a == BOSS_WARDEN) { static const uint8_t K[1] = {FK_WARDEN}; ivw_fight_begin(K, 1, BOSS_WARDEN); }
        else if (a == BOSS_MATRON) { static const uint8_t K[3] = {FK_MATRON, FK_CHITIN, FK_CHITIN}; ivw_fight_begin(K, 3, BOSS_MATRON); }
        else { static const uint8_t K[4] = {FK_REGENT, FK_STONE, FK_STONE, FK_STONE}; ivw_fight_begin(K, 4, BOSS_REGENT); }
        return 1;
    }
    if (sscanf(cmd, "fhp %d %d", &a, &b) == 2) { ig.f.foe[a].hp = (int16_t)b; return 1; }
    if (sscanf(cmd, "fpos %d %d", &a, &b) == 2) { ig.f.foe[a].x = (int16_t)(b * 16); ig.f.foe[a].tx = ig.f.foe[a].x; return 1; }
    if (sscanf(cmd, "fcd %d %d", &a, &b) == 2) { ig.f.foe[a].cd = (int16_t)b; return 1; }
    if (sscanf(cmd, "frow %d %d", &a, &b) == 2) { ig.f.foe[a].row = (uint8_t)b; return 1; }
    if (sscanf(cmd, "fpx %d", &a) == 1) { ig.f.px = (int16_t)(a * 16); return 1; }
    if (sscanf(cmd, "spell %d", &a) == 1) {
        if (ig.state == VS_FIGHT) ivw_fight_spell(a);
        else ivw_spell_world(a);
        return 1;
    }
    if (sscanf(cmd, "botdebug %d", &a) == 1) { ivw_bot_debug = a; return 1; }
    if (sscanf(cmd, "noloot %d", &a) == 1) { extern int ivw_no_loot; ivw_no_loot = a; return 1; }
    if (!strcmp(cmd, "save")) { ivw_save_now(); return 1; }
    if (sscanf(cmd, "secrets %d", &a) == 1) { h->secrets = (uint8_t)a; return 1; }
    if (sscanf(cmd, "dawnkill %d", &a) == 1) { ig.dawn_kill = a; return 1; }
    if (!strcmp(cmd, "escape")) { w->escape = 1; return 1; }
    return 0;
}

const GameDef GAME_IRONVOW = {
    "ironvow",
    "IRONVOW",
    "1988",
    "ADVENTURE / RPG",
    "IN RAGS, A KNIGHT DESCENDS BLACKSPIRE KEEP TO END THE PIT REGENT.",
    {"REACH 8 IN ANY STAT", "SLAY THE PIT REGENT AND ESCAPE", "SLAY HIM WITH THE DAWNBRAND"},
    "WALK\t" GLYPH_UP GLYPH_DOWN " STEP " GLYPH_LEFT GLYPH_RIGHT " TURN\n"
    "\t" GLYPH_A " MENU (PACK, DOORS, LEVERS...)\n"
    "FIGHT\t" GLYPH_LEFT GLYPH_RIGHT " STRAFE " GLYPH_DOWN " GUARD\n"
    "\t" GLYPH_DOWN " THEN " GLYPH_LEFT GLYPH_RIGHT " ROLL\n"
    "\t" GLYPH_A " STRIKE (AGAIN: COMBO)\n"
    "SPELLS\t" GLYPH_B ", THEN DRAW THE GLYPH\n"
    "START\tPAUSE",
    C_MAROON, C_AMBER,
    ivw_load, ivw_start, ivw_update, ivw_draw, ivw_quit, ivw_draw_label, ivw_query, ivw_cheat,
    "VALBRACE", 37,
    NULL,
};
