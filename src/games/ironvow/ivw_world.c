/* IRONVOW - exploring: steps and turns, what each tile does, the wandering
 * flames, and everything the context menu (A) can do. Every step and every
 * interaction passes a turn; turning on the spot does not. */
#include "ivw.h"

#define STEP_FRAMES 8
#define TURN_FRAMES 6
#define FLAME_CHASE 7     /* tiles: a flame closer than this (by path) hunts the knight */
#define SPAWN_EVERY 8     /* turns between new flames */

/* ---- the map ------------------------------------------------------------------------------- */

int ivw_tile(int x, int y) {
    if (x < 0 || y < 0 || x >= IVW_W || y >= IVW_H) return T_WALL;
    return ig.w.tile[y][x];
}

static bool tile_solid(int x, int y) {
    int t = ivw_tile(x, y);
    switch (t) {
    case T_WALL: case T_DOOR: case T_LOCKED: case T_GATE: case T_HIDDEN: return true;
    case T_SHIFT_A: return ig.w.shift == 0;
    case T_SHIFT_B: return ig.w.shift == 1;
    case T_EXIT: return !ig.w.escape;
    default: return IVW_IS_DECOR(t);
    }
}

bool ivw_opaque(int x, int y) {
    int t = ivw_tile(x, y);
    return tile_solid(x, y) && t != T_GATE;
}

IvwObj *ivw_obj_at(int x, int y) {
    for (int i = 0; i < ig.w.nobj; i++)
        if (ig.w.obj[i].kind != OB_NONE && ig.w.obj[i].x == x && ig.w.obj[i].y == y) return &ig.w.obj[i];
    return NULL;
}

static bool obj_blocks(int x, int y) {
    IvwObj *o = ivw_obj_at(x, y);
    return o && o->kind != OB_BOSS;
}

bool ivw_walkable(int x, int y) { return !tile_solid(x, y) && !obj_blocks(x, y); }

bool ivw_flame_walkable(int x, int y) {
    return !tile_solid(x, y) && !ivw_obj_at(x, y) && ivw_tile(x, y) != T_STAIRS && ivw_tile(x, y) != T_EXIT;
}

int ivw_flame_at(int x, int y) {
    for (int i = 0; i < IVW_MAX_FLAME; i++)
        if (ig.w.flame[i].alive && ig.w.flame[i].x == x && ig.w.flame[i].y == y) return i;
    return -1;
}

bool ivw_spikes_up(void) { return ((ig.w.turn / 2) & 1) != 0; }

static bool slime_wet(int t) { return (t == T_SLIME_A && !ig.w.slime_b) || (t == T_SLIME_B && ig.w.slime_b); }

/* ---- what the knight can see: the tiles round him and down the corridor ahead ---------------- */
void ivw_reveal_around(void) {
    IvwHero *h = &ig.hero;
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            int x = h->x + dx, y = h->y + dy;
            if (x >= 0 && y >= 0 && x < IVW_W && y < IVW_H) ig.w.seen[y][x] = 1;
        }
    for (int d = 0; d < 4; d++) {
        int x = h->x, y = h->y;
        for (int k = 0; k < 5; k++) {
            x += IVW_DX[d]; y += IVW_DY[d];
            if (x < 0 || y < 0 || x >= IVW_W || y >= IVW_H) break;
            ig.w.seen[y][x] = 1;
            /* the walls beside the corridor */
            for (int s = -1; s <= 1; s += 2) {
                int sx = x + IVW_DX[(d + 1) & 3] * s, sy = y + IVW_DY[(d + 1) & 3] * s;
                if (sx >= 0 && sy >= 0 && sx < IVW_W && sy < IVW_H) ig.w.seen[sy][sx] = 1;
            }
            if (ivw_opaque(x, y)) break;
        }
    }
}

/* ---- drops (items left on the floor stay there) ----------------------------------------------- */
int ivw_drops_here(int x, int y, uint8_t *items, int max) {
    int n = 0;
    for (int i = 0; i < ig.w.ndrop && n < max; i++)
        if (ig.w.drop[i].x == x && ig.w.drop[i].y == y) items[n++] = ig.w.drop[i].item;
    return n;
}

void ivw_drop_item(int item, int x, int y) {
    IvwWorld *w = &ig.w;
    if (w->ndrop >= IVW_MAX_DROP) { memmove(&w->drop[0], &w->drop[1], sizeof w->drop[0] * (IVW_MAX_DROP - 1)); w->ndrop--; }
    w->drop[w->ndrop++] = (IvwDrop){(uint8_t)item, (uint8_t)x, (uint8_t)y};
}

bool ivw_take_drop(int item, int x, int y) {
    IvwWorld *w = &ig.w;
    for (int i = 0; i < w->ndrop; i++)
        if (w->drop[i].item == item && w->drop[i].x == x && w->drop[i].y == y) {
            memmove(&w->drop[i], &w->drop[i + 1], sizeof w->drop[0] * (size_t)(w->ndrop - i - 1));
            w->ndrop--;
            return true;
        }
    return false;
}

/* an item found: into the pack, or at the knight's feet if it is full */
static void receive(int item) {
    if (item == IT_IRONKEY) { ig.hero.keys++; ivw_say("AN IRON KEY."); return; }
    if (ivw_give_item(item)) ivw_say("%s.", IVW_ITEM[item].name);
    else {
        ivw_drop_item(item, ig.hero.x, ig.hero.y);
        ivw_say("%s: THE PACK IS FULL, IT FALLS.", IVW_ITEM[item].name);
    }
}

/* ---- the flames ------------------------------------------------------------------------------- */

