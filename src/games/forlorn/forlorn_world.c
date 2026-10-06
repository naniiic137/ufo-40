/* FORLORN HOPE - the rules: the volunteer, the five trades and their gifts,
 * keys, doors, plates and the blocks they raise, chutes, waystones, pouches,
 * shots and the run's lives. Everything here persists for the whole run:
 * nothing is put back when a volunteer is lost. */
#include "forlorn.h"

const char *const FRL_CLASS_NAME[FRC_COUNT] = {"MASON", "HUNTER", "RUNNER", "TINKER", "SAPPER"};
/* ammo a volunteer of each trade carries (and a pouch fills back up) */
const int FRL_AMMO[FRC_COUNT] = {0, 20, 5, 15, 0};
const int FRL_DAMAGE[FRC_COUNT] = {2, 1, 1, 2, FRL_BLAST_DMG};

/* ------------------------------------------------------------------ */
/* small maths                                                          */

int frl_isqrt(int v) {
    if (v <= 0) return 0;
    int r = 0, b = 1 << 30;
    while (b > v) b >>= 2;
    while (b) {
        if (v >= r + b) { v -= r + b; r = (r >> 1) + b; }
        else r >>= 1;
        b >>= 2;
    }
    return r;
}

void frl_aim(int32_t x, int32_t y, int32_t tx, int32_t ty, int32_t speed, int32_t *vx, int32_t *vy) {
    int dx = (tx - x) >> 4, dy = (ty - y) >> 4; /* 1/16 px: room to square */
    int d = frl_isqrt(dx * dx + dy * dy);
    if (d <= 0) { *vx = speed; *vy = 0; return; }
    *vx = (int32_t)((int64_t)dx * speed / d);
    *vy = (int32_t)((int64_t)dy * speed / d);
}

/* ------------------------------------------------------------------ */
/* tiles                                                                */

int frl_tile(const FrlWorld *w, int tx, int ty) {
    if (tx < 0 || tx >= FRL_MW || ty >= FRL_MH) return FT_ROCK;
    if (ty < 0) return FT_AIR;
    return w->tile[ty][tx];
}

bool frl_plate_down(const FrlWorld *w, int i) { return i >= 0 && i < FRL_PLATES && w->plate[i].down; }

bool frl_solid(const FrlWorld *w, int tx, int ty) {
    switch (frl_tile(w, tx, ty)) {
    case FT_ROCK: case FT_BRICK: case FT_WOOD: case FT_LOOSE: case FT_SEAL: case FT_STONE:
    case FT_DOOR: case FT_COMB: case FT_GULP:
        return true;
    case FT_BLOCK1: return w->plate[0].down;
    case FT_BLOCK2: return w->plate[1].down;
    case FT_BLOCK3: return w->plate[2].down;
    default: return false;
    }
}

bool frl_box_solid(const FrlWorld *w, int px, int py, int bw, int bh) {
    int x0 = px / FRL_T - (px < 0), x1 = (px + bw - 1) / FRL_T;
    int y0 = py / FRL_T - (py < 0), y1 = (py + bh - 1) / FRL_T;
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++)
            if (frl_solid(w, tx, ty)) return true;
    return false;
}

void frl_add_fx(FrlWorld *w, int x, int y, int kind) {
    if (w->nfx >= FRL_FX) return;
    w->fx[w->nfx++] = (FrlFx){(int16_t)x, (int16_t)y, (uint8_t)kind};
}

/* ------------------------------------------------------------------ */
/* the map                                                              */

static int foe_kind_of(char c) {
    switch (c) {
    case 'e': return FK_WALLEYE;
    case 's': return FK_SHELLBACK;
    case 't': return FK_HATCHET;
    case 'q': return FK_SQUAWKER;
    case 'r': return FK_TUSKER;
    case 'i': case 'I': return FK_IDOL;
    case 'w': return FK_DRAKE;
    case 'H': return FK_BELL;
    case 'k': return FK_KNIGHT;
    case 'L': return FK_BLOATER;
    case 'h': return FK_BROODHEN;
    case 'M': return FK_HORNHEAD;
    case 'S': return FK_STINGBACK;
    case 'G': return FK_GULPER;
    case 'T': return FK_HEART;
    default: return FK_NONE;
    }
}

