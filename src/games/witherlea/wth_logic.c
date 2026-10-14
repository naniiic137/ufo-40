/* WITHERLEA - clicks: looking, taking, using things on the house and on
 * each other, the exits and the maze, noise. Every item has one job (a few
 * have two) and leaves the bag when its last job is done, leaving a hole
 * that the next thing picked up fills. */
#include "wth.h"
#include <stdarg.h>

/* ------------------------------------------------------------------ */
/* the bag                                                              */

bool wth_has(int it) { return wth_slot_of(it) >= 0; }

int wth_slot_of(int it) {
    for (int i = 0; i < WTH_BAG; i++)
        if (wth.s.bag[i] == it) return i;
    return -1;
}

int wth_give(int it) {
    if (it <= 0 || it >= I_COUNT) return -1;
    if (it == I_DIARY && wth_has(I_DIARY)) return wth_slot_of(I_DIARY);
    for (int i = 0; i < WTH_BAG; i++)
        if (!wth.s.bag[i]) {
            wth.s.bag[i] = (uint8_t)it;
            sfx_play_name("wth_take");
            return i;
        }
    return -1;
}

void wth_take_away(int it) {
    int s = wth_slot_of(it);
    if (s >= 0) wth.s.bag[s] = 0;
    if (wth.held == it) wth.held = 0;
}

int wth_bag_count(void) {
    int n = 0;
    for (int i = 0; i < WTH_BAG; i++) n += wth.s.bag[i] != 0;
    return n;
}

int wth_page_count(void) {
    int n = 0;
    for (int i = 0; i < WTH_PAGES; i++) n += (wth.s.pages >> i) & 1;
    return n;
}

/* ------------------------------------------------------------------ */
/* the text panel                                                       */

void wth_say(const char *text) {
    char lines[24][UI_WRAP_LEN];
    int n = ui_wrap(text, WTH_PW - 12, false, lines, 24);
    if (n > 24) n = 24;
    for (int i = 0; i < n; i++) {
        int page = wth.msg_n + i / WTH_TLINES;
        if (page >= WTH_MSG_PAGES) break;
        if (i % WTH_TLINES == 0)
            for (int l = 0; l < WTH_TLINES; l++) wth.msg[page][l][0] = 0;
        memcpy(wth.msg[page][i % WTH_TLINES], lines[i], UI_WRAP_LEN); /* both UI_WRAP_LEN long */
        wth.msg[page][i % WTH_TLINES][UI_WRAP_LEN - 1] = 0;
    }
    wth.msg_n = imin(WTH_MSG_PAGES, wth.msg_n + (n + WTH_TLINES - 1) / WTH_TLINES);
}

void wth_sayf(const char *fmt, ...) {
    char buf[400];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    wth_say(buf);
}

bool wth_talking(void) { return wth.msg_page < wth.msg_n; }

/* ------------------------------------------------------------------ */
/* the spots                                                            */

static int page_at(int spot) {
    for (int i = 0; i < WTH_PAGES; i++)
        if (WTH_PAGE_SPOT[i] == spot) return i;
    return -1;
}

static bool page_found_at(int spot) {
    int p = page_at(spot);
    return p < 0 || ((wth.s.pages >> p) & 1);
}

bool wth_spot_live(int s) {
    if (s < 0 || s >= S_COUNT) return false;
    switch (s) {
    case S_CELL_DOOR: return !wth_flag(F_CELL_OPEN);
    case S_CELL_EXIT: return wth_flag(F_CELL_OPEN);
    case S_HALL_STAIRS_DARK: return !wth_flag(F_STAIRS_LIT);
    case S_HALL_STAIRS: return wth_flag(F_STAIRS_LIT);
    case S_GAR_DOOR: return !wth_flag(F_GARAGE_OPEN);
    case S_GAR_OUT: return wth_flag(F_GARAGE_OPEN);
    case S_DRV_GARAGE: return !wth_flag(F_GARAGE_OPEN);
    case S_DRV_GARAGE_IN: return wth_flag(F_GARAGE_OPEN);
    case S_GDN_IVY: return !wth_flag(F_IVY_CUT);
    case S_GDN_ARCH: return wth_flag(F_IVY_CUT);
    case S_GDN_SHEDDOOR: return !wth_flag(F_SHED_OPEN);
    case S_GDN_SHED: return wth_flag(F_SHED_OPEN);
    case S_GDN_HANG: return !wth_flag(F_ROPE_CUT);
    case S_GDN_FALLEN: return wth_flag(F_ROPE_CUT);
    case S_DRV_STORM: return !wth_flag(F_STORM_OPEN);
    case S_DRV_CELLAR: return wth_flag(F_STORM_OPEN);
    case S_CRT_GATE: return !wth_flag(F_GATE_OPEN);
    case S_CRT_MAZE: return wth_flag(F_GATE_OPEN);
    case S_LAN_MASTER_SHUT: return !wth_flag(F_MASTER_OPEN);
    case S_LAN_MASTER: return wth_flag(F_MASTER_OPEN);
    case S_LAN_HATCH: return !wth_flag(F_HATCH_DOWN);
    case S_LAN_ATTIC: return wth_flag(F_HATCH_DOWN);
    case S_CEL_DOOR: return !wth_flag(F_IRONDOOR_OPEN);
    case S_CEL_SPORE: return wth_flag(F_IRONDOOR_OPEN);
    case S_PAR_PIG: return !wth_flag(F_PIG_TAKEN);
    case S_SCU_SODA: return !wth_flag(F_SODA_TAKEN);
    case S_GAR_SPANNER: return !wth_flag(F_SPANNER_TAKEN);
    case S_GAR_TWINE: return !wth_flag(F_TWINE_TAKEN);
    case S_SHE_SCREW: return !wth_flag(F_SCREW_TAKEN);
    case S_SHE_SHEARS: return !wth_flag(F_SHEARS_TAKEN);
    case S_SHE_SAW: return !wth_flag(F_SAW_TAKEN);
    case S_SHE_SPADE: return !wth_flag(F_SPADE_TAKEN);
    case S_CHI_DOLL: return !wth_flag(F_DOLL_TAKEN);
    case S_SCU_DIARY: case S_SHE_DIARY: case S_STU_DIARY: case S_BAT_DIARY: case S_NUR_DIARY: case S_CHI_DIARY:
    case S_ATT_DIARY: case S_CEL_DIARY:
        return !page_found_at(s);
    default: return true;
    }
}

int wth_spot_at(int x, int y) {
    int best = -1, best_area = 1 << 30;
    for (int s = 0; s < S_COUNT; s++) {
        const WthSpot *p = &WTH_SPOT[s];
        if (p->room != wth.s.room || !wth_spot_live(s)) continue;
        if (x < p->x || y < p->y || x >= p->x + p->w || y >= p->y + p->h) continue;
        int area = p->w * p->h;
        if (area < best_area) { best = s; best_area = area; }
    }
    return best;
}

int wth_exit_dest(int s) {
    const WthSpot *p = &WTH_SPOT[s];
    if (p->kind != HS_EXIT || !wth_spot_live(s)) return -1;
    if (s == S_DRV_CELLAR && !wth_flag(F_STAIRS_LIT)) return -1;
    return p->dest;
}

/* ------------------------------------------------------------------ */
/* small verbs                                                          */

void wth_noise(bool maze) {
    wth.noise_count++;
    wth.last_noise = maze ? 2 : 1;
    if (wth_flag(F_KEEPER_DEAD)) return;
    if (wth.k.state != E_NONE) return; /* one visit at a time: noises chain into it */
    wth.k.state = E_COMING;
    wth.k.t = maze ? WTH_MAZE_NOISE_T : WTH_NOISE_T;
    wth.k.noise_maze = maze;
}

/* take a thing once: the flag says it's gone from the house */
static bool take(int flag, int it, const char *text) {
    if (wth_flag(flag)) return false;
    wth_set(flag);
    wth_give(it);
    wth_say(text);
    return true;
}