/* path distances from the knight over the tiles flames float through */
static int16_t dist_map[IVW_H][IVW_W];
static void flame_distances(void) {
    for (int y = 0; y < IVW_H; y++)
        for (int x = 0; x < IVW_W; x++) dist_map[y][x] = -1;
    static int16_t qx[IVW_W * IVW_H], qy[IVW_W * IVW_H];
    int head = 0, tail = 0;
    dist_map[ig.hero.y][ig.hero.x] = 0;
    qx[tail] = ig.hero.x; qy[tail] = ig.hero.y; tail++;
    while (head < tail) {
        int x = qx[head], y = qy[head]; head++;
        if (dist_map[y][x] >= 12) continue;
        for (int d = 0; d < 4; d++) {
            int nx = x + IVW_DX[d], ny = y + IVW_DY[d];
            if (nx < 0 || ny < 0 || nx >= IVW_W || ny >= IVW_H || dist_map[ny][nx] >= 0) continue;
            if (!ivw_flame_walkable(nx, ny)) continue;
            dist_map[ny][nx] = (int16_t)(dist_map[y][x] + 1);
            qx[tail] = (int16_t)nx; qy[tail] = (int16_t)ny; tail++;
        }
    }
}

static bool lantern_near(int x, int y) {
    for (int dy = -4; dy <= 4; dy++)
        for (int dx = -4; dx <= 4; dx++)
            if (ivw_tile(x + dx, y + dy) == T_LANTERN && ig.w.st[y + dy][x + dx]) return true;
    return false;
}

static int flames_alive(void) {
    int n = 0;
    for (int i = 0; i < IVW_MAX_FLAME; i++) n += ig.w.flame[i].alive;
    return n;
}

static void spawn_flame(void) {
    IvwWorld *w = &ig.w;
    if (w->nspawn == 0 || flames_alive() >= IVW_FLOOR[ig.hero.floor].flame_cap) return;
    int start = rng_range(&ig.rng, 0, w->nspawn - 1);
    for (int k = 0; k < w->nspawn; k++) {
        int i = (start + k) % w->nspawn;
        int x = w->spawn_x[i], y = w->spawn_y[i];
        if (iabs(x - ig.hero.x) + iabs(y - ig.hero.y) < 5) continue;
        if (ivw_flame_at(x, y) >= 0 || !ivw_flame_walkable(x, y) || lantern_near(x, y)) continue;
        for (int f = 0; f < IVW_MAX_FLAME; f++)
            if (!w->flame[f].alive) {
                w->flame[f] = (IvwFlame){1, (uint8_t)x, (uint8_t)y, 0, 0};
                return;
            }
        return;
    }
}

/* a flame on the knight's tile: the fight is on */
static int flame_touch = -1;

static void move_flames(void) {
    IvwWorld *w = &ig.w;
    flame_distances();
    for (int i = 0; i < IVW_MAX_FLAME; i++) {
        IvwFlame *f = &w->flame[i];
        if (!f->alive) continue;
        if (!ivw_flame_walkable(f->x, f->y)) { f->alive = 0; continue; }
        /* three moves in four */
        if (++f->wait >= 4) { f->wait = 0; continue; }
        int d0 = dist_map[f->y][f->x];
        int best = -1, bestd = 999;
        bool hunt = !w->veil && d0 >= 0 && d0 <= FLAME_CHASE;
        int roll = rng_range(&ig.rng, 0, 3);
        for (int k = 0; k < 4; k++) {
            int d = (k + roll) & 3, nx = f->x + IVW_DX[d], ny = f->y + IVW_DY[d];
            if (!ivw_flame_walkable(nx, ny) || ivw_flame_at(nx, ny) >= 0) continue;
            if (nx == ig.hero.x && ny == ig.hero.y && w->veil) continue;
            int nd = hunt ? dist_map[ny][nx] : 0;
            if (nd < 0) continue;
            if (!hunt) { best = d; break; }
            if (nd < bestd) { bestd = nd; best = d; }
        }
        if (best < 0) continue;
        if (hunt && bestd >= d0) continue;
        f->x = (uint8_t)(f->x + IVW_DX[best]);
        f->y = (uint8_t)(f->y + IVW_DY[best]);
        if (f->x == ig.hero.x && f->y == ig.hero.y && flame_touch < 0) flame_touch = i;
    }
}

/* ---- a turn passes --------------------------------------------------------------------------- */

static void hero_hurt_world(int dmg, const char *why) {
    IvwHero *h = &ig.hero;
    h->hp = (int16_t)(h->hp - dmg);
    ivw_sfx("ivw_hurt");
    if (h->hp <= 0) { h->hp = 0; ivw_say("%s", why); ivw_game_over(); }
}

