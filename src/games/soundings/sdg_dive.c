/* SOUNDINGS - the dive: swimming in the dark, the creatures of each cave
 * and how they move, their shots, chests and pearls, levers and doors,
 * cracked rock and charges, the ink sac's cloud, and the item menu's uses.
 *
 * Everything here is whole numbers (positions in 1/256 px), so a dive plays
 * the same on every platform. */
#include "sdg.h"

#define D (sdg.dive)
#define P (sdg.prog)

int sdg_dive_news;
int sdg_dive_news_mob;

/* a sine in 32 steps a turn, times 127 */
static const int8_t SINE32[32] = {0, 25, 49, 71, 90, 106, 117, 125, 127, 125, 117, 106, 90, 71, 49, 25,
                                  0, -25, -49, -71, -90, -106, -117, -125, -127, -125, -117, -106, -90, -71, -49, -25};
static int sin32(int a) { return SINE32[a & 31]; }

/* ---- the rock as it stands now ---------------------------------------------- */

static bool cell_solid(int c, int r) {
    if (c < 0 || r < 0 || c >= SDG_MW || r >= SDG_MH) return true;
    char ch = SDG_MAP[r][c];
    switch (ch) {
    case '#':
    case '^': return true;
    case '1': return !(P.doors & (1 << DOOR_WEST));
    case '2': return !(P.doors & (1 << DOOR_SHORT));
    case '=': return !(P.doors & (1 << DOOR_WARDEN));
    case 'F': return !(P.doors & (1 << DOOR_FINAL));
    case 'X': {
        int g = sdg_mi.wall_of[r][c];
        return g < 0 || !(P.walls & (1 << g));
    }
    default: return false;
    }
}

bool sdg_cell_open(int c, int r) { return !cell_solid(c, r); }

bool sdg_solid_px(int px, int py) {
    if (px < 0 || py < 0) return true;
    return cell_solid(px / SDG_T, py / SDG_T);
}

static bool box_solid(int cx, int cy, int hw, int hh) {
    return sdg_solid_px(cx - hw, cy - hh) || sdg_solid_px(cx + hw - 1, cy - hh) || sdg_solid_px(cx - hw, cy + hh - 1) ||
           sdg_solid_px(cx + hw - 1, cy + hh - 1) || sdg_solid_px(cx, cy - hh) || sdg_solid_px(cx, cy + hh - 1);
}

int sdg_depth(void) { return imax(0, (int)(D.y >> 8) - SDG_SURF_Y); }

/* ---- the creatures ----------------------------------------------------------- */

static int mob_kind_of(char ch) {
    switch (ch) {
    case 'e': return MK_FIZZLE;
    case 'u': return MK_PRICKLE;
    case 'a': return MK_FROND;
    case 'n': return MK_NEST;
    case 'g': return MK_GLOB;
    case 'v': return MK_SMOGVENT;
    case 'k': return MK_CLAMPVENT;
    case 'm': return MK_TINFIN;
    case 'w': return MK_WHORL;
    case 'j': return MK_JELLY;
    case 'f': return MK_GROPER;
    case 't': return MK_BURRNUT;
    case 's': return MK_STILTER;
    case 'i': return MK_LOUSE;
    case 'h': return MK_HAUNT;
    case 'q': return MK_SQUID;
    case 'r': return MK_GRINFISH;
    case 'o': return MK_WORM;
    default: return MK_FIZZLE;
    }
}

/* creatures that are only there once something brings them out */
static bool latent_kind(int k) { return k == MK_NEST || k == MK_SMOGVENT || k == MK_CLAMPVENT || k == MK_HAUNT; }

static void mob_home(SdgMob *m) {
    m->x = m->hx * 256;
    m->y = m->hy * 256;
    m->vx = m->vy = 0;
    m->t = (int16_t)((m->hx * 7 + m->hy * 13) % 97);
    m->t2 = 0;
    m->out = 0;
    m->state = MS_HOME;
    m->dir = (m->hx / SDG_T + m->hy / SDG_T) & 1 ? 1 : -1;
}

