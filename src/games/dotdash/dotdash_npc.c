/* DOT & DASH - the people of the room: what they say, what they sell and
 * the favours they ask. Every name and line is our own. */
#include "dotdash.h"

const char *const DD_UPNAME[U_ABILITIES] = {
    "GRIP MITT", "GRIP MITT II", "SHRINK TONIC", "SHRINK TONIC II", "DROP BANGLE", "SATCHEL", "SATCHEL II",
    "SPRING BEAN", "SPRING BEAN II", "FEATHER CHARM", "KICK CLOGS", "KICK CLOGS II", "BUG MUSK", "BUZZBOOK",
    "BUZZBOOK II", "FIZZ DROP", "SPIN TOP", "SPIN TOP II", "PUP WHISTLE", "SPORE STEW", "SHELL VEST",
    "DASH'S PLATE", "KITE WINGS",
};
const char *const DD_UPDESC[U_ABILITIES] = {
    "LIFT CRITTERS", "LIFT THINGS FASTER", "SHRINK TO MICRO SIZE", "SHRINK TO DEEP SIZE",
    GLYPH_DOWN GLYPH_DOWN " DROPS THROUGH LEDGES", GLYPH_DOWN "+" GLYPH_A " STORES A THING", "STORES TWO THINGS",
    GLYPH_UP "+" GLYPH_A " JUMPS HIGHER", "JUMPS HIGHER STILL", "NO MORE FALL DAMAGE", GLYPH_UP "+" GLYPH_B " KICKS",
    "KICKS HURT", "BUGS CAN'T HURT YOU", "STAND ON A BUG, " GLYPH_UP " COMMANDS IT", "GERMS OBEY TOO",
    GLYPH_LEFT GLYPH_LEFT " OR " GLYPH_RIGHT GLYPH_RIGHT " SPRINTS", "JUMP INTO SHOTS AND HOPPERS", "FLIPPED FOES GET HURT",
    GLYPH_UP GLYPH_UP " CALLS DASH", "SLIME AND ACID CAN'T HURT YOU", "HALF DAMAGE", "DASH CAN'T BE HURT",
    "DASH CAN FLY",
};

static const char *const NAME[N_KINDS] = {
    [N_GRANNY] = "GRANNY THIMBLE", [N_CRUMB] = "PROFESSOR CRUMB", [N_VOLT] = "DR. VOLT", [N_PILGRIM] = "A PILGRIM",
    [N_SILK] = "MADAME SILK", [N_FILAMENT] = "THE OLD FILAMENT", [N_TOCK] = "TOCK", [N_MOTHERFLUFF] = "MOTHER FLUFF",
    [N_TUFTY] = "TUFTY", [N_PUFFIN] = "PUFFIN", [N_INKY] = "INKY", [N_COWPOKE] = "COWPOKE", [N_WEEPY] = "WEEPY",
    [N_SOLDIER] = "PRIVATE PEAT-MOSS", [N_SHREW] = "BARLEY THE SHREW", [N_SPINNER] = "SPINNER",
    [N_TAPER] = "TAPER", [N_TALLOW] = "ELDER TALLOW", [N_SNUFF] = "SNUFF", [N_SMUDGE] = "SMUDGE",
    [N_FERN] = "MAYOR FERN", [N_SPRIG] = "SPRIG", [N_LICHEN] = "LICHEN", [N_MASON] = "MASON MOSS", [N_BRACKEN] = "SCOUT BRACKEN",
    [N_CLOD] = "CLOD", [N_SORREL] = "SORREL", [N_PEAT] = "CAPTAIN PEAT", [N_SMITH] = "THE MOSS SMITH",
    [N_WRIGGLA] = "QUEEN WRIGGLA", [N_KNOT] = "KNOT", [N_BORER] = "CHEF BORER",
    [N_OLDCAP] = "OLD CAP", [N_ONEEYE] = "ONE-EYE", [N_SEEDKEEPER] = "THE SEEDKEEPER", [N_GILL] = "GILL", [N_MOREL] = "CHEF MOREL",
    [N_HIGHLUMEN] = "THE HIGH LUMEN", [N_WARDEN] = "THE WICK WARDEN",
    [N_TABITHA] = "QUEEN TABITHA", [N_MAGE] = "THE TIN MAGE", [N_KNIGHT] = "A TIN KNIGHT", [N_PELL] = "PELL", [N_RATCHET] = "COOK RATCHET",
    [N_NIB] = "NIB", [N_NATIVE] = "A LOCAL", [N_HOPPERSAGE] = "AN OLD GRASSHOPPER", [N_BUBBLE] = "SOMEONE IN A BUBBLE",
    [N_WAXLING] = "A WAXLING",
};
const char *dd_npc_name(int sub) { return sub >= 0 && sub < N_KINDS ? NAME[sub] : ""; }

/* ------------------------------------------------------------------ */
/* the dialogue box                                                     */

#define PAGES 8
static struct { char who[32]; char text[240]; } page[PAGES];
static int n_pages, cur, shown, box_t;
static void (*ask_cb)(int yes);
static bool asking;
static int ask_sel;
int dd_answer = -1;

void dd_say(const char *who, const char *text) {
    if (n_pages >= PAGES) return;
    snprintf(page[n_pages].who, sizeof page[n_pages].who, "%s", who);
    snprintf(page[n_pages].text, sizeof page[n_pages].text, "%s", text);
    n_pages++;
    if (n_pages == 1) { cur = 0; shown = 0; box_t = 0; }
}
void dd_say_more(const char *text) { dd_say(n_pages ? page[n_pages - 1].who : "", text); }

void dd_ask(const char *who, const char *text, void (*done)(int yes)) {
    dd_say(who, text);
    ask_cb = done;
    asking = true;
    ask_sel = 1;
}

bool dd_dialog_active(void) { return n_pages > 0; }

void dd_dialog_update(void) {
    if (!n_pages) return;
    box_t++;
    int len = (int)strlen(page[cur].text);
    bool a = (dd_in & BTN_A) && !(dd_in_prev & BTN_A), b = (dd_in & BTN_B) && !(dd_in_prev & BTN_B);
    bool lr = ((dd_in & (BTN_LEFT | BTN_RIGHT | BTN_UP | BTN_DOWN)) & ~dd_in_prev) != 0;
    if (shown < len) {
        shown += 2;
        if (box_t % 4 == 0) sfx_play_name("dd_blip");
        if ((a || b) && box_t > 3) shown = len;
        return;
    }
    bool last = cur == n_pages - 1;
    if (last && asking) {
        if (lr) { ask_sel ^= 1; sfx_play_name("ui_move"); }
        if (a || b) {
            int yes = a ? ask_sel == 0 : 0;
            void (*cb)(int) = ask_cb;
            asking = false;
            ask_cb = NULL;
            n_pages = 0;
            dd_answer = yes;
            sfx_play_name(yes ? "ui_ok" : "ui_back");
            if (cb) cb(yes);
        }
        return;
    }
    if (a || b) {
        if (last) n_pages = 0;
        else { cur++; shown = 0; box_t = 0; }
    }
}

void dd_dialog_draw(void) {
    if (!n_pages) return;
    int y = 118;
    ui_panel(6, y, 308, 58, C_NIGHT, C_CREAM);
    text_draw(page[cur].who, 14, y + 5, C_YELLOW);
    char buf[240];
    int len = (int)strlen(page[cur].text);
    int n = imin(shown, len);
    memcpy(buf, page[cur].text, (size_t)n);
    buf[n] = 0;
    text_wrap(buf, 14, y + 16, 292, C_WHITE, 9);
    if (shown >= len) {
        if (cur == n_pages - 1 && asking) {
            text_draw(ask_sel == 0 ? GLYPH_RIGHT "YES" : " YES", 220, y + 46, ask_sel == 0 ? C_YELLOW : C_GREY);
            text_draw(ask_sel == 1 ? GLYPH_RIGHT "NO" : " NO", 262, y + 46, ask_sel == 1 ? C_YELLOW : C_GREY);
        } else if ((box_t / 15) % 2) text_draw(GLYPH_A, 296, y + 47, C_CREAM);
    }
}

void dd_dialog_reset(void) { n_pages = 0; asking = false; ask_cb = NULL; }
bool dd_dialog_asking(void) { return n_pages > 0 && asking && cur == n_pages - 1 && shown >= (int)strlen(page[cur].text); }

/* ------------------------------------------------------------------ */
/* helpers                                                                */

