/* HELLSPUR - text boxes, questions and lists; the B menu (items, skills,
 * equipment, stats, the order of the posse, key items, mounting up); and
 * the town counters: store, hotel, chapel, bank, saloon, stables, train. */
#include "spur.h"

#define BOX_X 8
#define BOX_Y 124
#define BOX_W 304
#define BOX_H 52
#define TEXT_W 290
#define PAGE_LINES 4

/* ---- text ------------------------------------------------------------------ */
static void add_line(const char *s, int n) {
    SpurUi *u = &sg.ui;
    if (u->nlines >= SPUR_TEXT_MAX * 3) return;
    n = imin(n, 63);
    memcpy(u->buf[u->nlines], s, (size_t)n);
    u->buf[u->nlines][n] = 0;
    u->lines[u->nlines] = u->buf[u->nlines];
    u->nlines++;
}

/* split into lines that fit, on '\n', and pages on '\f' */
static void wrap_into(const char *text) {
    SpurUi *u = &sg.ui;
    u->nlines = 0;
    char line[96];
    int ln = 0;
    const char *p = text;
    int on_page = 0;
    while (*p) {
        if (*p == '\f') {
            if (ln) { add_line(line, ln); ln = 0; on_page++; }
            while (on_page % PAGE_LINES) { add_line("", 0); on_page++; }
            p++;
            continue;
        }
        if (*p == '\n') {
            add_line(line, ln);
            ln = 0;
            on_page++;
            p++;
            continue;
        }
        /* the next word */
        const char *q = p;
        while (*q && *q != ' ' && *q != '\n' && *q != '\f') q++;
        char word[96];
        int wl = (int)imin((int)(q - p), 90);
        memcpy(word, p, (size_t)wl);
        word[wl] = 0;
        char test[192];
        snprintf(test, sizeof test, "%.*s%s%s", ln, line, ln ? " " : "", word);
        if (ln && text_width(test) > TEXT_W) {
            add_line(line, ln);
            on_page++;
            ln = 0;
            snprintf(test, sizeof test, "%s", word);
        }
        ln = (int)imin((int)strlen(test), 90);
        memcpy(line, test, (size_t)ln);
        line[ln] = 0;
        p = q;
        while (*p == ' ') p++;
    }
    if (ln) add_line(line, ln);
}

static void open_text(const char *text, int after) {
    SpurUi *u = &sg.ui;
    memset(u, 0, sizeof *u);
    wrap_into(text);
    u->kind = UI_TEXT;
    u->after = after;
    u->yes_event = u->no_event = -1;
}

void spur_say(const char *text) { open_text(text, 0); }
void spur_say_then(const char *text, int event) { open_text(text, event); }

void spur_ask(const char *text, int yes_event, int no_event) {
    open_text(text, 0);
    sg.ui.kind = UI_YESNO;
    sg.ui.yes_event = yes_event;
    sg.ui.no_event = no_event;
    sg.ui.sel = 1;
}

bool spur_ui_open(void) { return sg.ui.kind != UI_NONE; }

static void close_ui(void) {
    int after = sg.ui.after;
    sg.ui.kind = UI_NONE;
    input_consume();
    if (after) spur_event(after);
}

static int pages(void) { return (sg.ui.nlines + PAGE_LINES - 1) / PAGE_LINES; }

/* ---- picks and numbers ---------------------------------------------------------- */
static char pick_title[64];
static char pick_opt[12][40];
static int pick_n;

void spur_pick(const char *title, const char *const *opts, int n, int mode) {
    SpurUi *u = &sg.ui;
    memset(u, 0, sizeof *u);
    u->kind = UI_PICK;
    u->mode = mode;
    snprintf(pick_title, sizeof pick_title, "%s", title);
    pick_n = imin(n, 12);
    for (int i = 0; i < pick_n; i++) snprintf(pick_opt[i], sizeof pick_opt[i], "%s", opts[i]);
    input_consume();
}

void spur_number(const char *title, int max, int mode) {
    SpurUi *u = &sg.ui;
    memset(u, 0, sizeof *u);
    u->kind = UI_NUMBER;
    u->mode = mode;
    u->num = 0;
    u->num_max = max;
    u->sel = 0; /* the digit: 0 ones ... 5 hundred thousands */
    snprintf(pick_title, sizeof pick_title, "%s", title);
    input_consume();
}

/* ---- using things ------------------------------------------------------------------ */
int spur_heal_amount(int it) {
    const SpurItemDef *d = &SPUR_ITEM[it];
    int doc = spur_doc_level();
    if (doc >= 4 && (it == IT_MEDCHEST || it == IT_HALFCHEST)) return 999;
    return d->power * (4 + doc) / 4;
}

static int spirit_mult(int h, int v) { return v * (2 + spur_passive(h, PS_SPIRIT)) / 2; }

/* what a skill heals */
int spur_skill_heal(int h, int sk) {
    return spirit_mult(h, SPUR_SKILL[sk].power + spur_stat(h, ST_RES) / 2);
}

static void heal(int h, int amt) {
    SpurHero *hr = &sg.s.hero[h];
    hr->hp = (int16_t)imin(spur_maxhp(h), hr->hp + amt);
}

/* the doctor's trick: a used item may leave half of itself behind */
static void consume(int slot) {
    int it = sg.s.bag[slot];
    const SpurItemDef *d = &SPUR_ITEM[it];
    if (d->half > 0 && spur_doc_level() >= d->half_doc) sg.s.bag[slot] = (uint8_t)d->half;
    else spur_bag_remove(slot);
}

static const uint8_t FORAGE_TABLE[3][6] = {
    {IT_HERB, IT_HERB, IT_HERB, IT_MINT, IT_MINT, IT_CACTUSFIG},
    {IT_HERB, IT_HERB, IT_MINT, IT_SALVE, IT_MINT, IT_SNAKEROOT},
    {IT_HERB, IT_CACTUSFIG, IT_SALVE, IT_HERB, IT_HOLYOIL, IT_BLACKBREW},
};
static const uint8_t ORE_ODDS[3][8] = { /* iron silver gold topaz emerald ruby diamond hellite, in 100 */
    {40, 25, 15, 10, 6, 4, 0, 0}, {36, 24, 16, 10, 8, 6, 0, 0}, {26, 20, 16, 12, 10, 8, 6, 2},
};

static int spot_facing(int kind) {
    int fx = spur_facing_x(), fy = spur_facing_y();
    for (int i = 0; i < sg.map.nspot; i++)
        if (sg.map.spot[i].kind == kind && sg.map.spot[i].x == fx && sg.map.spot[i].y == fy) return i;
    /* a herb patch can be stood on */
    if (kind == SPOT_FORAGE)
        for (int i = 0; i < sg.map.nspot; i++)
            if (sg.map.spot[i].kind == kind && sg.map.spot[i].x == sg.pos[0].x && sg.map.spot[i].y == sg.pos[0].y) return i;
    return -1;
}

static char msgbuf[160];

