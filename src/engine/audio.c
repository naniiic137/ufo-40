/* UFO 40 - chiptune synth and UFO-MML sequencer. */
#include "audio.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ------------------------------------------------------------------ */
/* compiled data                                                       */

enum { EV_NOTE = 0, EV_REST, EV_INS, EV_VOL, EV_GATE };

typedef struct Ev {
    uint8_t type;
    uint8_t a;
    uint16_t dur;
} Ev;

typedef struct Track {
    Ev *ev;
    int n;
    int loop_index;  /* event index to jump back to */
    int ticks;       /* total ticks in one pass */
    int loop_ticks;  /* ticks from loop point to end */
} Track;

typedef struct Song {
    char name[24];
    int bpm;
    bool loop;
    Track tr[CH_COUNT];
} Song;

typedef struct Sfx {
    char name[24];
    int channel;
    int bpm;
    Track tr;
} Sfx;

#define MAX_SONGS 256 /* room for forty cartridges */
#define MAX_SFX 768

static Song songs[MAX_SONGS];
static int n_songs;
static Sfx sfxs[MAX_SFX];
static int n_sfx;
static Instrument instruments[MAX_INSTRUMENTS];

/* ------------------------------------------------------------------ */
/* voices                                                              */

enum { ENV_OFF = 0, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE };

typedef struct Voice {
    int wave;
    const Instrument *ins;
    float note;       /* midi note (float for slides) */
    float phase;      /* 0..1 */
    float dt;         /* phase increment per sample */
    float noise_acc;
    uint16_t lfsr;
    int env_stage;
    float env;
    float env_step;
    float vol;        /* 0..1 note volume */
    int ctrl_t;       /* control ticks since note on */
    float sweep_ofs;
    float amp;        /* cached vol*env */
    float duty;
} Voice;

typedef struct Seq {
    const Track *tr;
    int pos;
    int wait;        /* seq ticks until next event */
    int gate_left;   /* seq ticks until note off (-1 = none) */
    int ins, vol, gate;
    bool active;
    bool finished;
    bool loop;
    Voice *voice;
    int wave_hint;
} Seq;

static Voice music_voice[CH_COUNT];
static Voice sfx_voice[CH_COUNT];
static Seq music_seq[CH_COUNT];
static Seq sfx_seq[CH_COUNT];

static int cur_song = -1;
static float music_spt;   /* samples per tick */
static float music_acc;
static float sfx_spt[CH_COUNT], sfx_acc[CH_COUNT];
static int ctrl_counter;
static float music_gain = 0.8f, sfx_gain = 0.8f;
static bool ducked;
static int fade_total, fade_left;
static bool music_done_flag;
static float stat_peak, stat_rms;
static float lp_l, lp_r, dc_xl, dc_yl, dc_xr, dc_yr;

static void (*lock_fn)(void);
static void (*unlock_fn)(void);
static void lock(void) { if (lock_fn) lock_fn(); }
static void unlock(void) { if (unlock_fn) unlock_fn(); }

void audio_set_lock(void (*l)(void), void (*u)(void)) { lock_fn = l; unlock_fn = u; }

/* ------------------------------------------------------------------ */
/* default instrument bank                                              */

static void ins(int id, int wave, int duty, int a, int d, int s, int r, int vd, int vdep, int vr,
                int sweep, int a0, int a1, int dm) {
    Instrument *i = &instruments[id];
    i->wave = (uint8_t)wave; i->duty = (uint8_t)duty;
    i->attack = (uint8_t)a; i->decay = (uint8_t)d; i->sustain = (uint8_t)s; i->release = (uint8_t)r;
    i->vib_delay = (uint8_t)vd; i->vib_depth = (uint8_t)vdep; i->vib_rate = (uint8_t)vr;
    i->sweep = (int8_t)sweep; i->arp[0] = (int8_t)a0; i->arp[1] = (int8_t)a1; i->arp[2] = 0;
    i->duty_mod = (uint8_t)dm;
}

