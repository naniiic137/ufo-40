/* TILTSHOT - the course and the ball: tiles turned into surfaces, bumpers,
 * junk and movers, and one golfer's play on a hole (aim, the meter, the
 * swing, the slam, bounces, water, pits and the cup). Everything here is
 * plain logic with no drawing, so the demo player and the solver can run
 * it on copies. */
#include "tiltshot.h"

#define GRAV 0.1f
#define VMIN 0.7f
#define VMAX 5.8f
#define SLAM_V 5.0f
#define VCAP 11.0f
#define REST_K 0.55f      /* how much of a hard landing comes back up */
#define BUCKET_W 16

/* ---- tiles ------------------------------------------------------------------ */

/* a full solid tile in the top row carries on up out of sight: a roof or a
 * wall that reaches the top of the screen can't be flown over */
static bool sky_high(char ch) { return ch && strchr("#XIsTR", ch) != NULL; }

char tsh_tile(const TshCourse *c, int col, int row) {
    if (row >= TSH_ROWS) return '.';
    if (col < 0 || col >= c->cols) return row < 0 ? '.' : 'X'; /* the course is walled at both ends */
    if (row < 0) return sky_high(c->tile[0][col]) ? 'X' : '.';
    return c->tile[row][col];
}

bool tsh_tile_solid(char ch) { return ch && strchr("#XIsTR/\\1234qp", ch) != NULL; }

/* each solid tile as a polygon, corners in clockwise order on screen */
typedef struct Poly {
    int n;
    int8_t v[4][2];
} Poly;

static bool tile_poly(char ch, Poly *p) {
    static const Poly SQUARE = {4, {{0, 0}, {8, 0}, {8, 8}, {0, 8}}};
    switch (ch) {
    case '#': case 'X': case 'I': case 's': case 'T': case 'R': *p = SQUARE; return true;
    case '/': *p = (Poly){3, {{8, 0}, {8, 8}, {0, 8}}}; return true;
    case '\\': *p = (Poly){3, {{0, 0}, {8, 8}, {0, 8}}}; return true;
    case '1': *p = (Poly){3, {{0, 8}, {8, 4}, {8, 8}}}; return true;
    case '2': *p = (Poly){4, {{0, 4}, {8, 0}, {8, 8}, {0, 8}}}; return true;
    case '3': *p = (Poly){4, {{0, 0}, {8, 4}, {8, 8}, {0, 8}}}; return true;
    case '4': *p = (Poly){3, {{0, 4}, {8, 8}, {0, 8}}}; return true;
    case 'q': *p = (Poly){3, {{0, 0}, {8, 0}, {0, 8}}}; return true;
    case 'p': *p = (Poly){3, {{0, 0}, {8, 0}, {8, 8}}}; return true;
    default: return false;
    }
}

/* is pixel (px, py) of a tile solid? (pixel centres against the polygon) */
bool tsh_tile_px(char ch, int px, int py) {
    Poly p;
    if (!tile_poly(ch, &p)) return false;
    float x = px + 0.5f, y = py + 0.5f;
    for (int k = 0; k < p.n; k++) {
        float ax = p.v[k][0], ay = p.v[k][1];
        float bx = p.v[(k + 1) % p.n][0], by = p.v[(k + 1) % p.n][1];
        /* clockwise on screen: the inside is to the right of each edge */
        if ((bx - ax) * (y - ay) - (by - ay) * (x - ax) < 0) return false;
    }
    return true;
}

enum { SIDE_L, SIDE_R, SIDE_T, SIDE_B, SIDE_NONE };

static int edge_side(int ax, int ay, int bx, int by) {
    if (ax == 0 && bx == 0) return SIDE_L;
    if (ax == 8 && bx == 8) return SIDE_R;
    if (ay == 0 && by == 0) return SIDE_T;
    if (ay == 8 && by == 8) return SIDE_B;
    return SIDE_NONE;
}

/* how much of one side of a tile its solid part covers: [lo, hi] */
static bool side_cover(const TshCourse *c, int col, int row, int side, int *lo, int *hi) {
    if ((col < 0 || col >= c->cols) && row >= 0 && row < TSH_ROWS) { *lo = 0; *hi = 8; return true; }
    Poly p;
    if (!tile_poly(tsh_tile(c, col, row), &p)) return false;
    for (int k = 0; k < p.n; k++) {
        int ax = p.v[k][0], ay = p.v[k][1], bx = p.v[(k + 1) % p.n][0], by = p.v[(k + 1) % p.n][1];
        if (edge_side(ax, ay, bx, by) != side) continue;
        bool vert = side == SIDE_L || side == SIDE_R;
        int a = vert ? ay : ax, b = vert ? by : bx;
        *lo = imin(a, b);
        *hi = imax(a, b);
        return true;
    }
    return false;
}

