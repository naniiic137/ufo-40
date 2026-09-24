# UFO 40 - local builds (headless runner, tests, desktop SDL2 build).
#
#   make headless      build the headless runner
#   make test          build it and run every tests/*.ufs script
#   make sdl           build the desktop game (needs SDL2)
#   make shots         render README screenshots into docs/shots
#
# Windows (w64devkit):  make sdl SDL2_DIR=C:/path/to/SDL2-2.x/x86_64-w64-mingw32
# Linux:                make sdl        (uses sdl2-config)

include sources.mk

CC      ?= gcc
BUILD   ?= build
OPT     ?= -O2
CFLAGS  += $(OPT) -std=c11 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
LDLIBS  += -lm

ifeq ($(OS),Windows_NT)
  EXE := .exe
else
  EXE :=
endif

HEADLESS_BIN := $(BUILD)/ufo40_headless$(EXE)
SDL_BIN      := $(BUILD)/ufo40$(EXE)

CORE_OBJ     := $(patsubst %.c,$(BUILD)/obj/%.o,$(CORE_SRC))
HEADLESS_OBJ := $(patsubst %.c,$(BUILD)/obj/%.o,$(HEADLESS_SRC))
SDL_OBJ      := $(patsubst %.c,$(BUILD)/sdlobj/%.o,$(SDL_SRC))

# ---- SDL2 flags
ifdef SDL2_DIR
  SDL_CFLAGS := -I$(SDL2_DIR)/include/SDL2 -I$(SDL2_DIR)/include
  SDL_LIBS   := -L$(SDL2_DIR)/lib -lmingw32 -lSDL2main -lSDL2 -mwindows
else
  SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null)
  SDL_LIBS   := $(shell sdl2-config --libs 2>/dev/null)
endif

.PHONY: all headless test sdl clean shots

all: headless

headless: $(HEADLESS_BIN)

$(HEADLESS_BIN): $(CORE_OBJ) $(HEADLESS_OBJ)
	$(CC) -o $@ $^ $(LDLIBS)

$(BUILD)/obj/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/sdlobj/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

sdl: $(SDL_BIN)

$(SDL_BIN): $(CORE_OBJ) $(SDL_OBJ)
	$(CC) -o $@ $^ $(SDL_LIBS) $(LDLIBS)
ifdef SDL2_DIR
	cp $(SDL2_DIR)/bin/SDL2.dll $(BUILD)/
endif

# Every test runs with its own fresh save directory.
test: $(HEADLESS_BIN)
	@rm -rf $(BUILD)/testsave
	@pass=0; fail=0; for t in tests/*.ufs; do \
	  n=$$(basename $$t .ufs); \
	  if ./$(HEADLESS_BIN) --script $$t --save-dir $(BUILD)/testsave/$$n --out $(BUILD)/testshots; then pass=$$((pass+1)); else fail=$$((fail+1)); fi; \
	done; \
	echo "== $$pass passed, $$fail failed =="; test $$fail -eq 0

shots: $(HEADLESS_BIN)
	@rm -rf $(BUILD)/shotsave
	@for t in tools/shots/*.ufs; do ./$(HEADLESS_BIN) --script $$t --save-dir $(BUILD)/shotsave --out docs/shots || exit 1; done

clean:
	rm -rf $(BUILD)
