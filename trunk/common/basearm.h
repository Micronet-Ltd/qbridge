/*******************************************************************/
/*                                                                 */
/* File:  basearm.h                                                */
/*                                                                 */
/* Description: Header for ARM core definitions.                   */
/*              (borrowed from Qlarity source code)                */
/*                                                                 */
/*******************************************************************/
/*      This program, in any form or on any media, may not be      */
/*        used for any purpose without the express, written        */
/*                      consent of QSI Corp.                       */
/*                                                                 */
/*             (c) Copyright 2004 QSI Corporation                  */
/*                                                                 */
/* Portions of this code are derived from code that is:            */
/* COPYRIGHT (C) 2002 SHARP MICROELECTRONICS OF THE AMERICAS, INC. */
/*      CAMAS, WA                                                  */
/*******************************************************************/
/* Release History                                                 */
/*                                                                 */
/*  Date        By           Rev     Description                   */
/*-----------------------------------------------------------------*/
/* 17 May 2004  RNM          1.0     1st Release                   */
/*******************************************************************/

#ifndef BASEARM_H
#define BASEARM_H

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
/* These macro's must be rammable - no functions calls! */
#define IRQSTATE UINT32
#define DISABLE_IRQ(saveState) { asm volatile ( \
                                     "\tMRS  r0, CPSR\n"        \
                                     "\tSTR  r0, %0\n"          \
                                     "\tORR  r0, r0, #0xc0\n" \
                                     "\tMSR  CPSR_c, r0\n" \
                                     : : "m" (saveState) : "r0", "memory" ); }
#define RESTORE_IRQ(saveState)  { asm volatile ( \
                                     "\tLDR  r0, %0\n"          \
                                     "\tMSR  CPSR_c, r0\n" \
                                     : : "m" (saveState) : "r0", "memory" ); }
#define IRQ_ENABLE()            { asm volatile ( \
                                     "\tMRS  r0, CPSR\n"        \
                                     "\tBIC  r0, r0, #0xc0\n" \
                                     "\tMSR  CPSR_c, r0\n" \
                                     : : : "r0" ); }

#define ARM_GET_CP15() ({ unsigned long cpval=0; \
          asm volatile ("MRC p15, 0, %0, c1, c0, 0" : "=r" (cpval) : ); cpval; })


#define ARCH_GET_PC() ({ unsigned long pc; asm volatile ( "MOV %[currpc], pc\n" : [currpc] "=g" (pc) : ); pc; })
#define ARCH_GET_SP() ({ unsigned long sp=0;  asm volatile("MOV %0, sp\n" : "=r" (sp) :); sp; })

#define noCacheSeg(addr) (((unsigned long) (addr)) | 0x08000000)
#define cacheSeg(addr)   (((unsigned long) (addr)) & 0xf7ffffff)

#define ARM_GET_PC() ({ unsigned long cpc=0; asm volatile("MOV %0, pc\n" : "=r" (cpc) :); cpc; })
#define ARM_GET_FP() ({ unsigned long fp=0;  asm volatile("MOV %0, fp\n" : "=r" (fp) :); fp; })
#define ARM_GET_LR() ({ unsigned long lr=0;  asm volatile("MOV %0, lr\n" : "=r" (lr) :); lr; })
#define ARM_GET_CPSR() ({ unsigned long cpsr=0; asm volatile("MRS %0, CPSR\n" : "=r" (cpsr) :); cpsr; })
#define ARM_GET_SPSR() ({ unsigned long spsr=0; asm volatile("MRS %0, SPSR\n" : "=r" (spsr) :); spsr; })
#define ARM_GET_FSR() ({ unsigned long fsr=0; asm volatile("MRC p15, 0, %0, c5, c0, 0\n" : "=r" (fsr) :); (fsr & 0x7ff); })
#define ARM_GET_FAR() ({ unsigned long far=0; asm volatile("MRC p15, 0, %0, c6, c0, 0\n" : "=r" (far) :); far; })

#define ARM_FLUSH_CACHE() asm volatile ("MCR     p15, 0, r1, c7,  c7, 0\n")

/*************************************/
/* ARM Hard Vector Address Locations */
/*************************************/
#define ARM_RESET_VEC   0x00
#define ARM_UNDEF_VEC   0x04
#define ARM_SWI_VEC     0x08
#define ARM_IABORT_VEC  0x0C
#define ARM_DABORT_VEC  0x10
#define ARM_IRQ_VEC     0x18
#define ARM_FIQ_VEC     0x1C

/***************************************************************/
/* ARM Current and Saved Processor Status Register Bits (xPSR) */
/***************************************************************/
/* ARM Condition Code Flag Bits (xPSR bits [31:27]) */
#define ARM_CCFLG_N     BIT(31)
#define ARM_CCFLG_Z     BIT(30)
#define ARM_CCFLG_C     BIT(29)
#define ARM_CCFLG_V     BIT(28)
#define ARM_CCFLG_Q     BIT(27)