/* a skill used out of a fight; false (with a message) when it can't be */
bool spur_use_skill_field(int who, int sk, int target) {
    const SpurSkillDef *d = &SPUR_SKILL[sk];
    SpurHero *hr = &sg.s.hero[who];
    if (!(d->use & SKU_FIELD)) { spur_say("THAT IS FOR A FIGHT."); return false; }
    if (hr->sp < d->sp) { spur_say("NOT ENOUGH SOUL."); return false; }
    if (hr->hp <= 0) return false;
    switch (sk) {
    case SK_MEND: case SK_SUTURE: {
        int t = spur_hero_at(target);
        if (t < 0) return false;
        if (sg.s.hero[t].hp <= 0) sg.s.hero[t].hp = 1; /* out of a fight it wakes the fallen */
        heal(t, spur_skill_heal(who, sk));
        snprintf(msgbuf, sizeof msgbuf, "%s IS PATCHED UP.", SPUR_HERO[t].name);
        break;
    }
    case SK_PURGE: {
        int t = spur_hero_at(target);
        if (t < 0) return false;
        sg.s.hero[t].status &= (uint8_t)~SS_POISON;
        snprintf(msgbuf, sizeof msgbuf, "THE POISON DRAINS OUT OF %s.", SPUR_HERO[t].name);
        break;
    }
    case SK_RAISE: {
        int t = spur_hero_at(target);
        if (t < 0 || sg.s.hero[t].hp > 0) { spur_say("THEY AREN'T DEAD."); return false; }
        sg.s.hero[t].hp = (int16_t)(spur_maxhp(t) / 2);
        snprintf(msgbuf, sizeof msgbuf, "%s DRAWS BREATH AGAIN.", SPUR_HERO[t].name);
        break;
    }
    case SK_REEL: case SK_MINUET: case SK_RENEW:
        for (int i = 0; i < 4; i++) {
            int t = spur_hero_at(i);
            if (spur_alive(t)) { heal(t, spur_skill_heal(who, sk)); if (sk == SK_RENEW) sg.s.hero[t].status &= (uint8_t)~SS_PARALYZED; }
        }
        snprintf(msgbuf, sizeof msgbuf, "THE POSSE FEELS BETTER.");
        break;
    case SK_BLENDIN:
        sg.ward_steps = 300;
        snprintf(msgbuf, sizeof msgbuf, "%s BLENDS THE POSSE INTO THE LAND.", SPUR_HERO[who].name);
        break;
    case SK_RUMMAGE: {
        int s = spot_facing(SPOT_FORAGE);
        if (s < 0) { spur_say("NOTHING HERE TO RUMMAGE IN."); return false; }
        int it = FORAGE_TABLE[sg.map.spot[s].table % 3][rng_range(&sg.rng, 0, 5)];
        if (spur_bag_add(it) < 0) { spur_say("THE BAGS ARE FULL."); return false; }
        snprintf(msgbuf, sizeof msgbuf, "%s TURNS UP %s.", SPUR_HERO[who].name, SPUR_ITEM[it].name);
        break;
    }
    case SK_PROSPECT: {
        int s = spot_facing(SPOT_ORE);
        if (s < 0) { spur_say("NO SEAM HERE WORTH DIGGING."); return false; }
        int roll = rng_range(&sg.rng, 0, 99), acc = 0, it = IT_IRONORE;
        const uint8_t *odds = ORE_ODDS[sg.map.spot[s].table % 3];
        for (int k = 0; k < 8; k++) { acc += odds[k]; if (roll < acc) { it = IT_IRONORE + k; break; } }
        if (spur_bag_add(it) < 0) { spur_say("THE BAGS ARE FULL."); return false; }
        snprintf(msgbuf, sizeof msgbuf, "%s DIGS OUT %s.", SPUR_HERO[who].name, SPUR_ITEM[it].name);
        break;
    }
    default: spur_say("NOTHING HAPPENS."); return false;
    }
    hr->sp = (int16_t)(hr->sp - d->sp);
    spur_sfx("spur_heal");
    spur_say(msgbuf);
    return true;
}

static bool needs_target(int it) {
    int u = SPUR_ITEM[it].use;
    return u == IU_HEAL || u == IU_SP || u == IU_SPALL || u == IU_REVIVE || u == IU_CURE || u == IU_TREAT_D ||
           u == IU_TREAT_A || u == IU_TREAT_E || u == IU_LESSON_STEADY || u == IU_LESSON_PROSPECT;
}

void spur_use_item_field(int slot, int who) {
    int it = sg.s.bag[slot];
    const SpurItemDef *d = &SPUR_ITEM[it];
    int h = who >= 0 ? spur_hero_at(who) : -1;
    if (d->kind != IK_USE) { spur_say("YOU CAN'T USE THAT HERE."); return; }
    if (!(d->flags & IF_FIELD)) { spur_say("THAT IS FOR A FIGHT."); return; }
    if (needs_target(it) && h < 0) return;
    switch (d->use) {
    case IU_HEAL:
        if (!spur_alive(h)) { spur_say("IT'S TOO LATE FOR THAT."); return; }
        heal(h, spur_heal_amount(it));
        snprintf(msgbuf, sizeof msgbuf, "%s USES %s.", SPUR_HERO[h].name, d->name);
        break;
    case IU_SP:
    case IU_SPALL:
        if (!spur_alive(h)) { spur_say("IT'S TOO LATE FOR THAT."); return; }
        sg.s.hero[h].sp = (int16_t)imin(spur_maxsp(h), sg.s.hero[h].sp + d->power);
        snprintf(msgbuf, sizeof msgbuf, "%s FEELS HIS SOUL STIR.", SPUR_HERO[h].name);
        if (h == H_SAGE || h == H_VERA || h == H_HATTIE || h == H_CORA)
            snprintf(msgbuf, sizeof msgbuf, "%s FEELS HER SOUL STIR.", SPUR_HERO[h].name);
        break;
    case IU_REVIVE:
        if (spur_alive(h)) { spur_say("THEY AREN'T DEAD."); return; }
        sg.s.hero[h].hp = (int16_t)(spur_doc_level() >= 4 ? spur_maxhp(h) / 2 : 1);
        snprintf(msgbuf, sizeof msgbuf, "%s COMES BACK, COUGHING.", SPUR_HERO[h].name);
        break;
    case IU_CURE:
        sg.s.hero[h].status &= (uint8_t)~SS_POISON;
        snprintf(msgbuf, sizeof msgbuf, "THE POISON IS GONE.");
        break;
    case IU_CAMP:
        for (int i = 0; i < 4; i++) {
            int t = spur_hero_at(i);
            if (!spur_alive(t)) continue;
            heal(t, spur_maxhp(t) / 2);
            sg.s.hero[t].sp = (int16_t)imin(spur_maxsp(t), sg.s.hero[t].sp + spur_maxsp(t) / 4);
        }
        spur_clear_wisps();
        snprintf(msgbuf, sizeof msgbuf, "THE POSSE MAKES CAMP AND RESTS A SPELL.");
        break;
    case IU_WARD:
        sg.ward_steps = d->power;
        snprintf(msgbuf, sizeof msgbuf, "THE SMOKE KEEPS THE WISPS AWAY A WHILE.");
        break;
    case IU_WARP:
        if (sg.map_id == M_SALOON || sg.map_id == M_SPUR3) { spur_say("NOT FROM HERE."); return; }
        spur_bag_remove(slot);
        sg.ui.kind = UI_NONE;
        spur_enter_map(sg.s.chapel ? sg.s.chapel : M_GLADWELL, -1, 0, 1);
        spur_sfx("spur_warp");
        spur_say("THE HALO PIN CARRIES YOU BACK TO THE CHAPEL.");
        return;
    case IU_TREAT_D: case IU_TREAT_A: case IU_TREAT_E:
        if (h != H_BISCUIT) { spur_say("ONLY A DOG WOULD EAT THAT."); return; }
        sg.s.hero[h].treat[ST_HP] += 20;
        sg.s.hero[h].treat[d->use == IU_TREAT_D ? ST_DEF : d->use == IU_TREAT_A ? ST_ATK : ST_EVD] += 4;
        snprintf(msgbuf, sizeof msgbuf, "BISCUIT CRUNCHES IT UP. HE LOOKS BIGGER.");
        break;
    case IU_LESSON_STEADY:
        if (h == H_QUILL || h == H_SAGE || h == H_BISCUIT) { snprintf(msgbuf, sizeof msgbuf, "%s CAN'T MAKE HEAD OR TAIL OF IT.", SPUR_HERO[h].name); spur_say(msgbuf); return; }
        if (spur_has_skill(h, SK_STEADY)) { spur_say("ALREADY KNOWN."); return; }
        spur_learn_skill(h, SK_STEADY);
        snprintf(msgbuf, sizeof msgbuf, "%s LEARNS STEADY.", SPUR_HERO[h].name);
        break;
    case IU_LESSON_PROSPECT:
        if (h == H_SAGE || h == H_BISCUIT) { snprintf(msgbuf, sizeof msgbuf, "%s WON'T HAVE ANY OF IT.", SPUR_HERO[h].name); spur_say(msgbuf); return; }
        if (spur_has_skill(h, SK_PROSPECT)) { spur_say("ALREADY KNOWN."); return; }
        spur_learn_skill(h, SK_PROSPECT);
        snprintf(msgbuf, sizeof msgbuf, "%s LEARNS PROSPECT.", SPUR_HERO[h].name);
        break;
    default: spur_say("NOTHING HAPPENS."); return;
    }
    consume(slot);
    spur_sfx("spur_heal");
    spur_say(msgbuf);
}

