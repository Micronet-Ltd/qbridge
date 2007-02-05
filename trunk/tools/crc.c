/**************/
/*   crc.c    */
/*            */
/* Dean Ertel */
/* March 1997 */
/**************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

#define TRUE 1
#define FALSE 0
#define _MAX_PATH 200

int genCrcTable (char *fName);
int writeCodeHdr (char *fName,int offset);
size_t fileGetLen (char *file);
unsigned short calcCrc16Buf(char *buf,size_t len,int init);
void swapEndian(void *arg,int len);

static int	useLittleEndian = 0;

static unsigned short crc16Tbl[256] =
	{
		0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
		0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
		0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
		0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
		0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
		0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
		0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
		0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
		0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
		0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
		0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
		0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
		0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
		0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
		0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
		0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
		0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
		0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
		0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
		0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
		0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
		0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
		0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
		0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
		0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
		0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
		0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
		0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
		0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
		0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
		0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
		0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
	};


/********/
/* main */
/********/
int main (int argc,const char **argv)
{
	char fName[_MAX_PATH];
	int i,offset=0;

	if ( argc<2 ) {
		//Print usage
		printf("\nCRC utility program (Version 1.2.1), Dean Ertel 1999, Paul Jackson 2007\n");
		printf("USAGE: crc [options]\n");
		printf("Options:\n");
		printf("\t-bFILE:OFFSET\twrite code hdr crc into FILE at OFFSET(hex)\n");
//		printf("\t-gFILE\tgenerate FILE containing crc table\n");
		printf("\t-l\twrite the CRC header using little endian (default=big endian)\n");
		printf("\n");
		exit(0);
	}

	for ( i=1; i<argc; i++ )
		{
			if ( !strncmp(argv[i],"-b",2) ) {
				if ( sscanf(&argv[i][2],"%[^: ]:%x",&fName[0],&offset) != 2 ) {
					printf("ERROR--syntax error in option: '%s'\n",argv[i]);
					exit(1);
				}
				if (writeCodeHdr(fName,offset) ) {
					printf("ERROR--could not write code header--exiting.\n");
					exit(1);
				}
//			} else if ( !strncmp(argv[i],"-g",2) ) {
//				strcpy(fName,&argv[i][2]);
//				if (genCrcTable(fName) ) {
//					printf("ERROR--could not write CRC table--exiting.\n");
//					exit(1);
//				}
			} else if ( !strncmp(argv[i],"-l",2) ) {
				useLittleEndian = 1;
			} else
				printf("ERROR--illegal command option '%s'\n",argv[i]);
		}

	printf("...Completed Successfully!!!\n");
	return(0);
} //main

/****************/
/* writeCodeHdr */
/****************/
//Writes a 16-bit CRC and the code length into the code header
// at fixed address 0x100.  This is for the TYPE-K terminal using the H8/300H
// processor.

//header for code ROM -- This must match what is used in the code
typedef struct {
	char magic[4];		//Magic string 'CrC!'
	unsigned long codeLen;
	unsigned short crc;
} crcROMHdrDefn;
#define crcROMMagic "CrC!"
//This value must remain constant to be compatible with older kernals!
#define crcHdrLen 0x10

int writeCodeHdr (char *fName, int codeHdrOff)
{
	FILE *fp;
	size_t fileLen,size;
	char *buf;
	crcROMHdrDefn crcROMHdr;

	//Open file & get file length
	if ( (fileLen=fileGetLen(fName))==0 ) return(1);
	if ( (fp=fopen(fName,"rb"))==NULL ) {
		printf("ERROR--cannot open the file '%s'\n",fName);
		return(1);
	}

	//Read file into buffer
	if ( (buf=(char*)malloc(fileLen))==NULL ) {
		printf("ERROR--could not allocate %d bytes\n",fileLen);
		return(1);
	}
	if ( fread(buf,1,fileLen,fp)!=fileLen ) {
		printf("ERROR--could not read %d bytes from %s\n",fileLen,fName);
		return(1);
	}

	//Calculate the CRC (skip the hdr area of the code)
	memcpy(crcROMHdr.magic,crcROMMagic,4);
	crcROMHdr.codeLen = fileLen;
	if ( codeHdrOff == 0 ) {
		crcROMHdr.crc = calcCrc16Buf(&buf[crcHdrLen],fileLen-crcHdrLen,TRUE);
	} else {
		crcROMHdr.crc = calcCrc16Buf(buf,codeHdrOff,TRUE);
		printf("CRC for offset $0000-$%04x = $%04x\n",codeHdrOff,crcROMHdr.crc);
		size = codeHdrOff+crcHdrLen;
		crcROMHdr.crc = calcCrc16Buf(&buf[size],fileLen-size,FALSE);
	}
	printf("Final CRC=$%04x, fileLength=%d\n",crcROMHdr.crc,crcROMHdr.codeLen);

	//Adjust endianness & copy header into code
	if ( !useLittleEndian ) {
		swapEndian(&crcROMHdr.codeLen,sizeof(crcROMHdr.codeLen));
		swapEndian(&crcROMHdr.crc,sizeof(crcROMHdr.crc));
	}
	memcpy(&buf[codeHdrOff],&crcROMHdr,sizeof(crcROMHdr));

	//Write file back to disk
	fclose(fp);
	if ( (fp=fopen(fName,"wb"))==NULL ) {
		printf("ERROR--cannot open the file '%s' for writing\n",fName);
		return(1);
	}
	if ( fwrite(buf,1,fileLen,fp)!=fileLen ) {
		printf("ERROR--could not write %d bytes to %s\n",fileLen,fName);
		return(1);
	}

	free(buf);
	fclose(fp);
	return(0);
} //writeCodeHdr

/**************/
/* fileGetLen */
/**************/
size_t fileGetLen (char *file)
{
#if 1
    struct stat buf;
    stat(file,&buf);
    return( buf.st_size );
#else
	int hFile;
	size_t len;

	//Get file length
	if ( (hFile=_open(file,_O_RDONLY|_O_BINARY))==-1 ) {
		printf("file '%s' does not exist\n",file);
		return(0);
	}
	len = filelength(hFile);
	_close(hFile);
	return(len);
#endif
} //fileGetLen

/****************/
/* calcCrc16Buf */
/****************/
//Calculates the CRC for a buffer (does not affect crc16Byte)
//This should be the CITT standard CRC
unsigned short calcCrc16Buf(char *buf,size_t len,int init)
{
	char *p1;
	size_t i;
	static unsigned short crc16b;

	if (init) crc16b = 0xffff;

	for ( i=len,p1=buf; i>0; i--)
		crc16b = crc16Tbl[((crc16b>>8) ^ (unsigned short)*(p1++)) & 0x00ff] ^ (crc16b<<8);
	return(crc16b);
} //calcCrc16Buf

/**************/
/* swapEndian */
/**************/
//Swaps the endianness of the supplied arguement (len=# bytes in arguement)
void swapEndian(void *arg,int len)
{
	char *p1,*p2,tmp;

	for ( p1=(char*)arg,p2=&p1[len-1]; p1<p2; p1++,p2--) {
		tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
	}

} //swapEndian
