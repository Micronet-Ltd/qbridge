// CopilotBusySimulator2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <becore.h>

beijer::EventHandle fileMapping;
beijer::FileHandle hFile;

BYTE *data = NULL;

FILE *otherFile;

BYTE someBuffer[65536];
DWORD const size = 512*1024*1024;
DWORD const otherSize = 15 * 1024 * 1024;

using namespace std;

//**--**--**--**--**--**
void CreateFileOnFlashCard() 
{
    cout << "Creating big file!" << endl;

    hFile.reset(CreateFile(L"\\Harddisk2\\BigFile.dat", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
    ASSERT(hFile);
    fileMapping.reset(CreateFileMapping(hFile.get(), NULL, PAGE_READWRITE | SEC_COMMIT, 0, size, NULL));
    ASSERT(fileMapping);

    data = reinterpret_cast<BYTE *>(MapViewOfFile(fileMapping.get(), FILE_MAP_ALL_ACCESS, 0, 0, size)); 
    int err = GetLastError();
    ASSERT(data);

    cout << "Creating other file" << endl;
    otherFile = fopen("\\Harddisk2\\OtherFile.dat", "wb");
    ASSERT (otherFile);
    char buf[4096];
    for (int i = 0; i < _countof(buf); i++) {
        buf[i] = static_cast<char>(i);
    }

    for (int i = 0; i < otherSize; i += sizeof(buf) ) {
        fwrite(buf, _countof(buf), sizeof(buf[0]), otherFile);
    }

    fclose(buf);

    otherFile = fopen("\\HardDisk2\\Otherfile.dat", "rb");
    ASSERT(otherFile);
}

DWORD Random(DWORD max) {
    union {
        byte bits[4];
        DWORD value;
    } v;

    WORD r = rand();
    memcpy(v.bits, &r, sizeof(r));
    r = rand();
    memcpy(v.bits+sizeof(r), &r, sizeof(r));

    return v.value % max;
}

void BusyLoop(DWORD ms) {
    for (DWORD start = GetTickCount(); GetTickCount() < start + ms; ) { }
}

void ReadFlashData() 
{
    cout << "Flash read loop" << endl;
    for (int i = 0; i < 100; i++) {
        DWORD readSize = Random(32768);
        memcpy(someBuffer, data + Random(size - readSize), readSize);

        BusyLoop(100);
        readSize = Random(32768);
        fseek(otherFile, Random(otherSize-readSize), SEEK_SET);
        fread(someBuffer, sizeof(someBuffer[0]), readSize, otherFile);

        BusyLoop(100);
    }
}

void UseCPU() 
{
    cout << "CPU loop" << endl;
    BusyLoop(2000);
}

DWORD ThreadProc(LPVOID) {
    while (true) {}
}

int _tmain(int argc, _TCHAR* argv[])
{
    srand(GetTickCount());
    CreateFileOnFlashCard();

    CreateThread(NULL, 0, &ThreadProc, NULL, 0, NULL);
    
    while (true) {
        ReadFlashData();
        UseCPU();
    }
}

