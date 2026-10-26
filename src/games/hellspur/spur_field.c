/* HELLSPUR - the field: walking tile by tile with the posse in a line
 * behind (the fallen too, in their pine boxes), the hellfire wisps that
 * appear around you and step when you step, the doors, stairs and edges
 * between maps, hired horses and the nightmares, and searching and talking
 * with A. */
#include "spur.h"

static const int DX[4] = {0, 0, -1, 1}; /* down up left right */
static const int DY[4] = {1, -1, 0, 0};

int spur_facing_x(void) { return sg.pos[0].x + DX[sg.face]; }
int spur_facing_y(void) { return sg.pos[0].y + DY[sg.face]; }

bool spur_in_town(void) { return sg.map.kind == MK_TOWN; }

static int posse_len(void) {
    if (!spur_flag(F_POSSE)) return 1; /* the angel alone, in the saloon */
    return imax(1, sg.s.nposse);
}

bool spur_npc_visible(int i) {
    const SpurNpc *n = &sg.map.npc[i];
    if (n->look == 0 && n->pal == 0 && n->x == 0 && n->y == 0 && n->talk == 0) return false;
    if (n->show_if >= 0 && !spur_flag(n->show_if)) return false;
    if (n->hide_if >= 0 && spur_flag(n->hide_if)) return false;
    /* a stranger the angel has carried out has gone from the saloon */
    if (sg.map_id == M_SALOON && n->look >= LOOK_COUNT && sg.saved[n->look - LOOK_COUNT]) return false;
    /* the hounds' kin: Biscuit is not left behind in the saloon twice */
    return true;
}

int spur_npc_at(int x, int y) {
    for (int i = 0; i < sg.map.nnpc; i++)
        if (spur_npc_visible(i) && sg.npc_x[i] == x && sg.npc_y[i] == y) return i;
    return -1;
}

/* the tiles the story keeps shut until it opens them */
static bool story_open(int t) {
    switch (t) {
    case T_FERRY: return spur_flag(F_FERRY);
    case T_FIREWALL: return spur_key(KI_FIRECHARM);
    case T_LOCKED: return spur_flag(F_ROOST_DOOR);
    default: return false;
    }
}

bool spur_walkable(int x, int y, bool flying) {
    int t = spur_tile_at(&sg.map, x, y);
    if (t < 0) return sg.map.kind == MK_TOWN || (sg.map.kind == MK_PLACE && false);
    if (flying) return (SPUR_TILE_FLAGS[t] & TF_FLY) != 0;
    return spur_tile_walk(t) || story_open(t);
}

void spur_clear_wisps(void) {
    for (int i = 0; i < SPUR_WISPS; i++) sg.wisp[i].alive = 0;
}

void spur_place_party(int x, int y, int dir) {
    for (int i = 0; i < SPUR_TRAIL; i++) sg.pos[i] = sg.prev[i] = (SpurPos){(int16_t)x, (int16_t)y, (int16_t)dir};
    sg.face = dir;
    sg.moving = 0;
    sg.move_t = 0;
}

