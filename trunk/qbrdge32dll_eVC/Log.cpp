#include "Log.h"
#include <Windows.h>
#pragma warning (disable:4996)

#include <vector>
#include <algorithm>
using namespace std;

const LogLev LogLev::ApiResult(true, L"Api Result");
const LogLev LogLev::Debug(true, L"Debug Msg");
const LogLev LogLev::UdpDebug(true, L"Udp Msg");
const LogLev LogLev::Error(true, L"Error");

//**--**--**--**--**--**
void Log::WriteRaw( LogLev const &lev, LPCWSTR msg )
{
    if (lev.showLog) {
        SYSTEMTIME locTime = {0};
        GetLocalTime(&locTime);       

        vector<char> buf(wcslen(msg) + 256);
            
        sprintf(&buf[0], "%04d-%02d-%02d %02d:%02d:%02d  (thrd=%08x, proc=%08x) %S: %S\r\n", 
            locTime.wYear, locTime.wDay, locTime.wMonth, locTime.wHour, locTime.wMinute, locTime.wSecond,
            GetCurrentThreadId(), GetCurrentProcessId(), lev.name.c_str(), msg); 

#ifdef _DEBUG
        {
            vector <wchar_t> wbuf(buf.size());
            stdext::unchecked_copy(buf.begin(), buf.end(), wbuf.begin());
            OutputDebugString(&wbuf[0]);
        }
#endif
        AppendToFile(&buf[0]);
    }
}

//**--**--**--**--**--**
void Log::Write( LogLev const &lev, LPCWSTR format, ... )
{
    if (lev.showLog) {
        va_list argptr;
        va_start(argptr, format);
        wchar_t buf[512];
        vswprintf_s(buf, _countof(buf), format, argptr);
        va_end(argptr);

        WriteRaw(lev, buf);
    }
}

//**--**--**--**--**--**
void Log::AppendToFile( LPCSTR msg )
{
    FILE *file(_wfopen(GetFilename(), L"ab"));
    if (file) {
        int len = strlen(msg);
        fwrite(msg, sizeof(msg[0]), len, file);
        fclose(file);
    }
}

//**--**--**--**--**--**
LPCWSTR Log::GetFilename()
{
    extern HANDLE myDLLHandle;
    static wchar_t retVal[MAX_PATH] = {0};
    if (!retVal[0]) {
        wchar_t buf[MAX_PATH] = {0};
        GetModuleFileName(reinterpret_cast<HMODULE>(myDLLHandle), buf, _countof(buf));
        wcscpy_s(retVal, _countof(retVal), buf);
        wchar_t *pathEnd = wcsrchr(retVal, '\\');

        if (pathEnd) {
            ++pathEnd;
            *pathEnd = 0;
        }
        wcscat(retVal, L"QBridgeDllLog.txt");
    }
    return retVal;
}