static bool carrying(int sub) {
    return dd_carry >= 0 && dd_ent[dd_carry].alive && dd_ent[dd_carry].kind == EK_OBJ && dd_ent[dd_carry].sub == sub;
}
static bool carrying_foe(int sub) {
    return dd_carry >= 0 && dd_ent[dd_carry].alive && dd_ent[dd_carry].kind == EK_FOE && dd_ent[dd_carry].sub == sub;
}
static int carried_param(void) { return dd_carry >= 0 ? dd_ent[dd_carry].param : -1; }
static void take_carried(void) {
    if (dd_carry >= 0) { dd_ent[dd_carry].alive = 0; dd_carry = -1; }
    sfx_play_name("dd_give");
}
static void swap_carried(int sub) {
    if (dd_carry >= 0) { dd_ent[dd_carry].sub = (uint8_t)sub; dd_ent[dd_carry].param = 0; }
}
static void pay(int n) {
    dd_give_glints(n);
    char b[24];
    snprintf(b, sizeof b, "+%d", n);
    dd_popup(dd_p.x, dd_p.y - 12, b, C_YELLOW);
    sfx_play_name("dd_glint5");
}
static int talk_ent;
static const char *who(void) { return talk_ent >= 0 ? NAME[dd_ent[talk_ent].sub] : ""; }
static void say(const char *t) { dd_say(who(), t); }
static int popcount8(int v) { int n = 0; for (int k = 0; k < 8; k++) n += (v >> k) & 1; return n; }

bool dd_npc_visible(int sub) {
    switch (sub) {
    case N_FILAMENT: return dd_flag(FL_LAMP_OFF);
    case N_PILGRIM: return !dd_flag(FL_LAMP_OFF);
    case N_NIB: return dd_flag(FL_FILAMENT_PAID);
    case N_PUFFIN: return true;
    default: return true;
    }
}

/* ------------------------------------------------------------------ */
/* shops: three stands each, a cheap thing, a useful thing, an upgrade   */

#define OBJ_ITEM(o) (100 + (o))
void dd_shop_items(int npc, int *it, int *pr) {
    static const int ITEMS[][6] = {
        {N_TAPER, OBJ_ITEM(O_HEARTBOX), OBJ_ITEM(O_HARD), U_TONIC2, 0, 0},
        {N_SPRIG, OBJ_ITEM(O_HEARTBOX), OBJ_ITEM(O_QUAKE), U_BEAN1, 0, 0},
        {N_MOTHERFLUFF, OBJ_ITEM(O_CRACKER), OBJ_ITEM(O_JAR), U_SATCHEL1, 0, 0},
        {N_CLOD, OBJ_ITEM(O_HEARTBOX), OBJ_ITEM(O_TABLET), U_BANGLE, 0, 0},
        {N_KNOT, OBJ_ITEM(O_JAR), OBJ_ITEM(O_AXE), U_FEATHER, 0, 0},
    };
    static const int PRICES[][3] = {{1, 3, 25}, {1, 3, 25}, {2, 3, 25}, {1, 10, 25}, {3, 5, 25}};
    for (int k = 0; k < 5; k++)
        if (ITEMS[k][0] == npc) {
            for (int j = 0; j < 3; j++) { it[j] = ITEMS[k][1 + j]; pr[j] = PRICES[k][j]; }
            return;
        }
    it[0] = it[1] = it[2] = -1;
}
int dd_item_upgrade(int item) { return item < 100 ? item : -1; }

static int buy_ent, buy_item, buy_price;
static const char *item_name(int item) {
    static const char *ON[O_KINDS] = {
        [O_HEARTBOX] = "A HEART BOX", [O_HARD] = "A HARD BLOCK", [O_QUAKE] = "A QUAKE BLOCK", [O_CRACKER] = "A CRACKER",
        [O_JAR] = "A GLASS JAR", [O_TABLET] = "A STONE TABLET", [O_AXE] = "A THROWING AXE",
    };
    if (item >= 100) return ON[item - 100] ? ON[item - 100] : "THAT";
    return DD_UPNAME[item];
}
static void buy_done(int yes) {
    if (!yes) return;
    if (dd_sv.glints < buy_price) { dd_say("", "NOT ENOUGH GLINTS."); sfx_play_name("ui_error"); return; }
    dd_sv.glints -= buy_price;
    Ent *s = &dd_ent[buy_ent];
    if (buy_item >= 100) {
        int j = dd_add_obj(buy_item - 100, s->x, s->y - 10);
        (void)j;
        sfx_play_name("dd_give");
    } else dd_give_upgrade(buy_item);
    dd_autosave();
}
void dd_buy(int ent) {
    Ent *s = &dd_ent[ent];
    int it[3], pr[3];
    dd_shop_items(s->param, it, pr);
    int k = iclamp(s->sub, 0, 2);
    if (it[k] < 0) return;
    if (it[k] < 100 && dd_has(it[k])) { dd_say(NAME[s->param], "SOLD OUT! YOU HAVE THAT ALREADY."); return; }
    buy_ent = ent;
    buy_item = it[k];
    buy_price = pr[k];
    char buf[160];
    if (it[k] < 100) snprintf(buf, sizeof buf, "%s: %s. %d GLINTS?", DD_UPNAME[it[k]], DD_UPDESC[it[k]], pr[k]);
    else snprintf(buf, sizeof buf, "%s FOR %d GLINT%s?", item_name(it[k]), pr[k], pr[k] == 1 ? "" : "S");
    dd_ask(NAME[s->param], buf, buy_done);
}

/* ------------------------------------------------------------------ */
/* yes/no callbacks                                                        */

static void cb_mitt(int yes) {
    if (!yes) return;
    if (dd_sv.glints < 20) { dd_say("GRANNY THIMBLE", "YOU'LL NEED 20 GLINTS, DEAR. THEY'RE EVERYWHERE ONCE YOU LOOK SMALL ENOUGH."); return; }
    dd_sv.glints -= 20;
    dd_set(FL_MITT_SOLD);
    dd_give_upgrade(U_MITT1);
}
static void cb_volt(int yes) {
    if (!yes) { dd_say("DR. VOLT", "SUIT YOURSELF. THE WALLS WILL KEEP HUMMING."); return; }
    dd_set(FL_POWER_OFF);
    dd_autosave();
    dd_say("DR. VOLT", "CLUNK! THE BREAKER'S THROWN. THE FRIED OUTLET BEHIND THE BOOKSHELF IS SAFE NOW, A DOOR INTO THE WALLS. MIND THE WORMS.");
}
static void cb_queen(int yes) {
    if (!yes) { dd_say("QUEEN TABITHA", "THEN SCURRY ALONG, LITTLE ONE."); return; }
    dd_sv.glints -= 500;
    dd_set(FL_PAID_QUEEN);
    if (dd_dash >= 0) { dd_ent[dd_dash].alive = 0; dd_dash = -1; }
    dd_say("QUEEN TABITHA", "MMM, SHINY. THANK YOU, DEAR. OH, AND I'LL BE KEEPING THE DOG. I'VE ALWAYS WANTED A FOOTSTOOL THAT BARKS.");
    dd_say_more("SIR SPROCKET! SEE OUR GUEST OUT. PERMANENTLY.");
    /* the knight takes the floor of the next hall */
    for (int c = 0; c < dd_lv.d.n; c++)
        if (dd_lv.town[c] == dd_town_id("latchtown") && dd_lv.town_part[c] == 3) {
            int j = dd_add_foe(F_SPROCKET, (float)((c * CHUNK_W + 22) * DD_TS), (float)(12 * DD_TS - 48));
            (void)j;
        }
    dd_autosave();
}
static void cb_mage(int yes) {
    if (!yes) return;
    if (dd_sv.glints < 50) { dd_say("THE TIN MAGE", "FIFTY. NOT ONE GLINT LESS."); return; }
    dd_sv.glints -= 50;
    dd_set(FL_THRONE_OPEN);
    for (int c = 0; c < dd_lv.d.n; c++)
        if (dd_lv.town[c] == dd_town_id("latchtown") && dd_lv.town_part[c] == 2)
            for (int y = 6; y <= 11; y++) { lv_set(&dd_lv, c * CHUNK_W + 1, y, T_AIR); lv_set(&dd_lv, c * CHUNK_W + 2, y, T_AIR); }
    dd_say("THE TIN MAGE", "THE THRONE HALL IS OPEN. HER MAJESTY WILL SEE YOU NOW.");
    dd_autosave();
}
static void cb_pell(int yes) {
    if (!yes) { dd_say("PELL", "OH. WELL. I'LL JUST... STAY HERE THEN."); return; }
    dd_set(FL_PELL_FREED);
    if (talk_ent >= 0) dd_ent[talk_ent].alive = 0;
    dd_say("PELL", "FREE! I'M OFF HOME TO TUFTVILLE. ELDER TALLOW WILL HEAR WHAT YOU DID!");
    dd_autosave();
}
static void cb_knight(int yes) {
    if (!yes) return;
    dd_set(FL_PELL_GUARDED);
    pay(10);
    dd_say("A TIN KNIGHT", "GOOD. YOU WATCH HIS LEFT, I'LL WATCH HIS RIGHT. HE'S NOT GOING ANYWHERE NOW.");
}
static void cb_tock(int yes) {
    if (!yes) { dd_say("TOCK", "TICK. TOCK. I WILL WAIT. I AM GOOD AT WAITING."); return; }
    if (dd_sv.glints < 1000) { dd_say("TOCK", "A THOUSAND, I SAID. THE CLOCK DOES NOT HAGGLE."); return; }
    dd_sv.glints -= 1000;
    dd_set(FL_BALANCE);
    dd_autosave();
    dd_say("TOCK", "WHIRR... CLICK. THE HANDS SWING BACK. THE ROOM SIGHS. THE GLINTS YOU TOOK ARE PAID FOR AND THE MINUTES YOU SKIPPED ARE RETURNED.");
    if (dd_flag(FL_ESCAPED) || dd_flag(FL_SPROCKET_DEAD)) {
        dd_say_more("THE DOOR WILL OPEN FOR YOU NOW, AND IT WILL STAY OPEN. GO AND JOIN THE PARTY, DOT.");
        dd_set(FL_TRUE_END);
        dd_start_ending(true);
    } else dd_say_more("NOW GET OUT OF THIS ROOM, AND THE ROOM WILL STAY GOOD.");
}
static void cb_warden(int yes) {
    if (!yes) return;
    dd_set(FL_LAMP_OFF);
    dd_lamp_dark = 1;
    dd_autosave();
    dd_say("THE WICK WARDEN", "CLICK. THE GREAT LAMP SLEEPS. LISTEN, UP ON THE SHADE: THE OLD FILAMENT IS AWAKE AND WILL SPEAK TO YOU.");
}
static void cb_letter(int yes) { (void)yes; }