static void map_music(void) {
    int mus = SPUR_MUS_WORLD;
    int id = sg.map_id;
    switch (sg.map.kind) {
    case MK_WORLD:
        mus = id == M_HELL ? SPUR_MUS_HELL : sg.mounted == 2 ? SPUR_MUS_FLY : sg.mounted == 1 ? SPUR_MUS_RIDE : SPUR_MUS_WORLD;
        break;
    case MK_TOWN:
        mus = SPUR_MUS_TOWN;
        if (id == M_HOPE && !spur_flag(F_HOPE_FREE)) mus = SPUR_MUS_GHOST;
        if (id == M_HOLLOW && !spur_flag(F_MISC0)) mus = SPUR_MUS_GHOST;
        if (id == M_ASHMARKET) mus = SPUR_MUS_HELL;
        if (id == M_STCLARE || id == M_HERMITAGE) mus = SPUR_MUS_CHURCH;
        break;
    case MK_PLACE:
        mus = id == M_SALOON || id == M_SKYBAR || id == M_ROOST ? SPUR_MUS_SALOON
            : id == M_THORNS || id == M_SHRINE ? SPUR_MUS_CHURCH
            : id == M_SPUR3 ? SPUR_MUS_CITADEL : SPUR_MUS_TOWN;
        if (id == M_SALOON && !spur_flag(F_POSSE)) mus = SPUR_MUS_SALOON;
        break;
    default:
        mus = (id == M_DREAM1 || id == M_DREAM2) ? SPUR_MUS_DREAM
            : (id >= M_TOWER1 && id <= M_TOWER4) ? SPUR_MUS_HELL
            : (id == M_SPUR1 || id == M_SPUR2) ? SPUR_MUS_CITADEL : SPUR_MUS_CAVE;
        break;
    }
    music_play(mus);
}

/* find a link on the current map that leads to a given map */
static bool link_to(int map, int *x, int *y) {
    for (int i = 0; i < sg.map.nlink; i++)
        if (sg.map.link[i].map == map) { *x = sg.map.link[i].x; *y = sg.map.link[i].y; return true; }
    return false;
}

/* x, y: a tile, or -1 the map's start, -2 its far end, -3 the link back to where we came from */
void spur_enter_map(int id, int x, int y, int dir) {
    int from = sg.map_id;
    spur_world_build(&sg.map, id);
    sg.map_id = id;
    if (x == -1) { x = sg.map.start_x; y = sg.map.start_y; }
    else if (x == -2) { x = sg.map.back_x; y = sg.map.back_y; }
    else if (x == -3) {
        if (!link_to(from, &x, &y)) {
            if (id == M_WORLD) { x = SPUR_MAPINFO[from].px; y = SPUR_MAPINFO[from].py; }
            else { x = sg.map.start_x; y = sg.map.start_y; }
        }
    }
    if (id == M_WORLD && (x <= 0 || y <= 0)) { x = SPUR_MAPINFO[from].px; y = SPUR_MAPINFO[from].py; }
    for (int i = 0; i < sg.map.nnpc; i++) {
        sg.npc_x[i] = sg.map.npc[i].x;
        sg.npc_y[i] = sg.map.npc[i].y;
        sg.npc_dir[i] = sg.map.npc[i].dir;
        sg.npc_t[i] = i * 37;
    }
    if (sg.map.kind != MK_WORLD) {
        if (sg.mounted) sg.mounted = 0;
        if (sg.map.kind == MK_TOWN) sg.s.horses = 0; /* the hired horses go back to their stables */
    }
    spur_place_party(x, y, dir);
    spur_clear_wisps();
    sg.step_count = 0;
    sg.map_t = 0;
    map_music();
    spur_enter_event(id);
}

/* rebuild the map where it stands (the story opened something) */
void spur_refresh_map(void) {
    int nx[SPUR_MAX_NPC], ny[SPUR_MAX_NPC], nd[SPUR_MAX_NPC];
    for (int i = 0; i < SPUR_MAX_NPC; i++) { nx[i] = sg.npc_x[i]; ny[i] = sg.npc_y[i]; nd[i] = sg.npc_dir[i]; }
    spur_world_build(&sg.map, sg.map_id);
    for (int i = 0; i < SPUR_MAX_NPC; i++) { sg.npc_x[i] = nx[i]; sg.npc_y[i] = ny[i]; sg.npc_dir[i] = nd[i]; }
}

void spur_set_mount(int m) {
    sg.mounted = m;
    if (m == 2) spur_clear_wisps();
    map_music();
}

/* ---- the wisps -------------------------------------------------------------------- */
static int nearest_member(int x, int y, int *mx, int *my) {
    int best = 9999;
    for (int i = 0; i < posse_len(); i++) {
        int d = iabs(sg.pos[i].x - x) + iabs(sg.pos[i].y - y);
        if (d < best) { best = d; *mx = sg.pos[i].x; *my = sg.pos[i].y; }
    }
    return best;
}

