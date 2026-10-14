/* WITHERLEA - clicks: looking, taking, using things on the house and on
 * each other, the exits and the maze, noise. Every thing has one job (a few
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
    case S_GDN_SHEDDOOR: return !wth_flag(F_SHED_OPEN);
    case S_GDN_SHED: return wth_flag(F_SHED_OPEN);
    case S_DRV_HATCH: return !wth_flag(F_HATCH_OPEN);
    case S_DRV_CELLAR: return wth_flag(F_HATCH_OPEN);
    case S_CRT_GATE: return !wth_flag(F_GATE_OPEN);
    case S_CRT_MAZE: return wth_flag(F_GATE_OPEN);
    case S_LAN_MASTER_SHUT: return !wth_flag(F_MASTER_OPEN);
    case S_LAN_MASTER: return wth_flag(F_MASTER_OPEN);
    case S_LAN_HATCH: return !wth_flag(F_HATCH_DOWN);
    case S_LAN_ATTIC: return wth_flag(F_HATCH_DOWN);
    case S_CEL_DOOR: return !wth_flag(F_CELLAR_DRAINED);
    case S_CEL_SPORE: return wth_flag(F_CELLAR_DRAINED);
    case S_SCU_BUCKET: return !wth_flag(F_BUCKET_TAKEN);
    case S_GAR_SCREW: return !wth_flag(F_SCREW_TAKEN);
    case S_GAR_GREASE: return !wth_flag(F_GREASE_TAKEN);
    case S_SHE_PLIERS: return !wth_flag(F_PLIERS_TAKEN);
    case S_SHE_CUTTERS: return !wth_flag(F_CUTTERS_TAKEN);
    case S_SHE_SMOKER: return !wth_flag(F_SMOKER_TAKEN);
    case S_SHE_SPRAYER: return !wth_flag(F_SPRAYER_TAKEN);
    case S_SHE_GLOVES: return !wth_flag(F_GLOVES_TAKEN);
    case S_CHI_POLE: return !wth_flag(F_POLE_TAKEN);
    case S_GLA_LAMP: return !wth_flag(F_LAMP_TAKEN);
    case S_SCU_DIARY: case S_SHE_DIARY: case S_STU_DIARY: case S_BAT_DIARY: case S_NUR_DIARY: case S_CHI_DIARY:
    case S_ATT_DIARY: case S_CEL_DIARY: case S_DIN_DIARY:
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

/* things with two jobs leave the bag after the second */
static void cutters_job(void) {
    if (wth_flag(F_CUTTERS_USED1)) wth_take_away(I_CUTTERS);
    else wth_set(F_CUTTERS_USED1);
}
static void screw_job(void) {
    if (wth_flag(F_MAGNET_TAKEN) && wth_flag(F_BEE_HORSE)) wth_take_away(I_SCREWDRIVER);
}
static void matches_job(void) {
    if (wth_flag(F_MATCHES_USED1)) wth_take_away(I_MATCHES);
    else wth_set(F_MATCHES_USED1);
}

static const char *hide_look(int s, bool gone) {
    switch (s) {
    case S_KIT_PANTRY: return gone ? "THE PANTRY DOOR HANGS OFF ONE HINGE, HACKED THROUGH. NOBODY COULD HIDE IN THERE NOW."
                                   : "A WALK-IN PANTRY WITH A SLATTED DOOR. ROOM FOR ONE, BEHIND THE FLOUR SACKS.";
    case S_DIN_TABLE: return gone ? "THE TABLECLOTH HAS BEEN DRAGGED OFF AND SLASHED, AND THE TABLE TIPPED ON ITS SIDE."
                                  : "A LONG TABLE UNDER A CLOTH THAT REACHES THE FLOOR. YOU COULD CRAWL UNDERNEATH.";
    case S_MAS_WARDROBE: return gone ? "THE WARDROBE DOORS ARE SPLIT DOWN THE MIDDLE. THE COATS LIE CUT TO RIBBONS."
                                     : "A TALL WARDROBE FULL OF HEAVY COATS. YOU COULD STAND AT THE BACK BEHIND THEM.";
    case S_BAT_AIRING: return gone ? "THE AIRING CUPBOARD HAS BEEN EMPTIED ONTO THE FLOOR, SHELF BY SHELF, AND THE DOOR SMASHED."
                                   : "AN AIRING CUPBOARD, WARM FROM THE TANK. ROOM BELOW THE SHELVES TO CURL UP.";
    case S_GAR_PIT: return "THE INSPECTION PIT UNDER THE GARAGE FLOOR. DARK, OILY, AND OUT OF SIGHT.";
    case S_YRD_BUNKER: return "A COAL BUNKER WITH A LID. A FEW SACKS OF COAL, AND ROOM FOR YOU.";
    default: return "";
    }
}

int wth_hide_one_use_flag(int spot) {
    switch (spot) {
    case S_KIT_PANTRY: return F_HIDE_PANTRY_GONE;
    case S_DIN_TABLE: return F_HIDE_TABLE_GONE;
    case S_MAS_WARDROBE: return F_HIDE_WARDROBE_GONE;
    case S_BAT_AIRING: return F_HIDE_AIRING_GONE;
    default: return -1;
    }
}

/* ------------------------------------------------------------------ */
/* the stone hive by the hedge gate                                     */

int wth_hive_count(void) {
    int n = 0;
    for (int k = 0; k < 4; k++) n += (wth.s.hive >> k) & 1;
    return n;
}

static void stone_hive(int held) {
    if (wth_flag(F_GATE_OPEN)) { wth_say("FOUR METAL BEES SIT IN THE STONE HIVE. THE GATE STANDS OPEN."); return; }
    int b = -1;
    for (int k = 0; k < 4; k++)
        if (held && held == WTH_BEES[k]) b = k;
    if (held && b < 0) { nothing(); return; }
    if (!held) {
        wth_sayf("A STONE HIVE SET INTO THE WALL BY THE GATE, WITH FOUR LITTLE CELLS SHAPED LIKE BEES. %d OF THEM "
                 "%s FILLED.",
                 wth_hive_count(), wth_hive_count() == 1 ? "IS" : "ARE");
        return;
    }
    wth.s.hive |= (uint8_t)(1 << b);
    wth_take_away(held);
    sfx_play_name("wth_click");
    if (wth_hive_count() < 4) {
        wth_sayf("THE %s CLICKS INTO A CELL OF THE STONE HIVE AND STAYS.", WTH_ITEM[held].name);
        return;
    }
    wth_set(F_GATE_OPEN);
    sfx_play_name("wth_grind");
    wth_say("THE LAST BEE CLICKS HOME. SOMEWHERE IN THE WALL A WEIGHT DROPS, AND THE IRON GATE INTO THE HEDGES "
            "SWINGS OPEN ON ITS OWN.");
}

