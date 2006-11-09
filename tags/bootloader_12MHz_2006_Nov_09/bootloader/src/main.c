/*******************************************************************/
/*                                                                 */
/* File:  main.c                                                   */
/*                                                                 */
/* Description: The Qlarity Bootloader                             */
/*                                                                 */
/*******************************************************************/

/*****************************/
/* Standard Library Includes */
/*****************************/
#include <stdlib.h>
#include <string.h>

/*******************************/
/* Programmer Library Includes */
/*******************************/
#include "stddefs.h"
#include "str712.h"
#include "common.h"

#include "misc.h"
#include "serial.h"
#include "flash.h"

/***********/
/* Pragmas */
/***********/

/***********/
/* Defines */
/***********/
#define MAXLINE   256

#define CR        0x0d
#define LF        0x0a
#define ACK       0x06
#define NAK       0x15

/************/
/* TypeDefs */
/************/

/**********/
/* Macros */
/**********/

/********************/
/* Global Variables */
/********************/

/********************/
/* global variables */
/********************/
extern unsigned char BuildDate[];

const char CopyrightMsg[] __attribute__((section(".Copyright"))) = {
    "Copyright 2000-05 QSI Corporation - all rights reserved"
};

/* The bootloader stack */
unsigned long bldr_stack[1024] __attribute__((section(".stack"))) = { 0 };

/*
 * The interrupt stack for the arm processors - since all exceptions except Rx receive
 * end up locking the machine, all the interrupt handlers (most of which end
 * up in irq_lockup) should be able to share a single stack.
 */
unsigned long irq_stack[256] __attribute__((section(".irqstack"))) = { 0 };

UINT32 BootFlag __attribute__ ((section(".BootFlag")));

/************/
/* Routines */
/************/
void SendACK(void);

/********************/
/* InitializeClocks */
/********************/
void InitializeClocks(void)
{
    RCCUREGS * const rccu = (RCCUREGS *)RCCU_REG_BASE;
    PCUREGS * const pcu = (PCUREGS *)PCU_REG_BASE;

    /* Wait for internal voltage regulator to settle */
    while ((pcu->pwrcr & VR_OK) == 0) ;

    /*
     * Set up the PLL:
     * CLK2 = CLK3 = CK/2 (6 MHz for 12 MHz oscillator input)
     * MX = 01b (multiply by 12)
     * DX = 010b (divide by 3)
     * FREF_RANGE = 1 (CLK2 > 3 MHz)
     * Therefore, RCLK = 24 MHz  (6*12/3)
     */
    rccu->pll1cr = 0x52;

    /* Wait for PLL lock */
    while ((rccu->cfr & LOCK) == 0) ;

    /* Switch to PLL clock */
    rccu->cfr |= CSU_CKSEL;

    /*
     * Set up peripheral clocks:
     * MCLK = 24 MHz (no divisor, default)
     * PCLK1 = 24 MHz (divide by 1)
     * PCLK2 = 24 MHz (divide by 1)
     */
    pcu->pdivr = 0x0000;

    /* Disable peripheral clocks (External Memory and USB) to save power */
    rccu->per = 0;
    pcu->pll2cr = 7;    //disable pll 2 to save power
    pcu->pwrcr |= 0x8200;   //put flash into low power mode for 0 wait state operation (up to 33MHz) - ie set FLASH_LP bit
}

/**********************/
/* S-Record Functions */
/**********************/

/***********/
/* hex2val */
/***********/
UINT32 hex2val(char *buf, int digitlen)
{
    UINT32 val = 0;
    UINT8 hexdigit;
    int i;

    /* Two hex chars per byte */
    for (i = 0; i < digitlen; ++i) {
        hexdigit = *buf++;
        if ((hexdigit >= 'a') && (hexdigit <= 'z')) {
            hexdigit -= ('a' - 'A');
        }
        if ((hexdigit >= '0') && (hexdigit <= '9')) {
            hexdigit -= '0';
        } else if ((hexdigit >= 'A') && (hexdigit <= 'F')) {
            hexdigit = hexdigit - 'A' + 10;
        } else {
            /* Illegal value */
            return 0;
        }
        val = (UINT32) (hexdigit | (val << 4));
    }

    return val;
}

/***********/
/* hex2str */
/***********/
void hex2str(char *buf, int len, char *strbuf)
{
    while (len > 0) {
        *strbuf++ = (char)hex2val(buf, 2);
        buf += 2;
        len -= 2;
    }
    *strbuf = 0;
}

/****************/
/* CheckSRecord */
/****************/
int CheckSRecord(UINT8 *buf, int len)
{
    int sreclen;
    UINT8 checksum = 0;

    /* Check for S-record sanity */
    if ((buf[0] != 'S' && buf[0] != 's') || (len % 2)) {
        return -1;
    }

    /* Check length */
    sreclen = (int)hex2val(&buf[2], 2);
    if (sreclen * 2 != len - 4) {
        return -2;
    }

    /* Check checksum */
    for (checksum = 0, buf += 2, len -= 2; len > 0; len -= 2, buf += 2) {
        checksum += (UINT8)hex2val(buf, 2);
    }
    if (checksum != 0xff) {
        return -3;
    }

    return 0;
}