static uint8_t tile_mat(char ch, int theme) {
    if (ch == 's') return TM_SAND;
    if (ch == 'T') return TM_SPRING;
    if (ch == 'I') return TM_ICE;
    if (ch == 'X' || ch == 'R') return TM_TURF;
    return theme == TH_ICE ? TM_ICE : TM_TURF;
}

static void add_seg(TshCourse *c, float x0, float y0, float x1, float y1, uint8_t mat, bool two_sided) {
    if (c->nseg >= TSH_MAXSEG) return;
    TshSeg *s = &c->seg[c->nseg++];
    s->x0 = x0; s->y0 = y0; s->x1 = x1; s->y1 = y1;
    float dx = x1 - x0, dy = y1 - y0, l = sqrtf(dx * dx + dy * dy);
    s->nx = dy / l;
    s->ny = -dx / l;
    s->mat = mat;
    s->two_sided = two_sided;
}

static void seg_bounds(TshSeg *s) {
    s->minx = fminf(s->x0, s->x1);
    s->maxx = fmaxf(s->x0, s->x1);
    s->miny = fminf(s->y0, s->y1);
    s->maxy = fmaxf(s->y0, s->y1);
}

/* join surfaces that carry straight on (a floor of many tiles is one line) */
static bool same_point(float ax, float ay, float bx, float by) { return fabsf(ax - bx) < 1e-3f && fabsf(ay - by) < 1e-3f; }

static void merge_segs(TshCourse *c) {
    for (int i = 0; i < c->nseg; i++) {
        bool grew = true;
        while (grew && !c->seg[i].two_sided) {
            grew = false;
            TshSeg *a = &c->seg[i];
            for (int j = 0; j < c->nseg; j++) {
                TshSeg *b = &c->seg[j];
                if (j == i || b->two_sided || b->mat != a->mat) continue;
                if (fabsf(a->nx - b->nx) > 1e-4f || fabsf(a->ny - b->ny) > 1e-4f) continue;
                if (same_point(a->x1, a->y1, b->x0, b->y0)) { a->x1 = b->x1; a->y1 = b->y1; }
                else if (same_point(b->x1, b->y1, a->x0, a->y0)) { a->x0 = b->x0; a->y0 = b->y0; }
                else continue;
                c->seg[j] = c->seg[--c->nseg];
                if (i == c->nseg) i = j; /* the grown one was moved into j's place */
                grew = true;
                break;
            }
        }
    }
}

static void build_buckets(TshCourse *c) {
    c->nbucket = imin(c->w / BUCKET_W + 2, TSH_MAXBUCKET);
    static uint16_t count[TSH_MAXBUCKET + 1];
    memset(count, 0, sizeof count);
    for (int pass = 0; pass < 2; pass++) {
        if (pass == 1) {
            int acc = 0;
            for (int b = 0; b < c->nbucket; b++) {
                c->bstart[b] = (uint16_t)acc;
                acc += count[b];
                count[b] = 0;
            }
            c->bstart[c->nbucket] = (uint16_t)acc;
        }
        for (int i = 0; i < c->nseg; i++) {
            const TshSeg *s = &c->seg[i];
            int b0 = iclamp((int)floorf((s->minx - TSH_R - 2) / BUCKET_W), 0, c->nbucket - 1);
            int b1 = iclamp((int)floorf((s->maxx + TSH_R + 2) / BUCKET_W), 0, c->nbucket - 1);
            for (int b = b0; b <= b1; b++) {
                if (pass == 0) count[b]++;
                else {
                    int at = c->bstart[b] + count[b]++;
                    if (at < TSH_MAXBLIST) c->blist[at] = (uint16_t)i;
                }
            }
        }
    }
}