static void add_mob(int kind, int c, int r, int spawn) {
    if (D.nmob >= SDG_MOBS) return;
    SdgMob *m = &D.mob[D.nmob++];
    memset(m, 0, sizeof *m);
    m->kind = (uint8_t)kind;
    m->spawn = (uint8_t)spawn;
    m->region = (uint8_t)sdg_region_at(c, r);
    m->hx = c * SDG_T + SDG_T / 2;
    m->hy = r * SDG_T + SDG_T / 2;
    mob_home(m);
}

void sdg_dive_reset_mobs(void) {
    sdg_map_index();
    D.nmob = 0;
    for (int i = 0; i < sdg_mi.nspawn; i++)
        add_mob(mob_kind_of((char)sdg_mi.spawn[i].letter), sdg_mi.spawn[i].c, sdg_mi.spawn[i].r, i);
    add_mob(MK_WARDEN, sdg_mi.warden.c, sdg_mi.warden.r, 255);
    if (P.doors & (1 << DOOR_WARDEN)) D.mob[D.nmob - 1].state = MS_GONE, D.mob[D.nmob - 1].gone_at = 0xFFFFFFFFu;
    add_mob(MK_GLOAM, sdg_mi.gloam.c, sdg_mi.gloam.r, 255);
    memset(D.shot, 0, sizeof D.shot);
}

void sdg_dive_begin(void) {
    sdg_map_index();
    D.x = ((SDG_SURF_C0 + SDG_SURF_C1) / 2 * SDG_T) * 256;
    D.y = (SDG_SURF_Y + 10) * 256;
    D.face = 1;
    D.left_surface = 0;
    D.inv = 60;
    D.mist = 0;
    D.t = 0;
    D.haunt_t = 600;
    sdg_dive_reset_mobs();
}

static int dist_px(const SdgMob *m) {
    int dx = (int)((m->x - D.x) >> 8), dy = (int)((m->y - D.y) >> 8);
    return sdg_isqrt(dx * dx + dy * dy);
}

static void aim(int32_t fx, int32_t fy, int32_t tx, int32_t ty, int speed, int32_t *vx, int32_t *vy) {
    int dx = (int)((tx - fx) >> 8), dy = (int)((ty - fy) >> 8);
    int len = sdg_isqrt(dx * dx + dy * dy);
    if (len == 0) { *vx = 0; *vy = 0; return; }
    *vx = dx * speed / len;
    *vy = dy * speed / len;
}

/* moves a creature unless rock is in the way; returns false if blocked */
static bool mob_move(SdgMob *m, int32_t vx, int32_t vy, bool through) {
    int32_t nx = m->x + vx, ny = m->y + vy;
    if (!through && sdg_solid_px((int)(nx >> 8), (int)(ny >> 8))) return false;
    m->x = nx;
    m->y = ny;
    return true;
}

static void shoot(const SdgMob *m) {
    for (int i = 0; i < SDG_SHOTS; i++) {
        SdgShot *s = &D.shot[i];
        if (s->on) continue;
        s->on = 1;
        s->x = m->x;
        s->y = m->y;
        aim(m->x, m->y, D.x, D.y, 256, &s->vx, &s->vy);
        s->life = 240;
        sfx_play_name("sdg_pew");
        return;
    }
}

static void return_home(SdgMob *m, int speed) {
    int32_t vx, vy;
    aim(m->x, m->y, m->hx * 256, m->hy * 256, speed, &vx, &vy);
    if (iabs((int)((m->x >> 8) - m->hx)) + iabs((int)((m->y >> 8) - m->hy)) <= 2) { m->x = m->hx * 256; m->y = m->hy * 256; return; }
    if (!mob_move(m, vx, vy, false)) { m->x = m->hx * 256; m->y = m->hy * 256; }
}