static bool wisp_at(int x, int y, int except) {
    for (int i = 0; i < SPUR_WISPS; i++)
        if (i != except && sg.wisp[i].alive && sg.wisp[i].x == x && sg.wisp[i].y == y) return true;
    return false;
}

static bool wisp_can(int x, int y) {
    int t = spur_tile_at(&sg.map, x, y);
    if (t < 0 || !spur_tile_walk(t)) return false;
    if (SPUR_TILE_FLAGS[t] & TF_ENTER) return false; /* never into a door */
    if (spur_npc_at(x, y) >= 0) return false;
    return true;
}

void spur_spawn_wisp_at(int x, int y) {
    for (int i = 0; i < SPUR_WISPS; i++)
        if (!sg.wisp[i].alive) {
            sg.wisp[i] = (SpurWisp){(int16_t)x, (int16_t)y, 1, 0, (int16_t)x, (int16_t)y, 0};
            return;
        }
}

static int zone_here(void) {
    if (sg.map.kind == MK_WORLD) return spur_world_zone(sg.map_id, sg.pos[0].x, sg.pos[0].y);
    return sg.map.zone;
}

static void maybe_spawn(void) {
    int z = zone_here();
    if (z < 0 || z >= SPUR_ZONES || sg.mounted == 2) return;
    int rate = SPUR_ZONE[z].rate;
    if (!rate) return;
    int live = 0;
    for (int i = 0; i < SPUR_WISPS; i++) live += sg.wisp[i].alive;
    int cap = sg.map.kind == MK_WORLD ? 3 : 4;
    if (live >= cap) return;
    int chance = rate * 100 / (sg.ward_steps > 0 ? 4 : 1);
    if (rng_range(&sg.rng, 0, 9999) >= chance) return;
    /* somewhere in sight, five to eight steps off */
    for (int tries = 0; tries < 24; tries++) {
        int dx = rng_range(&sg.rng, -8, 8), dy = rng_range(&sg.rng, -5, 5);
        int d = iabs(dx) + iabs(dy);
        if (d < 5 || d > 9) continue;
        int x = sg.pos[0].x + dx, y = sg.pos[0].y + dy;
        if (!wisp_can(x, y) || wisp_at(x, y, -1)) continue;
        bool on_member = false;
        for (int i = 0; i < posse_len(); i++) on_member |= sg.pos[i].x == x && sg.pos[i].y == y;
        if (on_member) continue;
        spur_spawn_wisp_at(x, y);
        spur_sfx("spur_wisp");
        return;
    }
}

static bool touching(void) {
    for (int w = 0; w < SPUR_WISPS; w++) {
        if (!sg.wisp[w].alive) continue;
        for (int i = 0; i < posse_len(); i++)
            if (sg.pos[i].x == sg.wisp[w].x && sg.pos[i].y == sg.wisp[w].y) return true;
    }
    return false;
}

/* every wisp steps once, toward the nearest of the posse (the fallen count) */
static void wisps_step(void) {
    for (int w = 0; w < SPUR_WISPS; w++) {
        SpurWisp *q = &sg.wisp[w];
        if (!q->alive) continue;
        q->ox = q->x;
        q->oy = q->y;
        q->t = 0;
        int mx = q->x, my = q->y;
        int d0 = nearest_member(q->x, q->y, &mx, &my);
        if (d0 > 16) { q->alive = 0; continue; } /* left far behind: it gutters out */
        int best = -1, bestd = d0, ties = 0;
        for (int k = 0; k < 4; k++) {
            int nx = q->x + DX[k], ny = q->y + DY[k];
            if (!wisp_can(nx, ny) && !(nx == mx && ny == my)) continue;
            if (wisp_at(nx, ny, w)) continue;
            int d = iabs(mx - nx) + iabs(my - ny);
            if (d < bestd) { bestd = d; best = k; ties = 1; }
            else if (d == bestd && best >= 0 && rng_range(&sg.rng, 0, ties) == 0) { best = k; ties++; }
        }
        if (best >= 0) { q->x += DX[best]; q->y += DY[best]; q->wait = 0; }
        else if (++q->wait > 6) q->alive = 0; /* hemmed in: it gutters out */
    }
}