void frl_world_new(FrlWorld *w, int players, uint8_t allowed) {
    memset(w, 0, sizeof *w);
    for (int ty = 0; ty < FRL_MH; ty++) {
        const char *row = FRL_MAP[ty];
        for (int tx = 0; tx < FRL_MW; tx++) {
            char c = row[tx];
            uint8_t t = FT_AIR;
            switch (c) {
            case '#': t = FT_ROCK; break;
            case '=': t = FT_BRICK; break;
            case '%': t = FT_WOOD; break;
            case ',': t = FT_LEAVES; break;
            case '!': t = FT_TRUNK; break;
            case '^': t = FT_SPIKES; break;
            case '*': t = FT_LOOSE; break;
            case '$': t = FT_SEAL; break;
            case 'D': t = FT_DOOR; break;
            case 'g': t = FT_GULP; break;     /* a gulper's body, under its head */
            case 'a': t = FT_BLOCK1; break;
            case 'b': t = FT_BLOCK2; break;
            case 'c': t = FT_BLOCK3; break;
            case 'm':
                t = FT_COMB;
                if (w->ncomb < FRL_COMBS) w->comb[w->ncomb++] = (FrlComb){(int16_t)tx, (int16_t)ty, 0, 0, 0, 0, FRL_COMB_HP};
                break;
            case 'K':
                if (w->nkey < FRL_KEYS) w->key[w->nkey++] = (FrlKey){(int16_t)tx, (int16_t)ty, 0};
                break;
            case '1': case '2': case '3':
                w->plate[c - '1'] = (FrlPlate){(int16_t)tx, (int16_t)ty, 0, 0};
                break;
            case 'B':
                w->base_x = (int16_t)(tx * FRL_T + FRL_T / 2);
                w->base_y = (int16_t)((ty + 1) * FRL_T);
                break;
            case 'P':
                w->pad_x = (int16_t)(tx * FRL_T + FRL_T / 2);
                w->pad_y = (int16_t)((ty + 1) * FRL_T);
                break;
            case 'o': case 'O':
                if (w->ndrain < FRL_DRAINS) w->drain[w->ndrain++] = (FrlDrain){(int16_t)tx, (int16_t)ty, (uint8_t)(c == 'O'), 0, 0, 0};
                break;
            case 'e': t = tx >= 74 && ty < 60 ? FT_BRICK : FT_ROCK; break;
            default: break;
            }
            w->tile[ty][tx] = t;
        }
    }
    /* the foes, once the tiles are in (the gulper's body needs its shaft) */
    for (int ty = 0; ty < FRL_MH; ty++)
        for (int tx = 0; tx < FRL_MW; tx++) {
            int k = foe_kind_of(FRL_MAP[ty][tx]);
            if (!k) continue;
            int bw, bh;
            frl_foe_size(k, &bw, &bh);
            int px, py;
            if (k == FK_WALLEYE || k == FK_GULPER) { px = tx * FRL_T; py = ty * FRL_T; }
            else if (k == FK_BELL) {
                /* hung from the cell's top, nudged clear of a wall beside it */
                px = tx * FRL_T + (FRL_T - bw) / 2;
                py = ty * FRL_T;
                for (int n = 0; n < 3 && frl_box_solid(w, px, py, bw, bh); n++) px++;
            }
            else if (bw <= FRL_T) { px = tx * FRL_T + (FRL_T - bw) / 2; py = (ty + 1) * FRL_T - bh; }
            else { px = tx * FRL_T; py = (ty + 1) * FRL_T - bh; }
            int i = frl_foe_add(w, k, px, py);
            if (i < 0) continue;
            FrlFoe *f = &w->foe[i];
            if (FRL_MAP[ty][tx] == 'i') f->face = -1;
            if (FRL_MAP[ty][tx] == 'I') f->face = 1;
            if (k == FK_HEART) w->hearts_left++;
        }
    w->nplaced = w->nfoe;
    /* each comb's mouth: the first open cell below, beside or above it */
    for (int i = 0; i < w->ncomb; i++) {
        FrlComb *c = &w->comb[i];
        static const int8_t D[4][2] = {{0, 1}, {-1, 0}, {1, 0}, {0, -1}};
        c->mx = c->tx;
        c->my = (int16_t)(c->ty + 1);
        for (int k = 0; k < 4; k++)
            if (!frl_solid(w, c->tx + D[k][0], c->ty + D[k][1])) { c->mx = (int16_t)(c->tx + D[k][0]); c->my = (int16_t)(c->ty + D[k][1]); break; }
    }
    w->lives = FRL_LIVES;
    w->players = (uint8_t)(players == 2 ? 2 : 1);
    w->allowed = allowed ? allowed : 0x1F;
    w->sel = 0;
    while (!(w->allowed & (1 << w->sel))) w->sel++;
    w->phase = FWP_SELECT;
    w->cam_x = (int16_t)iclamp(w->base_x - SCREEN_W / 2, 0, FRL_MW * FRL_T - SCREEN_W);
    w->cam_y = (int16_t)iclamp(w->base_y - 20 - FRL_VH / 2, 0, FRL_MH * FRL_T - FRL_VH);
}

/* ------------------------------------------------------------------ */
/* the volunteer                                                        */

int frl_unit_cx(const FrlWorld *w) { return (w->u.x >> 8) + FRL_UW / 2; }
int frl_unit_cy(const FrlWorld *w) { return (w->u.y >> 8) + FRL_UH / 2; }

static void unit_box(const FrlWorld *w, int *x, int *y) { *x = w->u.x >> 8; *y = w->u.y >> 8; }

bool frl_unit_on_pad(const FrlWorld *w) {
    int x, y;
    unit_box(w, &x, &y);
    return rects_overlap(x, y, FRL_UW, FRL_UH, w->pad_x - 6, w->pad_y - 14, 12, 14);
}

bool frl_unit_at_way(const FrlWorld *w) {
    if (!w->way_on) return false;
    int x, y;
    unit_box(w, &x, &y);
    return rects_overlap(x, y, FRL_UW, FRL_UH, w->way_x - 5, w->way_y - 7, 10, 14);
}