static void default_bank(void) {
    /*        id wave        duty  A   D   S   R  vdel vdep vrate sweep arp0 arp1 dmod */
    ins(0,  WAVE_PULSE, 2,  1, 24, 10, 12, 40, 18, 55, 0, 0, 0, 0);   /* round lead         */
    ins(1,  WAVE_PULSE, 1,  1, 30,  9, 14, 30, 22, 60, 0, 0, 0, 0);   /* bright lead + vib  */
    ins(2,  WAVE_PULSE, 0,  0, 20,  4,  8,  0,  0,  0, 0, 0, 0, 0);   /* thin pluck         */
    ins(3,  WAVE_PULSE, 1,  0, 36,  0,  6,  0,  0,  0, 0, 0, 0, 0);   /* staccato pluck     */
    ins(4,  WAVE_PULSE, 2, 30, 40,  8, 40,  0,  0,  0, 0, 0, 0, 1);   /* soft pad (pwm)     */
    ins(5,  WAVE_PULSE, 1,  2, 60,  5, 30, 60, 12, 50, 0, 0, 0, 0);   /* echo / soft lead   */
    ins(6,  WAVE_TRI,   0,  0,  0, 15,  6,  0,  0,  0, 0, 0, 0, 0);   /* triangle bass      */
    ins(7,  WAVE_TRI,   0,  0, 40,  6,  8,  0,  0,  0, 0, 0, 0, 0);   /* triangle pluck     */
    ins(8,  WAVE_TRI,   0,  0, 18,  0,  2,  0,  0,  0, -64, 0, 0, 0); /* triangle kick      */
    ins(9,  WAVE_NOISE, 0,  0,  8,  0,  2,  0,  0,  0, 0, 0, 0, 0);   /* closed hat         */
    ins(10, WAVE_NOISE, 0,  0, 40,  0,  8,  0,  0,  0, 0, 0, 0, 0);   /* open hat           */
    ins(11, WAVE_NOISE, 0,  0, 30,  0,  6,  0,  0,  0, -4, 0, 0, 0);  /* snare              */
    ins(12, WAVE_NOISE, 0,  0,160,  0, 20,  0,  0,  0, 0, 0, 0, 0);   /* crash              */
    ins(13, WAVE_NOISE, 0,  0, 12,  0,  2,  0,  0,  0, -24, 0, 0, 0); /* noise kick thump   */
    ins(14, WAVE_PULSE, 2,  2, 50, 11, 16, 50, 30, 50, 0, 0, 0, 0);   /* singing lead       */
    ins(15, WAVE_PULSE, 0,  0, 50,  2, 30,  0,  0,  0, 0, 0, 0, 0);   /* bell               */
    ins(16, WAVE_PULSE, 1,  0, 30,  6, 10,  0,  0,  0, 0, 4, 7, 0);   /* major arp chord    */
    ins(17, WAVE_PULSE, 1,  0, 30,  6, 10,  0,  0,  0, 0, 3, 7, 0);   /* minor arp chord    */
    ins(18, WAVE_PULSE, 0,  0, 30,  6, 10,  0,  0,  0, 0, 12, 0, 0);  /* octave arp         */
    ins(19, WAVE_TRI,   0,  0,  0, 15, 20, 40, 16, 50, 0, 0, 0, 0);   /* triangle lead+vib  */
    ins(20, WAVE_PULSE, 3,  0, 10,  0,  4,  0,  0,  0, 0, 0, 0, 0);   /* short blip         */
    ins(21, WAVE_NOISE, 1,  0, 20,  0,  4,  0,  0,  0, 0, 0, 0, 0);   /* metallic tick      */
    ins(22, WAVE_PULSE, 2,  0, 60,  7, 20,  0,  0,  0, 0, 0, 0, 1);   /* chorus strum       */
    ins(23, WAVE_PULSE, 1,  8, 40, 12, 24, 20, 26, 45, 0, 0, 0, 1);   /* warm brass         */
    /* sound-effect instruments */
    ins(32, WAVE_PULSE, 1,  0, 30,  0,  2,  0,  0,  0, 40, 0, 0, 0);  /* rising blip (jump) */
    ins(33, WAVE_PULSE, 2,  0, 40,  0,  2,  0,  0,  0, -40, 0, 0, 0); /* falling blip       */
    ins(34, WAVE_NOISE, 0,  0,110,  0, 10,  0,  0,  0, -6, 0, 0, 0);  /* explosion          */
    ins(35, WAVE_PULSE, 0,  0, 24,  3,  8,  0,  0,  0, 0, 0, 0, 0);   /* coin ping          */
    ins(36, WAVE_NOISE, 0,  0, 16,  0,  2,  0,  0,  0, 12, 0, 0, 0);  /* swish              */
    ins(37, WAVE_PULSE, 3,  0, 60,  0,  4,  0,  0,  0, -12, 0, 0, 0); /* hurt / bonk        */
    ins(38, WAVE_TRI,   0,  0, 30,  0,  4,  0,  0,  0, 24, 0, 0, 0);  /* tri rise           */
    ins(39, WAVE_PULSE, 1,  0, 20,  8,  6,  0,  0,  0, 0, 12, 0, 0);  /* sparkle arp        */
    ins(40, WAVE_NOISE, 1,  0, 50,  0,  8,  0,  0,  0, -3, 0, 0, 0);  /* metallic crash     */
    ins(41, WAVE_TRI,   0,  0, 70,  0,  6,  0,  0,  0, -10, 0, 0, 0); /* tri fall (thud)    */
    ins(42, WAVE_PULSE, 2,  0, 14,  0,  2,  0,  0,  0, 0, 0, 0, 0);   /* menu tick          */
}

