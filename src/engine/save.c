#include "save.h"
#include "platform.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

Progress g_progress;
int g_progress_origin;

_Static_assert(sizeof(Progress) == PROGRESS_UNTAGGED_SIZE + 16, "Progress is bytes only, no padding");
_Static_assert(PROGRESS_UNTAGGED_SIZE != PROGRESS_LEGACY40_SIZE, "the layouts must differ in size");
_Static_assert(MAX_GAMES <= 64, "known and opened hold a bit per slot");

#define SAVE_MAGIC 0x30344655u /* "UF40" little-endian */
#define SAVE_VERSION 1
#define HEADER_SIZE 16

static int toast_game[8], toast_bit[8], toast_n;

uint32_t crc32_buf(const void *data, int len) {
    const uint8_t *p = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFu;
    for (int i = 0; i < len; i++) {
        crc ^= p[i];
        for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

static void put32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static bool write_wrapped(const char *name, const void *data, int len) {
    uint8_t *buf = (uint8_t *)malloc((size_t)len + HEADER_SIZE);
    if (!buf) return false;
    put32(buf, SAVE_MAGIC);
    put32(buf + 4, SAVE_VERSION);
    put32(buf + 8, (uint32_t)len);
    put32(buf + 12, crc32_buf(data, len));
    memcpy(buf + HEADER_SIZE, data, (size_t)len);
    bool ok = plat_save_write(name, buf, len + HEADER_SIZE) == 0;
    free(buf);
    return ok;
}

static int read_wrapped(const char *name, void *data, int maxlen) {
    int cap = maxlen + HEADER_SIZE;
    uint8_t *buf = (uint8_t *)malloc((size_t)cap);
    if (!buf) return -1;
    int n = plat_save_read(name, buf, cap);
    int result = -1;
    if (n >= HEADER_SIZE && get32(buf) == SAVE_MAGIC && get32(buf + 4) == SAVE_VERSION) {
        int len = (int)get32(buf + 8);
        if (len <= maxlen && len == n - HEADER_SIZE && crc32_buf(buf + HEADER_SIZE, len) == get32(buf + 12)) {
            memcpy(data, buf + HEADER_SIZE, (size_t)len);
            result = len;
        }
    }
    free(buf);
    return result;
}

void progress_defaults(void) {
    memset(&g_progress, 0, sizeof g_progress);
    g_progress.music_vol = 7;
    g_progress.sfx_vol = 8;
    g_progress.scale = 3;
    g_progress.fullscreen = 0;
}

bool progress_bit(const uint8_t *set, int slot) {
    return slot >= 0 && slot < MAX_GAMES && (set[slot / 8] >> (slot % 8)) & 1;
}

void progress_set_bit(uint8_t *set, int slot, bool on) {
    if (slot < 0 || slot >= MAX_GAMES) return;
    if (on) set[slot / 8] = (uint8_t)(set[slot / 8] | (1u << (slot % 8)));
    else set[slot / 8] = (uint8_t)(set[slot / 8] & ~(1u << (slot % 8)));
}

/* A progress file from the 40-slot library: goals[40], played[40], then the
 * settings bytes in the order they still have. Slots 01-40 keep their goals
 * and play counts; 41-50 start empty. */
static void progress_from_legacy40(const uint8_t *b) {
    progress_defaults();
    memcpy(g_progress.goals, b, LEGACY_GAMES);
    memcpy(g_progress.played, b + LEGACY_GAMES, LEGACY_GAMES);
    const uint8_t *st = b + 2 * LEGACY_GAMES;
    g_progress.music_vol = st[0];
    g_progress.sfx_vol = st[1];
    g_progress.scale = st[2];
    g_progress.fullscreen = st[3];
    g_progress.last_game = st[4];
    g_progress.menu_pos = st[5];
    memcpy(g_progress.reserved, st + 6, sizeof g_progress.reserved);
}

/* The older layouts are written back in the current one by the shell, once
 * it has filled in the known and opened bits (shell_sync_cartridges): a file
 * written back before that would make every cartridge look new. */
bool progress_load(void) {
    union { Progress p; uint8_t raw[sizeof(Progress)]; } u;
    memset(&u, 0, sizeof u);
    int n = read_wrapped("progress.dat", &u, (int)sizeof u);
    if (n == PROGRESS_LEGACY40_SIZE) {
        progress_from_legacy40(u.raw);
        g_progress_origin = PROGRESS_UPGRADED;
    } else if (n == PROGRESS_UNTAGGED_SIZE) {
        g_progress = u.p; /* the bits it lacks read 0: nothing known yet */
        g_progress_origin = PROGRESS_UPGRADED;
    } else if (n == (int)sizeof u.p) {
        g_progress = u.p;
        g_progress_origin = PROGRESS_CURRENT;
    } else {
        progress_defaults();
        g_progress_origin = PROGRESS_FRESH;
        return false;
    }
    if (g_progress.music_vol > 10) g_progress.music_vol = 7;
    if (g_progress.sfx_vol > 10) g_progress.sfx_vol = 8;
    if (g_progress.scale < 1 || g_progress.scale > 8) g_progress.scale = 3;
    return true;
}

bool progress_save(void) { return write_wrapped("progress.dat", &g_progress, (int)sizeof g_progress); }

bool progress_award(int game, int bit) {
    if (game < 0 || game >= MAX_GAMES) return false;
    if (bit != GOAL_BEACON && bit != GOAL_SAUCER && bit != GOAL_ALIEN) return false;
    if (g_progress.goals[game] & bit) return false;
    g_progress.goals[game] = (uint8_t)(g_progress.goals[game] | bit);
    progress_save();
    if (toast_n < 8) { toast_game[toast_n] = game; toast_bit[toast_n] = bit; toast_n++; }
    return true;
}

int progress_goal_count(void) {
    int n = 0;
    for (int i = 0; i < MAX_GAMES; i++)
        for (int b = 0; b < 3; b++) n += (g_progress.goals[i] >> b) & 1;
    return n;
}

bool progress_pop_toast(int *game, int *bit) {
    if (toast_n == 0) return false;
    *game = toast_game[0];
    *bit = toast_bit[0];
    for (int i = 1; i < toast_n; i++) { toast_game[i - 1] = toast_game[i]; toast_bit[i - 1] = toast_bit[i]; }
    toast_n--;
    return true;
}

static void game_file(int game, char *out, int n) { snprintf(out, (size_t)n, "game%02d.sav", game + 1); }

bool game_save_write(int game, const void *data, int len) {
    char name[32];
    game_file(game, name, sizeof name);
    return write_wrapped(name, data, len);
}

int game_save_read(int game, void *data, int len) {
    char name[32];
    game_file(game, name, sizeof name);
    return read_wrapped(name, data, len);
}

void game_save_erase(int game) {
    char name[32];
    game_file(game, name, sizeof name);
    uint8_t zero = 0;
    plat_save_write(name, &zero, 0);
}

int game_save_raw_size(int game) {
    char name[32];
    game_file(game, name, sizeof name);
    int cap = 65536 + HEADER_SIZE;
    uint8_t *buf = (uint8_t *)malloc((size_t)cap);
    if (!buf) return 0;
    int n = plat_save_read(name, buf, cap);
    free(buf);
    return n > 0 ? n : 0;
}
