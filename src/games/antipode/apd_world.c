/* ANTIPODE - Tallow Station: working out the world from the hand-made
 * station (apd_rooms.c, written by tools/antipode/make_station.py from
 * tools/antipode/station.txt) and building a room's tiles. All our own
 * map and rooms: only the structure follows the original (three sectors of
 * rooms joined by doors, teleporters between them, a fourth sector opened by
 * four ciphers). */
#include "apd.h"

const char *const APD_SECTOR_NAME[APD_SECTORS] = {"GANTRY", "BRINE", "COIL", "THE CROWN"};

/* ---- the world, worked out once ------------------------------------------- */
ApdRoom apd_rooms[APD_MAX_ROOMS];
int apd_nrooms;
ApdDoor apd_doors[APD_MAX_DOORS];
int apd_ndoors;
ApdItemDef apd_items[APD_MAX_ITEMS];
int apd_nitems;
int16_t apd_cell[APD_SECTORS][APD_GH][APD_GW];
int apd_npods, apd_nterms;
static int16_t room_first_pod[APD_MAX_ROOMS];
static int16_t tele_room[10][2];
static uint8_t door_pos[APD_MAX_DOORS];

static int edge_type(char c) {
    switch (c) {
    case 'o': return E_DOOR;
    case '1': return E_PASS1;
    case '2': return E_PASS2;
    case '3': return E_PASS3;
    case 'b': return E_BOSS;
    case 'n': return E_NANO;
    case 'h': return E_BARREL;
    case 'm': return E_CRACK;
    case 'e': return E_ZAP;
    case 'z': return E_TARGET;
    default: return E_WALL;
    }
}

static char map_at(const char *const *m, int row, int col) {
    for (int r = 0; r <= row; r++)
        if (!m[r]) return ' ';
    const char *s = m[row];
    return col < (int)strlen(s) ? s[col] : ' ';
}

int apd_room_find(int sector, char id) {
    for (int i = 0; i < apd_nrooms; i++)
        if (apd_rooms[i].sector == sector && apd_rooms[i].id == id) return i;
    return -1;
}

int apd_door_at(int sector, int ax, int ay, int vertical) {
    for (int i = 0; i < apd_ndoors; i++) {
        const ApdDoor *d = &apd_doors[i];
        if (d->sector == sector && d->ax == ax && d->ay == ay && d->vertical == vertical) return i;
    }
    return -1;
}

static int spec_of(int sector, char id) {
    for (int i = 0; i < APD_NSPECS; i++)
        if (APD_SPECS[i].sector == sector && APD_SPECS[i].id == id) return i;
    return -1;
}

/* next token of a things string: returns false at the end */
static bool next_token(const char **p, char *tok, int n) {
    const char *s = *p;
    while (*s == ' ') s++;
    if (!*s) return false;
    int k = 0;
    while (*s && *s != ' ') {
        if (k < n - 1) tok[k++] = *s;
        s++;
    }
    tok[k] = 0;
    *p = s;
    return true;
}

static int item_kind_of(const char *tok, int *var) {
    *var = 0;
    if (!strncmp(tok, "armor", 5)) return IT_ARMOR;
    if (!strncmp(tok, "spare", 5)) return IT_SPARE;
    if (!strncmp(tok, "damper", 6)) return IT_DAMPER;
    if (!strncmp(tok, "mod", 3) && isdigit((unsigned char)tok[3])) { *var = tok[3] - '0'; return IT_MOD; }
    return -1;
}