static void mob_update(SdgMob *m, int idx) {
    bool hide = D.mist > 0;
    int dist = dist_px(m);
    m->t++;
    switch (m->kind) {
    case MK_FIZZLE:
    case MK_TINFIN: {
        int sp = m->kind == MK_TINFIN ? 410 : 384;
        if (m->t2 > 0) {             /* darting */
            m->t2--;
            if (!mob_move(m, m->vx, m->vy, false)) m->t2 = 0;
            if (m->t2 == 0) m->out = 60;
        } else if (m->out > 0) {
            m->out--;
        } else if (!hide && dist < 80) {
            aim(m->x, m->y, D.x, D.y, sp, &m->vx, &m->vy);
            m->t2 = 36;
            sfx_play_name("sdg_dart"); /* the silence breaks */
            m->dir = m->vx < 0 ? -1 : 1;
        } else if (iabs((int)(m->x >> 8) - m->hx) + iabs((int)(m->y >> 8) - m->hy) > 4) {
            return_home(m, 96);
        } else {
            mob_move(m, sin32(m->t / 4) / 2, sin32(m->t / 6 + 8) / 3, false);
        }
        break;
    }
    case MK_PRICKLE:
    case MK_BURRNUT: {
        int32_t vx = m->dir * 64, vy = sin32(m->t / 5) / 4;
        if (!mob_move(m, vx, vy, false) || iabs((int)(m->x >> 8) - m->hx) > 32) m->dir = (int8_t)-m->dir;
        break;
    }
    case MK_FROND:
    case MK_WHORL:
    case MK_LOUSE:
        if (!hide && dist < 112 && m->t % 150 == 0) shoot(m);
        break;
    case MK_STILTER: {
        int32_t vx = m->dir * 100;
        if (!mob_move(m, vx, 0, false) || iabs((int)(m->x >> 8) - m->hx) > 40) m->dir = (int8_t)-m->dir;
        /* a hop: up and back down every two seconds */
        int ph = m->t % 120;
        if (ph < 24) mob_move(m, 0, -256, false);
        else if (ph < 48) mob_move(m, 0, 256, false);
        else if (iabs((int)(m->y >> 8) - m->hy) > 0) mob_move(m, 0, m->y > m->hy * 256 ? -256 : 256, false);
        if (!hide && dist < 112 && m->t % 170 == 30) shoot(m);
        break;
    }
    case MK_NEST:
        if (m->state == MS_HOME) {
            if (!hide && dist < 56) { m->state = MS_ACTIVE; m->t = 0; sfx_play_name("sdg_burst"); }
        } else if (m->state == MS_ACTIVE) {
            int32_t vx, vy;
            int far = iabs((int)(m->x >> 8) - m->hx) + iabs((int)(m->y >> 8) - m->hy);
            if (!hide && far < 400) {
                aim(m->x, m->y, D.x, D.y, 218, &vx, &vy);
                if (!mob_move(m, vx, 0, false)) mob_move(m, 0, vy, false);
                else mob_move(m, 0, vy, false);
                m->dir = vx < 0 ? -1 : 1;
            } else {
                return_home(m, 160);
                if (m->x == m->hx * 256 && m->y == m->hy * 256) m->state = MS_HOME;
            }
        }
        break;
    case MK_GLOB: {
        if (!mob_move(m, m->dir * 128, 0, false)) m->dir = (int8_t)-m->dir;
        break;
    }
    case MK_SMOGVENT:
        if (m->state == MS_HOME) {
            if (m->t >= 240) { m->state = MS_ACTIVE; m->t = 0; }
        } else if (m->state == MS_ACTIVE) {
            if (m->t < 180) mob_move(m, sin32(m->t / 3) / 2, sin32(m->t / 5 + 8) / 2 - 30, false);
            else {
                return_home(m, 128);
                if (m->x == m->hx * 256 && m->y == m->hy * 256) { m->state = MS_HOME; m->t = 0; }
            }
        }
        break;
    case MK_CLAMPVENT:
        if (m->state == MS_HOME) {
            if (!hide && dist < 96) { m->state = MS_ACTIVE; m->t = 0; }
        } else if (m->state == MS_ACTIVE) {
            if (!hide && dist < 160) {
                int32_t vx = D.x < m->x ? -90 : 90;
                mob_move(m, vx, 0, false);
                m->dir = vx < 0 ? -1 : 1;
            } else {
                return_home(m, 90);
                if (m->x == m->hx * 256 && m->y == m->hy * 256) m->state = MS_HOME;
            }
        }
        break;
    case MK_JELLY: {
        /* a slow square, 48 px a side */
        int leg = (m->t / 160) & 3;
        static const int8_t SX[4] = {1, 0, -1, 0}, SY[4] = {0, 1, 0, -1};
        mob_move(m, SX[leg] * 77, SY[leg] * 77, false);
        break;
    }
    case MK_GROPER:
    case MK_GRINFISH: {
        int range = m->kind == MK_GROPER ? 120 : 96;
        if (!hide && dist < range) {
            int32_t vx, vy;
            if (!m->out) { m->out = 1; sfx_play_name("sdg_dart"); }
            aim(m->x, m->y, D.x, D.y, m->kind == MK_GROPER ? 102 : 140, &vx, &vy);
            if (!mob_move(m, vx, vy, false)) { mob_move(m, vx, 0, false); mob_move(m, 0, vy, false); }
            m->dir = vx < 0 ? -1 : 1;
        } else if (m->kind == MK_GRINFISH && iabs((int)(m->y >> 8) - m->hy) < 3) {
            m->out = 0;
            if (!mob_move(m, m->dir * 128, 0, false) || iabs((int)(m->x >> 8) - m->hx) > 48) m->dir = (int8_t)-m->dir;
        } else {
            m->out = 0;
            return_home(m, 96);
        }
        break;
    }
    case MK_HAUNT:
        if (m->state == MS_ACTIVE) {
            int here = sdg_region_at((int)(D.x >> 8) / SDG_T, (int)(D.y >> 8) / SDG_T);
            if (here != m->region || m->t > 900) { m->state = MS_HOME; m->t = 0; break; }
            if (!hide) {
                int32_t vx, vy;
                aim(m->x, m->y, D.x, D.y, 115, &vx, &vy);
                mob_move(m, vx, vy, true);
                m->dir = vx < 0 ? -1 : 1;
            }
        }
        break;
    case MK_SQUID: {
        int32_t vx = m->dir * 154;
        int32_t ny = (m->hy + sin32(m->t / 3) * 24 / 127) * 256;
        if (!mob_move(m, vx, 0, false) || iabs((int)(m->x >> 8) - m->hx) > 160) m->dir = (int8_t)-m->dir;
        if (!sdg_solid_px((int)(m->x >> 8), (int)(ny >> 8))) m->y = ny;
        break;
    }
    case MK_WORM: {
        /* anchored at home; the head reaches for the diver */
        int32_t tx = m->hx * 256, ty = m->hy * 256;
        if (!hide && dist < 88) {
            int32_t vx, vy;
            aim(tx, ty, D.x, D.y, 48 * 256, &vx, &vy);
            tx += vx;
            ty += vy;
        }
        int32_t vx, vy;
        aim(m->x, m->y, tx, ty, 192, &vx, &vy);
        if (iabs((int)((tx - m->x) >> 8)) + iabs((int)((ty - m->y) >> 8)) > 1) mob_move(m, vx, vy, true);
        m->dir = m->x < m->hx * 256 ? -1 : 1;
        break;
    }
    default: break;
    }
    (void)idx;
}