/* ---- battles from the field ------------------------------------------------------------ */
void spur_start_battle(int troop, int flag) {
    spur_clear_wisps();
    sg.moving = 0;
    spur_battle_begin(troop, flag);
}

static void wisp_battle(void) {
    int z = zone_here();
    if (z < 0) z = 0;
    const SpurZone *zn = &SPUR_ZONE[z];
    int troop = zn->troop[0];
    if (zn->rare && rng_range(&sg.rng, 0, 99) < zn->rare_pct) troop = zn->rare;
    else {
        int n = 0;
        while (n < SPUR_ZONE_TROOPS && zn->troop[n]) n++;
        if (n > 0) troop = zn->troop[rng_range(&sg.rng, 0, n - 1)];
    }
    spur_start_battle(troop, -1);
}

void spur_field_after_battle(bool won) {
    spur_clear_wisps();
    sg.state = GS_FIELD;
    map_music();
    (void)won;
}

/* everyone falls: back to the last chapel, all on their feet, purse empty */
void spur_respawn_at_chapel(void) {
    int town = sg.s.chapel ? sg.s.chapel : M_GLADWELL;
    sg.s.teeth = 0;
    for (int i = 0; i < 4; i++) {
        int h = spur_hero_at(i);
        if (h < 0) continue;
        sg.s.hero[h].hp = (int16_t)spur_maxhp(h);
        sg.s.hero[h].sp = (int16_t)spur_maxsp(h);
        sg.s.hero[h].status = 0;
        sg.s.hero[h].reload = 0;
    }
    sg.state = GS_FIELD;
    sg.mounted = 0;
    sg.s.horses = 0;
    spur_enter_map(town, -1, 0, 1);
    spur_save_game();
    spur_say("YOU WAKE IN THE CHAPEL PEWS.\nSOMEONE HAS GONE THROUGH YOUR POCKETS.\nEVERY TOOTH YOU CARRIED IS GONE.");
}

void spur_wipe_return(void) { spur_respawn_at_chapel(); }

/* ---- stepping onto things ---------------------------------------------------------------- */
static bool try_link(int x, int y) {
    for (int i = 0; i < sg.map.nlink; i++) {
        const SpurLink *l = &sg.map.link[i];
        if (l->x != x || l->y != y) continue;
        int t = spur_tile_at(&sg.map, x, y);
        /* a link on a tile the story hasn't opened yet does nothing */
        if (!(SPUR_TILE_FLAGS[t] & TF_ENTER) && t != T_DOWN && t != T_UP && t != T_EXIT && t != T_PORTAL && t != T_LIFT) return false;
        if (sg.mounted == 2) return false; /* land first */
        int target = l->map;
        if (sg.map_id == M_WORLD && !spur_event_door(target)) return true;
        spur_sfx(t == T_DOWN || t == T_UP ? "spur_stairs" : "spur_door");
        spur_enter_map(target, l->tx, l->ty, 0);
        return true;
    }
    return false;
}