/* ---- equipping ----------------------------------------------------------------------- */
static void equip(int h, int slot, int bag_slot) {
    SpurHero *hr = &sg.s.hero[h];
    int old = hr->equip[slot];
    int it = bag_slot >= 0 ? sg.s.bag[bag_slot] : 0;
    if (bag_slot >= 0) spur_bag_remove(bag_slot);
    hr->equip[slot] = (uint8_t)it;
    if (old) spur_bag_add(old);
    /* a two-hander, or a lost second skill, empties the left hand */
    if (slot == EQ_RHAND && hr->equip[EQ_LHAND] && (!hr->equip[EQ_RHAND] || !spur_can_equip(h, hr->equip[EQ_LHAND], EQ_LHAND))) {
        if (spur_bag_add(hr->equip[EQ_LHAND]) >= 0) hr->equip[EQ_LHAND] = 0;
    }
    hr->hp = (int16_t)imin(hr->hp, spur_maxhp(h));
    hr->sp = (int16_t)imin(hr->sp, spur_maxsp(h));
}

/* ---- the B menu ---------------------------------------------------------------------- */
static const char *const ROOT[] = {"ITEMS", "SKILLS", "EQUIP", "STATS", "ORDER", "KEYS", "MOUNT"};
static int root_n(void) {
    bool mount = sg.map.kind == MK_WORLD && (sg.s.horses || spur_key(KI_NIGHTMARES));
    return mount ? 7 : 6;
}

void spur_open_field_menu(void) {
    memset(&sg.ui, 0, sizeof sg.ui);
    sg.ui.kind = UI_MENU;
    sg.menu_page = FM_ROOT;
    sg.menu_sel = 0;
    spur_sfx("spur_menu");
    input_consume();
}

static int nposse(void) { return imax(1, sg.s.nposse); }

static int menu_skills[SK_COUNT];
static int menu_nskills;

static void menu_back(int page, int sel) {
    sg.menu_page = page;
    sg.menu_sel = sel;
    spur_sfx("spur_back");
}

static void mount_toggle(void) {
    if (sg.mounted) {
        int t = spur_tile_at(&sg.map, sg.pos[0].x, sg.pos[0].y);
        if (sg.mounted == 2 && !(t >= 0 && (SPUR_TILE_FLAGS[t] & TF_LAND) && spur_tile_walk(t))) {
            spur_say("THERE'S NOWHERE TO LAND HERE.");
            return;
        }
        sg.ui.kind = UI_NONE;
        spur_set_mount(0);
        spur_sfx("spur_dismount");
        return;
    }
    sg.ui.kind = UI_NONE;
    if (spur_key(KI_NIGHTMARES)) {
        spur_set_mount(2);
        spur_sfx("spur_wings");
        spur_toast("THE NIGHTMARES TAKE WING");
    } else {
        spur_set_mount(1);
        spur_sfx("spur_mount");
        spur_toast("THE POSSE RIDES");
    }
}

static void field_menu_update(void) {
    int n;
    bool up = btn_repeat(BTN_UP), down = btn_repeat(BTN_DOWN);
    bool left = btnp(BTN_LEFT), right = btnp(BTN_RIGHT);
    bool ok = btnp(SPUR_OK), back = btnp(SPUR_BACK);
    switch (sg.menu_page) {
    case FM_ROOT:
        n = root_n();
        if (up) { sg.menu_sel = (sg.menu_sel + n - 1) % n; spur_sfx("spur_tick"); }
        if (down) { sg.menu_sel = (sg.menu_sel + 1) % n; spur_sfx("spur_tick"); }
        if (back) { sg.ui.kind = UI_NONE; input_consume(); return; }
        if (!ok) return;
        spur_sfx("spur_ok");
        switch (sg.menu_sel) {
        case 0: sg.menu_page = FM_ITEM; sg.menu_slot = 0; break;
        case 1: sg.menu_page = FM_SKILL_WHO; sg.menu_who = 0; break;
        case 2: sg.menu_page = FM_EQUIP_WHO; sg.menu_who = 0; break;
        case 3: sg.menu_page = FM_STATS_WHO; sg.menu_who = 0; break;
        case 4: sg.menu_page = FM_ORDER; sg.menu_who = 0; break;
        case 5: sg.menu_page = FM_KEYS; sg.menu_sel2 = 0; break;
        case 6: mount_toggle(); break;
        }
        break;
    case FM_ITEM:
        if (up) { sg.menu_slot = (sg.menu_slot + SPUR_BAG - 1) % SPUR_BAG; spur_sfx("spur_tick"); }
        if (down) { sg.menu_slot = (sg.menu_slot + 1) % SPUR_BAG; spur_sfx("spur_tick"); }
        if (left || right) { sg.menu_slot = (sg.menu_slot + 10) % SPUR_BAG; spur_sfx("spur_tick"); }
        if (back) { menu_back(FM_ROOT, 0); return; }
        if (ok && sg.s.bag[sg.menu_slot]) { sg.menu_page = FM_ITEM_DO; sg.menu_sel2 = 0; spur_sfx("spur_ok"); }
        break;
    case FM_ITEM_DO: /* USE / DROP */
        if (up || down) { sg.menu_sel2 ^= 1; spur_sfx("spur_tick"); }
        if (back) { menu_back(FM_ITEM, 0); return; }
        if (!ok) return;
        if (sg.menu_sel2 == 1) {
            int it = sg.s.bag[sg.menu_slot];
            if (it == IT_DOOMSDAY || it == IT_HELLITE) { spur_say("BETTER HANG ON TO THAT."); return; }
            spur_bag_remove(sg.menu_slot);
            spur_sfx("spur_drop");
            sg.menu_page = FM_ITEM;
            return;
        }
        if (needs_target(sg.s.bag[sg.menu_slot])) { sg.menu_page = FM_ITEM_WHO; sg.menu_who = 0; spur_sfx("spur_ok"); }
        else { sg.menu_page = FM_ITEM; spur_use_item_field(sg.menu_slot, -1); }
        break;
    case FM_ITEM_WHO:
        n = nposse();
        if (up) { sg.menu_who = (sg.menu_who + n - 1) % n; spur_sfx("spur_tick"); }
        if (down) { sg.menu_who = (sg.menu_who + 1) % n; spur_sfx("spur_tick"); }
        if (back) { menu_back(FM_ITEM_DO, 0); return; }
        if (ok) { sg.menu_page = FM_ITEM; spur_use_item_field(sg.menu_slot, sg.menu_who); }
        break;
    case FM_SKILL_WHO: case FM_EQUIP_WHO: case FM_STATS_WHO:
        n = nposse();
        if (up) { sg.menu_who = (sg.menu_who + n - 1) % n; spur_sfx("spur_tick"); }
        if (down) { sg.menu_who = (sg.menu_who + 1) % n; spur_sfx("spur_tick"); }
        if (back) { menu_back(FM_ROOT, sg.menu_page == FM_SKILL_WHO ? 1 : sg.menu_page == FM_EQUIP_WHO ? 2 : 3); return; }
        if (!ok) return;
        spur_sfx("spur_ok");
        if (sg.menu_page == FM_SKILL_WHO) {
            int h = spur_hero_at(sg.menu_who);
            menu_nskills = spur_skill_list(h, menu_skills);
            if (!menu_nskills) { spur_say("NO SKILLS TO SPEAK OF."); return; }
            sg.menu_page = FM_SKILL;
            sg.menu_skill = 0;
        } else if (sg.menu_page == FM_EQUIP_WHO) { sg.menu_page = FM_EQUIP; sg.menu_slot = 0; }
        else { sg.menu_page = FM_STATS; sg.menu_sel2 = 0; }
        break;
    case FM_SKILL: {
        int h = spur_hero_at(sg.menu_who);
        menu_nskills = spur_skill_list(h, menu_skills);
        n = imax(1, menu_nskills);
        if (up) { sg.menu_skill = (sg.menu_skill + n - 1) % n; spur_sfx("spur_tick"); }
        if (down) { sg.menu_skill = (sg.menu_skill + 1) % n; spur_sfx("spur_tick"); }
        if (back) { menu_back(FM_SKILL_WHO, 0); return; }
        if (!ok) return;
        int sk = menu_skills[sg.menu_skill];
        const SpurSkillDef *d = &SPUR_SKILL[sk];
        if (!(d->use & SKU_FIELD)) { spur_sfx("spur_no"); return; }
        if (sg.s.hero[h].sp < d->sp || !spur_alive(h)) { spur_sfx("spur_no"); return; }
        if (d->target == SKT_ALLY || d->target == SKT_DEADALLY) { sg.menu_page = FM_SKILL_TGT; sg.menu_sel2 = 0; spur_sfx("spur_ok"); }
        else spur_use_skill_field(h, sk, -1);
        break;
    }
    case FM_SKILL_TGT:
        n = nposse();
        if (up) { sg.menu_sel2 = (sg.menu_sel2 + n - 1) % n; spur_sfx("spur_tick"); }
        if (down) { sg.menu_sel2 = (sg.menu_sel2 + 1) % n; spur_sfx("spur_tick"); }
        if (back) { menu_back(FM_SKILL, 0); sg.menu_page = FM_SKILL; return; }
        if (ok) {
            sg.menu_page = FM_SKILL;
            spur_use_skill_field(spur_hero_at(sg.menu_who), menu_skills[sg.menu_skill], sg.menu_sel2);
        }
        break;
    case FM_EQUIP:
        if (up) { sg.menu_slot = (sg.menu_slot + EQ_COUNT - 1) % EQ_COUNT; spur_sfx("spur_tick"); }
        if (down) { sg.menu_slot = (sg.menu_slot + 1) % EQ_COUNT; spur_sfx("spur_tick"); }
        if (back) { menu_back(FM_EQUIP_WHO, 0); sg.menu_page = FM_EQUIP_WHO; return; }
        if (ok) { sg.menu_page = FM_EQUIP_PICK; sg.menu_sel2 = 0; spur_sfx("spur_ok"); }
        break;
    case FM_EQUIP_PICK: {
        /* the choices: TAKE OFF, then what in the bag fits the slot */
        int h = spur_hero_at(sg.menu_who);
        int opts[SPUR_BAG + 1], no = 0;
        opts[no++] = -1;
        for (int i = 0; i < SPUR_BAG; i++) if (sg.s.bag[i] && spur_can_equip(h, sg.s.bag[i], sg.menu_slot)) opts[no++] = i;
        if (up) { sg.menu_sel2 = (sg.menu_sel2 + no - 1) % no; spur_sfx("spur_tick"); }
        if (down) { sg.menu_sel2 = (sg.menu_sel2 + 1) % no; spur_sfx("spur_tick"); }
        sg.menu_sel2 = iclamp(sg.menu_sel2, 0, no - 1);
        if (back) { sg.menu_page = FM_EQUIP; spur_sfx("spur_back"); return; }
        if (!ok) return;
        int pick = opts[sg.menu_sel2];
        if (pick < 0 && sg.s.hero[h].equip[sg.menu_slot] && spur_bag_count() >= SPUR_BAG) { spur_say("NO ROOM IN THE BAGS."); return; }
        equip(h, sg.menu_slot, pick);
        spur_sfx("spur_equip");
        sg.menu_page = FM_EQUIP;
        break;
    }
    case FM_STATS: {
        int h = spur_hero_at(sg.menu_who);
        int np = 0;
        for (int p = 0; p < PS_COUNT; p++) np += spur_passive(h, p) > 0;
        if (up && np) sg.menu_sel2 = (sg.menu_sel2 + np - 1) % np;
        if (down && np) sg.menu_sel2 = (sg.menu_sel2 + 1) % np;
        if (left || right) { int n2 = nposse(); sg.menu_who = (sg.menu_who + (right ? 1 : n2 - 1)) % n2; sg.menu_sel2 = 0; }
        if (back || ok) { menu_back(FM_STATS_WHO, 0); sg.menu_page = FM_STATS_WHO; }
        break;
    }
    case FM_ORDER: case FM_ORDER2:
        n = nposse();
        if (up) { sg.menu_who = (sg.menu_who + n - 1) % n; spur_sfx("spur_tick"); }
        if (down) { sg.menu_who = (sg.menu_who + 1) % n; spur_sfx("spur_tick"); }
        if (back) {
            if (sg.menu_page == FM_ORDER2) { sg.menu_page = FM_ORDER; spur_sfx("spur_back"); }
            else menu_back(FM_ROOT, 4);
            return;
        }
        if (!ok) return;
        if (sg.menu_page == FM_ORDER) { sg.menu_slot = sg.menu_who; sg.menu_page = FM_ORDER2; spur_sfx("spur_ok"); }
        else {
            uint8_t t = sg.s.order[sg.menu_slot];
            sg.s.order[sg.menu_slot] = sg.s.order[sg.menu_who];
            sg.s.order[sg.menu_who] = t;
            sg.menu_page = FM_ORDER;
            spur_sfx("spur_equip");
        }
        break;
    case FM_KEYS: {
        int nk = 0;
        for (int k = 0; k < KI_COUNT; k++) nk += spur_key(k);
        if (up && nk) sg.menu_sel2 = (sg.menu_sel2 + nk - 1) % nk;
        if (down && nk) sg.menu_sel2 = (sg.menu_sel2 + 1) % nk;
        if (back || ok) menu_back(FM_ROOT, 5);
        break;
    }
    default: break;
    }
}

