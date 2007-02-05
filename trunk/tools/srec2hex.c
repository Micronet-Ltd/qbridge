/******************************************************************************/
/* This file is from the original comp.sys.m68k FAQ by Greq Hawley.           */
/*                                                                            */
/*                                                                            */
/*  [S-Record Format]                                                         */
/*                                                                            */
/*         Chaplin@keinstr.uucp (Roger Chaplin) reposted an article written   */
/* 	by mcdchg!motmpl!ron (Ron Widell) that explained how Motorola           */
/* 	S-Records are formatted.  This comes from a unix man page.  No          */
/*         mention of which version of Unix is specified.  This section       */
/* 	of the FAQ is a bit long.  An anonymous ftp archive is currently        */
/* 	being sought.  When one is found, the section will be placed in         */
/* 	the archive.                                                            */
/*                                                                            */
/*         SREC(4)                UNIX 5.0 (03/21/84)                SREC(4)  */
/*                                                                            */
/*                                                                            */
/*         An S-record file consists of a sequence of specially formatted     */
/* 	ASCII character strings.  An S-record will be less than or equal to     */
/* 	78 bytes in length.                                                     */
/*                                                                            */
/*         The order of S-records within a file is of no significance and no  */
/* 	particular order may be assumed.                                        */
/*                                                                            */
/*         The general format of an S-record follow:                          */
/*                                                                            */
/*         +------------------//-------------------//-----------------------+ */
/*         | type | count | address  |            data           | checksum | */
/*         +------------------//-------------------//-----------------------+ */
/*                                                                            */
/*           type      A char[2] field.  These characters describe the        */
/*                     type of record (S0, S1, S2, S3, S5, S7, S8, or         */
/*                     S9).                                                   */
/*           count     A char[2] field.  These characters when paired and     */
/*                     interpreted as a hexadecimal value, display the        */
/*                     count of remaining character pairs in the record.      */
/*                                                                            */
/*           address   A char[4,6, or 8] field.  These characters grouped     */
/*                     and interpreted as a hexadecimal value, display        */
/*                     the address at which the data field is to be           */
/*                     loaded into memory.  The length of the field           */
/*                     depends on the number of bytes necessary to hold       */
/*                     the address.  A 2-byte address uses 4 characters,      */
/*                     a 3-byte address uses 6 characters, and a 4-byte       */
/*                     address uses 8 characters.                             */
/*           data      A char [0-64] field.  These characters when paired     */
/*                     and interpreted as hexadecimal values represent        */
/*                     the memory loadable data or descriptive                */
/*                     information.                                           */
/*                                                                            */
/*           checksum  A char[2] field.  These characters when paired and     */
/*                     interpreted as a hexadecimal value display the         */
/*                     least significant byte of the ones complement of       */
/*                     the sum of the byte values represented by the          */
/*                     pairs of characters making up the count, the           */
/*                     address, and the data fields.                          */
/*                                                                            */
/*           Each record is terminated with a line feed.  If any              */
/*           additional or different record terminator(s) or delay            */
/*           characters are needed during transmission to the target          */
/*           system it is the responsibility of the transmitting program      */
/*           to provide them.                                                 */
/*                                                                            */
/*           S0 Record  The type of record is 'S0' (0x5330).  The address     */
/*                      field is unused and will be filled with zeros         */
/*                      (0x0000).  The header information within the data     */
/*                      field is divided into the following subfields.        */
/*                                                                            */
/*                                   mname      is char[20] and is the        */
/*                                              module name.                  */
/*                                   ver        is char[2] and is the         */
/*                                              version number.               */
/*                                   rev        is char[2] and is the         */
/*                                              revision number.              */
/*                                   description is char[0-36] and is a       */
/*                                              text comment.                 */
/*                                                                            */
/*                      Each of the subfields is composed of ASCII bytes      */
/*                      whose associated characters, when paired,             */
/*                      represent one byte hexadecimal values in the case     */
/*                      of the version and revision numbers, or represent     */
/*                      the hexadecimal values of the ASCII characters        */
/*                      comprising the module name and description.           */
/*                                                                            */
/*           S1 Record  The type of record field is 'S1' (0x5331).  The       */
/*                      address field is interpreted as a 2-byte address.     */
/*                      The data field is composed of memory loadable         */
/*                      data.                                                 */
/*           S2 Record  The type of record field is 'S2' (0x5332).  The       */
/*                      address field is interpreted as a 3-byte address.     */
/*                      The data field is composed of memory loadable         */
/*                      data.                                                 */
/*                                                                            */
/*           S3 Record  The type of record field is 'S3' (0x5333).  The       */
/*                      address field is interpreted as a 4-byte address.     */
/*                      The data field is composed of memory loadable         */
/*                      data.                                                 */
/*           S5 Record  The type of record field is 'S5' (0x5335).  The       */
/*                      address field is interpreted as a 2-byte value        */
/*                      and contains the count of S1, S2, and S3 records      */
/*                      previously transmitted.  There is no data field.      */
/*                                                                            */
/*           S7 Record  The type of record field is 'S7' (0x5337).  The       */
/*                      address field contains the starting execution         */
/*                      address and is interpreted as  4-byte address.        */
/*                      There is no data field.                               */
/*           S8 Record  The type of record field is 'S8' (0x5338).  The       */
/*                      address field contains the starting execution         */
/*                      address and is interpreted as  3-byte address.        */
/*                      There is no data field.                               */
/*                                                                            */
/*           S9 Record  The type of record field is 'S9' (0x5339).  The       */
/*                      address field contains the starting execution         */
/*                      address and is interpreted as  2-byte address.        */
/*                      There is no data field.                               */
/*                                                                            */
/*      EXAMPLE                                                               */
/*                                                                            */
/*           Shown below is a typical S-record format file.                   */
/*                                                                            */
/*                  S00600004844521B                                          */
/*                  S1130000285F245F2212226A000424290008237C2A                */
/*                  S11300100002000800082629001853812341001813                */
/*                  S113002041E900084E42234300182342000824A952                */
/*                  S107003000144ED492                                        */
/*                  S5030004F8                                                */
/*                  S9030000FC                                                */
/*                                                                            */
/*           The file consists of one S0 record, four S1 records, one S5      */
/*           record and an S9 record.                                         */
/*                                                                            */
/*           The S0 record is comprised as follows:                           */
/*                                                                            */
/*              S0     S-record type S0, indicating it is a header            */
/*                     record.                                                */
/*              06     Hexadecimal 06 (decimal 6), indicating that six        */
/*                     character pairs (or ASCII bytes) follow.               */
/*                                                                            */
/*              00 00  Four character 2-byte address field, zeroes in         */
/*                     this example.                                          */
/*              48     ASCII H, D, and R - "HDR".                             */
/*                                                                            */
/*              1B     The checksum.                                          */
/*                                                                            */
/*           The first S1 record is comprised as follows:                     */
/*              S1     S-record type S1, indicating it is a data record       */
/*                     to be loaded at a 2-byte address.                      */
/*                                                                            */
/*              13     Hexadecimal 13 (decimal 19), indicating that           */
/*                     nineteen character pairs, representing a 2 byte        */
/*                     address, 16 bytes of binary data, and a 1 byte         */
/*                     checksum, follow.                                      */
/*              00 00  Four character 2-byte address field; hexidecimal       */
/*                     address 0x0000, where the data which follows is to     */
/*                     be loaded.                                             */
/*                                                                            */
/*              28 5F 24 5F 22 12 22 6A 00 04 24 29 00 08 23 7C Sixteen       */
/*                     character pairs representing the actual binary         */
/*                     data.                                                  */
/*              2A     The checksum.                                          */
/*                                                                            */
/*           The second and third S1 records each contain 0x13 (19)           */
/*           character pairs and are ended with checksums of 13 and 52,       */
/*           respectively.  The fourth S1 record contains 07 character        */
/*           pairs and has a checksum of 92.                                  */
/*                                                                            */
/*           The S5 record is comprised as follows:                           */
/*                                                                            */
/*              S5     S-record type S5, indicating it is a count record      */
/*                     indicating the number of S1 records.                   */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*              03     Hexadecimal 03 (decimal 3), indicating that three      */
/*                     character pairs follow.                                */
/*                                                                            */
/*              00 04  Hexadecimal 0004 (decimal 4), indicating that          */
/*                     there are four data records previous to this           */
/*                     record.                                                */
/*              F8     The checksum.                                          */
/*                                                                            */
/*           The S9 record is comprised as follows:                           */
/*                                                                            */
/*              S9     S-record type S9, indicating it is a termination       */
/*                     record.                                                */
/*              03     Hexadecimal 03 (decimal 3), indicating that three      */
/*                     character pairs follow.                                */
/*                                                                            */
/*              00 00  The address field, hexadecimal 0 (decimal 0)           */
/*                     indicating the starting execution address.             */
/*              FC     The checksum.                                          */
/*                                                                            */
/*                                                                            */
/*     [Intel Hex ASCII Format]                                               */
/*                                                                            */
/* 	Intel HEX-ASCII format takes the form:                                  */
/*                                                                            */
/*             +----------------------------------- Start Character           */
/*             |                                                              */
/*             |  +-------------------------------- Byte Count                */
/*             |  |                                     (# of data bytes)     */
/*             |  |                                                           */
/*             |  |     +-------------------------- Address of first data.    */
/*             |  |     |                                                     */
/*             |  |     |     +-------------------- Record Type (00 data,     */
/*             |  |     |     |                         01 end of record)     */
/*             |  |     |     |                                               */
/*             |  |     |     |       +------------ Data Bytes                */
/*             |  |     |     |       |                                       */
/*             |  |     |     |       |       +---- Checksum                  */
/*             |  |     |     |       |       |                               */
/* 	    | / \ /     \ / \ /         \ / \                                   */
/* 	    : B C A A A A T T H H ... H H C C                                   */
/*                                                                            */
/* 	An examples:                                                            */
/*                                                                            */
/* 	    :10000000DB00E60F5F1600211100197ED300C3004C                         */
/* 	    :1000100000000101030307070F0F1F1F3F3F7F7FF2                         */
/* 	    :01002000FFE0                                                       */
/* 	    :00000001FF                                                         */
/*                                                                            */
/* 	This information comes from _Microprocessors and Programmed             */
/* 	Logic_, Second Edition, Kenneth L. Short, 1987, Prentice-Hall,          */
/* 	ISBN 0-13-580606-2.                                                     */
/*                                                                            */
/* 	Provisions have been made for data spaces larger than 64 kBytes.        */
/* 	The above reference does not discuss them.  I suspect there is          */
/* 	a start of segment type record, but I do not know how it is             */
/* 	implemented.                                                            */
/******************************************************************************/

