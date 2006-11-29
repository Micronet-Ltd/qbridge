@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@                                                                  @
@  File:  init_arm.S                                               @
@                                                                  @
@  Description:  Initialization code for STR712 Qbridge processor  @
@                (ARM7TDMI core)                                   @
@                                                                  @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#define _ASM_

#include "stddefs.h"
#include "basearm.h"
#include "str712.h"

   .section .vectors,"ax"
vectable:

   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   @@@@ Exception vector table @@@@
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

   @ For debug, must use long calls for exception vectors
   LDR   pc, =Reset_Handler  @ 0x00
   LDR   pc, =Undefined_Handler  @ 0x04
   LDR   pc, =SWI_Handler    @ 0x08
   LDR   pc, =Prefetch_Handler   @ 0x0c
   LDR   pc, =Abort_Handler  @ 0x10
   NOP               @ Reserved vector
   LDR   pc, [pc, #-0x808]   @ 0x18 instruction to invoke VIC for IRQ
FIQ_Handler:
   LDR   pc, =irq_lockup     @ 0x1c

   @ FIQ code can go here if desired

vector_literals:
   .ltorg

   .section .arminit,"ax"

@ These are the qbridge interrupt handlers (mostly map to irq_lockup)
Undefined_Handler:
   B     irq_lockup

SWI_Handler:
   B     irq_lockup

Prefetch_Handler:
   B     irq_lockup

Abort_Handler:
   B     irq_lockup

   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   @@@@ Initialization Code @@@@
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   .global __start
__start:
Reset_Handler:
   @ Clock setup is done in InitializeClocks() routine of bootloader

   @ Interrupts must be disabled on Reset_Handler entry!!

   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   @@@@ Vector Initialization @@@@
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

init_vectors:
   LDR   r1, =_vectorRAMBegin
   LDR   r2, =_vectorRAMEnd
   LDR   r3, =_vectorROM
   CMP   r2, r1
   BEQ   map_sram_to_zero
vector_cpy:
   LDR   r0, [r3], #4
   STR   r0, [r1], #4
   CMP   r2, r1
   BNE   vector_cpy

map_sram_to_zero:
   LDR   r1, =PCU_REG_BASE
   LDR   r2, [r1, #PCU_BOOTCR_OFFSET]
   ORR   r2, r2, #0x2       @ Should be safe since we will most likely enter in flash or sram mode
   STR   r2, [r1, #PCU_BOOTCR_OFFSET]


   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   @@@@ Set up temporary stack in IRAM @@@@
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

init_stacks:
   @ Since all exceptions but one (the Rx receive) end up
   @ locking the machine, all exception modes should be
   @ safe to use the same stack.
   @ Jeremy, change this as necessary.

   @ Make sure interrupts are disabled in all modes
   MOV   r1, #(ARM_IRQ | ARM_FIQ)

   @ Set up IRQ mode stack
   ORR   r0, r1, #ARM_MODE_IRQ
   MSR   CPSR_cxsf, r0
   LDR   sp, =_irqstackend

   @ Set up FIQ mode stack
   ORR   r0, r1, #ARM_MODE_FIQ
   MSR   CPSR_cxsf, r0
   LDR   sp, =_irqstackend

   @ Set up Abort mode stack
   ORR   r0, r1, #ARM_MODE_ABT
   MSR   CPSR_cxsf, r0
   LDR   sp, =_irqstackend

   @ Set up Undefined mode stack
   ORR   r0, r1, #ARM_MODE_UND
   MSR   CPSR_cxsf, r0
   LDR   sp, =_irqstackend

   @ Set up Supervisor mode stack
   ORR   r0, r1, #ARM_MODE_SVC
   MSR   CPSR_cxsf, r0
   LDR   sp, =_irqstackend

   @ Set up the System mode stack (run in System mode from here on)
   MSR   CPSR_c, #(ARM_MODE_SYS | ARM_IRQ | ARM_FIQ)
   LDR   sp, =_stackend

   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   @@@@ Fill Stack with known values @@@@
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
fill_stacks:
   @ Fill the user and IRQ stacks with a known value to help detect overruns
   @ NOTE:  This routine clobbers r0-r6 and r12!!
   LDR   r1, =_stackbegin
   LDR   r2, =_irqstackend @ The IRQ stack immediately follows the main stack
   LDR   r0, =0xAABBCCDD
   MOV   r4, r0
   MOV   r5, r0
   MOV   r6, r0

   @ Fill stack with the static value
fill_sysstack:
   STMIA  r1!, {r0, r4, r5, r6}
   CMP   r2, r1
   BNE   fill_sysstack



   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   @@@@ Initialize sections @@@@
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@

//#ifndef RVDEBUG
   @ Copy the data section from flash to RAM
   @ _dataRAMBegin and _dataRAMEnd are should both be 4 byte aligned
   @ FIXME: optimize for block moves?
init_data:
   LDR   r1, =_dataRAMBegin
   LDR   r2, =_dataRAMEnd
   LDR   r3, =_dataROM
   CMP   r2, r1
   BEQ   init_bss
data_cpy:
   LDR   r0, [r3], #4
   STR   r0, [r1], #4
   CMP   r2, r1
   BNE   data_cpy
//#endif /* RVDEBUG */

   @ Clear the bss section in RAM
init_bss:
   MOV   r0, #0
   LDR   r1, =_bss_start
   LDR   r2, =_bss_end
   CMP   r2, r1
   BEQ   init_bss_done
bss_cpy:
   STR   r0, [r1], #4
   CMP   r2, r1
   BNE   bss_cpy
init_bss_done:


   @ Enable Interrupts
   //however, since this might be a 'warm' boot, clear the interrupt sources until fw has a chance to set them up
   ldr   R0,=APB2_REG_BASE+0x14 //APB2_SWRES at offset 0x14
   MOV   R1,#(1<<14)            //EIC controller is bit 14
   STR   R1,[R0]                //do a software reset on the EIC
   MOV   R1,#0
   STR   R1,[R0]                //take him out of reset
   MSR   CPSR_c, #(ARM_MODE_SYS)

   @ Now jump to main (use link in case main returns)
jump_to_main:
   BL    main

   @ NO CODE BELOW THIS JUMP IS EXECUTED UNLESS MAIN RETURNS!

   @ End of init code

Init_literals:
   .ltorg