/* ------------------------------------------------------------------ */
/* talking                                                                 */

static void talk_granny(void) {
    if (!dd_flag(FL_MET_GRANNY)) {
        dd_set(FL_MET_GRANNY);
        say("WELL I NEVER! A LITTLE GIRL NO BIGGER THAN A BUTTON, AND A DOG TO MATCH. I'M GRANNY THIMBLE. I SHRANK ONE AFTERNOON AND NEVER BOTHERED GROWING BACK.");
        say("LOCKED IN, ARE YOU? THE DOOR ANSWERS TO THE CAT WHO SITS ON TOP OF IT. TABITHA. SHE WAS MY CAT ONCE. NOW SHE CALLS HERSELF A QUEEN AND CHARGES 500 GLINTS TO LET ANYONE OUT.");
        say("THOSE SPECTACLES ON MY TABLE BELONG TO PROFESSOR CRUMB IN THE WEST POT. HE CAN'T SEE PAST HIS NOSE WITHOUT THEM. TAKE THEM TO HIM. HE KNOWS HOW TO GET SMALLER STILL.");
        return;
    }
    if (carrying(O_SPECS)) { say("THAT'S THEM! UP THE IVY, OVER THE RIM OF THE WEST POT. CRUMB'S THE ONE SQUINTING AT THE SOIL."); return; }
    if (!dd_has(U_MITT1)) {
        dd_ask("GRANNY THIMBLE", "I STILL HAVE MY OLD GRIP MITT. WITH IT YOU CAN PICK UP CRITTERS, NOT JUST BLOCKS. YOURS FOR 20 GLINTS?", cb_mitt);
        return;
    }
    if (dd_flag(FL_ESCAPED) && !dd_flag(FL_TRUE_END)) { say("YOU GOT OUT? THEN WHAT ARE YOU DOING BACK IN HERE... ANOTHER PARTY, AND YOUR BROTHER LOCKED YOU IN AGAIN? OH, THAT BOY."); return; }
    static const char *H[] = {
        "EVERY SPECK OF THIS ROOM HAS A WHOLE COUNTRY INSIDE IT. HOLD DOWN AND SEE.",
        "THE BIG GLINTS, THE FIFTY-GLINT ONES, HIDE IN THE DANGEROUS CAVES DEEP IN THINGS.",
        "IF YOU'RE EVER TOO SMALL AND TOO LOST, HOLD UP. YOU'LL GROW BACK A SIZE.",
        "TABITHA LOVED HER DINNER. HER COOK IN LATCHTOWN STILL ASKS AFTER MY CAT FOOD.",
    };
    say(H[dd_sv.clock_min % 4]);
}

static void talk_crumb(void) {
    if (dd_flag(FL_TONIC_GIVEN)) {
        if (!dd_flag(FL_DASH_CLEAN)) say("YOUR DOG IS SCRATCHING LIKE MAD. STAND ON HIS BACK AND SHRINK, AND DEAL WITH WHATEVER'S BITING HIM.");
        else say("HOLD DOWN ON ANY SPECK AND YOU'LL FIND A WHOLE COUNTRY IN IT. THE FOLK DOWN THERE WILL POINT YOU THE WAY.");
        return;
    }
    if (carrying(O_SPECS)) {
        take_carried();
        dd_set(FL_SPECS_GIVEN);
        dd_set(FL_TONIC_GIVEN);
        say("MY SPECTACLES! AH, NOW I CAN SEE YOU'RE A GIRL AND THAT'S A DOG. I CAME DOWN HERE ON PURPOSE, YOU KNOW, TO STUDY THE VERY SMALL.");
        dd_give_upgrade(U_TONIC1);
        say("HAVE A SWIG OF MY SHRINK TONIC. NOW YOU CAN SHRINK INTO WHATEVER YOU STAND ON, ALL THE WAY DOWN TO MICRO SIZE.");
        return;
    }
    say("WHO'S THERE? ALL I SEE IS FUZZ! I'VE LOST MY SPECTACLES. GRANNY THIMBLE, IN THE THIMBLE BY THE DOOR, MIGHT HAVE SEEN THEM.");
}

static void talk_volt(void) {
    if (dd_flag(FL_POWER_OFF)) { say("THE BREAKER'S OFF. THE FRIED OUTLET BEHIND THE BOOKSHELF IS A DOOR NOW."); return; }
    dd_ask("DR. VOLT", "I STUDY THE LIGHTNING IN THE WALLS. THE OUTLET BEHIND THE BOOKSHELF BLEW LAST WEEK. IF I THROW THE BREAKER, IT'S JUST A HOLE. SHALL I?", cb_volt);
}

static void talk_silk(void) {
    if (dd_flag(FL_SILK_DONE)) { say("DIVINE. I SHALL WRITE A REVIEW. FIVE LEGS OUT OF EIGHT."); return; }
    if (carrying_foe(F_GERM)) {
        take_carried();
        dd_set(FL_SILK_DONE);
        pay(30);
        say("A GREEN GERM! FRESH! OH, THE TANG OF IT. HERE, FOR YOUR TROUBLE, LITTLE GIRL.");
        return;
    }
    say("I HAVE EATEN EVERY FLY IN THIS ROOM AND THEY ALL TASTE OF DUST. BRING ME SOMETHING NEW. A GREEN GERM, FROM THE DUST UNDER THE BOOKSHELF. CARRY IT, DON'T SQUASH IT.");
}

static void talk_filament(void) {
    if (dd_flag(FL_FILAMENT_PAID)) { say("BEHIND THE QUEEN'S THRONE, SMALLER THAN SMALL. GO."); return; }
    if (carrying(O_BIGBANG)) {
        take_carried();
        dd_set(FL_BIGBANG_DONE);
        dd_set(FL_FILAMENT_PAID);
        pay(80);
        say("THE BIG BANG. SAFE WITH ME, WHERE THERE IS NOTHING TO BURN. NOW LISTEN: THE CAT IS NOT THE TRUE POWER ON THE DOOR.");
        say("SOMEONE LIVES BEHIND HER THRONE, SMALLER THAN SMALL. STAND THERE AND SHRINK AS DEEP AS YOU CAN GO. ASK THEM WHY THE CLOCK RUNS WRONG.");
        return;
    }
    dd_set(FL_FILAMENT_ASKED);
    say("THE LAMP IS DARK, SO I CAN SPEAK. I AM THE OLD FILAMENT. I HAVE WATCHED THE DOOR FOR YEARS, AND I KNOW WHO REALLY RULES IT.");
    say("BUT FIRST: THERE IS A FIRECRACKER CALLED THE BIG BANG AT THE FAR END OF THE SIEGE BOX ON THE MIDDLE SHELF. BRING IT HERE BEFORE IT BLOWS THIS ROOM TO BITS.");
}

