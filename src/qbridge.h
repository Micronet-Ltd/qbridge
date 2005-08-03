/*******************************************************************/
/*                                                                 */
/* File:  g55arch.h                                                */
/*                                                                 */
/* Description:  Support for the G55 architecture.                 */
/*                                                                 */
/*******************************************************************/
/*      This program, in any form or on any media, may not be      */
/*        used for any purpose without the express, written        */
/*                      consent of QSI Corp.                       */
/*                                                                 */
/*             (c) Copyright 2003 QSI Corporation                  */
/*******************************************************************/
/* Release History                                                 */
/*                                                                 */
/*  Date        By           Rev     Description                   */
/*-----------------------------------------------------------------*/
/* 26-Aug-03    MKE          1.0     1st Release                   */
/*******************************************************************/

#ifndef G55ARCH_H
#define G55ARCH_H

/*****************************/
/* Standard Library Includes */
/*****************************/

/*******************************/
/* Programmer Library Includes */
/*******************************/

/***********/
/* Pragmas */
/***********/

/***********/
/* Defines */
/***********/
#define CPU_TYPE "Sharp LH79520"
#define CPU_FREQ "77414400"

#define OS_TICK_RATE 50

/* System clocks */
#define FCLK		   77414400L
#define HCLK			51609600L

/**********************************/
/* Register Initialization Values */
/**********************************/
/* 
 * IOCON MemMux value for 1 bank of SDRAM, 16 bit data bus, 
 * no BLE2 or BLE3, all chip selects active.
 */
#define IOCON_MEMMUX_INIT	0x00000f6d

/*
 * SMCBCR0 value for boot flash device:
 * 16 bits wide, 2 write wait states, 3 read wait states,
 * no idle cycle waits, nWE enabled
 */
#define SMCBCR0_INIT			0x10001460

/*
 * SDRAM Config0 value for 128 Mbit SDRAM:
 * Auto-Precharge, R2C latency=2, CAS latency=2,
 * 16 bit bus, continuous clock, 4-bank x16 128 Mbit device
 */
#define SDRC_CONFIG0_INIT	0x01ac0008

/* SDRAM Config1 value for 128 Mbit SDRAM: enable read & write buffers */
#define SDRC_CONFIG1_INIT	0x0000000c

/* SDRAM Refresh count = 784 (0x310) to meet 4096 refreshed every 64 ms */
#define SDRC_REFRESH_INIT	0x310

/*
 * Program word for the SDRAM internal mode registers
 * Burst Length - 8  (A2:A0 = 0b011)
 * Burst Type - Sequential (A3 = 0)
 * CAS Latency - 2 (A6:A4 = 0x010)
 * Operating Mode - Standard (A8:A7 = 0)
 * Write Burst Mode - Programmed Burst Length (A9 = 0)
 * On 8Mx16 SDRAM, address bits 21-12 map to SDRAM row address 9-0
 */
#define SDRAM_MODEREG_INIT	0x23000

/************************/
/* Interrupt Priorities */
/************************/
#define VIC_PRIO_POWERKEY	1
#define VIC_PRIO_DMA			3
#define VIC_PRIO_SSPROR    4
#define VIC_PRIO_SSP       5
#define VIC_PRIO_TSC       6
#define VIC_PRIO_DAV       7
#define VIC_PRIO_RTX0		8
#define VIC_PRIO_RTX1		9
#define VIC_PRIO_RTX2		10
#define VIC_PRIO_ETHERNET	11
#define VIC_PRIO_RTC			14
#define VIC_PRIO_SYSTIMER	15 /* lowest priority */

/************/
/* TypeDefs */
/************/

/**********/
/* Macros */
/**********/
/* Determines of the unit is a Z60. */
#define IS_Z60()    (((~(hwparms->Config) & Z60_ARCH) == Z60_ARCH)? TRUE:FALSE)
#define HAS_KEYPAD_BACKLIGHT()    (((hwparms->Config & G55_KEYPAD_BACKLIGHT) == 0)? TRUE:FALSE)

#define RESET() do {	RCPCREGS *rcpc = (RCPCREGS *)RCPC_REG_BASE; rcpc->softreset = RCPC_SOFTRESET_ALL; } while(0)

/* Nothing to synchronize for sharp - it is unified */
#define SYNC_CACHE() 

#define OS_CRITICAL_ROUTINE register UINT32 __temp_psr

#define OS_ENTER_CRITICAL() asm volatile ("MRS %0, CPSR\nORR ip, %0, #0xc0\nMSR CPSR_c, ip\n" : "=r" (__temp_psr) : : "ip", "memory", "cc");
#define OS_EXIT_CRITICAL()	 asm volatile ("MSR CPSR_c, %0\n" : :"r" (__temp_psr) : "memory", "cc");