void tsh_course_build(TshCourse *c, int hole) {
    memset(c, 0, sizeof *c);
    const TshHoleDef *d = &TSH_HOLE[hole];
    const char *const *rows = TSH_MAP[hole];
    c->hole = hole;
    c->theme = d->theme;
    c->par = d->par;
    c->cols = imin((int)strlen(rows[0]), TSH_MAXCOLS);
    c->w = c->cols * TSH_T;
    c->red_col = c->red_row = -1;
    for (int r = 0; r < TSH_ROWS; r++)
        for (int col = 0; col < c->cols; col++) {
            char ch = rows[r][col];
            float cx = col * TSH_T + 4.0f, cy = r * TSH_T + 4.0f;
            switch (ch) {
            case 'S': c->tee_x = cx; c->tee_y = (r + 1) * TSH_T - TSH_R; ch = '.'; break;
            case 'U': c->cup_col = col; c->cup_row = r; c->cup_x = cx; c->cup_y = (float)(r * TSH_T); break;
            case 'R': c->red_col = col; c->red_row = r; break;
            case 'O': case 'o': case '!':
                if (c->ncirc < TSH_MAXCIRC)
                    c->circ[c->ncirc++] = (TshCirc){cx, cy, ch == 'O' ? 7.0f : ch == 'o' ? 4.0f : 1.5f,
                                                    (uint8_t)(ch == 'O' ? TC_BIG : ch == 'o' ? TC_SMALL : TC_PEG)};
                ch = '.';
                break;
            case 'C': case 'K': case 'N': case 'Y':
                if (c->njunk < TSH_MAXTRASH)
                    c->junk[c->njunk++] = (TshJunk){cx, (r + 1) * TSH_T - 4.0f,
                                                    (uint8_t)(ch == 'C' ? TJ_CRATE : ch == 'K' ? TJ_CONE : ch == 'N' ? TJ_CHURN : TJ_BUCKET)};
                ch = '.';
                break;
            case ' ': ch = '.'; break;
            default: break;
            }
            c->tile[r][col] = ch;
        }
    /* surfaces: every edge of a solid tile that faces open space */
    for (int r = 0; r < TSH_ROWS; r++)
        for (int col = 0; col < c->cols; col++) {
            Poly p;
            char ch = c->tile[r][col];
            if (!tile_poly(ch, &p)) continue;
            uint8_t mat = tile_mat(ch, c->theme);
            float ox = (float)(col * TSH_T), oy = (float)(r * TSH_T);
            for (int k = 0; k < p.n; k++) {
                int ax = p.v[k][0], ay = p.v[k][1], bx = p.v[(k + 1) % p.n][0], by = p.v[(k + 1) % p.n][1];
                int side = edge_side(ax, ay, bx, by);
                if (side == SIDE_NONE) {
                    add_seg(c, ox + ax, oy + ay, ox + bx, oy + by, mat, false);
                    continue;
                }
                static const int NDX[4] = {-1, 1, 0, 0}, NDY[4] = {0, 0, -1, 1}, OPP[4] = {SIDE_R, SIDE_L, SIDE_B, SIDE_T};
                int lo, hi, nlo = 0, nhi = 0;
                bool vert = side == SIDE_L || side == SIDE_R;
                int a = vert ? ay : ax, b = vert ? by : bx;
                lo = imin(a, b);
                hi = imax(a, b);
                bool cov = side_cover(c, col + NDX[side], r + NDY[side], OPP[side], &nlo, &nhi);
                /* the parts of [lo, hi] the neighbour leaves open */
                int piece[2][2], np = 0;
                if (!cov || nhi <= lo || nlo >= hi) {
                    piece[np][0] = lo; piece[np][1] = hi; np++;
                } else {
                    if (nlo > lo) { piece[np][0] = lo; piece[np][1] = nlo; np++; }
                    if (nhi < hi) { piece[np][0] = nhi; piece[np][1] = hi; np++; }
                }
                for (int q = 0; q < np; q++) {
                    int p0 = a < b ? piece[q][0] : piece[q][1], p1 = a < b ? piece[q][1] : piece[q][0];
                    if (vert) add_seg(c, ox + ax, oy + p0, ox + bx, oy + p1, mat, false);
                    else add_seg(c, ox + p0, oy + ay, ox + p1, oy + by, mat, false);
                }
            }
        }
    merge_segs(c);
    /* the walls at both ends reach well above the sky, and so does anything
     * standing in the top row */
    add_seg(c, 0, -3000.0f, 0, TSH_VIEW_H + 40.0f, TM_TURF, false);
    add_seg(c, (float)c->w, TSH_VIEW_H + 40.0f, (float)c->w, -3000.0f, TM_TURF, false);
    for (int col = 0; col < c->cols; col++) {
        if (!sky_high(c->tile[0][col])) continue;
        float x0 = (float)(col * TSH_T), x1 = x0 + TSH_T;
        if (col > 0 && !sky_high(c->tile[0][col - 1])) add_seg(c, x0, 0, x0, -3000.0f, TM_TURF, false);
        if (col + 1 < c->cols && !sky_high(c->tile[0][col + 1])) add_seg(c, x1, -3000.0f, x1, 0, TM_TURF, false);
    }
    for (int i = 0; i < d->nsprings; i++) {
        const TshSpringDef *s = &d->springs[i];
        add_seg(c, s->x0, s->y0, s->x1, s->y1, TM_SPRING, true);
    }
    for (int i = 0; i < c->nseg; i++) seg_bounds(&c->seg[i]);
    build_buckets(c);
    c->nmover = imin(d->nmovers, TSH_MAXMOVER);
    for (int i = 0; i < c->nmover; i++) c->mover[i] = d->movers[i];
}