/******************/
/* ProcessSRecord */
/******************/
int ProcessSRecord(UINT8 *buf, int len)
{
    UINT8 srecbuf[32];
    UINT8 *address;
    int bytecount;
    int i;
    UINT8 *dptr;

    switch (buf[1]) {
    case '0': /* S-record header */
        /* Erase sections according to what is being sent */
        hex2str(&buf[8], len - 10, srecbuf);

        if (strcmp(srecbuf,"qbridge.srec") == 0) {
            FlashEraseRegion(_FirmwareStartAddr, 128 * 1024);
        }
#ifdef RVDEBUG
        if (strcmp(srecbuf,"qbboot.srec") == 0) {
            FlashEraseSector((UINT32)_RomStartAddr);
            FlashEraseSector((UINT32)_BootloaderStartAddr);
        }
#endif /* RVDEBUG */
        break;
    case '3': /* S-record data (4 byte address) */
        /* Burn it to flash */
        bytecount = (int)hex2val(&buf[2], 2);
        bytecount -= 5; /* Subtract address and checksum */
        address = (UINT8 *)hex2val(&buf[4], 8);

        /* Get srec data into buffer */
        for (dptr = &buf[12], i = 0; i < bytecount; ++i) {
            srecbuf[i] = (UINT8)hex2val(dptr, 2);
            dptr += 2;
        }

        if (FlashWriteBuffer(srecbuf, address, bytecount) != 0) {
            print("Flash Write Error!\r\n");
        }

        break;
    case '7': /* S-record termination (4 byte address) */
        SendACK();
        bootKRNL(0, (void *) _FirmwareStartAddr);
        break;
    default:
        return -1;
    }

    return 0;
}

/***********/
/* GetLine */
/***********/
/* Expects that lines will end with LF or CRLF and host will wait for ACK after each line */
int GetLine(UINT8 *buf, int maxlen)
{
    UINT8 *cptr = buf;
    UINT8 len = 0;
    int numchars = 0;

    /* Clear the buffer */
    bzero(buf, maxlen);

    /* Get characters up to CRLF */
    while (memchr(buf, LF, len) == NULL && len < maxlen) {
        PollReceive(hostPort, cptr, maxlen - len, &numchars);
        cptr += numchars;
        len += numchars;
    }

    if (len == maxlen) {
        /* Line too long, dump it */
        return 0;
    }

    /* Terminate line */
    if (*(cptr-2) == CR) {
        len -= 2;
    } else if (*(cptr-1) == LF) {
        --len;
    } else {
        return 0; /* Didn't end in LF or CRLF */
    }

    return len;

}

/***********/
/* SendACK */
/***********/
void SendACK(void)
{
    UINT8 ack[1];

    ack[0] = ACK;
    Transmit(hostPort, ack, 1);
}

/***********/
/* SendNAK */
/***********/
void SendNAK(void)
{
    UINT8 nak[1];

    nak[0] = NAK;
    Transmit(hostPort, nak, 1);
}

/**************/
/* bootloader */
/**************/
void bootloader(void) {
    UINT8 linebuf[MAXLINE];
    int len;

    print("Entering QSI Qbridge Bootloader " VERSION "\r\n");
    print(BuildDate);

    for (;;) {
        len = GetLine(linebuf, MAXLINE);
        if (len) {
            if (CheckSRecord(linebuf, len) == 0) {
                if (ProcessSRecord(linebuf, len) == 0) {
                    SendACK();
                } else {
                    SendNAK();
                }
            } else {
                SendNAK();
            }
        }
#if 0
        {
            /* Loopback mode */
            UINT8 rxbuf[16];
            int numchars = 0;
            PollReceive(hostPort, rxbuf, sizeof(rxbuf), &numchars);
            if (numchars) {
                Transmit(hostPort, rxbuf, numchars);
            }
        }
#endif /* #if 0 */
    }

    /* Never executes to here!!!! */
}

/********/
/* main */
/********/
int main(void) {
    /*
     * NOTE: The bootloader does not use interrupts at all, they
     * are never enabled. All serial routines use polled I/O.
     */
    InitializeClocks();

    InitializeHostPort();

    /* See if kernel requested the bootloader */
    if (BootFlag == BOOTFLAG_ENTER_BL) {
        print("NOTICE--received request from kernel to enter bootloader\r\n");
        /* So if the user shuts off his machine, it doesn't retain memory to come back in */
        BootFlag = 0;
        goto EnterBootloader;
    }

    /* Invalid kernels will return from this function */
    bootKRNL(0, (void *) _FirmwareStartAddr);

    /* Bootloader */
EnterBootloader:
    bootloader();

    /* Never returns!!!! */
    return (0);
}