void audio_set_instrument(int id, const Instrument *i) {
    if (id < 0 || id >= MAX_INSTRUMENTS) return;
    lock();
    instruments[id] = *i;
    unlock();
}

/* ------------------------------------------------------------------ */
/* UFO-MML compiler                                                     */

static void expand_repeats(const char *src, char *dst, size_t cap, size_t *len) {
    while (*src && *len + 1 < cap) {
        if (*src == '[') {
            int depth = 1;
            const char *start = src + 1, *p = start;
            while (*p && depth > 0) {
                if (*p == '[') depth++;
                else if (*p == ']') depth--;
                if (depth > 0) p++;
            }
            size_t inner_len = (size_t)(p - start);
            char *inner = (char *)malloc(inner_len + 1);
            memcpy(inner, start, inner_len);
            inner[inner_len] = 0;
            if (*p == ']') p++;
            int count = 0;
            while (*p >= '0' && *p <= '9') count = count * 10 + (*p++ - '0');
            if (count <= 0) count = 2;
            char *exp = (char *)malloc(cap);
            size_t elen = 0;
            expand_repeats(inner, exp, cap, &elen);
            for (int i = 0; i < count && *len + elen + 1 < cap; i++) {
                memcpy(dst + *len, exp, elen);
                *len += elen;
            }
            free(exp);
            free(inner);
            src = p;
        } else {
            dst[(*len)++] = *src++;
        }
    }
    dst[*len] = 0;
}

static int parse_int(const char **p, int *out) {
    int sign = 1, v = 0, any = 0;
    if (**p == '-') { sign = -1; (*p)++; }
    else if (**p == '+') (*p)++;
    while (**p >= '0' && **p <= '9') { v = v * 10 + (**p - '0'); (*p)++; any = 1; }
    *out = v * sign;
    return any;
}

static int parse_len(const char **p, int deflen) {
    int n, t = deflen;
    if (parse_int(p, &n) && n > 0) t = TICKS_PER_WHOLE / n;
    int add = t / 2;
    while (**p == '.') { t += add; add /= 2; (*p)++; }
    return t;
}

