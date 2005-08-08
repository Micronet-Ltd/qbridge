/*******************************************************************/
/*                                                                 */
/* File:  str712.h                                                 */
/*                                                                 */
/* Description: Header for ST STR712 processor hardware.           */
/*                                                                 */
/*******************************************************************/
/*      This program, in any form or on any media, may not be      */
/*        used for any purpose without the express, written        */
/*                      consent of QSI Corp.                       */
/*                                                                 */
/*             (c) Copyright 2005 QSI Corporation                  */
/*                                                                 */
/*******************************************************************/
/* Release History                                                 */
/*                                                                 */
/*  Date        By           Rev     Description                   */
/*-----------------------------------------------------------------*/
/* 02 Aug 2005  MK Elwood    1.0     1st Release                   */
/*******************************************************************/

#ifndef	STR712_H
#define	STR712_H

/************************/
/* Memory Map Addresses */
/************************/

/* Flash ROM */
#define FLASH_PHYS_BASE	0x40000000

/* SRAM */
#define RAM_PHYS_BASE	0x20000000

/* Internal registers */
#define PRCCU_REG_BASE	0xa0000000
#define APB1_REG_BASE	0xc0000000
#define APB2_REG_BASE	0xe0000000


/*****************************/
/* STR712 internal registers */
/*****************************/

/*********/
/* PRCCU */
/*********/
#define RCCU_REG_BASE		(PRCCU_REG_BASE)

#define RCCU_CCR_OFFSET				0x00
#define RCCU_CFR_OFFSET				0x08
#define RCCU_PLL1CR_OFFSET			0x18
#define RCCU_PER_OFFSET				0x1c
#define RCCU_SMR_OFFSET				0x20

#ifndef _ASM_
typedef struct 
{
	volatile unsigned long ccr;
	volatile unsigned long pad1;
	volatile unsigned long cfr;
	volatile unsigned long pad2;
	volatile unsigned long pad3;
	volatile unsigned long pad4;
	volatile unsigned long pll1cr;
	volatile unsigned long per;
	volatile unsigned long smr;
} RCCUREGS;
#endif /* _ASM_ */


#define PCU_REG_BASE			(PRCCU_REG_BASE + 0x40)

#define PCU_MDIVR_OFFSET			0x00
#define PCU_PDIVR_OFFSET			0x04
#define PCU_RSTR_OFFSET				0x08
#define PCU_PLL2CR_OFFSET			0x0c
#define PCU_BOOTCR_OFFSET			0x10
#define PCU_PWRCR_OFFSET			0x14

#ifndef _ASM_
typedef struct 
{
	volatile unsigned long mdivr;
	volatile unsigned long pdivr;
	volatile unsigned long rstr;
	volatile unsigned long pll2cr;
	volatile unsigned long bootcr;
	volatile unsigned long pwrcr;
} PCUREGS;
#endif /* _ASM_ */


/*******/
/* APB */
/*******/

/*******/
/* I2C */
/*******/
#define I2C0_REG_BASE		(APB1_REG_BASE + 0x1000)
#define I2C1_REG_BASE		(APB1_REG_BASE + 0x2000)


/********/
/* UART */
/********/
#define UART0_REG_BASE		(APB1_REG_BASE + 0x4000)
#define UART1_REG_BASE		(APB1_REG_BASE + 0x5000)
#define UART2_REG_BASE		(APB1_REG_BASE + 0x6000)
#define UART3_REG_BASE		(APB1_REG_BASE + 0x7000)

enum UartInterrupts { 
	RxHalfFullIE		= 0x0100,
	TimeoutIdleIE		= 0x0080,
	TimeoutNotEmptyIE	= 0x0040,
	OverrunErrorIE		= 0x0020,
	FrameErrorIE		= 0x0010,
	ParityErrorIE		= 0x0008,
	TxHalfEmptyIE		= 0x0004,
	TxEmptyIE			= 0x0002,
	RxBufNotEmptyIE	= 0x0001,
};

enum UartStatusBits {
	TxFull				= 0x0200,
	RxHalfFull			= 0x0100,
	TimeoutIdle			= 0x0080,
	TimeoutNotEmtpy	= 0x0040,
	OverrunError		= 0x0020,
	FrameError			= 0x0010,
	ParityError			= 0x0008,
	TxHalfEmpty			= 0x0004,
	TxEmpty				= 0x0002,
	RxBufNotEmtpy		= 0x0001,
};

typedef struct _UARTRegisterMap {
	volatile UINT32 BaudRate;
	volatile UINT32 txBuffer;
	volatile UINT32 rxBuffer;
	volatile UINT32 portSettings;
	volatile UINT32 intEnable;
	volatile UINT32 status;
	volatile UINT32 guardTime;
	volatile UINT32 timeout;
	volatile UINT32 txReset;
	volatile UINT32 rxReset;
} UARTRegisterMap;

typedef union _UARTSettingsMap {
	UINT32 value;
	struct {
		UINT32 mode:3;
		UINT32 stopBits:2;
		UINT32 parityOdd:1;
		UINT32 loopBack:1;
		UINT32 run:1;
		UINT32 rxEnable:1;
		UINT32 reserved1:1;
		UINT32 fifoEnable:1;
		UINT32 reserved2:21;
	};
} UARTSettingsMap;

/*******/
/* USB */
/*******/

/*******/
/* CAN */
/*******/

/********/
/* BSPI */
/********/

/********/
/* HDLC */
/********/

/*******/
/* XTI */
/*******/

/********/
/* GPIO */
/********/

#define IOPORT0_REG_BASE (APB2_REG_BASE + 0x3000)
#define IOPORT1_REG_BASE (APB2_REG_BASE + 0x4000)
#define IOPORT2_REG_BASE (APB2_REG_BASE + 0x5000)

typedef struct _IOPortRegisterMap {
	volatile UINT32 PC0;
	volatile UINT32 PC1;
	volatile UINT32 PC2;
	volatile UINT32 PD;
} IOPortRegisterMap;

/*******/
/* ADC */
/*******/

/*********/
/* TIMER */
/*********/

/*******/
/* RTC */
/*******/

/*******/
/* WDG */
/*******/

/*******/
/* EIC */
/*******/


#endif /* STR712_H */
