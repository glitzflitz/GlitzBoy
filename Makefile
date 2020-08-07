.POSIX:
.SUFFIXES:
OPT ?= -O3
SDL2_CFLAGS = $(shell sdl2-config --cflags)
SDL2_LDLIBS =
EMU_FLAGS = -I./src

SDL2_ERRCHECK = 0
CWARNINGS = -Wall -Wextra

CC = cc
CFLAGS = -std=c99 $(SDL2_CFLAGS) $(OPT) $(CWARNINGS) $(EMU_FLAGS)
LDLIBS = 
LINKER = $(CC)

ifeq ($(STATIC),yes)
	SDL2_LDLIBS = $(shell sdl2-config --static-libs)
	SDL2_ERRCHECK = $(.SHELLSTATUS)
	CFLAGS += -static
else
	SDL2_LDLIBS = $(shell sdl2-config --libs)
endif
LDLIBS += $(SDL2_LDLIBS)

LDLIBS += -lm

ifeq ($(OS),Windows_NT)
	LDLIBS += -lcomctl32 -lole32 -loleaut32 -luuid
	# Skip dll generation on windows
	STATIC ?= yes
endif

all:GlitzBoy
GlitzBoy: src/emulator.o
	$(LINKER) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

emulator.o: sdl2_check src/emulator.c src/glitzboy.h

sdl2_check:
ifneq (0,$(SDL2_ERRCHECK))
	$(error Error calling sdl2-config. Maybe --static-libs was not accepted)
endif

clean:
	rm -f GlitzBoy build/emulator.o $(SOUND_OBJECTS) $(FILE_GUI_LIB)

help:
	@echo Options:
	@echo \ STATIC=yes\	Enable static build. Enabled by default on Windows.
	@echo \	 	\	Requires that SDL2 be compiled with --static-libs enabled.
	@echo

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