static void compile_track(const char *mml, Track *tr) {
    memset(tr, 0, sizeof *tr);
    if (!mml || !*mml) return;
    size_t cap = strlen(mml) * 16 + 4096;
    char *flat = (char *)malloc(cap);
    size_t flen = 0;
    expand_repeats(mml, flat, cap, &flen);

    int evcap = 256;
    tr->ev = (Ev *)malloc(sizeof(Ev) * evcap);
    int octave = 4, deflen = 48, transpose = 0, loop_index = 0, loop_tick = 0, ticks = 0;
    int last_timed = -1;
    const char *p = flat;
    static const int semis[7] = {9, 11, 0, 2, 4, 5, 7}; /* a b c d e f g */
    while (*p) {
        char c = *p++;
        if (tr->n + 2 >= evcap) {
            evcap *= 2;
            tr->ev = (Ev *)realloc(tr->ev, sizeof(Ev) * evcap);
        }
        Ev *e = &tr->ev[tr->n];
        if (c >= 'a' && c <= 'g') {
            int s = semis[c - 'a'];
            while (*p == '+' || *p == '#' || *p == '-') { s += (*p == '-') ? -1 : 1; p++; }
            int note = 12 * (octave + 1) + s + transpose;
            if (note < 0) note = 0;
            if (note > 127) note = 127;
            int d = parse_len(&p, deflen);
            e->type = EV_NOTE; e->a = (uint8_t)note; e->dur = (uint16_t)d;
            last_timed = tr->n++;
            ticks += d;
        } else if (c == 'r') {
            int d = parse_len(&p, deflen);
            e->type = EV_REST; e->a = 0; e->dur = (uint16_t)d;
            last_timed = tr->n++;
            ticks += d;
        } else if (c == '^') {
            int d = parse_len(&p, deflen);
            if (last_timed >= 0) { tr->ev[last_timed].dur = (uint16_t)(tr->ev[last_timed].dur + d); ticks += d; }
        } else if (c == 'o') {
            int v; if (parse_int(&p, &v)) octave = v;
        } else if (c == '>') {
            octave++;
        } else if (c == '<') {
            octave--;
        } else if (c == 'l') {
            deflen = parse_len(&p, deflen);
        } else if (c == 'v') {
            int v; parse_int(&p, &v);
            e->type = EV_VOL; e->a = (uint8_t)(v < 0 ? 0 : v > 15 ? 15 : v); e->dur = 0; tr->n++;
        } else if (c == '@') {
            int v; parse_int(&p, &v);
            e->type = EV_INS; e->a = (uint8_t)(v < 0 ? 0 : v >= MAX_INSTRUMENTS ? 0 : v); e->dur = 0; tr->n++;
        } else if (c == 'q') {
            int v; parse_int(&p, &v);
            e->type = EV_GATE; e->a = (uint8_t)(v < 1 ? 1 : v > 8 ? 8 : v); e->dur = 0; tr->n++;
        } else if (c == 'k') {
            int v; parse_int(&p, &v); transpose = v;
        } else if (c == 'L') {
            loop_index = tr->n;
            loop_tick = ticks;
        }
        /* everything else (spaces, bars, newlines) is ignored */
    }
    tr->loop_index = loop_index;
    tr->ticks = ticks;
    tr->loop_ticks = ticks - loop_tick;
    free(flat);
}

int song_define(const char *name, int bpm, bool loop, const char *p1, const char *p2,
                const char *tri, const char *noise) {
    if (n_songs >= MAX_SONGS) return -1;
    Song *s = &songs[n_songs];
    memset(s, 0, sizeof *s);
    snprintf(s->name, sizeof s->name, "%s", name);
    s->bpm = bpm;
    s->loop = loop;
    const char *src[4] = {p1, p2, tri, noise};
    for (int c = 0; c < CH_COUNT; c++) compile_track(src[c], &s->tr[c]);
    return n_songs++;
}