bool tsh_mover_pos(const TshMoverDef *m, int t, float *x, float *y) {
    int per = m->period ? m->period : 1;
    float u = (float)((t + m->phase) % per) / per;
    if (m->kind == MV_FISH) {
        /* half the round in the air, in an arc; the other half under water */
        if (u >= 0.5f) { *x = m->cx; *y = m->cy; return false; }
        float s = u * 2;
        *x = m->cx - m->ax + 2 * m->ax * s;
        *y = m->cy - m->ay * 4 * s * (1 - s);
        return true;
    }
    if (m->kind == MV_ROLLER) {
        /* along its floor and back at a steady speed */
        float s = u < 0.5f ? u * 2 : 2 - u * 2;
        *x = m->cx - m->ax + 2 * m->ax * s;
        *y = m->cy;
        return true;
    }
    if (m->kind == MV_SWING) {
        /* a pendulum: out to one side, back through the bottom, out to the other */
        float len = m->ay > 0 ? m->ay : 1;
        float th = asinf(fminf(1.0f, fabsf((float)m->ax) / len)) * sinf(u * 6.2831853f);
        *x = m->cx + len * sinf(th);
        *y = m->cy + len * cosf(th);
        return true;
    }
    float a = u * 6.2831853f;
    *x = m->cx + m->ax * cosf(a);
    *y = m->cy + m->ay * sinf(a);
    return true;
}

/* ---- the shot ------------------------------------------------------------------ */

float tsh_aim_dx(int aim) { return cosf(aim * 5.0f * 3.14159265f / 180.0f); }
float tsh_aim_dy(int aim) { return -sinf(aim * 5.0f * 3.14159265f / 180.0f); }
float tsh_shot_speed(int meter) { return VMIN + (VMAX - VMIN) * iclamp(meter, 0, TSH_FILL) / (float)TSH_FILL; }

int tsh_distance(const TshPlay *p, const TshCourse *c) { return (int)(fabsf(c->cup_x - p->x) / 4 + 0.5f); }

void tsh_play_begin(TshPlay *p, const TshCourse *c) {
    memset(p, 0, sizeof *p);
    p->x = p->sx = c->tee_x;
    p->y = p->sy = c->tee_y;
    p->aim = TSH_AIM_START;
    p->phase = TP_AIM;
    p->ground_mat = TM_TURF;
    p->fx_circ = -1;
}

static void set_phase(TshPlay *p, int ph) {
    p->phase = ph;
    p->phase_t = 0;
}

static void swing(TshPlay *p) {
    float v = tsh_shot_speed(p->meter);
    p->vx = tsh_aim_dx(p->aim) * v;
    p->vy = tsh_aim_dy(p->aim) * v;
    p->sx = p->x;
    p->sy = p->y;
    p->strokes++;
    p->fly_t = p->air_t = p->rest_t = p->fire_t = 0;
    p->slam_used = p->slammed = false;
    p->mover_gone = 0;
    p->skips = 0;
    p->fx |= FXT_SWING;
    set_phase(p, TP_FLIGHT);
}

static void lose(TshPlay *p, int why) {
    p->lost_why = (uint8_t)why;
    p->vx = p->vy = 0;
    p->fx |= why == TL_WATER ? FXT_SPLASH : FXT_PIT;
    p->fx_x = p->x;
    p->fx_y = p->y;
    set_phase(p, TP_LOST);
}

