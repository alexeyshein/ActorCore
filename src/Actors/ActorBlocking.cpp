#include "ActorBlocking.h"
#include <json.hpp>
#include <chrono>
#include <iostream>
using rf::ActorBlocking;
using rf::IPort;

using nlohmann::json;

ActorBlocking::ActorBlocking(const std::string& id) 
: ActorLocal(id)
, minLoopTimeMks(100)
{
	_type = "ActorBlocking";
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

std::variant<bool, int, double> ActorBlocking::GetProperty(const std::string&)
{
	return std::variant<bool, int, double>();
}


void ActorBlocking::OnActivate()
{
	_flagStop = false;
	if (_processingLoopThread.joinable())
	{
		return; //Уже запущен
	}

	_processingLoopThread = std::thread(&ActorBlocking::processingLoop, this);
	return;
}

/*!
\brief Останов  потока циклической обработки.
\return успех
*/
void ActorBlocking::OnDeactivate()
{
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
			bool res =  Process();
			std::this_thread::sleep_until(timeout);
	}
}