#define OS_TASK_SW() asm volatile ("SWI #0\n")

/* Macros to enable and disable the hardware (for power up/down) */
#define DISABLE_VIDEO() { \
	CLCDCREGS *clcdc = (CLCDCREGS *)CLCDC_REG_BASE; \
	GPIOREGS *portg = (GPIOREGS *)GPIOG_REG_BASE;   \
	/* Disable LCD Power Supply */						\
	portg->dr &= ~2;											\
	delayMS(20);												\
	/* Disable the video */									\
	clcdc->lcdctrl &= ~(CLCDC_LCDCTRL_ENABLE | CLCDC_LCDCTRL_PWR); }

#define ENABLE_VIDEO() { \
	CLCDCREGS *clcdc = (CLCDCREGS *)CLCDC_REG_BASE; \
	GPIOREGS *portg = (GPIOREGS *)GPIOG_REG_BASE;   \
	/* Enable the video */									\
	clcdc->lcdctrl |= CLCDC_LCDCTRL_ENABLE;			\
	delayMS(20);												\
	/* Enable LCD Power Supply */							\
	portg->dr |= 2;											\
	clcdc->lcdctrl |= CLCDC_LCDCTRL_PWR; }

#define DELAY_MS(x) delayMS(x)

/**************/
/* Prototypes */
/**************/
#ifndef _ASM_
/* These function live in except.c - at some point they should be placed somewhere more approriate */
enum irq_sense { IRQSENSE_LOW, IRQSENSE_HIGH, IRQSENSE_FALLING, IRQSENSE_RISING };
void VICInit(void);
int RegisterVICHdlr(VIC_SOURCE src, void (*hdlr)(void), unsigned int priority);
int RegisterVICExtHdlr(VIC_SOURCE src, void (*hdlr)(void), unsigned int priority, enum irq_sense sense);
void VICDisableIRQ(VIC_SOURCE src);
void VICEnableIRQ(VIC_SOURCE src);
int addBottomHdlr(void (*hdlr)());

/* These functions live in init.S */
void delayMS(unsigned long);
void delayUS(unsigned long);
#endif

/***********************************************************************/
/* System Memory Map Definitions (defined in the linker scripts [*.x]) */
/***********************************************************************/
#ifndef _ASM_
extern unsigned char _RamStartAddr[];
extern unsigned char _RamEndAddr[];

extern unsigned char _RomStartAddr[];
extern unsigned char _bootFlashStartAddr[];

extern unsigned char _IRamStartAddr[];
extern unsigned char _IRamCodeAddr[];
extern unsigned char _IRamDataAddr[];
extern unsigned char _TTAddr[];

/* Heap addresses */
extern unsigned char _heapBegin[];
extern unsigned char _end[];
#endif /* _ASM */

/************************/
/* System RAM addresses */
/************************/
/* This is the largest possible area of DRAM that we use (hopefully, this stays correct :))*/
#define MaxRamSize				  0x07ffffff

/* Address of where rammable kernels are stored */
#define ramkrnlStartAddr		  (_RamStartAddr + 0x120000)

/* 
 * The bootflag is used to induce the bootloader from the firmware.
 * It is hardcoded at a location in internal SRAM
 * This should not interfere with either the bootloader or the firmware
 * unless a memory test destroys the bootflag (currently does not).
 */
#ifndef _ASM_
#define SET_BOOTFLAG(x)    ((*(volatile unsigned long *)(_IRamStartAddr + 0x3ff8)) = (x))
#define GET_BOOTFLAG()		(*(volatile unsigned long *)(_IRamStartAddr + 0x3ff8)) /* was IRAMStartAddr */
#endif /* _ASM_ */
#define BOOTFLAG_ENTER_BL	0xa9a99a9a
#define BOOTFLAG_ENTER_APP 0x9a9aa9a9
#define BOOTFLAG_TOUCH_CAL 0x55AA55AA
#define BOOTFLAG_ENTER_POS 0xAA55AA55

#define IS_IN_RAM(ptr)		(((cacheSeg(ptr) >= cacheSeg(_RamStartAddr)) && \
									  (cacheSeg(ptr) < cacheSeg(_RamStartAddr + MaxRamSize)))? 1 : 0)

#define noCacheSeg(addr) (((unsigned long) (addr)) | 0x08000000)
#define cacheSeg(addr)	 (((unsigned long) (addr)) & 0xf7ffffff)

