##################################################
#       Makefile for Qbridge Bootloader          #
#       MKE - Oct 2005                           #
##################################################

#############
#  Defines  #
#############

version	        := V0.002
test_version    := 
versdash        := -

target	  	:= qbboot

linkscript 	:= $(target)_rel.x

ARCH            := arm
COMPILER_PREFIX := arm-elf-
CFLAGS          := -mcpu=arm7tdmi -mlittle-endian -mapcs-32 -Wall -fno-strict-aliasing -fno-builtin -g
OPTIMIZE        := -Os
IFLAGS          := -Isrc -I../common -I../newlib/arm-elf/include

LIBPATH         := -L../newlib/arm-elf
LIBINCLUDES     := -lc -lgcc

SEC_EXCLUDES    := -R .bss -R .stack

version 	:= $(addsuffix $(test_version),$(version))
defs 		+= -DVERSION=\"$(version)\"

targettxt       := (ROM image)


######################
#  Tool Definitions  #
######################

CC              := $(COMPILER_PREFIX)gcc
OBJCPY          := $(COMPILER_PREFIX)objcopy
RM              := rm -rf

##################
#  Object Files  #
##################

SOBJS 		:= $(patsubst src/%.S,obj/%.o,$(wildcard src/*.S))
SSRCS		:= $(wildcard src/*.S)
SDEPENDS	:= $(patsubst src/%.S,dpn/%.d,$(wildcard src/*.S))

COBJS 		:= $(patsubst src/%.c,obj/%.o,$(wildcard src/*.c))
CSRCS		:= $(wildcard src/*.c)
CDEPENDS	:= $(patsubst src/%.c,dpn/%.d,$(wildcard src/*.c))

OBJS        	:= $(SOBJS) $(COBJS)
SRCS		:= $(CSRCS) $(SSRCS)

######################
#  Built In Targets  #
######################

HOUSEKEEPGOALS  := clean realclean listsrc listdep listobj doecho showoptions tags
.PHONY: all $(HOUSEKEEPGOALS)
.SUFFIXES:

####################
#  DEFAULT TARGET  #
####################

all : doecho $(target).bin

####################################################
#  REALVIEW DEBUG TARGET (execute from SRAM only)  #
####################################################

rvdebug: linkscript := $(target)_deb.x
rvdebug: defs += -DRVDEBUG
rvdebug: targettxt := (RVDEBUG image)
rvdebug: doecho $(target).bin

###################
#  Pattern Rules  #
###################

$(CDEPENDS): dpn/%.d: src/%.c
	@$(CC) -MM $(IFLAGS) $(defs) $< | sed 's,$*\.o,obj/& $@,g' > $@

$(SDEPENDS): dpn/%.d: src/%.S
	@$(CC) -MM $(IFLAGS) $(defs) $< | sed 's,$*\.o,obj/& $@,g' > $@

$(COBJS): obj/%.o : src/%.c dpn/%.d
	@echo -e '\E[32m'"\033[1m$<\033[0m"
	$(CC) $(DEBUG_FLAGS) $(OPTIMIZE) $(CFLAGS) $(IFLAGS) $(defs) -Wa,-ahld=lst/$*.lst -c -o $@ $< 

$(SOBJS): obj/%.o : src/%.S dpn/%.d
	@echo -e '\E[32m'"\033[1m$<\033[0m"
	$(CC) $(DEBUG_FLAGS) $(OPTIMIZE) $(CFLAGS) $(IFLAGS) $(defs) -Wa,-ahld=lst/$*.lst -c -o $@ $< 

##########
#  Link  #
##########

$(target).srec: $(target).elf
	$(OBJCPY) -v -S $(SEC_EXCLUDES) -O symbolsrec $(target).elf $(target).srec

$(target).bin: $(target).srec
	$(OBJCPY) -v -I symbolsrec -O binary $(target).srec $(target).bin
	@echo -e '\E[36m'"\033[1m**************Target Made**************\033[0m"

$(target).elf: $(OBJS) $(linkscript)
	@echo -e '\E[32m'"\033[1mLinking $@\033[0m"
	$(CC) $(CFLAGS) $(OBJS) -Xlinker -Map -Xlinker $(target).map --warn-common \
		-T$(linkscript) -nostdlib -static $(LIBPATH) $(LIBINCLUDES) -o $(target).elf

##################
#  Dependencies  #
##################

ifeq ($(MAKECMDGOALS),)
  include $(SDEPENDS) $(CDEPENDS)
endif
ifneq ($(strip $(foreach var, $(MAKECMDGOALS), $(filter $(var), $(HOUSEKEEPGOALS)))), $(strip $(MAKECMDGOALS)))
  include $(SDEPENDS) $(CDEPENDS)
endif

#######################
#  HOUSEKEEPING/MISC  #
#######################

clean:
	@echo -e '\E[37m'"\033[1m#################################################"
	@echo -e '### Cleaning all generated files'
	@echo -e '#################################################'"\033[0m"
	@$(RM) obj/*.o lst/*.lst *.elf *.bin *.map *.srec TAGS

realclean: clean
	@echo '#################################################'
	@echo '### Cleaning dependencies'
	@echo '#################################################'
	@$(RM) dpn/*.d

listsrc:
	@echo 'Ssrcs are: $(SSRCS)'
	@echo 'Csrcs are: $(CSRCS)'

listdep:
	@echo 'Sdepends are: $(SDEPENDS)'
	@echo 'Cdepends are: $(CDEPENDS)'

listobj:
	@echo 'Sobjs are: $(SOBJS)'
	@echo 'Cobjs are: $(COBJS)'
	@echo 'Objs are: $(OBJS)'

tags:
	@etags src/*.c src/*.h ../common/*.h

doecho:
	@echo -e '\E[37m'"\033[1m#################################################"
	@echo -e '### Building $(target) $(targettxt)'
	@echo -e '#################################################'"\033[0m"
	@echo
	@$(RM) obj/build.o #This throws in the current date and time every time.
	@echo $(MAKECMDGOALS)