/* ARM Interrupt Disable Bits (xPSR bits [7:6])  */
#define ARM_IRQ         BIT(7)
#define ARM_FIQ         BIT(6)

#define ARM_INTMASK     (ARM_IRQ | ARM_FIQ)

/* ARM Thumb State Bit (xPSR bit [5])  */
#define ARM_THUMB       BIT(5)

/*
 * ARM Processor Mode Values (xPSR bits [4:0])
 * Use ARM_MODE macro and constants to test ARM mode
 * Example, where tmp has xPSR value:
 * if (ARM_MODE(tmp, ARM_MODE_IRQ))
 *     statement;
 */
#define ARM_MODE_USR    0x10
#define ARM_MODE_FIQ    0x11
#define ARM_MODE_IRQ    0x12
#define ARM_MODE_SVC    0x13
#define ARM_MODE_ABT    0x17
#define ARM_MODE_UND    0x1B
#define ARM_MODE_SYS    0x1F

#define ARM_MODE_MASK   0x1F


/***********************************/
/* System Control Coprocessor CP15 */
/***********************************/

/* Valid CP15 registers */
/*#define CP15_REG_ID           c0 */
#define CP15_REG_CTRL           c1, c0, 0
#define CP15_REG_AUXCTRL        c1, c0, 1
#define CP15_REG_TTBASE         c2, c0, 0       /* Translation Table Base Address */
#define CP15_REG_DAC          c3, c0, 0     /* Domain Access Control */
/*#define CP15_REG_FSTAT        c5      * Fault Status */
/*#define CP15_REG_FADDR        c6      * Fault Address */
#define CP15_REG_CACHEOP(cx,op) c7, TOKEN_CONCAT(c,cx), op
#define CP15_REG_TLBOP(cx,op)       c8, TOKEN_CONCAT(c,cx), op      /* Translation Lookaside Buffer */
/*#define CP15_REG_FCSE_PID c13 * Fast Context Switch Extension */
#define CP15_REG_CACHELOCK(cx,op) c9, TOKEN_CONCAT(c,cx), op
#define CP15_REG_TLBLOCK(cx,op) c10, TOKEN_CONCAT(c,cx), op

/* Control register (c1) bits */
#define CP15_CTRL_MMU_BIT       BIT(0)  /* MMU Enable */
#define CP15_CTRL_AFAULT_BIT    BIT(1)  /* Alignment Fault */
#define CP15_CTRL_CACHE_BIT BIT(2)  /* Data Cache or Unified Cache (no Instruction Cache) */
#define CP15_CTRL_WB_BIT        BIT(3)  /* Write Buffer */
#define CP15_CTRL_SPROT_BIT BIT(8)  /* System Protection */
#define CP15_CTRL_RPROT_BIT BIT(9)  /* ROM Protection */
#define CP15_CTRL_BRANCH_BIT  BIT(11)  /* Branch prediction bit */
#define CP15_CTRL_ICACHE_BIT    BIT(12) /* Instruction Cache */
#define CP15_CTRL_HIGHVEC_BIT   BIT(13) /* Exception Vector location */

#define CP14_REG_CLKCFG         c6, c0, 0

/*
 * Tiny page size - 1KB
 * Small page size - 4KB
 * Large page size - 64KB
 * Section size - 1MB
 */
#define TINY_PAGE_SIZE (0x100 * 4)
#define SMALL_PAGE_SIZE (0x400 * 4)
#define LARGE_PAGE_SIZE (0x4000 * 4)
#define SECTION_SIZE        (0x40000 * 4)

/*
 * Small page tables and Large page tables are the same size,
 * 1024 bytes or 256 entries
 * Large page tables have each of 16 entries replicated 16 times
 * in succeeding memory location
 */
#define PAGE_TABLE_SIZE     (0x100 * 4)  /* 256 entries */
#define TRANS_TABLE_SIZE    (0x1000 * 4) /* 4096 entries */

/*
 * Translation table must be based on a 16KB boundary.
 * Page tables must be based on 1KB boundaries.
 */

/***********************/
/* Memory Manager Unit */
/***********************/

#define MMU_CONTROL_M           BIT(0)
#define MMU_CONTROL_A           BIT(1)
#define MMU_CONTROL_C           BIT(2)
#define MMU_CONTROL_W           BIT(3)
#define MMU_CONTROL_S           BIT(8)
#define MMU_CONTROL_R           BIT(9)
#define MMU_CONTROL_V           BIT(13)

#define MMU_CONTROL_FIELD_ENABLE    1
#define MMU_CONTROL_FIELD_DISABLE   0

/*********************/
/* Translation Table */
/*********************/
/* Number of entries in TT */
#define TT_ENTRIES  4096
#define TT_SIZE     (TT_ENTRIES * 4)

/**************/
/* Page Table */
/**************/
/* Number of entries in PT */
#define PT_ENTRIES  256
#define PT_SIZE     (PT_ENTRIES * 4)

/*
 * Level 1 Descriptor fields
 * L1D_x fields apply to both section and page descriptor,
 * where applicable
 */
