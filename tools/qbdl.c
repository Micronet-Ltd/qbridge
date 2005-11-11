/*******************************************************************/
/*                                                                 */
/* File:  LG55DL.C                                                 */
/*                                                                 */
/* Description:  Downloads a file to the R45/R55/G55 terminal -    */
/*               linux version                                     */
/*                                                                 */
/*******************************************************************/
/*      This program, in any form or on any media, may not be      */
/*        used for any purpose without the express, written        */
/*                      consent of QSI Corp.                       */
/*                                                                 */
/*             (c) Copyright 1997 QSI Corporation                  */
/*******************************************************************/
/* Release History                                                 */
/*                                                                 */
/*  Date        By            Rev     Description                  */
/*-----------------------------------------------------------------*/
/* 26-Oct-98    MK Elwood     1.0     1st Release (win32)          */
/* 26-Aug-99    RY Martindale 2.0     2nd Release (for linux)      */
/*******************************************************************/

/*****************************/
/* Standard Library Includes */
/*****************************/
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>   
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>

/*******************************/
/* Programmer Library Includes */
/*******************************/

/***********/
/* Pragmas */
/***********/

/***********/
/* Defines */
/***********/
#define VERSION                 2.139
#define BLK_SIZE                512
#define CR							  0x0D
#define LF							  0x0A
#define SPACE						  0x20
#define K65ESC						  0xBF

#define BAUDRATE                B115200
#define COM_DEV_STR             "/dev/ttyS0"
#define _POSIX_SOURCE           1 /* POSIX compliant source */
#define FALSE                   0
#define TRUE                    1

#define FIRMWARE                0x01
#define OLD_BL                  0x02
#define UPGRADE_BL              0x04
#define G60DL                   0x10
#define XON_XOFF                0x40
#define HWFC                    0x80

#define XON                     0x13
#define XOFF                    0x11

#define BUFFER_SIZE             256

/************/
/* TypeDefs */
/************/
typedef unsigned short int WORD;
typedef unsigned char BYTE;

/**********/
/* Macros */
/**********/

/********************/
/* Global Variables */
/********************/
volatile int STOP = FALSE;
FILE *DownFile = NULL;

const WORD crc16Tbl[256] =
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


/************/
/* Routines */
/************/
WORD calcCrc16Buf(BYTE *buf,long len);
void PrintUsage(char *prog);
void Download(int comFileDesc, BYTE *buf, long len, char *hdr, long delay, BYTE Flags);
void SigHandler(int value);

/***************/
/* PrettyPrint */
/***************/
void PrettyPrint(unsigned char *buf, unsigned int size, FILE *LogFile1, FILE *LogFile2) 
{
   int i, j;
	static unsigned char buf2[BUFFER_SIZE + 4];

   // Prefer to log the data in this manner
   if (LogFile2 != NULL) {
      fwrite(buf, 1, size, LogFile2);
   }

   // Print out buffer, indicating hex values for unprintable characters.
   j = 0;
   for (i = 0; i < size; i++) {
      if (isprint(buf[i])) {
         buf2[j++] = buf[i];
         // Subtraction amount (6) corresponds with the size of hex info down in else section
         if (j == sizeof(buf2) - 1 - 6) {
            buf2[j] = '\0';
            fprintf(stdout, "%s", buf2);
            if (LogFile1 != NULL) {
               fprintf(LogFile1, "%s", buf2);
            }
            j = 0;
         }
      } else if (buf[i] == '\n') {
         buf2[j] = '\0';
         fprintf(stdout, "%s<\\n>\n", buf2);
         if (LogFile1 != NULL) {
            fprintf(LogFile1, "%s<\\n>\n", buf2);
         }
         j = 0;
      } else {
         sprintf(buf2 + j, "<0x%02X>", (int) buf[i]);
         fprintf(stdout, "%s", buf2);
         if (LogFile1 != NULL) {
            fprintf(LogFile1, "%s", buf2);
         }
         j = 0;
      }
   }
   
   // Only print stuff out if there is something left to print out.
   if (j != 0) {
      buf2[j] = '\0';
      fprintf(stdout, "%s", buf2);
      if (LogFile1 != NULL) {
         fprintf(LogFile1, "%s", buf2);
      }
   }
   
   fflush(stdout);
   if (LogFile1 != NULL) {
      fflush(LogFile1);
   }
   if (LogFile2 != NULL) {
      fflush(LogFile2);
   }
}