void ivw_pass_turn(void) {
    IvwHero *h = &ig.hero;
    IvwWorld *w = &ig.w;
    w->turn++;
    ig.clock_t = 24;
    /* mana comes back as time passes; stamina refills out of a fight */
    int arm = ivw_armour_item();
    int every = arm == IT_COWL ? 1 : 2;
    if (w->turn % every == 0 && h->mp < ivw_max_mp(h)) h->mp = (int16_t)imin(ivw_max_mp(h), h->mp + 1 + h->stat[STAT_LORE] / 8);
    h->st = (int16_t)imin(ivw_max_st(h), h->st + ivw_max_st(h) / 6);
    if (w->veil) w->veil--;
    if (w->fly) {
        w->fly--;
        if (!w->fly) ivw_say("YOUR FEET FIND THEIR WEIGHT.");
    }
    if (h->poison) {
        h->poison--;
        if (h->hp > 1) h->hp--; /* poison leaves the knight at 1 */
    }
    if (h->burn) {
        h->burn--;
        hero_hurt_world(2, "THE FIRE TAKES YOU.");
        if (ig.state == VS_OVER) return;
    }
    /* what the knight stands on */
    int t = ivw_tile(h->x, h->y);
    if (!w->fly) {
        if (t == T_PIT) { h->hp = 0; ivw_say("YOU FALL INTO THE DARK."); ivw_game_over(); return; }
        if (t == T_LAVA) h->burn = (uint8_t)imax(h->burn, 4);
        if (slime_wet(t)) h->poison = (uint8_t)imax(h->poison, 8);
    }
    if (t == T_SPIKES && ivw_spikes_up()) {
        hero_hurt_world(8, "THE SPIKES FIND YOU.");
        if (ig.state == VS_OVER) return;
        ivw_say("SPIKES!");
    }
    /* the flames */
    flame_touch = -1;
    move_flames();
    if (w->turn % SPAWN_EVERY == 0) spawn_flame();
    if (flame_touch >= 0 && ig.state == VS_EXPLORE && !w->veil) ivw_start_fight_flame(flame_touch);
}

/* the tile the knight just stepped on */
void ivw_touch_tile(void) {
    IvwHero *h = &ig.hero;
    IvwWorld *w = &ig.w;
    ivw_reveal_around();
    int t = ivw_tile(h->x, h->y);
    if (t == T_STAIRS) {
        ig.confirm_kind = 0;
        ig.confirm_sel = 1;
        ivw_set_state(VS_CONFIRM);
        return;
    }
    if (t == T_EXIT && w->escape) { ivw_win_escape(); return; }
    /* a boss guards its hall: it sees the knight on its tile or beside it */
    IvwObj *o = ivw_obj_at(h->x, h->y);
    for (int d = 0; d < 4 && !(o && o->kind == OB_BOSS); d++) o = ivw_obj_at(h->x + IVW_DX[d], h->y + IVW_DY[d]);
    if (o && o->kind == OB_BOSS && !w->veil) {
        static const uint8_t K_WARDEN[1] = {FK_WARDEN};
        static const uint8_t K_MATRON[3] = {FK_MATRON, FK_CHITIN, FK_CHITIN};
        if (o->param == BOSS_WARDEN) ivw_fight_begin(K_WARDEN, 1, BOSS_WARDEN);
        else ivw_fight_begin(K_MATRON, 3, BOSS_MATRON);
        return;
    }
    uint8_t items[8];
    if (ivw_drops_here(h->x, h->y, items, 8)) ivw_say("SOMETHING LIES HERE.");
}

/* ---- moving ------------------------------------------------------------------------------------ */

static void try_step(int dir) {
    IvwHero *h = &ig.hero;
    int nx = h->x + IVW_DX[dir], ny = h->y + IVW_DY[dir];
    if (!ivw_walkable(nx, ny)) {
        ivw_sfx("ivw_bump");
        return;
    }
    int f = ivw_flame_at(nx, ny);
    if (f >= 0 && !ig.w.veil) {
        ivw_start_fight_flame(f);
        return;
    }
    ig.from_x = h->x; ig.from_y = h->y; ig.from_dir = h->dir;
    h->x = (int8_t)nx; h->y = (int8_t)ny;
    ig.anim_kind = 1;
    ig.anim_t = STEP_FRAMES;
    ivw_sfx("ivw_step");
    ivw_touch_tile();
    if (ig.state != VS_EXPLORE) return;
    ivw_pass_turn();
}

static void turn_to(int d) {
    IvwHero *h = &ig.hero;
    ig.from_x = h->x; ig.from_y = h->y; ig.from_dir = h->dir;
    h->dir = (int8_t)(d & 3);
    ig.anim_kind = 2;
    ig.anim_t = TURN_FRAMES;
}

void ivw_explore_update(void) {
    if (ig.anim_t > 0) { ig.anim_t--; if (!ig.anim_t) ig.anim_kind = 0; return; }
    IvwHero *h = &ig.hero;
    if (btnp(BTN_B) || (btn(BTN_B) && !ig.cast.on)) {
        if (btnp(BTN_B)) { ivw_cast_begin(); return; }
    }
    if (btnp(BTN_A)) {
        input_consume();
        ivw_build_menu();
        ig.menu_sel = 0;
        ivw_set_state(VS_MENU);
        ivw_sfx("ui_ok");
        return;
    }
    if (btn(BTN_UP)) { try_step(h->dir); return; }
    if (btn(BTN_DOWN)) { try_step((h->dir + 2) & 3); return; }
    if (btn(BTN_LEFT)) { turn_to(h->dir + 3); return; }
    if (btn(BTN_RIGHT)) { turn_to(h->dir + 1); return; }
}

/* ---- the context menu ---------------------------------------------------------------------------- */

static void ahead(int *x, int *y) {
    *x = ig.hero.x + IVW_DX[ig.hero.dir];
    *y = ig.hero.y + IVW_DY[ig.hero.dir];
}

static void add(int a) {
    if (ig.nmenu < (int)sizeof ig.menu) ig.menu[ig.nmenu++] = (uint8_t)a;
}

static bool has_all_stones(void) {
    return ivw_find_item(IT_CINDER) >= 0 && ivw_find_item(IT_MOSS) >= 0 && ivw_find_item(IT_FROST) >= 0;
}

static int search_max(void) {
    int s = IVW_FLOOR[ig.hero.floor].search_kind;
    return s == IVW_RANDOM ? 4 : s ? 5 : 0;
}

