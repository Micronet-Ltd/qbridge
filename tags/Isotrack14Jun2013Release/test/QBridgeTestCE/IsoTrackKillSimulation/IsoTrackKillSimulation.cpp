// IsoTrackKillSimulation.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <exception>
using namespace std;

#include <beCore.h>
#include <beRegistry.h>
#include <TlHelp32.h>
#pragma comment (lib, "Toolhelp.lib")
#include <atlstr.h>
#include <sstream>

#include "..\..\..\qbrdge32dll_eVC\Qbrdge32.h"
#pragma comment (lib, "..\\..\\..\\qbrdge32dll_vs2008\\qbrdge32dll\\TREQ-VM (ARMV4I)\\Release\\Qbrdge32.lib")

#define VALIDATE(exp) Validate(exp, #exp, true)
#define LOG(exp) { ostringstream os; os exp; Log(os.str().c_str()); }

void Log(LPCSTR msg) {
    int len = strlen(msg);
    cout << msg;

    CStringW ods(msg);
    OutputDebugString(ods);
}

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
                LOG( << "        Terminated process " << name << " with process ID of " << pe.th32ProcessID << "  result=" << result << endl);
            }
            pe.dwSize = sizeof(pe);
        } while (Process32Next(d.get(), &pe));
    }
}

short Validate(short code, char const *expression, bool doThrow) {
    if (code == ERR_CLIENT_DISCONNECTED) {
        LOG( << "        ERR_CLIENT_DISCONNECTED from expression " << expression << endl);
        KillDriver();
        if (doThrow) { throw exception(); }
    } else if (code < 0) {
        LOG( << "        Expression " << expression << " returned invalid negative value " << code << endl);
        if (doThrow) { throw exception(); }
    } else if (code > 127) {
        LOG( << "        Expression " << expression << " returned error code " << code << endl);
        if (doThrow) { throw exception(); }
    } 
    return code;
}

short CloseRP1210Handle(short handle) {
    LOG( << "    Closing connection..." << endl);
    short retVal = Validate (RP1210_ClientDisconnect(handle), "RP1210_ClientDisconnect(handle)", false);
    LOG( << "    Handle closed" << endl);
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
    LOG( << "    Connecting... " << endl);
    RP1210Handle client1;
    //short client1;
    client1.reset(VALIDATE(RP1210_ClientConnect(NULL, QBRIDGE_COM7, QBRIDGE_J1708_PROTOCOL, 0,0, 0)));
    //client1 = VALIDATE(RP1210_ClientConnect(NULL, QBRIDGE_COM7, QBRIDGE_J1708_PROTOCOL, 0,0, 0));
    LOG( << "    Received connection with ID " << client1.get() << endl);
    
    LOG( << "    Setting filters to pass..." << endl);
    VALIDATE(RP1210_SendCommand(CMD_ALL_FILTERS_PASS, client1.get(), "", 0)); 
    LOG( << "    Filters set to pass" << endl);

    LOG( << "    Reading messages..." << endl);
    for (int i = 0; i < 3; i++) {
        LOG( << "        Reading message " << i << "..." << endl);
        char buffer[256];
        short readResult = My_RP1210_ReadMessage(client1.get(), buffer, _countof(buffer), false);
        if (readResult < 0) { VALIDATE(-readResult); }
        
        LOG( << "        Read message " << i << "  Received " << readResult << " bytes" << endl);
    }
    LOG( << "    Done Reading messages" << endl);

    //CloseRP1210Handle(client1);
}

void DoStuff() {

    for (int i = 0; i < INT_MAX; i++) {
        LOG( << "Starting connection loop " << i << endl);
        try {
            DoConnection();
            LOG( << "Loop " << i << " ended normally" << endl);
        } catch(...) {
            LOG( << "Loop " << i << " ended with exception.  Waiting to reconnect..." << endl);
            Sleep(1000);
            LOG( << "Done waiting " << endl);
        }
    }
}

//-------------------------------------------------------------------------------------------------------------------------------

DWORD ThreadProc(LPVOID lpParameter) {
    short client1 = (short)lpParameter;

    LOG( << "    Reading in thread..." << endl);
    char buf[256];
    short result = RP1210_ReadMessage(client1, buf, _countof(buf), true);
    LOG( << "    Read complete.  Returned " << result << ".  " << endl);

    return 0;
}

void DoConnection2() {
    struct ScopeGuard {
        ScopeGuard() : commit(false) {}
        ~ScopeGuard() {
            if (!commit) { 
                LOG( << "Killing Driver.  Probably due to error in earlier command" << endl);
                KillDriver();
            }
        }
        bool commit;
    } sg;

    LOG( << "    Connecting... " << endl);
    RP1210Handle client1;
    //short client1;
    client1.reset(VALIDATE(RP1210_ClientConnect(NULL, QBRIDGE_COM7, QBRIDGE_J1708_PROTOCOL, 0,0, 0)));
    //client1 = VALIDATE(RP1210_ClientConnect(NULL, QBRIDGE_COM7, QBRIDGE_J1708_PROTOCOL, 0,0, 0));
    LOG( << "    Received connection with ID " << client1.get() << endl);

    LOG( << "    Setting filters to pass..." << endl);
    VALIDATE(RP1210_SendCommand(CMD_ALL_FILTERS_PASS, client1.get(), "", 0)); 
    LOG( << "    Filters set to pass" << endl);

    LOG( << "    Doing dummy commands..." << endl);
    for (int i = 0; i < 200; i++) {
        LOG( << i << " ");
        VALIDATE(RP1210_SendCommand(CMD_ALL_FILTERS_PASS, client1.get(), "", 0)); 
        char buf[256];
        short result = RP1210_ReadMessage(client1.get(), buf, _countof(buf), false);
        VALIDATE(-result);
    }
    LOG( << endl << "    ...done" << endl);

    sg.commit = true;

    LOG( << "    Spawning Read Thread" << endl);
    beijer::EventHandle thrdH (CreateThread(NULL, 0, &::ThreadProc, (LPVOID)client1.get(), 0, NULL));
    if (!thrdH) {
        LOG( << "Error spawning thread" << endl);
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
        LOG( << "Starting connection loop " << i << endl);
        try {
            DoConnection2();
            LOG( << "Loop " << i << " ended normally" << endl);
        } catch(...) {
            LOG( << "Loop " << i << " ended with exception.  Waiting to reconnect..." << endl);
            Sleep(1000);
            LOG( << "Done waiting " << endl);
        }
    }
}


int _tmain(int argc, _TCHAR* argv[])
{
    int pri = beijer::RegistryKey::ReadDWORDValue(HKEY_LOCAL_MACHINE, L"Drivers\\Qbridge", L"Priority", 251);
    CeSetThreadPriority(GetCurrentThread(), pri);
     DoStuff2();

    return 0;
}

