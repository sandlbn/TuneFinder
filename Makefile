# Makefile for Radio Browser
OS := $(shell uname)
#CPU_FLAGS = -m68000 -mnobitfield -fno-peephole2
CPU_FLAGS = 
# Directories
LIBDIR = /opt/amiga/m68k-amigaos/lib
SDKDIR = /opt/amiga/m68k-amigaos/sys-include
NDKDIR = /opt/amiga/m68k-amigaos/ndk-include
INCDIR = /opt/amiga/m68k-amigaos/include
SRCDIR = src
BUILDDIR = build/os3/obj
OUTDIR = out
AMINET_NAME = TuneFinder
ASSETS_DIR = assets
AMINET_DIR = $(ASSETS_DIR)/aminet
ICON_DIR = $(ASSETS_DIR)/icon
RELEASE_DIR = release
FLEXCAT = flexcat
CD_FILE = assets/translation/tunefinder.cd

# Program settings
CC = m68k-amigaos-gcc
PROGRAM_NAME = TuneFinder

# Source files
SOURCES = $(SRCDIR)/settings/settings.c \
		  $(SRCDIR)/settings/country_config.c \
          $(SRCDIR)/data/data.c \
          $(SRCDIR)/amigaamp/amigaamp.c \
          $(SRCDIR)/locale/locale.c \
          $(SRCDIR)/network/network.c \
          $(SRCDIR)/utils/gui_layout.c \
		  $(SRCDIR)/utils/utils.c \
          $(SRCDIR)/gui/gui.c \
          $(SRCDIR)/main.c

# Object files
OBJECTS = \
    $(BUILDDIR)/settings/settings.o \
	$(BUILDDIR)/settings/country_config.o \
    $(BUILDDIR)/data/data.o \
    $(BUILDDIR)/amigaamp/amigaamp.o \
    $(BUILDDIR)/locale/locale.o \
    $(BUILDDIR)/network/network.o \
	$(BUILDDIR)/utils/gui_layout.o \
    $(BUILDDIR)/utils/utils.o \
    $(BUILDDIR)/gui/gui.o \
    $(BUILDDIR)/main.o

# Compiler flags
BASE_CCFLAGS = -MP -MMD -Wextra -Wno-unused-function \
 -Wno-discarded-qualifiers -Wno-int-conversion \
 -Wno-volatile-register-var -fno-lto -noixemul \
 -fbaserel -lamiga -lm -D__AMIGAOS3__ $(CPU_FLAGS) \
 -I$(INCDIR) -I$(SDKDIR) -I$(NDKDIR) -Iinclude
 
ifdef DEBUG
CCFLAGS = $(BASE_CCFLAGS) -DDEBUG_BUILD -O0 -g
BUILD_TYPE = debug
else
CCFLAGS = $(BASE_CCFLAGS) -O2 -fomit-frame-pointer
BUILD_TYPE = release
endif

LDFLAGS = -Wl,-Map=$(OUTDIR)/$(PROGRAM_NAME).map,-L$(LIBDIR),-ljson-c,-lamiga,-lm

# Targets
.PHONY: all clean debug release dirs aminet-release aminet-clean