typedef struct Contact {
    float pen, nx, ny;
    int mat, circ;
    bool corner; /* touching the end of a surface, not its face */
} Contact;

static void seg_contact(const TshSeg *s, float cx, float cy, Contact *best) {
    float dx = s->x1 - s->x0, dy = s->y1 - s->y0;
    float l2 = dx * dx + dy * dy;
    float t = ((cx - s->x0) * dx + (cy - s->y0) * dy) / l2;
    float nx = s->nx, ny = s->ny, pen;
    bool corner = !(t > 0 && t < 1);
    if (!corner) {
        float ds = (cx - s->x0) * nx + (cy - s->y0) * ny;
        if (s->two_sided && ds < 0) { ds = -ds; nx = -nx; ny = -ny; }
        if (ds >= TSH_R || ds < -TSH_R) return;
        pen = TSH_R - ds;
    } else {
        float px = t <= 0 ? s->x0 : s->x1, py = t <= 0 ? s->y0 : s->y1;
        float ex = cx - px, ey = cy - py, d2 = ex * ex + ey * ey;
        if (d2 >= TSH_R * TSH_R) return;
        if (!s->two_sided && ex * nx + ey * ny < 0) return; /* behind: the next surface has it */
        float d = sqrtf(d2);
        if (d > 1e-4f) { nx = ex / d; ny = ey / d; }
        else if (s->two_sided) return;
        pen = TSH_R - d;
    }
    if (pen > best->pen) {
        best->pen = pen;
        best->nx = nx;
        best->ny = ny;
        best->mat = s->mat;
        best->circ = -1;
        best->corner = corner;
    }
}

static void find_contact(const TshPlay *p, const TshCourse *c, Contact *best) {
    *best = (Contact){0, 0, -1, TM_TURF, -1, false};
    int b0 = iclamp((int)floorf((p->x - TSH_R) / BUCKET_W), 0, c->nbucket - 1);
    int b1 = iclamp((int)floorf((p->x + TSH_R) / BUCKET_W), 0, c->nbucket - 1);
    for (int b = b0; b <= b1; b++)
        for (int k = c->bstart[b]; k < c->bstart[b + 1]; k++) {
            const TshSeg *s = &c->seg[c->blist[k]];
            if (p->x + TSH_R < s->minx || p->x - TSH_R > s->maxx || p->y + TSH_R < s->miny || p->y - TSH_R > s->maxy)
                continue;
            seg_contact(s, p->x, p->y, best);
        }
    for (int i = 0; i < c->ncirc; i++) {
        const TshCirc *q = &c->circ[i];
        float ex = p->x - q->x, ey = p->y - q->y, rr = q->r + TSH_R;
        if (fabsf(ex) >= rr || fabsf(ey) >= rr) continue;
        float d2 = ex * ex + ey * ey;
        if (d2 >= rr * rr) continue;
        float d = sqrtf(d2);
        float pen = rr - d;
        if (pen > best->pen) {
            best->pen = pen;
            best->nx = d > 1e-4f ? ex / d : 0;
            best->ny = d > 1e-4f ? ey / d : -1;
            best->mat = q->kind == TC_PEG ? TM_PEG : TM_BUMPER;
            best->circ = i;
            best->corner = false;
        }
    }
}

