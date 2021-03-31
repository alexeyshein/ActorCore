#pragma once

#include <memory>
#include <string>

#include <P7_Trace.h>
#include <P7_Telemetry.h>


////////////////////////////////////////////////////////////////////////////////
#define DELIVER(i_wID, i_eLevel, i_hModule, ...) Trace(i_wID,\
                                                                     i_eLevel,\
                                                                     i_hModule,\
                                                                     (tUINT16)__LINE__,\
                                                                     __FILE__,\
                                                                     __FUNCTION__,\
                                                                     __VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////
#define QTRACE(i_wID, i_hModule, ...)    DELIVER(i_wID,\
                                                                  EP7TRACE_LEVEL_TRACE,\
                                                                  i_hModule,\
                                                                  __VA_ARGS__)

#define TRACE(i_hModule, ...)            QTRACE(0,\
                                                                 i_hModule,\
                                                                 __VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////
#define QDEBUG(i_wID, i_hModule,  ...)    DELIVER(i_wID,\
                                                                  EP7TRACE_LEVEL_DEBUG,\
                                                                  i_hModule,\
                                                                  __VA_ARGS__)

#define DEBUG(i_hModule,  ...)            QDEBUG(0,\
                                                                 i_hModule,\
                                                                 __VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////
#define QINFO(i_wID, i_hModule,  ...)     DELIVER(i_wID,\
                                                                  EP7TRACE_LEVEL_INFO,\
                                                                  i_hModule,\
                                                                  __VA_ARGS__)

#define INFO(i_hModule,  ...)             QINFO(0,\
                                                                i_hModule,\
                                                                __VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////
#define QWARNING(i_wID, i_hModule,  ...)  DELIVER(i_wID,\
                                                                  EP7TRACE_LEVEL_WARNING,\
                                                                  i_hModule,\
                                                                  __VA_ARGS__)

#define WARNING(i_hModule,  ...)          QWARNING(0,\
                                                                   i_hModule,\
                                                                   __VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////
#define QERROR(i_wID, i_hModule,  ...)    DELIVER(i_wID,\
                                                                  EP7TRACE_LEVEL_ERROR,\
                                                                  i_hModule,\
                                                                  __VA_ARGS__)

#define ERROR(i_hModule,  ...)            QERROR(0,\
                                                                 i_hModule,\
                                                                 __VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////
#define QCRITICAL(i_wID, i_hModule,  ...) DELIVER(i_wID,\
                                                                  EP7TRACE_LEVEL_CRITICAL,\
                                                                  i_hModule,\
                                                                  __VA_ARGS__)

#define CRITICAL(i_hModule,  ...)         QCRITICAL(0,\
                                                                    i_hModule,\
                                                                    __VA_ARGS__)
////////////////////////////////////////////////////////////////////////////////




namespace rf{
class Logger
{
    public:
    Logger();
    ~Logger();
    bool CreateAndShare(std::wstring initParams);
    bool ConnectToShare();

    void Telemetry(uint16_t tId, double value);
    bool CreateTelemetryChannel(const tXCHAR *i_pName, 
                         tDOUBLE       i_dbMin,
                         tDOUBLE       i_dbAlarmMin,
                         tDOUBLE       i_dbMax,
                         tDOUBLE       i_dbAlarmMax,
                         tBOOL         i_bOn,
                         tUINT16      *o_pID );


    bool Trace( tUINT16            i_wTrace_ID,   
                eP7Trace_Level     i_eLevel, 
                IP7_Trace::hModule i_hModule,
                tUINT16            i_wLine,
                const char        *i_pFile,
                const char        *i_pFunction,
                const tXCHAR      *i_pFormat,
                ...);

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