void apd_world_init(void) {
    static bool done;
    if (done) return;
    done = true;
    memset(apd_cell, 0xff, sizeof apd_cell);
    apd_nrooms = apd_ndoors = apd_nitems = apd_npods = apd_nterms = 0;
    memset(tele_room, 0xff, sizeof tele_room);
    for (int s = 0; s < APD_SECTORS; s++) {
        const char *const *m = APD_MAPS[s];
        /* rooms */
        for (int gy = 0; gy < APD_GH; gy++)
            for (int gx = 0; gx < APD_GW; gx++) {
                char c = map_at(m, gy * 2, gx * 2);
                if (c == '.' || c == ' ' || apd_cell[s][gy][gx] >= 0) continue;
                int w = 1, h = 1;
                while (gx + w < APD_GW && map_at(m, gy * 2, (gx + w) * 2) == c) w++;
                while (gy + h < APD_GH && map_at(m, (gy + h) * 2, gx * 2) == c) h++;
                ApdRoom *r = &apd_rooms[apd_nrooms];
                r->id = c;
                r->sector = (uint8_t)s;
                r->gx = (uint8_t)gx;
                r->gy = (uint8_t)gy;
                r->w = (uint8_t)w;
                r->h = (uint8_t)h;
                r->spec = (int16_t)spec_of(s, c);
                for (int y = 0; y < h; y++)
                    for (int x = 0; x < w; x++) apd_cell[s][gy + y][gx + x] = (int16_t)apd_nrooms;
                apd_nrooms++;
            }
        /* edges */
        for (int gy = 0; gy < APD_GH; gy++)
            for (int gx = 0; gx < APD_GW; gx++) {
                int a = apd_cell[s][gy][gx];
                if (a < 0) continue;
                for (int v = 0; v < 2; v++) {
                    int bx = gx + !v, by = gy + v;
                    if (bx >= APD_GW || by >= APD_GH) continue;
                    int b = apd_cell[s][by][bx];
                    int t = edge_type(v ? map_at(m, gy * 2 + 1, gx * 2) : map_at(m, gy * 2, gx * 2 + 1));
                    if (b < 0 || b == a || t == E_WALL) continue;
                    ApdDoor *d = &apd_doors[apd_ndoors];
                    d->sector = (uint8_t)s; d->type = (uint8_t)t; d->vertical = (uint8_t)v;
                    d->ax = (uint8_t)gx; d->ay = (uint8_t)gy; d->ra = (int16_t)a; d->rb = (int16_t)b;
                    door_pos[apd_ndoors] = v ? 9 : 4;
                    for (int k = 0; k < APD_NDOORPOS; k++) {
                        const ApdDoorPos *p = &APD_DOORPOS[k];
                        if (p->sector == s && p->ax == gx && p->ay == gy && p->vertical == v) door_pos[apd_ndoors] = p->pos;
                    }
                    apd_ndoors++;
                }
            }
    }
    /* items, cradles, terminals and teleporters, in the room table's order */
    for (int i = 0; i < apd_nrooms; i++) {
        ApdRoom *r = &apd_rooms[i];
        r->first_item = (int16_t)apd_nitems;
        r->nitems = 0;
        room_first_pod[i] = -1;
        if (r->spec < 0) continue;
        const char *p = APD_SPECS[r->spec].things;
        char tok[32];
        while (next_token(&p, tok, sizeof tok)) {
            int var, k = item_kind_of(tok, &var);
            if (k >= 0 && apd_nitems < APD_MAX_ITEMS) {
                apd_items[apd_nitems].kind = (uint8_t)k;
                apd_items[apd_nitems].var = (uint8_t)var;
                apd_items[apd_nitems].room = (int16_t)i;
                apd_nitems++;
                r->nitems++;
            } else if (!strncmp(tok, "pod", 3)) {
                if (room_first_pod[i] < 0) room_first_pod[i] = (int16_t)apd_npods;
                apd_npods++;
            } else if (!strncmp(tok, "log", 3) || !strncmp(tok, "pass", 4)) {
                apd_nterms++;
            } else if (!strncmp(tok, "tele", 4)) {
                int l = atoi(tok + 4);
                if (l >= 0 && l < 10) tele_room[l][tele_room[l][0] < 0 ? 0 : 1] = (int16_t)i;
            }
        }
    }
}