int song_find(const char *name) {
    for (int i = 0; i < n_songs; i++)
        if (strcmp(songs[i].name, name) == 0) return i;
    return -1;
}
int song_count(void) { return n_songs; }
const char *song_name(int id) { return (id >= 0 && id < n_songs) ? songs[id].name : ""; }
bool song_loops(int id) { return id >= 0 && id < n_songs && songs[id].loop; }
int song_bpm(int id) { return (id >= 0 && id < n_songs) ? songs[id].bpm : 0; }
int song_channel_ticks(int id, int ch) { return (id >= 0 && id < n_songs) ? songs[id].tr[ch].ticks : 0; }
int song_channel_loop_ticks(int id, int ch) { return (id >= 0 && id < n_songs) ? songs[id].tr[ch].loop_ticks : 0; }

int sfx_define(const char *name, int channel, int bpm, const char *mml) {
    if (n_sfx >= MAX_SFX) return -1;
    Sfx *s = &sfxs[n_sfx];
    memset(s, 0, sizeof *s);
    snprintf(s->name, sizeof s->name, "%s", name);
    s->channel = channel;
    s->bpm = bpm;
    compile_track(mml, &s->tr);
    return n_sfx++;
}

int sfx_find(const char *name) {
    for (int i = 0; i < n_sfx; i++)
        if (strcmp(sfxs[i].name, name) == 0) return i;
    return -1;
}

/* ------------------------------------------------------------------ */
/* voice control                                                        */

static float spt_for_bpm(int bpm) {
    if (bpm <= 0) bpm = 120;
    /* a quarter note is 48 ticks */
    return (float)AUDIO_RATE * 60.0f / ((float)bpm * 48.0f);
}

static void voice_note_on(Voice *v, const Instrument *in, int note, int vol) {
    v->ins = in;
    v->wave = in->wave;
    v->note = (float)note;
    v->vol = (float)vol / 15.0f;
    v->ctrl_t = 0;
    v->sweep_ofs = 0;
    v->env_stage = ENV_ATTACK;
    if (in->attack == 0) {
        v->env = 1.0f;
        v->env_stage = ENV_DECAY;
    } else {
        v->env = 0.0f;
    }
    if (!v->lfsr) v->lfsr = 1;
    static const float duties[4] = {0.125f, 0.25f, 0.5f, 0.75f};
    v->duty = duties[in->duty & 3];
}

static void voice_note_off(Voice *v) {
    if (v->env_stage != ENV_OFF && v->env_stage != ENV_RELEASE) v->env_stage = ENV_RELEASE;
}

static float midi_to_hz(float n) { return 440.0f * powf(2.0f, (n - 69.0f) / 12.0f); }