void ivw_build_menu(void) {
    int x, y;
    ahead(&x, &y);
    ig.nmenu = 0;
    IvwWorld *w = &ig.w;
    int t = ivw_tile(x, y);
    int st = (x >= 0 && y >= 0 && x < IVW_W && y < IVW_H) ? w->st[y][x] : 0;
    switch (t) {
    case T_DOOR: add(MA_OPEN); break;
    case T_DOOR_OPEN:
        if (ivw_flame_at(x, y) < 0 && !ivw_obj_at(x, y)) add(MA_CLOSE);
        break;
    case T_LOCKED: add(MA_UNLOCK); break;
    case T_LEVER: add(MA_LEVER); break;
    case T_TORCH: if (st) add(MA_DOUSE); break;
    case T_FUNGUS: add(st ? MA_SMOTHER : MA_POKE); break;
    case T_LANTERN: add(MA_LANTERN); break;
    case T_GEMS: if (!st) add(MA_GEMS); break;
    case T_SWITCH: add(MA_SWITCH); break;
    case T_MURAL: add(MA_STUDY); break;
    default: break;
    }
    IvwObj *o = ivw_obj_at(x, y);
    if (o) switch (o->kind) {
        case OB_CHEST: case OB_MIMIC: if (!o->state) add(MA_CHEST); break;
        case OB_SWEET_WELL: case OB_FOUL_WELL: add(MA_DRINK); add(MA_WELL_ITEM); break;
        case OB_THRONE: if (o->uses) add(MA_SIT); break;
        case OB_PEDLAR: add(MA_TRADE); add(MA_TALK); add(MA_STEAL); break;
        case OB_KNIGHT: add(MA_TALK); add(MA_GIVE); add(MA_ATTACK); break;
        case OB_SOCKET: if (!w->stones_set) add(MA_SOCKET); break;
        default: break;
        }
    uint8_t items[4];
    if (ivw_drops_here(ig.hero.x, ig.hero.y, items, 4)) add(MA_PICKUP);
    if ((w->st[ig.hero.y][ig.hero.x] & 0x80) && w->searches < search_max()) add(MA_SEARCH);
    add(MA_INV);
    add(MA_PASS);
}

void ivw_world_message_lever(int job) {
    static const char *const M[] = {"SOMEWHERE, A GATE MOVES.", "A BOULDER ROLLS AWAY DOWN THE DARK.",
                                    "A BOULDER FALLS ON YOU!", "THE COMB SHIFTS AROUND YOU.", "THE SLIME SHIFTS."};
    ivw_say("%s", M[job]);
}

static void pull_lever(int x, int y) {
    IvwWorld *w = &ig.w;
    const IvwFloorDef *fd = &IVW_FLOOR[ig.hero.floor];
    int id = w->id[y][x];
    if (id >= fd->nlevers) return;
    const IvwLever *lv = &fd->levers[id];
    ivw_sfx("ivw_lever");
    w->st[y][x] ^= 1;
    switch (lv->job) {
    case LV_GATE: {
        uint8_t *g = &w->tile[lv->ty][lv->tx];
        if (*g == T_GATE) *g = T_GATE_OPEN;
        else if (*g == T_GATE_OPEN && !(ig.hero.x == lv->tx && ig.hero.y == lv->ty)) *g = T_GATE;
        ivw_world_message_lever(LV_GATE);
        break;
    }
    case LV_BOULDER:
        if (w->tile[lv->ty][lv->tx] == T_PIT) {
            w->tile[lv->ty][lv->tx] = T_FLOOR;
            ivw_sfx("ivw_rumble");
            ivw_world_message_lever(LV_BOULDER);
        } else ivw_say("NOTHING MORE HAPPENS.");
        break;
    case LV_TRAP:
        if (w->st[y][x]) {
            ivw_world_message_lever(LV_TRAP);
            hero_hurt_world(12, "CRUSHED UNDER A BOULDER.");
        } else ivw_say("THE LEVER CREAKS BACK.");
        break;
    case LV_SHIFT:
        w->shift ^= 1;
        ivw_sfx("ivw_rumble");
        ivw_world_message_lever(LV_SHIFT);
        break;
    default: break;
    }
}

static void open_chest(IvwObj *o) {
    const IvwFloorDef *fd = &IVW_FLOOR[ig.hero.floor];
    if (o->kind == OB_MIMIC) {
        static const uint8_t K[1] = {FK_LIDMAW};
        ivw_say("THE CHEST HAS TEETH!");
        ig.f.mimic_obj = (uint8_t)(o - ig.w.obj);
        ivw_fight_begin(K, 1, BOSS_NONE);
        ig.f.mimic_obj = (uint8_t)(o - ig.w.obj);
        return;
    }
    o->state = 1;
    ivw_sfx("ivw_chest");
    IvwChest c = o->param < fd->nchests ? fd->chests[o->param] : (IvwChest){IVW_RANDOM, 0};
    if (c.item == IVW_SHARDS) { ig.hero.shards = (int16_t)(ig.hero.shards + c.amount); ivw_say("%d SHARDS.", c.amount); }
    else if (c.item == IVW_RANDOM) receive(ivw_random_item(ig.hero.floor, &ig.rng));
    else receive(c.item);
}

/* the mimic's chest, once it is beaten */
void ivw_mimic_loot(int obj) {
    if (obj < 0 || obj >= ig.w.nobj) return;
    IvwObj *o = &ig.w.obj[obj];
    o->kind = OB_CHEST;
    open_chest(o);
}