int apd_room_pod(int room) { return room >= 0 && room < apd_nrooms ? room_first_pod[room] : -1; }

/* the room at the other end of a teleporter link */
int apd_tele_dest(int link, int from_room);
int apd_tele_dest(int link, int from_room) {
    if (link < 0 || link >= 10) return -1;
    return tele_room[link][0] == from_room ? tele_room[link][1] : tele_room[link][0];
}

/* a doorway's row in its cell (or a shaft's column) */
int apd_door_pos(int door);
int apd_door_pos(int door) { return door >= 0 && door < apd_ndoors ? door_pos[door] : 4; }

const char *apd_room_name(int room) {
    if (room < 0 || room >= apd_nrooms || apd_rooms[room].spec < 0) return "";
    const char *n = APD_SPECS[apd_rooms[room].spec].name;
    return n ? n : APD_SECTOR_NAME[apd_rooms[room].sector];
}

/* ---- building a room ------------------------------------------------------ */

static int mark_x[16], mark_y[16];

static bool in_room(int tx, int ty) { return tx >= 0 && ty >= 0 && tx < ag.rw && ty < ag.rh; }
static int tile(int tx, int ty) { return in_room(tx, ty) ? ag.tile[ty][tx] : T_WALL; }

static void set_t(int tx, int ty, int t) {
    if (in_room(tx, ty)) ag.tile[ty][tx] = (uint8_t)t;
}

static int add_thing(int kind, int var, int tx, int ty, int id) {
    if (ag.nth >= APD_MAX_THINGS) return -1;
    ApdThing *t = &ag.th[ag.nth];
    memset(t, 0, sizeof *t);
    t->kind = (uint8_t)kind;
    t->var = (uint8_t)var;
    t->alive = 1;
    t->tx = (int16_t)tx;
    t->ty = (int16_t)ty;
    t->id = (int16_t)id;
    return ag.nth++;
}

static int foe_kind_of(const char *tok) {
    static const struct { const char *n; int k; } F[] = {
        {"treadr", FO_TREAD_R}, {"treadg", FO_TREAD_G}, {"tread", FO_TREAD}, {"spitter", FO_SPITTER},
        {"eye", FO_STAREYE}, {"hive", FO_HIVE}, {"skip", FO_SKIP}, {"skimmer", FO_SKIMMER},
        {"sentry", FO_SENTRY},
    };
    for (int i = 0; i < ARRAY_LEN(F); i++) {
        size_t n = strlen(F[i].n);
        if (!strncmp(tok, F[i].n, n) && (tok[n] == 0 || tok[n] == '*' || tok[n] == '@')) return F[i].k;
    }
    return 0;
}

void apd_foe_size(int kind, int *w, int *h);

static bool solid_tile_c(int t) { return t == T_WALL || t == T_SPUP || t == T_SPDN || t == T_CRACK || t == T_GREY; }

/* a foe on its mark: standing on a floor, hanging from a ceiling, or in the air */
static void place_foe(int kind, int tx, int ty) {
    int w, h;
    apd_foe_size(kind, &w, &h);
    int x = tx * APD_TILE + 8, y;
    bool floor = solid_tile_c(tile(tx, ty + 1)), ceil = solid_tile_c(tile(tx, ty - 1));
    bool up = false;
    if (floor) y = (ty + 1) * APD_TILE - h / 2;
    else if (ceil) { y = ty * APD_TILE + h / 2; up = true; }
    else y = ty * APD_TILE + 8;
    apd_spawn_foe(kind, x * APD_FX, y * APD_FX, up);
}