bool frl_way_red(const FrlWorld *w) {
    if (!w->way_on) return false;
    for (int i = 0; i < w->nfoe; i++) {
        const FrlFoe *f = &w->foe[i];
        if (!f->on || f->kind == FK_GULPER || f->kind == FK_WALLEYE) continue;
        int fx = (f->x >> 8) + f->w / 2, fy = (f->y >> 8) + f->h / 2;
        if (iabs(fx - w->way_x) < 30 && iabs(fy - w->way_y) < 24) return true;
    }
    return false;
}

/* the chute (if any) whose cells the box overlaps, in a column it is centred on */
int frl_pipe_at(const FrlWorld *w, int px, int py, int bw, int bh) {
    int cx = px + bw / 2;
    for (int i = 0; i < w->npipe; i++) {
        const FrlPipe *p = &w->pipe[i];
        if (cx < p->tx * FRL_T || cx >= (p->tx + 1) * FRL_T) continue;
        if (rects_overlap(px, py, bw, bh, p->tx * FRL_T, p->ty * FRL_T, FRL_T, p->len * FRL_T)) return i;
    }
    return -1;
}

static void open_door(FrlWorld *w, int tx, int ty) {
    for (int y = ty - 1; y <= ty + 1; y++)
        if (frl_tile(w, tx, y) == FT_DOOR) w->tile[y][tx] = FT_DOOR_OPEN;
    w->keys--;
    w->doors++;
    w->ev |= FEV_DOOR;
    frl_add_fx(w, tx * FRL_T + 5, ty * FRL_T + 5, FFX_KEY);
}

/* a door in the box's way, with a key to open it */
static bool try_door(FrlWorld *w, int px, int py) {
    if (w->keys <= 0) return false;
    int x0 = px / FRL_T, x1 = (px + FRL_UW - 1) / FRL_T;
    int y0 = py / FRL_T, y1 = (py + FRL_UH - 1) / FRL_T;
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++)
            if (frl_tile(w, tx, ty) == FT_DOOR) { open_door(w, tx, ty); return true; }
    return false;
}

static void move_unit(FrlWorld *w) {
    FrlUnit *u = &w->u;
    /* across, a pixel at a time */
    int32_t nx = u->x + u->vx;
    int px = u->x >> 8, py = u->y >> 8, tx = nx >> 8;
    while (px != tx) {
        int s = tx > px ? 1 : -1;
        if (frl_box_solid(w, px + s, py, FRL_UW, FRL_UH) && !(try_door(w, px + s, py) && !frl_box_solid(w, px + s, py, FRL_UW, FRL_UH))) {
            nx = (int32_t)(s > 0 ? (px << 8) | 0xFF : px << 8);
            u->vx = 0;
            break;
        }
        px += s;
    }
    u->x = nx;
    /* and down or up */
    int32_t ny = u->y + u->vy;
    px = u->x >> 8;
    int ty = ny >> 8;
    while (py != ty) {
        int s = ty > py ? 1 : -1;
        if (frl_box_solid(w, px, py + s, FRL_UW, FRL_UH)) {
            ny = (int32_t)(s > 0 ? (py << 8) | 0xFF : py << 8);
            u->vy = 0;
            break;
        }
        py += s;
    }
    u->y = ny;
    u->ground = frl_box_solid(w, u->x >> 8, (u->y >> 8) + 1, FRL_UW, FRL_UH);
}

static void run_accel(FrlUnit *u, int hx) {
    int acc = u->ground ? FRL_ACC_GROUND : FRL_ACC_AIR, dec = u->ground ? FRL_DEC_GROUND : FRL_ACC_AIR;
    if (hx && u->vx * hx >= 0) {
        if (iabs(u->vx) < FRL_RUN) u->vx = iclamp(u->vx + hx * acc, -FRL_RUN, FRL_RUN);
    } else if (hx) {
        u->vx += hx * (dec + acc);
    } else {
        if (u->vx > 0) u->vx = imax(0, u->vx - dec);
        else if (u->vx < 0) u->vx = imin(0, u->vx + dec);
    }
}

FrlShot *frl_shot_add(FrlWorld *w, int kind, int32_t x, int32_t y, int32_t vx, int32_t vy, int life, bool mine) {
    for (int i = 0; i < FRL_SHOTS; i++)
        if (!w->shot[i].on) {
            w->shot[i] = (FrlShot){(uint8_t)kind, 1, (uint8_t)mine, x, y, vx, vy, (int16_t)life, 0};
            return &w->shot[i];
        }
    return NULL;
}

void frl_hurt_foe(FrlWorld *w, int i, int dmg) {
    FrlFoe *f = &w->foe[i];
    if (!f->on) return;
    if (f->kind == FK_GULPER) { w->ev |= FEV_TING; return; }
    f->hp = (int16_t)(f->hp - dmg);
    f->flash = 8;
    int cx = (f->x >> 8) + f->w / 2, cy = (f->y >> 8) + f->h / 2;
    if (f->hp <= 0) {
        f->on = 0;
        f->hp = 0;
        w->ev |= FEV_KILL;
        frl_add_fx(w, cx, cy, f->kind == FK_HEART ? FFX_HEART : FFX_KILL);
        if (f->kind == FK_HEART) {
            w->hearts_left--;
            w->ev |= FEV_HEART;
            w->shake = 30;
        }
        if (f->minion && f->owner >= 0 && f->owner < w->nfoe && w->foe[f->owner].kids) w->foe[f->owner].kids--;
    } else {
        w->ev |= FEV_HIT;
        frl_add_fx(w, cx, cy, FFX_HIT);
    }
}