static const char *const KNIGHT_LINES[3][3] = {
    {"WELL MET, FELLOW PRISONER. WE BOTH CAME DOWN HERE FOR THE REGENT.",
     "I WILL NOT CROSS SWORDS WITH YOU. MY BLADE IS SWORN TO HIM ALONE.",
     "THEY SAY AN OLD GREAT SWORD LIES SOMEWHERE AMONG THE PILLARS BELOW."},
    {"STILL BREATHING? GOOD. SO AM I, MOSTLY.",
     "MIND THE DARK HERE. SOME DOORS ONLY SHOW WHERE THE FUNGUS GLOWS.",
     "GO CAREFULLY, FRIEND. IT ONLY GETS WORSE BELOW."},
    {"THE STARVAULT. WE ARE CLOSE TO HIM NOW.",
     "THE REGENT DRAWS HIS POWER FROM THREE STONES. SET THEM, AND HE WAKES.",
     "STRIKE HIS STONES WHEN THEY SINK LOW, AND HE WEAKENS."},
};

static void show_talk(const char *a, const char *b) {
    ig.ntalk = 0;
    if (a) ig.talk[ig.ntalk++] = a;
    if (b) ig.talk[ig.ntalk++] = b;
    ig.talk_page = 0;
    ivw_set_state(VS_TALK);
}

void ivw_knight_talk(void) {
    IvwHero *h = &ig.hero;
    if (h->honor && !h->dawn_given) {
        if (ivw_give_item(IT_DAWN)) {
            h->dawn_given = 1;
            ivw_add_secret(SEC_DAWN);
            show_talk("SUCH HONOUR! I CAME TO END THE REGENT, BUT I SEE NOW IT IS YOURS TO DO.",
                      "TAKE THE DAWNBRAND. LET IT BE THE LAST THING HE SEES.");
        } else {
            h->dawn_given = 2; /* no room: the gift is gone for good */
            show_talk("SUCH HONOUR! I WOULD GIVE YOU THE DAWNBRAND...",
                      "...BUT YOU CARRY TOO MUCH. NO MATTER. I SHALL CARRY IT MYSELF.");
        }
        return;
    }
    int f = h->floor <= 2 ? 0 : h->floor <= 4 ? 1 : 2;
    show_talk(KNIGHT_LINES[f][h->knight_talk % 3], NULL);
    h->knight_talk++;
}

void ivw_knight_give(int slot) {
    IvwHero *h = &ig.hero;
    int item = h->inv[slot];
    if (slot == h->wpn || slot == h->arm || slot == h->shd) { ivw_say("TAKE IT OFF FIRST."); return; }
    if (IVW_ITEM[item].kind == IK_KEY) { ivw_say("HE SHAKES HIS HEAD. KEEP THAT."); return; }
    ivw_remove_slot(slot);
    ivw_pass_turn();
    if (item == IT_OATH && !h->coin_given) {
        h->coin_given = 1;
        ivw_give_item(IT_COIN);
        ivw_add_secret(SEC_COIN);
        show_talk("THE OATHBLADE! THIS IS THE SWORD I SOUGHT. YOU ARE A TRUE FRIEND.",
                  "TAKE THIS AMBER COIN, A TOKEN BETWEEN US.");
        return;
    }
    int before = h->knight_favour / 10;
    h->knight_favour = (uint8_t)imin(250, h->knight_favour + imax(1, IVW_ITEM[item].value));
    if (h->knight_favour / 10 > before) {
        int roll = rng_range(&ig.rng, 0, 99);
        if (roll >= 90 && h->inv_cap < IVW_INV_MAX) {
            h->inv_cap++;
            h->knight_inv_gifts++;
            show_talk("GENEROUS FRIEND, TAKE THIS: A BETTER PACK.", "(YOU CAN CARRY ONE MORE THING.)");
        } else if (roll >= 75 && h->hp_bonus < 20) {
            h->hp_bonus = (int16_t)(h->hp_bonus + 10);
            h->hp = (int16_t)(h->hp + 10);
            h->knight_hp_gifts++;
            show_talk("GENEROUS FRIEND, TAKE THIS: A CHARM OF OLD IRON.", "(YOUR HEALTH GROWS BY 10.)");
        } else if (roll >= 45) {
            static const uint8_t TON[5] = {IT_RED, IT_GREY, IT_GREEN, IT_BLACK, IT_BLUE};
            receive(TON[rng_range(&ig.rng, 0, 4)]);
            show_talk("GENEROUS FRIEND, I SHALL REPAY YOUR KINDNESS. TAKE THIS!", NULL);
        } else {
            int n = rng_range(&ig.rng, 5, 14);
            h->shards = (int16_t)(h->shards + n);
            show_talk("GENEROUS FRIEND, I SHALL REPAY YOUR KINDNESS. TAKE THESE SHARDS!", NULL);
        }
    } else show_talk("YOUR KINDNESS WILL NOT BE FORGOTTEN, FRIEND.", NULL);
}

/* the coin in a well: a wish (sweet) or a cruel joke (foul) */
static int wish_well_foul;
void ivw_wish(int choice) {
    IvwHero *h = &ig.hero;
    if (!wish_well_foul) {
        if (choice == 0) {
            for (int s = 0; s < 3; s++) h->stat[s] = (uint8_t)imax(1, h->stat[s] - 2);
            for (int s = 0; s < 3; s++) ig.stat_before[s] = h->stat[s];
            ig.throne_pts = 7;
            ig.throne_sel = 0;
            ivw_say("YOUR BODY CHANGES.");
            ivw_set_state(VS_THRONE);
            return;
        }
        if (choice == 1) {
            h->shards = (int16_t)(h->shards + 20);
            receive(ivw_random_item(imax(2, h->floor), &ig.rng));
            static const uint8_t WPN[5] = {IT_STAR, IT_GLAIVE, IT_BROAD, IT_MAUL, IT_AXE};
            receive(WPN[rng_range(&ig.rng, 0, 4)]);
            ivw_say("TREASURE RAINS FROM THE WELL.");
        } else {
            h->honor = 1;
            ivw_say("YOU FEEL LIGHTER, AND SURE OF YOUR PATH.");
        }
    } else {
        if (choice == 0) {
            for (int s = 0; s < 3; s++) h->stat[s] = (uint8_t)imax(1, h->stat[s] - 2);
            for (int s = 0; s < 3; s++) ig.stat_before[s] = h->stat[s];
            ig.throne_pts = 3;
            ig.throne_sel = 0;
            ivw_say("PAIN! YOUR BODY TWISTS.");
            ivw_set_state(VS_THRONE);
            return;
        }
        ivw_say(choice == 1 ? "GREED EARNS NOTHING HERE." : "HONOUR? THE WELL ONLY LAUGHS.");
    }
    ivw_set_state(VS_EXPLORE);
}