static int door_tile(const ApdDoor *d, int di, bool vertical) {
    bool open = ag.run.gate_open[di] != 0;
    switch (d->type) {
    case E_PASS1: case E_PASS2: case E_PASS3: return T_LOCK;
    case E_NANO: case E_TARGET: return open ? (vertical ? T_AIR : T_DOOR) : T_SHUT;
    case E_BARREL: return open ? T_AIR : T_BARREL;
    case E_HIDDEN: return open ? T_AIR : T_FAKE;
    case E_CRACK: return open ? T_AIR : T_CRACK;
    case E_ZAP: return T_ZAP;
    default: return vertical ? T_AIR : T_DOOR;
    }
}

static void put_doors(int room) {
    const ApdRoom *r = &apd_rooms[room];
    for (int di = 0; di < apd_ndoors; di++) {
        const ApdDoor *d = &apd_doors[di];
        if (d->ra != room && d->rb != room) continue;
        bool is_a = d->ra == room;
        int gx = d->ax + (!is_a && !d->vertical ? 1 : 0);
        int gy = d->ay + (!is_a && d->vertical ? 1 : 0);
        int lx = (gx - r->gx) * APD_CW, ly = (gy - r->gy) * APD_CH;
        int pos = door_pos[di];
        int t = door_tile(d, di, d->vertical);
        if (!d->vertical) {
            int bx = is_a ? lx + APD_CW - 1 : lx;      /* the border column */
            /* an electric field across a side doorway stands four tiles tall
             * (from two rows over the doorway's row): room to jump into it and
             * flip, so the STORMBALL carries the suit across in the air */
            int top = d->type == E_ZAP ? pos - 2 : pos;
            for (int row = top; row <= pos + 1; row++) {
                set_t(bx, ly + row, t);
                ag.tdoor[ly + row][bx] = (uint8_t)(di + 1);
            }
            /* a target sits in the wall beside the shutter */
            int ty = tile(bx, ly + pos - 1) == T_WALL && pos > 1 ? ly + pos - 1 : ly + pos + 2;
            if (d->type == E_TARGET && !ag.run.gate_open[di]) add_thing(TH_TARGET, 0, bx, ty, di);
            if (d->type == E_ZAP) {
                add_thing(TH_EMITTER, 0, bx, ly + top - 1, di);
                add_thing(TH_EMITTER, 1, bx, ly + pos + 2, di);
            }
        } else {
            int sy = is_a ? ly + APD_CH - 1 : ly;      /* the slab row */
            for (int col = pos; col <= pos + 1; col++) {
                set_t(lx + col, sy, t);
                ag.tdoor[sy][lx + col] = (uint8_t)(di + 1);
            }
            if (d->type == E_ZAP) {
                add_thing(TH_EMITTER, 2, lx + pos - 1, sy, di);
                add_thing(TH_EMITTER, 3, lx + pos + 2, sy, di);
            }
        }
    }
}

int apd_room_cell_x(int px) { return iclamp(px / APD_CPW, 0, apd_rooms[ag.room].w - 1); }
int apd_room_cell_y(int py) { return iclamp(py / APD_CPH, 0, apd_rooms[ag.room].h - 1); }

