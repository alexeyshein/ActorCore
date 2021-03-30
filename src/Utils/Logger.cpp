#include "Logger.h"

//#include <locale>
#include <codecvt>

using rf::Logger;

Logger::Logger():
  clientInstanceName(L"ActorSystem log client")
, traceInstanceName(L"Actor System trace channel")
, telemetryInstanceName(L"Actor System Telemetry channel")
{

}

Logger::~Logger()
{
   Logger::Close();
}


bool Logger::CreateAndShare(std::wstring initParams)
{
    Close();

    P7_Set_Crash_Handler();

    client.reset(P7_Create_Client(initParams.c_str()));
    if(!client)
      return false;
    client->Share(clientInstanceName.c_str());

    trace.reset(P7_Create_Trace(client.get(), traceInstanceName.c_str()));
    if(!trace)
      return false;
    trace->Share(traceInstanceName.c_str());

    telemetry.reset(P7_Create_Telemetry(client.get(), telemetryInstanceName.c_str()));
    if(!telemetry)
        return false;
    telemetry->Share(telemetryInstanceName.c_str());
}


bool Logger::ConnectToShare()
{
    Close();
    client.reset(P7_Get_Shared(clientInstanceName.c_str()));
    if(!client)
      return false;
    
    trace.reset(P7_Get_Shared_Trace(traceInstanceName.c_str()));
    if(!trace)
      return false;
    
    telemetry.reset(P7_Get_Shared_Telemetry(telemetryInstanceName.c_str()));
    if(!telemetry)
        return false;
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