static void throw_in_well(int slot) {
    IvwHero *h = &ig.hero;
    int x, y;
    ahead(&x, &y);
    IvwObj *o = ivw_obj_at(x, y);
    if (!o) return;
    int item = h->inv[slot];
    if (slot == h->wpn || slot == h->arm || slot == h->shd) { ivw_say("TAKE IT OFF FIRST."); ivw_set_state(VS_EXPLORE); return; }
    if (IVW_ITEM[item].kind == IK_KEY) { ivw_say("NOT THAT."); ivw_set_state(VS_EXPLORE); return; }
    ivw_remove_slot(slot);
    ivw_pass_turn();
    if (item != IT_COIN) { ivw_say("IT SINKS. NOTHING HAPPENS."); ivw_set_state(VS_EXPLORE); return; }
    wish_well_foul = o->kind == OB_FOUL_WELL;
    if (!wish_well_foul) ivw_add_secret(SEC_WISH);
    ig.wish_sel = 0;
    ivw_say(wish_well_foul ? "THE WELL BUBBLES. MAKE YOUR WISH..." : "THE WELL GLOWS. MAKE YOUR WISH...");
    ivw_set_state(VS_WISH);
}

int ivw_wish_foul(void) { return wish_well_foul; }

void ivw_search_room(void) {
    IvwWorld *w = &ig.w;
    int kind = IVW_FLOOR[ig.hero.floor].search_kind;
    int n = w->searches++;
    bool last = w->searches >= search_max();
    /* what the search turns up comes first; then the turn passes */
    if (last && kind != IVW_RANDOM) {
        if (kind == IT_OATH) ivw_add_secret(SEC_OATH);
        ivw_say("BEHIND A PILLAR, SOMETHING WAITS...");
        receive(kind);
        ivw_pass_turn();
        return;
    }
    bool fight = kind == IVW_RANDOM ? rng_chance(&ig.rng, 50) : (n % 2) == 0;
    if (fight) {
        uint8_t kinds[4];
        int k = ivw_floor_pool(ig.hero.floor, &ig.rng, kinds, 4);
        ivw_say("SOMETHING STIRS IN THE DARK!");
        ivw_fight_begin(kinds, k, BOSS_NONE);
        ig.f.search = 1;
        return;
    }
    ivw_say("YOU SEARCH...");
    receive(ivw_random_item(ig.hero.floor, &ig.rng));
    ivw_pass_turn();
}

void ivw_set_stones(void) {
    if (!has_all_stones()) { show_talk("THREE HOLLOWS IN THE PILLAR: ONE WARM, ONE DAMP, ONE COLD.", NULL); return; }
    ivw_remove_slot(ivw_find_item(IT_CINDER));
    ivw_remove_slot(ivw_find_item(IT_MOSS));
    ivw_remove_slot(ivw_find_item(IT_FROST));
    ig.w.stones_set = 1;
    ivw_say("THE STONES BLAZE. THE PIT REGENT RISES!");
    static const uint8_t K[4] = {FK_REGENT, FK_STONE, FK_STONE, FK_STONE};
    ivw_fight_begin(K, 4, BOSS_REGENT);
}