static void talk_tock(void) {
    dd_set(FL_TOCK_MET);
    if (dd_flag(FL_BALANCE)) { say("TICK. TOCK. ALL IS IN BALANCE."); return; }
    if (!dd_flag(FL_MET_NIB)) {
        say("TICK. TOCK. I KEEP THE CLOCK. EVERY TIME YOU GROW TO FULL SIZE, OR FALL DOWN AND GROW BACK, IT JUMPS A MINUTE. THAT IS ALL YOU NEED TO KNOW.");
        return;
    }
    dd_ask("TOCK", "SO NIB SENT YOU. YES, THE ROOM IS OUT OF BALANCE: TOO MANY GLINTS GRABBED, TOO MANY MINUTES SKIPPED. A THOUSAND GLINTS AND I WILL WIND IT RIGHT. NOW?", cb_tock);
}

static void talk_oldcap(void) {
    if (dd_lv.d.kind == LV_AREA && dd_lv.d.id == AR_CLOCKWORKS) {
        if (!dd_flag(FL_OLDCAP_DONE)) {
            dd_set(FL_OLDCAP_DONE);
            say("YOU CAME! THE TABLET SAYS THE CLOCKWORKS HIDE A PEP EGG, AND HERE IT IS, BEHIND THE ESCAPEMENT. TAKE IT. YOUR THROWS WILL THANK YOU.");
            dd_give_upgrade(U_EGG0 + 2);
        } else say("SUCH A LOVELY TICKING, ISN'T IT?");
        return;
    }
    if (carrying(O_TABLET)) {
        take_carried();
        dd_set(FL_TABLET_GIVEN);
        say("A STONE TABLET FROM LOAMTON! LET ME READ... 'IN THE HEART OF THE CLOCK, AN EGG OF PEP.' MEET ME IN THE CLOCKWORKS. THE LITTLE DOOR AT THE FOOT OF THE CLOCK.");
        return;
    }
    say("WE SPORELINGS OF TICKBURG READ STONES. THE CLODS OF LOAMTON SELL THEM, TEN GLINTS A TABLET. BRING ME ONE AND WE'LL SEE WHAT IT SAYS.");
}

static void talk_mother(void) {
    if (dd_flag(FL_PUFF_CLEAN) && !dd_flag(FL_PUFF_PAID)) {
        dd_set(FL_PUFF_PAID);
        pay(20);
        say("PUFFIN'S FLUFFY AGAIN! YOU WENT RIGHT INTO HIS FUR, DIDN'T YOU? BRAVE THING. HERE.");
        return;
    }
    if (!dd_flag(FL_PUFF_CLEAN)) { say("WELCOME TO FLUFF HOLLOW. BUY SOMETHING, DO. AND IF YOU'RE SMALL ENOUGH... MY PUFFIN IS POORLY. SOMETHING IS CRAWLING IN HIS FLUFF."); return; }
    say("CRACKERS GO BANG, JARS HOLD THINGS, AND A SATCHEL KEEPS WHAT YOU CARRY SAFE EVEN IF YOU TAKE A TUMBLE.");
}

static void talk_tufty(void) {
    if (dd_flag(FL_BLUEDUST_DONE)) { say("MEET BLUEBELL! MY NEW LITTLE BROTHER. HE SAYS HELLO. WELL, HE SAYS 'FLUMPH'."); return; }
    if (carrying(O_BLUEDUST)) {
        take_carried();
        dd_set(FL_BLUEDUST_DONE);
        pay(15);
        say("BLUE DUST! WATCH... ROLL IT, FLUFF IT, AND... A NEW DUST BUNNY! THANK YOU!");
        return;
    }
    dd_set(FL_TUFTY_ASKED);
    say("I WANT A LITTLE BROTHER. DUST BUNNIES ARE MADE FROM DUST, BUT IT HAS TO BE BLUE DUST. THERE'S A DRIFT OF IT ON THE BOOKS IN THE TOP OF THE BOOKSHELF, IF YOU GO SMALL.");
}

static void talk_puffin(void) {
    if (dd_flag(FL_PUFF_CLEAN)) { say("I FEEL SO FLUFFY! FLUMPH!"); return; }
    dd_set(FL_PUFF_MET);
    say("ACHOO! ITCHY... SO ITCHY... (SOMETHING IS CRAWLING IN PUFFIN'S FLUFF. STAND ON HIM AND SHRINK.)");
}

static void talk_inky(void) {
    int n = popcount8(dd_sv.counts[QC_PAPERFISH]);
    if (dd_flag(FL_INKY_DONE)) { say("MY BOOKS ARE SAFE. I CAN FINALLY FINISH CHAPTER NINE."); return; }
    if (n >= 6) {
        dd_set(FL_INKY_DONE);
        pay(20);
        say("ALL SIX PAPERFISH, GONE! TAKE THESE, AND WHAT I WAS HIDING BEHIND ME. I'M A BOOKWORM, NOT A BANK.");
        for (int k = 0; k < 4; k++) dd_add_pick(P_GLINT5, dd_p.x + 10 + k * 8, dd_p.y, 0);
        return;
    }
    char b[200];
    snprintf(b, sizeof b, "PAPERFISH ARE EATING MY BOOKS FROM THE INSIDE! SIX OF THEM. SHRINK INTO THE BOOKS AND SQUASH THEM. (%d OF 6 SO FAR)", n);
    say(b);
}

static void talk_cowpoke(void) {
    if (dd_flag(FL_COWPOKE_DONE)) { say("THIS TRAIN AIN'T MOVED IN YEARS, BUT A FELLER CAN DREAM."); return; }
    if (carrying(O_DRINK)) {
        take_carried();
        dd_set(FL_COWPOKE_DONE);
        say("NOW THAT'S A DRINK. HERE'S YOUR SECRET, PARDNER: THE SIEGE BOX HAS TWO BIG GLINTS, ONE ON THE TALL TOWER AND ONE ON A LEDGE PAST THE BRIDGE. AND IN THE WALLS, A CRACKER BLOWS THE WEAK WOOD BY THE FIRST STUD.");
        return;
    }
    say("HOWDY, PARDNER. I'D TELL YOU A SECRET OR TWO FOR A MUSHROOM DRINK. BARLEY THE SHREW MIXES 'EM, BETWEEN THE WALLS.");
}

static void talk_weepy(void) {
    if (dd_flag(FL_WEEPY_DONE)) { say("WHEE! LOOK AT MY NEW WEB! IT'S BLUE!"); return; }
    if (carrying(O_THREAD)) {
        take_carried();
        dd_set(FL_WEEPY_DONE);
        pay(15);
        say("MY BLUE THREAD! OH THANK YOU THANK YOU! NOW I CAN WEAVE AGAIN.");
        return;
    }
    say("BOO HOO. I DROPPED MY SPOOL OF BLUE THREAD WAY UP ON TOP OF THE BOOKSHELF AND I CAN'T WEAVE WITHOUT IT. BOO HOO HOO.");
}

static void talk_soldier(void) {
    if (dd_flag(FL_WATER_PAID)) { say("AT EASE. AND THANK YOU AGAIN."); return; }
    if (dd_flag(FL_WATER_GIVEN)) {
        if (dd_lv.d.kind == LV_STRIP && dd_town_at(&dd_lv, (int)(dd_p.x / DD_TS)) == dd_town_id("loamton")) {
            dd_set(FL_WATER_PAID);
            pay(50);
            say("THERE YOU ARE! AS PROMISED: MY WHOLE PATROL BONUS. I'D BE DUST WITHOUT YOU.");
        } else say("MEET ME IN LOAMTON FOR YOUR REWARD.");
        return;
    }
    if (carrying(O_JARWATER)) {
        take_carried();
        dd_set(FL_WATER_GIVEN);
        say("WATER! GLUG GLUG GLUG... AHH. BLESS YOU. COME SEE ME IN LOAMTON WHEN MY PATROL IS DONE. I'LL HAVE YOUR REWARD.");
        if (talk_ent >= 0) dd_ent[talk_ent].alive = 0;
        return;
    }
    say("WATER... WE PATROL THE DRY FIELDS AND THE WELL'S RUN DRY... THE CEILING DRIPS ONTO THE RIM OF THIS POT, THEY SAY... A JAR...");
}

