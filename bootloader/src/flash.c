/*******************************************************************/
/*                                                                 */
/* File:  flash.c                                                  */
/*                                                                 */
/* Description:  Flash programming routines for STR712             */
/*                                                                 */
/*******************************************************************/
/*      This program, in any form or on any media, may not be      */
/*        used for any purpose without the express, written        */
/*                      consent of QSI Corp.                       */
/*                                                                 */
/*             (c) Copyright 2005 QSI Corporation                  */
/*******************************************************************/
/* Release History                                                 */
/*                                                                 */
/*  Date        By           Rev     Description                   */
/*-----------------------------------------------------------------*/
/* 04 Nov 2005  MK Elwood    1.0     First release                 */
/*******************************************************************/

/*****************************/
/* Standard Library Includes */
/*****************************/

/*******************************/
/* Programmer Library Includes */
/*******************************/
#include "stddefs.h"
#include "str712.h"

/***********/
/* Pragmas */
/***********/

/***********/
/* Defines */
/***********/

/************/
/* TypeDefs */
/************/

/**********/
/* Macros */
/**********/

/********************/
/* Global Variables */
/********************/

/************/
/* Routines */
/************/
/******************/
/* FlashWriteWord */
/******************/
static int FlashWriteWord(unsigned long addr, unsigned long data)
{
    FLASHREGS *flash = (FLASHREGS *)FLASH_REG_BASE;

    /* Must be word aligned */
    if (addr & 0x3) {
        return -1;
    }

    flash->cr0 |= FLASHCMD_WPG;
    flash->dr0 = data;
    flash->ar = addr & FLASH_ARMASK;
    flash->cr0 |= FLASHCMD_START;

    /* Just monitor both busy bits; one will always be low */
    while (flash->cr0 & FLASH_BSY) ;

    /* Let caller handle and clear errors */
    return flash->er & FLASH_ERMASK;
}

/************************/
/* FlashWriteDoubleWord */
/************************/
static int FlashWriteDoubleWord(unsigned long addr, unsigned long datalow, unsigned long datahigh)
{
    FLASHREGS *flash = (FLASHREGS *)FLASH_REG_BASE;

    /* Must be double word aligned */
    if (addr & 0x7) {
        return -1;
    }

    flash->cr0 |= FLASHCMD_DWPG;
    flash->dr0 = datalow;
    flash->dr1 = datahigh;
    flash->ar = addr & FLASH_ARMASK;
    flash->cr0 |= FLASHCMD_START;

    /* Just monitor both busy bits; one will always be low */
    while (flash->cr0 & FLASH_BSY) ;

    /* Let caller handle and clear errors */
    return flash->er & FLASH_ERMASK;
}

/************************/
/* FlashGetSectorNumber */
/************************/
static unsigned long FlashGetSectorNumber(unsigned long addr)
{
    unsigned long badAddr = 0xffffffff;

    /* Error checking is minimal here; assumes caller knows what it's doing */
    if (addr >> 24 != 0x40) {
        return badAddr;
    }

    addr &= FLASH_ARMASK;

    if (addr & BANK1_ADDR) {
        /* Address in Bank 1 */
        if (addr & BANK1F1) {
            return FLASH_B1F1;
        } else {
            return FLASH_B1F0;
        }
    } else {
        /* Address in Bank 0 */
        switch (addr >> 16) {
        case 0:
            /* In first 64k - smaller blocks */
            if (addr & 0x8000) {
                return FLASH_B0F4;
            } else {
                switch (addr >> 13) {
                case 0:
                    return FLASH_B0F0;
                case 1:
                    return FLASH_B0F1;
                case 2:
                    return FLASH_B0F2;
                case 3:
                    return FLASH_B0F3;
                }
            }
        case 1:
            return FLASH_B0F5;
        case 2:
            return FLASH_B0F6;
        case 3:
            return FLASH_B0F7;
        default:
            return badAddr;
        }
    }
}

