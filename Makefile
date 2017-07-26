##################################################
#       Makefile for Qbridge                     #
#       MKE - Aug 2005                           #
##################################################

#############
#  Defines  #
#############

version	        := V1.D12
test_version    := b
versdash        := -

target	  	:= qbridge

linkscript 	:= $(target).x

ARCH            := arm
COMPILER_PREFIX := arm-elf-
CFLAGS          := -mcpu=arm7tdmi -mlittle-endian -Wall -Wcast-align -Wno-pointer-sign -fno-strict-aliasing -fno-builtin -g
OPTIMIZE        := -Os
IFLAGS          := -Isrc -Icommon -I./newlib/arm-elf/include

LIBPATH         := -L./newlib/arm-elf
LIBINCLUDES     := -lc -lgcc

SEC_EXCLUDES    := -R .bss -R .stack

version 	:= $(addsuffix $(test_version),$(version))
defs 		+= -DVERSION=\"$(version)\"

targettxt       := (ROM image)


######################
#  Tool Definitions  #
######################

CC              := $(COMPILER_PREFIX)gcc-4.1.1
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

HOUSEKEEPGOALS  := clean realclean listsrc listdep listobj doecho showoptions tags objdir lstdir
.PHONY: all $(HOUSEKEEPGOALS)
.SUFFIXES:

####################
#  DEFAULT TARGET  #
####################

all : doecho objdir lstdir $(target).bin

####################################################
#  REALVIEW DEBUG TARGET (execute from SRAM only)  #
####################################################

rvdebug: linkscript := $(target)_deb.x
rvdebug: defs += -DRVDEBUG
rvdebug: targettxt := (RVDEBUG image)
rvdebug: doecho $(target).bin

##########################################
# TARGET WITH MODEM RESET (see QCO #796) #
# After 13 Aug 2012, always define		 #
# MODEM_RESET in main.c . No need here   #
##########################################

modemreset: defs += -DMODEM_RESET
modemreset: targettxt := (MODEMRESET image)
modemreset: doecho objdir lstdir $(target).bin

###################
#  Pattern Rules  #
###################

$(CDEPENDS): dpn/%.d: src/%.c
	@$(CC) -MM $(IFLAGS) $(defs) $< | sed 's,$*\.o,obj/& $@,g' > $@

$(SDEPENDS): dpn/%.d: src/%.S
	@$(CC) -MM $(IFLAGS) $(defs) $< | sed 's,$*\.o,obj/& $@,g' > $@

$(COBJS): obj/%.o : src/%.c dpn/%.d	Makefile
	@echo -e '\E[32m'"\033[1m$<\033[0m"
	$(CC) $(DEBUG_FLAGS) $(OPTIMIZE) $(CFLAGS) $(IFLAGS) $(defs) -Wa,-ahld=lst/$*.lst -c -o $@ $<

$(SOBJS): obj/%.o : src/%.S dpn/%.d Makefile
	@echo -e '\E[32m'"\033[1m$<\033[0m"
	$(CC) $(DEBUG_FLAGS) $(OPTIMIZE) $(CFLAGS) $(IFLAGS) $(defs) -Wa,-ahld=lst/$*.lst -c -o $@ $<

tools/sreccrc: tools/sreccrc.c Makefile
	@echo -e '\E[34m'"\033[1mBuilding helper tool $@\033[0m"
	@gcc -m32 -o tools/sreccrc tools/sreccrc.c

tools/crc: tools/crc.c Makefile
	@echo -e '\E[34m'"\033[1mBuilding helper tool $@\033[0m"
	@gcc -m32 -o tools/crc tools/crc.c

tools/srec2hex: tools/srec2hex.c Makefile
	@echo -e '\E[34m'"\033[1mBuilding helper tool $@\033[0m"
	@gcc -m32 -o tools/srec2hex tools/srec2hex.c

bootloader/qbboot.srec:
	$(MAKE) -C ./bootloader

.DELETE_ON_ERROR:


##########
#  Link  #
##########

$(target).srec: $(target).elf tools/sreccrc
	$(OBJCPY) -v -S $(SEC_EXCLUDES) --gap-fill=51 -O symbolsrec $(target).elf $(target).srec
	@tools/sreccrc $(target).srec

$(target).bin: $(target).srec tools/crc
	$(OBJCPY) -v -I symbolsrec -O binary $(target).srec $(target).bin
	@tools/crc -l -b$(target).bin:0
	@echo -e '\E[36m'"\033[1m**************$(target).bin made successfully**************\033[0m"

$(target).elf: $(OBJS) $(linkscript)
	@echo -e '\E[32m'"\033[1mLinking $@\033[0m"
	$(CC) $(CFLAGS) $(OBJS) -Xlinker -Map -Xlinker $(target).map --warn-common \
		-T$(linkscript) -nostdlib -static $(LIBPATH) $(LIBINCLUDES) -o $(target).elf

flashimage.srec: bootloader/qbboot.srec $(target).srec
	cat bootloader/qbboot.srec $(target).srec > flashimage.srec

flashimage.hex: flashimage.srec tools/srec2hex
	tools/srec2hex flashimage.srec flashimage.hex
	@echo -e '\E[34m'"\033[1m**************flashimage.hex made successfully**************\033[0m"

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
	@$(RM) obj/*.o lst/*.lst *.elf *.bin *.map *.srec *.hex TAGS
	@$(RM) tools/sreccrc tools/crc tools/srec2hex

realclean: clean
	@echo '#################################################'
	@echo '### Cleaning dependencies'
	@echo '#################################################'
	@$(RM) dpn/*.d

sweepclean: realclean
	@$(MAKE) -C ./bootloader realclean


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
	@etags src/*.c src/*.h

objdir:
	@if test -d obj; \
	then \
		:; \
	else \
		mkdir obj; \
		:>obj/.cvsignore; \
	fi

lstdir:
	@if test -d lst; \
	then \
		:; \
	else \
		mkdir lst; \
		:>lst/.cvsignore; \
	fi

doecho:
	@echo -e '\E[37m'"\033[1m#################################################"
	@echo -e '### Building $(target) $(targettxt)'
	@echo -e '#################################################'"\033[0m"
	@echo
	@$(RM) obj/build.o #This throws in the current date and time every time.
	@echo $(MAKECMDGOALS)