static void step_done(void) {
    sg.step_count++;
    sg.s.steps++;
    if (sg.ward_steps > 0) sg.ward_steps--;
    int x = sg.pos[0].x, y = sg.pos[0].y;
    /* poison eats at the walkers */
    if (++sg.poison_steps >= 4) {
        sg.poison_steps = 0;
        for (int i = 0; i < 4; i++) {
            int h = spur_hero_at(i);
            if (spur_alive(h) && (sg.s.hero[h].status & SS_POISON) && sg.s.hero[h].hp > 1) {
                sg.s.hero[h].hp--;
                sg.flash_poison = 6;
            }
        }
    }
    /* doors and stairs */
    int t = spur_tile_at(&sg.map, x, y);
    if (sg.map.kind == MK_TOWN || sg.map.kind == MK_PLACE) {
        for (int i = 0; i < sg.map.ndoor; i++)
            if (sg.map.door[i].x == x && sg.map.door[i].y == y) {
                /* step back out under the sign, then the counter */
                sg.pos[0].y = (int16_t)(y + 1);
                sg.prev[0] = sg.pos[0];
                sg.face = 1;
                spur_open_door(sg.map.door[i].service, sg.map.door[i].arg);
                return;
            }
    }
    if ((SPUR_TILE_FLAGS[t] & TF_ENTER) || t == T_EXIT || t == T_UP || t == T_DOWN)
        if (try_link(x, y)) return;
    /* off the edge of a town: out into the Territory */
    if (sg.map.kind == MK_TOWN && (x == 0 || y == 0 || x == sg.map.w - 1 || y == sg.map.h - 1)) {
        spur_leave_town();
        return;
    }
    /* ambushes */
    for (int i = 0; i < sg.map.nambush; i++) {
        const SpurAmbush *a = &sg.map.ambush[i];
        if (a->x == x && a->y == y && (a->flag < 0 || !spur_flag(a->flag))) {
            spur_sfx("spur_ambush");
            spur_start_battle(a->troop, a->flag);
            return;
        }
    }
    spur_step_event(x, y);
    if (sg.state != GS_FIELD || spur_ui_open()) return;
    /* the wisps answer every step (one for every two on horseback) */
    bool step = true;
    if (sg.mounted == 1) step = (++sg.horse_step & 1) == 0;
    if (sg.mounted == 2) step = false;
    if (touching()) { wisp_battle(); return; }
    if (step) {
        wisps_step();
        if (touching()) { wisp_battle(); return; }
    }
    if (sg.mounted != 2) maybe_spawn();
}

void spur_leave_town(void) {
    spur_save_game();
    spur_enter_map(M_WORLD, -3, 0, 0);
}

/* ---- people walking about ------------------------------------------------------------------ */
static void npcs_update(void) {
    for (int i = 0; i < sg.map.nnpc; i++) {
        const SpurNpc *n = &sg.map.npc[i];
        if (!n->wander || !spur_npc_visible(i)) continue;
        if (++sg.npc_t[i] < 90) continue;
        sg.npc_t[i] = rng_range(&sg.rng, 0, 40);
        int d = rng_range(&sg.rng, 0, 3);
        int nx = sg.npc_x[i] + DX[d], ny = sg.npc_y[i] + DY[d];
        sg.npc_dir[i] = d;
        /* stay within three of home, on open ground, off the posse */
        if (iabs(nx - n->x) > 3 || iabs(ny - n->y) > 3) continue;
        int t = spur_tile_at(&sg.map, nx, ny);
        if (t < 0 || !spur_tile_walk(t) || (SPUR_TILE_FLAGS[t] & TF_ENTER)) continue;
        if (nx == 0 || ny == 0 || nx == sg.map.w - 1 || ny == sg.map.h - 1) continue;
        bool blocked = spur_npc_at(nx, ny) >= 0;
        for (int k = 0; k < posse_len(); k++) blocked |= sg.pos[k].x == nx && sg.pos[k].y == ny;
        for (int k = 0; k < sg.map.ndoor; k++) blocked |= sg.map.door[k].x == nx && sg.map.door[k].y == ny + 1;
        if (blocked) continue;
        sg.npc_x[i] = nx;
        sg.npc_y[i] = ny;
    }
}