/* damage to the comb in cell (tx, ty), if there is one */
bool frl_hurt_comb(FrlWorld *w, int tx, int ty, int dmg) {
    for (int i = 0; i < w->ncomb; i++) {
        FrlComb *c = &w->comb[i];
        if (c->tx != tx || c->ty != ty || c->hp <= 0) continue;
        c->hp = (int16_t)(c->hp - dmg);
        int x = tx * FRL_T + 5, y = ty * FRL_T + 5;
        if (c->hp <= 0) {
            c->hp = 0;
            w->tile[ty][tx] = FT_AIR;
            w->ev |= FEV_KILL;
            frl_add_fx(w, x, y, FFX_KILL);
        } else {
            w->ev |= FEV_HIT;
            frl_add_fx(w, x, y, FFX_HIT);
        }
        return true;
    }
    return false;
}

void frl_blast(FrlWorld *w, int cx, int cy) {
    w->ev |= FEV_BLAST;
    w->shake = imax(w->shake, 16);
    frl_add_fx(w, cx, cy, FFX_BLAST);
    for (int i = 0; i < w->nfoe; i++) {
        FrlFoe *f = &w->foe[i];
        if (!f->on) continue;
        int fx = (f->x >> 8), fy = (f->y >> 8);
        int nx = iclamp(cx, fx, fx + f->w - 1), ny = iclamp(cy, fy, fy + f->h - 1);
        int dx = nx - cx, dy = ny - cy;
        if (dx * dx + dy * dy <= FRL_BLAST_R * FRL_BLAST_R) frl_hurt_foe(w, i, FRL_BLAST_DMG);
    }
    for (int ty = (cy - FRL_BLAST_R) / FRL_T - 1; ty <= (cy + FRL_BLAST_R) / FRL_T + 1; ty++)
        for (int tx = (cx - FRL_BLAST_R) / FRL_T - 1; tx <= (cx + FRL_BLAST_R) / FRL_T + 1; tx++) {
            int t = frl_tile(w, tx, ty);
            if (t != FT_LOOSE && t != FT_SEAL && t != FT_COMB) continue;
            int dx = tx * FRL_T + 5 - cx, dy = ty * FRL_T + 5 - cy;
            if (dx * dx + dy * dy > FRL_BLAST_R * FRL_BLAST_R) continue;
            if (t == FT_COMB) { frl_hurt_comb(w, tx, ty, FRL_BLAST_DMG); continue; }
            w->tile[ty][tx] = FT_AIR;
            w->ev |= FEV_BREAK;
            frl_add_fx(w, tx * FRL_T + 5, ty * FRL_T + 5, FFX_BREAK);
        }
}

/* where a mason's stone goes: the cell of the volunteer's middle, or the one
 * above if that one is taken */
static bool stone_ok(const FrlWorld *w, int tx, int ty) {
    if (tx < 0 || tx >= FRL_MW || ty < 0 || ty >= FRL_MH) return false;
    switch (w->tile[ty][tx]) {
    case FT_AIR: case FT_LEAVES: case FT_TRUNK: case FT_SPIKES: case FT_DOOR_OPEN: return true;
    case FT_BLOCK1: case FT_BLOCK2: case FT_BLOCK3: return !frl_solid(w, tx, ty);
    default: return false;
    }
}

/* the trade's gift: what a volunteer leaves when they give themselves up */
static void gift(FrlWorld *w) {
    FrlUnit *u = &w->u;
    int cx = frl_unit_cx(w), cy = frl_unit_cy(w);
    switch (u->cls) {
    case FRC_MASON: {
        int tx = cx / FRL_T, ty = cy / FRL_T;
        if (u->below) ty = ((u->y >> 8) + FRL_UH - 1) / FRL_T + 1; /* just under the arrival */
        if (!stone_ok(w, tx, ty)) ty--;
        if (stone_ok(w, tx, ty)) {
            w->tile[ty][tx] = FT_STONE;
            w->stones++;
            w->ev |= FEV_STONE;
            frl_add_fx(w, tx * FRL_T + 5, ty * FRL_T + 5, FFX_STONE);
        }
        break;
    }
    case FRC_HUNTER: {
        int y = cy;
        while (y < FRL_MH * FRL_T - 1 && !frl_box_solid(w, cx - 3, y, 6, 1)) y++;
        if (w->npouch < FRL_POUCHES) w->pouch[w->npouch++] = (FrlPouch){(int16_t)cx, (int16_t)y};
        w->ev |= FEV_POUCH;
        break;
    }
    case FRC_RUNNER:
        w->way_on = 1;
        w->way_x = (int16_t)cx;
        w->way_y = (int16_t)cy;
        w->ev |= FEV_WAY;
        frl_add_fx(w, cx, cy, FFX_WARP);
        break;
    case FRC_TINKER: {
        int tx = cx / FRL_T, ty = cy / FRL_T;
        int len = imin(FRL_PIPE_LEN, FRL_MH - 1 - ty);
        if (len > 0 && w->npipe < FRL_PIPES) w->pipe[w->npipe++] = (FrlPipe){(int16_t)tx, (int16_t)ty, (int16_t)len};
        w->ev |= FEV_PIPE;
        break;
    }
    case FRC_SAPPER:
        frl_blast(w, cx, cy);
        break;
    }
}

