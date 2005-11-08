/**********************/
/* Linker Script File */
/* for STR712 QBridge */
/* Execute from SRAM  */
/**********************/

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)

ENTRY(__start)

MEMORY
{
	rom 	   : o = 0x40000000 , l = 256k
	ram        : o = 0x20000000 , l = 64k
}

/* Address Definitions */
_RamStartAddr       = 0x20000000;
_RamEndAddr         = 0x20010000;

_RomStartAddr       = 0x40000000;

/* Firmware linked to RAM */
_FirmwareStartAddr  = 0x20002000;

SECTIONS
{
	/***********************************/
	/***** Sections in "ROM" only ******/
	/***********************************/

	/* Firmware header */
	.firmwarehdr _FirmwareStartAddr :
	{
		*(.firmwarehdr)
	} >ram=0xff

	.Copyright ALIGN(4) : { *(.Copyright) } >ram=0xff
	.rodata ALIGN(4) :
	{
		*(.rodata)
		*(.rodata.str1.4)
	} >ram=0xff

	/* Init Code */
	.arminit ALIGN(4) : { *(.arminit) } >ram=0xff

  	.text ALIGN(4) :
  	{
		_ftext = . ;
		*(.text)
		*(.stub)
		*(.glue_7)
		*(.glue_7t)
		_etext = .;
	} >ram=0xff

	PROVIDE (etext = .);
	/*************************************/
	/***** Sections in "ROM" and RAM *****/
	/*************************************/

	_vectorROM = ALIGN(4);
	/* Exception Vector Table */
	.vectors _RamStartAddr : AT ( _vectorROM )
	{
	        _vectorRAMBegin = . ;
		*(.vectors)
		. = ALIGN(16) ; /* Keep the following .data section 16-byte aligned */
		_vectorRAMEnd = . ;
	} >ram=0xff

	_dataROM = LOADADDR(.vectors) + SIZEOF(.vectors) ;
	. = _vectorROM + SIZEOF(.vectors) ;
	.data ALIGN(0x2000) : AT ( _dataROM )
	{
		_dataRAMBegin = . ;
		/* _fdata = . ; */
 		*(.data)
		_dataRAMEnd = . ;
	} >ram=0xff

	/*********************************/
	/******* RAM ONLY SECTIONS *******/
	/*********************************/

	/* Uninitialized data has no address in ROM */
	.bss ALIGN(4) (NOLOAD) :
	{
		_bss_start = . ;
		*(.bss)
		*(COMMON)
		_bss_end = ALIGN(4) ;
	} > ram=0

	.stack ALIGN(8) (NOLOAD) :
	{
		_stackbegin = . ;
		*(.stack)
		_stackend = . ;
		*(.irqstack)
		_irqstackend = . ;				
	} > ram=0
	_heapBegin = ALIGN(16) ;
	end = . ;



} /*SECTIONS*/
