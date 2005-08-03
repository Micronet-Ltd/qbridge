/*******************************************************************/
/*                                                                 */
/* File:  build.c                                                  */
/*                                                                 */
/* Description:  Build Date/Time information                       */
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
/* xx-xxx-xx    who          1.0     1st Release                   */
/*******************************************************************/

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

/************/
/* TypeDefs */
/************/

/**********/
/* Macros */
/**********/

/********************/
/* Global Variables */
/********************/
unsigned char BuildDate[] = "Built " __DATE__ " at " __TIME__ "\r\n";

/************/
/* Routines */
/************/