void apd_build_room(int room) {
    const ApdRoom *r = &apd_rooms[room];
    const ApdSpec *sp = r->spec >= 0 ? &APD_SPECS[r->spec] : NULL;
    ag.room = room;
    ag.rw = r->w * APD_CW;
    ag.rh = r->h * APD_CH;
    memset(ag.tile, T_WALL, sizeof ag.tile);
    memset(ag.tdoor, 0, sizeof ag.tdoor);
    memset(ag.foe, 0, sizeof ag.foe);
    memset(ag.es, 0, sizeof ag.es);
    memset(ag.shot, 0, sizeof ag.shot);
    memset(ag.th, 0, sizeof ag.th);
    memset(ag.part, 0, sizeof ag.part);
    ag.nth = 0;
    ag.boss_on = false;
    ag.boss_lock = false;
    ag.boss_foe = -1;
    ag.boss = -1;
    ag.lightning_t = 0;
    ag.button_t[0] = ag.button_t[1] = 0;
    ag.shadow.on = false;
    ag.drone.latch = -1;
    for (int k = 0; k < 16; k++) mark_x[k] = mark_y[k] = -1;
    if (!sp) return;
    /* the tiles, as drawn */
    for (int ty = 0; ty < ag.rh; ty++)
        for (int tx = 0; tx < ag.rw; tx++) {
            char c = sp->rows[ty][tx];
            int t = T_AIR;
            switch (c) {
            case '#': t = T_WALL; break;
            case '^': t = T_SPUP; break;
            case 'v': t = T_SPDN; break;
            case '=': t = T_PLAT; break;
            case 'a': t = T_ACID; break;
            case 'X': t = T_CRACK; break;
            case 'G': t = ag.run.secret_open ? T_AIR : T_GREY; break;
            case '%': t = T_PROP; break;
            case 'r': add_thing(TH_BALL, 0, tx, ty, -1); break;
            case 'p': add_thing(TH_BALL, 1, tx, ty, -1); break;
            default:
                if (c >= '0' && c <= '9') { mark_x[c - '0'] = tx; mark_y[c - '0'] = ty; }
                else if (c >= 'A' && c <= 'F') { mark_x[c - 'A' + 10] = tx; mark_y[c - 'A' + 10] = ty; }
                break;
            }
            ag.tile[ty][tx] = (uint8_t)t;
        }
    put_doors(room);
    /* things, each on its mark */
    const char *p = sp->things;
    char tok[32];
    int item_k = 0, pod_k = 0;
    while (next_token(&p, tok, sizeof tok)) {
        const char *at = strchr(tok, '@');
        int m = -1;
        if (at && at[1]) m = isdigit((unsigned char)at[1]) ? at[1] - '0' : at[1] - 'A' + 10;
        if (m >= 16) m = -1;
        int tx = m >= 0 ? mark_x[m] : -1, ty = m >= 0 ? mark_y[m] : -1;
        bool hidden = at && at[1] && at[2] == 'h';
        int var, ik = item_kind_of(tok, &var);
        if (ik >= 0) {
            int item = r->first_item + item_k++;
            if (tx >= 0 && !ag.run.item_got[item]) {
                add_thing(TH_ITEM, hidden, tx, ty, item);
                if (hidden) set_t(tx, ty, T_PROP);
            }
            continue;
        }
        if (!strncmp(tok, "pod", 3)) {
            if (tx >= 0) add_thing(TH_POD, 0, tx, ty, room_first_pod[room] + pod_k);
            pod_k++;
        } else if (!strncmp(tok, "log", 3) || !strncmp(tok, "pass", 4)) {
            bool pass = tok[0] == 'p' && tok[1] == 'a';
            int n = atoi(tok + (pass ? 4 : 3));
            if (tx >= 0) add_thing(TH_TERM, (uint8_t)(pass ? n : 0), tx, ty, pass ? -1 : n);
        } else if (!strncmp(tok, "tele", 4)) {
            if (tx >= 0) add_thing(TH_TELE, 0, tx, ty, atoi(tok + 4));
        } else if (!strncmp(tok, "padf", 4) || !strncmp(tok, "padc", 4)) {
            if (tx >= 0) add_thing(TH_BUTTON, (uint8_t)(tok[3] == 'c'), tx, ty, -1);
        } else if (!strncmp(tok, "start", 5)) {
            if (tx >= 0) {
                ag.pl.entry_x = (tx * APD_TILE + 8) * APD_FX;
                ag.pl.entry_y = ((ty + 1) * APD_TILE - APD_PH / 2) * APD_FX;
                ag.pl.entry_grav = 1;
                ag.pl.entry_face = 1;
            }
        } else if (!strncmp(tok, "boss", 4)) {
            ag.boss = atoi(tok + 4);
        } else {
            int k = foe_kind_of(tok);
            if (k && !ag.no_foes && tx >= 0) place_foe(k, tx, ty);
        }
    }
}