/* ---- the counters ---------------------------------------------------------------------- */
enum { PK_SHOP_ROOT = 1, PK_CHAPEL, PK_BANK, PK_TRAIN, PK_SALOON, PK_BOARD };
enum { NUM_DEPOSIT = 1, NUM_WITHDRAW };

static int town_index(void) { return sg.map_id >= SPUR_FIRST_TOWN && sg.map_id < SPUR_FIRST_TOWN + SPUR_TOWNS ? sg.map_id - SPUR_FIRST_TOWN : -1; }

static void open_shop(int shop) {
    if (shop <= 0 || shop >= SPUR_SHOPS) { spur_say("CLOSED."); return; }
    memset(&sg.ui, 0, sizeof sg.ui);
    sg.ui.kind = UI_SHOP;
    sg.ui.shop = shop;
    sg.ui.mode = 0;
    sg.ui.sel = 0;
    music_play(SPUR_MUS_SHOP);
    input_consume();
}

static int shop_count(int shop) {
    int n = 0;
    while (n < SPUR_SHOP_ITEMS && SPUR_SHOP[shop].item[n]) n++;
    return n;
}

static char shop_note[48];

static void shop_update(void) {
    SpurUi *u = &sg.ui;
    bool up = btn_repeat(BTN_UP), down = btn_repeat(BTN_DOWN);
    bool ok = btnp(SPUR_OK), back = btnp(SPUR_BACK);
    if (u->mode == 0) {
        if (up) { u->sel = (u->sel + 2) % 3; spur_sfx("spur_tick"); }
        if (down) { u->sel = (u->sel + 1) % 3; spur_sfx("spur_tick"); }
        if (back || (ok && u->sel == 2)) { u->kind = UI_NONE; input_consume(); spur_field_after_battle(true); return; }
        if (ok) { u->mode = u->sel + 1; u->sel2 = 0; shop_note[0] = 0; spur_sfx("spur_ok"); }
        return;
    }
    if (u->mode == 1) { /* buying */
        int n = shop_count(u->shop);
        if (up) { u->sel2 = (u->sel2 + n - 1) % n; spur_sfx("spur_tick"); }
        if (down) { u->sel2 = (u->sel2 + 1) % n; spur_sfx("spur_tick"); }
        if (back) { u->mode = 0; spur_sfx("spur_back"); return; }
        if (!ok) return;
        int it = SPUR_SHOP[u->shop].item[u->sel2];
        int price = SPUR_ITEM[it].price;
        if (sg.s.teeth < price) { snprintf(shop_note, sizeof shop_note, "NOT ENOUGH TEETH."); spur_sfx("spur_no"); return; }
        if (spur_bag_add(it) < 0) { snprintf(shop_note, sizeof shop_note, "NO ROOM IN THE BAGS."); spur_sfx("spur_no"); return; }
        sg.s.teeth -= price;
        snprintf(shop_note, sizeof shop_note, "BOUGHT %s.", SPUR_ITEM[it].name);
        spur_sfx("spur_buy");
        return;
    }
    /* selling */
    int n = 0;
    for (int i = 0; i < SPUR_BAG; i++) n += sg.s.bag[i] != 0;
    if (!n) { if (back || ok) { u->mode = 0; spur_sfx("spur_back"); } return; }
    if (up) { u->sel2 = (u->sel2 + n - 1) % n; spur_sfx("spur_tick"); }
    if (down) { u->sel2 = (u->sel2 + 1) % n; spur_sfx("spur_tick"); }
    u->sel2 = iclamp(u->sel2, 0, n - 1);
    if (back) { u->mode = 0; spur_sfx("spur_back"); return; }
    if (!ok) return;
    int it = sg.s.bag[u->sel2];
    int pr = spur_sell_price(it);
    if (pr <= 0) { snprintf(shop_note, sizeof shop_note, "NOBODY WANTS THAT."); spur_sfx("spur_no"); return; }
    spur_bag_remove(u->sel2);
    sg.s.teeth = imin(sg.s.teeth + pr, 9999999);
    snprintf(shop_note, sizeof shop_note, "SOLD %s FOR %d.", SPUR_ITEM[it].name, pr);
    spur_sfx("spur_buy");
}