/* ------------------------------------------------------------------ */
/* the cellar pump                                                      */

static void pump(int held) {
    if (wth_flag(F_CELLAR_DRAINED)) { wth_say("THE PUMP STANDS IN A PUDDLE. THE INNER DOORWAY IS CLEAR."); return; }
    if (held == I_PUMPHANDLE && !wth_flag(F_PUMP_HANDLE)) {
        wth_set(F_PUMP_HANDLE);
        wth_take_away(I_PUMPHANDLE);
        wth_say("THE HANDLE DROPS INTO ITS SOCKET AND YOU PIN IT THERE.");
    } else if (held == I_WASHER && !wth_flag(F_PUMP_WASHER)) {
        wth_set(F_PUMP_WASHER);
        wth_take_away(I_WASHER);
        wth_say("YOU UNSCREW THE PUMP'S BARREL AND FIT THE LEATHER WASHER WHERE THE OLD ONE HAS ROTTED AWAY.");
    } else if (held == I_HOSE && !wth_flag(F_PUMP_HOSE)) {
        wth_set(F_PUMP_HOSE);
        wth_take_away(I_HOSE);
        wth_say("YOU PUSH THE HOSE ONTO THE SPOUT AND RUN IT TO THE DRAIN IN THE FLOOR.");
    } else if (held == I_GREASE && !wth_flag(F_PUMP_GREASE)) {
        wth_set(F_PUMP_GREASE);
        wth_take_away(I_GREASE);
        wth_say("YOU WORK BLACK GREASE INTO THE PISTON AND THE PIVOT UNTIL THEY MOVE.");
    } else if (held) {
        nothing();
        return;
    } else {
        char buf[300];
        snprintf(buf, sizeof buf, "AN OLD HAND PUMP BESIDE THE FLOODED DOORWAY.%s%s%s%s",
                 wth_flag(F_PUMP_HANDLE) ? "" : " IT HAS NO HANDLE.",
                 wth_flag(F_PUMP_WASHER) ? "" : " ITS WASHER HAS ROTTED TO NOTHING.",
                 wth_flag(F_PUMP_HOSE) ? "" : " THERE'S NO HOSE ON THE SPOUT.",
                 wth_flag(F_PUMP_GREASE) ? "" : " THE PISTON IS SEIZED.");
        wth_say(buf);
        return;
    }
    if (wth_flag(F_PUMP_HANDLE) && wth_flag(F_PUMP_WASHER) && wth_flag(F_PUMP_HOSE) && wth_flag(F_PUMP_GREASE)) {
        wth_set(F_CELLAR_DRAINED);
        sfx_play_name("wth_drain");
        wth_say("YOU PUMP UNTIL YOUR ARMS BURN. THE BLACK WATER IN THE DOORWAY DROPS, AND DROPS, AND GURGLES AWAY "
                "DOWN THE DRAIN. THE WAY IN IS CLEAR. WARM AIR. SWEET AIR.");
    }
}

/* ------------------------------------------------------------------ */
/* A on a spot                                                          */

static void take_or(int flag, int it, const char *got, const char *after, int held) {
    if (held) { nothing(); return; }
    if (!take(flag, it, got)) wth_say(after);
}

