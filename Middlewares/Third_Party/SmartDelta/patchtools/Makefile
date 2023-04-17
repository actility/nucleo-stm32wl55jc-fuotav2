
##############################################################################################
SRCROOT = ./src
FUOTA_PATCH_LIB_PATH = ../src

SRC_COMN += $(SRCROOT)/fota_misc.c
SRC_COMN += $(FUOTA_PATCH_LIB_PATH)/fota_patch_hash.c

SRC_DIFF += $(SRCROOT)/fota-pack_main.c
SRC_DIFF += $(SRCROOT)/lzg_e/encode.c
SRC_DIFF += $(SRCROOT)/Cheedoong-bsdiff/bsdiff32.c

SRC_ADD  += $(SRCROOT)/fota-unpack_main.c
SRC_ADD  += $(SRCROOT)/fota_patch.c
SRC_ADD  += $(FUOTA_PATCH_LIB_PATH)/fota_patch_stream.c
SRC_ADD  += $(FUOTA_PATCH_LIB_PATH)/fota_decompress_lzg.c
SRC_ADD  += $(FUOTA_PATCH_LIB_PATH)/fota_bspatch32.c

INC      += $(SRCROOT)
INC      += $(FUOTA_PATCH_LIB_PATH)
INC      += $(SRCROOT)/lzg_e
INC      += $(SRCROOT)/Cheedoong-bsdiff

##############################################################################################

#The '-r' or '--no-builtin-rules' for make causes the default list of suffixes to be empty it is a same .SUFFIXES:
.SUFFIXES:  # Delete the default suffixes (need for make 4.0)

CFLAGS += -pipe -g -Wall -Wextra
CFLAGS += -mno-ms-bitfields # for MinGW

LDFLAGS += #-Wl,-subsystem,console #-mthreads

CFLAGS  += -DGIT_DESCRIPTION=\"$(GIT_DESCRIPTION)\"
#DEFS   += -DCOMPILE_DATE=\"$(COMPILE_DATE)\"
#DEFS   += -DPROJECT_NAME=\"$(PROJECT)\"

# Generate dependency information:
# EITHER without system includes
CFLAGS += -MMD -MP -MF $(DEPDIR)/$(@F:.o=.d)
# OR     with system includes
#CFLAGS += -MD -MP -MF $(DEPDIR)/$(@F:.o=.d)

##############################################################################################


#OBJECTDIR = ./obj
#DEPDIR    = ./dep
OBJECTDIR = $(SRCROOT)/../obj/obj
DEPDIR    = $(SRCROOT)/../obj/dep
OUTDIR    = $(SRCROOT)/../out

INCLUDES = $(addprefix -I,$(abspath $(INC)))

SRCF_DIFF   = $(abspath $(SRC_COMN) $(SRC_DIFF))
SRCF_ADD    = $(abspath $(SRC_COMN) $(SRC_ADD))
OBJSF_DIFF  = $(SRCF_DIFF:.c=.o)
OBJSF_ADD   = $(SRCF_ADD:.c=.o)
OBJS_DIFF   = $(addprefix $(OBJECTDIR)/,$(notdir $(OBJSF_DIFF)))
OBJS_ADD    = $(addprefix $(OBJECTDIR)/,$(notdir $(OBJSF_ADD)))

#vpath %.c $(sort $(dir $(SRC_C)))   # here 'sort' removes duplicates

ifeq (,$(findstring $(MAKECMDGOALS),clean))
  $(info GIT describing... )
#  GIT_REVISION := ${shell git describe --dirty=+ --long | sed 's:[^-]*-::g' }
  GIT_DESCRIPTION := ${shell $(SRCROOT)/gitdescribe }

  GIT_DESC_FILE = ./.git_desc~
  GIT_DESC_OLD := $(shell [ -f $(GIT_DESC_FILE) ] && cat $(GIT_DESC_FILE))
  ifneq ($(GIT_DESC_OLD),$(GIT_DESCRIPTION))
    ${shell echo $(GIT_DESCRIPTION) > $(GIT_DESC_FILE)}
  else
    # leave $(GIT_DESC_FILE) untouched
  endif

  spc:=#
  $(info $(spc)            GIT description: $(GIT_DESCRIPTION))
  COMPILE_DATE := ${shell date +%d.%m.%Y }
  $(info $(spc)            date: $(COMPILE_DATE))
  #$(info $(shell $(CC) --version ))
endif

# makefile rules #############################################################################

.PHONY: all clean

all: $(OUTDIR)/fota-pack $(OUTDIR)/fota-unpack
	@echo GIT description: $(GIT_DESCRIPTION)

$(OUTDIR)/fota-pack: $(OBJS_DIFF) | $(OUTDIR) #$(LDSCRIPT)
	gcc $^ $(LDFLAGS) -o $@

$(OUTDIR)/fota-unpack: $(OBJS_ADD) | $(OUTDIR) #$(LDSCRIPT)
	gcc $^ $(LDFLAGS) -o $@

### create rules for each *.c file
define COMPILE_C
$(OBJECTDIR)/$(notdir $(1:.c=.o)): $(1) Makefile | $$(OBJECTDIR) $$(DEPDIR) #$$(LDSCRIPT)
	gcc -c $$(CFLAGS) $$(INCLUDES) $$< -o $$@
endef
$(foreach cfile, $(abspath $(SRC_COMN) $(SRC_DIFF) $(SRC_ADD)), $(eval $(call COMPILE_C,$(cfile))) )
###

$(OBJECTDIR):
	@mkdir -p $(OBJECTDIR)

$(DEPDIR):
	@mkdir -p $(DEPDIR)

$(OUTDIR):
	@mkdir -p $(OUTDIR)

clean:
	@#rm -f $(SRC:.c=.c.bak)
	@#rm -f $(ASRC:.s=.s.bak)
	rm -f $(DEPDIR)/* $(DEPDIR)/.d
	rm -f $(OBJECTDIR)/*
	rm -f $(OUTDIR)/*

#-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)
-include $(wildcard $(DEPDIR)/*)

# *** EOF ***
