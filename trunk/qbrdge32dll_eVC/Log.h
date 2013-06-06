#pragma once

#include <string>

struct LogLev {
    LogLev(bool showLog_, std::wstring name_) : showLog(showLog_), name(name_) {}

    const bool showLog;
    const std::wstring name;

    static const LogLev ApiResult;
    static const LogLev Debug;
    static const LogLev UdpDebug;
    static const LogLev Error;
    static const LogLev ErrClientDisconnected;

};

class Log
{
public:

    static void Write(LogLev const &lev, LPCWSTR format, ...);
    static void WriteRaw(LogLev const &lev, LPCWSTR msg);

    static void AppendToFile(LPCSTR msg);

    static LPCWSTR GetFilename();
};