/* control-rate update: envelope, vibrato, sweeps, arps (240 Hz) */
static void voice_ctrl(Voice *v) {
    const Instrument *in = v->ins;
    if (!in || v->env_stage == ENV_OFF) { v->amp = 0; return; }
    float sus = (float)in->sustain / 15.0f;
    switch (v->env_stage) {
    case ENV_ATTACK:
        v->env += 1.0f / (float)(in->attack ? in->attack : 1);
        if (v->env >= 1.0f) { v->env = 1.0f; v->env_stage = ENV_DECAY; }
        break;
    case ENV_DECAY:
        if (in->decay == 0) { v->env = sus; v->env_stage = ENV_SUSTAIN; }
        else {
            v->env -= (1.0f - sus) / (float)in->decay;
            if (v->env <= sus) { v->env = sus; v->env_stage = ENV_SUSTAIN; }
        }
        if (v->env <= 0.0005f && sus <= 0.0f) { v->env = 0; v->env_stage = ENV_OFF; }
        break;
    case ENV_SUSTAIN:
        if (sus <= 0.0f) v->env_stage = ENV_OFF;
        break;
    case ENV_RELEASE:
        if (in->release == 0) v->env = 0;
        else v->env -= 1.0f / (float)in->release;
        if (v->env <= 0) { v->env = 0; v->env_stage = ENV_OFF; }
        break;
    }
    float n = v->note;
    v->sweep_ofs += (float)in->sweep / 16.0f;
    n += v->sweep_ofs;
    if (in->vib_depth && v->ctrl_t >= in->vib_delay) {
        float t = (float)(v->ctrl_t - in->vib_delay) / (float)AUDIO_CTRL_HZ;
        float ramp = (float)(v->ctrl_t - in->vib_delay) / 60.0f;
        if (ramp > 1.0f) ramp = 1.0f;
        n += ramp * ((float)in->vib_depth / 100.0f) * sinf(2.0f * (float)M_PI * ((float)in->vib_rate / 10.0f) * t);
    }
    if (in->arp[0] || in->arp[1]) {
        int steps = in->arp[1] ? 3 : 2;
        int idx = (v->ctrl_t / 4) % steps;
        if (idx == 1) n += in->arp[0];
        else if (idx == 2) n += in->arp[1];
    }
    if (in->duty_mod && v->wave == WAVE_PULSE) {
        float t = (float)v->ctrl_t / (float)AUDIO_CTRL_HZ;
        v->duty = 0.36f + 0.13f * sinf(2.0f * (float)M_PI * 0.8f * t);
    }
    if (n < 0) n = 0;
    if (n > 135) n = 135;
    float hz = midi_to_hz(n);
    if (v->wave == WAVE_NOISE) v->dt = hz * 24.0f / (float)AUDIO_RATE;
    else v->dt = hz / (float)AUDIO_RATE;
    v->amp = v->vol * v->env;
    v->ctrl_t++;
}

