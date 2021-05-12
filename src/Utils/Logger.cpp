#include "Logger.h"

#include <locale>
#include <codecvt>

using rf::Logger;

Logger::Logger()
{

}

Logger::~Logger()
{
   Logger::Close();
}

void Logger::Telemetry(uint16_t tId, double value)
{
  if(telemetry)
    telemetry->Add(tId, value);
}

bool Logger::CreateTelemetryChannel(const tXCHAR *i_pName, tDOUBLE i_dbMin,  tDOUBLE  i_dbAlarmMin,
                         tDOUBLE  i_dbMax, tDOUBLE  i_dbAlarmMax, tBOOL  i_bOn, tUINT16      *o_pID )
{
  if(telemetry)
    return telemetry->Create(i_pName, i_dbMin,i_dbAlarmMin,i_dbMax,i_dbAlarmMax,i_bOn,o_pID);
  return false;
}

bool Logger::Trace( tUINT16            i_wTrace_ID, eP7Trace_Level  i_eLevel, IP7_Trace::hModule i_hModule,
                tUINT16    i_wLine, const char   *i_pFile, const char  *i_pFunction, const tXCHAR   *i_pFormat,
                ...)
{
    if(trace)
    {
      va_list args;
      va_start(args, i_pFormat);
      bool res = trace->Trace_Embedded(i_wTrace_ID, i_eLevel, i_hModule, i_wLine, i_pFile, i_pFunction, &i_pFormat, &args);
      va_end(args);
      return res;
    } 
    return false;
}


bool Logger::Create(XSTRING initParams, XSTRING traceInstanceName, XSTRING telemetryInstanceName)
{
    Close();

    P7_Set_Crash_Handler();

    client.reset(P7_Create_Client(initParams.c_str()));
    if(!client)
      return false;

    trace.reset(P7_Create_Trace(client.get(), traceInstanceName.c_str()));
    if(!trace)
      return false;

    telemetry.reset(P7_Create_Telemetry(client.get(), telemetryInstanceName.c_str()));
    if(!telemetry)
        return false;
    return true;
}

bool Logger::Share(XSTRING clientShareName, XSTRING traceShareName, XSTRING telemetryShareName)
{
    if(!client)
      return false;
    client->Share(clientShareName.c_str());

    if(!trace)
      return false;
    trace->Share(traceShareName.c_str());

    if(!telemetry)
        return false;
    return telemetry->Share(telemetryShareName.c_str());
    
    return true;
}


bool Logger::ConnectToShare(XSTRING clientShareName, XSTRING traceShareName, XSTRING telemetryShareName)
{
    Close();
    client.reset(P7_Get_Shared(clientShareName.c_str()));
    if(!client)
      return false;
    
    trace.reset(P7_Get_Shared_Trace(traceShareName.c_str()));
    if(!trace)
      return false;
    
    telemetry.reset(P7_Get_Shared_Telemetry(telemetryShareName.c_str()));
    if(!telemetry)
        return false;
    
    return true;
}


void Logger::Close()
{
    if (telemetry)
    {
        telemetry->Release();
        telemetry.release();
    }

    if (trace)
    {
        trace->Release();
        trace.release();
    }

    if (client)
    {
        client->Release();
        client.release();
    }
}

std::wstring Logger::StrToWstr(std::string str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}