static int train_dest[5];
static int train_n;

static bool station_open(int map) {
    if (!(sg.s.stations & (1u << (SPUR_MAPINFO[map].station - 1)))) return false;
    switch (map) {
    case M_HOPE: return spur_flag(F_HOPE_FREE);
    case M_STEEPLE: return spur_flag(F_JACK);
    case M_LASTCHANCE: return spur_flag(F_WORM);
    default: return spur_flag(F_HOPE_FREE);
    }
}

static int fare(int from, int to) { return imin(120, 40 * iabs(SPUR_MAPINFO[from].station - SPUR_MAPINFO[to].station)); }

static void open_train(void) {
    static const int LINE[5] = {M_GLADWELL, M_MERIDIAN, M_HOPE, M_STEEPLE, M_LASTCHANCE};
    if (sg.s.horses) { spur_say("NO HORSES ON THE TRAIN, FRIEND. TAKE THEM BACK TO THE STABLE FIRST."); return; }
    if (!station_open(sg.map_id)) { spur_say("NO TRAINS RUN FROM HERE YET."); return; }
    train_n = 0;
    static char opt[5][40];
    const char *opts[5];
    for (int i = 0; i < 5; i++) {
        if (LINE[i] == sg.map_id || !station_open(LINE[i])) continue;
        snprintf(opt[train_n], sizeof opt[train_n], "%s  %d", SPUR_MAPINFO[LINE[i]].name, fare(sg.map_id, LINE[i]));
        opts[train_n] = opt[train_n];
        train_dest[train_n++] = LINE[i];
    }
    if (!train_n) { spur_say("THE LINE IS CUT EVERY WHICH WAY. NO TRAINS TODAY."); return; }
    spur_pick("WHERE TO? 40 TEETH A STOP", opts, train_n, PK_TRAIN);
}

static int chapel_who[4];
static int chapel_n;

static void open_chapel(void) {
    sg.s.chapel = (uint8_t)sg.map_id;
    chapel_n = 0;
    static char opt[4][40];
    const char *opts[4];
    for (int i = 0; i < 4; i++) {
        int h = spur_hero_at(i);
        if (h < 0 || spur_alive(h)) continue;
        snprintf(opt[chapel_n], sizeof opt[chapel_n], "%s  %d", SPUR_HERO[h].name, 20 * sg.s.hero[h].level);
        opts[chapel_n] = opt[chapel_n];
        chapel_who[chapel_n++] = h;
    }
    if (!chapel_n) { spur_say("THE PREACHER LOOKS UP.\n\"NOBODY HERE NEEDS RAISING. GO WITH GOD, WHATEVER'S LEFT OF HIM.\""); return; }
    spur_pick("RAISE THE FALLEN: 20 TEETH A LEVEL", opts, chapel_n, PK_CHAPEL);
}

static void hotel(void) {
    int ti = town_index();
    int price = ti >= 0 ? SPUR_TOWN[ti].hotel : 0;
    if (sg.map_id == M_WIDOW) price = 0;
    char q[96];
    if (price > 0) snprintf(q, sizeof q, "A NIGHT FOR THE POSSE IS %d TEETH. STAY?", price);
    else snprintf(q, sizeof q, "THE BEDS ARE FREE. REST A WHILE?");
    sg.ui.num = price;
    spur_ask(q, 9001, 0);
}

void spur_hotel_rest(int price) {
    if (sg.s.teeth < price) { spur_say("YOU CAN'T AFFORD IT."); return; }
    sg.s.teeth -= price;
    for (int i = 0; i < 4; i++) {
        int h = spur_hero_at(i);
        if (!spur_alive(h)) continue;
        sg.s.hero[h].hp = (int16_t)spur_maxhp(h);
        sg.s.hero[h].sp = (int16_t)spur_maxsp(h);
        sg.s.hero[h].status = 0;
    }
    spur_save_game();
    music_play(SPUR_MUS_LEVEL);
    spur_say_then("THE POSSE SLEEPS LIKE THE DEAD.\nTHE GAME IS SAVED.", 9002);
}

static int hotel_price;

void spur_open_door(int service, int arg) {
    if (spur_story_door(service, arg)) return;
    int ti = town_index();
    const SpurTownDef *t = ti >= 0 ? &SPUR_TOWN[ti] : NULL;
    switch (service) {
    case SV_HOTEL:
        if (!t || t->hotel < 0) { spur_say("NO ROOMS."); return; }
        hotel_price = t->hotel;
        hotel();
        break;
    case SV_CHAPEL:
        if (!t || !t->chapel) { spur_say("THE CHAPEL DOORS ARE NAILED SHUT."); return; }
        open_chapel();
        break;
    case SV_SHOP_A: open_shop(t ? t->shop_a : 0); break;
    case SV_SHOP_B: open_shop(t ? t->shop_b : 0); break;
    case SV_BANK: {
        static const char *const B[] = {"DEPOSIT", "WITHDRAW", "LEAVE"};
        if (!t || !t->bank) { spur_say("CLOSED."); return; }
        spur_pick("THE BANK OF CALICO", B, 3, PK_BANK);
        break;
    }
    case SV_SALOON: {
        static const char *const S[] = {"GINGER POP  120", "A WORD WITH THE BARKEEP", "LEAVE"};
        if (!t || !t->saloon) { spur_say("THE SALOON IS SHUT."); return; }
        spur_pick("THE SALOON", S, 3, PK_SALOON);
        break;
    }
    case SV_TRAIN: open_train(); break;
    case SV_STABLES: {
        if (!t || !t->stables) { spur_say("NO HORSES HERE."); return; }
        char q[120];
        snprintf(q, sizeof q, "HORSES FOR THE WHOLE POSSE, %d TEETH, TILL YOU NEXT REACH A TOWN. WISPS CAN'T KEEP UP WITH A HORSE. RENT THEM?", t->stable_price);
        sg.ui.num = t->stable_price;
        hotel_price = t->stable_price;
        spur_ask(q, 9003, 0);
        break;
    }
    case SV_SHERIFF: spur_say("THE SHERIFF'S OUT."); break;
    case SV_LOCKED: spur_say("LOCKED."); break;
    default: spur_say("NOBODY ANSWERS."); break;
    }
}

/* events this file owns (9000s) */
bool spur_menu_event(int ev) {
    switch (ev) {
    case 9001: spur_hotel_rest(hotel_price); return true;
    case 9002: spur_field_after_battle(true); return true;
    case 9003:
        if (sg.s.teeth < hotel_price) { spur_say("YOU CAN'T AFFORD THEM."); return true; }
        sg.s.teeth -= hotel_price;
        sg.s.horses = 1;
        spur_sfx("spur_buy");
        spur_say("THE HORSES WAIT OUTSIDE TOWN. MOUNT UP FROM THE MENU OUT ON THE TRAIL.");
        return true;
    default: return false;
    }
}