void frl_unit_die(FrlWorld *w, int cause) {
    if (w->phase != FWP_PLAY) return;
    if (cause != FCAUSE_GAVE && w->u.charge >= FRL_ARMED) gift(w);
    w->last_cause = (uint8_t)cause;
    w->lost++;
    w->phase = FWP_DEAD;
    w->phase_t = 0;
    w->ev |= cause == FCAUSE_EATEN ? FEV_EATEN : FEV_DIE;
    frl_add_fx(w, frl_unit_cx(w), frl_unit_cy(w), FFX_DIE);
}

void frl_give_up(FrlWorld *w) {
    if (w->phase != FWP_PLAY) return;
    gift(w);
    frl_unit_die(w, FCAUSE_GAVE);
}

void frl_choose(FrlWorld *w, int cls) {
    FrlUnit *u = &w->u;
    memset(u, 0, sizeof *u);
    u->cls = (uint8_t)cls;
    u->player = (uint8_t)(w->units % w->players);
    u->x = (w->base_x - FRL_UW / 2) << 8;
    u->y = (w->base_y - FRL_UH) << 8;
    u->face = 1;
    u->ammo = (int16_t)FRL_AMMO[cls];
    u->ground = 1;
    w->units++;
    w->phase = FWP_PLAY;
    w->phase_t = 0;
}

static void warp_to(FrlWorld *w, int px, int py) {
    FrlUnit *u = &w->u;
    u->mode = FUM_WARP;
    u->mode_t = 0;
    u->tx = (px - FRL_UW / 2) << 8;
    u->ty = (py - FRL_UH) << 8;
    u->vx = u->vy = 0;
    w->ev |= FEV_WARP;
    frl_add_fx(w, frl_unit_cx(w), frl_unit_cy(w), FFX_WARP);
}

static void enter_pipe(FrlWorld *w, int i) {
    FrlUnit *u = &w->u;
    const FrlPipe *p = &w->pipe[i];
    /* chutes that overlap in one column join into one longer chute */
    int bot = p->ty + p->len - 1;
    bool grew = true;
    while (grew) {
        grew = false;
        for (int k = 0; k < w->npipe; k++) {
            const FrlPipe *q = &w->pipe[k];
            if (q->tx != p->tx) continue;
            int qb = q->ty + q->len - 1;
            if (q->ty <= bot + 1 && qb > bot) { bot = qb; grew = true; }
        }
    }
    /* out at the lowest open cell (a chute through rock ends where there's room) */
    int cy = frl_unit_cy(w) / FRL_T;
    int out = -1;
    for (int y = bot; y > cy; y--)
        if (!frl_solid(w, p->tx, y)) { out = y; break; }
    if (out < 0) return;
    u->mode = FUM_PIPE;
    u->mode_t = 0;
    u->tx = (p->tx * FRL_T + (FRL_T - FRL_UW) / 2) << 8;
    u->ty = ((out + 1) * FRL_T - FRL_UH) << 8;
    u->x = u->tx;
    u->vx = u->vy = 0;
    w->ev |= FEV_CHUTE;
}

static bool pressed(const FrlWorld *w, uint32_t m) { return (w->ctl & m) && !(w->ctl_prev & m); }
static bool held(const FrlWorld *w, uint32_t m) { return (w->ctl & m) != 0; }
static bool released(const FrlWorld *w, uint32_t m) { return !(w->ctl & m) && (w->ctl_prev & m); }

static void attack(FrlWorld *w) {
    FrlUnit *u = &w->u;
    int cx = frl_unit_cx(w), cy = frl_unit_cy(w);
    switch (u->cls) {
    case FRC_MASON:
        u->atk_t = 14;
        u->atk_cd = 18;
        u->swing_no++;
        w->ev |= FEV_SWING;
        break;
    case FRC_HUNTER: case FRC_RUNNER: case FRC_TINKER:
        if (u->ammo <= 0) { w->ev |= FEV_EMPTY; u->atk_cd = 10; return; }
        u->ammo--;
        u->atk_t = 10;
        if (u->cls == FRC_HUNTER) {
            frl_shot_add(w, FS_BULLET, (cx + u->face * 4) << 8, (cy - 1) << 8, u->face * 4 * FRL_ONE, 0, 45, true);
            u->atk_cd = 12;
        } else if (u->cls == FRC_RUNNER) {
            frl_shot_add(w, FS_STAR, (cx + u->face * 3) << 8, (cy - 1) << 8, u->face * 3 * FRL_ONE, 0, 24, true);
            u->atk_cd = 14;
        } else {
            frl_shot_add(w, FS_WRENCH, (cx + u->face * 3) << 8, (cy - 2) << 8, u->face * 512, -256, 70, true);
            u->atk_cd = 18;
        }
        w->ev |= FEV_SHOOT;
        break;
    default: break;
    }
}