all: release
aminet-release: $(OUTDIR)/$(PROGRAM_NAME)
	$(info Creating Aminet release...)
	mkdir -p $(RELEASE_DIR)/$(AMINET_NAME)
	# Copy main program and info
	cp $(OUTDIR)/$(PROGRAM_NAME) $(RELEASE_DIR)/$(AMINET_NAME)/
	cp $(AMINET_DIR)/$(PROGRAM_NAME).README $(RELEASE_DIR)/$(AMINET_NAME)/
	cp $(AMINET_DIR)/$(PROGRAM_NAME)_deutsch.README $(RELEASE_DIR)/$(AMINET_NAME)/
	mkdir -p $(RELEASE_DIR)/$(AMINET_NAME)/Icons
	cp $(ICON_DIR)/$(PROGRAM_NAME).info $(RELEASE_DIR)/$(AMINET_NAME)/
	cp $(ICON_DIR)/$(PROGRAM_NAME)Drawer.info $(RELEASE_DIR)/$(AMINET_NAME).info
	cp $(ICON_DIR)/$(PROGRAM_NAME)Drawer.info $(RELEASE_DIR)/$(AMINET_NAME)/Icons/TuneFinder.info

	# Create and copy guide
	mkdir -p $(RELEASE_DIR)/$(AMINET_NAME)/Docs
	cp assets/amigaguide/tunefinder.guide $(RELEASE_DIR)/$(AMINET_NAME)/Docs/
	cp assets/amigaguide/tunefinder.guide.info $(RELEASE_DIR)/$(AMINET_NAME)/Docs/ || true

	# Copy examples 
	mkdir -p $(RELEASE_DIR)/$(AMINET_NAME)/Example
	cp assets/example/example_countries.cfg $(RELEASE_DIR)/$(AMINET_NAME)/Example/example_countries.cfg

	# Copy installer
	cp assets/icon/Install.info $(RELEASE_DIR)/$(AMINET_NAME)/Install_TuneFinder.info
	cp assets/install/Install_TuneFinder $(RELEASE_DIR)/$(AMINET_NAME)/Install_TuneFinder

	# Create and copy translations
	mkdir -p $(RELEASE_DIR)/$(AMINET_NAME)/Catalogs/polski
	mkdir -p $(RELEASE_DIR)/$(AMINET_NAME)/Catalogs/deutsch
	mkdir -p $(RELEASE_DIR)/$(AMINET_NAME)/Catalogs/italiano
	cp assets/translation/polski/tunefinder.catalog $(RELEASE_DIR)/$(AMINET_NAME)/Catalogs/polski/ 
	cp assets/translation/deutsch/tunefinder.catalog $(RELEASE_DIR)/$(AMINET_NAME)/Catalogs/deutsch/ 
	cp assets/translation/italiano/tunefinder.catalog $(RELEASE_DIR)/$(AMINET_NAME)/Catalogs/italiano/ 

	# Create archive
	cd $(RELEASE_DIR) && lha -ag0o5 ../$(AMINET_NAME).lha .
	$(info Aminet release created: $(AMINET_NAME).lha)

catalogs:
	$(info Generating translation catalogs...)
	$(FLEXCAT) $(CD_FILE) assets/translation/polski/tunefinder.ct CATALOG assets/translation/polski/tunefinder.catalog
	$(FLEXCAT) $(CD_FILE) assets/translation/deutsch/tunefinder.ct CATALOG assets/translation/deutsch/tunefinder.catalog
	$(FLEXCAT) $(CD_FILE) assets/translation/italiano/tunefinder.ct CATALOG assets/translation/italiano/tunefinder.catalog || true

aminet-clean:
	$(info Cleaning Aminet release files...)
	$(RM) -rf $(RELEASE_DIR)
	$(RM) -f $(AMINET_NAME).lha

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
	@mkdir -p $(BUILDDIR)/amigaamp
	@mkdir -p $(BUILDDIR)/locale
	@mkdir -p $(BUILDDIR)/network
	@mkdir -p $(BUILDDIR)/data
	@mkdir -p $(BUILDDIR)/utils
	@mkdir -p $(BUILDDIR)/settings
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
	@$(RM) assets/translation/polski/tunefinder.catalog
	@$(RM) assets/translation/deutsch/tunefinder.catalog
	@$(RM) assets/translation/italiano/tunefinder.catalog

debug-print:
	@echo "Source Directory: $(SRCDIR)"
	@echo "Build Directory: $(BUILDDIR)"
	@echo "Object Files: $(OBJECTS)"
	@echo "CCFLAGS: $(CCFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"

-include $(OBJECTS:.o=.d)