static void pick_done(int mode, int c) {
    switch (mode) {
    case PK_TRAIN: {
        int to = train_dest[c];
        int f = fare(sg.map_id, to);
        if (sg.s.teeth < f) { spur_say("YOU CAN'T AFFORD THE FARE."); return; }
        sg.s.teeth -= f;
        spur_sfx("spur_train");
        spur_enter_map(to, -1, 0, 1);
        spur_say("THE TRAIN RATTLES THROUGH HELL'S OWN PRAIRIE, AND LETS YOU OFF.");
        return;
    }
    case PK_CHAPEL: {
        int h = chapel_who[c];
        int cost = 20 * sg.s.hero[h].level;
        if (sg.s.teeth < cost) { spur_say("YOU CAN'T AFFORD IT."); return; }
        sg.s.teeth -= cost;
        sg.s.hero[h].hp = (int16_t)spur_maxhp(h);
        sg.s.hero[h].status = 0;
        spur_sfx("spur_raise");
        snprintf(msgbuf, sizeof msgbuf, "%s RISES, BLINKING.", SPUR_HERO[h].name);
        spur_say(msgbuf);
        return;
    }
    case PK_BANK:
        if (c == 0) {
            spur_number("DEPOSIT HOW MUCH?", imin(sg.s.teeth, 100000 - imin(sg.s.bank, 100000)), NUM_DEPOSIT);
            if (sg.s.bank >= 100000) { sg.ui.kind = UI_NONE; spur_say("YOUR ACCOUNT IS FULL. THE BANK HOLDS NO MORE THAN 100,000."); }
        } else if (c == 1) spur_number("WITHDRAW HOW MUCH?", sg.s.bank, NUM_WITHDRAW);
        else spur_field_after_battle(true);
        return;
    case PK_SALOON:
        if (c == 0) {
            if (sg.s.teeth < 120) { spur_say("YOU CAN'T AFFORD IT."); return; }
            if (spur_bag_add(IT_GINGERPOP) < 0) { spur_say("NO ROOM IN THE BAGS."); return; }
            sg.s.teeth -= 120;
            spur_sfx("spur_buy");
            spur_say("ONE GINGER POP. IT FIZZES THE SOUL.");
        } else if (c == 1) spur_story_pick(100 + sg.map_id, 0);
        return;
    default: spur_story_pick(mode, c); return;
    }
}

static void number_done(int mode, int v) {
    if (mode == NUM_DEPOSIT) {
        v = imin(v, sg.s.teeth);
        sg.s.teeth -= v;
        sg.s.bank += v;
        snprintf(msgbuf, sizeof msgbuf, "DEPOSITED %d. THE BALANCE IS %d. IT GROWS TWO IN A HUNDRED AFTER EVERY FIGHT.", v, sg.s.bank);
        spur_say(msgbuf);
    } else if (mode == NUM_WITHDRAW) {
        v = imin(v, sg.s.bank);
        sg.s.bank -= v;
        sg.s.teeth += v;
        snprintf(msgbuf, sizeof msgbuf, "WITHDRAWN %d. THE BALANCE IS %d.", v, sg.s.bank);
        spur_say(msgbuf);
    } else spur_story_number(mode, v);
}

/* ---- one frame of whatever is open ---------------------------------------------------- */
void spur_ui_update(void) {
    SpurUi *u = &sg.ui;
    u->t++;
    switch (u->kind) {
    case UI_TEXT:
    case UI_YESNO: {
        bool last = u->page >= pages() - 1;
        if (u->kind == UI_YESNO && last) {
            if (btnp(BTN_LEFT) || btnp(BTN_RIGHT) || btnp(BTN_UP) || btnp(BTN_DOWN)) { u->sel ^= 1; spur_sfx("spur_tick"); }
            if (u->t < 8) return;
            if (btnp(SPUR_OK) || btnp(SPUR_BACK)) {
                int ev = (btnp(SPUR_OK) && u->sel == 0) ? u->yes_event : u->no_event;
                u->kind = UI_NONE;
                spur_sfx(ev == u->yes_event ? "spur_ok" : "spur_back");
                input_consume();
                if (ev > 0) spur_event(ev);
            }
            return;
        }
        if (u->t < 8) return;
        if (btnp(SPUR_OK) || btnp(SPUR_BACK)) {
            input_consume();
            if (!last) { u->page++; u->t = 0; spur_sfx("spur_tick"); }
            else close_ui();
        }
        return;
    }
    case UI_MENU: field_menu_update(); return;
    case UI_SHOP: shop_update(); return;
    case UI_PICK:
        if (btn_repeat(BTN_UP)) { u->sel = (u->sel + pick_n - 1) % pick_n; spur_sfx("spur_tick"); }
        if (btn_repeat(BTN_DOWN)) { u->sel = (u->sel + 1) % pick_n; spur_sfx("spur_tick"); }
        if (btnp(SPUR_BACK)) { u->kind = UI_NONE; input_consume(); spur_sfx("spur_back"); spur_story_pick(u->mode, -1); return; }
        if (btnp(SPUR_OK) && u->t > 4) {
            int c = u->sel, m = u->mode;
            u->kind = UI_NONE;
            input_consume();
            spur_sfx("spur_ok");
            pick_done(m, c);
        }
        return;
    case UI_NUMBER: {
        static const int POW[7] = {1, 10, 100, 1000, 10000, 100000, 1000000};
        if (btnp(BTN_LEFT)) u->sel = imin(u->sel + 1, 6);
        if (btnp(BTN_RIGHT)) u->sel = imax(u->sel - 1, 0);
        if (btn_repeat(BTN_UP)) u->num = imin(u->num + POW[u->sel], u->num_max);
        if (btn_repeat(BTN_DOWN)) u->num = imax(u->num - POW[u->sel], 0);
        if (btnp(SPUR_BACK)) { u->kind = UI_NONE; input_consume(); spur_sfx("spur_back"); return; }
        if (btnp(SPUR_OK) && u->t > 4) {
            int v = u->num, m = u->mode;
            u->kind = UI_NONE;
            input_consume();
            number_done(m, v);
        }
        return;
    }
    default: u->kind = UI_NONE; return;
    }
}

/* ---- drawing ---------------------------------------------------------------------------- */
static void panel(int x, int y, int w, int h) {
    gfx_rect(x, y, w, h, C_INK);
    gfx_rectb(x, y, w, h, C_CREAM);
    gfx_rectb(x + 1, y + 1, w - 2, h - 2, C_BROWN);
}

static void cursor(int x, int y) {
    if ((sg.ani / 8) % 4 != 3) text_draw(GLYPH_RIGHT, x, y, C_YELLOW);
}

static void bar(int x, int y, int w, int v, int max, int col) {
    gfx_rect(x, y, w, 2, C_NIGHT);
    if (max > 0) gfx_rect(x, y, w * imax(0, v) / max, 2, col);
}

/* the posse at a glance: names, levels, health and soul */
void spur_draw_posse_panel(int x, int y, int w) {
    for (int i = 0; i < nposse(); i++) {
        int h = spur_hero_at(i);
        if (h < 0) continue;
        int yy = y + i * 24;
        const SpurHero *hr = &sg.s.hero[h];
        bool dead = hr->hp <= 0;
        spur_draw_hero(h, 0, 0, x, yy + 2, dead);
        char b[48];
        text_draw(SPUR_HERO[h].name, x + 20, yy + 1, dead ? C_GREY : C_WHITE);
        snprintf(b, sizeof b, "LV %d", hr->level);
        tiny_draw(b, x + 22 + text_width(SPUR_HERO[h].name) + 2, yy + 2, C_TAN);
        if (hr->status & SS_POISON) tiny_draw("POISON", x + w - 30, yy + 2, C_LIME);
        if (hr->status & SS_PARALYZED) tiny_draw("STIFF", x + w - 30, yy + 2, C_SKY);
        if (dead) tiny_draw("DEAD", x + w - 30, yy + 2, C_RED);
        snprintf(b, sizeof b, "%d/%d", hr->hp, spur_maxhp(h));
        tiny_draw("HP", x + 20, yy + 11, C_RED);
        tiny_draw(b, x + 30, yy + 11, C_WHITE);
        bar(x + 20, yy + 18, 60, hr->hp, spur_maxhp(h), C_RED);
        snprintf(b, sizeof b, "%d/%d", hr->sp, spur_maxsp(h));
        tiny_draw("SP", x + 88, yy + 11, C_SKY);
        tiny_draw(b, x + 98, yy + 11, C_WHITE);
        bar(x + 88, yy + 18, 60, hr->sp, spur_maxsp(h), C_SKY);
    }
}