/* ---- tiles and collision ------------------------------------------------- */

int apd_tile_at(int px, int py) {
    if (px < 0 || py < 0) return T_WALL;
    int tx = px / APD_TILE, ty = py / APD_TILE;
    return tile(tx, ty);
}

static bool tile_solid(int t) {
    return t == T_WALL || t == T_SPUP || t == T_SPDN || t == T_CRACK || t == T_BARREL || t == T_DOOR ||
           t == T_LOCK || t == T_SHUT || t == T_FAKE || t == T_GREY;
}

bool apd_solid_tile(int tx, int ty, int grav, int dir_y) {
    (void)grav;
    (void)dir_y;
    if (tx < 0 || tx >= ag.rw) {
        /* off the side of the room: only a door's opening leads on */
        return !(ty >= 0 && ty < ag.rh);
    }
    if (ty < 0 || ty >= ag.rh) return !(tx >= 0 && tx < ag.rw);
    return tile_solid(ag.tile[ty][tx]);
}

/* Is the box (centre cx, cy, size w x h, in pixels) in something solid?
 * One-way plates count only when the box comes onto them along gravity:
 * vy is the movement's direction (0: plates don't count). Outside the room
 * only the door openings are free. */
bool apd_box_solid(int cx, int cy, int w, int h, int grav, int vy) {
    int x0 = cx - w / 2, x1 = cx - w / 2 + w - 1;
    int y0 = cy - h / 2, y1 = cy - h / 2 + h - 1;
    int tx0 = (x0 >= 0 ? x0 : x0 - 15) / APD_TILE, tx1 = (x1 >= 0 ? x1 : x1 - 15) / APD_TILE;
    int ty0 = (y0 >= 0 ? y0 : y0 - 15) / APD_TILE, ty1 = (y1 >= 0 ? y1 : y1 - 15) / APD_TILE;
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++) {
            int t;
            if (tx < 0 || tx >= ag.rw || ty < 0 || ty >= ag.rh) {
                /* beyond the border: free only straight out of an open doorway */
                int bx = iclamp(tx, 0, ag.rw - 1), by = iclamp(ty, 0, ag.rh - 1);
                t = ag.tile[by][bx];
                if (tile_solid(t) || !ag.tdoor[by][bx]) return true;
                continue;
            }
            t = ag.tile[ty][tx];
            if (tile_solid(t)) return true;
            if (t == T_PLAT && vy != 0 && isign(vy) == grav) {
                /* a plate: solid only at its face the feet come down onto */
                int edge = grav > 0 ? ty * APD_TILE : ty * APD_TILE + APD_TILE - 1;
                int feet = grav > 0 ? y1 : y0;
                if (grav > 0 ? (feet - vy <= edge && feet >= edge) : (feet - vy >= edge && feet <= edge)) return true;
            }
        }
    return false;
}

/* ---- gates ------------------------------------------------------------------ */

void apd_open_gate(int door) {
    if (door < 0 || door >= apd_ndoors) return;
    ag.run.gate_open[door] = 1;
    for (int y = 0; y < ag.rh; y++)
        for (int x = 0; x < ag.rw; x++)
            if (ag.tdoor[y][x] == door + 1) {
                int t = ag.tile[y][x];
                if (t != T_ZAP) ag.tile[y][x] = T_AIR;
            }
    for (int i = 0; i < ag.nth; i++)
        if (ag.th[i].kind == TH_TARGET && ag.th[i].id == door) ag.th[i].alive = 0;
}

/* a fight shuts every way out of the arena (and opens them again after) */
void apd_lock_boss_doors(bool lock) {
    ag.boss_lock = lock;
    for (int y = 0; y < ag.rh; y++)
        for (int x = 0; x < ag.rw; x++) {
            int d = ag.tdoor[y][x];
            if (!d) continue;
            const ApdDoor *dd = &apd_doors[d - 1];
            if (lock) ag.tile[y][x] = T_SHUT;
            else ag.tile[y][x] = (uint8_t)door_tile(dd, d - 1, dd->vertical);
        }
}