/********/
/* main */
/********/
main(int argc, char **argv)
{
	int c;
	long i, j, k;
	char upgradeType;

	int fd, res;
	struct termios oldtio, newtio;
	struct termios oldstdtio, newstdtio;

	unsigned char buf[BUFFER_SIZE];

	FILE *tempFile = NULL;
	FILE *memGCMFile = NULL;
	FILE *memMainFile = NULL;
	FILE *LogFile1 = NULL;
	FILE *LogFile2 = NULL;

	int NumGCM;
	char *memGCMBuf;
	size_t memGCMSize;
	char *memMainBuf;
	size_t memMainSize;
	char *tempBuf;
	size_t tempSize;

	unsigned int comport, baud;
	unsigned char PortStr[255];

	// Command Headers.
	// Transmit all characters of header (these should never contain xon, xoff, or escape characters)
	unsigned char UpgradeBL[3] = { 0xA5, ',', 0x00 };
	unsigned char UpgradeRamKrnl[3] = { 0xBD, ',', 0x00 };
	unsigned char UpgradeParm[3] = { 0xC3, ',', 0x00 };

	char *cmd[] = { "","S,","R45D","G55D","GCMD", (char *) &UpgradeBL,"G60D", 
						 (char *) &UpgradeRamKrnl, (char *) &UpgradeParm };
	int cmdnum = 2;

	char **FileName = NULL;
	int filenum = 0;

	fd_set rfds;
	struct timeval tv;

	long delay = 100000000;

	// Flags
	BYTE GetOptions = TRUE;
	BYTE Flags = 0;

	void (* OldHand)(int);

	// Set default com port string.
	sprintf(PortStr, "%s", COM_DEV_STR);

	// Set default baud rate.
	baud = BAUDRATE;

	// Check for print usage.
	if ((argc == 2) && !strncmp( argv[1], "-h", 2))
		{
			// Pass program name to PrintUsage.
			PrintUsage(argv[0]);
			exit(0);
		}

	// Get comm parameters
	for (c = 1; c < argc; c++)	{
		// Check for options.
		if ((GetOptions) && (argv[c][0] == '-')) {
			switch(argv[c][1]) {
			case 'c':
				sscanf(argv[c],"-c%u",&comport);
				switch (comport) {
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 8:
					sprintf(PortStr + strlen(PortStr) - 1, "%d", comport - 1);
					break;
				default:
					printf("ERROR:  Illegal communications port\n");
					exit(-1);
					break;
				}
				break;
			case 'b':
				sscanf( argv[c],"-b%u",&baud);
				switch (baud) {
				case 1200:
					baud = B1200;
					break;
				case 2400:
					baud = B2400;
					break;
				case 4800:
					baud = B4800;
					break;
				case 9600:
					baud = B9600;
					break;
				case 19200:
					baud = B19200;
					break;
				case 38400:
					baud = B38400;
					break;
				case 57600:
					baud = B57600;
					break;
				case 115200:
					baud = B115200;
					break;
				default:
					printf("ERROR:  Illegal baud value\n");
					exit(-1);
					break;
				}
				break;
			case 'd':
            {
               long tmp = 0;
               sscanf(argv[c],"-d%lu", &tmp);
               switch (tmp) {
               case 1:
                  if (LogFile1 != NULL) {
                     printf("ERROR:  File data.log already opened\n");
                  }
                  LogFile1 = fopen("./data.log", "wb");
                  if (LogFile1 == NULL) {
                     printf("ERROR:  Unable to open data.log file\n");
                     exit(-1);
                  }
                  break;
               case 2:
                  if (LogFile2 != NULL) {
                     printf("ERROR:  File actual.log already opened\n");
                  }
                  LogFile2 = fopen("./actual.log", "wb");
                  if (LogFile2 == NULL) {
                     printf("ERROR:  Unable to open actual.log file\n");
                     exit(-1);
                  }
                  break;
               default:
                  printf("ERROR:  Illegal option\n");
                  PrintUsage(argv[0]);
                  exit(-1);
               }
            }
				break;
			case 't':
				sscanf(argv[c],"-t%lu", &delay);
				break;
			case 'r':
				if (Flags & XON_XOFF)
					printf("Using both hardware and software flow control??!!??\n");

				Flags |= HWFC;
				break;
			case 'x':
				if (Flags & HWFC)
					printf("Using both hardware and software flow control??!!??\n");

				Flags |= XON_XOFF;
				break;
			case 'f':
				// This is a firmware upgrade
				if (cmdnum == 0) {
					printf("ERROR:  Illegal combination using -f and -n\n");
					exit(-1);
				} else if (Flags & UPGRADE_BL) {
					printf("ERROR:  Illegal combination using -f and -l\n");
					exit(-1);
				} else if (Flags & G60DL) {
					printf("ERROR:  Illegal combination using -f and -g\n");
					exit(-1);
				}
				upgradeType = argv[c][2];
				Flags |= FIRMWARE;
				break;
			case 'l':
				// This is a bootloader upgrade
				if (cmdnum == 0) {
					printf("ERROR:  Illegal combination using -l and -n\n");
					exit(-1);
				} else if (Flags & FIRMWARE) {
					printf("ERROR:  Illegal combination using -l and -f\n");
					exit(-1);
				} else if (Flags & G60DL) {
					printf("ERROR:  Illegal combination using -l and -g\n");
					exit(-1);
				}
				Flags |= UPGRADE_BL;
				break;
			case 'n':
				if (Flags & FIRMWARE) {
					printf("ERROR:  Illegal combination using -n and -f\n");
					exit(-1);
				} else if (Flags & UPGRADE_BL) {
					printf("ERROR:  Illegal combination using -n and -l\n");
					exit(-1);
				} else if (Flags & G60DL) {
					printf("ERROR:  Illegal combination using -n and -g\n");
					exit(-1);
				}
				cmdnum = 0;
				break;
			case 'o':
				// Support for the old bootloader
				Flags |= OLD_BL; 
				break;
			case 'g':
				/* Support G60 downloads */
				if (cmdnum == 0) {
					printf("ERROR:  Illegal combination using -g and -n\n");
					exit(-1);
				} else if (Flags & FIRMWARE) {
					printf("ERROR:  Illegal combination using -g and -f\n");
					exit(-1);
				} else if (Flags & UPGRADE_BL)  {
					printf("ERROR:  Illegal combination using -g and -l\n");
					exit(-1);
				}
				Flags |= G60DL;
				break;
			default:
				printf("ERROR:  Illegal option\n");
				PrintUsage(argv[0]);
				exit(-1);
			}
		} else {
			// Don't allow any more options once a filename is reached.
			GetOptions = FALSE;

			// Figure out how many filenames we have and allocate enough pointers for them the first time.
			if (filenum == 0) {
				FileName = (char **) malloc((argc - c) * sizeof(char *));
				if (FileName == NULL) {
					printf("ERROR:  Unable to allocate memory\n");
					exit(-1);
				}
			}

			// Get normal file name.
			FileName[filenum++] = argv[c];
		}
	}

	// Figure out which command header to use (only use g55 if multiple files are listed).
	if (filenum == 1) {
		if (Flags & FIRMWARE) {
			switch (upgradeType) {
			case 'r':
				cmdnum = 7;
				break;
			case 'p':
				cmdnum = 8;
				break;
			default:
				cmdnum = 1;
				break;
			}
		} else if (Flags & UPGRADE_BL) {
			cmdnum = 5;
		} else if (Flags & G60DL) {
			cmdnum = 6;
		}
	} else if (filenum > 1) {
		if (cmdnum == 0) {
			printf("ERROR:  Can only download one normal file\n");
			exit(-1);
		} else
			cmdnum = 3;

		// Firmware and bootloader can only have one file.
		if ((Flags & FIRMWARE) || (Flags & UPGRADE_BL)) {
			printf("ERROR:  Only one upgrade file allowed\n");
			exit(-1);
		}

		// Compact all extra files into one memory file.
		memGCMFile = (FILE *) open_memstream(&memGCMBuf, &memGCMSize);
		if (memGCMFile == NULL) {
			printf("ERROR:  Problem allocating memory for graphics files\n");
			exit(-1);
		}

		NumGCM = filenum - 1;

		// Write how many gcm's are part of this.
		fwrite(&NumGCM, sizeof(WORD), 1, memGCMFile);

		// Write files into the memfile.
		for (i = 1; i < filenum; i++) {
			// Open file.
			if ((tempFile = fopen( FileName[i], "rb" )) == NULL) {
				printf("\nCould not open file %s...aborting download.\n", FileName[i]);
				fclose(memGCMFile);
				exit(-1);
			}

			// Copy over characters.
			while ((c = fgetc(tempFile)) != EOF)
				fputc(c, memGCMFile);

			// Close file.
			fclose(tempFile);
		}

		// Update the memGCM values.
		fflush(memGCMFile);
	}

	// Open the serial port for input and output
	fd = open(PortStr, O_RDWR | O_NOCTTY | O_NONBLOCK); 

	// Check to see if things opened up right.
	if (fd < 0) {
		// Print out error message for port.
		sprintf(PortStr + strlen(PortStr), ": ");
		perror(PortStr);

		if (memGCMFile)
			fclose(memGCMFile);
    
		exit(-1); 
	}

	// Save current port settings (to be restored in the end).
	tcgetattr(fd, &oldtio);

	// Zero out new io block descriptor.
	bzero(&newtio, sizeof(newtio));

	// Setup port.
	newtio.c_cflag = baud | CS8 | CLOCAL | CREAD | CSTOPB;
	if (Flags & HWFC)
		newtio.c_cflag |= CRTSCTS;
	newtio.c_iflag = IGNPAR;
	if (Flags & XON_XOFF)
		newtio.c_iflag |= IXON | IXOFF;
	newtio.c_oflag = 0;

	// Set input mode (non-canonical, no echo,...).
	newtio.c_lflag = 0;

	// Wait .5 seconds for a character.
	newtio.c_cc[VTIME]    = 5;   
	newtio.c_cc[VMIN]     = 1;   

	newtio.c_cc[VSTART]   = XOFF;
	newtio.c_cc[VSTOP]    = XON;

	tcflush(fd, TCIOFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);

	if (filenum != 0) {
		// Open main file.
		if ((tempFile = fopen( FileName[0], "rb" )) == NULL) {
			printf("\nCould not open file %s...aborting download.\n", FileName[0]);
			if (memGCMFile)
				fclose(memGCMFile);
			exit(-1);
		}

		// Copy main file over into buffer.
		memMainFile = (FILE *) open_memstream(&memMainBuf, &memMainSize);
		if (memMainFile == NULL) {
			printf("ERROR:  Problem allocating memory for downloading main file\n");
			fclose(tempFile);
			if (memGCMFile)
				fclose(memGCMFile);
			exit(-1);
		}

		//    DownFile = (FILE *) fopen("temp.fil", "wb");
		DownFile = (FILE *) open_memstream(&tempBuf, &tempSize);
		if (DownFile == NULL) {
			printf("ERROR:  Problem allocating memory for necessary memory file\n");
			fclose(tempFile);
			if (memGCMFile)
				fclose(memGCMFile);
			fclose(memMainFile);
			exit(-1);
		}

		// Copy over characters.
		while ((c = fgetc(tempFile)) != EOF)
			fputc(c, memMainFile);

		// Close file.
		fclose(tempFile);

		// Update the memMain values.
		fflush(memMainFile);

		// Download main file.
		Download(fd, memMainBuf, memMainSize, cmd[cmdnum++], delay, Flags);

		// Download GCM file stuff.
		if (memGCMFile != NULL)
			Download(fd, memGCMBuf, memGCMSize, cmd[cmdnum++], delay, Flags);

		// Close up files.
		fclose(memMainFile);
		if (memGCMFile)
			fclose(memGCMFile);
	}

	// Print out instructions for communications.
	fprintf(stdout, "Entering bi-directional communications mode (Ctrl-C exits)\n");
	fprintf(stdout, "All incoming unprintable characters will be translated to hex values\n");
	fprintf(stdout, "and will be shown as <0x##> with the exception of newline characters\n");
	fprintf(stdout, "which are shown as <\\n> followed by a newline.\n");
	fprintf(stdout, "--------------------------------------------------------------------\n");

   fflush(stdout);

	// Copy over characters received during download.
	if (DownFile) {
		rewind(DownFile);
		while ((c = fgetc(DownFile)) != EOF) {
         buf[0] = c;
         PrettyPrint(buf, 1, LogFile1, LogFile2);
		}
		fclose(DownFile);
	}

	// Install Control-C signal handler.
	OldHand = signal(SIGINT, SigHandler);

	// Save current stdin settings (to be restored in the end).
	tcgetattr(STDIN_FILENO, &oldstdtio);

	// Zero out new io block descriptor.
	bzero(&newstdtio, sizeof(newstdtio));

	newstdtio = oldstdtio;
	newstdtio.c_lflag = IEXTEN | ISIG;

	// Wait .5 seconds for a character.
	newstdtio.c_cc[VTIME]    = 5;
	newstdtio.c_cc[VMIN]     = 1;

	tcflush(STDIN_FILENO, TCIOFLUSH);
	tcsetattr(STDIN_FILENO, TCSANOW, &newstdtio);

	fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);

	// Main loop to check for incoming data.
	while (STOP==FALSE) {
		// Read from the serial port.
		res = read(fd, buf, 255);

		// Check to see if we have something to print out.
		if (res > 0) {
         PrettyPrint(buf, res, LogFile1, LogFile2);
		}

		// Read characters from stdin.
		res = read(STDIN_FILENO, buf, 255);

		// Transmit all characters of header.
		i = res;
		j = 0;
		while (i > 0) {
			k = write(fd, buf + j, i);
			i -= k;
			j += k;
		}
	}

	// Indicate that the program is finished.
	printf("\nDone.\n");

	// Re-install old signal handler.
	signal(SIGINT, OldHand);

	// Set com port as before.
	tcsetattr(STDIN_FILENO,TCSANOW,&oldstdtio);
	tcsetattr(fd,TCSANOW,&oldtio);
	close(fd);
  
	if (LogFile1 != NULL) {
		fclose(LogFile1);
   }

	if (LogFile2 != NULL) {
		fclose(LogFile2);
   }

	exit(0);
}