#define L1D_TYPE_FAULT     0
#define L1D_TYPE_PAGE      0x11 /* includes compatibility bit 4 */
#define L1D_TYPE_SECTION   0x12 /* includes compatibility bit 4 */
#define L1D_BUFFERABLE     BIT(2)
#define L1D_CACHEABLE      BIT(3)
#define L1D_XCACHE         BIT(12)

/*
 * Section AP field meaning depends on CP15 Control Reg S and R bits
 * See LH79520 User's Guide
 */
#define L1D_AP_SVC_ONLY    0x40000000
#define L1D_AP_USR_RO      0x00000800
#define L1D_AP_ALL         0x00000c00

#define L1D_DOMAIN(n)           ((((n) & 0x0f)) << 5)
#define L1D_SEC_BASE_ADDR(n)  ((((n) & 0xfff)) << 20)

/*
 * Level 2 Descriptor fields
 * L2D_x fields apply to both large page and small page descriptors,
 * where applicable.
 */
#define L2D_TYPE_FAULT          0x00000000
#define L2D_TYPE_LARGE_PAGE     0x00000001
#define L2D_TYPE_SMALL_PAGE     0x00000002
#define L2D_BUFFERABLE          0x00000002
#define L2D_CACHEABLE           0x00000004
#define L2D_AP0_SVC_ONLY        0x00000010
#define L2D_AP0_USR_RO          0x00000020
#define L2D_AP0_ALL             0x00000030
#define L2D_AP1_SVC_ONLY        0x00000040
#define L2D_AP1_USR_RO          0x00000080
#define L2D_AP1_ALL             0x000000c0
#define L2D_AP2_SVC_ONLY        0x00000100
#define L2D_AP2_USR_RO          0x00000200
#define L2D_AP2_ALL             0x00000300
#define L2D_AP3_SVC_ONLY        0x00000400
#define L2D_AP3_USR_RO          0x00000800
#define L2D_AP3_ALL             0x00000c00
#define L2D_SPAGE_BASE_ADDR(n)  (((n) & 0xfffff) << 12)
#define L2D_LPAGE_BASE_ADDR(n)  (((n) & 0xffff) << 16)

/*
 * Domain Access Control Register Fields
 * There are 16 domains, 0 - 15
 */
#define MMU_DOMAIN_NONE     0
#define MMU_DOMAIN_CLIENT   1
#define MMU_DOMAIN_MANAGER  3

/* The following macros may be used to set Domain Access Control */
/* The range of argument 'n' is 0 -15 */
#define MMU_DOMAIN_NO_ACCESS(n)         (MMU_DOMAIN_NONE << ((n) * 2))
#define MMU_DOMAIN_CLIENT_ACCESS(n)     (MMU_DOMAIN_CLIENT << ((n) * 2))
#define MMU_DOMAIN_MANAGER_ACCESS(n)    (MMU_DOMAIN_MANAGER << ((n) * 2))

/* Fault Status Register Fields */
#define MMU_FSR_DOMAIN(n)   (((n) & 0xf0) >> 4)
#define MMU_FSR_TYPE(n)     ((n) & 0x0f)

/************/
/* TypeDefs */
/************/
#ifndef _ASM_
typedef struct {
        unsigned long vidx[TT_ENTRIES];
} TRANSTABLE;

typedef struct {
        unsigned long vidx[PT_ENTRIES];
} PAGETABLE;

/*
 * num_sections: number of sections >=1 for all blocks
 *                            except last; last = 0
 * virt_addr: as required, base Virtual address for block
 * phys_addr: as required, PT address or Section address
 * entry is composed of the following 'or'd' together:
 *  access_perm:  L1D_AP_x (x = SVC_ONLY, USR_RO, ALL)
 *  domain:   L1D_DOMAIN(n) as applicable
 *  cacheable:  L1D_CACHEABLE if applicable
 *  write_buffered:  L1D_BUFFERABLE if applicable
 *  descriptor_type: L1D_TYPE_x (x = FAULT, PAGE, SECTION)
 *
 */
typedef const struct {
    unsigned long num_sections;
    unsigned long virt_addr; /* calculate index from this */
    unsigned long phys_addr; /* initialize location @ index w/this */
    unsigned long entry;      /* 'or'd' combinations of entry settings;
                                          'or' this with phys_addr */
    /*
      * Entry settings:
      *  access_perm, domain, cacheable, write_buffered, descriptor_type
      */
} TT_SECTION_BLOCK;

typedef const struct {
    unsigned long num_sections;
    unsigned long virt_addr; /* calculate index from this */
    unsigned long phys_addr; /* initialize location @ index w/this */
    unsigned long entry;      /* 'or'd' combinations of entry settings;
                                          'or' this with phys_addr */
    /*
      * Entry settings:
      *  access_perm, domain, cacheable, write_buffered, descriptor_type
     */
} PT_PAGE_BLOCK;
#endif /* _ASM_ */

/**********/
/* Macros */
/**********/

/**************/
/* Prototypes */
/**************/

#endif /* BASEARM_H */