static void talk_shrew(void) {
    if (!dd_flag(FL_MAGE_BEATEN)) { say("(HIS EYES ARE GLASSY.) MUST... SERVE... THE TIN MAGE... ON THE BOARD... BY THE GRATE..."); return; }
    if (!dd_flag(FL_SHREW_PAID)) {
        dd_set(FL_SHREW_FREE);
        dd_set(FL_SHREW_PAID);
        pay(20);
        say("MY HEAD'S CLEAR! THAT TIN MAGE HAD ME POURING FOR HIS MICE DAY AND NIGHT. HERE, FOR YOUR TROUBLE.");
        return;
    }
    if (carrying(O_SPORE) || carrying(O_JARHONEY)) {
        swap_carried(O_DRINK);
        say("ONE MUSHROOM DRINK, COMING UP. SHAKE, STIR... THROW IT AT SOMETHING NASTY AND WATCH IT WILT. POISON'S CATCHING, TOO.");
        return;
    }
    say("BRING ME A SPORE, OR A JAR OF HONEY FROM THE HIVE ON THE WEST POT, AND I'LL MIX YOU A MUSHROOM DRINK. ANOTHER, ANY TIME.");
}

static void talk_spinner(void) {
    if (dd_flag(FL_GEAR_DONE)) {
        say("WHEN SIR SPROCKET THROWS HIS HEAD, CLIMB ON HIS SHOULDERS AND SHRINK DOWN THE NECK. YOU'LL NEED THE SECOND TONIC. HIS MAINSPRING IS INSIDE.");
        return;
    }
    if (carrying(O_GEAR)) {
        take_carried();
        dd_set(FL_GEAR_DONE);
        pay(15);
        say("THE GEAR! PERFECT. MY CLOCKWORK FLYER WILL FLY YET. LISTEN, ONE ENGINEER TO ANOTHER: THE QUEEN'S KNIGHT, SIR SPROCKET, IS ARMOUR ALL THE WAY THROUGH. NOTHING HURTS HIM FROM OUTSIDE.");
        say("BUT WHEN HE THROWS HIS HEAD, CLIMB ON HIS SHOULDERS AND SHRINK DOWN THE NECK. YOU'LL NEED THE SECOND TONIC. BREAK HIS MAINSPRING AND HE FALLS APART.");
        return;
    }
    say("I'M BUILDING A CLOCKWORK FLYER IN THIS MONEY BOX, BUT I'M ONE GEAR SHORT. THE BLOWN OUTLET BEHIND THE BOOKSHELF SPAT ONE INTO THE WALLS.");
}

static void talk_tallow(void) {
    if (carrying(O_ARTIFACT)) {
        take_carried();
        dd_set(FL_SNARL_DONE);
        pay(10);
        say("THE COVER STONE OF OUR BUZZBOOK! THE SNARL GANG STOLE IT MOONS AGO. WITH IT WHOLE, THE BOOK SPEAKS AGAIN. TAKE IT: STAND ON A BUG AND PRESS " GLYPH_UP ", AND IT WILL DO AS YOU SAY.");
        dd_give_upgrade(U_BUZZ1);
        return;
    }
    if (carrying(O_BABY)) {
        int k = carried_param();
        take_carried();
        if (k >= 0 && k < 8) dd_sv.counts[QC_BABIES] |= (uint8_t)(1u << k);
        pay(15);
        char b[160];
        snprintf(b, sizeof b, "A LOST WAXLING, HOME AGAIN! THANK YOU. THAT'S %d OF THE 8 WHO WANDERED OFF.", popcount8(dd_sv.counts[QC_BABIES]));
        say(b);
        return;
    }
    if (carrying(O_SCROLL)) {
        take_carried();
        dd_set(FL_SCROLL_DONE);
        pay(15);
        say("A LUMEN SCROLL! OUR SNUFF HAS ALWAYS WANTED TO SEE THE LIGHT. HE'LL BE A PILGRIM BY SUPPER.");
        return;
    }
    if (carrying_foe(F_GERM2) && !dd_flag(FL_GERM2_DONE)) {
        take_carried();
        dd_set(FL_GERM2_DONE);
        pay(15);
        say("A PURPLE GERM FOR MY COLLECTION! I HAD ONLY GREENS. HERE.");
        return;
    }
    if (dd_flag(FL_PELL_FREED) && !dd_flag(FL_PELL_GUARDED) && !dd_flag(FL_KNIGHT_MET)) {
        dd_set(FL_KNIGHT_MET);
        pay(30);
        say("PELL IS HOME! THEY'D LOCKED HIM UP IN LATCHTOWN. YOU'RE A HERO OF TUFTVILLE.");
        return;
    }
    if (!dd_flag(FL_SNARL_DONE)) {
        say("WE WAXKIN OF TUFTVILLE ONCE COMMANDED EVERY BUG IN THE RUG WITH OUR BUZZBOOK. THEN THE SNARL GANG, EAST OF HERE, STOLE ITS COVER STONE. BRING IT BACK?");
        return;
    }
    char b[200];
    snprintf(b, sizeof b, "EIGHT OF OUR WAXLINGS WANDERED OFF INTO THE ROOM. %d ARE HOME. AND IF YOU FIND A PURPLE GERM, I COLLECT THEM.", popcount8(dd_sv.counts[QC_BABIES]));
    say(b);
}

static void talk_fern(void) {
    if (!dd_flag(FL_OUTPOST)) { say("WELCOME TO FERNBY! WE'VE HEARD NOTHING FROM OUR OUTPOST ON TOP OF THE PLANT FOR DAYS. COULD YOU CLIMB UP AND CHECK ON SCOUT BRACKEN?"); return; }
    if (!dd_flag(FL_ROTIFER_DEAD)) {
        dd_set(FL_ROTIFER_ASKED);
        say("A ROTIFER, IN THE WHIRLPOOL JUST WEST OF TOWN? NO WONDER NOBODY COMES BACK. PLEASE, GET RID OF IT. STOMP IT AND IT GAPES; HIT IT THEN.");
        return;
    }
    if (!dd_flag(FL_ROTIFER_PAID)) {
        dd_set(FL_ROTIFER_PAID);
        pay(50);
        say("THE ROTIFER IS GONE! THE WEST ROAD IS OPEN! ALL OF FERNBY THANKS YOU.");
        return;
    }
    int n = popcount8(dd_sv.counts[QC_BUBBLES]);
    char b[160];
    snprintf(b, sizeof b, "SOME OF OUR FOLK FLOATED OFF IN BUBBLES. POP THEM FREE IF YOU SEE THEM. %d OF 4 ARE BACK.", n);
    dd_set(FL_BUBBLES_ASKED);
    say(b);
}

static void talk_lichen(void) {
    if (dd_flag(FL_LETTER_DONE)) { say("SORREL AND I ARE TO BE MARRIED! IN SPRING. WELL, WHEN THE WATERING CAN COMES."); return; }
    if (carrying(O_LETTER) && carried_param() == 1) {
        take_carried();
        dd_set(FL_LETTER_DONE);
        pay(30);
        say("A REPLY FROM SORREL! ... SHE SAYS YES! OH, YOU WONDERFUL POSTIE. TAKE ALL THIS.");
        return;
    }
    if (!dd_flag(FL_LETTER1)) {
        dd_set(FL_LETTER1);
        int j = dd_add_obj(O_LETTER, dd_p.x + 8, dd_p.y);
        if (j >= 0) dd_ent[j].param = 0;
        say("PSST. WOULD YOU CARRY THIS LETTER TO SORREL IN LOAMTON, IN THE EAST POT? DON'T READ IT. IT'S... PERSONAL.");
        return;
    }
    if (!dd_obj_exists(O_LETTER, -1) && !dd_flag(FL_LETTER2)) {
        int j = dd_add_obj(O_LETTER, dd_p.x + 8, dd_p.y);
        if (j >= 0) dd_ent[j].param = 0;
        say("YOU LOST IT? HERE, I WROTE IT OUT AGAIN. SORREL, LOAMTON, EAST POT.");
        return;
    }
    say("HAS SORREL WRITTEN BACK YET?");
}