/* the menu's chosen action */
void ivw_menu_do(int a) {
    IvwHero *h = &ig.hero;
    IvwWorld *w = &ig.w;
    int x, y;
    ahead(&x, &y);
    IvwObj *o = ivw_obj_at(x, y);
    ivw_set_state(VS_EXPLORE);
    switch (a) {
    case MA_INV: ig.inv_sel = 0; ivw_set_state(VS_INV); return;
    case MA_PASS: ivw_pass_turn(); return;
    case MA_OPEN: w->tile[y][x] = T_DOOR_OPEN; ivw_sfx("ivw_door"); break;
    case MA_CLOSE: w->tile[y][x] = T_DOOR; ivw_sfx("ivw_door"); break;
    case MA_UNLOCK:
        if (!h->keys) { ivw_say("LOCKED. YOU HAVE NO KEY."); return; }
        h->keys--;
        w->tile[y][x] = T_DOOR_OPEN;
        ivw_sfx("ivw_unlock");
        ivw_say("THE KEY TURNS.");
        break;
    case MA_LEVER: pull_lever(x, y); break;
    case MA_DOUSE: w->st[y][x] = 0; ivw_sfx("ivw_douse"); ivw_say("THE TORCH GOES OUT."); break;
    case MA_POKE:
        w->st[y][x] = 1;
        h->st = 0;
        ivw_sfx("ivw_spore");
        ivw_say("THE FUNGUS GLOWS. ITS SPORES SAP YOUR STRENGTH.");
        break;
    case MA_SMOTHER: w->st[y][x] = 0; ivw_sfx("ivw_douse"); ivw_say("THE FUNGUS DIMS."); break;
    case MA_LANTERN:
        if (w->st[y][x]) {
            w->st[y][x] = 0;
            h->essence += 10;
            ivw_say("YOU TAKE THE LIGHT: 10 MOTES.");
        } else if (h->essence >= 30) {
            w->st[y][x] = 1;
            h->essence -= 30;
            ivw_say("YOU GIVE THE LIGHT 30 MOTES. IT BURNS.");
        } else {
            ivw_say("THE LANTERN WANTS 30 MOTES.");
            return;
        }
        ivw_sfx("ivw_lantern");
        break;
    case MA_GEMS:
        memcpy(ig.gem_board, IVW_GEMSET[w->id[y][x] & 3], 9);
        ig.gem_sel = 4;
        ig.gem_held = -1;
        ig.gem_panel_x = x;
        ig.gem_panel_y = y;
        ivw_set_state(VS_GEMS);
        return;
    case MA_SWITCH:
        w->slime_b ^= 1;
        ivw_sfx("ivw_slime");
        ivw_world_message_lever(LV_SLIME);
        break;
    case MA_STUDY: ig.glyph_view = w->id[y][x]; ivw_set_state(VS_GLYPH); return;
    case MA_CHEST: if (o) { ivw_pass_turn(); if (ig.state == VS_EXPLORE) open_chest(o); } return;
    case MA_DRINK:
        if (!o) return;
        if (!o->uses) { ivw_say("THE WELL IS DRY."); break; }
        o->uses--;
        if (o->kind == OB_SWEET_WELL) {
            h->hp = (int16_t)imin(ivw_max_hp(h), h->hp + 25);
            h->poison = 0;
            ivw_say("SWEET WATER. YOU FEEL BETTER.");
        } else {
            h->poison = 10;
            ivw_say("FOUL WATER! POISON.");
        }
        ivw_sfx("ivw_drink");
        break;
    case MA_WELL_ITEM: ig.pick_mode = 1; ig.pick_sel = 0; ivw_set_state(VS_PICK); return;
    case MA_SIT: {
        if (!o) return;
        int cost = IVW_FLOOR[h->floor].throne_cost;
        if (h->essence < cost) { ivw_say("THE THRONE WANTS %d MOTES.", cost); return; }
        h->essence -= cost;
        o->uses = 0;
        h->thrones_used |= (uint8_t)(1 << h->floor);
        for (int s = 0; s < 3; s++) ig.stat_before[s] = h->stat[s];
        ig.throne_pts = 3;
        ig.throne_sel = 0;
        music_play(IVW_MUS_THRONE);
        ivw_set_state(VS_THRONE);
        return;
    }
    case MA_TRADE: ig.shop_sel = 0; ig.shop_buy = 1; ivw_set_state(VS_SHOP); return;
    case MA_TALK:
        if (o && o->kind == OB_KNIGHT) ivw_knight_talk();
        else show_talk("SHARDS FOR WARES, WARES FOR SHARDS. AND NO FUNNY BUSINESS, DEARIE.", NULL);
        return;
    case MA_STEAL: ig.pick_mode = 3; ig.pick_sel = 0; ivw_set_state(VS_PICK); return;
    case MA_GIVE: ig.pick_mode = 0; ig.pick_sel = 0; ivw_set_state(VS_PICK); return;
    case MA_ATTACK:
        if (o) o->kind = OB_NONE;
        h->knight_gone = 1;
        show_talk("I WILL NOT FIGHT YOU, STRANGER. MY QUARREL IS NOT WITH YOU.", "(THE AMBER KNIGHT IS GONE.)");
        return;
    case MA_SOCKET: ivw_set_stones(); return;
    case MA_PICKUP: ig.pick_mode = 2; ig.pick_sel = 0; ivw_set_state(VS_PICK); return;
    case MA_SEARCH: ivw_search_room(); return;
    default: break;
    }
    if (ig.state == VS_EXPLORE) ivw_pass_turn();
}

/* the item picker's choice */
void ivw_pick_do(int sel) {
    IvwHero *h = &ig.hero;
    switch (ig.pick_mode) {
    case 0: ivw_set_state(VS_EXPLORE); ivw_knight_give(sel); break;
    case 1: throw_in_well(sel); break;
    case 2: {
        uint8_t items[16];
        int n = ivw_drops_here(h->x, h->y, items, 16);
        ivw_set_state(VS_EXPLORE);
        if (sel >= n) return;
        int it = items[sel];
        if (it != IT_IRONKEY && ivw_free_slots() == 0) { ivw_say("THE PACK IS FULL."); return; }
        ivw_take_drop(it, h->x, h->y);
        receive(it);
        ivw_pass_turn();
        break;
    }
    case 3: {
        ivw_set_state(VS_EXPLORE);
        if (sel < 0 || sel >= 4 || !ig.w.shop[sel]) return;
        static const uint8_t K[1] = {FK_PEDLAR};
        show_talk("THIEF! I SAW THAT, DEARIE.", NULL);
        ig.f.pedlar = (uint8_t)(sel + 1);
        ivw_fight_begin(K, 1, BOSS_NONE);
        ig.f.pedlar = (uint8_t)(sel + 1);
        break;
    }
    default: ivw_set_state(VS_EXPLORE); break;
    }
}

