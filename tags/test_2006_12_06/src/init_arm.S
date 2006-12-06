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
   MOV   R1,#0                  //EIC controller is bit 14, but let's reset everybody!
   SUB   R1,R1,#1               //this gives us 0xffff
   STR   R1,[R0]                //do a software reset on the EIC
   MOV   R1,#0
   STR   R1,[R0]                //take him out of reset
   ldr   R0,=APB1_REG_BASE+0x14 //APB1_SWRES at offset 0x14
   MOV   R1,#0                  //Reset all the UARTS
   SUB   R1,R1,#1               //this gives us 0xffff
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



@*********************************************************************************************************
@                                          START J1708 TRANSMIT
@                                      bool StartJ1708Transmit(Byte data)
@
@  This code is called to start a J1708 Transmit operation.  According to the J1708 specification a device
@  must ensure the bus is idle within 1/2 bit time of starting a transmit.  Our device (when writting to
@  the serial transmit fifo) has one bit of timing uncertainty if you just asynchronously write to the
@  fifo to get it started.  What we do here is attempt to "sync up" to the "baud clock".  Here we assume
@  (based on a fair amount of testing) that there is a timing relationship between the "baud clock" and
@  timer0 (with a carefully chosen clock prescaler).  We watch the value of this timer and then at the
@  specified time, we start the serial transmit.  In order to meet the timings, I'm making
@  LOTS of assumptions here.
@
@  Measured times are as follows:
@  target count:                              0  |  1 |  2  |  3 |  4  |  5 |  6  |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | 15 |
@  uS before transmission actually begins:   71.5| 65 | 58.5| 52 | 45.5| 39 | 32.5| 26 |19.5| 13 | 6.5| 0  | 98 | 91 | 84 | 78 |
@
@  We implement a state machine here so we don't have to site around with interrupts disabled for a long time
@  and so we can sample the bus for busy/idle immediately before starting transfer and still meet timing
@
@  at entry: R0 has the data byte to be transmitted
@  at exit:  R0 = 0 if we failed to transmit (due to bus not idle)
@            R0 = 1 if we were were able to write the txBuffer while the bus was idle
@      interrupts are disabled if we were successful, otherwise they are in the same state as at entry
@*********************************************************************************************************
   .text
StartJ1708Transmit:
   .global StartJ1708Transmit

    stmdb sp!,{r4,r5,r6,lr}
    ldr r1,=TIMER0_REG_BASE+COUNTER_OFFSET          //TestTimer.timer->Counter
    ldr r2,=UART1_REG_BASE+TXBUFFER_OFFSET          //j1708Port->port->txBuffer
    ldr r3,=IOPORT1_REG_BASE+GPIO_PORT_DATA_OFFSET  //J1708_BUS_ASSERTED IOPORT1_REG_BASE->PD

    ldrh r4,[r1]    //sync up to the baud clock
bcsync1:
    ldrh r5,[r1]    // ||
    cmp r4,r5       //by waiting until we see a change in value
    beq bcsync1
    mov  r4,r5
    ands r5,r5,#0x0f//running same rate as our baud clock but that is 16x over sampled
    cmp  r5,#8      //so we have to wait for it to be on our desired bit
    bne bcsync1
    //Interrupts go off now, may miss our event and have to wait whole bit, but hope not
    mrs r6,CPSR     //DISABLE_IRQ( ...but note that we don't save the current state
    ORR r5,r6,#0xc0
    msr cpsr_c,r5
bcsync2:
    ldrh r5,[r1]    //sync to baud clock again now that interrupts are disabled
    cmp r4,r5       //by waiting until we see a change in value
    beq bcsync2
    mov  r4,r5
    ands r5,r5,#0x0f//running same rate as our baud clock but that is 16x over sampled
    cmp  r5,#9      //so we have to wait for it to be on our desired bit
    bne bcsync2
    //check for unhandeled edge of bus having occured before we got interrupts off and sync'd up
    ldr r5,=TIMER1_REG_BASE+STATUS_OFFSET //UNHANDELED_BUS_TRANSITION() (MainTimer.timer->StatusRegister & (InputCaptureFlagA | InputCaptureFlagB) )
    ldrh r5,[r5]
    tst r5,#INPUT_CAPTURE_A_OR_B
    bne failcode
bcsync3:
    ldrh r5,[r1]    //sync to baud clock again
    cmp r4,r5       //by waiting until we see a change in value
    beq bcsync3
    mov  r4,r5
    ands r5,r5,#0x0f//running same rate as our baud clock but that is 16x over sampled
    cmp  r5,#10     //so we have to wait for it to be on our desired bit
    bne bcsync3
    ldrh r1,[r3]    //now that we are close.... one last ditch check of bus asserted status
    tst r1,#P1_4_MASK //Bit 4 of GPIO1 = P1.4 = T1.ICAPA = J1708RX
    beq failcode    //if( J1708_BUS_ASSERTED() ) return FALSE

    strh r0,[r2]    //start the transmit now (this will give us 10 bit times to do everything else)
//#ifdef GPIO1p7_PULSE_ON_J1708_TX_BEGIN
//    //if test trigger needed, use GPIO 1.7
//    ldrh r1,[r3]
//    orr  r2,r1,#0x80    //set a trigger bit so we can see it (GPIO_SET(1,7))
//    strh r2,[r3]
//    strh r1,[r3]        //clear the trigger bit (GPIO_CLR(1,7))
//    //else, not test    //do we need to take same amount of time???, no because critical work has been done
//#endif

    //Yeah!!! We did it!!!
    //return TRUE
    //msr cpsr_c,r6 //interrupts are restored in upper level!
    mov r0,#1
    ldmia sp!,{r4,r5,r6,pc}
failcode:
    //return FALSE
    msr cpsr_c,r6
    mov r0,#0
    ldmia sp!,{r4,r5,r6,pc}

   .ltorg
@ End of StartJ1708Transmit

@*********************************************************************************************************
@                                     START J1708 BAUD GENERATOR AND TIMER
@                                      void StartJ1708BaudGeneratorAndTimer( void )
@
@  This code is called to start the J1708 com port baud rate generator and a timer that is used
@  to sync the firmware with the baudrate generator "simultaneously"
@  This is required so that we can get the actual transmission to begin
@  with known/controlled timing.
@  LOTS of assumptions here.
@*********************************************************************************************************
StartJ1708BaudGeneratorAndTimer:
   .global StartJ1708BaudGeneratorAndTimer

    stmdb sp!,{r0,r1,r2,r3,r4,lr}

    ldr r0,=TIMER0_REG_BASE+0x014   //TestTimer.timer->ControlRegister1
    ldr r1,=UART1_REG_BASE+0x0c     //j1708Port->port->portSettings
    ldrh r2,[r0]    //r2 now has timer control
    ldrh r3,[r1]    //r3 now has baud rate control (uart control reg)
    orr  r2,r2,#0x8000   //TimerCountEnable
    orr  r3,r3,#0x80     //map.run
    strh r2,[r0]    //start the timer
    mov r0,r0
    mov r0,r0
    mov r0,r0
    mov r0,r0
    mov r0,r0
    mov r0,r0
    mov r0,r0
    strh r3,[r1]    //start the baud rate generator

    ldmia sp!,{r0,r1,r2,r3,r4,pc}

   .ltorg
@ End of StartJ1708BaudGeneratorAndTimer