/* the haunts of a region turn up near the diver now and then */
static void haunt_spawns(void) {
    if (D.mist > 0) return;
    if (D.haunt_t > 0) { D.haunt_t--; return; }
    D.haunt_t = (int16_t)sdg_rand(420, 900);
    int here = sdg_region_at((int)(D.x >> 8) / SDG_T, (int)(D.y >> 8) / SDG_T);
    for (int i = 0; i < D.nmob; i++) {
        SdgMob *m = &D.mob[i];
        if (m->kind != MK_HAUNT || m->state != MS_HOME || m->region != here) continue;
        int a = sdg_rand(0, 31);
        m->x = D.x + sin32(a + 8) * 110 * 2;
        m->y = D.y + sin32(a) * 110 * 2;
        m->t = 0;
        m->state = MS_ACTIVE;
        return;
    }
}

static void mob_box(const SdgMob *m, int *hw, int *hh) {
    switch (m->kind) {
    case MK_WARDEN: *hw = 24; *hh = 22; break;
    case MK_GLOAM: *hw = 28; *hh = 20; break;
    case MK_NEST: *hw = 10; *hh = 7; break;
    case MK_SQUID: case MK_GROPER: case MK_GRINFISH: *hw = 8; *hh = 6; break;
    default: *hw = 6; *hh = 5; break;
    }
}

