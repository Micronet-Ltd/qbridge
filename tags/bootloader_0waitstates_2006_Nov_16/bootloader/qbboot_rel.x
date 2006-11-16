/**************************/
/* Linker Script File     */
/* for Qbridge Bootloader */
/**************************/

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)

ENTRY(__start)

MEMORY
{
    rom        : o = 0x40000000 , l = 256k
    bootromdata : o = 0x400C2000 , l = 8k    /*note that there are two 8k sectors here... the first starts at 0x400C0000 */
    ram        : o = 0x20000000 , l = 64k
}

/* Address Definitions */
_RamStartAddr        = 0x20000000;
_RamEndAddr          = 0x20010000;

_RomStartAddr        = 0x40000000;
_FirmwareStartAddr   = 0x40002000;
_BootloaderStartAddr = 0x400C2000; /* Store text/data in Bank 1 Sector 1 */

SECTIONS
{
    /*********************************/
    /***** Sections in ROM only ******/
    /*********************************/

    /* Exception Vector Table */
    .vectors _RomStartAddr :
    {
        *(.vectors)
    } >rom = 0xff

    .Copyright ALIGN(4) : { *(.Copyright) } >rom = 0xff
    .rodata ALIGN(4) :
    {
        *(.rodata)
        *(.rodata.str1.4)
    } >rom = 0xff

    /* Init Code */
    .arminit _BootloaderStartAddr : { *(.arminit) } >bootromdata = 0xff

    .text ALIGN(4) :
    {
        _ftext = . ;
        *(.text)
        *(.stub)
        *(.glue_7)
        *(.glue_7t)
    _etext = .;
    } >bootromdata = 0xff

    PROVIDE (etext = .);

    /***********************************/
    /***** Sections in ROM and RAM *****/
    /***********************************/

    _dataROM = ALIGN(4);
    .data _RamStartAddr : AT ( _dataROM )
    {
        _dataRAMBegin = . ;
        /* _fdata = . ; */
        *(.data)
        _dataRAMEnd = ALIGN(4) ;
    } >ram = 0xff

    /*********************************/
    /******* RAM ONLY SECTIONS *******/
    /*********************************/

    /* Uninitialized data has no address in ROM */
    .bss ALIGN(16) (NOLOAD) :
    {
        _bss_start = . ;
        *(.bss)
        *(COMMON)
        _bss_end = ALIGN(4) ;
    } > ram = 0

    .stack ALIGN(8) (NOLOAD) :
    {
        _stackbegin = . ;
        *(.stack)
        _stackend = . ;
        *(.irqstack)
        _irqstackend = . ;
    } > ram = 0

    .BootFlag ALIGN(4) (NOLOAD) :
    {
        *(.BootFlag)
    } > ram = 0

    _heapBegin = ALIGN(16) ;
    end = . ;

} /*SECTIONS*/

/***************************************/
/****** The files to link go here ******/
/***************************************/