static void talk_sorrel(void) {
    if (carrying(O_LETTER) && carried_param() == 0) {
        dd_ent[dd_carry].param = 1;
        dd_set(FL_LETTER2);
        say("A LETTER FROM LICHEN? ... OH MY. OH MY MY. WAIT THERE. (SHE SCRIBBLES A REPLY AND HANDS IT BACK.) TAKE THIS TO FERNBY, QUICKLY!");
        return;
    }
    if (dd_flag(FL_LETTER2) && !dd_flag(FL_LETTER_DONE) && !dd_obj_exists(O_LETTER, -1)) {
        int j = dd_add_obj(O_LETTER, dd_p.x + 8, dd_p.y);
        if (j >= 0) dd_ent[j].param = 1;
        say("YOU DROPPED MY REPLY? HERE'S ANOTHER. TO LICHEN, IN FERNBY.");
        return;
    }
    say("LOAMTON IS LOVELY THIS TIME OF YEAR. ALL THIS DAMP.");
}

static void talk_mason(void) {
    int n = popcount8(dd_sv.counts[QC_GLUE]);
    if (carrying(O_GLUE)) {
        int k = carried_param();
        take_carried();
        if (k >= 0 && k < 3) dd_sv.counts[QC_GLUE] |= (uint8_t)(1u << k);
        pay(10);
        say("A GLUE BLOCK! JUST THE THING FOR A NEW ROOF. THANK YOU.");
        return;
    }
    dd_set(FL_STICKY_ASKED);
    char b[160];
    snprintf(b, sizeof b, "WE BUILD WITH GLUE BLOCKS, BUT THEY'RE SCATTERED ALL OVER THE POT. BRING ME THREE AND I'LL PAY TEN APIECE. %d OF 3 SO FAR.", n);
    say(b);
}

static void talk_bracken(void) {
    if (!dd_flag(FL_OUTPOST)) {
        dd_set(FL_OUTPOST);
        say("A VISITOR! TELL THE MAYOR: A ROTIFER HAS MOVED INTO THE WHIRLPOOL WEST OF FERNBY. NOTHING GETS PAST IT, NOT EVEN THE SUPPLY SNAILS. I'VE BEEN STUCK UP HERE A WEEK.");
        dd_autosave();
        return;
    }
    say("ALL QUIET ON TOP OF THE PLANT. YOU CAN SEE THE WHOLE SHELF FROM HERE, AND THE CLOCK.");
}

static void talk_peat(void) {
    if (carrying(O_CRATE)) {
        take_carried();
        dd_set(FL_CRATE_DONE);
        pay(50);
        say("OUR SHIPMENT! THE BUZZER THAT CARRIED IT CRASHED ON THE COIN PILE BY THE MONEY BOX, AND WE'D GIVEN IT UP. YOU'VE EARNED THIS.");
        return;
    }
    if (dd_flag(FL_CRATE_DONE)) { say("LOAMTON STANDS READY, THANKS TO YOU."); return; }
    say("OUR SUPPLY CRATE WENT DOWN WITH A BUZZER, SOMEWHERE ON THE COINS BY THE MONEY BOX. BRING IT HOME AND THERE'S FIFTY GLINTS IN IT FOR YOU.");
}

static void talk_smith(void) {
    if (dd_flag(FL_SMITH_DONE)) { say("THAT PLATE WILL NEVER DENT. TRUST A SMITH."); return; }
    if (carrying(O_HARD)) {
        take_carried();
        dd_set(FL_SMITH_DONE);
        say("A HARD BLOCK! STAND BACK... CLANG... CLANG... THERE. A PLATE FOR YOUR DOG. NOTHING WILL HURT HIM NOW, NOT EVEN BEING THROWN AT THINGS.");
        dd_give_upgrade(U_PLATE);
        return;
    }
    say("BRING ME A HARD BLOCK, THE KIND THE WAXKIN SELL IN TUFTVILLE, AND I'LL BEAT IT INTO ARMOUR FOR THAT DOG OF YOURS.");
}

static void talk_wriggla(void) {
    if (carrying(O_EGG)) {
        take_carried();
        dd_set(FL_EGG_DONE);
        dd_set(FL_WORM_FRIENDS);
        say("MY EGG! MY BEAUTIFUL EGG! THE RED LANCERS SNATCHED IT AND I HAVE NOT SAID A WORD SINCE. WORMWOOD IS YOURS, FRIEND. EVERY WOODWORM DOOR WILL OPEN FOR YOU.");
        dd_autosave();
        return;
    }
    if (dd_flag(FL_WORM_FRIENDS)) { say("BEHIND THE WOODWORM DOOR IN THE LAST STUD LIVES THE GREAT EARWIG. NOT EVEN MY WORKERS GO THERE."); return; }
    say("(THE QUEEN SAYS NOTHING. HER ATTENDANTS WHISPER: THE RED LANCERS TOOK HER EGG TO THEIR DEN IN THE WEST POT, AND SHE HAS NOT SPOKEN SINCE.)");
}

static void talk_borer(void) {
    if (carrying(O_TWIG)) {
        int k = carried_param();
        take_carried();
        if (k >= 0 && k < 3) dd_sv.counts[QC_TWIGS] |= (uint8_t)(1u << k);
        if (popcount8(dd_sv.counts[QC_TWIGS]) >= 3 && !dd_flag(FL_TWIG_DONE)) {
            dd_set(FL_TWIG_DONE);
            pay(15);
            say("THREE GREEN TWIGS! A FEAST FOR THE WHOLE BURROW. HERE, WITH OUR THANKS.");
        } else say("GREEN WOOD! ONE MORE FOR THE POT. BRING ME ALL THREE FROM THE EAST PLANT.");
        return;
    }
    if (dd_flag(FL_TWIG_DONE)) { say("WE'LL BE EATING FOR WEEKS."); return; }
    say("THIS DRY OLD WALL WOOD IS NO FOOD FOR A GROWING WORM. BRING ME GREEN TWIGS, THREE OF THEM. THEY GROW ON THE LEAVES OF THE EAST PLANT.");
}

static void talk_knot(void) {
    if (!dd_flag(FL_WORM_FRIENDS)) { say("SHOP'S SHUT. NO STRANGERS SINCE THE QUEEN WENT QUIET."); return; }
    say("FRIEND OF THE QUEEN, FRIEND OF KNOT! JARS, AXES, AND A FEATHER CHARM FOR THOSE LONG FALLS.");
}