#include <stdio.h>
#include <malloc.h>
#include <string.h>

/* Maximum of 78 bytes (78 bytes fits all of the data - the pairing of the data divides it by 2) */
#define MAX_CHAR_DIGITS 78


typedef struct {
   unsigned char type;
   unsigned char count;
   unsigned long addr;
   unsigned char *data;
   unsigned char chksum;
   unsigned char linebuf[MAX_CHAR_DIGITS/2];
} srecItem;

typedef struct {
   unsigned long minAddr;
   unsigned long maxAddr;
   unsigned char *buf;
   unsigned long len;
} readSRecItem;

static unsigned char *scanLineBuf = NULL;
static int scanLineBufLen = 0;

int verbose = 1;

/************/
/* readLine */
/************/
/* Read a line from the file */
int readLine(unsigned char *inbuf, int inlen, FILE *f)
{
   int c;
   int index;

   index = 0;
   c = fgetc(f);
   while ((index < (inlen-1)) && (c != EOF) && (c != '\n')) {
      inbuf[index++] = (unsigned char) c;
      c = fgetc(f);
   }
   /* Check to see if we ran out of space */
   if (index >= (inlen-1)) {
      /* Too much data for buffer */
      return -1;
   }

   /* Null terminate the string (it should be all text data) */
   inbuf[index] = '\0';

   if (c == EOF) {
      /* End of file reached */
      return 1;
   }
   return 0;
}