static bool take_page(int spot) {
    int p = page_at(spot);
    if (p < 0 || ((wth.s.pages >> p) & 1)) return false;
    wth.s.pages |= 1u << p;
    wth_give(I_DIARY);
    sfx_play_name("wth_page");
    wth_sayf("A DIARY PAGE: %s. (%d OF %d.) YOU ADD IT TO THE OTHERS.", WTH_PAGE[p].date, wth_page_count(), WTH_PAGES);
    return true;
}

static void nothing(void) { wth_say("THAT DOESN'T DO ANYTHING."); }

/* the long hook and the machine oil each have two jobs */
static void hook_job_done(void) {
    if (wth_flag(F_POND_DONE) && wth_flag(F_HATCH_DOWN)) wth_take_away(I_LONGHOOK);
}
static void oil_job_done(void) {
    if (wth_flag(F_OIL_USED1)) wth_take_away(I_OIL);
    else wth_set(F_OIL_USED1);
}

static void floor_put(int room, int it) {
    for (int i = 0; i < WTH_FLOOR; i++)
        if (!wth.s.floor[room][i]) { wth.s.floor[room][i] = (uint8_t)it; return; }
}

static const char *hide_look(int s, bool gone) {
    switch (s) {
    case S_PAR_SCREEN: return gone ? "THE FOLDING SCREEN LIES SLASHED TO RIBBONS. NO ONE CAN HIDE BEHIND IT NOW."
                                   : "A TALL FOLDING SCREEN PAINTED WITH HERONS. YOU COULD HIDE BEHIND IT, IF YOU HAD TO.";
    case S_SMO_ALCOVE: return gone ? "THE ALCOVE CURTAIN HAS BEEN TORN DOWN AND HACKED APART."
                                   : "A CURTAINED ALCOVE, DEEP ENOUGH TO STAND IN. A GOOD PLACE TO HIDE.";
    case S_GUE_BED: return gone ? "THE BED HAS BEEN TIPPED ON ITS SIDE AND THE SPACE UNDER IT CHOPPED FULL OF HOLES."
                                : "A HIGH OLD BED WITH A LONG VALANCE. THERE'S ROOM UNDERNEATH FOR SOMEONE TO HIDE.";
    case S_NUR_CHEST: return gone ? "THE TOY CHEST IS SMASHED OPEN. TOYS EVERYWHERE, BEHEADED."
                                  : "A BIG TOY CHEST. YOU COULD CURL UP INSIDE IT AND PULL THE LID DOWN.";
    case S_GDN_LAUREL: return "A THICK LAUREL HEDGE AGAINST THE GLASSHOUSE. THERE'S A HOLLOW INSIDE IT, ROOM FOR ONE.";
    case S_ATT_DUMMIES: return "DRESSMAKER'S DUMMIES IN A HUDDLE, DRAPED IN SHEETS. ONE MORE SHAPE WOULDN'T SHOW.";
    default: return "";
    }
}

int wth_hide_one_use_flag(int spot) {
    switch (spot) {
    case S_PAR_SCREEN: return F_HIDE_SCREEN_GONE;
    case S_SMO_ALCOVE: return F_HIDE_ALCOVE_GONE;
    case S_GUE_BED: return F_HIDE_BED_GONE;
    case S_NUR_CHEST: return F_HIDE_CHEST_GONE;
    default: return -1;
    }
}

/* ------------------------------------------------------------------ */
/* putting the gems in the hares' eyes                                  */

static bool is_gem(int it) { return it == I_GARNET || it == I_JADE || it == I_AMBER || it == I_OPAL; }

static void hare(int i, int held) {
    if (wth_flag(F_GATE_OPEN)) {
        wth_say("THE HARE STARES AT THE OPEN GATE WITH ITS STONE EYE.");
        return;
    }
    if (!held) {
        if (wth.s.hare[i]) {
            wth_give(wth.s.hare[i]);
            wth_sayf("YOU PRISE THE %s BACK OUT OF THE HARE'S EYE.", WTH_ITEM[wth.s.hare[i]].name);
            wth.s.hare[i] = 0;
        } else wth_say("A STONE HARE IN A NICHE, SITTING UP. ITS EYE IS AN EMPTY ROUND SOCKET.");
        return;
    }
    if (!is_gem(held)) { nothing(); return; }
    if (wth.s.hare[i]) { wth_say("THIS HARE ALREADY HAS AN EYE."); return; }
    wth.s.hare[i] = (uint8_t)held;
    wth_take_away(held);
    sfx_play_name("wth_click");
    for (int k = 0; k < 4; k++)
        if (!wth.s.hare[k]) {
            wth_sayf("THE %s FITS THE SOCKET EXACTLY.", WTH_ITEM[held].name);
            return;
        }
    bool right = true;
    for (int k = 0; k < 4; k++) right = right && wth.s.hare[k] == WTH_HARE_GEM[k];
    if (right) {
        wth_set(F_GATE_OPEN);
        sfx_play_name("wth_grind");
        wth_say("THE LAST STONE CLICKS HOME. FOUR HARES LOOK AT YOU WITH COLOURED EYES, AND IN THE CORNER THE IRON "
                "GATE GRINDS OPEN.");
    } else {
        for (int k = 0; k < 4; k++) {
            wth_give(wth.s.hare[k]);
            wth.s.hare[k] = 0;
        }
        wth_say("THE LAST STONE CLICKS HOME... AND ALL FOUR EYES DROP OUT INTO THE GRAVEL. NOT IN THAT ORDER, THEN. "
                "YOU PICK THEM UP.");
    }
}

/* ------------------------------------------------------------------ */
/* the cellar winch                                                     */

static void winch(int held) {
    if (wth_flag(F_IRONDOOR_OPEN)) { wth_say("THE WINCH HOLDS THE DOOR UP. IT CREAKS."); return; }
    if (held == I_GEAR && !wth_flag(F_MECH_GEAR)) {
        wth_set(F_MECH_GEAR);
        wth_take_away(I_GEAR);
        wth_say("THE COG SLIDES ONTO THE SQUARE SPINDLE. IT FITS.");
    } else if (held == I_STEELWIRE && !wth_flag(F_MECH_WIRE)) {
        wth_set(F_MECH_WIRE);
        wth_take_away(I_STEELWIRE);
        wth_say("YOU THREAD THE STEEL WIRE ROUND THE DRUM WHERE THE OLD CABLE SNAPPED.");
    } else if (held == I_SCREWDRIVER && !wth_flag(F_MECH_SCREW)) {
        wth_set(F_MECH_SCREW);
        wth_take_away(I_SCREWDRIVER);
        wth_say("YOU SCREW THE COVER BACK ON, TIGHT, SO THE DRUM SITS STRAIGHT.");
    } else if (held == I_OIL && !wth_flag(F_MECH_OIL)) {
        wth_set(F_MECH_OIL);
        oil_job_done();
        wth_say("YOU OIL EVERY JOINT OF THE WINCH. THE RUST WEEPS BROWN.");
    } else if (held) {
        nothing();
        return;
    } else {
        char buf[300];
        snprintf(buf, sizeof buf, "THE IRON DOOR'S WINCH.%s%s%s%s", wth_flag(F_MECH_GEAR) ? "" : " A COG IS MISSING.",
                 wth_flag(F_MECH_WIRE) ? "" : " THE CABLE HAS SNAPPED.",
                 wth_flag(F_MECH_SCREW) ? "" : " THE COVER HANGS OFF BY ONE SCREW.",
                 wth_flag(F_MECH_OIL) ? "" : " IT'S ALL RUSTED STIFF.");
        wth_say(buf);
        return;
    }
    if (wth_flag(F_MECH_GEAR) && wth_flag(F_MECH_WIRE) && wth_flag(F_MECH_SCREW) && wth_flag(F_MECH_OIL)) {
        wth_set(F_IRONDOOR_OPEN);
        sfx_play_name("wth_grind");
        wth_say("THE WINCH TURNS OVER. CHAIN RUNS, AND THE IRON DOOR GRINDS UP INTO THE WALL. WARM AIR. SWEET AIR.");
    }
}

