#pragma once

#include <memory>
#include <string>

#include <P7_Trace.h>
#include <P7_Telemetry.h>

namespace rf{
class Logger
{
    public:
    Logger();
    ~Logger();
    bool CreateAndShare(std::wstring initParams);
    bool ConnectToShare();
    static std::wstring StrToWstr(std::string); // helper function
    protected:
        void Close();

    private:
    std::wstring clientInstanceName;
    std::wstring traceInstanceName;
    std::wstring telemetryInstanceName;

    public:
    std::unique_ptr<IP7_Client> client;
    std::unique_ptr<IP7_Trace> trace;
    std::unique_ptr<IP7_Telemetry> telemetry;
};
}