/************/
/* scanLine */
/************/
int scanLine(srecItem *srec, FILE *f)
{
   int tmp;
   int readResult;
   unsigned char *bufptr = NULL;
   unsigned char digit;
   long pos;

   for (;;) {
      pos = ftell(f);
      readResult = readLine(scanLineBuf, scanLineBufLen, f);
      if (readResult == -1) {
         unsigned char *tmpbuf = scanLineBuf;
         scanLineBuf = (unsigned char *) malloc(scanLineBufLen + 128);
         if (scanLineBuf == NULL) {
            scanLineBuf = tmpbuf;
            return -1;
         }
         scanLineBufLen += 128;
         if (tmpbuf != NULL) {
            free(tmpbuf);
         }

         /* Retry the readline */
         if (fseek(f, pos, SEEK_SET) != 0) {
            return -2;
         }
         continue;
      }
      /* Must have read the line ok, now to break it up */

      /* Find first text character in line */
      for (bufptr = scanLineBuf; (*bufptr != '\0') && (isspace(*bufptr) != 0); bufptr++);
      if (*bufptr == '\0') {
         if (readResult == 1) {
            /* If this is the last line, no worries */
            return 1;
         }
         /* Otherwise, we have an empty line, so get another */
         continue;
      }

      /* Now read data into srec item - filling in pertinent information */
      if ((*bufptr >= 'a') && (*bufptr <= 'z')) {
         *bufptr -= ('a' - 'A');
      }
      if (*bufptr != 'S') {
         /* Illegal line */
         return -3;
      }
      bufptr++;
      for (digit = 0, tmp = 1; tmp < MAX_CHAR_DIGITS; bufptr++, tmp++) {
         if ((*bufptr >= 'a') && (*bufptr <= 'z')) {
            *bufptr -= ('a' - 'A');
         }
         if ((*bufptr >= '0') && (*bufptr <= '9')) {
            *bufptr -= '0';
         } else if ((*bufptr >= 'A') && (*bufptr <= 'F')) {
            *bufptr = *bufptr - 'A' + 10;
         } else if ((isspace(*bufptr) != 0) || (*bufptr == '\0')) {
            /* Read all we had on the line, time to process it */
            break;
         } else {
            /* Illegal line */
            return -3;
         }

         digit = (unsigned char) (*bufptr | (digit << 4));

         /* A new digit is available at the end of each line */
         if ((tmp % 2) != 0) {
            if (tmp == 1) {
               if ((digit > 9) || (digit == 4) || (digit == 6)) {
                  /* Don't know what this would be */
                  return -3;
               }
               srec->type = digit;
            } else {
               srec->linebuf[tmp/2 - 1] = digit;
            }
         }
      }

      /* Now fill in some of the record fields */
      /* First the count */
      srec->count = srec->linebuf[0];
      if (srec->count >= sizeof(srec->linebuf)) {
         /* Bad count */
         return -4;
      }
      /* Now check the checksum */
      srec->chksum = srec->linebuf[srec->count];
      for (digit = 0, tmp = 0; tmp <= srec->count; tmp++) {
         digit += srec->linebuf[tmp];
      }
      if (digit != 0xFF) {
         /* Bad checksum */
         return -5;
      }
      switch (srec->type) {
      case 0:
         /* Two bytes for the address (which is unused in this case) */
         srec->addr = 0;
         for (tmp = 0; tmp < 2; tmp++) {
            srec->addr = (srec->addr << 8) | srec->linebuf[1 + tmp];
         }
         srec->data = &(srec->linebuf[3]);
         /* Subtract off the checksum and address length */
         srec->count -= 4;
         break;
      case 1:
      case 5:
      case 9:
         /* Two bytes for the address */
         srec->addr = 0;
         for (tmp = 0; tmp < 2; tmp++) {
            srec->addr = (srec->addr << 8) | srec->linebuf[1 + tmp];
         }
         srec->data = &(srec->linebuf[3]);
         /* Subtract off the checksum and address length */
         srec->count -= 3;
         break;
      case 2:
      case 8:
         /* Three bytes for the address */
         srec->addr = 0;
         for (tmp = 0; tmp < 3; tmp++) {
            srec->addr = (srec->addr << 8) | srec->linebuf[1 + tmp];
         }
         srec->data = &(srec->linebuf[4]);
         /* Subtract off the checksum and address length */
         srec->count -= 4;
         break;
      case 3:
      case 7:
         /* Four bytes for the address */
         srec->addr = 0;
         for (tmp = 0; tmp < 4; tmp++) {
            srec->addr = (srec->addr << 8) | srec->linebuf[1 + tmp];
         }
         srec->data = &(srec->linebuf[5]);
         /* Subtract off the checksum and address length */
         srec->count -= 5;
         break;
      }
      break;
   }

   return 0;
}

