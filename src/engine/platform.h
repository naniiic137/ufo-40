/* UFO 40 - services every platform layer must provide to the engine. */
#ifndef UFO_PLATFORM_H
#define UFO_PLATFORM_H

#include <stdint.h>

enum { PLAT_PC = 0, PLAT_VITA, PLAT_WEB, PLAT_HEADLESS };

/* Persistent storage. name is a bare file name like "progress.dat".
 * write returns 0 on success; read returns bytes read or -1. */
int plat_save_write(const char *name, const void *data, int len);
int plat_save_read(const char *name, void *data, int maxlen);
/* Where the saves live, for the SAVE DATA screen ("ux0:data/UFO40"). */
const char *plat_save_where(void);

int plat_kind(void);
const char *plat_name(void);
/* PC only: window scale (1..8) and fullscreen. Others ignore. */
void plat_apply_video(int scale, int fullscreen);
void plat_request_quit(void);
/* A real-time clock in milliseconds that keeps counting while the frame loop
 * stalls (a hidden browser tab, a slow frame). Any starting point; it may wrap,
 * so callers only ever use the unsigned difference of two readings. */
uint32_t plat_clock_ms(void);

#endif