/* ------------------------------------------------------------------ */
/* A on a spot                                                          */

void wth_click(int s, int held) {
    const WthSpot *sp = &WTH_SPOT[s];
    if (sp->kind == HS_HIDE) {
        int f = wth_hide_one_use_flag(s);
        bool gone = f >= 0 && wth_flag(f);
        if (!gone && wth.k.state != E_NONE && wth.k.state != E_HIDING && wth.k.state != E_CAUGHT) {
            wth_hide_begin(s); /* whatever you hold goes back in the bag */
            return;
        }
        if (held) { nothing(); return; }
        wth_say(hide_look(s, gone));
        return;
    }
    if (!held && sp->look) {
        if (!take_page(s)) wth_say(sp->look);
        return;
    }
    switch (s) {
    /* the cold store */
    case S_CELL_DOOR:
        if (held == I_WIRE) {
            wth_take_away(I_WIRE);
            wth_set(F_CELL_OPEN);
            sfx_play_name("wth_door");
            wth_say("YOU FEED THE HOOKED WIRE UNDER THE DOOR AND FEEL UP THE OTHER SIDE FOR THE BOLT. IT CATCHES. IT "
                    "SLIDES. THE DOOR SWINGS OPEN ON A DARK PASSAGE.");
        } else if (held) wth_say("THAT WON'T GET THE DOOR OPEN.");
        else wth_say("A HEAVY DOOR, BOLTED ON THE OUTSIDE. THERE'S A FINGER'S GAP UNDERNEATH IT.");
        break;
    case S_CELL_VENT:
        if (!wth_flag(F_VENT_OPEN)) {
            if (held == I_SHOEHORN) {
                wth_set(F_VENT_OPEN);
                wth_take_away(I_SHOEHORN);
                sfx_play_name("wth_clank");
                wth_say("YOU WORK THE LIP OF THE SHOEHORN UNDER THE GRILLE AND LEVER. THE RUSTED SCREWS GIVE ALL AT "
                        "ONCE AND THE GRILLE CLATTERS DOWN.");
            } else if (held) nothing();
            else wth_say("A GRILLE OVER AN AIR VENT, HIGH IN THE WALL. THE SCREW HEADS HAVE RUSTED AWAY. THERE'S A "
                         "THIN GAP UNDER ITS EDGE.");
        } else if (held) nothing();
        else if (!take(F_VENT_WIRE, I_WIRE, "YOU REACH INTO THE VENT. A STIFF BENT WIRE, HOOKED AT THE END."))
            if (!take(F_VENT_PAPER, I_NEWSPAPER, "FURTHER IN, A FOLDED SCRAP OF NEWSPAPER."))
                wth_say("NOTHING ELSE IN THE VENT BUT DUST, AND A DRAUGHT FROM SOMEWHERE FAR AWAY.");
        break;
    case S_CELL_STRAW:
        if (held) nothing();
        else if (!take(F_STRAW, I_SHOEHORN, "YOU DIG THROUGH THE STRAW. UNDER IT, PRESSED INTO THE DIRT: A SHOEHORN."))
            wth_say("DAMP STRAW. YOU WOKE UP ON IT.");
        break;
    case S_CELL_TRAY:
        if (held) nothing();
        else if (!take(F_MUG_TAKEN, I_MUG, "A TRAY ON THE FLOOR, AND ON IT A TIN MUG. SOMEONE MEANT YOU TO STAY A WHILE."))
            wth_say("AN EMPTY TRAY.");
        break;
    /* the back passage */
    case S_PAS_COAT:
        if (held) nothing();
        else if (!take(F_COAT, I_PENNIES, "A MAN'S COAT ON A PEG. IN THE POCKET, A FISTFUL OF PENNIES."))
            wth_say("A MAN'S COAT, TOO GOOD FOR THIS HOUSE. THE LABEL INSIDE SAYS FEN WATER BOARD.");
        break;
    case S_PAS_RUG:
        if (held) nothing();
        else if (!take_page(s)) wth_say("A LONG RUNNER RUG. THE PILE IS WORN DOWN TO THE THREADS IN A PATH TO THE COLD STORE.");
        break;
    /* the sickroom */
    case S_SICK_BED:
        if (held) nothing();
        else if (!take_page(s)) wth_say("A NARROW IRON BED. THE SHEETS ARE GREY AND SMELL OF HONEY.");
        break;
    case S_SICK_STAND:
        if (held) nothing();
        else if (!take(F_TORCH_TAKEN, I_TORCH, "IN THE NIGHTSTAND DRAWER: A HAND TORCH. YOU CLICK IT. NOTHING. NO BATTERIES."))
            wth_say("AN EMPTY NIGHTSTAND.");
        break;
    /* the hall */
    case S_HALL_CLOCK:
        if (held == I_COPPERKEY && !wth_flag(F_CLOCK_WOUND)) {
            wth_set(F_CLOCK_WOUND);
            wth_take_away(I_COPPERKEY);
            floor_put(R_HALL, I_GEAR);
            wth_noise(false);
            sfx_play_name("wth_chime");
            wth_say("YOU WIND THE CLOCK. IT STRIKES. AND STRIKES, AND WON'T STOP, ON AND ON THROUGH THE WHOLE HOUSE. "
                    "SOMETHING INSIDE SPRINGS LOOSE AND A COG ROLLS OUT ACROSS THE FLOOR.");
        } else if (held) nothing();
        else if (wth_flag(F_CLOCK_WOUND)) wth_say("THE CLOCK HAS STOPPED STRIKING AT LAST. ITS HANDS HANG STRAIGHT DOWN.");
        else wth_say("A LONGCASE CLOCK, STOPPED AT TEN TO THREE. THERE'S A SQUARE HOLE IN THE DIAL FOR A WINDING KEY.");
        break;
    case S_HALL_UMBRELLA:
        if (held) nothing();
        else if (!take(F_CANE_TAKEN, I_CANE, "AN UMBRELLA STAND WITH NO UMBRELLAS. YOU TAKE THE WALKING CANE."))
            wth_say("AN EMPTY UMBRELLA STAND.");
        break;
    case S_HALL_STAIRS_DARK:
        if (held == I_TORCH && wth_flag(F_TORCH_LOADED)) {
            wth_set(F_STAIRS_LIT);
            wth_take_away(I_TORCH);
            sfx_play_name("wth_click");
            wth_say("YOU SWITCH ON THE TORCH AND POINT IT UP THE STAIRS. A SMALL CIRCLE OF LIGHT. IT WILL HAVE TO DO.");
        } else if (held == I_TORCH) wth_say("THE TORCH HAS NO BATTERIES IN IT.");
        else if (held) nothing();
        else wth_say("THE STAIRS GO UP INTO A DARK SO THICK IT FEELS WET. NOT WITHOUT A LIGHT.");
        break;
    /* the parlour */
    case S_PAR_PIG:
        if (held) nothing();
        else take(F_PIG_TAKEN, I_PIG, "A CHINA MONEY PIG ON THE MANTELPIECE. IT'S HEAVY, AND SOMETHING INSIDE RATTLES.");
        break;
    case S_PAR_FIRE:
        if (held == I_PARAFFIN && !wth_flag(F_FIRE_SOAKED)) {
            wth_set(F_FIRE_SOAKED);
            wth_take_away(I_PARAFFIN);
            wth_say("YOU SLOSH PARAFFIN OVER THE WAX FIGURE AND THE KINDLING UNDER IT.");
        } else if (held == I_MATCHES && !wth_flag(F_FIRE_LIT)) {
            if (!wth_flag(F_FIRE_SOAKED)) {
                wth_say("THE MATCH BURNS DOWN TO YOUR FINGERS. THE WAX WON'T CATCH ON ITS OWN.");
            } else {
                wth_set(F_FIRE_LIT);
                wth_take_away(I_MATCHES);
                sfx_play_name("wth_fire");
                wth_say("THE FIRE ROARS UP THE CHIMNEY. THE WAX MAN SAGS, AND RUNS, AND SOMETHING DARK RED DROPS OUT "
                        "OF HIS CHEST INTO THE ASHES.");
            }
        } else if (held) nothing();
        else if (wth_flag(F_FIRE_LIT) && !wth_flag(F_GARNET_TAKEN)) {
            take(F_GARNET_TAKEN, I_GARNET, "YOU RAKE A GARNET OUT OF THE HOT ASHES.");
        } else if (wth_flag(F_FIRE_LIT)) wth_say("THE FIRE BURNS LOW. THE WAX HAS GONE.");
        else wth_say("A WAX FIGURE OF A MAN SITS IN THE COLD GRATE WITH ITS HANDS FOLDED. SOMETHING IN ITS CHEST "
                     "CATCHES THE LIGHT.");
        break;
    case S_PAR_RADIO:
        if (held) nothing();
        else {
            wth_noise(false);
            sfx_play_name("wth_static");
            wth_say("YOU TURN THE KNOB. A ROAR OF STATIC AT FULL BLAST, AND UNDER IT SOMETHING LIKE BREATHING. YOU "
                    "SNAP IT OFF. TOO LATE.");
        }
        break;
    case S_PAR_SOFA:
        if (held) nothing();
        else if (!take_page(s)) wth_say("A SAGGING SOFA. THE ANTIMACASSARS ARE STIFF WITH SOMETHING THAT DRIED.");
        break;
    /* the dining room */
    case S_DIN_ORCH:
        if (held == I_PENNIES && !wth_flag(F_HYMN_OUT)) {
            wth_set(F_HYMN_OUT);
            wth_take_away(I_PENNIES);
            wth_give(I_HYMN);
            wth_noise(false);
            sfx_play_name("wth_orch");
            wth_say("THE ORCHESTRELLE WAKES WITH A CRASH OF DRUMS AND PIPES, LOUD ENOUGH TO RAISE THE DEAD. WHEN IT "
                    "RUNS DOWN, A DRAWER HAS SLID OPEN. YOU TAKE THE SHEET OF MUSIC INSIDE.");
        } else if (held) nothing();
        else if (wth_flag(F_HYMN_OUT)) wth_say("IT HAS PLAYED ITS TUNE. IT WON'T PLAY ANOTHER.");
        else wth_say("A PENNY-IN-THE-SLOT ORCHESTRELLE: DRUMS, PIPES AND CYMBALS IN A GLASS CABINET. THE SLOT IS "
                     "EMPTY AND WAITING.");
        break;
    case S_DIN_SIDE:
        if (held) nothing();
        else if (!take(F_TONGS_TAKEN, I_TONGS, "IN THE SIDEBOARD DRAWER, ROLLED IN A NAPKIN: SILVER SUGAR TONGS."))
            wth_say("THE SIDEBOARD HOLDS NOTHING BUT NAPKINS.");
        break;
    case S_DIN_TABLE:
        if (held) nothing();
        else if (!take_page(s)) wth_say("THE TABLE IS LAID FOR FOUR. EVERY PLATE HOLDS A SPOONFUL OF HONEY, DRIED HARD.");
        break;
    /* the kitchen */
    case S_KIT_STOVE:
        if (held) nothing();
        else if (!take(F_MATCHES_TAKEN, I_MATCHES, "A BOX OF MATCHES ON THE STOVE. YOU POCKET IT."))
            wth_say("A COLD BLACK STOVE.");
        break;
    case S_KIT_KNIVES:
        if (held) nothing();
        else if (!take(F_KNIFE_TAKEN, I_KNIFE, "A KNIFE BLOCK. YOU DRAW OUT THE LONGEST ONE."))
            wth_say("THE OTHER SLOTS IN THE BLOCK ARE EMPTY. SOMEONE ELSE HAS THE REST.");
        break;
    case S_KIT_BOOK:
        if (held) nothing();
        else if (!take_page(s)) wth_say("COOKERY BOOKS. EVERY RECIPE HAS HAD HONEY WRITTEN INTO IT IN PENCIL.");
        break;
    /* the scullery */
    case S_SCU_TUB:
        if (held) nothing();
        else if (!take(F_BAR_TAKEN, I_BAR, "YOU TIP THE COPPER WASH TUB. UNDERNEATH IT: A WRECKING BAR."))
            wth_say("A COPPER WASH TUB, GREEN WITH AGE.");
        break;
    case S_SCU_SODA:
        if (held) nothing();
        else take(F_SODA_TAKEN, I_SODA, "A TIN OF DRAIN SODA ON THE SHELF. YOU TAKE IT.");
        break;
    case S_SCU_DIARY: case S_SHE_DIARY: case S_STU_DIARY: case S_BAT_DIARY: case S_NUR_DIARY: case S_CHI_DIARY:
    case S_ATT_DIARY: case S_CEL_DIARY:
        if (held) nothing();
        else take_page(s);
        break;
    /* the smoking room */
    case S_SMO_HUMIDOR:
        if (held) nothing();
        else if (!take(F_SNUFF_TAKEN, I_SNUFF, "THE HUMIDOR HOLDS NO CIGARS, ONLY A TIN OF BLACK SNUFF."))
            wth_say("AN EMPTY HUMIDOR. IT SMELLS OF CEDAR AND SOMETHING SWEETER.");
        break;
    case S_SMO_DESK:
        if (held) nothing();
        else if (!take(F_BATT_TAKEN, I_BATTERIES, "IN THE DESK DRAWER, AMONG BILLS NOBODY PAID: TWO BIG BATTERIES."))
            wth_say("A WRITING DESK. UNPAID BILLS, A BLOTTER, A PEN DRIED SOLID.");
        break;
    case S_SMO_CHAIR:
        if (held) nothing();
        else if (!take_page(s)) wth_say("A WING CHAIR. THE SEAT IS DENTED IN THE SHAPE OF SOMEONE WHO SAT A VERY LONG TIME.");
        break;
    /* the washroom */
    case S_WAS_BATH:
        if (!wth_flag(F_BATH_DRAINED)) {
            if (held == I_SODA) {
                wth_set(F_BATH_DRAINED);
                wth_take_away(I_SODA);
                sfx_play_name("wth_drain");
                wth_say("YOU POUR IN THE SODA. THE BLACK WATER FIZZES, THEN GURGLES AWAY, AND DOWN IN THE PLUGHOLE "
                        "GRATE SOMETHING GLINTS.");
            } else if (held) nothing();
            else wth_say("THE BATH IS FULL OF BLACK, STILL WATER, THICK AS SYRUP. THE DRAIN IS STOPPED UP. YOU WON'T "
                         "PUT YOUR HAND IN.");
        } else if (!wth_flag(F_TINKEY_TAKEN)) {
            if (held == I_TONGS) {
                wth_set(F_TINKEY_TAKEN);
                wth_take_away(I_TONGS);
                wth_give(I_TINKEY);
                wth_say("YOU FEED THE TONGS DOWN THROUGH THE GRATE, AND CATCH, AND DRAW OUT A LITTLE TIN KEY.");
            } else if (held) nothing();
            else wth_say("SOMETHING IS CAUGHT DOWN IN THE PLUGHOLE GRATE. YOUR FINGERS ARE FAR TOO THICK.");
        } else if (held) nothing();
        else wth_say("AN EMPTY BATH WITH A GREY TIDEMARK.");
        break;
    case S_WAS_CAB:
        if (held) nothing();
        else if (!take(F_BORAX_TAKEN, I_BORAX, "IN THE CABINET, BEHIND THE CARBOLIC: A PACKET OF BORAX."))
            wth_say("A CABINET OF SOAPS GONE SOFT AND WHITE.");
        break;
    case S_WAS_TOILET: case S_BAT_TOILET:
        if (held) nothing();
        else {
            wth_noise(false);
            sfx_play_name("wth_flush");
            wth_say("YOU PULL THE CHAIN. THE CISTERN ROARS AND THE PIPES BANG ALL THROUGH THE HOUSE.");
        }
        break;
    /* the guest room */
    case S_GUE_BOX:
        if (held == I_TINKEY && !wth_flag(F_BOX_OPEN)) {
            wth_set(F_BOX_OPEN);
            wth_take_away(I_TINKEY);
            wth_give(I_COPPERKEY);
            wth_say("THE TIN KEY TURNS. IN THE WRITING BOX: A COPPER WINDING KEY.");
            take_page(s);
        } else if (held) nothing();
        else if (wth_flag(F_BOX_OPEN)) wth_say("THE WRITING BOX IS EMPTY.");
        else wth_say("A WRITING BOX ON THE TABLE, WITH A LITTLE TIN LOCK.");
        break;
    case S_GUE_SEW:
        if (held) nothing();
        else if (!take(F_OIL_TAKEN, I_OIL, "A SEWING MACHINE. BESIDE IT, A CAN OF MACHINE OIL."))
            wth_say("A SEWING MACHINE WITH A HALF-SEWN SHROUD UNDER ITS NEEDLE.");
        break;
    /* the glasshouse */
    case S_GLA_TRELLIS:
        if (held) nothing();
        else if (!take(F_HOOK_TAKEN, I_CUPHOOK, "A BRASS CUP HOOK HOLDS A DEAD VINE TO THE TRELLIS. YOU WORK IT OUT OF THE WOOD."))
            wth_say("A TRELLIS AND A DEAD VINE.");
        break;
    case S_GLA_PANE:
        if (held == I_BAR) {
            wth_noise(false);
            sfx_play_name("wth_glass");
            wth_say("YOU JAB THE BAR THROUGH THE CRACKED PANE. GLASS RAINS DOWN WITH A CRASH LIKE A GUNSHOT.");
        } else if (held) nothing();
        else wth_say("A CRACKED PANE IN THE ROOF. ONE GOOD KNOCK WOULD BRING IT DOWN.");
        break;
    /* the garage */
    case S_GAR_OPENER:
        if (held == I_MOTOR && !wth_flag(F_MOTOR_FITTED)) {
            wth_set(F_MOTOR_FITTED);
            wth_take_away(I_MOTOR);
            wth_say("YOU BOLT THE MOTOR INTO THE EMPTY CRADLE OF THE DOOR OPENER.");
        } else if (held == I_OIL && !wth_flag(F_RAILS_OILED)) {
            wth_set(F_RAILS_OILED);
            oil_job_done();
            wth_say("YOU OIL THE DOOR RAILS ALL THE WAY ALONG. THE RUST LOOSENS.");
        } else if (held) nothing();
        else if (!wth_flag(F_MOTOR_FITTED))
            wth_say(wth_flag(F_RAILS_OILED) ? "THE DOOR OPENER ON THE CEILING. THE RAILS ARE OILED, BUT ITS MOTOR HAS BEEN TAKEN OUT."
                                            : "THE DOOR OPENER ON THE CEILING. ITS MOTOR HAS BEEN TAKEN OUT, AND THE RAILS ARE RUSTED STIFF.");
        else if (!wth_flag(F_RAILS_OILED)) wth_say("THE MOTOR IS IN. THE RAILS ARE STILL RUSTED STIFF.");
        else wth_say("THE OPENER IS MENDED. IT RUNS FROM THE BUTTON ON THE WALL.");
        break;
    case S_GAR_BUTTON:
        if (held) nothing();
        else if (wth_flag(F_GARAGE_OPEN)) wth_say("THE DOOR IS ALREADY UP.");
        else if (wth_flag(F_MOTOR_FITTED) && wth_flag(F_RAILS_OILED)) {
            wth_set(F_GARAGE_OPEN);
            wth_noise(false);
            sfx_play_name("wth_rumble");
            wth_say("THE MOTOR WHINES AND THE GARAGE DOOR RATTLES UP ITS RAILS, LOUD AS A GOODS TRAIN. COLD AIR. THE DRIVE.");
        } else wth_say("YOU PRESS THE BUTTON. A CLICK, AND NOTHING ELSE.");
        break;
    case S_GAR_SPANNER:
        if (held) nothing();
        else take(F_SPANNER_TAKEN, I_SPANNER, "A SPANNER ON THE TOOL BOARD. YOU TAKE IT DOWN.");
        break;
    case S_GAR_TWINE:
        if (held) nothing();
        else take(F_TWINE_TAKEN, I_TWINE, "A BALL OF TARRED TWINE ON A NAIL. YOU TAKE IT.");
        break;
    case S_GAR_SHELF:
        if (held) nothing();
        else if (!take(F_PARAFFIN_TAKEN, I_PARAFFIN, "ON THE SHELF, AMONG EMPTY HONEY JARS: A CAN OF PARAFFIN."))
            wth_say("EMPTY HONEY JARS, WASHED AND WAITING.");
        break;
    case S_GAR_CAR:
        if (held == I_CARKEYS) {
            if (wth_flag(F_GARAGE_OPEN)) wth_end_game(END_CAR);
            else {
                wth_noise(false);
                sfx_play_name("wth_engine");
                wth_say("THE ENGINE ROARS INTO LIFE, DEAFENING IN THE SHUT GARAGE. THE DOOR IS DOWN. YOU KILL THE "
                        "ENGINE, SHAKING.");
            }
        } else if (held) nothing();
        else if (!take_page(s))
            wth_say("A BIG GREEN ROVER. THE DOORS AREN'T LOCKED, BUT THERE ARE NO KEYS IN IT.");
        break;
    case S_GAR_DOOR:
        if (held) nothing();
        else wth_say("THE GARAGE DOOR, PULLED DOWN. FAR TOO HEAVY TO LIFT BY HAND.");
        break;
    /* the pond */
    case S_POND_BODY:
        if (held == I_LONGHOOK && !wth_flag(F_POND_DONE)) {
            wth_set(F_POND_DONE);
            wth_give(I_SHEDKEY);
            hook_job_done();
            wth_say("YOU REACH OUT WITH THE LONG HOOK, CATCH THE BODY BY ITS COLLAR AND DRAW IT TO THE BANK. A MAN IN A "
                    "WATER BOARD JACKET. IN HIS POCKET, A KEY ON A WOODEN TAG.");
        } else if ((held == I_CANE || held == I_CUPHOOK) && !wth_flag(F_POND_DONE)) {
            wth_say("IT REACHES HIM, BUT IT CAN'T GET A GRIP.");
        } else if (held) nothing();
        else if (wth_flag(F_POND_DONE)) wth_say("THE MAN LIES ON THE BANK NOW. YOU DON'T LOOK AT HIS FACE.");
        else wth_say("A MAN FLOATS FACE DOWN IN THE MIDDLE OF THE POND, FAR OUT OF REACH.");
        break;
    /* the shed */
    case S_SHE_GRINDER:
        if (held == I_SPANNER && !wth_flag(F_MOTOR_TAKEN)) {
            wth_set(F_MOTOR_TAKEN);
            wth_take_away(I_SPANNER);
            wth_give(I_MOTOR);
            wth_say("YOU UNBOLT THE BENCH GRINDER AND PULL ITS ELECTRIC MOTOR FREE.");
        } else if (held) nothing();
        else if (wth_flag(F_MOTOR_TAKEN)) wth_say("WHAT'S LEFT OF THE GRINDER.");
        else wth_say("A BENCH GRINDER, BOLTED DOWN TIGHT. ITS LITTLE MOTOR COULD RUN SOMETHING ELSE.");
        break;
    case S_SHE_VISE:
        if (held == I_PIG) {
            wth_take_away(I_PIG);
            wth_give(I_BRASSKEY);
            wth_say("YOU CLAMP THE PIG IN THE VISE AND TURN THE HANDLE. IT CRACKS QUIETLY IN TWO. A BRASS KEY DROPS OUT.");
        } else if (held) nothing();
        else wth_say("A BENCH VISE. IT GRIPS, SLOWLY AND HARD.");
        break;
    case S_SHE_SCREW:
        if (held) nothing();
        else take(F_SCREW_TAKEN, I_SCREWDRIVER, "A LONG SCREWDRIVER HANGS ON THE PEGBOARD. YOU TAKE IT.");
        break;
    case S_SHE_SHEARS:
        if (held) nothing();
        else take(F_SHEARS_TAKEN, I_SHEARS, "GARDEN SHEARS, NEWLY SHARPENED. YOU TAKE THEM.");
        break;
    case S_SHE_SAW:
        if (held) nothing();
        else take(F_SAW_TAKEN, I_SAW, "A PRUNING SAW. YOU LIFT IT OFF ITS PEGS.");
        break;
    case S_SHE_SPADE:
        if (held) nothing();
        else take(F_SPADE_TAKEN, I_SPADE, "A SPADE LEANS IN THE CORNER. YOU TAKE IT.");
        break;
    case S_SHE_SACK:
        if (held) nothing();
        else if (!take(F_PEWTER_TAKEN, I_PEWTERKEY, "A SACK OF DAFFODIL BULBS, ALL ROTTEN. AT THE BOTTOM: A PEWTER KEY."))
            wth_say("A SACK OF ROTTEN BULBS.");
        break;
    /* the garden */
    case S_GDN_FALLEN:
        if (held) nothing();
        else if (!take(F_AMBER_TAKEN, I_AMBER, "THE BODY FROM THE BALCONY LIES CRUMPLED ON THE GRASS. ITS FIST HAS BURST OPEN ON A LUMP OF AMBER."))
            wth_say("YOU DON'T LOOK AT IT AGAIN.");
        break;
    case S_GDN_GRAVE:
        if (held == I_SPADE && !wth_flag(F_GRAVE_DUG)) {
            wth_set(F_GRAVE_DUG);
            wth_take_away(I_SPADE);
            wth_give(I_CARKEYS);
            sfx_play_name("wth_dig");
            wth_say("YOU DIG UNDER THE YEW. A DOG'S COLLAR, A LEAD, A CHEWED BALL, AND A TOBACCO TIN. IN THE TIN: A "
                    "RING OF CAR KEYS.");
        } else if (held) nothing();
        else if (wth_flag(F_GRAVE_DUG)) wth_say("THE GRAVE STANDS OPEN. YOU SHOULD FILL IT IN. YOU DON'T.");
        else wth_say("A LOW MOUND UNDER THE YEW. A CROSS OF TWO STICKS SAYS SORREL.");
        break;
    case S_GDN_IVY:
        if (held == I_SHEARS) {
            wth_set(F_IVY_CUT);
            wth_take_away(I_SHEARS);
            wth_say("YOU HACK THROUGH THE IVY. BEHIND IT, AN ARCH IN THE WALL, AND A COURTYARD.");
        } else if (held) nothing();
        else wth_say("AN ARCH IN THE GARDEN WALL, CHOKED WITH IVY AS THICK AS YOUR ARM.");
        break;
    case S_GDN_SHEDDOOR:
        if (held == I_SHEDKEY) {
            wth_set(F_SHED_OPEN);
            wth_take_away(I_SHEDKEY);
            sfx_play_name("wth_door");
            wth_say("THE SHED KEY OPENS THE PADLOCK.");
        } else if (held) nothing();
        else wth_say("THE POTTING SHED. A PADLOCK ON THE DOOR.");
        break;
    /* the drive */
    case S_DRV_GARAGE:
        if (held) nothing();
        else wth_say("THE GARAGE DOOR. SHUT FROM THE INSIDE.");
        break;
    case S_DRV_STATUE:
        if (held == I_RING && !wth_flag(F_FOUNTAIN_DRAINED)) {
            wth_set(F_FOUNTAIN_DRAINED);
            wth_take_away(I_RING);
            sfx_play_name("wth_drain");
            wth_say("YOU PRESS THE RING INTO THE HARE ON THE PLINTH. A CLUNK DEEP UNDERGROUND, AND THE FOUNTAIN "
                    "BEGINS TO DRAIN.");
        } else if (held) nothing();
        else wth_say("A STONE WOMAN POURING WATER FROM A JAR. SUNK INTO THE PLINTH, LIKE A SEAL IN WAX: A HARE IN A "
                     "RING OF THORNS.");
        break;
    case S_DRV_FOUNTAIN:
        if (held) nothing();
        else if (!wth_flag(F_FOUNTAIN_DRAINED))
            wth_say("THE BASIN IS FULL OF DARK WATER. SOMETHING GREEN GLINTS ON THE BOTTOM, TOO DEEP TO REACH.");
        else if (!take(F_JADE_TAKEN, I_JADE, "IN THE MUD AT THE BOTTOM OF THE EMPTY BASIN: A GREEN STONE. JADE."))
            wth_say("AN EMPTY BASIN AND A SMELL OF MUD.");
        break;
    case S_DRV_STORM:
        if (held == I_KEEPERKEY) {
            wth_set(F_STORM_OPEN);
            wth_take_away(I_KEEPERKEY);
            sfx_play_name("wth_door");
            wth_say("THE BLACK KEY FITS. THE CELLAR DOORS LIFT ON A BREATH OF WARM, SWEET AIR.");
        } else if (held) nothing();
        else wth_say("TWO SLOPING CELLAR DOORS IN THE GRAVEL, CHAINED, WITH A BIG BLACK PADLOCK.");
        break;
    /* the courtyard */
    case S_CRT_HARE0: hare(0, held); break;
    case S_CRT_HARE1: hare(1, held); break;
    case S_CRT_HARE2: hare(2, held); break;
    case S_CRT_HARE3: hare(3, held); break;
    case S_CRT_GATE:
        if (held) nothing();
        else wth_say("AN IRON GATE IN THE CORNER, HELD BY NOTHING YOU CAN SEE. BEHIND IT, HIGH BLACK HEDGES.");
        break;
    /* the heart of the hedges */
    case S_HRT_GIRL:
        if (held) nothing();
        else if (!take(F_BOLT_TAKEN, I_BOLT, "A STONE GIRL HOLDS OUT HER CUPPED HANDS. IN THEM LIES A CROSSBOW BOLT. YOU TAKE IT."))
            wth_say("HER HANDS ARE EMPTY NOW. SHE LOOKS DISAPPOINTED IN YOU.");
        break;
    /* the landing */
    case S_LAN_HATCH:
        if (held == I_LONGHOOK) {
            wth_set(F_HATCH_DOWN);
            hook_job_done();
            sfx_play_name("wth_creak");
            wth_say("YOU CATCH THE RING ON THE HATCH WITH THE LONG HOOK AND PULL. A LADDER SLIDES DOWN WITH A SHRIEK.");
        } else if (held) nothing();
        else wth_say("A HATCH IN THE CEILING WITH A RING ON IT. FAR TOO HIGH TO REACH.");
        break;
    case S_LAN_MASTER_SHUT:
        if (held == I_BRASSKEY) {
            wth_set(F_MASTER_OPEN);
            wth_take_away(I_BRASSKEY);
            sfx_play_name("wth_door");
            wth_say("THE BRASS KEY TURNS. THE BIG BEDROOM DOOR SWINGS IN.");
        } else if (held) nothing();
        else wth_say("A LOCKED DOOR WITH A BRASS KEYHOLE.");
        break;
    /* the big bedroom */
    case S_MAS_BED:
        if (held) nothing();
        else if (!take_page(s)) wth_say("A FOUR-POSTER BED. ONE SIDE IS NEATLY MADE. THE OTHER IS A NEST OF WHITE THREADS.");
        break;
    case S_MAS_DRESSER:
        if (held) nothing();
        else if (!take(F_LENS_TAKEN, I_LENS, "ON THE DRESSING TABLE, AMONG HAIRPINS: A MAGNIFYING GLASS."))
            wth_say("A DRESSING TABLE. THE MIRROR HAS BEEN TURNED TO THE WALL.");
        break;
    case S_MAS_SAFE:
        if (held == I_COMBO && !wth_flag(F_SAFE_OPEN)) {
            wth_set(F_SAFE_OPEN);
            wth_take_away(I_COMBO);
            wth_set(F_OPAL_TAKEN);
            wth_give(I_OPAL);
            sfx_play_name("wth_click");
            wth_say("RIGHT 14, LEFT 3, RIGHT 27. THE SAFE SWINGS OPEN. INSIDE, ON A PAD OF VELVET: AN OPAL. YOU TAKE IT.");
        } else if (held) nothing();
        else if (wth_flag(F_SAFE_OPEN)) wth_say("THE SAFE IS EMPTY.");
        else wth_say("A WALL SAFE WITH A NUMBERED DIAL. YOU DON'T KNOW THE NUMBERS.");
        break;
    /* the balcony */
    case S_BAL_ROPE:
        if (held == I_SAW && !wth_flag(F_ROPE_CUT)) {
            wth_set(F_ROPE_CUT);
            wth_take_away(I_SAW);
            sfx_play_name("wth_thud");
            wth_say("YOU SAW THROUGH THE ROPE. IT PARTS, AND THE WEIGHT ON THE END DROPS INTO THE GARDEN WITH A SOFT, "
                    "AWFUL SOUND.");
        } else if (held == I_KNIFE && !wth_flag(F_ROPE_CUT)) {
            wth_say("THE ROPE IS TARRED AND AS THICK AS YOUR WRIST. THE KNIFE ONLY SLIDES OFF IT.");
        } else if (held) nothing();
        else if (wth_flag(F_ROPE_CUT)) wth_say("A FRAYED END OF ROPE ON THE RAIL.");
        else wth_say("A ROPE TIED TO THE RAIL, RUNNING DOWN OVER THE EDGE, PULLED TIGHT BY SOMETHING HEAVY. THE KNOT "
                     "HAS SWOLLEN SHUT.");
        break;
    /* the study */
    case S_STU_COMPUTER:
        if (held == I_PASSWORD && !wth_flag(F_RECIPE)) {
            wth_set(F_RECIPE);
            wth_take_away(I_PASSWORD);
            sfx_play_name("wth_click");
            wth_say("YOU TYPE QUEENLESS. THE SCREEN FILLS WITH GREEN LETTERS: TO KILL IT, MIX IN ONE CUP BLACK "
                    "SNUFF, OIL OF CLOVES AND BORAX. POUR IT ON THE FACE. NOTHING ELSE WILL DO.");
        } else if (held) nothing();
        else if (wth_flag(F_RECIPE))
            wth_say("THE SCREEN STILL SAYS: MIX IN ONE CUP BLACK SNUFF, OIL OF CLOVES AND BORAX. POUR IT ON THE FACE.");
        else wth_say("A HOME COMPUTER, STILL SWITCHED ON. GREEN LETTERS: PASSWORD?");
        break;
    case S_STU_CASE:
        if (held == I_BAR && !wth_flag(F_CASE_BROKEN)) {
            wth_set(F_CASE_BROKEN);
            wth_set(F_RING_TAKEN);
            wth_give(I_RING);
            wth_noise(false);
            sfx_play_name("wth_glass");
            wth_say("YOU SMASH THE CASE. GLASS FLIES, LOUD AS A DROPPED TRAY OF BELLS. A SIGNET RING IS CAUGHT ON THE "
                    "STUFFED PIKE'S TEETH. YOU WORK IT OFF.");
        } else if (held == I_KNIFE && !wth_flag(F_CASE_BROKEN)) {
            wth_say("THE GLASS IS THICK. YOU'D NEED SOMETHING HEAVY.");
        } else if (held) nothing();
        else if (wth_flag(F_CASE_BROKEN)) wth_say("A BROKEN CASE AND A PIKE WITH ITS MOUTH OPEN.");
        else wth_say("A GLASS CASE WITH A STUFFED PIKE IN IT. A RING HAS CAUGHT ON ITS TEETH. THE CASE IS SCREWED SHUT.");
        break;
    /* the bathroom */
    case S_BAT_TUB:
        if (held) nothing();
        else wth_say("SOMEONE LIES IN THE BATH WITH THEIR MOUTH WIDE OPEN IN A SCREAM, AND WHITE THREADS GROW OUT OF IT "
                     "LIKE BREATH ON A COLD DAY.");
        break;
    case S_BAT_MIRROR:
        if (held == I_BAR && !wth_flag(F_MIRROR_BROKEN)) {
            wth_set(F_MIRROR_BROKEN);
            wth_set(F_COMBO_TAKEN);
            wth_give(I_COMBO);
            wth_noise(false);
            sfx_play_name("wth_glass");
            wth_say("YOU PUT THE BAR THROUGH THE MIRROR. THE CRASH GOES ON AND ON. TAPED TO THE WALL BEHIND IT: A "
                    "CARD OF NUMBERS.");
        } else if (held) nothing();
        else if (wth_flag(F_MIRROR_BROKEN)) wth_say("A SMASHED MIRROR. SEVEN YEARS, AT LEAST.");
        else wth_say("A MIRROR. YOU LOOK AWFUL. THE GLASS STANDS A LITTLE OUT FROM THE WALL, AS IF SOMETHING IS "
                     "BEHIND IT.");
        break;
    case S_BAT_SHELF:
        if (held) nothing();
        else if (!take(F_CLOVE_TAKEN, I_CLOVE, "ON THE SHELF, A BROWN BOTTLE: OIL OF CLOVES."))
            wth_say("A SHELF OF EMPTY MEDICINE BOTTLES.");
        break;
    /* the nursery */
    case S_NUR_BOX:
        if (held == I_SILVERKEY && !wth_flag(F_BOXMUSIC)) {
            wth_set(F_BOXMUSIC);
            wth_take_away(I_SILVERKEY);
            wth_set(F_GLASSES_TAKEN);
            wth_give(I_GLASSES);
            wth_noise(false);
            sfx_play_name("wth_musicbox");
            wth_say("THE SILVER KEY WINDS IT. THE LID SPRINGS UP AND A TUNE PLAYS, TINNY AND FAR TOO LOUD, ROUND AND "
                    "ROUND. IN THE TRAY UNDER THE DANCER: A PAIR OF RED SPECTACLES.");
        } else if (held) nothing();
        else if (wth_flag(F_BOXMUSIC)) wth_say("THE MUSIC BOX HAS STOPPED PLAYING, THANK GOD.");
        else wth_say("A MUSIC BOX WITH A DANCER ON THE LID AND A LITTLE SILVER LOCK.");
        break;
    case S_NUR_WALL:
        if (held == I_GLASSES && !wth_flag(F_WALL_READ)) {
            wth_set(F_WALL_READ);
            wth_take_away(I_GLASSES);
            wth_give(I_DIRECTIONS);
            wth_say("THROUGH THE RED GLASS THE SCRIBBLE VANISHES, AND WORDS STAND OUT THAT WERE WRITTEN UNDER IT: "
                    "THE WAY IN. RIGHT, LEFT, LEFT, STRAIGHT ON, RIGHT, LEFT.");
        } else if (held) nothing();
        else if (wth_flag(F_WALL_READ)) wth_say("RED CRAYON SCRIBBLE. YOU KNOW WHAT IS WRITTEN UNDER IT NOW.");
        else wth_say("THE WALLPAPER IS COVERED IN RED CRAYON, ROUND AND ROUND AND ROUND, AS HIGH AS A CHILD CAN REACH.");
        break;
    /* the children's room */
    case S_CHI_HOUSE:
        if (held == I_LENS && !wth_flag(F_DOLLHOUSE_READ)) {
            wth_set(F_DOLLHOUSE_READ);
            wth_take_away(I_LENS);
            wth_give(I_PASSWORD);
            wth_say("THROUGH THE MAGNIFIER, IN THE DOLLHOUSE STUDY, A TINY NOTE ON A TINY DESK: THE WORD FOR THE "
                    "MACHINE IS QUEENLESS.");
        } else if (held) nothing();
        else wth_say("A DOLLHOUSE OF THIS HOUSE, ROOM FOR ROOM. THERE'S WRITING ON THE DESK IN THE LITTLE STUDY, FAR "
                     "TOO SMALL TO READ.");
        break;
    case S_CHI_DOLL:
        if (held) nothing();
        else take(F_DOLL_TAKEN, I_DOLL, "A RAG DOLL ON THE FLOOR, SEWN SHUT ALL DOWN ITS BACK. SOMETHING HARD INSIDE. YOU TAKE IT.");
        break;
    /* the attic */
    case S_ATT_TRUNK:
        if (held == I_PEWTERKEY && !wth_flag(F_TRUNK_OPEN)) {
            wth_set(F_TRUNK_OPEN);
            wth_take_away(I_PEWTERKEY);
            wth_give(I_CROSSBOW);
            sfx_play_name("wth_creak");
            wth_say("THE PEWTER KEY TURNS. IN THE TRUNK, WRAPPED IN OILCLOTH: A HUNTING CROSSBOW. NO BOLTS.");
        } else if (held) nothing();
        else if (wth_flag(F_TRUNK_OPEN)) wth_say("AN EMPTY TRUNK THAT SMELLS OF GUN OIL.");
        else wth_say("A STEAMER TRUNK WITH A PEWTER LOCK.");
        break;
    case S_ATT_HARM:
        if (held == I_HYMN && !wth_flag(F_HARMONIUM_PLAYED)) {
            wth_set(F_HARMONIUM_PLAYED);
            wth_take_away(I_HYMN);
            wth_give(I_STEELWIRE);
            wth_noise(false);
            sfx_play_name("wth_organ");
            wth_say("YOU SET THE HYMN ON THE STAND AND PUMP THE PEDALS. THE ORGAN BELLOWS THROUGH THE ROOF. A STOP "
                    "JAMS, SOMETHING TWANGS, AND A COIL OF STEEL WIRE SPRINGS OUT FROM INSIDE.");
        } else if (held) nothing();
        else if (wth_flag(F_HARMONIUM_PLAYED)) wth_say("THE ORGAN WHEEZES. IT WON'T PLAY AGAIN.");
        else wth_say("A PUMP ORGAN UNDER A DUST SHEET. NOTHING ON ITS MUSIC STAND.");
        break;
    /* the cellar */
    case S_CEL_MECH: winch(held); break;
    case S_CEL_DOOR:
        if (held) nothing();
        else wth_say("AN IRON DOOR, WOUND DOWN SHUT. ITS WINCH IS BROKEN.");
        break;
    /* the white room */
    case S_SPO_FACE:
        if (held == I_BREW) wth_end_game(END_CELLAR);
        else if (held) wth_say("IT TAKES IT. IT SEEMS TO LIKE IT.");
        else wth_say("A FACE IN THE WALL, AS BIG AS A DOOR, MADE OF WHITE THREADS. IT IS BREATHING. IT KNOWS YOU.");
        break;
    default:
        if (held) nothing();
        else wth_say("NOTHING.");
        break;
    }
}

