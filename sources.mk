# UFO 40 - the single source list, shared by the Makefile and CMakeLists.txt.
# One file per line, "VAR += path". CMake parses this file with a regex.

CORE_SRC += src/engine/gfx.c
CORE_SRC += src/engine/font.c
CORE_SRC += src/engine/input.c
CORE_SRC += src/engine/rng.c
CORE_SRC += src/engine/audio.c
CORE_SRC += src/engine/save.c
CORE_SRC += src/engine/scene.c
CORE_SRC += src/engine/engine.c
CORE_SRC += src/shell/ui.c
CORE_SRC += src/shell/app.c
CORE_SRC += src/shell/boot.c
CORE_SRC += src/shell/library.c
CORE_SRC += src/shell/settings.c
CORE_SRC += src/shell/shell_audio.c
CORE_SRC += src/shell/games.c
CORE_SRC += src/shell/vita_assets.c
CORE_SRC += src/games/underdelve/underdelve.c
CORE_SRC += src/games/underdelve/underdelve_rooms.c
CORE_SRC += src/games/underdelve/underdelve_art.c
CORE_SRC += src/games/underdelve/underdelve_audio.c
CORE_SRC += src/games/grubshift/grubshift.c
CORE_SRC += src/games/grubshift/grubshift_logic.c
CORE_SRC += src/games/grubshift/grubshift_art.c
CORE_SRC += src/games/grubshift/grubshift_audio.c
CORE_SRC += src/games/roofcat/roofcat.c

HEADLESS_SRC += src/platform/headless/main_headless.c
HEADLESS_SRC += src/platform/headless/imgwrite.c

SDL_SRC += src/platform/sdl2/main_sdl.c