/****************/
/* readSRecFile */
/****************/
int readSRecFile(readSRecItem *srec, char *infilename, char *outfilename)
{
   int tmp;
   long idx;
   int retVal = 0;
   FILE *in;
   FILE *out;
   srecItem item;
   unsigned char *tmpptr;
   unsigned char verboseStrBuf[128] = {0};
   unsigned char *verboseStr = verboseStrBuf;
   int outhex;
   extern void genhexrec(FILE *f, srecItem *i);

   /* Set up some initial values */
   srec->minAddr = 0xFFFFFFFF;
   srec->maxAddr = 0;
   srec->buf = NULL;
   srec->len = 0;

   /* Open input file here */
   in = fopen(infilename, "rb");
   if (in == NULL) {
		fprintf(stderr, "Unable to open input srec file %s!\n", infilename);
      retVal = -1;
      goto cleanExitRead;
	}
    out = fopen(outfilename,"wb");
    if (out== NULL) {
        fprintf(stderr, "Unable to open output hex file %s!\n", outfilename);
        retVal = -1;
        goto cleanExitRead;
    }

   for (idx = 0;;) {
      outhex = 0;
      tmp = scanLine(&item, in);
      idx++;
      if (tmp == 1) {
         /* End of file reached - do this here, so we can break */
         break;
      }
      switch (tmp) {
      case 0:
         /* Keep reading - noting max and min addresses */
         break;
      case -1:
         /* Out of memory */
         fprintf(stderr, "Out of memory!\n");
         retVal = -1;
         goto cleanExitRead;
         break;
      case -2:
         /* File access error */
         fprintf(stderr, "File access error!\n");
         retVal = -1;
         goto cleanExitRead;
         break;
      case -3:
         /* Error in file data */
         fprintf(stderr, "Error in data on line %d!\n", idx);
         retVal = -1;
         goto cleanExitRead;
         break;
      case -4:
         /* Bad count */
         fprintf(stderr, "Error in count on line %d!\n", idx);
         retVal = -1;
         goto cleanExitRead;
         break;
      case -5:
         /* Bad checksum */
         fprintf(stderr, "Error in checksum on line %d!\n", idx);
         retVal = -1;
         goto cleanExitRead;
         break;
      }

      switch (item.type) {
      case 0: //module name... display
        if( item.addr != 0 ){
            fprintf(stderr, "Error in module specifier record at line %d!\n", idx);
            retVal = -1;
            goto cleanExitRead;
            break;
        }
        item.data[item.count+1] = '\0';
        fprintf(stderr,"Module Specifier='%s' at line %d\n", item.data, idx );
      case 7: //execution start... ignore
      case 8: //execution start... ignore
      case 9: //execution start... ignore
         /* Ignore these, we really don't care */
         break;
      case 2:
      case 3:
         if (item.addr < srec->minAddr) {
            srec->minAddr = item.addr;
         }
         if ((item.addr + item.count) > srec->maxAddr) {
            srec->maxAddr = item.addr + item.count;
         }
         outhex = 1;
         break;
      default:
         fprintf(stderr, "Currently unsupported srec type %d!\n", item.type);
         retVal = -1;
         goto cleanExitRead;
         break;
      }
      if( outhex ){
        genhexrec(out, &item);
      }
   }          
   fprintf(out,":00000001FF\r\n");//End of file record


   switch (verbose) {
   case 1:
//      idx = sprintf(verboseStr, "%s", infilename);
//      verboseStr += idx;
//      for (; idx < 20; idx++) {
//         *(verboseStr++) = ' ';
//      }
//      verboseStr += sprintf(verboseStr, "(0x%08x-0x%08x)   size: %6d bytes\n", srec->minAddr, srec->maxAddr, srec->len);
      /* Fall-through */
   default:
      break;
   }

cleanExitRead:
   if (retVal == 0) {
      fprintf(stdout, verboseStrBuf);
   }
   if (in != NULL) {
      fclose(in);
   }
   if (out != NULL ) {
      fclose(out);
   }
   if (scanLineBuf != NULL) {
      free(scanLineBuf);
      scanLineBuf = NULL;
      scanLineBufLen = 0;
   }
   if ((retVal != 0) && (srec->buf != NULL)) {
      free(srec->buf);
      srec->buf = NULL;
      srec->len = 0;
   }

   return retVal;
}