static void teeth_box(void) {
    char b[48];
    snprintf(b, sizeof b, "%d TEETH", sg.s.teeth);
    int w = text_width(b) + 12;
    panel(SCREEN_W - 8 - w, 158, w, 16);
    text_draw(b, SCREEN_W - 2 - w, 162, C_CREAM);
}

static void draw_text_box(void) {
    SpurUi *u = &sg.ui;
    panel(BOX_X, BOX_Y, BOX_W, BOX_H);
    int first = u->page * PAGE_LINES;
    for (int i = 0; i < PAGE_LINES && first + i < u->nlines; i++) text_draw(u->lines[first + i], BOX_X + 8, BOX_Y + 7 + i * 10, C_WHITE);
    bool last = u->page >= pages() - 1;
    if (u->kind == UI_YESNO && last) {
        panel(BOX_X + BOX_W - 62, BOX_Y - 28, 62, 26);
        text_draw("YES", BOX_X + BOX_W - 48, BOX_Y - 23, u->sel == 0 ? C_YELLOW : C_GREY);
        text_draw("NO", BOX_X + BOX_W - 48, BOX_Y - 13, u->sel == 1 ? C_YELLOW : C_GREY);
        cursor(BOX_X + BOX_W - 58, BOX_Y - 23 + u->sel * 10);
    } else if (u->t > 8 && (sg.ani / 10) % 2) text_draw(last ? GLYPH_A : GLYPH_DOWN, BOX_X + BOX_W - 12, BOX_Y + BOX_H - 11, C_YELLOW);
}

static void list_box(const char *title, const char *const *items, int n, int sel, int x, int y, int w) {
    int h = 18 + n * 10;
    panel(x, y, w, h);
    text_draw(title, x + 8, y + 5, C_TAN);
    for (int i = 0; i < n; i++) text_draw(items[i], x + 16, y + 16 + i * 10, i == sel ? C_YELLOW : C_WHITE);
    cursor(x + 6, y + 16 + sel * 10);
}

static void who_list(int sel, int x, int y) {
    panel(x, y, 196, 8 + nposse() * 24);
    spur_draw_posse_panel(x + 14, y + 4, 176);
    cursor(x + 3, y + 8 + sel * 24);
}

static void draw_field_menu(void) {
    int n = root_n();
    const char *items[7];
    for (int i = 0; i < n; i++) items[i] = ROOT[i];
    if (n == 7 && sg.mounted) items[6] = sg.mounted == 2 ? "LAND" : "DISMOUNT";
    list_box("POSSE", items, n, sg.menu_page == FM_ROOT ? sg.menu_sel : -1, 8, 8, 84);
    teeth_box();
    char b[96];
    switch (sg.menu_page) {
    case FM_ROOT:
    case FM_SKILL_WHO: case FM_EQUIP_WHO: case FM_STATS_WHO: case FM_ORDER: case FM_ORDER2: case FM_ITEM_WHO:
        if (sg.menu_page == FM_ROOT) {
            panel(100, 8, 212, 8 + nposse() * 24);
            spur_draw_posse_panel(110, 12, 196);
        } else {
            who_list(sg.menu_page == FM_ORDER2 ? sg.menu_who : sg.menu_who, 100, 8);
            if (sg.menu_page == FM_ORDER2) text_draw(GLYPH_STAR, 104, 12 + sg.menu_slot * 24, C_ORANGE);
            const char *hint = sg.menu_page == FM_ORDER ? "SWAP WHOM?" : sg.menu_page == FM_ORDER2 ? "WITH WHOM?" : "WHO?";
            text_draw(hint, 104, 12 + nposse() * 24 + 2, C_TAN);
        }
        snprintf(b, sizeof b, "%s", spur_map_name(sg.map_id));
        tiny_draw(b, 10, 170, C_GREY);
        break;
    case FM_ITEM: case FM_ITEM_DO: {
        int pg = sg.menu_slot / 10;
        panel(100, 8, 212, 118);
        snprintf(b, sizeof b, "BAGS %d/2", pg + 1);
        text_draw(b, 108, 12, C_TAN);
        for (int i = 0; i < 10; i++) {
            int it = sg.s.bag[pg * 10 + i];
            text_draw(it ? SPUR_ITEM[it].name : "-", 120, 24 + i * 10, pg * 10 + i == sg.menu_slot ? C_YELLOW : it ? C_WHITE : C_SLATE);
        }
        cursor(110, 24 + (sg.menu_slot % 10) * 10);
        int it = sg.s.bag[sg.menu_slot];
        if (it) {
            const SpurItemDef *d = &SPUR_ITEM[it];
            if (d->kind == IK_WEAPON) snprintf(b, sizeof b, "ATK %d%s", d->power, d->hits > 1 && d->wclass != WC_HEAVY ? (d->hits == 2 ? " TWICE" : " THRICE") : "");
            else if (d->kind == IK_NECK) snprintf(b, sizeof b, "EVD %d%s", d->power, d->res ? " RES 30" : "");
            else if (d->kind == IK_BODY) snprintf(b, sizeof b, "DEF %d%s", d->power, d->flags & IF_LIGHT ? " LIGHT" : "");
            else if (d->kind == IK_FEET) snprintf(b, sizeof b, "SPD %d", d->power);
            else if (d->kind == IK_ORE) snprintf(b, sizeof b, "SELLS FOR %d", spur_sell_price(it));
            else b[0] = 0;
            tiny_draw(b, 230, 26, C_TAN);
        }
        if (sg.menu_page == FM_ITEM_DO) {
            static const char *const DO[] = {"USE", "DROP"};
            list_box("", DO, 2, sg.menu_sel2, 236, 60, 64);
        }
        break;
    }
    case FM_SKILL: case FM_SKILL_TGT: {
        int h = spur_hero_at(sg.menu_who);
        menu_nskills = spur_skill_list(h, menu_skills);
        panel(100, 8, 212, 118);
        snprintf(b, sizeof b, "%s  SP %d/%d", SPUR_HERO[h].name, sg.s.hero[h].sp, spur_maxsp(h));
        text_draw(b, 108, 12, C_TAN);
        int top = imax(0, sg.menu_skill - 8);
        for (int i = top; i < menu_nskills && i < top + 9; i++) {
            const SpurSkillDef *d = &SPUR_SKILL[menu_skills[i]];
            bool usable = (d->use & SKU_FIELD) && sg.s.hero[h].sp >= d->sp;
            text_draw(d->name, 120, 24 + (i - top) * 10, i == sg.menu_skill ? C_YELLOW : usable ? C_WHITE : C_SLATE);
            snprintf(b, sizeof b, "%d", d->sp);
            tiny_draw(b, 210, 25 + (i - top) * 10, C_SKY);
        }
        cursor(110, 24 + (sg.menu_skill - top) * 10);
        if (menu_nskills) {
            char lines[3][UI_WRAP_LEN];
            int nl = ui_wrap(SPUR_SKILL[menu_skills[sg.menu_skill]].desc, 196, true, lines, 3);
            for (int i = 0; i < imin(nl, 3); i++) tiny_draw(lines[i], 108, 114 + i * 7 - 14, C_CREAM);
        }
        if (sg.menu_page == FM_SKILL_TGT) {
            who_list(sg.menu_sel2, 116, 30);
        }
        break;
    }
    case FM_EQUIP: case FM_EQUIP_PICK: {
        int h = spur_hero_at(sg.menu_who);
        static const char *const SLOT[EQ_COUNT] = {"R HAND", "L HAND", "NECK", "BODY", "FEET"};
        panel(100, 8, 212, 118);
        snprintf(b, sizeof b, "%s  ATK %d DEF %d SPD %d EVD %d", SPUR_HERO[h].name, spur_attack_power(h), spur_stat(h, ST_DEF),
                 spur_stat(h, ST_SPD), spur_stat(h, ST_EVD));
        tiny_draw(b, 108, 13, C_TAN);
        for (int s = 0; s < EQ_COUNT; s++) {
            int it = sg.s.hero[h].equip[s];
            tiny_draw(SLOT[s], 116, 26 + s * 12, C_GREY);
            text_draw(it ? SPUR_ITEM[it].name : "-", 152, 24 + s * 12, s == sg.menu_slot ? C_YELLOW : C_WHITE);
        }
        cursor(106, 24 + sg.menu_slot * 12);
        if (sg.menu_page == FM_EQUIP_PICK) {
            int opts[SPUR_BAG + 1], no = 0;
            opts[no++] = -1;
            for (int i = 0; i < SPUR_BAG; i++) if (sg.s.bag[i] && spur_can_equip(h, sg.s.bag[i], sg.menu_slot)) opts[no++] = i;
            panel(150, 40, 150, 14 + imin(no, 8) * 10);
            int top = imax(0, sg.menu_sel2 - 7);
            for (int i = top; i < no && i < top + 8; i++)
                text_draw(opts[i] < 0 ? "(TAKE OFF)" : SPUR_ITEM[sg.s.bag[opts[i]]].name, 166, 46 + (i - top) * 10,
                          i == sg.menu_sel2 ? C_YELLOW : C_WHITE);
            cursor(156, 46 + (sg.menu_sel2 - top) * 10);
        }
        break;
    }
    case FM_STATS: {
        int h = spur_hero_at(sg.menu_who);
        const SpurHero *hr = &sg.s.hero[h];
        panel(100, 8, 212, 150);
        snprintf(b, sizeof b, "%s THE %s", SPUR_HERO[h].name, SPUR_HERO[h].klass);
        text_draw(b, 108, 12, C_YELLOW);
        snprintf(b, sizeof b, "LEVEL %d   XP %d   NEXT %d", hr->level, hr->xp, hr->level < 50 ? spur_xp_for(hr->level + 1) - hr->xp : 0);
        tiny_draw(b, 108, 24, C_WHITE);
        static const char *const SN[ST_COUNT] = {"HP", "SP", "ATK", "DEF", "SPD", "EVD", "RES", "VIT"};
        for (int s = 0; s < ST_COUNT; s++) {
            snprintf(b, sizeof b, "%s %d", SN[s], s == ST_HP ? spur_maxhp(h) : s == ST_SP ? spur_maxsp(h) : spur_stat(h, s));
            tiny_draw(b, 108 + (s % 4) * 50, 34 + (s / 4) * 9, C_CREAM);
        }
        tiny_draw("TRAITS", 108, 56, C_TAN);
        int k = 0;
        for (int p = 0; p < PS_COUNT; p++) {
            int lv = spur_passive(h, p);
            if (!lv) continue;
            if (p == PS_ONEARM || p == PS_DOG || p == PS_DUAL) snprintf(b, sizeof b, "%s", SPUR_PASSIVE_NAME[p]);
            else snprintf(b, sizeof b, "%s %d", SPUR_PASSIVE_NAME[p], lv);
            tiny_draw(b, 116 + (k % 2) * 90, 65 + (k / 2) * 8, k == sg.menu_sel2 ? C_YELLOW : C_WHITE);
            if (k == sg.menu_sel2) {
                char lines[2][UI_WRAP_LEN];
                int nl = ui_wrap(SPUR_PASSIVE_DESC[p], 196, true, lines, 2);
                for (int i = 0; i < imin(nl, 2); i++) tiny_draw(lines[i], 108, 140 + i * 7, C_CREAM);
            }
            k++;
        }
        break;
    }
    case FM_KEYS: {
        panel(100, 8, 212, 150);
        text_draw("KEY ITEMS", 108, 12, C_TAN);
        int k = 0;
        for (int i = 0; i < KI_COUNT; i++) {
            if (!spur_key(i)) continue;
            text_draw(SPUR_KEYITEM_NAME[i], 120, 24 + k * 10, k == sg.menu_sel2 ? C_YELLOW : C_WHITE);
            if (k == sg.menu_sel2) {
                char lines[2][UI_WRAP_LEN];
                int nl = ui_wrap(SPUR_KEYITEM_DESC[i], 196, true, lines, 2);
                for (int j = 0; j < imin(nl, 2); j++) tiny_draw(lines[j], 108, 140 + j * 7, C_CREAM);
            }
            k++;
        }
        if (!k) text_draw("NOTHING YET.", 120, 24, C_GREY);
        break;
    }
    default: break;
    }
}

