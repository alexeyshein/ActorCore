#include "ActorBlocking.h"
#include <json.hpp>
#include <chrono>
#include <iostream>
using rf::ActorBlocking;
using rf::IPort;

using nlohmann::json;

ActorBlocking::ActorBlocking(std::string id) 
: ActorLocal(id)
, minLoopTimeMks(100)
{
	_typeId = "ActorBlocking";
}

 ActorBlocking::~ActorBlocking()
{
 	Deactivate();
}


bool ActorBlocking::Init(json actorConfig)
{
	try {
		minLoopTimeMks = actorConfig["minLoopTimeMks"].get<size_t>();
	}
	catch (json::parse_error& e) {
		std::cout << "Json Config for ActorBlocking don`t contain ""minLoopTimeMks"" value" << e.what();
		return false;
	}//try...
	return false;
}

std::variant<bool, int, double> ActorBlocking::GetProperty(std::string)
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