/******************/
/* calcSRecChksum */
/******************/
void calcSRecChksum(srecItem *rec, int datalen)
{
	unsigned char chksum = 0;
	unsigned char *addr = (unsigned char *)&(rec->addr);
	unsigned char *data;
	int i;

	chksum += rec->count;

	/* Should be OK for all records; high addr bits should be 0 for shorter addresses */
	for (i = 0; i < 4; ++i) {
		chksum += addr[i];
	}

	for (i = datalen, data = rec->data; i; --i) {
		chksum += *data++;
	}

	rec->chksum = 0xff - chksum;

}

void genhexrec(FILE *f, srecItem *i){
    static int hexaddrvalid = 0;
    static unsigned long hexaddr;
    int n;
    if( (hexaddrvalid == 0) || (i->addr != hexaddr) ){
        hexaddr = i->addr;
        hexaddrvalid = 1;
        int checksum = 2 + 4 + ((hexaddr>>(16+8)) & 0xff) + ((hexaddr>>(16+0)) & 0xff);
        fprintf(f,":%02X%04X%02X%04X%02X\r\n",2, 0, 4, (int)((hexaddr >> 16) & 0xffff), ((-checksum) & 0xff));
    }
    int mycnt = 0;
    do{
        int cando = ((hexaddr + 0x10000) - hexaddr);
        int willdo = cando > i->count ? i->count : cando;
        //           num data bytes, addr, rec type, data, checksum
        fprintf(f,":%02X%04X%02X", willdo, (hexaddr & 0xffff), 0);
        int checksum = i->count + ((hexaddr >> 8) & 0xff) + (hexaddr & 0xff);
        for( n=0; n<willdo; n++ ){
          fprintf(f,"%02X", i->data[mycnt]);
          checksum += i->data[mycnt];
          hexaddr++;
          mycnt++;
        }
        fprintf(f,"%02X\r\n", (-checksum) & 0xff );
        if( mycnt != i->count ){
          int checksum = 2+4+((hexaddr>>(16+8)) & 0xff)+((hexaddr>>(16)) & 0xff);
          //               num data bytes, addr, rec type, data, checksum
          fprintf(f,":%02X%04X%02X%04X%02X\r\n",2, 0, 4, (int)((hexaddr >> 16) & 0xffff), ((-checksum) & 0xff));
        }else{
          if( (hexaddr & 0xffff) == 0 ) hexaddrvalid = 0;
        }
    }while( mycnt < i->count );
}

