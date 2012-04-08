.SUFFIXES:

CONFIG_MAK := Config.mak
include Config.mak

SUFFIX := 

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
AR := $(CROSS_COMPILE)ar

EXTRA_DEFS := -D_FILE_OFFSET_BITS=64 -DHAVE_ACCEPT4=1
CFLAGS := -g -pipe -O2 -Wall \
	-Wsign-compare -Wcast-align \
	-Waggregate-return \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-Wmissing-declarations \
	-Wmissing-noreturn \
	-finline-functions \
	-Wmissing-format-attribute \
	-fwrapv \
	-Iinclude \
	-I/usr/include \
	$(SDL_CFLAGS) \
	$(EXTRA_DEFS)

ifeq ($(OS), win32)
	OS_OBJ := blob_win32.o
else
	OS_OBJ := blob.o
endif

ENGINE_OBJ := r_gl.o \
		img_png.o \
		renderer.o \
		asset.o \
		asset_render.o \
		tile.o \
		tile_render.o \
		map.o \
		tex.o \
		game.o \
		$(OS_OBJ)
ENGINE_LIBS := $(SDL_LIBS) $(GL_LIBS) -lpng

DS_BIN := dessert-stroke$(SUFFIX)
DS_OBJ := dessert-stroke.o \
		world.o \
		chopper.o \
		lobby.o \
		$(ENGINE_OBJ)

SPANK_BIN := spankassets$(SUFFIX)
SPANK_OBJ := spankassets.o \
		hgang.o

MKTILE_BIN := mktile$(SUFFIX)
MKTILE_OBJ := mktile.o

ALL_BIN := $(DS_BIN) $(SPANK_BIN) $(MKTILE_BIN)
ALL_OBJ := $(DS_OBJ) $(SPANK_OBJ) $(MKTILE_OBJ)
ALL_DEP := $(patsubst %.o, .%.d, $(ALL_OBJ))
ALL_TARGETS := $(ALL_BIN)

TARGET: all

.PHONY: all clean walk

all: $(ALL_BIN)

ifeq ($(filter clean, $(MAKECMDGOALS)),clean)
CLEAN_DEP := clean
else
CLEAN_DEP :=
endif

%.o %.d: %.c $(CLEAN_DEP) $(CONFIG_MAK) Makefile
	@echo " [C] $<"
	@$(CC) $(CFLAGS) -MMD -MF $(patsubst %.o, .%.d, $@) \
		-MT $(patsubst .%.d, %.o, $@) \
		-c -o $(patsubst .%.d, %.o, $@) $<

$(DS_BIN): $(DS_OBJ)
	@echo " [LINK] $@"
	@$(CC) $(CFLAGS) -o $@ $(DS_OBJ) $(ENGINE_LIBS)

$(SPANK_BIN): $(SPANK_OBJ)
	@echo " [LINK] $@"
	@$(CC) $(CFLAGS) -o $@ $(SPANK_OBJ)

$(MKTILE_BIN): $(MKTILE_OBJ)
	@echo " [LINK] $@"
	@$(CC) $(CFLAGS) -o $@ $(MKTILE_OBJ)

clean:
	rm -f $(ALL_TARGETS) $(ALL_OBJ) $(ALL_DEP)

ifneq ($(MAKECMDGOALS),clean)
-include $(ALL_DEP)
endif
