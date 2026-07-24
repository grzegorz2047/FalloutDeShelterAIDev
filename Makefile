#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET      := DeepShelter3D
BUILD       := build
SOURCES     := source
DATA        := data
INCLUDES    := include
ROMFS       := romfs

export APP_TITLE       := Deep Shelter 3D
export APP_DESCRIPTION := Original shelter-management game for Nintendo 3DS
export APP_AUTHOR      := grzegorz2047

TOOLS_BIN     ?= $(TOPDIR)/.tools/bin
MAKEROM       ?= $(TOOLS_BIN)/makerom
BANNERTOOL    ?= $(TOOLS_BIN)/bannertool
PYTHON        ?= python3
SOURCE_RSF    := $(TOPDIR)/config/application.rsf
ASSET_DIR     := $(TOPDIR)/$(BUILD)/release-assets
RSF_FILE      := $(ASSET_DIR)/application.rsf
ICON_FILE     := $(ASSET_DIR)/icon.png
BANNER_IMAGE  := $(ASSET_DIR)/banner.png
BANNER_AUDIO  := $(ASSET_DIR)/banner.wav
BANNER_FILE   := $(ASSET_DIR)/banner.bnr
CIA_ICON      := $(ASSET_DIR)/icon.icn

ARCH        := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS      := -g -Wall -Wextra -Werror -O2 -mword-relocations -ffunction-sections $(ARCH)
CFLAGS      += $(INCLUDE) -D__3DS__
CXXFLAGS    := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++17
ASFLAGS     := -g $(ARCH)
LDFLAGS     := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS        := -lcitro2d -lcitro3d -lctru -lm
LIBDIRS     := $(CTRULIB)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT  := $(CURDIR)/$(TARGET)
export TOPDIR  := $(CURDIR)
export VPATH   := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                  $(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES      := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES    := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES      := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PICAFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.v.pica)))
BINFILES    := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

export LD := $(CXX)
export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES_BIN := $(addsuffix .o,$(BINFILES)) $(PICAFILES:.v.pica=.shbin.o)
export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)
export HFILES := $(PICAFILES:.v.pica=_shbin.h) $(addsuffix .h,$(subst .,_,$(BINFILES)))
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export _3DSXDEPS := $(if $(NO_SMDH),,$(OUTPUT).smdh)
export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh --romfs=$(CURDIR)/$(ROMFS)
export MAKEROM BANNERTOOL PYTHON SOURCE_RSF RSF_FILE ASSET_DIR ICON_FILE BANNER_IMAGE BANNER_AUDIO BANNER_FILE CIA_ICON

.PHONY: all test 3dsx cia release tools assets clean checksums

all: 3dsx

test:
	@$(PYTHON) scripts/check_asset_policy.py
	@$(PYTHON) scripts/validate_content.py
	@$(PYTHON) tests/content_validator_tests.py
	@$(PYTHON) -m py_compile scripts/check_asset_policy.py scripts/generate_release_assets.py scripts/validate_content.py tests/content_validator_tests.py

3dsx: tools assets $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile 3dsx

cia: tools assets $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile cia

release:
	@$(MAKE) --no-print-directory 3dsx
	@$(MAKE) --no-print-directory cia
	@$(MAKE) --no-print-directory checksums

tools:
	@bash scripts/bootstrap_cia_tools.sh

assets: $(ICON_FILE) $(BANNER_IMAGE) $(BANNER_AUDIO) $(RSF_FILE)

$(ICON_FILE) $(BANNER_IMAGE) $(BANNER_AUDIO): scripts/generate_release_assets.py
	@$(PYTHON) scripts/generate_release_assets.py

$(RSF_FILE): $(SOURCE_RSF) scripts/prepare_release_rsf.sh
	@bash scripts/prepare_release_rsf.sh "$(SOURCE_RSF)" "$@"

$(BUILD):
	@mkdir -p $@

clean:
	@rm -rf $(BUILD) $(TARGET).3dsx $(TARGET).smdh $(TARGET).elf $(TARGET).map $(TARGET).cia dist

checksums:
	@test -s $(TARGET).3dsx
	@test -s $(TARGET).cia
	@mkdir -p dist
	@cp $(TARGET).3dsx $(TARGET).cia dist/
	@cd dist && sha256sum $(TARGET).3dsx $(TARGET).cia > SHA256SUMS.txt

else

.PHONY: all 3dsx cia

all: 3dsx
3dsx: $(OUTPUT).3dsx
cia: $(OUTPUT).cia

$(OUTPUT).3dsx: $(OUTPUT).elf $(_3DSXDEPS)
$(OFILES_SOURCES): $(HFILES)
$(OUTPUT).elf: $(OFILES)

$(OUTPUT).smdh: $(ICON_FILE)
	@$(BANNERTOOL) makesmdh -s "$(APP_TITLE)" -l "$(APP_DESCRIPTION)" -p "$(APP_AUTHOR)" -i "$<" -f visible,allow3d,recordusage -o "$@"
	@echo "built ... $(notdir $@)"

$(CIA_ICON): $(ICON_FILE)
	@$(BANNERTOOL) makesmdh -s "$(APP_TITLE)" -l "$(APP_DESCRIPTION)" -p "$(APP_AUTHOR)" -i "$<" -f visible,allow3d,recordusage -o "$@"

$(BANNER_FILE): $(BANNER_IMAGE) $(BANNER_AUDIO)
	@$(BANNERTOOL) makebanner -i "$(BANNER_IMAGE)" -a "$(BANNER_AUDIO)" -o "$@"

$(OUTPUT).cia: $(OUTPUT).elf $(CIA_ICON) $(BANNER_FILE) $(RSF_FILE)
	@cd "$(TOPDIR)" && "$(MAKEROM)" -f cia -target t -exefslogo -o "$(OUTPUT).cia" \
		-elf "$(OUTPUT).elf" -rsf "$(RSF_FILE)" \
		-banner "$(BANNER_FILE)" -icon "$(CIA_ICON)"
	@echo "built ... $(notdir $@)"

%.bin.o %_bin.h: %.bin
	@$(bin2o)

.PRECIOUS: %.shbin
%.shbin.o %_shbin.h: %.shbin
	@$(bin2o)

-include $(DEPSDIR)/*.d

endif