#if 0
/****************/
/* writeSRecord */
/****************/
int writeSRecord(srecItem *rec, FILE *file)
{
	int datacnt = 0;
	unsigned char *data;

	switch(rec->type) {
	case 0:
		fprintf(file, "S%1X%02X0000", rec->type, rec->count);
		datacnt = rec->count - 3;
		break;
	case 1:
	case 9:
		fprintf(file, "S%1X%02X%04X", rec->type, rec->count, rec->addr);
		datacnt = rec->count - 3;
		break;
	case 2:
	case 8:
		fprintf(file, "S%1X%02X%06X", rec->type, rec->count, rec->addr);
		datacnt = rec->count - 4;
		break;
	case 3:
	case 7:
		fprintf(file, "S%1X%02X%08X", rec->type, rec->count, rec->addr);
		datacnt = rec->count - 5;
		break;
	default:
		fprintf(stderr, "Illegal S-Record type: %d!\r\n", rec->type);
		return -1;
	}

	calcSRecChksum(rec, datacnt);

	/* Write data field */
	for (data = rec->data; datacnt; --datacnt) {
		fprintf(file, "%02X", *data++);
	}

	/* Write checksum */
	fprintf(file, "%02X\r\n", rec->chksum);

	return 0;
}

/*****************/
/* writeSRecFile */
/*****************/
int writeSRecFile(readSRecItem *srec, char *filename)
{
	srecItem rec;
	FILE *outfile;
	unsigned long addr;
	unsigned char *data;
	unsigned long len;

   outfile = fopen(filename, "w");
   if (outfile == NULL) {
		return -1;
   }

	/* Write S0 record */
	rec.type = 0;
	rec.count = strlen(filename) + 3; /* 2 addr pairs and 1 chksum pair */
	rec.addr = 0;
	rec.data = filename;
	writeSRecord(&rec, outfile);

	/* Write data (S3) records */
	addr = srec->minAddr; 
	data = srec->buf;
	len = srec->len;
	rec.type = 3;
	while (len) {
		rec.addr = addr;
		rec.data = data;
		if (len > 16) {
			rec.count = 16 + 5; /* 4 addr pairs and 1 chksum pair */
			len -= 16;
			addr += 16;
			data += 16;
		} else {
			rec.count = len + 5; /* 4 addr pairs and 1 chksum pair */
			len = 0;
		}
		writeSRecord(&rec, outfile);
	}

	/* Write termination record */
	rec.type = 7;
	rec.count = 5; /* 4 addr pairs (entrypoint) and 1 chksum pair */
	rec.addr = (unsigned long)romHdr->entry;
	rec.data = 0;
	writeSRecord(&rec, outfile);

	fclose(outfile);

	return 0;
}
#endif