/*************/
/* WriteByte */
/*************/
void WriteByte(int comFileDesc, BYTE txbyte, BYTE Flags)
{
	if (((Flags & XON_XOFF) && ((txbyte == XON) || (txbyte == XOFF) || (txbyte == K65ESC))) ||
		 ((Flags & OLD_BL & (FIRMWARE | UPGRADE_BL)) && (txbyte == K65ESC))) {
		unsigned char c;
		
		c = K65ESC;
		while(write(comFileDesc, &c, 1) != 1);
		c = txbyte | 0x80;
		while (write(comFileDesc, &c, 1) != 1);
	} else {
		while (write(comFileDesc, &txbyte, 1) != 1);
	}
}

/****************/
/* Download     */
/****************/
void Download(int comFileDesc, BYTE *buf, long len, char *hdr, long delay, BYTE Flags)
{
	int i,j,k;
	BYTE *ptr;
	BYTE txbyte;
	WORD crc;
	long TransLen, DoneLen, tlen;
	long complen = -len;
	struct timespec req;
	struct timespec rem;

	int res;
	unsigned char buf1[256];

	// Transmit all characters of header (should never contain xon/xoff/escape characters)
	TransLen = strlen(hdr);
	DoneLen = 0;
	while (TransLen) {
		tlen = write(comFileDesc, hdr + DoneLen, TransLen);
		TransLen -= tlen;
		DoneLen += tlen;
	}

	// Only transmit if the header was anything useful.
	if (strlen(hdr) != 0) {
		/* Transmit length */
		for (i=4;i;i--) {
			txbyte = (BYTE)(len >> ((i-1)*8));
			WriteByte(comFileDesc, txbyte, Flags);
		}

		/* Transmit length complement */
		for (i=4;i;i--) {
			txbyte = (BYTE)(complen >> ((i-1)*8));
			WriteByte(comFileDesc, txbyte, Flags);
		}
	}

	printf("Transmitted bytes:         ");

	/* Transmit buffer */
	for (i=1,ptr=buf;i <= len;) {
		// Read from the serial port (check to see if we got an xon/xoff).
		res = read(comFileDesc, buf1, 255);

		// Got something, is it xon/xoff?
		// Check to see if we have something to print out.
		if (res > 0) {
         // Save buffer off
         fwrite(buf1, 1, res, DownFile);
			fflush(DownFile);
		}

		// Check for Xon/Xoff character in software flow control, and send escape if
		// needed.
		WriteByte(comFileDesc, *ptr, Flags);
		ptr++;

		fsync(comFileDesc);

		// Update the output on how many bytes have been sent and wait
		// to send the next set.
		if (!(i % BLK_SIZE)) {
			printf("\b\b\b\b\b\b\b%7ld",i);
			fflush(stdout);

			// Only wait if we aren't using flow control (and have a delay).
			if (!((Flags & HWFC) || (Flags & XON_XOFF)) && (delay != 0)) {
				// Wait for some time.
				req.tv_sec = 0;
				req.tv_nsec = delay;

				while (nanosleep(&req, &rem) == -1) {
					req.tv_sec = rem.tv_sec;
					req.tv_nsec = rem.tv_nsec;
				}
			}
		}

		i++;
	}

	// Update how many bytes have been sent for the last time.
	printf("\b\b\b\b\b\b%6ld\n",i-1);

	if (strlen(hdr) != 0) {
		/* Calculate and transmit crc */
		crc = calcCrc16Buf(buf,len);
		for (i=2;i;i--) {
			txbyte = (BYTE)(crc >> ((i-1)*8));
			WriteByte(comFileDesc, txbyte, Flags);
		}
    
		printf("Config file sent with crc 0x%04X.\n",crc);
	} else {
		printf("File sent.\n");
   }
}