/* the pack: what using an item does out of a fight */
void ivw_use_item_world(int slot) {
    IvwHero *h = &ig.hero;
    int item = h->inv[slot];
    const IvwItem *it = &IVW_ITEM[item];
    if (it->kind == IK_POTION) {
        ivw_remove_slot(slot);
        ivw_sfx("ivw_drink");
        switch (item) {
        case IT_RED: h->hp = (int16_t)imin(ivw_max_hp(h), h->hp + it->a); ivw_say("YOU FEEL MUCH BETTER."); break;
        case IT_BLUE:
            h->st = (int16_t)ivw_max_st(h);
            h->mp = (int16_t)imin(ivw_max_mp(h), h->mp + it->a);
            ivw_say("STAMINA AND MANA FLOW BACK.");
            break;
        case IT_BLACK: ig.buff |= 1; ivw_say("YOUR ARMS BURN WITH STRENGTH."); break;
        case IT_GREEN: ig.buff |= 2; ivw_say("YOUR FEET FEEL LIGHT."); break;
        case IT_GREY: ig.buff |= 4; ivw_say("YOUR GUARD FEELS LIKE STONE."); break;
        case IT_YELLOW: ig.buff |= 8; ivw_say("YOUR EYES SHARPEN."); break;
        default: break;
        }
        ivw_pass_turn();
        return;
    }
    switch (item) {
    case IT_BONE: {
        ivw_remove_slot(slot);
        int n = 0;
        for (int i = 0; i < IVW_MAX_FLAME; i++) {
            IvwFlame *f = &ig.w.flame[i];
            if (f->alive && iabs(f->x - h->x) + iabs(f->y - h->y) <= 6) { f->alive = 0; n++; }
        }
        ivw_say(n ? "THE FLAMES SHY AWAY FROM THE BONE." : "YOU RATTLE THE BONE. NOTHING.");
        ivw_pass_turn();
        return;
    }
    case IT_HEAD:
        ig.tele_x = h->x;
        ig.tele_y = h->y;
        ivw_set_state(VS_TELEPORT);
        return;
    case IT_COIN:
        if (rng_chance(&ig.rng, 50)) {
            h->hp = (int16_t)imin(ivw_max_hp(h), h->hp + 15);
            h->mp = (int16_t)imin(ivw_max_mp(h), h->mp + 10);
            ivw_say("HEADS! FORTUNE SMILES.");
        } else {
            h->poison = (uint8_t)imax(h->poison, 6);
            ivw_say("TAILS. YOU FEEL ILL.");
        }
        ivw_sfx("ivw_coin");
        ivw_pass_turn();
        return;
    default: ivw_say("NOTHING HAPPENS."); return;
    }
}

/* the chitin head's leap: to a tile already seen, at most 8 away */
void ivw_teleport_to(int x, int y) {
    IvwHero *h = &ig.hero;
    ivw_set_state(VS_EXPLORE);
    if (iabs(x - h->x) > 8 || iabs(y - h->y) > 8 || !ig.w.seen[y][x] || !ivw_walkable(x, y) ||
        ivw_flame_at(x, y) >= 0) {
        ivw_say("THE HEAD STAYS SILENT.");
        return;
    }
    int s = ivw_find_item(IT_HEAD);
    if (s >= 0) ivw_remove_slot(s);
    h->x = (int8_t)x;
    h->y = (int8_t)y;
    ivw_sfx("ivw_warp");
    ivw_touch_tile();
    if (ig.state == VS_EXPLORE) ivw_pass_turn();
}

/* ---- fights from the map ---------------------------------------------------------------------- */

void ivw_start_fight_flame(int flame) {
    uint8_t kinds[4];
    int n = ivw_floor_pool(ig.hero.floor, &ig.rng, kinds, 4);
    int burned = ig.w.flame[flame].burned;
    ivw_fight_begin(kinds, n, BOSS_NONE);
    ig.f.flame = (uint8_t)flame;
    ig.f.start_burn = (int16_t)burned;
    if (burned)
        for (int i = 0; i < ig.f.nfoe; i++)
            if (!(IVW_FOE[ig.f.foe[i].kind].flags & FF_NOBURN)) ig.f.foe[i].burn = 360;
}

/* the Gaol's torches on this floor (lit ones in *lit) */
int ivw_torches(int *lit) {
    int n = 0, l = 0;
    for (int y = 0; y < IVW_H; y++)
        for (int x = 0; x < IVW_W; x++)
            if (ig.w.tile[y][x] == T_TORCH) { n++; l += ig.w.st[y][x] != 0; }
    if (lit) *lit = l;
    return n;
}

/* the fight is over and won: back to the map */
void ivw_fight_done(void) {
    IvwFight *f = &ig.f;
    IvwWorld *w = &ig.w;
    if (f->flame < IVW_MAX_FLAME) w->flame[f->flame].alive = 0;
    /* the boss's tile is clear now */
    if (f->boss == BOSS_WARDEN || f->boss == BOSS_MATRON)
        for (int i = 0; i < w->nobj; i++)
            if (w->obj[i].kind == OB_BOSS) w->obj[i].kind = OB_NONE;
    if (f->boss) { w->boss_dead = 1; ig.hero.bosses |= (uint8_t)(1 << f->boss); }
    if (f->boss == BOSS_REGENT) {
        w->escape = 1;
        ivw_say("THE VAULT SHAKES. A WAY UP OPENS!");
        music_play(IVW_MUS_REGENT);
    } else ivw_floor_music();
    if (f->pedlar) {
        int it = w->shop[f->pedlar - 1];
        w->shop[f->pedlar - 1] = 0;
        for (int i = 0; i < w->nobj; i++)
            if (w->obj[i].kind == OB_PEDLAR) w->obj[i].kind = OB_NONE;
        w->pedlar_gone = 1;
        if (it) receive(it);
    }
    ivw_set_state(VS_EXPLORE);
    if (f->mimic_obj != 255) ivw_mimic_loot(f->mimic_obj);
    ivw_reveal_around();
}