/* the sword's reach this frame (false: not swinging) */
static bool sword_box(const FrlWorld *w, int *x, int *y, int *bw, int *bh) {
    const FrlUnit *u = &w->u;
    if (u->cls != FRC_MASON || u->atk_t < 4 || u->atk_t > 12) return false;
    int cx = frl_unit_cx(w);
    *bw = 14;
    *bh = 13;
    *x = u->face > 0 ? cx + 1 : cx - 15;
    *y = (u->y >> 8) - 3;
    return true;
}

static void unit_hazards(FrlWorld *w) {
    int px, py;
    unit_box(w, &px, &py);
    /* spikes: the lower half of a spike cell */
    for (int ty = py / FRL_T; ty <= (py + FRL_UH - 1) / FRL_T; ty++)
        for (int tx = px / FRL_T; tx <= (px + FRL_UW - 1) / FRL_T; tx++)
            if (frl_tile(w, tx, ty) == FT_SPIKES && py + FRL_UH > ty * FRL_T + 5) { frl_unit_die(w, FCAUSE_SPIKES); return; }
    /* foes */
    for (int i = 0; i < w->nfoe; i++) {
        FrlFoe *f = &w->foe[i];
        if (!f->on || !frl_foe_deadly(f)) continue;
        if (!rects_overlap(px, py, FRL_UW, FRL_UH, (f->x >> 8) + 1, (f->y >> 8) + 1, f->w - 2, f->h - 2)) continue;
        if (f->kind == FK_GULPER) {
            /* it swallows the volunteer whole, and is never seen again */
            f->on = 0;
            int gx = (f->x >> 8) / FRL_T;
            for (int y = (f->y >> 8) / FRL_T + 1; y < FRL_MH && w->tile[y][gx] == FT_GULP; y++)
                w->tile[y][gx] = w->tile[y][gx + 1] = FT_AIR;
            frl_unit_die(w, FCAUSE_EATEN);
            return;
        }
        frl_unit_die(w, FCAUSE_FOE);
        return;
    }
}

static void unit_pickups(FrlWorld *w) {
    FrlUnit *u = &w->u;
    int px, py;
    unit_box(w, &px, &py);
    for (int i = 0; i < w->nkey; i++) {
        FrlKey *k = &w->key[i];
        if (k->taken || !rects_overlap(px, py, FRL_UW, FRL_UH, k->tx * FRL_T + 2, k->ty * FRL_T + 1, 6, 8)) continue;
        k->taken = 1;
        w->keys++;
        w->ev |= FEV_KEY;
        frl_add_fx(w, k->tx * FRL_T + 5, k->ty * FRL_T + 5, FFX_KEY);
    }
    if (FRL_AMMO[u->cls] && u->ammo < FRL_AMMO[u->cls])
        for (int i = 0; i < w->npouch; i++)
            if (rects_overlap(px, py, FRL_UW, FRL_UH, w->pouch[i].x - 4, w->pouch[i].y - 6, 8, 6)) {
                u->ammo = (int16_t)FRL_AMMO[u->cls];
                w->ev |= FEV_AMMO;
                break;
            }
}

static void sword_hits(FrlWorld *w) {
    int sx, sy, sw, sh;
    if (!sword_box(w, &sx, &sy, &sw, &sh)) return;
    for (int i = 0; i < w->nfoe; i++) {
        FrlFoe *f = &w->foe[i];
        if (!f->on || f->hit_no == w->u.swing_no) continue;
        if (!rects_overlap(sx, sy, sw, sh, f->x >> 8, f->y >> 8, f->w, f->h)) continue;
        f->hit_no = w->u.swing_no;
        frl_hurt_foe(w, i, FRL_DAMAGE[FRC_MASON]);
    }
    if (w->u.atk_t == 8)                   /* once a swing */
        for (int i = 0; i < w->ncomb; i++) {
            const FrlComb *c = &w->comb[i];
            if (c->hp > 0 && rects_overlap(sx, sy, sw, sh, c->tx * FRL_T, c->ty * FRL_T, FRL_T, FRL_T))
                frl_hurt_comb(w, c->tx, c->ty, FRL_DAMAGE[FRC_MASON]);
        }
}