static bool mob_touchable(const SdgMob *m) {
    if (m->state == MS_GONE) return false;
    if (latent_kind(m->kind)) return m->state == MS_ACTIVE;
    return true;
}

/* ---- things to find ----------------------------------------------------------- */

void sdg_note(const char *a, const char *b);

void sdg_open_door(int door) {
    if (P.doors & (1 << door)) return;
    P.doors |= (uint8_t)(1 << door);
    P.doors_open++;
}

static bool near_cell(int c, int r, int pad) {
    int cx = c * SDG_T + SDG_T / 2, cy = r * SDG_T + SDG_T / 2;
    int x = (int)(D.x >> 8), y = (int)(D.y >> 8);
    return iabs(x - cx) <= SDG_T / 2 + SDG_HW + pad && iabs(y - cy) <= SDG_T / 2 + SDG_HH + pad;
}

static void open_chest(int i) {
    const SdgChest *ch = &SDG_CHEST[i];
    P.chests |= 1u << i;
    P.chests_open++;
    sfx_play_name("sdg_chest");
    switch (ch->kind) {
    case CH_GOLD:
        P.gold = sdg_cap_gold(P.gold + ch->n);
        snprintf(sdg.notebuf[0], sizeof sdg.notebuf[0], "%d GOLD!", ch->n);
        sdg_note(sdg.notebuf[0], NULL);
        break;
    case CH_RELIC:
        sdg_gain_relic(&P, ch->n);
        snprintf(sdg.notebuf[0], sizeof sdg.notebuf[0], "A %s!", SDG_RELIC_NAME[ch->n]);
        sdg_note(sdg.notebuf[0], NULL);
        break;
    default:
        if (P.npearl < SDG_PEARLS) P.pearl[P.npearl++] = (uint8_t)ch->n;
        sdg_note("A PEARL!", "IT WILL OPEN AT THE RAFT.");
        break;
    }
}

static void pull_lever(int lv) {
    P.levers |= (uint8_t)(1 << lv);
    sfx_play_name("sdg_lever");
    if (lv == LV_WEST) { sdg_open_door(DOOR_WEST); sdg_note("CLUNK. A GATE GRINDS OPEN.", NULL); }
    else if (lv == LV_SHORT) { sdg_open_door(DOOR_SHORT); sdg_note("CLUNK. A GATE GRINDS OPEN.", NULL); }
    else {
        bool both = (P.levers & (1 << LV_FINAL_W)) && (P.levers & (1 << LV_FINAL_E));
        if (both) { sdg_open_door(DOOR_FINAL); sdg_note("CLUNK. FAR BELOW, A DOOR OPENS.", NULL); }
        else sdg_note("CLUNK. SOMETHING SHIFTS FAR BELOW.", NULL);
    }
}