static void draw_shop(void) {
    SpurUi *u = &sg.ui;
    const SpurShop *s = &SPUR_SHOP[u->shop];
    static const char *const ROOTS[] = {"BUY", "SELL", "LEAVE"};
    list_box(s->name, ROOTS, 3, u->mode == 0 ? u->sel : -1, 8, 8, 100);
    teeth_box();
    char b[64];
    if (u->mode == 1) {
        int n = shop_count(u->shop);
        panel(114, 8, 198, 18 + n * 10);
        for (int i = 0; i < n; i++) {
            int it = s->item[i];
            text_draw(SPUR_ITEM[it].name, 130, 14 + i * 10, i == u->sel2 ? C_YELLOW : C_WHITE);
            snprintf(b, sizeof b, "%d", SPUR_ITEM[it].price);
            text_draw(b, 304 - text_width(b), 14 + i * 10, C_CREAM);
        }
        cursor(120, 14 + u->sel2 * 10);
        /* who could use it */
        int it = s->item[u->sel2];
        if (SPUR_ITEM[it].kind >= IK_WEAPON && SPUR_ITEM[it].kind <= IK_FEET) {
            int x = 10;
            static const int SLOTS[] = {EQ_RHAND, EQ_RHAND, EQ_NECK, EQ_BODY, EQ_FEET};
            for (int i = 0; i < nposse(); i++) {
                int h = spur_hero_at(i);
                if (h < 0) continue;
                bool can = spur_can_equip(h, it, SLOTS[SPUR_ITEM[it].kind - IK_WEAPON]);
                text_draw(SPUR_HERO[h].name, x, 70, can ? C_LIME : C_SLATE);
                x += text_width(SPUR_HERO[h].name) + 8;
            }
        }
    } else if (u->mode == 2) {
        int n = 0;
        for (int i = 0; i < SPUR_BAG; i++) n += sg.s.bag[i] != 0;
        panel(114, 8, 198, 118);
        int top = imax(0, u->sel2 - 9);
        for (int i = top; i < n && i < top + 10; i++) {
            int it = sg.s.bag[i];
            text_draw(SPUR_ITEM[it].name, 130, 14 + (i - top) * 10, i == u->sel2 ? C_YELLOW : C_WHITE);
            snprintf(b, sizeof b, "%d", spur_sell_price(it));
            text_draw(b, 304 - text_width(b), 14 + (i - top) * 10, C_CREAM);
        }
        if (n) cursor(120, 14 + (u->sel2 - top) * 10);
        else text_draw("NOTHING TO SELL.", 130, 14, C_GREY);
    }
    if (shop_note[0] && u->mode) text_draw(shop_note, 12, 162, C_YELLOW);
}

static void draw_pick(void) {
    const char *items[12];
    for (int i = 0; i < pick_n; i++) items[i] = pick_opt[i];
    int w = text_width(pick_title) + 20;
    for (int i = 0; i < pick_n; i++) w = imax(w, text_width(items[i]) + 30);
    list_box(pick_title, items, pick_n, sg.ui.sel, 160 - w / 2, 30, w);
    teeth_box();
}

static void draw_number(void) {
    char b[32];
    panel(80, 50, 160, 50);
    text_draw(pick_title, 90, 56, C_TAN);
    snprintf(b, sizeof b, "%07d", sg.ui.num);
    int x = 120;
    for (int i = 0; i < 7; i++) {
        char c[2] = {b[i], 0};
        bool cur = 6 - i == sg.ui.sel;
        text_draw_scaled(c, x + i * 12, 70, cur ? C_YELLOW : C_WHITE, 2);
    }
    tiny_draw(GLYPH_LEFT GLYPH_RIGHT " DIGIT   " GLYPH_UP GLYPH_DOWN " AMOUNT", 96, 90, C_GREY);
    teeth_box();
}

void spur_menu_draw(void) {
    switch (sg.ui.kind) {
    case UI_TEXT: case UI_YESNO: draw_text_box(); break;
    case UI_MENU: draw_field_menu(); break;
    case UI_SHOP: draw_shop(); break;
    case UI_PICK: draw_pick(); break;
    case UI_NUMBER: draw_number(); break;
    default: break;
    }
}