static void unit_step(FrlWorld *w) {
    FrlUnit *u = &w->u;
    u->life_t++;
    if (u->atk_t) u->atk_t--;
    if (u->atk_cd) u->atk_cd--;
    if (u->mode == FUM_PIPE || u->mode == FUM_WARP) {
        /* the charge carries through a waystone or a chute, and can be let
         * go there: in a chute it goes off where the volunteer is; through
         * a waystone it goes off on arrival (a mason's stone just below) */
        if (pressed(w, BTN_B)) u->charge = 1;
        else if (held(w, BTN_B) && u->charge > 0) {
            if (u->charge < 30000) u->charge++;
            if (u->charge == FRL_CHARGE) w->ev |= FEV_READY;
        }
        bool go = released(w, BTN_B) && u->charge >= FRL_CHARGE;
        if (released(w, BTN_B) && !go) u->charge = 0;
        if (u->mode == FUM_PIPE) {
            u->mode_t++;
            if (go) { frl_give_up(w); return; }
            u->y = imin(u->y + 3 * FRL_ONE, u->ty);
            if (u->y >= u->ty) { u->mode = FUM_WALK; u->vy = 0; }
            return;
        }
        if (++u->mode_t >= FRL_WARP_T || go) {
            u->x = u->tx;
            u->y = u->ty;
            u->mode = FUM_WALK;
            frl_add_fx(w, frl_unit_cx(w), frl_unit_cy(w), FFX_WARP);
            if (go) { u->below = 1; frl_give_up(w); }
        }
        return;
    }
    int hx = (held(w, BTN_RIGHT) ? 1 : 0) - (held(w, BTN_LEFT) ? 1 : 0);
    if (hx) u->face = (int8_t)hx;
    run_accel(u, hx);
    if (u->ground) { u->coyote = FRL_COYOTE; u->air_jump = u->cls == FRC_RUNNER; }
    if (pressed(w, BTN_A)) {
        if (u->ground || u->coyote > 0) {
            u->vy = -FRL_JUMP;
            u->coyote = 0;
            u->jump_held = 1;
            w->ev |= FEV_JUMP;
        } else if (u->air_jump) {
            /* the runner's second jump: let go of A in the air, then press again */
            u->vy = -FRL_JUMP;
            u->air_jump = 0;
            u->jump_held = 1;
            w->ev |= FEV_DOUBLE;
        }
    }
    if (!u->ground && u->coyote > 0) u->coyote--;
    if (!held(w, BTN_A)) u->jump_held = 0;
    if (!u->jump_held && u->vy < -FRL_ONE) u->vy = u->vy * 154 / 256;
    u->vy = imin(u->vy + FRL_GRAV, FRL_FALL_MAX);
    move_unit(w);
    if (u->ground && u->vy > 0) u->vy = 0;

    /* B: a tap attacks; held, the volunteer flashes, and letting go then gives
     * them up for their trade's gift */
    if (pressed(w, BTN_B)) {
        if (!u->atk_cd) attack(w);
        u->charge = 1;
    } else if (held(w, BTN_B) && u->charge > 0) {
        if (u->charge < 30000) u->charge++;
        if (u->charge == FRL_CHARGE) w->ev |= FEV_READY;
    }
    if (released(w, BTN_B)) {
        if (u->charge >= FRL_CHARGE) { frl_give_up(w); return; }
        u->charge = 0;
    }
    sword_hits(w);

    /* down a chute; up through the waystone (either way) */
    if (pressed(w, BTN_DOWN) && u->ground) {
        int px, py;
        unit_box(w, &px, &py);
        int p = frl_pipe_at(w, px, py, FRL_UW, FRL_UH);
        if (p >= 0) { enter_pipe(w, p); return; }
    }
    if (pressed(w, BTN_UP)) {
        if (w->way_on && frl_unit_on_pad(w)) { warp_to(w, w->way_x, w->way_y + FRL_UH / 2); return; }
        if (frl_unit_at_way(w)) { warp_to(w, w->pad_x, w->pad_y); return; }
    }
    unit_pickups(w);
    unit_hazards(w);
}

/* ------------------------------------------------------------------ */
/* plates, shots                                                        */

static void plates_step(FrlWorld *w) {
    for (int i = 0; i < FRL_PLATES; i++) {
        FrlPlate *p = &w->plate[i];
        if (!p->tx && !p->ty) continue;
        bool down = w->tile[p->ty][p->tx] == FT_STONE;
        if (!down && w->phase == FWP_PLAY && w->u.mode == FUM_WALK) {
            int px = w->u.x >> 8, py = w->u.y >> 8;
            down = rects_overlap(px, py, FRL_UW, FRL_UH, p->tx * FRL_T, p->ty * FRL_T + 6, FRL_T, 4);
        }
        if (down && !p->down) {
            w->switches++;
            p->ever = 1;
            w->ev |= FEV_PLATE;
            p->down = 1;
            /* a block that rises round the volunteer lifts them on top */
            for (int k = 0; k < 20 && w->phase == FWP_PLAY && frl_box_solid(w, w->u.x >> 8, w->u.y >> 8, FRL_UW, FRL_UH); k++)
                w->u.y -= FRL_ONE;
        } else if (!down && p->down) {
            p->down = 0;
            w->ev |= FEV_PLATE_UP;
        }
    }
}

static int foe_at_point(FrlWorld *w, int x, int y, int r) {
    for (int i = 0; i < w->nfoe; i++) {
        const FrlFoe *f = &w->foe[i];
        if (!f->on) continue;
        if (rects_overlap(x - r, y - r, 2 * r + 1, 2 * r + 1, f->x >> 8, f->y >> 8, f->w, f->h)) return i;
    }
    return -1;
}