static void talk_generic_quest(int sub) {
    switch (sub) {
    case N_ONEEYE:
        if (dd_flag(FL_BLUEEYE_DONE)) { say("I CAN SEE IN STEREO! EVERYTHING HAS A FRONT AND A BACK NOW."); return; }
        if (carrying(O_BLUEEYE)) { take_carried(); dd_set(FL_BLUEEYE_DONE); say("A BLUE EYE BEAD, PERFECT! ...THERE. TWO EYES. HERE, A HALF BUTTON FOR YOUR HEART."); dd_give_upgrade(U_HEART0 + 7); return; }
        say("I WAS BORN WITH ONE EYE, LIKE ALL SPORELINGS, BUT I'VE ALWAYS WANTED TWO. THERE'S A BLUE BEAD IN A CAVE UNDER THE TOY TRAIN'S ROOF. IT WOULD DO NICELY.");
        return;
    case N_SEEDKEEPER:
        if (dd_flag(FL_SEED_DONE)) { say("THE SEED SLEEPS IN THE VAULT. ONE DAY, A FOREST."); return; }
        if (carrying(O_SEED)) { take_carried(); dd_set(FL_SEED_DONE); say("A MOLD SEED! FOR THE SEED VAULT. TAKE THIS HEART BUTTON, WE KEEP THEM FOR JUST SUCH A DAY."); dd_give_upgrade(U_HEART0 + 4); return; }
        say("I KEEP THE SEED VAULT OF TICKBURG. WE HAVE NO MOLD SEED. THROW A MOLD FRUIT AND IT SPLITS INTO ONE. THE MOLD PATCH IS IN THE EAST POT'S SOIL.");
        return;
    case N_GILL:
        if (dd_flag(FL_REDEGG_DONE)) { say("THE RED EGG HATCHED INTO... A VERY CONFUSED LANCER. HE'S OURS NOW."); return; }
        if (carrying(O_REDEGG)) { take_carried(); dd_set(FL_REDEGG_DONE); say("A RED EGG FROM THE LANCERS' NEST! WE'LL RAISE IT GENTLE. HERE, A HEART BUTTON."); dd_give_upgrade(U_HEART0 + 3); return; }
        say("THE RED LANCERS NEST IN A CAVE AT THE EAST END OF THE EAST POT. BRING ME ONE OF THEIR EGGS AND WE'LL RAISE A LANCER WITH MANNERS.");
        return;
    case N_MOREL:
        if (dd_has(U_STEW)) { say("THE STEW STAYS WITH YOU. NO SLIME OR ACID WILL SING YOU NOW."); return; }
        if (carrying(O_JARSLIME)) { take_carried(); dd_set(FL_MOREL_DONE); say("PINK SLIME! THE SECRET INGREDIENT. SIMMER, STIR... SPORE STEW! EAT UP. SLIME AND ACID WON'T HURT YOU AFTER THIS."); dd_give_upgrade(U_STEW); return; }
        say("THE BEST COOK IN THE ROOM, ON THE LOWEST COIN IN THE PILE. I NEED PINK SLIME FOR MY STEW. IT SEEPS BETWEEN THE WALLS. BRING IT IN A JAR.");
        return;
    case N_RATCHET:
        if (dd_flag(FL_CATFOOD_DONE)) { say("HER MAJESTY PURRED! FIRST TIME IN YEARS."); return; }
        if (carrying(O_CATFOOD)) { take_carried(); dd_set(FL_CATFOOD_DONE); pay(15); say("REAL CAT FOOD! FROM GRANNY THIMBLE'S, I CAN SMELL IT. THE QUEEN WILL BE IN A GOOD MOOD FOR A WEEK."); return; }
        say("HER MAJESTY TURNS HER NOSE UP AT EVERYTHING I COOK. SHE ONLY EVER LIKED THE CAT FOOD GRANNY THIMBLE KEPT.");
        return;
    case N_HIGHLUMEN:
        if (dd_flag(FL_LUMEN)) { say("GO IN LIGHT, HONOURED LUMEN. THE WARDEN WILL DO AS YOU ASK."); return; }
        if (dd_flag(FL_SHRINE)) {
            dd_set(FL_LUMEN);
            say("YOU HAVE BOWED AT THE LAMP SHRINE. I NAME YOU AN HONOURED LUMEN OF GLIMMER! TAKE THIS SCROLL OF OUR LIGHT; THE WAXKIN HAVE LONG ASKED FOR ONE.");
            int j = dd_add_obj(O_SCROLL, dd_p.x + 10, dd_p.y);
            (void)j;
            dd_autosave();
            return;
        }
        say("WE LUMEN LIVE IN THE LIGHT OF THE GREAT LAMP. ONLY A PILGRIM WHO HAS BOWED AT THE SHRINE ON TOP OF THE SHADE MAY BE ONE OF US.");
        return;
    case N_WARDEN:
        if (dd_flag(FL_LAMP_OFF)) { say("THE LAMP SLEEPS. SHALL IT WAKE? NO. NOT YET."); return; }
        if (!dd_flag(FL_LUMEN)) { say("I KEEP THE SWITCH OF THE GREAT LAMP. ONLY FOR A LUMEN WOULD I EVER TOUCH IT."); return; }
        dd_ask("THE WICK WARDEN", "HONOURED LUMEN. YOU WISH THE GREAT LAMP OFF, SO THE OLD FILAMENT MAY SPEAK?", cb_warden);
        return;
    case N_TABITHA:
        if (dd_flag(FL_SPROCKET_DEAD)) { say("MY KNIGHT... FINE. THE DOOR IS YOURS, AND THE MUTT. SHOO."); return; }
        if (dd_flag(FL_PAID_QUEEN)) { say("SIR SPROCKET WILL SEE YOU OUT."); return; }
        dd_set(FL_MET_QUEEN);
        if (dd_sv.glints >= 500) dd_ask("QUEEN TABITHA", "SO THE LITTLE GIRL WANTS OUT. MY DOOR, MY PRICE: 500 GLINTS. PAY?", cb_queen);
        else say("SO THE LITTLE GIRL WANTS OUT. MY DOOR, MY PRICE: 500 GLINTS. COME BACK WHEN YOU'RE RICH, DEAR.");
        return;
    case N_MAGE:
        if (dd_flag(FL_THRONE_OPEN)) { say("THE THRONE HALL IS OPEN. MIND YOUR MANNERS."); return; }
        dd_ask("THE TIN MAGE", "THE THRONE HALL IS SHUT TO THE PUBLIC. FIFTY GLINTS AND I'LL OPEN THE GATE. WELL?", cb_mage);
        return;
    case N_KNIGHT:
        if (dd_flag(FL_PELL_FREED)) { say("THE PRISONER'S GONE! THE QUEEN WILL HAVE MY BOLTS."); return; }
        if (dd_flag(FL_PELL_GUARDED)) { say("STEADY. HE'S NOT GOING ANYWHERE."); return; }
        dd_ask("A TIN KNIGHT", "THE PRISONER STAYS. BUT MY FEET ACHE. STAND GUARD WITH ME A WHILE? TEN GLINTS.", cb_knight);
        return;
    case N_PELL:
        if (dd_flag(FL_PELL_GUARDED)) { say("TRAITOR."); return; }
        dd_ask("PELL", "PSST! I'M PELL, FROM TUFTVILLE. THEY CAUGHT ME PEEKING AT THE QUEEN'S TREASURE. OPEN MY CELL?", cb_pell);
        return;
    case N_SNUFF:
        if (dd_flag(FL_SCROLL_DONE)) { say("I'VE BEEN READING THE LUMEN SCROLL. I THINK I'M A PILGRIM NOW."); return; }
        say("SNARL IS EAST, PAST THE BIG POT'S SAUCER. THEY'RE MEAN. AND THE THIMBLE WEST OF HERE IS GRANNY'S. SHE'S NICE.");
        return;
    case N_SMUDGE:
        say(dd_flag(FL_SNARL_DONE) ? "YOU STOLE OUR STONE! WELL. WE STOLE IT FIRST, BUT STILL." : "THIS IS SNARL. WE KEEP WHAT WE TAKE. GET LOST, GIANT.");
        return;
    case N_PILGRIM:
        say(dd_flag(FL_SHRINE) ? "YOU'VE BOWED AT THE SHRINE. GO DOWN TO GLIMMER, UNDER OUR FEET, AND THE HIGH LUMEN WILL BLESS YOU." :
                                 "EVERY LUMEN MUST BOW AT THE LAMP SHRINE ONCE. IT'S RIGHT HERE ON THE SHADE. PRESS " GLYPH_UP " AT IT.");
        return;
    case N_NIB:
        dd_set(FL_MET_NIB);
        say("YOU FOUND ME. I AM NIB. I LIVE IN THE KEYHOLE AND I TURN THE LOCK FROM INSIDE; THE CAT ONLY SITS ON TOP AND COUNTS GLINTS.");
        say("THE ROOM IS OUT OF BALANCE. TOO MUCH TAKEN, TOO MANY MINUTES SKIPPED. TOCK, THE SPIDER IN THE CLOCK, CAN WIND IT RIGHT, FOR A THOUSAND GLINTS. TELL HIM NIB SENT YOU.");
        dd_autosave();
        return;
    case N_HOPPERSAGE:
        dd_set(FL_GRASS_SEEN);
        say("I HAVE WATCHED THAT BLOB FOR A HUNDRED SUMMERS. IT EATS, IT ROLLS, IT NEVER ASKS WHY. SOME DAYS I THINK IT KNOWS SOMETHING I DON'T.");
        return;
    case N_SPRIG: say("FERNBY'S FINEST: HEART BOXES, QUAKE BLOCKS, AND A SPRING BEAN FOR JUMPING HIGHER. " GLYPH_UP "+" GLYPH_A ", LIKE A FROG."); return;
    case N_CLOD: say("STONE TABLETS, HEART BOXES, AND A DROP BANGLE. TAP " GLYPH_DOWN " TWICE ON A LEDGE AND DOWN YOU GO."); return;
    case N_TAPER: say("HEART BOXES, HARD BLOCKS, AND THE SECOND SHRINK TONIC, FOR GOING DEEPER THAN DEEP. WAXKIN QUALITY."); return;
    default: break;
    }
    say("...");
}