static void respond(TshPlay *p, const Contact *k, bool kicks, bool *ground) {
    p->x += k->nx * k->pen;
    p->y += k->ny * k->pen;
    float nx = k->nx, ny = k->ny;
    if (ny < -0.35f && k->mat != TM_BUMPER && k->mat != TM_PEG) {
        *ground = true;
        p->ground_mat = (uint8_t)k->mat;
    }
    float vn = p->vx * nx + p->vy * ny;
    if (vn >= 0) return;
    float tx = p->vx - vn * nx, ty = p->vy - vn * ny, out = 0;
    int mat = k->mat;
    if (!kicks && (mat == TM_SPRING || mat == TM_BUMPER)) mat = TM_TURF;
    if (-vn > 1.0f) {
        p->fx |= FXT_BOUNCE;
        p->fx_power = -vn;
        p->fx_x = p->x - nx * TSH_R;
        p->fx_y = p->y - ny * TSH_R;
    }
    switch (mat) {
    case TM_SAND:
        p->vx = p->vy = 0;
        if (!(p->fx & FXT_SAND) && -vn > 0.3f) p->fx |= FXT_SAND;
        p->slammed = false;
        return;
    case TM_SPRING:
        /* far wilder than a bumper: it gives back more than it gets */
        if (-vn > 0.8f) {
            out = fminf(-vn * 1.3f + 2.4f, 10.0f);
            p->fx |= FXT_SPRING;
        } else out = 0;
        break;
    case TM_BUMPER:
        out = fmaxf(-vn * 0.75f + 2.2f, 2.8f);
        p->fx |= FXT_BUMPER;
        p->fx_circ = k->circ;
        break;
    default:
        if (-vn > 0.8f) {
            out = -vn * REST_K;
            tx *= 0.9f;
            ty *= 0.9f;
        } else out = 0;
        break;
    }
    /* a slam onto a slope that falls away: the ball takes off down it, on
     * fire (the face of a slope; the corner of a ledge is not a slope) */
    if (p->slammed) {
        p->slammed = false;
        if ((mat == TM_TURF || mat == TM_ICE) && !k->corner && fabsf(nx) >= 0.4f && ny < 0) {
            float dx = -ny, dy = nx; /* along the surface */
            if (dy < 0) { dx = -dx; dy = -dy; }
            float along = tx * dx + ty * dy;
            if (along > -0.5f) {
                float sp = fmaxf(along, 0) * 1.45f + 2.2f;
                tx = dx * sp;
                ty = dy * sp;
                out = 0;
                p->fire_t = 75;
                p->fx |= FXT_FIRE;
            }
        }
    }
    p->vx = tx + nx * out;
    p->vy = ty + ny * out;
}

static void ball_step(TshPlay *p, const TshCourse *c) {
    if (p->fire_t > 0) p->fire_t--;
    p->vy += GRAV;
    float sp = sqrtf(p->vx * p->vx + p->vy * p->vy);
    if (sp > VCAP) {
        p->vx *= VCAP / sp;
        p->vy *= VCAP / sp;
        sp = VCAP;
    }
    int n = (int)(sp / 1.25f) + 1;
    bool ground = false, kicks = p->fly_t < 900;
    for (int s = 0; s < n; s++) {
        p->x += p->vx / n;
        p->y += p->vy / n;
        int col = (int)floorf(p->x / TSH_T), row = (int)floorf(p->y / TSH_T);
        char ch = tsh_tile(c, col, row);
        if (ch == 'U' && p->y - row * TSH_T > TSH_R + 0.5f) { /* wholly below the rim */
            p->vx *= 0.3f;
            p->vy = 0;
            p->holed_in = true;
            p->fx |= FXT_CUP;
            set_phase(p, TP_HOLED);
            return;
        }
        if (ch == '~') {
            int r0 = row;
            while (tsh_tile(c, col, r0 - 1) == '~') r0--;
            float ax = fabsf(p->vx);
            if (ax >= 3.0f && p->vy > 0 && p->vy <= ax * 0.5f && p->skips < 6) {
                /* fast and flat: it skips off the water */
                p->y = r0 * TSH_T - 0.5f;
                p->vy = -(p->vy * 0.7f) - 0.3f;
                p->vx *= 0.85f;
                p->skips++;
                p->fx |= FXT_SKIP;
                p->fx_x = p->x;
                p->fx_y = p->y;
            } else {
                p->y = (float)(r0 * TSH_T);
                lose(p, TL_WATER);
                return;
            }
        }
        for (int it = 0; it < 4; it++) {
            Contact k;
            find_contact(p, c, &k);
            if (k.pen <= 0.0005f) break;
            respond(p, &k, kicks, &ground);
            if (k.mat == TM_TURF || k.mat == TM_ICE) {
                if (c->red_row >= 0 && !p->secret) {
                    float rx = c->red_col * TSH_T + 4.0f, ry = c->red_row * TSH_T + 4.0f;
                    if (fabsf(p->x - rx) < 4 + TSH_R + 0.5f && fabsf(p->y - ry) < 4 + TSH_R + 0.5f) {
                        p->secret = true;
                        p->fx |= FXT_SECRET;
                    }
                }
            }
        }
        for (int i = 0; i < c->njunk; i++) {
            if (p->junk_gone & (1u << i)) continue;
            float ex = p->x - c->junk[i].x, ey = p->y - c->junk[i].y;
            if (ex * ex + ey * ey < (TSH_R + 4) * (TSH_R + 4)) {
                p->junk_gone |= 1u << i;
                p->vx *= 0.6f;
                p->vy *= 0.6f;
                p->fx |= FXT_JUNK;
                p->fx_x = c->junk[i].x;
                p->fx_y = c->junk[i].y;
            }
        }
        for (int i = 0; i < c->nmover; i++) {
            if (p->mover_gone & (1u << i)) continue;
            float mx, my;
            if (!tsh_mover_pos(&c->mover[i], p->clock, &mx, &my)) continue;
            float ex = p->x - mx, ey = p->y - my;
            if (ex * ex + ey * ey < (TSH_R + 5) * (TSH_R + 5)) {
                p->mover_gone |= 1u << i;
                p->vx *= 0.2f;
                p->vy *= 0.2f;
                p->fx |= FXT_MOVER;
                p->fx_x = mx;
                p->fx_y = my;
            }
        }
        if (p->y > TSH_VIEW_H + 12) {
            lose(p, TL_PIT);
            return;
        }
    }
    if (ground) {
        p->air_t = 0;
        sp = sqrtf(p->vx * p->vx + p->vy * p->vy);
        if (sp > 0) {
            float ns;
            if (p->ground_mat == TM_ICE) ns = sp * 0.9995f - 0.002f;
            else if (p->ground_mat == TM_SAND) ns = 0;
            else ns = sp * 0.995f - 0.022f;
            if (ns < 0) ns = 0;
            p->vx *= ns / sp;
            p->vy *= ns / sp;
            sp = ns;
        }
        p->rest_t = sp < 0.07f ? p->rest_t + 1 : 0;
    } else {
        p->air_t++;
        p->rest_t = 0;
    }
    if (p->rest_t >= 6 || (p->fly_t > 2400 && ground)) {
        p->vx = p->vy = 0;
        p->sx = p->x;
        p->sy = p->y;
        p->fx |= FXT_REST;
        set_phase(p, TP_AIM);
        return;
    }
    if (p->fly_t > 3000) lose(p, TL_PIT); /* never settles: back to where it was hit from */
}