static void page_or(int s, const char *after, int held) {
    if (held) { nothing(); return; }
    if (!take_page(s) && after) wth_say(after);
}

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
    /* the cold store: hinges on the wrong side */
    case S_CELL_DOOR:
        if (held == I_NAIL && !wth_flag(F_NAIL_SET)) {
            wth_set(F_NAIL_SET);
            wth_take_away(I_NAIL);
            wth_say("YOU SET THE POINT OF THE NAIL UNDER THE TOP HINGE PIN. IT NEEDS DRIVING HOME.");
        } else if (held == I_STONE) {
            if (!wth_flag(F_NAIL_SET)) {
                wth_say("YOU BANG AT THE HINGES WITH THE STONE. THE PINS ARE TOO DEEP; YOU NEED SOMETHING TO DRIVE "
                        "THEM UP FROM BELOW.");
            } else {
                wth_set(F_CELL_OPEN);
                wth_take_away(I_STONE);
                sfx_play_name("wth_door");
                wth_say("YOU HAMMER THE NAIL WITH THE STONE. THE TOP PIN JUMPS OUT, THEN THE BOTTOM ONE, AND THE DOOR "
                        "SAGS OFF ITS HINGES INTO THE DARK PASSAGE.");
            }
        } else if (held) wth_say("THAT WON'T GET THE DOOR OPEN.");
        else if (wth_flag(F_NAIL_SET)) wth_say("THE NAIL IS WEDGED UNDER THE HINGE PIN, WAITING TO BE DRIVEN.");
        else wth_say("A HEAVY DOOR, LOCKED ON THE OUTSIDE. BUT WHOEVER HUNG IT PUT THE HINGES ON THIS SIDE.");
        break;
    case S_CELL_WALL:
        take_or(F_STONE_TAKEN, I_STONE, "ONE STONE IN THE WALL IS LOOSE IN ITS MORTAR. YOU WORK IT OUT.",
                "A HOLE WHERE THE LOOSE STONE WAS. COLD AIR BREATHES THROUGH IT.", held);
        break;
    case S_CELL_BARREL:
        take_or(F_NAIL_TAKEN, I_NAIL, "THE BARREL LID IS HELD BY ONE LONG NAIL, HALF OUT ALREADY. YOU WORK IT FREE.",
                "AN EMPTY SALT BARREL. IT STILL SMELLS OF EELS.", held);
        break;
    /* the passage, the sickroom, the hall */
    case S_PAS_RUG:
        page_or(s, "A LONG RUNNER RUG, WORN TO THE THREADS IN A PATH TO THE COLD STORE.", held);
        break;
    case S_SICK_BED: page_or(s, "A NARROW IRON BED. THE SHEETS ARE GREY AND SMELL OF HONEY.", held); break;
    case S_SICK_STAND:
        take_or(F_FUSE_TAKEN, I_FUSE,
                "IN THE NIGHTSTAND DRAWER, AMONG PILL BOTTLES: A BLOWN FUSE, PUT AWAY AS IF TO STOP ANYONE MENDING IT.",
                "AN EMPTY NIGHTSTAND.", held);
        break;
    case S_HALL_TABLE:
        take_or(F_PHOTO_TAKEN, I_PHOTO, "ON THE HALL TABLE, FACE DOWN: A WEDDING PHOTOGRAPH IN A CARD FRAME. YOU TAKE IT.",
                "A HALL TABLE. A CLEAN SQUARE IN THE DUST WHERE THE PHOTOGRAPH STOOD.", held);
        break;
    case S_HALL_UMBRELLA:
        take_or(F_HANDLE_TAKEN, I_PUMPHANDLE,
                "AMONG THE WALKING STICKS IN THE STAND, ONE IS NOT A STICK: AN IRON PUMP HANDLE. YOU TAKE IT.",
                "WALKING STICKS. NONE OF THEM IS ANY USE.", held);
        break;
    case S_HALL_STAIRS_DARK:
        if (held == I_LAMP && wth_flag(F_LAMP_LIT)) {
            wth_set(F_STAIRS_LIT);
            wth_take_away(I_LAMP);
            sfx_play_name("wth_click");
            wth_say("YOU HOLD THE LAMP UP THE STAIRWELL. A SMALL CIRCLE OF LIGHT. IT WILL HAVE TO DO.");
        } else if (held == I_LAMP) wth_say(wth_flag(F_LAMP_FILLED) ? "THE LAMP ISN'T LIT." : "THE LAMP'S TANK IS DRY.");
        else if (held) nothing();
        else wth_say("THE STAIRS GO UP INTO A DARK SO THICK IT FEELS WET. NOT WITHOUT A LIGHT.");
        break;
    /* the parlour */
    case S_PAR_IRONS:
        take_or(F_POKER_TAKEN, I_POKER, "A STAND OF FIRE IRONS. YOU TAKE THE POKER, THE HEAVIEST.",
                "A SHOVEL AND TONGS. NOTHING HEAVY ENOUGH.", held);
        break;
    case S_PAR_RADIO:
        if (held == I_SCREWDRIVER && !wth_flag(F_MAGNET_TAKEN)) {
            wth_set(F_MAGNET_TAKEN);
            wth_give(I_MAGNET);
            screw_job();
            wth_say("YOU UNSCREW THE BACK OF THE RADIOGRAM. THE LOUDSPEAKER'S HEAVY MAGNET COMES AWAY IN YOUR HAND.");
        } else if (held) nothing();
        else if (!wth_flag(F_POWER)) wth_say("A RADIOGRAM, SCREWED UP AT THE BACK. IT'S DEAD: NO POWER IN THE HOUSE.");
        else {
            wth_noise(false);
            sfx_play_name("wth_static");
            wth_say("YOU TURN THE KNOB. A ROAR OF STATIC AT FULL BLAST, AND UNDER IT SOMETHING LIKE BREATHING. YOU "
                    "SNAP IT OFF. TOO LATE.");
        }
        break;
    case S_PAR_SOFA: page_or(s, "A SAGGING SOFA. THE CUSHIONS ARE STIFF WITH SOMETHING THAT DRIED.", held); break;
    /* the dining room */
    case S_DIN_PIANOLA:
        if (held) nothing();
        else {
            wth_noise(false);
            sfx_play_name("wth_orch");
            wth_say("YOU PUMP THE PEDALS. THE PIANOLA HAMMERS OUT A MUSIC-HALL SONG AT FULL TILT, THE KEYS GOING DOWN "
                    "ON THEIR OWN. YOU STOP. THE HOUSE HAS HEARD.");
        }
        break;
    case S_DIN_SIDE:
        take_or(F_HONEY_TAKEN, I_HONEY,
                "ON THE SIDEBOARD, A JAR OF PALE HONEY LABELLED FOR OUR GUEST. YOU TAKE IT, AND YOU DON'T EAT IT.",
                "A SIDEBOARD. THE SILVER HAS GONE.", held);
        break;
    /* the kitchen */
    case S_KIT_SALT:
        take_or(F_SALT_TAKEN, I_SALT, "A SALT BOX ON THE WALL, HALF FULL OF ROCK SALT. YOU TAKE A FISTFUL.",
                "THE SALT BOX.", held);
        break;
    case S_KIT_KNIVES:
        take_or(F_KNIFE_TAKEN, I_KNIFE, "A KNIFE BLOCK. YOU DRAW OUT THE LONGEST ONE.",
                "THE OTHER SLOTS IN THE BLOCK ARE EMPTY. SOMEONE ELSE HAS THE REST.", held);
        break;
    case S_KIT_BOOK: page_or(s, "COOKERY BOOKS. EVERY RECIPE HAS HAD HONEY WRITTEN INTO IT IN PENCIL.", held); break;
    /* the scullery */
    case S_SCU_ICEBOX:
        if (held == I_SALT && !wth_flag(F_BEE_ICE)) {
            wth_set(F_BEE_ICE);
            wth_take_away(I_SALT);
            wth_give(I_BEE_BRASS);
            wth_say("YOU RUB THE ROCK SALT INTO THE BLOCK OF ICE. IT SWEATS, AND PITS, AND SOMETHING SMALL AND BRASS "
                    "DROPS OUT INTO YOUR HAND: A BEE.");
        } else if (held) nothing();
        else if (wth_flag(F_BEE_ICE)) wth_say("AN ICE CHEST WITH A HOLLOWED-OUT BLOCK IN IT.");
        else wth_say("AN ICE CHEST. ONE BIG BLOCK OF ICE, AND FROZEN IN THE MIDDLE OF IT SOMETHING THAT GLINTS. IT "
                     "WON'T MELT ON ITS OWN, NOT TONIGHT.");
        break;
    case S_SCU_SINK:
        if (held == I_BUCKET) {
            wth_take_away(I_BUCKET);
            wth_give(I_WATER);
            wth_say("YOU WORK THE SCULLERY PUMP AND FILL THE BUCKET TO THE BRIM.");
        } else if (held) nothing();
        else wth_say("A STONE SINK WITH A HAND PUMP OVER IT. THE WATER COMES UP COLD AND CLEAR.");
        break;
    case S_SCU_BUCKET:
        take_or(F_BUCKET_TAKEN, I_BUCKET, "UNDER THE SINK, A GALVANISED BUCKET. YOU TAKE IT.", "", held);
        break;
    case S_SCU_DRAWER:
        take_or(F_TAPKEY_TAKEN, I_TAPKEY, "IN THE DRESSER DRAWER, AMONG STRING AND CANDLE ENDS: A LITTLE TAP KEY.",
                "STRING AND CANDLE ENDS.", held);
        break;
    case S_SCU_FUSEBOX:
        if (held == I_GOODFUSE) {
            wth_set(F_POWER);
            wth_take_away(I_GOODFUSE);
            sfx_play_name("wth_click");
            wth_say("YOU PUSH THE MENDED FUSE HOME. SOMEWHERE A FRIDGE SHUDDERS AND HUMS. THE HOUSE HAS POWER AGAIN.");
        } else if (held == I_FUSE) wth_say("IT'S BLOWN. IT WOULD NEED NEW WIRE.");
        else if (held) nothing();
        else if (wth_flag(F_POWER)) wth_say("THE FUSE BOX HUMS.");
        else wth_say("A FUSE BOX. ONE CARRIER IS MISSING: THE ONE MARKED STUDY AND PARLOUR.");
        break;
    case S_SCU_DIARY: case S_SHE_DIARY: case S_STU_DIARY: case S_BAT_DIARY: case S_NUR_DIARY: case S_CHI_DIARY:
    case S_ATT_DIARY: case S_CEL_DIARY: case S_DIN_DIARY:
        page_or(s, NULL, held);
        break;
    /* the smoking room */
    case S_SMO_CABINET:
        if (held == I_POKER && !wth_flag(F_CABINET_BROKEN)) {
            wth_set(F_CABINET_BROKEN);
            wth_give(I_CROSSBOW);
            wth_noise(false);
            sfx_play_name("wth_glass");
            wth_say("YOU PUT THE POKER THROUGH THE GLASS FRONT. THE CRASH GOES ON AND ON. YOU LIFT THE CROSSBOW OFF "
                    "ITS PEGS.");
        } else if (held == I_KNIFE && !wth_flag(F_CABINET_BROKEN)) {
            wth_say("THE GLASS IS THICK. YOU'D NEED SOMETHING HEAVY.");
        } else if (held) nothing();
        else if (wth_flag(F_CABINET_BROKEN)) wth_say("A GUN CABINET WITH NO GLASS AND NOTHING ON ITS PEGS.");
        else wth_say("A GUN CABINET WITH A GLASS FRONT, LOCKED. INSIDE, AN OLD CROSSBOW ON TWO PEGS. THE KEY IS NOWHERE.");
        break;
    case S_SMO_PIPES:
        take_or(F_MATCHES_TAKEN, I_MATCHES, "A PIPE RACK, AND BESIDE IT A BOX OF MATCHES. YOU POCKET THEM.",
                "PIPES THAT SMELL OF SOMETHING SWEETER THAN TOBACCO.", held);
        break;
    case S_SMO_CHAIR:
        page_or(s, "A WING CHAIR. THE SEAT IS DENTED IN THE SHAPE OF SOMEONE WHO SAT A VERY LONG TIME.", held);
        break;
    /* the washroom */
    case S_WAS_CISTERN:
        take_or(F_TAPE_TAKEN, I_TAPE,
                "YOU LIFT THE LID OF THE HIGH CISTERN. TAPED UNDER IT, IN A BREAD BAG: A SPOOL OF RECORDING TAPE.",
                "A HIGH IRON CISTERN ON BRACKETS.", held);
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
    case S_GUE_DRESSER: page_or(s, "A DRESSER WITH ITS MIRROR TURNED TO THE WALL.", held); break;
    case S_GUE_SEW:
        take_or(F_FUSEWIRE_TAKEN, I_FUSEWIRE, "A SEWING BOX. AMONG THE BUTTONS, A CARD OF FUSE WIRE.",
                "BUTTONS, AND A SHROUD HALF SEWN.", held);
        break;
    /* the glasshouse */
    case S_GLA_LAMP:
        take_or(F_LAMP_TAKEN, I_LAMP, "A HURRICANE LAMP HANGS FROM A HOOK. ITS TANK IS DRY. YOU TAKE IT DOWN.", "", held);
        break;
    case S_GLA_SULPHATE:
        take_or(F_SULPHATE_TAKEN, I_SULPHATE,
                "A SACK OF BLUE CRYSTALS: COPPER SULPHATE, FOR THE VINES. YOU TWIST SOME UP IN A PAPER.",
                "THE SACK OF BLUE CRYSTALS.", held);
        break;
    case S_GLA_TROWEL:
        take_or(F_TROWEL_TAKEN, I_TROWEL, "A POTTING TRAY, AND IN IT A TROWEL.", "A POTTING TRAY.", held);
        break;
    case S_GLA_PANE:
        if (held == I_POKER) {
            wth_noise(false);
            sfx_play_name("wth_glass");
            wth_say("YOU JAB THE POKER THROUGH THE CRACKED PANE. GLASS RAINS DOWN WITH A CRASH LIKE A GUNSHOT.");
        } else if (held) nothing();
        else wth_say("A CRACKED PANE IN THE ROOF. ONE GOOD KNOCK WOULD BRING IT DOWN.");
        break;
    /* the garage */
    case S_GAR_SACK:
        if (held == I_TROWEL && !wth_flag(F_LIME_TAKEN)) {
            if (!wth_has(I_GLOVES)) wth_say("THE QUICKLIME BITES YOUR FINGERS AS SOON AS YOU GET NEAR. YOU'D NEED GLOVES.");
            else {
                wth_set(F_LIME_TAKEN);
                wth_take_away(I_TROWEL);
                wth_take_away(I_GLOVES);
                wth_give(I_LIME);
                wth_say("WITH THE GLOVES ON, YOU DIG A TROWELFUL OF QUICKLIME OUT OF THE SACK AND FOLD IT IN NEWSPAPER.");
            }
        } else if (held) nothing();
        else wth_say("A SACK OF BUILDER'S QUICKLIME, SPLIT AT THE SEAM. IT BURNS SKIN.");
        break;
    case S_GAR_SCREW: take_or(F_SCREW_TAKEN, I_SCREWDRIVER, "A SCREWDRIVER ON THE TOOL BOARD. YOU TAKE IT.", "", held); break;
    case S_GAR_GREASE: take_or(F_GREASE_TAKEN, I_GREASE, "A FLAT TIN OF AXLE GREASE. YOU TAKE IT.", "", held); break;
    case S_GAR_SHELF:
        take_or(F_PARAFFIN_TAKEN, I_PARAFFIN, "ON THE SHELF, AMONG EMPTY HONEY JARS: A CAN OF PARAFFIN.",
                "EMPTY HONEY JARS, WASHED AND WAITING.", held);
        break;
    case S_GAR_CAR:
        if (held == I_ROTOR) {
            wth_set(F_ROTOR_FITTED);
            wth_take_away(I_ROTOR);
            sfx_play_name("wth_click");
            wth_say("YOU LIFT THE BONNET, PRISE OFF THE DISTRIBUTOR CAP AND PRESS THE ROTOR ARM ONTO ITS SPINDLE.");
        } else if (held == I_CARKEY) {
            if (wth_flag(F_ROTOR_FITTED) && wth_flag(F_GARAGE_OPEN)) wth_end_game(END_CAR);
            else {
                wth_noise(false);
                sfx_play_name("wth_engine");
                wth_say(!wth_flag(F_ROTOR_FITTED)
                            ? "THE STARTER WHINES AND WHINES, LOUD IN THE SHUT GARAGE, AND THE ENGINE WON'T CATCH. "
                              "SOMETHING IS MISSING UNDER THE BONNET."
                            : "THE ENGINE ROARS INTO LIFE, DEAFENING IN THE SHUT GARAGE. THE DOOR IS DOWN. YOU KILL "
                              "THE ENGINE, SHAKING.");
            }
        } else if (held) nothing();
        else page_or(s, "A BIG GREEN ROVER. THE DOORS AREN'T LOCKED, BUT THERE ARE NO KEYS IN IT.", 0);
        break;
    case S_GAR_DOOR:
        if (held == I_CUTTERS && !wth_flag(F_CHAIN_CUT)) {
            wth_set(F_CHAIN_CUT);
            cutters_job();
            sfx_play_name("wth_clank");
            wth_say("YOU BITE THROUGH THE CHAIN THAT HOLDS THE GARAGE DOOR DOWN TO ITS RING. IT FALLS WITH A CLATTER.");
        } else if (held) nothing();
        else if (wth_flag(F_CHAIN_CUT)) {
            wth_set(F_GARAGE_OPEN);
            wth_noise(false);
            sfx_play_name("wth_rumble");
            wth_say("YOU HEAVE THE DOOR UP. IT THUNDERS UP ITS RAILS, LOUD AS A GOODS TRAIN. COLD AIR. THE DRIVE.");
        } else wth_say("THE GARAGE DOOR IS CHAINED DOWN TO A RING IN THE FLOOR, AND THE CHAIN IS PADLOCKED.");
        break;
    /* the pond */
    case S_POND_BOAT:
        take_or(F_TACKLE_TAKEN, I_TACKLE, "A ROWING BOAT, SUNK TO ITS GUNWALES AT THE BANK. UNDER THE SEAT: A TACKLE BOX.",
                "THE SUNKEN BOAT. THE OARS ARE GONE.", held);
        break;
    /* the shed */
    case S_SHE_PLIERS: take_or(F_PLIERS_TAKEN, I_PLIERS, "PLIERS ON THE PEGBOARD. YOU TAKE THEM.", "", held); break;
    case S_SHE_CUTTERS: take_or(F_CUTTERS_TAKEN, I_CUTTERS, "BOLT CUTTERS ON THE PEGBOARD. YOU TAKE THEM.", "", held); break;
    case S_SHE_SMOKER: take_or(F_SMOKER_TAKEN, I_SMOKER, "A BEE SMOKER ON THE SHELF. YOU TAKE IT.", "", held); break;
    case S_SHE_SPRAYER:
        take_or(F_SPRAYER_TAKEN, I_SPRAYER, "A KNAPSACK SPRAYER FOR THE ROSES. YOU SHRUG IT ON.", "", held);
        break;
    case S_SHE_GLOVES: take_or(F_GLOVES_TAKEN, I_GLOVES, "LEATHER GLOVES ON A SACK. YOU TAKE THEM.", "", held); break;
    /* the back garden */
    case S_GDN_WELL:
        if (held == I_CUTTERS && !wth_flag(F_GRATE_CUT)) {
            wth_set(F_GRATE_CUT);
            cutters_job();
            sfx_play_name("wth_clank");
            wth_say("YOU CUT THE PADLOCK OFF THE WELL GRATE AND HEAVE IT ASIDE. COLD AIR COMES UP, AND A SMELL OF "
                    "MUSHROOMS.");
        } else if (held == I_MAGNETLINE && !wth_flag(F_BEE_WELL)) {
            if (!wth_flag(F_GRATE_CUT)) wth_say("THE GRATE IS IN THE WAY.");
            else {
                wth_set(F_BEE_WELL);
                wth_take_away(I_MAGNETLINE);
                wth_give(I_BEE_COPPER);
                wth_say("YOU LOWER THE MAGNET DOWN THE WELL, A LONG WAY, AND TROLL IT ABOUT IN THE DARK. A CLICK. YOU "
                        "REEL IN A COPPER BEE WITH AN IRON PIN THROUGH IT.");
            }
        } else if (held == I_MAGNET) wth_say("IT'S A LONG WAY DOWN. THE MAGNET WOULD NEED A LINE.");
        else if (held) nothing();
        else if (!wth_flag(F_GRATE_CUT)) wth_say("AN OLD WELL WITH AN IRON GRATE PADLOCKED OVER IT. FAR DOWN, SOMETHING GLINTS.");
        else if (!wth_flag(F_BEE_WELL)) wth_say("THE OPEN WELL. A LONG WAY DOWN, SOMETHING GLINTS IN THE WATER.");
        else wth_say("THE OPEN WELL. COLD AIR, AND A SMELL OF MUSHROOMS.");
        break;
    case S_GDN_HOSE:
        if (held == I_KNIFE && !wth_flag(F_HOSE_TAKEN)) {
            wth_set(F_HOSE_TAKEN);
            wth_give(I_HOSE);
            wth_say("YOU CUT A GOOD LENGTH OF HOSE OFF THE REEL.");
        } else if (held) nothing();
        else if (wth_flag(F_HOSE_TAKEN)) wth_say("A HOSE REEL WITH A SHORT CUT END.");
        else wth_say("A GARDEN HOSE ON A REEL, PERISHED BUT WHOLE. YOU COULD CUT A LENGTH OFF.");
        break;
    case S_GDN_HIVES:
        if (held == I_KNIFE && !wth_flag(F_HIVE_CUT)) {
            wth_set(F_HIVE_CUT);
            wth_give(I_CARKEY);
            wth_say("YOU SLICE THROUGH THE WAX SEALING THE NEAREST HIVE AND LIFT A FRAME. DEAD BEES, PALE COMB, AND "
                    "PRESSED INTO THE COMB: A CAR KEY.");
        } else if (held) nothing();
        else if (wth_flag(F_HIVE_CUT)) wth_say("THE OPENED HIVE. YOU DON'T LOOK IN AGAIN.");
        else wth_say("THREE BEEHIVES. NOT ONE HUMS. EVERY ENTRANCE AND EVERY LID IS SEALED WITH WHITE WAX.");
        break;
    case S_GDN_SHEDDOOR:
        if (held == I_PHOTO) {
            wth_set(F_SHED_OPEN);
            wth_take_away(I_PHOTO);
            sfx_play_name("wth_door");
            wth_say("THE PADLOCK HAS FOUR NUMBER WHEELS. YOU TURN THEM TO THE DATE ON THE PHOTOGRAPH: 4, 6, 7, 1. "
                    "IT OPENS.");
        } else if (held) nothing();
        else wth_say("THE POTTING SHED. A PADLOCK WITH FOUR LITTLE NUMBER WHEELS.");
        break;
    /* the side yard */
    case S_YRD_BUTT:
        if (held == I_TAPKEY && !wth_flag(F_BUTT_DRAINED)) {
            wth_set(F_BUTT_DRAINED);
            wth_set(F_TIN_TAKEN);
            wth_take_away(I_TAPKEY);
            wth_give(I_TIN);
            sfx_play_name("wth_drain");
            wth_say("YOU FIT THE TAP KEY AND TURN. THE WATER BUTT EMPTIES ACROSS THE YARD, AND SOMETHING CLANKS IN THE "
                    "BOTTOM. YOU FISH OUT A TOBACCO TIN.");
        } else if (held) nothing();
        else if (wth_flag(F_BUTT_DRAINED)) wth_say("AN EMPTY WATER BUTT.");
        else wth_say("A WATER BUTT, FULL TO THE BRIM. THE TAP AT THE BOTTOM HAS NO HANDLE, ONLY A SQUARE SPINDLE.");
        break;
    case S_YRD_KENNEL:
        take_or(F_COLLAR_TAKEN, I_BUREAUKEY,
                "AN EMPTY KENNEL. ON THE CHAIN, A COLLAR, AND ON THE COLLAR A SMALL BRASS KEY. YOU TAKE THE KEY.",
                "AN EMPTY KENNEL AND A COLLAR ON A CHAIN. THE NAME ON THE TAG IS SCRATCHED OUT.", held);
        break;
    case S_YRD_PINE:
        take_or(F_CONES_TAKEN, I_CONES, "UNDER THE PINE, DRY CONES. YOU GATHER AN ARMFUL.", "A BLACK PINE. IT CREAKS.", held);
        break;
    /* the drive */
    case S_DRV_GARAGE:
        if (held) nothing();
        else wth_say("THE GARAGE DOOR. SHUT, AND HELD FROM THE INSIDE.");
        break;
    case S_DRV_HATCH:
        if (held == I_BLACKKEY) {
            wth_set(F_HATCH_OPEN);
            wth_take_away(I_BLACKKEY);
            sfx_play_name("wth_door");
            wth_say("THE BLACK KEY FITS. THE CELLAR DOORS LIFT ON A BREATH OF WARM, SWEET AIR.");
        } else if (held) nothing();
        else wth_say("TWO SLOPING CELLAR DOORS IN THE GRAVEL, CHAINED, WITH A BIG BLACK PADLOCK.");
        break;
    /* the hedge court */
    case S_CRT_HIVE: stone_hive(held); break;
    case S_CRT_GATE:
        if (held) nothing();
        else wth_say("AN IRON GATE INTO THE HEDGES. IT WON'T MOVE AN INCH. BESIDE IT, THE STONE HIVE.");
        break;
    /* the heart of the hedges */
    case S_HRT_OAK:
        if (held == I_PLIERS && !wth_flag(F_BOLT_TAKEN)) {
            wth_set(F_BOLT_TAKEN);
            wth_take_away(I_PLIERS);
            wth_give(I_BOLT);
            wth_say("WITH THE PLIERS YOU WORK THE BOLT BACK AND FORTH IN THE OAK UNTIL IT COMES OUT.");
        } else if (held == I_KNIFE && !wth_flag(F_BOLT_TAKEN)) {
            wth_say("THE BOLT IS IN TOO DEEP TO DIG OUT WITH A KNIFE.");
        } else if (held) nothing();
        else if (wth_flag(F_BOLT_TAKEN)) wth_say("AN OLD OAK STUCK FULL OF HOLES, AND ONE NEW ONE.");
        else wth_say("AN OLD OAK IN THE MIDDLE OF THE HEDGES, STUCK FULL OF OLD HOLES, AS IF SOMEONE PRACTISED ON IT "
                     "FOR YEARS. ONE CROSSBOW BOLT IS STILL DRIVEN DEEP IN THE TRUNK.");
        break;
    /* upstairs */
    case S_LAN_HATCH:
        if (held == I_POLE) {
            wth_set(F_HATCH_DOWN);
            wth_take_away(I_POLE);
            sfx_play_name("wth_creak");
            wth_say("YOU HOOK THE RING ON THE HATCH WITH THE WINDOW POLE AND PULL. A LADDER SLIDES DOWN WITH A SHRIEK.");
        } else if (held) nothing();
        else wth_say("A HATCH IN THE CEILING WITH A RING ON IT. FAR TOO HIGH TO REACH.");
        break;
    case S_LAN_MASTER_SHUT:
        if (held == I_POKER) {
            wth_set(F_MASTER_OPEN);
            sfx_play_name("wth_creak");
            wth_say("YOU JAM THE POKER INTO THE JAMB AND LEAN ON IT. THE SWOLLEN DOOR GIVES WITH A CRACK.");
        } else if (held == I_SCREWDRIVER) {
            wth_set(F_MASTER_OPEN);
            wth_say("YOU UNSCREW THE HINGES, ALL TWELVE SCREWS, AND LIFT THE DOOR OUT OF ITS FRAME.");
        } else if (held) nothing();
        else wth_say("THE BIG BEDROOM DOOR. IT ISN'T LOCKED: THE DAMP HAS SWOLLEN IT SOLID IN ITS FRAME. YOU COULD "
                     "LEVER IT, OR TAKE IT OFF ITS HINGES.");
        break;
    case S_MAS_BED:
        page_or(s, "A FOUR-POSTER BED. ONE SIDE IS NEATLY MADE. THE OTHER IS A NEST OF WHITE THREADS.", held);
        break;
    case S_MAS_POST: take_or(F_BELT_TAKEN, I_BELT, "A MAN'S BELT HANGS ON THE BEDPOST. YOU TAKE IT.", "A CARVED BEDPOST.", held); break;
    case S_MAS_GRAMOPHONE:
        if (held) nothing();
        else {
            wth_noise(false);
            sfx_play_name("wth_organ");
            wth_say("YOU WIND THE GRAMOPHONE AND DROP THE NEEDLE. A BRASS BAND BLARES OUT OF THE HORN. YOU SNATCH THE "
                    "NEEDLE OFF. TOO LATE.");
        }
        break;
    case S_BAL_VIEW:
        if (held == I_BINOCULARS && !wth_flag(F_SKETCH_TAKEN)) {
            wth_set(F_SKETCH_TAKEN);
            wth_take_away(I_BINOCULARS);
            wth_give(I_SKETCH);
            wth_say("THROUGH THE BINOCULARS THE HEDGES ARE A PUZZLE ON A PLATE. YOU TRACE THE WAY IN AND SKETCH IT ON "
                    "THE BACK OF YOUR HAND: LEFT, LEFT, RIGHT, STRAIGHT ON, LEFT, RIGHT.");
        } else if (held) nothing();
        else wth_say("THE HEDGES LIE BELOW IN THE DARK, A KNOT OF PATHS TOO FAR OFF TO MAKE OUT.");
        break;
    case S_STU_BUREAU:
        if (held == I_BUREAUKEY) {
            wth_set(F_BINOCULARS_TAKEN);
            wth_take_away(I_BUREAUKEY);
            wth_give(I_BINOCULARS);
            wth_say("THE KEY FITS THE BUREAU. INSIDE THE ROLL-TOP, IN A LEATHER CASE: ARMY BINOCULARS.");
        } else if (held) nothing();
        else if (wth_flag(F_BINOCULARS_TAKEN)) wth_say("AN EMPTY BUREAU.");
        else wth_say("A ROLL-TOP BUREAU, LOCKED.");
        break;
    case S_STU_RECORDER:
        if (held == I_REEL && !wth_flag(F_REEL_FITTED)) {
            wth_set(F_REEL_FITTED);
            wth_take_away(I_REEL);
            wth_say("YOU PUT THE EMPTY SPOOL ON THE RIGHT-HAND SPINDLE.");
        } else if (held == I_TAPE && !wth_flag(F_TAPE_FITTED)) {
            wth_set(F_TAPE_FITTED);
            wth_take_away(I_TAPE);
            wth_say("YOU PUT THE TAPE ON THE LEFT-HAND SPINDLE AND THREAD IT PAST THE HEADS.");
        } else if (held) nothing();
        else if (!wth_flag(F_POWER)) wth_say("A REEL-TO-REEL TAPE RECORDER. THE LIGHT IS OUT: NO POWER IN THE HOUSE.");
        else if (!wth_flag(F_TAPE_FITTED)) wth_say("THE TAPE RECORDER HUMS. IT HAS NOTHING TO PLAY.");
        else if (!wth_flag(F_REEL_FITTED)) wth_say("THE TAPE HAS NOWHERE TO WIND: THE RIGHT-HAND SPINDLE IS EMPTY.");
        else {
            wth_set(F_RECIPE);
            wth_say("YOU PRESS PLAY. TOM'S VOICE, VERY CALM: TO KILL IT, MIX BLUE COPPER CRYSTALS AND QUICKLIME IN "
                    "WATER, IN THE ROSE SPRAYER, AND SPRAY IT IN ITS FACE. FIRE WON'T DO IT ALONE. THEN HE LAUGHS FOR "
                    "A LONG TIME, AND THE TAPE RUNS OUT.");
        }
        break;
    case S_BAT_CURTAIN:
        if (held) nothing();
        else {
            gfx_set_flash(2);
            wth_say("YOU PULL THE SHOWER CURTAIN BACK. BEHIND IT, WHITE THREADS HAVE GROWN UP OUT OF THE PLUGHOLE INTO "
                    "THE SHAPE OF A MAN, STANDING, ARMS AT HIS SIDES. ITS HEAD TURNS TOWARD YOU.");
        }
        break;
    case S_BAT_CABINET:
        take_or(F_RAZOR_TAKEN, I_RAZOR, "IN THE BATHROOM CABINET: A CUT-THROAT RAZOR.", "SHAVING SOAP, DRIED TO CHALK.", held);
        break;
    case S_NUR_HORSE:
        if (held == I_SCREWDRIVER && !wth_flag(F_BEE_HORSE)) {
            wth_set(F_BEE_HORSE);
            wth_give(I_BEE_SILVER);
            screw_job();
            wth_say("YOU UNSCREW THE LITTLE HATCH IN THE ROCKING HORSE'S BELLY. A SILVER BEE FALLS INTO YOUR PALM.");
        } else if (held) nothing();
        else wth_say("A ROCKING HORSE WITH A SCREWED-ON HATCH IN ITS BELLY. IT ROCKS, VERY GENTLY, WHEN NOTHING IS "
                     "TOUCHING IT.");
        break;
    case S_CHI_POLE:
        take_or(F_POLE_TAKEN, I_POLE, "A WINDOW POLE LEANS BY THE HIGH SASH WINDOW. YOU TAKE IT.", "", held);
        break;
    case S_ATT_NEST:
        if (held == I_SMOKER && !wth_flag(F_BEE_NEST)) {
            if (wth_flag(F_SMOKER_LIT)) {
                wth_set(F_BEE_NEST);
                wth_take_away(I_SMOKER);
                wth_give(I_BEE_IRON);
                wth_say("YOU PUFF SMOKE INTO THE NEST UNTIL THE WASPS DROP, DOZY, TO THE BOARDS. YOU REACH INTO THE "
                        "PAPER AND FIND AN IRON BEE.");
            } else wth_say(wth_flag(F_SMOKER_LOADED) ? "THE SMOKER ISN'T LIT." : "THE SMOKER IS EMPTY.");
        } else if (held == I_KNIFE && !wth_flag(F_BEE_NEST)) {
            wth_say("THE WASPS POUR OUT AT THE FIRST TOUCH. YOU BACK AWAY FAST.");
        } else if (held) nothing();
        else if (wth_flag(F_BEE_NEST)) wth_say("A TORN WASPS' NEST. THE WASPS ARE STILL ASLEEP.");
        else wth_say("A WASPS' NEST AS BIG AS A HEAD, HANGING FROM A RAFTER. THEY'RE ALIVE AND ANGRY. SOMETHING IRON "
                     "GLINTS INSIDE THE PAPER.");
        break;
    case S_ATT_BOXES:
        take_or(F_REEL_TAKEN, I_REEL, "A BOX OF HOME MOVIES. AMONG THE FILM CANS, AN EMPTY TAPE SPOOL. YOU TAKE IT.",
                "FILM CANS LABELLED SUMMER 1986, BEFORE THE HOUSE.", held);
        break;
    /* the cellar */
    case S_CEL_PUMP: pump(held); break;
    case S_CEL_DOOR:
        if (held) nothing();
        else wth_say("THE INNER DOORWAY IS FULL OF BLACK WATER UP TO THE LINTEL. A HAND PUMP STANDS BESIDE IT.");
        break;
    /* the white room */
    case S_SPO_FACE:
        if (held == I_MIXTURE) wth_end_game(END_CELLAR);
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
    if (pair(a, b, I_FUSE, I_FUSEWIRE)) {
        wth_take_away(I_FUSE);
        wth_take_away(I_FUSEWIRE);
        wth_give(I_GOODFUSE);
        wth_say("YOU PICK OUT THE BURNT WIRE AND WIND A NEW LENGTH ROUND THE TERMINALS. A MENDED FUSE.");
        return;
    }
    if (pair(a, b, I_KNIFE, I_TIN)) {
        wth_take_away(I_TIN);
        wth_give(I_ROTOR);
        wth_say("YOU PRISE THE RUSTED TIN OPEN WITH THE KNIFE. INSIDE, WRAPPED IN A HANDKERCHIEF: A CAR'S ROTOR ARM.");
        return;
    }
    if (pair(a, b, I_KNIFE, I_TACKLE)) {
        wth_take_away(I_TACKLE);
        wth_give(I_LINE);
        wth_say("YOU FORCE THE TACKLE BOX'S CATCH WITH THE KNIFE. HOOKS, FLOATS, AND A GOOD SPOOL OF FISHING LINE.");
        return;
    }
    if (pair(a, b, I_MAGNET, I_LINE)) {
        wth_take_away(I_MAGNET);
        wth_take_away(I_LINE);
        wth_give(I_MAGNETLINE);
        wth_say("YOU TIE THE MAGNET ON THE END OF THE FISHING LINE.");
        return;
    }
    if (pair(a, b, I_RAZOR, I_BELT)) {
        wth_take_away(I_RAZOR);
        wth_take_away(I_BELT);
        wth_give(I_WASHER);
        wth_say("YOU CUT A ROUND OF THICK LEATHER OUT OF THE BELT WITH THE RAZOR AND BORE A HOLE IN THE MIDDLE.");
        return;
    }
    if (pair(a, b, I_LAMP, I_PARAFFIN)) {
        wth_set(F_LAMP_FILLED);
        wth_take_away(I_PARAFFIN);
        wth_say("YOU FILL THE LAMP'S TANK WITH PARAFFIN.");
        return;
    }
    if (pair(a, b, I_SMOKER, I_CONES)) {
        wth_set(F_SMOKER_LOADED);
        wth_take_away(I_CONES);
        wth_say("YOU PACK THE SMOKER WITH DRY PINE CONES.");
        return;
    }
    if (pair(a, b, I_MATCHES, I_LAMP)) {
        if (!wth_flag(F_LAMP_FILLED)) { wth_say("THE LAMP'S TANK IS DRY. THE WICK WON'T TAKE."); return; }
        if (wth_flag(F_LAMP_LIT)) { wth_say("THE LAMP IS ALREADY LIT."); return; }
        wth_set(F_LAMP_LIT);
        matches_job();
        wth_say("A MATCH TO THE WICK. THE LAMP CATCHES AND BURNS STEADY BEHIND ITS GLASS.");
        return;
    }
    if (pair(a, b, I_MATCHES, I_SMOKER)) {
        if (!wth_flag(F_SMOKER_LOADED)) { wth_say("THE SMOKER IS EMPTY. THERE'S NOTHING TO LIGHT."); return; }
        if (wth_flag(F_SMOKER_LIT)) { wth_say("THE SMOKER IS ALREADY SMOULDERING."); return; }
        wth_set(F_SMOKER_LIT);
        matches_job();
        wth_say("YOU DROP A LIT MATCH ON THE CONES AND PUMP THE BELLOWS. THICK WHITE SMOKE.");
        return;
    }
    if (pair(a, b, I_CROSSBOW, I_BOLT)) {
        wth_set(F_BOW_LOADED);
        wth_take_away(I_BOLT);
        sfx_play_name("wth_click");
        wth_say("YOU CRANK THE CROSSBOW BACK AND DROP THE BOLT INTO ITS GROOVE. ONE SHOT.");
        return;
    }
    int other = a == I_SPRAYER ? b : b == I_SPRAYER ? a : 0;
    if (other == I_SULPHATE || other == I_LIME || other == I_WATER) {
        if (!wth_flag(F_RECIPE)) {
            wth_say("YOU DON'T KNOW WHAT THAT WOULD DO. NOT YET.");
            return;
        }
        wth.s.mix |= (uint8_t)(other == I_SULPHATE ? 1 : other == I_LIME ? 2 : 4);
        wth_take_away(other);
        if (wth.s.mix == 7) {
            wth_take_away(I_SPRAYER);
            wth_give(I_MIXTURE);
            sfx_play_name("wth_fizz");
            wth_say("THE LAST OF IT GOES IN. THE SPRAYER FIZZES AND TURNS A BRIGHT, SICK BLUE, AND STINKS.");
        } else wth_sayf("YOU TIP THE %s INTO THE SPRAYER AND SHAKE IT.", WTH_ITEM[other].name);
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
        wth_take_away(I_SKETCH);
    }
    wth_keeper_room_changed(from, room);
    wth_room_music();
    wth_save_now();
}