static void interact(void) {
    for (int i = 0; i < sdg_mi.nchest; i++)
        if (!(P.chests & (1u << i)) && near_cell(sdg_mi.chest[i].c, sdg_mi.chest[i].r, 0)) { open_chest(i); return; }
    for (int l = 0; l < LV_COUNT; l++)
        if (!(P.levers & (1 << l)) && near_cell(sdg_mi.lever[l].c, sdg_mi.lever[l].r, 0)) { pull_lever(l); return; }
    for (int i = 0; i < sdg_mi.nlore; i++)
        if (near_cell(sdg_mi.lore[i].c, sdg_mi.lore[i].r, 0)) {
            sdg_note(SDG_LORE_TEXT[i][0], SDG_LORE_TEXT[i][1]);
            sfx_play_name("sdg_tick");
            return;
        }
}

void sdg_bomb_blast(int px, int py) {
    bool broke = false;
    for (int r = py / SDG_T - 3; r <= py / SDG_T + 3; r++)
        for (int c = px / SDG_T - 3; c <= px / SDG_T + 3; c++) {
            if (c < 0 || r < 0 || c >= SDG_MW || r >= SDG_MH) continue;
            int g = sdg_mi.wall_of[r][c];
            if (g < 0 || (P.walls & (1 << g))) continue;
            int dx = c * SDG_T + SDG_T / 2 - px, dy = r * SDG_T + SDG_T / 2 - py;
            if (dx * dx + dy * dy > 40 * 40) continue;
            P.walls |= (uint8_t)(1 << g);
            P.doors_open++;
            broke = true;
        }
    sfx_play_name("sdg_boom");
    if (broke) sdg_note("BOOM! THE ROCK GIVES WAY.", NULL);
    else sdg_note("BOOM! NOTHING GIVES.", NULL);
}

bool sdg_dive_usable(int it) {
    if (it <= IT_NONE || it >= IT_COUNT) return false;
    int k = SDG_ITEM[it].kind;
    return k == K_POTION || k == K_EGG || k == K_BOMB || k == K_MIST || (SDG_ITEM[it].flags & IF_HOLY);
}

void sdg_use_item_dive(int d, int s, int target) {
    int it = P.equip[d][s];
    if (!sdg_dive_usable(it) || P.hp[d] <= 0 || P.uses[d][s] == 0) { sfx_play_name("sdg_nope"); return; }
    const SdgItem *I = &SDG_ITEM[it];
    P.uses[d][s]--;
    switch (I->kind) {
    case K_POTION:
        if (P.hp[target] > 0) P.hp[target] = (int16_t)imin(sdg_diver_maxhp(&P, target), P.hp[target] + I->heal);
        sfx_play_name("sdg_heal");
        break;
    case K_EGG:
        if (P.hp[target] <= 0) P.hp[target] = (int16_t)(sdg_diver_maxhp(&P, target) / 2);
        sfx_play_name("sdg_heal");
        break;
    case K_BOMB: sdg_bomb_blast((int)(D.x >> 8), (int)(D.y >> 8)); break;
    case K_MIST:
        D.mist = SDG_MIST_M * 256;
        sfx_play_name("sdg_ink");
        break;
    default:
        if (I->flags & IF_HOLY) {
            if (P.hp[target] > 0) P.hp[target] = (int16_t)imin(sdg_diver_maxhp(&P, target), P.hp[target] + sdg_rand(300, 400));
            sfx_play_name("sdg_heal");
        }
        break;
    }
}

/* ---- one frame -------------------------------------------------------------------- */