/* ---- A: talk, open, search -------------------------------------------------------------- */
static void interact(void) {
    int fx = spur_facing_x(), fy = spur_facing_y();
    /* over a counter */
    int t = spur_tile_at(&sg.map, fx, fy);
    int n = spur_npc_at(fx, fy);
    if (n < 0 && t == T_COUNTER) n = spur_npc_at(fx + DX[sg.face], fy + DY[sg.face]);
    if (n >= 0) {
        /* they turn to face you */
        static const int OPP[4] = {1, 0, 3, 2};
        sg.npc_dir[n] = OPP[sg.face];
        spur_talk(n);
        return;
    }
    for (int i = 0; i < sg.map.nchest; i++) {
        const SpurChest *c = &sg.map.chest[i];
        if (c->x != fx || c->y != fy) continue;
        if (spur_chest_open(c->id)) {
            if (!c->hidden) spur_say("IT'S EMPTY.");
            else continue;
            return;
        }
        spur_open_chest(i);
        return;
    }
    if (t >= 0 && (SPUR_TILE_FLAGS[t] & TF_SEARCH)) { spur_search_tile(fx, fy, t); return; }
}

/* ---- one frame -------------------------------------------------------------------------- */
void spur_field_update(void) {
    sg.map_t++;
    if (sg.flash_poison > 0) sg.flash_poison--;
    for (int w = 0; w < SPUR_WISPS; w++) if (sg.wisp[w].alive && sg.wisp[w].t < 255) sg.wisp[w].t++;
    if (sg.msg_t > 0) sg.msg_t--;
    if (spur_ui_open()) { spur_ui_update(); return; }
    if (sg.pending_battle) {
        int tr = sg.pending_battle, fl = sg.pending_flag;
        sg.pending_battle = 0;
        spur_start_battle(tr, fl);
        return;
    }
    if (sg.pending_event) {
        int ev = sg.pending_event;
        sg.pending_event = 0;
        spur_event(ev);
        return;
    }
    npcs_update();
    if (sg.moving) {
        sg.move_t++;
        if (sg.move_t >= sg.step_frames) {
            sg.moving = 0;
            sg.move_t = 0;
            step_done();
        }
        return;
    }
    if (btnp(SPUR_BACK)) {
        if (sg.map_id == M_SALOON && !spur_flag(F_POSSE)) return;
        spur_open_field_menu();
        return;
    }
    if (btnp(SPUR_OK)) { interact(); return; }
    int dir = -1;
    if (btn(BTN_DOWN)) dir = 0;
    else if (btn(BTN_UP)) dir = 1;
    else if (btn(BTN_LEFT)) dir = 2;
    else if (btn(BTN_RIGHT)) dir = 3;
    if (dir < 0) return;
    sg.face = dir;
    int nx = sg.pos[0].x + DX[dir], ny = sg.pos[0].y + DY[dir];
    bool flying = sg.mounted == 2;
    bool can = spur_walkable(nx, ny, flying) && spur_npc_at(nx, ny) < 0;
    if (sg.map.kind != MK_TOWN && (nx < 0 || ny < 0 || nx >= sg.map.w || ny >= sg.map.h)) can = false;
    if (!can) {
        /* the story's gates speak when bumped */
        int t = spur_tile_at(&sg.map, nx, ny);
        if (btnp(BTN_DOWN | BTN_UP | BTN_LEFT | BTN_RIGHT) && t >= 0 && (t == T_FERRY || t == T_ANGELS || t == T_RUBBLE ||
            t == T_FIREWALL || t == T_LOCKED || t == T_BARRIER))
            spur_search_tile(nx, ny, t);
        return;
    }
    for (int i = 0; i < SPUR_TRAIL; i++) sg.prev[i] = sg.pos[i];
    for (int i = SPUR_TRAIL - 1; i > 0; i--) sg.pos[i] = sg.pos[i - 1];
    sg.pos[0].x = (int16_t)nx;
    sg.pos[0].y = (int16_t)ny;
    sg.pos[0].dir = (int16_t)dir;
    sg.moving = 1;
    sg.move_t = 0;
    sg.step_frames = sg.mounted == 2 ? 4 : sg.mounted == 1 ? 5 : 9;
}
