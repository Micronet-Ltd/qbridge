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

   .section .vectors,"ax"
vectable:

   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   @@@@ Exception vector table @@@@
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

   @ For debug, must use long calls for exception vectors
   LDR	 pc, =Reset_Handler	 @ 0x00
   LDR	 pc, =Undefined_Handler  @ 0x04
   LDR	 pc, =SWI_Handler	 @ 0x08
   LDR	 pc, =Prefetch_Handler   @ 0x0c
   LDR	 pc, =Abort_Handler	 @ 0x10
   NOP				 @ Reserved vector
   LDR	 pc, [pc, #-0x808] 	 @ 0x18 instruction to invoke VIC for IRQ
FIQ_Handler:   
   LDR	 pc, =irq_lockup	 @ 0x1c   

   @ FIQ code can go here if desired

vector_literals:
   .ltorg

   .section .arminit,"ax"

@ These are the qbridge interrupt handlers (mostly map to irq_lockup)
Undefined_Handler:
   B	 irq_lockup

SWI_Handler:
   B	 irq_lockup

Prefetch_Handler:
   B	 irq_lockup

Abort_Handler:
   B	 irq_lockup
   
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   @@@@ Initialization Code @@@@
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   .global __start
__start:
Reset_Handler:
   @ Clock setup is done in InitializeClocks() routine
   
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   @@@@ Memory Initialization @@@@
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   
   @ Initialize the static memory controller (if necessary)

   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   @@@@ Set up temporary stack in IRAM @@@@
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

init_stacks:
   @ Since all exceptions but one (the Rx receive) end up
   @ locking the machine, all exception modes should be
   @ safe to use the same stack.
   @ Jeremy, change this as necessary.
   
   @ Make sure interrupts are disabled in all modes
   MOV	 r1, #(ARM_IRQ | ARM_FIQ)

   @ Set up IRQ mode stack
   ORR	 r0, r1, #ARM_MODE_IRQ
   MSR	 CPSR_cxsf, r0
   LDR	 sp, =_irqstackend

   @ Set up FIQ mode stack
   ORR	 r0, r1, #ARM_MODE_FIQ
   MSR	 CPSR_cxsf, r0
   LDR	 sp, =_irqstackend

   @ Set up Abort mode stack
   ORR	 r0, r1, #ARM_MODE_ABT
   MSR	 CPSR_cxsf, r0
   LDR	 sp, =_irqstackend
   
   @ Set up Undefined mode stack
   ORR	 r0, r1, #ARM_MODE_UND
   MSR	 CPSR_cxsf, r0
   LDR	 sp, =_irqstackend

   @ Set up Supervisor mode stack
   ORR	 r0, r1, #ARM_MODE_SVC
   MSR	 CPSR_cxsf, r0
   LDR	 sp, =_irqstackend
   
   @ Set up the System mode stack (run in System mode from here on)
   MSR	 CPSR_c, #(ARM_MODE_SYS | ARM_IRQ | ARM_FIQ)
   LDR	 sp, =_stackend
   
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   @@@@ Fill Stack with known values @@@@
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
fill_stacks:
   @ Fill the user and IRQ stacks with a known value to help detect overruns
   @ NOTE:  This routine clobbers r0-r6 and r12!!
   LDR	 r1, =_stackbegin
   LDR	 r2, =_irqstackend @ The IRQ stack immediately follows the main stack
   LDR	 r0, =0xAABBCCDD
   MOV	 r4, r0
   MOV	 r5, r0
   MOV	 r6, r0
   
   @ Fill stack with the static value
fill_sysstack:  
   STMIA  r1!, {r0, r4, r5, r6}
   CMP	 r2, r1
   BNE	 fill_sysstack
   
   

   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   @@@@ Initialize sections @@@@
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#ifndef RVDEBUG
   @ Copy the data section from flash to RAM
   @ _dataRAMBegin and _dataRAMEnd are should both be 4 byte aligned
   @ FIXME: optimize for block moves?
init_data:  
   LDR	 r1, =_dataRAMBegin
   LDR	 r2, =_dataRAMEnd
   LDR	 r3, =_dataROM
   CMP	 r2, r1
   BEQ	 init_bss
data_cpy:
   LDR	 r0, [r3], #4
   STR	 r0, [r1], #4
   CMP	 r2, r1
   BNE	 data_cpy
#endif /* RVDEBUG */
      
   @ Clear the bss section in RAM
init_bss:
   MOV	 r0, #0
   LDR	 r1, =_bss_start
   LDR	 r2, =_bss_end
   CMP	 r2, r1
   BEQ	 init_bss_done
bss_cpy:
   STR	 r0, [r1], #4
   CMP	 r2, r1
   BNE	 bss_cpy
init_bss_done:


   @ Enable Interrupts
   MSR	 CPSR_c, #(ARM_MODE_SYS)

   @ Now jump to main (use link in case main returns)
jump_to_main:  
   BL	 main

   @ NO CODE BELOW THIS JUMP IS EXECUTED UNLESS MAIN RETURNS!

   @ End of init code

Init_literals:
   .ltorg

   
   .text

#if 0
@@@@@@@@@@@@@@@@@
@@@@ delayMS @@@@   
@@@@@@@@@@@@@@@@@
delayMS:
   .global delayMS
   @ Uses Timer1 in LH79520 for the delay
   @ Assumes HCLK is 52 MHz
   @ r0 contains the desired delay in msec

   @ Calculate delay*(52000/prescale 256) =~ delay*202
   MOV	 r1, #202
   MUL	 r1, r0, r1
   MOVS	 r2, r1, lsr #15  @ Number of timer half-periods
   MOV	 r1, r1, lsl #17
   MOV	 r1, r1, lsr #17  @ Fractional half-period (last cycle)
   LDR	 r0, =TIMER1_REG_BASE

   @ Now set up the timer for the full half-period (if required)
1:
   BEQ	 3f
   LDR	 r3, =0x7fff      @ Load value for entire half-period
   STR	 r3, [r0, #TIMER_LOAD_OFFSET]
   MOV	 r3, #0x88   @ Enable timer, free running, prescale by 256
   STR	 r3, [r0, #TIMER_CONTROL_OFFSET]

   @ Timer setup for half-period, now watch for underflow
2:
   LDR	 r3, [r0, #TIMER_VALUE_OFFSET]
   TST	 r3, #0x8000
   BEQ	 2b

   @ Disable timer, decrement half-periods count, and loop
   MOV	 r3, #0
   STR	 r3, [r0, #TIMER_CONTROL_OFFSET]
   SUBS	 r2, r2, #1
   B	 1b
  
   @ Now do the last (fractional) timer count 
3:
   STR	 r1, [r0, #TIMER_LOAD_OFFSET]
   MOV	 r3, #0x88   @ Enable timer, free running, prescale by 256
   STR	 r3, [r0, #TIMER_CONTROL_OFFSET]

   @ Timer setup for half-period, now watch for underflow
4:
   LDR	 r3, [r0, #TIMER_VALUE_OFFSET]
   TST	 r3, #0x8000
   BEQ	 4b

   @ Disable timer and return
   MOV	 r3, #0
   STR	 r3, [r0, #TIMER_CONTROL_OFFSET]
   MOV	 pc, lr
@ End of delayMS
      
@@@@@@@@@@@@@@@@@
@@@@ delayUS @@@@   
@@@@@@@@@@@@@@@@@
delayUS:
   .global delayUS
   @ Uses Timer1 in LH79520 for the delay
   @ Assumes HCLK is 52 MHz
   @ r0 contains the desired delay in usec

   @ Calculate (delay*52)/prescale (16 in this case) - lower 15 bits
   MOV	 r1, #52
   MUL	 r1, r0, r1
   MOV	 r1, r1, lsl #13
   MOV	 r1, r1, lsr #17
   LDR	 r0, =TIMER1_REG_BASE
   STR	 r1, [r0, #TIMER_LOAD_OFFSET]
   MOV	 r1, #0x84    @ Enable timer, free running, prescale by 16
   STR	 r1, [r0, #TIMER_CONTROL_OFFSET]

   @ Timer is set up, now watch for underflow
1:
   LDR	 r1, [r0, #TIMER_VALUE_OFFSET]
   TST	 r1, #0x8000
   BEQ	 1b

   @ if underflow, disable timer and return
   MOV	 r1, #0
   STR	 r1, [r0, #TIMER_CONTROL_OFFSET]
   MOV	 pc, lr
@ End of delayUS

ASM_literals:   
   .ltorg

#endif /* #if 0 */