/**************/
/* printUsage */
/**************/
void printUsage(char *progname)
{
	fprintf(stdout, "Usage: %s <input srec file> <output hex file>\n", progname);
	fprintf(stdout, "       Converts an srec file to an intel hex file\n");
}

/********/
/* main */
/********/
int main(int argc, char *argv[])
{
   int idx;
   int retVal;
   readSRecItem *srec = NULL;

	if (argc != 3) {
		printUsage(argv[0]);
        goto cleanExit;
	}

   /* Have to set up some records for each file */
   srec = (readSRecItem *) malloc(sizeof(readSRecItem));
   if (srec == NULL) {
      /* Out of memory */
      fprintf(stderr, "Out of memory!\n");
      retVal = -1;
      goto cleanExit;
   }
	srec->buf = NULL;

	/* Read the srec into memory */
	retVal = readSRecFile(srec, argv[1], argv[2]);
	if (retVal != 0) {
		/* Error has already printed */
        FILE *in;
        in = fopen(argv[2], "rb");
        if (in != NULL) {
            fclose(in);
            in = fopen(argv[2], "wb");
        	fclose(in);
        }
		goto cleanExit;
	}

cleanExit:
   if (srec != NULL) {
		if (srec->buf != NULL) {
			free(srec->buf);
		}
	}
	free(srec);

	return retVal;
}
