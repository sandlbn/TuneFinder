# Makefile for Radio Browser
OS := $(shell uname)

# Directories
LIBDIR = /opt/amiga/m68k-amigaos/lib
SDKDIR = /opt/amiga/m68k-amigaos/sys-include
NDKDIR = /opt/amiga/m68k-amigaos/ndk-include
INCDIR = /opt/amiga/m68k-amigaos/include
SRCDIR = .
BUILDDIR = build/os3/obj
OUTDIR = out

# Program settings
CC = m68k-amigaos-gcc
PROGRAM_NAME = TuneFinder

# Source files
SOURCES = $(SRCDIR)/data/data.c \
          $(SRCDIR)/network/network.c \
          $(SRCDIR)/utils/utils.c \
          $(SRCDIR)/gui/gui.c \
          $(SRCDIR)/main.c

# Object files
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

# Compiler flags
BASE_CCFLAGS = -MP -MMD -Wextra -Wno-unused-function \
 -Wno-discarded-qualifiers -Wno-int-conversion \
 -Wno-volatile-register-var -fno-lto -noixemul \
 -fbaserel -lamiga -lm -D__AMIGAOS3__ \
 -I$(INCDIR) -I$(SDKDIR) -I$(NDKDIR) -Iinclude

ifdef DEBUG
CCFLAGS = $(BASE_CCFLAGS) -DDEBUG_BUILD -O0 -g
BUILD_TYPE = debug
else
CCFLAGS = $(BASE_CCFLAGS) -Ofast -fomit-frame-pointer
BUILD_TYPE = release
endif

LDFLAGS = -Wl,-Map=$(OUTDIR)/$(PROGRAM_NAME).map,-L$(LIBDIR),-ljson-c,-lamiga,-lm,-lmui

# Targets
.PHONY: all clean debug release dirs

all: release

debug:
	@echo "Building debug version..."
	$(MAKE) dirs
	$(MAKE) $(OUTDIR)/$(PROGRAM_NAME) DEBUG=1

release:
	@echo "Building release version..."
	$(MAKE) dirs
	$(MAKE) $(OUTDIR)/$(PROGRAM_NAME)

dirs:
	@mkdir -p $(BUILDDIR)/gui
	@mkdir -p $(BUILDDIR)/network
	@mkdir -p $(BUILDDIR)/data
	@mkdir -p $(BUILDDIR)/utils
	@mkdir -p $(OUTDIR)

$(OUTDIR)/$(PROGRAM_NAME): $(OBJECTS)
	$(info Linking $(PROGRAM_NAME) ($(BUILD_TYPE) build))
	$(CC) $(CCFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(info Compiling $< ($(BUILD_TYPE) build))
	$(CC) $(CCFLAGS) -c -o $@ $<

clean:
	$(info Cleaning...)
	@$(RM) -rf $(BUILDDIR)
	@$(RM) -f $(OUTDIR)/$(PROGRAM_NAME)
	@$(RM) -f $(OUTDIR)/$(PROGRAM_NAME).map

-include $(OBJECTS:.o=.d)