/********************/
/* FlashEraseSector */
/********************/
int FlashEraseSector(unsigned long addr)
{
    FLASHREGS *flash = (FLASHREGS *)FLASH_REG_BASE;
    unsigned long sector;

    sector = FlashGetSectorNumber(addr);
    if (sector == 0xffffffff) {
        return -1;
    }

    flash->cr0 |= FLASHCMD_SER;
    flash->cr1 |= sector;
    flash->cr0 |= FLASHCMD_START;

    /* Just monitor both busy bits; one will always be low */
    while (flash->cr0 & FLASH_BSY) {
        if (flash->er & FLASH_ERMASK) {
            break;
        }
    }

    return flash->er & FLASH_ERMASK;
}


/********************/
/* FlashEraseRegion */
/********************/
int FlashEraseRegion(unsigned char *addrptr, int len)
{
    FLASHREGS *flash = (FLASHREGS *)FLASH_REG_BASE;
    unsigned long startsector;
    unsigned long endsector;
    unsigned long addr = (unsigned long )addrptr;

    /* Almost no error checking; make sure you know what you are doing!! */

    startsector = FlashGetSectorNumber(addr);
    endsector = FlashGetSectorNumber(addr + len);
    if ((startsector | endsector) == 0xffffffff) {
        return -1;
    }

    while (!(startsector & endsector)) {
        startsector |= startsector << 1;
    }

    flash->cr0 |= FLASHCMD_SER;
    flash->cr1 |= startsector;
    flash->cr0 |= FLASHCMD_START;

    /* Just monitor both busy bits; one will always be low */
    while (flash->cr0 & FLASH_BSY) {
        if (flash->er & FLASH_ERMASK) {
            break;
        }
    }

    return flash->er & FLASH_ERMASK;
}

static unsigned long ba2ul( unsigned char *d){
    unsigned long tmp;
    switch( (unsigned long)d & 3 ){
        case 0: //word aligned
            tmp = *(unsigned long *)(void *)d;
            break;
        default:
        case 1:
        case 3:
            tmp = d[0];
            tmp |= d[1]<<8;
            tmp |= d[2]<<16;
            tmp |= d[3]<<24;
            break;
        case 2: //16bit aligned
            tmp = *(unsigned short *)(void *)d;
            tmp |= *((unsigned short *)(void *)(d+2)) << 16;
            break;
    }
    return( tmp );
}

/********************/
/* FlashWriteBuffer */
/********************/
int FlashWriteBuffer(unsigned char *data, unsigned char *addrptr, int len)
{
    const unsigned long mask[3] = { 0x000000ff, 0x0000ffff, 0x00ffffff };
    unsigned long addr = (unsigned long)addrptr;
    unsigned long err;

    /* Must be at least word aligned */
    if (addr & 0x3) {
        //return -1;
        unsigned long firstword = *(unsigned long *)(void *)(addr & ~3);
        int mylen = 4 - (addr & 3);
        int i;
        mylen = mylen > len ? len : mylen;
        for( i=0; i<mylen; i++ ){
            firstword &= ~(0xff << (8*((addr+i) & 3)));
            firstword |= ((data[i]) << (8*((addr+i) & 3)));
        }
        err = FlashWriteWord((addr & ~3), firstword);
        if (err) return err;
        data += mylen;
        addr += mylen;
        len -= mylen;
    }

    while (len > 0) {
        if (len >= 8 && !(addr & 0x7)) {
            //err = FlashWriteDoubleWord(addr, *(unsigned long *)data, *(unsigned long *)(data + 4));
            err = FlashWriteDoubleWord(addr, ba2ul(&data[0]), ba2ul(&data[4]));
            if (err) return err;
            data += 8;
            addr += 8;
            len -= 8;
        } else if (len >= 4) {
            //err = FlashWriteWord(addr, *(unsigned long *)data);
            err = FlashWriteWord(addr, ba2ul(data));
            if (err) return err;
            data += 4;
            addr += 4;
            len -= 4;
        } else {
            //unsigned long lastword = *(unsigned long *)data;
            unsigned long lastword = ba2ul(data);
            lastword &= mask[len - 1];
            lastword |= (*(unsigned long *)addr & ~mask[len - 1]);
            err = FlashWriteWord(addr, lastword);
            if (err) return err;
            len = 0;
        }
    }
    return 0;

}


