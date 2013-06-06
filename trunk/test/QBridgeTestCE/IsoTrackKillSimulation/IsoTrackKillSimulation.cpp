// IsoTrackKillSimulation.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <exception>
using namespace std;

#include <beCore.h>
#include <TlHelp32.h>
#pragma comment (lib, "Toolhelp.lib")
#include <atlstr.h>

#include "..\..\..\qbrdge32dll_eVC\Qbrdge32.h"
#pragma comment (lib, "..\\..\\..\\qbrdge32dll_vs2008\\qbrdge32dll\\TREQ-VM (ARMV4I)\\Release\\Qbrdge32.lib")

#define VALIDATE(exp) Validate(exp, #exp, true)

void KillDriver() {
    typedef beijer::unique_handle<beijer::HandleTraits<HANDLE, INVALID_HANDLE_VALUE, BOOL (WINAPI *)(HANDLE), CloseToolhelp32Snapshot> > ToolHelpSnapshotHandle;
    CString toKill = L"qbridgewincedriver.exe";

    ToolHelpSnapshotHandle d(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));

    PROCESSENTRY32 pe = {0};
    pe.dwSize = sizeof(pe);
    if (Process32First(d.get(), &pe)) {
        do {
            CString name = pe.szExeFile;
            name.MakeLower();
            if (name.Right(toKill.GetLength()) == toKill) {
                BOOL result = TerminateProcess(reinterpret_cast<HANDLE>(pe.th32ProcessID), 1);
                cout << "        Terminated process " << name << " with process ID of " << pe.th32ProcessID << "  result=" << result << endl;
            }
            pe.dwSize = sizeof(pe);
        } while (Process32Next(d.get(), &pe));
    }
}

short Validate(short code, char const *expression, bool doThrow) {
    if (code == ERR_CLIENT_DISCONNECTED) {
        cout << "        ERR_CLIENT_DISCONNECTED from expression " << expression << endl;
        KillDriver();
        if (doThrow) { throw exception(); }
    } else if (code < 0) {
        cout << "        Expression " << expression << " returned invalid negative value " << code << endl;
        if (doThrow) { throw exception(); }
    } else if (code > 127) {
        cout << "        Expression " << expression << " returned error code " << code << endl;
        if (doThrow) { throw exception(); }
    } 
    return code;
}

short CloseRP1210Handle(short handle) {
    cout << "    Closing connection..." << endl;
    short retVal = Validate (RP1210_ClientDisconnect(handle), "RP1210_ClientDisconnect(handle)", false);
    cout << "    Handle closed" << endl;
    return retVal;
}

typedef beijer::unique_handle<beijer::HandleTraits<short, -1, short (*)(short), CloseRP1210Handle> > RP1210Handle;

short My_RP1210_ReadMessage(short nClientID, char *fpchAPIMessage, short nBufferSize, short nBlockOnRead) {
    static int counter = 0;
    static const int errFrq = 17;

    counter = (counter + 1) % errFrq;
    if (counter == 0) { 
        return -ERR_CLIENT_DISCONNECTED; 
    }

    return RP1210_ReadMessage(nClientID, fpchAPIMessage, nBufferSize, nBlockOnRead);
}

void DoConnection() {
    cout << "    Connecting... " << endl;
    RP1210Handle client1;
    //short client1;
    client1.reset(VALIDATE(RP1210_ClientConnect(NULL, QBRIDGE_COM7, QBRIDGE_J1708_PROTOCOL, 0,0, 0)));
    //client1 = VALIDATE(RP1210_ClientConnect(NULL, QBRIDGE_COM7, QBRIDGE_J1708_PROTOCOL, 0,0, 0));
    cout << "    Received connection with ID " << client1.get() << endl;
    
    cout << "    Setting filters to pass..." << endl;
    VALIDATE(RP1210_SendCommand(CMD_ALL_FILTERS_PASS, client1.get(), "", 0)); 
    cout << "    Filters set to pass" << endl;

    cout << "    Reading messages..." << endl;
    for (int i = 0; i < 3; i++) {
        cout << "        Reading message " << i << "..." << endl;
        char buffer[256];
        short readResult = My_RP1210_ReadMessage(client1.get(), buffer, _countof(buffer), false);
        if (readResult < 0) { VALIDATE(-readResult); }
        
        cout << "        Read message " << i << "  Received " << readResult << " bytes" << endl;
    }
    cout << "    Done Reading messages" << endl;

    //CloseRP1210Handle(client1);
}

void DoStuff() {

    for (int i = 0; i < INT_MAX; i++) {
        cout << "Starting connection loop " << i << endl;
        try {
            DoConnection();
            cout << "Loop " << i << " ended normally" << endl;
        } catch(...) {
            cout << "Loop " << i << " ended with exception.  Waiting to reconnect..." << endl;
            Sleep(1000);
            cout << "Done waiting " << endl;
        }
    }
}

//-------------------------------------------------------------------------------------------------------------------------------

DWORD ThreadProc(LPVOID lpParameter) {
    short client1 = (short)lpParameter;

    cout << "    Reading in thread..." << endl;
    char buf[256];
    short result = RP1210_ReadMessage(client1, buf, _countof(buf), true);
    cout << "    Read complete.  Returned " << result << ".  " << endl;

    return 0;
}

void DoConnection2() {
    cout << "    Connecting... " << endl;
    RP1210Handle client1;
    //short client1;
    client1.reset(VALIDATE(RP1210_ClientConnect(NULL, QBRIDGE_COM7, QBRIDGE_J1708_PROTOCOL, 0,0, 0)));
    //client1 = VALIDATE(RP1210_ClientConnect(NULL, QBRIDGE_COM7, QBRIDGE_J1708_PROTOCOL, 0,0, 0));
    cout << "    Received connection with ID " << client1.get() << endl;

    cout << "    Setting filters to pass..." << endl;
    VALIDATE(RP1210_SendCommand(CMD_ALL_FILTERS_PASS, client1.get(), "", 0)); 
    cout << "    Filters set to pass" << endl;

    cout << "    Spawning Read Thread" << endl;
    beijer::EventHandle thrdH (CreateThread(NULL, 0, &::ThreadProc, (LPVOID)client1.get(), 0, NULL));
    if (!thrdH) {
        cout << "Error spawning thread" << endl;
        throw exception(); 
    }

    Sleep(5000);

    static int cnt = 0;
    cnt ++;
    if (cnt %2 == 0) {    
        KillDriver();
    }

}

void DoStuff2() {
    for (int i = 0; i < INT_MAX; i++) {
        cout << "Starting connection loop " << i << endl;
        try {
            DoConnection2();
            cout << "Loop " << i << " ended normally" << endl;
        } catch(...) {
            cout << "Loop " << i << " ended with exception.  Waiting to reconnect..." << endl;
            Sleep(1000);
            cout << "Done waiting " << endl;
        }
    }
}


int _tmain(int argc, _TCHAR* argv[])
{
    __try {
        DoStuff2();
    } __finally {
        cout << "DEAD!";
    }

    return 0;
}

