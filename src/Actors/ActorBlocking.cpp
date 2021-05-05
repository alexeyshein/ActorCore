#include "ActorBlocking.h"
#include <json.hpp>
#include <chrono>
#include <iostream>

#include "Logger.h"

using rf::ActorBlocking;
using rf::IPort;
using rf::Logger;

using nlohmann::json;

ActorBlocking::ActorBlocking(const std::string& id) 
: ActorLocal(id)
, minLoopTimeMks(100)
{
	_type = "ActorBlocking";
	
	//std::wstring telemetryName{Logger::StrToWstr(id)+L"_isProcess"};
	std::string telemetryName{ id + "_isProcess" };

    logger->CreateTelemetryChannel(telemetryName.c_str(), 0,-1,2,1,true, &teleChannelIsProcessing);
}

 ActorBlocking::~ActorBlocking()
{
 	Deactivate();
}


bool ActorBlocking::Init(const json& config)
{
	if(!ActorLocal::Init(config))
		return false;
	
	if(config.contains("minLoopTimeMks"))
     if(config.at("minLoopTimeMks").is_number())
      minLoopTimeMks = config.at("minLoopTimeMks").get<size_t>();

	return true;
}

std::variant<std::monostate, bool, int, double, std::string> ActorBlocking::GetProperty(const std::string& propertyName)
{
  if(propertyName.compare("minLoopTimeMks") == 0)
    return static_cast<int>(minLoopTimeMks);
  return ActorLocal::GetProperty(propertyName);
}

bool ActorBlocking::SetProperty(const std::string& propertyName, int value) 
{
    if(propertyName.compare("minLoopTimeMks"))
    {
      minLoopTimeMks = value ;
      return true;
    }
  return ActorLocal::SetProperty(propertyName, value);
}

void ActorBlocking::Activate()
{
	ActorLocal::Activate();
  	_flagStop = false;
	if (_processingLoopThread.joinable())
	{
		return; //Уже запущен
	}

	_processingLoopThread = std::thread(&ActorBlocking::processingLoop, this);
	return;
}
void ActorBlocking::Deactivate()
{
	ActorLocal::Deactivate();
    _flagStop = true;
	//поток контроля состояния каналов
	if (_processingLoopThread.joinable()) //Запущен?
		_processingLoopThread.join();
	return ;
}


json ActorBlocking::GetStatus()
{
	json res = ActorLocal::GetStatus();
	// res["Recived frames"] = recivedFramesByPeriod;
	// recivedFramesByPeriod = 0;
	// res["Processed frames"] = processedFramesByPeriod;
	// processedFramesByPeriod = 0;
  
	return res;
}

void ActorBlocking::processingLoop()
{
	std::chrono::microseconds minLoopTime(minLoopTimeMks);
	while (!_flagStop)
	{
			auto timeout = std::chrono::system_clock::now()+minLoopTime;
			logger->Telemetry(teleChannelIsProcessing, 1);
			bool res =  Process();
			logger->Telemetry(teleChannelIsProcessing, 0);
			std::this_thread::sleep_until(timeout);
	}
}