void dd_native_hint(int ent, char *buf, int n) {
    const Ent *e = &dd_ent[ent];
    uint32_t h = (uint32_t)e->param;
    /* the nearest town in this strip, if any */
    int here = (int)(e->x / (CHUNK_W * DD_TS)), best = -1, bd = 999;
    if (dd_lv.d.kind == LV_STRIP)
        for (int c = 0; c < dd_lv.d.n; c++)
            if (dd_lv.town[c] >= 0 && iabs(c - here) < bd) { bd = iabs(c - here); best = c; }
    if (best >= 0 && h % 3 != 0) {
        snprintf(buf, (size_t)n, "%s IS %d SPECK%s %s OF HERE.", dd_town_name(dd_lv.town[best]), bd, bd == 1 ? "" : "S", best < here ? "WEST" : "EAST");
        return;
    }
    static const char *H[] = {
        "THEY SAY THE SECOND GRIP MITT LIES IN THE BOOKS, AND ANOTHER BETWEEN THE WALLS.",
        "A PAIR OF KICK CLOGS SITS IN THE PENCIL ON TOP OF THE BOOKSHELF.",
        "THE DANGEROUS CAVES HOLD BIG GLINTS, TWO APIECE. MIND THE THORNS.",
        "A SPRING BEAN AS STRONG AS TWO GROWS ON THE LAMP AND ON THE CLOCK.",
        "EVERY TREASURE DOWN HERE EXISTS TWICE. TAKE ONE AND THE OTHER TURNS TO GLINTS.",
        "THE WAXLINGS OF TUFTVILLE LOST THEIR LITTLE ONES ALL OVER THE ROOM.",
        "A PEP EGG MAKES EVERY THROW HIT HARDER. THERE ARE EIGHT.",
        "THE STONE OWL ON THE BOTTOM SHELF WAKES FOR A GOOD HARD QUAKE.",
        "IF YOU CAN SHRINK DEEPER, DO. THE MOTES HIDE THINGS TOO.",
        "BETWEEN THE WALLS LIVE THE WOODWORMS. THEIR QUEEN HAS LOST SOMETHING.",
    };
    snprintf(buf, (size_t)n, "%s", H[(h / 3) % ARRAY_LEN(H)]);
}

void dd_talk(int ent) {
    talk_ent = ent;
    int sub = dd_ent[ent].sub;
    dd_ent[ent].dir = dd_p.x < dd_ent[ent].x ? 0 : 1;
    switch (sub) {
    case N_GRANNY: talk_granny(); break;
    case N_CRUMB: talk_crumb(); break;
    case N_VOLT: talk_volt(); break;
    case N_SILK: talk_silk(); break;
    case N_FILAMENT: talk_filament(); break;
    case N_TOCK: talk_tock(); break;
    case N_OLDCAP: talk_oldcap(); break;
    case N_MOTHERFLUFF: talk_mother(); break;
    case N_TUFTY: talk_tufty(); break;
    case N_PUFFIN: talk_puffin(); break;
    case N_INKY: talk_inky(); break;
    case N_COWPOKE: talk_cowpoke(); break;
    case N_WEEPY: talk_weepy(); break;
    case N_SOLDIER: talk_soldier(); break;
    case N_SHREW: talk_shrew(); break;
    case N_SPINNER: talk_spinner(); break;
    case N_TALLOW: talk_tallow(); break;
    case N_FERN: talk_fern(); break;
    case N_LICHEN: talk_lichen(); break;
    case N_SORREL: talk_sorrel(); break;
    case N_MASON: talk_mason(); break;
    case N_BRACKEN: talk_bracken(); break;
    case N_PEAT: talk_peat(); break;
    case N_SMITH: talk_smith(); break;
    case N_WRIGGLA: talk_wriggla(); break;
    case N_BORER: talk_borer(); break;
    case N_KNOT: talk_knot(); break;
    case N_NATIVE: {
        char b[200];
        dd_native_hint(ent, b, sizeof b);
        static const char *FOLK[M_COUNT] = {"A LOCAL", "A WOODWORM", "A CHALK MITE", "A GLAZE SNAIL", "A MOSSFOLK", "A WAXKIN",
                                            "A PAGE MITE", "A SPORELING", "A LUMEN", "A DUST KIT", "A CARD PAWN", "A FUR NIT", "A MOTE", "A GRIT BUG", "A MOSSFOLK", "A WAXKIN"};
        int m = iclamp(dd_ent[ent].param2, 0, M_COUNT - 1);
        dd_say(FOLK[m], b);
        break;
    }
    case N_BUBBLE: dd_npc_touch(ent); break;
    default: talk_generic_quest(sub); break;
    }
    dd_autosave();
    (void)cb_letter;
}

/* bubbles pop when you touch them */
void dd_npc_touch(int ent) {
    Ent *e = &dd_ent[ent];
    if (e->sub != N_BUBBLE || !e->alive) return;
    int k = e->param;
    e->alive = 0;
    if (k >= 0 && k < 4) dd_sv.counts[QC_BUBBLES] |= (uint8_t)(1u << k);
    dd_burst(e->x + 4, e->y + 6, C_ICE, 12);
    sfx_play_name("dd_pop");
    pay(5);
    dd_say("A MOSSFOLK", "POP! THANK YOU! I'VE BEEN FLOATING ROUND THIS POT FOR DAYS. I'LL WALK HOME TO FERNBY FROM HERE.");
}

/* creatures that count for quests */
void dd_on_kill(int sub, int ent) {
    const Ent *e = &dd_ent[ent];
    switch (sub) {
    case F_NIP:
        dd_sv.counts[QC_NIPS] |= (uint8_t)(1u << (e->param & 7));
        if (dd_sv.counts[QC_NIPS] == 0xFF && !dd_flag(FL_DASH_CLEAN)) {
            dd_set(FL_DASH_CLEAN);
            dd_say("DASH", "(FROM FAR ABOVE, A HAPPY WOOF.) NO MORE ITCH! AND DASH'S NOSE IS WORKING AGAIN: HE'LL SNIFF OUT TREASURE WHEREVER YOU TAKE HIM.");
        }
        break;
    case F_MITE:
        if (dd_lv.d.kind == LV_SPECIAL && dd_lv.d.id == SP_PUFFFUR) {
            dd_sv.counts[QC_MITES] |= (uint8_t)(1u << (e->param & 7));
            if (dd_sv.counts[QC_MITES] == 0x3F && !dd_flag(FL_PUFF_CLEAN)) {
                dd_set(FL_PUFF_CLEAN);
                dd_say("PUFFIN", "(A MUFFLED SNEEZE FROM OUTSIDE.) ACHOO! ...OH! THE ITCH IS GONE!");
            }
        }
        break;
    case F_PAPERFISH:
        if (e->param > 0) {
            dd_sv.counts[QC_PAPERFISH] |= (uint8_t)(1u << ((e->param - 1) & 7));
            if (popcount8(dd_sv.counts[QC_PAPERFISH]) == 6) dd_set_message("THAT'S ALL SIX PAPERFISH. INKY WILL BE PLEASED");
        }
        break;
    case F_TINMAGE:
        if (!dd_flag(FL_MAGE_BEATEN)) {
            dd_set(FL_MAGE_BEATEN);
            pay(5);
            dd_set_message("THE TIN MAGE CLATTERS APART. +5");
        }
        break;
    }
    dd_autosave();
}

void dd_on_boss_dead(int sub) {
    switch (sub) {
    case F_ROTIFER:
        dd_set(FL_ROTIFER_DEAD);
        dd_set_message("THE ROTIFER SPINS AWAY INTO THE MUD!");
        music_play(DD_MUS[MU_MICRO]);
        break;
    case F_EARWIG:
        dd_set(FL_EARWIG_DEAD);
        dd_give_upgrade(U_SHELL);
        dd_say("", "THE GREAT EARWIG CURLS UP AND IS STILL. ITS SHELL SPLITS INTO A VEST THAT FITS YOU PERFECTLY: THE SHELL VEST. HITS HURT HALF AS MUCH.");
        music_play(DD_MUS[MU_WALLS]);
        break;
    case F_SIEGE:
        dd_set(FL_SIEGE_DEAD);
        dd_add_obj(O_BIGBANG, (float)(151 * DD_TS), (float)(13 * DD_TS - 8));
        dd_set_message("THE SIEGE ENGINE FALLS! SOMETHING ROLLS OUT");
        music_play(DD_MUS[MU_SIEGE]);
        break;
    case F_SPRING_CORE:
        dd_set(FL_SPROCKET_DEAD);
        dd_say("", "SPROINNNG! THE MAINSPRING SNAPS. EVERY GEAR IN SIR SPROCKET STOPS AT ONCE, AND THE KNIGHT FALLS APART AROUND YOU.");
        dd_say_more("DASH COMES BOUNDING OUT FROM UNDER THE THRONE. THE GREAT DOOR SWINGS OPEN.");
        dd_start_ending(dd_flag(FL_BALANCE));
        break;
    }
    dd_autosave();
}