/* ------------------------------------------------------------------ */
/* one thing on another                                                 */

static bool pair(int a, int b, int x, int y) { return (a == x && b == y) || (a == y && b == x); }

void wth_combine(int a, int b) {
    /* the cane, the cup hook and the twine: all three make the long hook */
    bool pole_part_a = a == I_CANE || a == I_CUPHOOK || a == I_TWINE;
    bool pole_part_b = b == I_CANE || b == I_CUPHOOK || b == I_TWINE;
    if (pole_part_a && pole_part_b && a != b) {
        if (wth_has(I_CANE) && wth_has(I_CUPHOOK) && wth_has(I_TWINE)) {
            wth_take_away(I_CANE);
            wth_take_away(I_CUPHOOK);
            wth_take_away(I_TWINE);
            wth_give(I_LONGHOOK);
            wth_say("YOU SET THE CUP HOOK AGAINST THE TIP OF THE CANE AND BIND IT ON TIGHT WITH TWINE. A LONG HOOK.");
        } else if (!wth_has(I_TWINE)) wth_say("THE HOOK WON'T STAY ON THE CANE. YOU'D NEED SOMETHING TO BIND IT.");
        else if (!wth_has(I_CANE)) wth_say("SOMETHING TO TIE THE HOOK ONTO, THEN. SOMETHING LONG.");
        else wth_say("A CANE AND TWINE. SOMETHING TO TIE ON THE END WOULD HELP.");
        return;
    }
    if (pair(a, b, I_TORCH, I_BATTERIES)) {
        wth_set(F_TORCH_LOADED);
        wth_take_away(I_BATTERIES);
        sfx_play_name("wth_click");
        wth_say("YOU LOAD THE BATTERIES. THE TORCH FLICKERS, AND HOLDS.");
        return;
    }
    if (pair(a, b, I_CROSSBOW, I_BOLT)) {
        wth_set(F_BOW_LOADED);
        wth_take_away(I_BOLT);
        sfx_play_name("wth_click");
        wth_say("YOU CRANK THE CROSSBOW BACK AND DROP THE BOLT INTO ITS GROOVE. ONE SHOT.");
        return;
    }
    if (pair(a, b, I_KNIFE, I_DOLL)) {
        wth_take_away(I_DOLL);
        wth_give(I_SILVERKEY);
        wth_say("YOU SLIT OPEN THE DOLL'S BACK. IN THE STUFFING: A TINY SILVER KEY.");
        return;
    }
    if (pair(a, b, I_BAR, I_PIG)) {
        wth_take_away(I_PIG);
        wth_give(I_BRASSKEY);
        sfx_play_name("wth_smash");
        wth_say("YOU BRING THE BAR DOWN ON THE PIG. IT BURSTS. AMONG THE PIECES: A BRASS KEY.");
        return;
    }
    int other = a == I_MUG ? b : b == I_MUG ? a : 0;
    if (other == I_SNUFF || other == I_CLOVE || other == I_BORAX) {
        if (!wth_flag(F_RECIPE)) {
            wth_say("YOU DON'T KNOW WHAT THAT WOULD DO. NOT YET.");
            return;
        }
        wth.s.mug |= (uint8_t)(other == I_SNUFF ? 1 : other == I_CLOVE ? 2 : 4);
        wth_take_away(other);
        if (wth.s.mug == 7) {
            wth_take_away(I_MUG);
            wth_give(I_BREW);
            sfx_play_name("wth_fizz");
            wth_say("THE LAST OF IT GOES IN. THE MUG FIZZES AND GIVES OFF A BROWN SMOKE THAT MAKES YOUR EYES STREAM.");
        } else wth_sayf("YOU TIP THE %s INTO THE MUG AND STIR IT WITH A FINGER.", WTH_ITEM[other].name);
        return;
    }
    wth_say("THOSE DON'T GO TOGETHER.");
}

/* ------------------------------------------------------------------ */
/* leaving a room                                                       */

void wth_start_exit(int dest) {
    if (wth.trans_t > 0) return;
    wth.trans_t = 1;
    wth.trans_to = dest;
    sfx_play_name("wth_step");
}

void wth_goto_room(int room) {
    int from = wth.s.room;
    wth.s.room = (uint8_t)room;
    if (from == R_CELL && room != R_CELL) wth_set(F_LEFT_CELL);
    if (room == R_HEART && !wth_flag(F_HEART_SEEN)) {
        wth_set(F_HEART_SEEN);
        wth_take_away(I_DIRECTIONS);
    }
    wth_keeper_room_changed(from, room);
    wth_room_music();
    wth_save_now();
}
