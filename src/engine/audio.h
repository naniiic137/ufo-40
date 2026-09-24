/* UFO 40 - 4-channel chiptune synth (2 pulse, triangle, noise) with an
 * MML-style text sequencer ("UFO-MML") for music and sound effects.
 *
 * UFO-MML quick reference (one string per channel):
 *   c d e f g a b   note, optional + # (sharp) or - (flat), length, dots
 *   r               rest            ^n   tie (extend previous note)
 *   o4              octave          < >  octave down / up
 *   l8              default length (1,2,3,4,6,8,12,16,24,32,48)
 *   v0..v15         volume          @n   instrument
 *   q1..q8          gate (note length fraction in eighths)
 *   k-2 / k3        transpose in semitones
 *   [ ... ]3        repeat block n times (nestable)
 *   L               loop point for looping songs
 *   spaces and '|'  ignored (use them as bar lines)
 */
#ifndef UFO_AUDIO_H
#define UFO_AUDIO_H

#include <stdint.h>
#include <stdbool.h>

#define AUDIO_RATE 48000
#define AUDIO_CTRL_HZ 240
#define TICKS_PER_WHOLE 192

enum { CH_P1 = 0, CH_P2, CH_TRI, CH_NOISE, CH_COUNT };
enum { WAVE_PULSE = 0, WAVE_TRI, WAVE_NOISE };

typedef struct Instrument {
    uint8_t wave;      /* WAVE_* */
    uint8_t duty;      /* pulse: 0=12.5% 1=25% 2=50% 3=75% ; noise: 1 = metallic */
    uint8_t attack;    /* control ticks (1/240 s) */
    uint8_t decay;     /* control ticks from peak to sustain */
    uint8_t sustain;   /* 0..15 */
    uint8_t release;   /* control ticks */
    uint8_t vib_delay; /* control ticks before vibrato starts */
    uint8_t vib_depth; /* in 1/100 semitone */
    uint8_t vib_rate;  /* cycles per second * 10 */
    int8_t sweep;      /* pitch slide, 1/16 semitone per control tick */
    int8_t arp[3];     /* arpeggio offsets, cycled every 2 control ticks */
    uint8_t duty_mod;  /* 1 = cycle duty over time (chorus-ish shimmer) */
} Instrument;

#define MAX_INSTRUMENTS 48

void audio_init(void);
void audio_set_lock(void (*lock)(void), void (*unlock)(void));
/* Render interleaved stereo S16 frames. Called from the platform audio thread. */
void audio_render(int16_t *out, int frames);

void audio_set_instrument(int id, const Instrument *ins);

/* Songs: returns an id >= 0. loop=false for one-shot jingles. */
int song_define(const char *name, int bpm, bool loop,
                const char *p1, const char *p2, const char *tri, const char *noise);
int song_find(const char *name);
int song_count(void);
const char *song_name(int id);
bool song_loops(int id);
int song_channel_ticks(int id, int ch);       /* length of one pass, for tests */
int song_channel_loop_ticks(int id, int ch);  /* length from loop point */

void music_play(int id);          /* restarts only if different song */
void music_restart(int id);       /* always restart */
void music_stop(void);
void music_fade(int frames);      /* fade out then stop */
int music_playing(void);          /* id or -1 */
bool music_finished(void);        /* one-shot song ended */
void music_duck(bool on);         /* lower volume (pause menu) */

/* Sound effects: one channel each; they temporarily take over that channel. */
int sfx_define(const char *name, int channel, int bpm, const char *mml);
int sfx_find(const char *name);
void sfx_play(int id);
void sfx_play_name(const char *name);

void audio_set_volume(int music_0_10, int sfx_0_10);
/* Diagnostics for tests: peak/RMS of the most recent render call. */
void audio_stats(float *peak, float *rms);

#endif