void tsh_play_step(TshPlay *p, const TshCourse *c, uint8_t b) {
    uint8_t pressed = (uint8_t)(b & ~p->prev);
    p->clock++;
    p->phase_t++;
    switch (p->phase) {
    case TP_AIM:
        if (b & (TB_LEFT | TB_RIGHT)) {
            p->rep_t++;
            bool step = (pressed & (TB_LEFT | TB_RIGHT)) || (p->rep_t > 14 && (p->rep_t - 14) % 3 == 0);
            if (step) {
                int na = iclamp(p->aim + ((b & TB_LEFT) ? 1 : -1), 0, TSH_AIMS - 1);
                if (na != p->aim) p->fx |= FXT_AIM;
                p->aim = na;
            }
        } else p->rep_t = 0;
        if (pressed & TB_A) {
            p->meter = 0;
            p->max_t = 0;
            set_phase(p, TP_CHARGE);
        }
        break;
    case TP_CHARGE:
        if (!(b & TB_A)) {
            swing(p);
            break;
        }
        if (p->meter < TSH_FILL) {
            if (++p->meter == TSH_FILL) p->fx |= FXT_FULL;
        } else {
            p->max_t++;
            if (p->max_t == TSH_GRACE) p->fx |= FXT_WARN;
            if (p->max_t >= TSH_GRACE + TSH_WARN) {
                /* held too long: the golfer blows up and the stroke is spent */
                p->strokes++;
                p->meter = 0;
                p->max_t = 0;
                p->fx |= FXT_BOOM;
                set_phase(p, TP_BOOM);
            }
        }
        break;
    case TP_FLIGHT:
        p->fly_t++;
        if ((pressed & TB_A) && !p->slam_used && p->air_t >= 2) {
            p->slam_used = p->slammed = true;
            p->vy = fmaxf(p->vy, 0) * 0.3f + SLAM_V;
            p->fx |= FXT_SLAM;
        }
        ball_step(p, c);
        if (p->fly_t > p->max_fly_t) p->max_fly_t = p->fly_t;
        break;
    case TP_LOST:
        if (p->phase_t >= TSH_LOST_T) {
            p->x = p->sx;
            p->y = p->sy;
            p->vx = p->vy = 0;
            p->fx |= FXT_BACK;
            set_phase(p, TP_AIM);
        }
        break;
    case TP_BOOM:
        if (p->phase_t >= TSH_BOOM_T) set_phase(p, TP_AIM);
        break;
    default:
        break;
    }
    p->prev = b;
}