/**********************/
/* Flash declarations */
/**********************/
/* These block sizes are WORD block size (2-bytes)! */
#define PARM_BLK_WSIZE			0x1000  /* = 0x1000 W = 0x2000 B = 8kB */
#define MAIN_BLK_WSIZE			0x8000  /* = 0x8000 W = 0x10000 B = 64kB */

/* Note: These have to start and end on flash block boundaries */
#define bootFlashLen				0x8000		/* 32K for bootloader */

#define parmBlockStartAddr		(_bootFlashStartAddr + bootFlashLen)
#define parmBlockLen				0x4000		/* 16K for normal parameters */

#define userBlockStartAddr		(parmBlockStartAddr + parmBlockLen)
#define userBlockLen				0x2000		/* 8K for user block */

#define HWParmBlockStartAddr	(userBlockStartAddr + userBlockLen)
#define HWParmBlockLen			0x2000		/* 8K for hardware parameters */

/* Kernel is expected to not be in parameter block section */
#define krnlFlashStartAddr		(HWParmBlockStartAddr + HWParmBlockLen)

/*********************************/
/* Peripheral Hardware Addresses */
/*********************************/
/* Define here for now - consider moving them to the linker scripts */

#define IOBASE_CS1		0x31000000
#define IOBASE_CS2		0x32000000
#define IOBASE_CS3		0x33000000
#define IOBASE_CS4		0x34000000
#define IOBASE_CS5		0x35000000
#define IOBASE_CS6		0x36000000

#define IOBASE_ETHERNET IOBASE_CS2
#define IOBASE_KEYLATCH IOBASE_CS5

#define ContrastLatch	(*(volatile unsigned char *)IOBASE_CS4)
#define LEDLatch			(*(volatile unsigned char *)IOBASE_CS6)

/* Location of XR16L788 Octal UART used in C008 - accessed through nCS1 */
#define XR16L78X_REGBASE        ((volatile unsigned char *) IOBASE_CS1)

/*************************/
/* CRC table definitions */
/*************************/
#define CRC_TBL_ADDR	krnlFlashStartAddr
#define CRC_TBL_LEN	16

#define crcROMMagicL 0x35354721	/* "!G55"	(little endian) */
#define crcROMMagicB 0x21473535	/* "!G55"	(big endian) */
#ifndef _ASM_
/*
 * NOTE: the size and definition of this structure must be compatible with CRC.EXE
 * NOTE: make sure the 'C' structure matches the _ASM_ defines
 */
/* WARNING: This section is fixed in size and link location. DO NOT MODIFY!! */
typedef struct {
	unsigned long magic;		/* Magic string 'CrC!' */
	unsigned long codeLen;
	unsigned short crc;
	unsigned short dummy1;
	unsigned long	dummy2;
} crcROMHdrDefn;
#else	/* _ASM_ */
#define	crcHdr_magic	0
#define	crcHdr_codeLen	4
#define	crcHdr_crc		8
#define	crcHdr_dummy1	10
#define	crcHdr_dummy2	12
#endif	/* _ASM_ */

/****************************/
/* Vector table definitions */
/****************************/
#define VECT_TBL_ADDR	(krnlFlashStartAddr+CRC_TBL_LEN)
#define VECT_TBL_LEN		32
#define vectTblMagicL 	0x21517249	/* "IrQ!" (little endian) */
#define vectTblMagicB 	0x49725121	/* "IrQ!" (big endian) */
#ifndef _ASM_
/* NOTE: make sure the 'C' structure matches the _ASM_ defines */
/* WARNING: This section is fixed in size and link location. DO NOT MODIFY!! */
typedef struct {
	unsigned long magic;		/* Magic string 'IrQ!' */
	unsigned long UTLB_exception;
	unsigned long debug_exception;
	unsigned long Other_exception;
	unsigned long entryPoint;	/* Code entry point */
	unsigned long compress[3];
} vectTblDefn;
#else	/* _ASM_ */
#define vTbl_magic				0
#define vTbl_UTLB_exception	4
#define vTbl_debug_exception	8
#define vTbl_Other_exception	12
#define vTbl_entryPoint			16
#define vTbl_textCompress		20
#define vTbl_dataCompress		24
#define vTbl_posCompress		28
#endif	/* _ASM_ */

/*********************************/
/* Compression Header definition */
/*********************************/
#define cmpMagicValB	0x436d507a	/* big endian */
#define cmpMagicValL	0x7a506d43	/* little endian */
#ifndef _ASM_
typedef struct {
	unsigned long magic;	/* identifier value */
	unsigned long startAddr;
	unsigned long endAddr;
	unsigned long entryPoint;
	unsigned long dataOffset;	/* where compressed data starts in this file */
} cmpFileHdrDefn;
#endif	/* #ifndef _ASM_ */

#endif /* G55ARCH_H */