void sdg_dive_step(unsigned held, unsigned pressed) {
    sdg_dive_news = DV_NONE;
    D.t++;
    if (D.inv > 0) D.inv--;
    if (D.hit_flash > 0) D.hit_flash--;
    /* swim */
    int sp = SDG_SPEED * (sdg_party_holds(&P, IT_FLIPPERS) ? 2 : 1);
    int dx = ((held & BTN_RIGHT) ? 1 : 0) - ((held & BTN_LEFT) ? 1 : 0);
    int dy = ((held & BTN_DOWN) ? 1 : 0) - ((held & BTN_UP) ? 1 : 0);
    int32_t vx = dx * sp, vy = dy * sp;
    if (dx && dy) { vx = vx * 181 / 256; vy = vy * 181 / 256; }
    if (dx) D.face = (int8_t)dx;
    D.moving = (dx || dy) ? 1 : 0;
    int32_t ox = D.x, oy = D.y;
    if (vx && !box_solid((int)((D.x + vx) >> 8), (int)(D.y >> 8), SDG_HW, SDG_HH)) D.x += vx;
    if (vy && !box_solid((int)(D.x >> 8), (int)((D.y + vy) >> 8), SDG_HW, SDG_HH)) D.y += vy;
    if (D.mist > 0) {
        int mx = (int)(D.x - ox), my = (int)(D.y - oy);
        D.mist -= sdg_isqrt(mx * mx + my * my);
        if (D.mist < 0) D.mist = 0;
    }
    int x = (int)(D.x >> 8), y = (int)(D.y >> 8);
    if (sdg_depth() > P.deepest) P.deepest = (int16_t)sdg_depth();
    if (y > SDG_SURF_Y + 40) D.left_surface = 1;
    if (D.left_surface && y - SDG_HH <= SDG_SURF_Y + 2 && x >= SDG_SURF_C0 * SDG_T && x < (SDG_SURF_C1 + 1) * SDG_T) {
        sdg_dive_news = DV_SURFACE;
        return;
    }
    /* heads are picked up by touch */
    for (int h = 0; h < sdg_mi.nhead; h++)
        if (!(P.heads & (1 << h)) && near_cell(sdg_mi.head[h].c, sdg_mi.head[h].r, -2)) {
            P.heads |= (uint8_t)(1 << h);
            sfx_play_name("sdg_head");
            snprintf(sdg.notebuf[1], sizeof sdg.notebuf[1], "%s.", SDG_HEAD_NAME[h]);
            sdg_note("FOUND SOMETHING:", sdg.notebuf[1]);
        }
    if (pressed & BTN_A) interact();
    /* the creatures */
    haunt_spawns();
    for (int i = 0; i < D.nmob; i++) {
        SdgMob *m = &D.mob[i];
        if (m->state == MS_GONE) continue; /* back only after surfacing */
        mob_update(m, i);
    }
    /* shots: they sting but never finish anyone */
    for (int i = 0; i < SDG_SHOTS; i++) {
        SdgShot *s = &D.shot[i];
        if (!s->on) continue;
        s->x += s->vx;
        s->y += s->vy;
        if (--s->life == 0 || sdg_solid_px((int)(s->x >> 8), (int)(s->y >> 8))) { s->on = 0; continue; }
        int sx = (int)(s->x >> 8), sy = (int)(s->y >> 8);
        if (iabs(sx - x) <= SDG_HW + 1 && iabs(sy - y) <= SDG_HH + 1) {
            s->on = 0;
            int pick[3], n = 0;
            for (int d = 0; d < 3; d++)
                if (P.hp[d] > 1) pick[n++] = d;
            if (n) {
                int d = pick[sdg_rand(0, n - 1)];
                P.hp[d] = (int16_t)imax(1, P.hp[d] - SDG_SHOT_DMG);
            }
            D.hit_flash = 10;
            sfx_play_name("sdg_sting");
        }
    }
    /* touching a creature starts a fight */
    if (D.inv == 0)
        for (int i = 0; i < D.nmob; i++) {
            SdgMob *m = &D.mob[i];
            if (!mob_touchable(m)) continue;
            int hw, hh;
            mob_box(m, &hw, &hh);
            if (rects_overlap(x - SDG_HW, y - SDG_HH, SDG_HW * 2, SDG_HH * 2, (int)(m->x >> 8) - hw, (int)(m->y >> 8) - hh, hw * 2, hh * 2)) {
                sdg_dive_news = DV_BATTLE;
                sdg_dive_news_mob = i;
                return;
            }
        }
}
