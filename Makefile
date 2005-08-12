##################################################
#       Makefile for Qbridge                     #
#       MKE - Aug 2005                           #
##################################################

#############
#  Defines  #
#############

version	        := V0.001
test_version    := 
versdash        := -

target	  	:= qbridge

linkscript 	:= $(target)_deb.x

ARCH            := arm
COMPILER_PREFIX := arm-elf-
CFLAGS          := -mcpu=arm7tdmi -mlittle-endian -mapcs-32 -Wa,-ahld -Wall -fno-strict-aliasing -fno-builtin -g
OPTIMIZE        := 
IFLAGS          := -Isrc -I/usr/local/arm-elf/include

LIBPATH         := -L/usr/local/arm-elf/lib -L/usr/local/lib/gcc-lib/arm-elf/3.3.1
LIBINCLUDES     := -lc -lgcc

SEC_EXCLUDES    := -R .bss -R .stack

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

###################
#  Pattern Rules  #
###################

$(CDEPENDS): dpn/%.d: src/%.c
	@$(CC) -MM $(IFLAGS) $(defs) $< | sed 's,$*\.o,obj/& $@,g' > $@

$(SDEPENDS): dpn/%.d: src/%.S
	@$(CC) -MM $(IFLAGS) $(defs) $< | sed 's,$*\.o,obj/& $@,g' > $@

$(COBJS): obj/%.o : src/%.c dpn/%.d
	$(CC) $(DEBUG_FLAGS) $(OPTIMIZE) $(CFLAGS) $(IFLAGS) $(defs) -c -o $@ $< > lst/$*.lst

$(SOBJS): obj/%.o : src/%.S dpn/%.d
	$(CC) $(DEBUG_FLAGS) $(OPTIMIZE) $(CFLAGS) $(IFLAGS) $(defs) -c -o $@ $< > lst/$*.lst

##########
#  Link  #
##########

$(target).srec: $(target).elf
	$(OBJCPY) -v -S $(SEC_EXCLUDES) --gap-fill=51 -O symbolsrec $(target).elf $(target).srec

$(target).bin: $(target).srec
	$(OBJCPY) -v -I symbolsrec -O binary $(target).srec $(target).bin

$(target).elf: $(OBJS) $(linkscript)
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
	@echo '#################################################'
	@echo '### Cleaning all generated files'
	@echo '#################################################'
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
	@etags src/*.c src/*.h

doecho:
	@echo '#################################################'
	@echo '### Building $(target)'
	@echo '#################################################'
	@echo
	@$(RM) obj/build.o #This throws in the current date and time every time.
	@echo $(MAKECMDGOALS)