/* every sentry orb gone: the room's nano-walls open for good */
void apd_open_nano(void) {
    for (int di = 0; di < apd_ndoors; di++) {
        const ApdDoor *d = &apd_doors[di];
        if (d->type == E_NANO && (d->ra == ag.room || d->rb == ag.room) && !ag.run.gate_open[di]) {
            apd_open_gate(di);
            for (int y = 0; y < ag.rh; y++)
                for (int x = 0; x < ag.rw; x++)
                    if (ag.tdoor[y][x] == di + 1 && !d->vertical) ag.tile[y][x] = T_DOOR;
        }
    }
}

/* tests: the room as text */
void apd_room_dump(int room) {
    static const char CH[] = ".#^v=a%BDLSpFzG";
    for (int y = 0; y < ag.rh; y++) {
        char line[APD_RTW + 1];
        for (int x = 0; x < ag.rw; x++) {
            int t = ag.tile[y][x];
            char c = t == T_CRACK ? 'X' : t == T_PROP ? '%' : CH[t < (int)sizeof CH - 1 ? t : 0];
            for (int i = 0; i < ag.nth; i++)
                if (ag.th[i].alive && ag.th[i].tx == x && ag.th[i].ty == y) c = "PTt*obB!"[ag.th[i].kind];
            for (int i = 0; i < APD_MAX_FOES; i++)
                if (ag.foe[i].alive && APD_PX(ag.foe[i].x) / APD_TILE == x && APD_PX(ag.foe[i].y) / APD_TILE == y) c = 'e';
            line[x] = c;
        }
        line[ag.rw] = 0;
        printf("%s\n", line);
    }
    (void)room;
}

/* tests: every room's things found a place (items, cradles, terminals,
 * teleporters, foes), with nothing collected */
int apd_place_check(void);
int apd_place_check(void) {
    ApdGame keep = ag;
    int bad = 0;
    memset(&ag.run, 0, sizeof ag.run);
    for (int r = 0; r < apd_nrooms; r++) {
        apd_build_room(r);
        const ApdSpec *sp = &APD_SPECS[apd_rooms[r].spec];
        int want[4] = {0, 0, 0, 0}, got[4] = {0, 0, 0, 0}, foes = 0, nfoes = 0;
        const char *p = sp->things;
        char tok[32];
        while (next_token(&p, tok, sizeof tok)) {
            int var;
            if (item_kind_of(tok, &var) >= 0) want[0]++;
            else if (!strncmp(tok, "pod", 3)) want[1]++;
            else if (!strncmp(tok, "log", 3) || !strncmp(tok, "pass", 4)) want[2]++;
            else if (!strncmp(tok, "tele", 4)) want[3]++;
            else if (foe_kind_of(tok)) {
                const char *st = strchr(tok, '*');
                foes += st ? atoi(st + 1) : 1;
            }
        }
        for (int i = 0; i < ag.nth; i++) {
            int k = ag.th[i].kind;
            if (k == TH_ITEM) got[0]++;
            if (k == TH_POD) got[1]++;
            if (k == TH_TERM) got[2]++;
            if (k == TH_TELE) got[3]++;
        }
        nfoes = apd_foes_alive();
        if (memcmp(want, got, sizeof want) || foes != nfoes) {
            bad++;
            fprintf(stderr, "room %d %c: items %d/%d pods %d/%d terms %d/%d teles %d/%d foes %d/%d\n", apd_rooms[r].sector,
                    apd_rooms[r].id, got[0], want[0], got[1], want[1], got[2], want[2], got[3], want[3], nfoes, foes);
        }
    }
    ag = keep;
    return bad;
}