static void shots_step(FrlWorld *w) {
    int ux = w->u.x >> 8, uy = w->u.y >> 8;
    bool unit_hit = w->phase == FWP_PLAY && w->u.mode == FUM_WALK;
    for (int i = 0; i < FRL_SHOTS; i++) {
        FrlShot *s = &w->shot[i];
        if (!s->on) continue;
        s->t++;
        switch (s->kind) {
        case FS_WRENCH: s->vy += 26; break;
        case FS_AXE: s->vy += 30; break;
        case FS_CRESCENT: s->vy += 18; break;
        case FS_BUBBLE: s->vy = ((s->t / 16) & 1) ? 40 : -40; break;
        default: break;
        }
        s->x += s->vx;
        s->y += s->vy;
        int x = s->x >> 8, y = s->y >> 8;
        if (--s->life <= 0 || x < 0 || y < 0 || x >= FRL_MW * FRL_T || y >= FRL_MH * FRL_T) { s->on = 0; continue; }
        if (s->mine) {
            int f = foe_at_point(w, x, y, s->kind == FS_WRENCH ? 3 : 2);
            if (f >= 0) {
                frl_hurt_foe(w, f, s->kind == FS_WRENCH ? FRL_DAMAGE[FRC_TINKER] : 1);
                s->on = 0;
                continue;
            }
        } else if (unit_hit && rects_overlap(x - 2, y - 2, 4, 4, ux, uy, FRL_UW, FRL_UH)) {
            s->on = 0;
            frl_unit_die(w, FCAUSE_SHOT);
            unit_hit = false;
            continue;
        }
        /* a foe's shot starts inside it (a wall-eye's inside its rock) */
        if (frl_solid(w, x / FRL_T, y / FRL_T) && (s->mine || s->t > 5)) {
            if (s->mine && frl_tile(w, x / FRL_T, y / FRL_T) == FT_COMB)
                frl_hurt_comb(w, x / FRL_T, y / FRL_T, s->kind == FS_WRENCH ? FRL_DAMAGE[FRC_TINKER] : 1);
            s->on = 0;
            frl_add_fx(w, x, y, FFX_DUST);
        }
    }
}

static void camera_step(FrlWorld *w) {
    int fx, fy;
    if (w->phase == FWP_SELECT || w->phase == FWP_DARK) { fx = w->base_x; fy = w->base_y - 20; }
    else {
        fx = frl_unit_cx(w);
        fy = frl_unit_cy(w);
        if (w->u.mode == FUM_WARP && w->u.mode_t > FRL_WARP_T / 2) { fx = (w->u.tx >> 8) + 3; fy = (w->u.ty >> 8) + 4; }
    }
    int tx = iclamp(fx - SCREEN_W / 2, 0, FRL_MW * FRL_T - SCREEN_W);
    int ty = iclamp(fy - FRL_VH / 2, 0, FRL_MH * FRL_T - FRL_VH);
    /* quick, but not a jump cut, unless it's far */
    int dx = tx - w->cam_x, dy = ty - w->cam_y;
    if (iabs(dx) > 120 || iabs(dy) > 90) { w->cam_x = (int16_t)tx; w->cam_y = (int16_t)ty; }
    else {
        w->cam_x = (int16_t)(w->cam_x + (dx + (dx > 0 ? 3 : dx < 0 ? -3 : 0)) / 4);
        w->cam_y = (int16_t)(w->cam_y + (dy + (dy > 0 ? 3 : dy < 0 ? -3 : 0)) / 4);
    }
}

/* ------------------------------------------------------------------ */
/* the frame                                                            */

static void select_step(FrlWorld *w) {
    if (w->phase_t < 12) return;
    if (pressed(w, BTN_LEFT) || pressed(w, BTN_RIGHT)) {
        int d = pressed(w, BTN_RIGHT) ? 1 : FRC_COUNT - 1;
        for (int k = 0; k < FRC_COUNT; k++) {
            w->sel = (w->sel + d) % FRC_COUNT;
            if (w->allowed & (1 << w->sel)) break;
        }
        w->ev |= FEV_SPAWN;
    }
    if (pressed(w, BTN_A) && (w->allowed & (1 << w->sel))) frl_choose(w, w->sel);
}

void frl_world_step(FrlWorld *w, uint32_t pad1, uint32_t pad2) {
    w->ev = 0;
    w->nfx = 0;
    w->t++;
    w->phase_t++;
    if (w->shake) w->shake--;
    int player = w->phase == FWP_SELECT ? w->units % w->players : w->u.player;
    uint32_t pad = player ? pad2 : pad1;
    w->ctl = pad;
    switch (w->phase) {
    case FWP_SELECT:
        select_step(w);
        if (w->phase == FWP_PLAY) w->ctl = pad; /* the A that chose isn't a jump */
        break;
    case FWP_PLAY:
        unit_step(w);
        break;
    case FWP_DEAD:
        if (w->phase_t >= FRL_DEAD_T) {
            if (w->hearts_left <= 0) { w->phase = FWP_WON; w->phase_t = 0; }
            else if (w->lives <= 0) { w->phase = FWP_OVER; w->phase_t = 0; }
            else {
                w->lives--;
                w->phase = FWP_DARK;
                w->phase_t = 0;
                frl_clear_minions(w);
                camera_step(w);
                return;
            }
        }
        break;
    case FWP_DARK:
        /* black, but for the hearts still beating; then the door */
        if (w->phase_t >= FRL_DARK_T) {
            w->phase = FWP_SELECT;
            w->phase_t = 0;
            w->ctl_prev = 0xFFFFFFFFu; /* no press carries over to the door */
            camera_step(w);
            return;
        }
        break;
    default: break;
    }
    if (w->phase == FWP_PLAY || w->phase == FWP_DEAD || w->phase == FWP_WON) {
        plates_step(w);
        frl_spawners_step(w);
        frl_foes_step(w);
        shots_step(w);
        if (w->phase == FWP_PLAY && w->hearts_left <= 0) { w->phase = FWP_WON; w->phase_t = 0; w->ev |= FEV_WIN; }
    }
    camera_step(w);
    w->ctl_prev = w->ctl;
}