/****************/
/* calcCrc16Buf */
/****************/
//Calculates the CRC for a buffer
//This should be the CITT standard CRC
WORD calcCrc16Buf(BYTE *buf,long len)
{
	BYTE *p1;
	long i;
	static WORD crc16b;

	crc16b = 0xffff;

	for ( i=len,p1=buf; i>0; i--)
		crc16b = crc16Tbl[((crc16b>>8) ^ (WORD)*(p1++)) & 0x00ff] ^ (crc16b<<8);
	return(crc16b);
} //calcCrc16Buf

/****************/
/* SigHandler   */
/****************/
void SigHandler(int value)
{
	STOP = TRUE;
}

/****************/
/* PrintUsage   */
/****************/
void PrintUsage(char *prog)
{
	double ver_num = VERSION;
	int i;
	int index = 0;

	for (i = 0; i < strlen(prog); i++)
		if (prog[i] == '/')
			index = i + 1;

	/* usage */
	printf("\n");
	printf("     R45/R55/G55/G60 terminal configuration downloader\n");
	printf("Copyright 1998 - QSI Corporation (Version %.3f for Linux - RNM)\n\n",ver_num);
	printf("Usage:  %s [options] [<file>] [<graphic file>*]\n\n", prog + index);
	printf("\t-cNUM\tPC Com Port #(1-8), default = 1\n");
	printf("\t-bNUM\tBaud rate (1200,2400,4800,9600,19200,38400,57600,115200)\n");
	printf("\t     \tdefault baud rate = 115200\n");
	printf("\t-tNUM\ttime to delay in ns (default is 100000000 - 100 ms)\n");
	printf("\t-d1  \tLog all received data as seen on stdout in data.log (in current directory)\n");
	printf("\t-d2  \tLog all actual received data in actual.log (in current directory)\n");
	printf("\t-r   \tenable hardware (RTS/CTS) flow control\n");
	printf("\t-x   \tenable software flow control\n");
	printf("\t-n   \tset for normal file sending (no headers)\n");
	printf("\t-g   \tset for G60 application file download\n");
	printf("\t-f   \tset for firmware upgrade\n");
	printf("\t-fr  \tset for rammable kernel upgrade\n");
	printf("\t-fp  \tset for paramter block upgrade\n");
	printf("\t-l   \tset for bootloader upgrade\n");
	printf("\t-o   \tset to support bootloader v1.00\n");
	printf("\n\tNo file specifed will simply enter into bi-directional terminal mode\n");
}


