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

#ifndef _ASM_
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
#endif /* _ASM_ */

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
#define XTI_REG_BASE     (APB2_REG_BASE + 0x1000)

#define XTI_SR_OFFSET    0x1c
#define XTI_CTRL_OFFSET  0x24
#define XTI_MRH_OFFSET   0x28
#define XTI_MRL_OFFSET   0x2c
#define XTI_TRH_OFFSET   0x30
#define XTI_TRL_OFFSET   0x34
#define XTI_PRH_OFFSET   0x38
#define XTI_PRL_OFFSET   0x3c

#ifndef _ASM_
typedef struct {
	volatile unsigned long pad1[7];
	volatile unsigned long sr;   /* 0x1c */
	volatile unsigned long pad2;
	volatile unsigned long ctrl; /* 0x24 */
	volatile unsigned long mrh;
	volatile unsigned long mrl;
	volatile unsigned long trh;
	volatile unsigned long trl;
	volatile unsigned long prh;
	volatile unsigned long prl;
} XTIREGS;

typedef enum {
	XTI_SW,          /* S/W interrupt */
	XTI_USB,         /* USB wake-up event */
	XTI_PORT2_8,     /* Port 2.8 */
	XTI_PORT2_9,     /* Port 2.9 */
	XTI_PORT2_10,    /* Port 2.10 */
	XTI_PORT2_11,    /* Port 2.11 */
	XTI_PORT1_11,    /* Port 1.11 */
	XTI_PORT1_13,    /* Port 1.13 */
	XTI_PORT1_14,    /* Port 1.14 */
	XTI_PORT0_1,     /* Port 0.1 */
	XTI_PORT0_2,     /* Port 0.2 */
	XTI_PORT0_6,     /* Port 0.6 */
	XTI_PORT0_8,     /* Port 0.8 */
	XTI_PORT0_10,    /* Port 0.10 */
	XTI_PORT0_13,    /* Port 0.13 */
	XTI_PORT0_15     /* Port 0.15 */
} XTI_SOURCE;

enum irq_sense { IRQ_FALLING, IRQ_RISING };
#endif /* _ASM_ */

/********/
/* GPIO */
/********/

#define IOPORT0_REG_BASE (APB2_REG_BASE + 0x3000)
#define IOPORT1_REG_BASE (APB2_REG_BASE + 0x4000)
#define IOPORT2_REG_BASE (APB2_REG_BASE + 0x5000)

#ifndef _ASM_
typedef struct _IOPortRegisterMap {
	volatile UINT32 PC0;
	volatile UINT32 PC1;
	volatile UINT32 PC2;
	volatile UINT32 PD;
} IOPortRegisterMap;
#endif /* _ASM_ */

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
#define EIC_REG_BASE		(0xfffff800)

#define EIC_ICR_OFFSET   0x00
#define EIC_CICR_OFFSET  0x04
#define EIC_CIPR_OFFSET  0x08
#define EIC_IVR_OFFSET   0x18
#define EIC_FIR_OFFSET   0x1C
#define EIC_IER_OFFSET   0x20
#define EIC_IPR_OFFSET   0x40
#define EIC_SIR_OFFSET   0x60

#ifndef _ASM_
typedef struct {
	volatile unsigned long icr;
	volatile unsigned long cicr;
	volatile unsigned long cipr;
	volatile unsigned long pad1[3];
	volatile unsigned long ivr;     /* 0x18 */
	volatile unsigned long fir;
	volatile unsigned long ier;
	volatile unsigned long pad2[7];
	volatile unsigned long ipr;     /* 0x40 */
	volatile unsigned long pad3[7];
	volatile unsigned long sir[32]; /* 0x60 */
} EICREGS;

/* STR712 interrupt sources */
typedef enum {
	EIC_TIMER0,      /* Timer 0 */
	EIC_FLASH,       /* Flash */
	EIC_PRCCU,       /* PRCCU */
	EIC_RTC,         /* Real Time Clock */
	EIC_WDG,         /* Watchdog Timer */
	EIC_XTI,         /* External interrupt */
	EIC_USB_HP,      /* USB high priority */
	EIC_I2C0ERR,     /* I2C 0 error */
	EIC_I2C1ERR,     /* I2C 1 error */
	EIC_UART0,       /* UART 0 */
	EIC_UART1,       /* UART 1 */
	EIC_UART2,       /* UART 2 */
	EIC_UART3,       /* UART 3 */
	EIC_SPI0,        /* SPI 0 */
	EIC_SPI1,        /* SPI 1 */
	EIC_I2C0,        /* I2C 0 Rx/Tx */
	EIC_I2C1,        /* I2C 1 Rx/Tx */
	EIC_CAN,         /* CAN */
	EIC_ADC,         /* ADC */
	EIC_TIMER1,      /* Timer 1 */
	EIC_TIMER2,      /* Timer 2 */
	EIC_TIMER3,      /* Timer 3 */
	EIC_RESERVED1,   /* Reserved */
	EIC_RESERVED2,   /* Reserved */
	EIC_RESERVED3,   /* Reserved */
	EIC_HDLC,        /* HDLC */
	EIC_USB_LP,      /* USB low priority */
	EIC_RESERVED4,   /* Reserved */
	EIC_RESERVED5,   /* Reserved */
	EIC_TIMER0_OVF,  /* Timer 0 overflow */
	EIC_TIMER0_OC1,  /* Timer 0 output compare 1 */
	EIC_TIMER0_OC2   /* Timer 0 output compare 2 */
} EIC_SOURCE;
#endif /* _ASM_ */

#define IRQ_EN      0x1
#define FIQ_EN      0x2


#endif /* STR712_H */