static inline float poly_blep(float t, float dt) {
    if (t < dt) { t /= dt; return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

static inline float voice_sample(Voice *v) {
    if (v->amp <= 0.0f) return 0.0f;
    float out;
    switch (v->wave) {
    case WAVE_PULSE: {
        float t = v->phase, dt = v->dt, d = v->duty;
        out = t < d ? 1.0f : -1.0f;
        out += poly_blep(t, dt);
        float t2 = t + (1.0f - d);
        if (t2 >= 1.0f) t2 -= 1.0f;
        out -= poly_blep(t2, dt);
        v->phase += dt;
        if (v->phase >= 1.0f) v->phase -= 1.0f;
        break;
    }
    case WAVE_TRI: {
        float t = v->phase;
        float tri = t < 0.5f ? t * 2.0f : 2.0f - t * 2.0f; /* 0..1 */
        int step = (int)(tri * 15.99f);
        out = (float)step / 7.5f - 1.0f;
        v->phase += v->dt;
        if (v->phase >= 1.0f) v->phase -= 1.0f;
        break;
    }
    default: { /* noise */
        v->noise_acc += v->dt;
        int guard = 0;
        while (v->noise_acc >= 1.0f && guard++ < 64) {
            v->noise_acc -= 1.0f;
            int tap = (v->ins && v->ins->duty) ? 6 : 1;
            uint16_t bit = (uint16_t)(((v->lfsr >> 0) ^ (v->lfsr >> tap)) & 1u);
            v->lfsr = (uint16_t)((v->lfsr >> 1) | (bit << 14));
        }
        if (v->noise_acc >= 1.0f) v->noise_acc = 0;
        out = (v->lfsr & 1u) ? 1.0f : -1.0f;
        break;
    }
    }
    return out * v->amp;
}

/* ------------------------------------------------------------------ */
/* sequencer                                                            */

static void seq_start(Seq *s, const Track *tr, Voice *v, bool loop) {
    memset(s, 0, sizeof *s);
    s->tr = tr;
    s->voice = v;
    s->vol = 12;
    s->gate = 7;
    s->ins = 0;
    s->loop = loop;
    s->active = tr && tr->n > 0;
    s->finished = !s->active;
    s->wait = 0;
    s->gate_left = -1;
}

static void seq_tick(Seq *s) {
    if (!s->active) return;
    if (s->gate_left > 0 && --s->gate_left == 0) voice_note_off(s->voice);
    if (s->wait > 0) s->wait--;
    int guard = 0;
    while (s->wait == 0 && s->active && guard++ < 512) {
        if (s->pos >= s->tr->n) {
            if (s->loop && s->tr->ticks > 0) s->pos = s->tr->loop_index;
            else { s->active = false; s->finished = true; voice_note_off(s->voice); return; }
            if (s->tr->loop_ticks <= 0) { s->active = false; s->finished = true; return; }
        }
        const Ev *e = &s->tr->ev[s->pos++];
        switch (e->type) {
        case EV_NOTE: {
            voice_note_on(s->voice, &instruments[s->ins], e->a, s->vol);
            s->wait = e->dur;
            s->gate_left = s->gate >= 8 ? -1 : (e->dur * s->gate) / 8;
            if (s->gate_left == 0) s->gate_left = 1;
            break;
        }
        case EV_REST:
            voice_note_off(s->voice);
            s->wait = e->dur;
            s->gate_left = -1;
            break;
        case EV_INS: s->ins = e->a; break;
        case EV_VOL: s->vol = e->a; break;
        case EV_GATE: s->gate = e->a; break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* public control                                                       */

void audio_init(void) {
    memset(music_voice, 0, sizeof music_voice);
    memset(sfx_voice, 0, sizeof sfx_voice);
    memset(music_seq, 0, sizeof music_seq);
    memset(sfx_seq, 0, sizeof sfx_seq);
    for (int i = 0; i < CH_COUNT; i++) { music_voice[i].lfsr = 1; sfx_voice[i].lfsr = 1; }
    default_bank();
    cur_song = -1;
    ctrl_counter = 0;
}

static void music_start_locked(int id) {
    cur_song = id;
    music_done_flag = false;
    fade_total = fade_left = 0;
    if (id < 0 || id >= n_songs) {
        for (int c = 0; c < CH_COUNT; c++) { music_seq[c].active = false; voice_note_off(&music_voice[c]); }
        cur_song = -1;
        return;
    }
    Song *s = &songs[id];
    music_spt = spt_for_bpm(s->bpm);
    music_acc = 0;
    for (int c = 0; c < CH_COUNT; c++) {
        music_voice[c].env_stage = ENV_OFF;
        seq_start(&music_seq[c], &s->tr[c], &music_voice[c], s->loop);
    }
}

void music_play(int id) {
    lock();
    if (id != cur_song || fade_left > 0) music_start_locked(id);
    unlock();
}
void music_restart(int id) { lock(); music_start_locked(id); unlock(); }
void music_stop(void) { lock(); music_start_locked(-1); unlock(); }
void music_fade(int frames) {
    lock();
    if (cur_song >= 0) { fade_total = fade_left = frames * (AUDIO_RATE / 60); }
    unlock();
}
int music_playing(void) { return cur_song; }
bool music_finished(void) { return music_done_flag || cur_song < 0; }
void music_duck(bool on) { ducked = on; }

void sfx_play(int id) {
    if (id < 0 || id >= n_sfx) return;
    lock();
    Sfx *s = &sfxs[id];
    int c = s->channel;
    sfx_spt[c] = spt_for_bpm(s->bpm);
    sfx_acc[c] = 0;
    sfx_voice[c].env_stage = ENV_OFF;
    sfx_voice[c].phase = 0;
    seq_start(&sfx_seq[c], &s->tr, &sfx_voice[c], false);
    unlock();
}

void sfx_play_name(const char *name) { sfx_play(sfx_find(name)); }

void audio_set_volume(int m, int s) {
    if (m < 0) m = 0;
    if (m > 10) m = 10;
    if (s < 0) s = 0;
    if (s > 10) s = 10;
    music_gain = (float)m / 10.0f;
    sfx_gain = (float)s / 10.0f;
}

void audio_stats(float *peak, float *rms) {
    if (peak) *peak = stat_peak;
    if (rms) *rms = stat_rms;
}

/* ------------------------------------------------------------------ */
/* render                                                               */

static const float CH_GAIN[CH_COUNT] = {0.20f, 0.19f, 0.32f, 0.15f};
static const float PAN_L[CH_COUNT] = {1.00f, 0.72f, 1.0f, 0.92f};
static const float PAN_R[CH_COUNT] = {0.72f, 1.00f, 1.0f, 0.92f};

void audio_render(int16_t *out, int frames) {
    float peak = 0, sumsq = 0;
    const float ctrl_period = (float)(AUDIO_RATE / AUDIO_CTRL_HZ);
    for (int i = 0; i < frames; i++) {
        /* sequencers */
        if (cur_song >= 0) {
            music_acc += 1.0f;
            while (music_acc >= music_spt) {
                music_acc -= music_spt;
                bool any = false;
                for (int c = 0; c < CH_COUNT; c++) { seq_tick(&music_seq[c]); any |= music_seq[c].active; }
                if (!any && !songs[cur_song].loop) { music_done_flag = true; }
            }
        }
        for (int c = 0; c < CH_COUNT; c++) {
            if (!sfx_seq[c].active) continue;
            sfx_acc[c] += 1.0f;
            while (sfx_acc[c] >= sfx_spt[c]) { sfx_acc[c] -= sfx_spt[c]; seq_tick(&sfx_seq[c]); }
        }
        if (--ctrl_counter <= 0) {
            ctrl_counter = (int)ctrl_period;
            for (int c = 0; c < CH_COUNT; c++) { voice_ctrl(&music_voice[c]); voice_ctrl(&sfx_voice[c]); }
        }
        float mg = music_gain * (ducked ? 0.35f : 1.0f);
        if (fade_left > 0) {
            mg *= (float)fade_left / (float)fade_total;
            if (--fade_left == 0) { music_start_locked(-1); }
        }
        float l = 0, r = 0;
        for (int c = 0; c < CH_COUNT; c++) {
            Voice *sv = &sfx_voice[c];
            bool sfx_on = sfx_seq[c].active || sv->env_stage != ENV_OFF;
            float m = voice_sample(&music_voice[c]); /* keep phase running */
            float s;
            if (sfx_on) s = voice_sample(sv) * sfx_gain;
            else s = m * mg;
            s *= CH_GAIN[c];
            l += s * PAN_L[c];
            r += s * PAN_R[c];
        }
        /* gentle low-pass + DC blocker */
        lp_l += 0.80f * (l - lp_l);
        lp_r += 0.80f * (r - lp_r);
        float yl = lp_l - dc_xl + 0.9975f * dc_yl; dc_xl = lp_l; dc_yl = yl;
        float yr = lp_r - dc_xr + 0.9975f * dc_yr; dc_xr = lp_r; dc_yr = yr;
        /* soft clip */
        if (yl > 1.0f) yl = 1.0f; else if (yl < -1.0f) yl = -1.0f;
        if (yr > 1.0f) yr = 1.0f; else if (yr < -1.0f) yr = -1.0f;
        float a = fabsf(yl);
        if (a > peak) peak = a;
        sumsq += yl * yl;
        out[i * 2] = (int16_t)(yl * 30000.0f);
        out[i * 2 + 1] = (int16_t)(yr * 30000.0f);
    }
    stat_peak = peak;
    stat_rms = frames > 0 ? sqrtf(sumsq / (float)frames) : 0;
}
