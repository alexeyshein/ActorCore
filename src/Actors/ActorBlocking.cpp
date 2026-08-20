#include "ActorBlocking.h"
#include <json.hpp>
#include <chrono>
#include <iostream>

#include "Logger.h"
#include "RuntimeClock.hpp"
#include "RuntimeTypes.hpp"

using rf::ActorBlocking;
using rf::IPort;
using rf::Logger;

using nlohmann::json;

ActorBlocking::ActorBlocking(const std::string& id, IUnit* parent)
	: ActorLocal(id, parent)
, minLoopTimeMks(100)
, _flagStop(true)
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



json ActorBlocking::Configuration()
{
	auto config = ActorLocal::Configuration();
	auto& configProps = config["properties"];
	//auto& configProps = config.at("properties").get_ref<json::object_t&>();
	configProps["minLoopTimeMks"] = minLoopTimeMks;
	return  config;
}

bool ActorBlocking::SetProperties(const json& properties)
{
	if (!ActorLocal::SetProperties(properties))
		return false;
	if (properties.contains("minLoopTimeMks"))
		if (properties.at("minLoopTimeMks").is_number())
		{
			minLoopTimeMks = properties.at("minLoopTimeMks").get<size_t>();
			if (this->IsActive()) // restart processing loop with new minLoopTimeMks
			{
				this->Deactivate();
				this->Activate();
			}
		}
			
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
	std::lock_guard<std::mutex>  lock(mutexActivateDeactivate);
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
	std::lock_guard<std::mutex>  lock(mutexActivateDeactivate);
    _flagStop = true;
	//поток контроля состояния каналов
	if (_processingLoopThread.joinable()) //Запущен?
		_processingLoopThread.join();
	//должен запускаться после останова loop
	ActorLocal::Deactivate();

	return ;
}

void ActorBlocking::WaitForTasks()
{
	Deactivate();
}

json ActorBlocking::GetStatus()
{
	json res = ActorLocal::GetStatus();

	bool processing = _runtimeStats.isProcessingNow.load(std::memory_order_relaxed);

	// --- runtime state ---
	RuntimeState state = RuntimeState::Idle;
	if (!_flagActive)
		state = RuntimeState::Inactive;
	else if (_flagStop)
		state = RuntimeState::Stopping;
	else if (processing)
		state = RuntimeState::Processing;

	res["runtimeState"] = ToString(state);
	res["isProcessingNow"] = processing;
	res["minLoopTimeMks"] = static_cast<int>(minLoopTimeMks);

	return res;
}

void ActorBlocking::processingLoop()
{
	std::chrono::microseconds minLoopTime(minLoopTimeMks);
	while (!_flagStop)
	{
		auto timeout = std::chrono::system_clock::now() + minLoopTime;

		// --- NEW: record process start ---
		_runtimeStats.isProcessingNow.store(true, std::memory_order_relaxed);
		uint64_t startTs = SteadyTimeUs();
		_runtimeStats.lastProcessStartTs.store(startTs, std::memory_order_relaxed);

		logger->Telemetry(teleChannelIsProcessing, 1);

		try
		{
			bool res = Process();

			// --- record success ---
			_runtimeStats.processedTotal.fetch_add(1, std::memory_order_relaxed);
		}
		catch (const std::exception& e)
		{
			// ---  record error ---
			_runtimeStats.errorTotal.fetch_add(1, std::memory_order_relaxed);
			_runtimeStats.lastErrorTs.store(SteadyTimeUs(), std::memory_order_relaxed);
			logger->WARNING(0, TM("%s processingLoop exception: %s"), Id().c_str(), e.what());
		}
		catch (...)
		{
			// ---  record unknown error ---
			_runtimeStats.errorTotal.fetch_add(1, std::memory_order_relaxed);
			_runtimeStats.lastErrorTs.store(SteadyTimeUs(), std::memory_order_relaxed);
			logger->WARNING(0, TM("%s processingLoop unknown exception"), Id().c_str());
		}

		// --- record process end + duration ---
		uint64_t endTs = SteadyTimeUs();
		_runtimeStats.lastProcessEndTs.store(endTs, std::memory_order_relaxed);
		_runtimeStats.isProcessingNow.store(false, std::memory_order_relaxed);

		uint64_t duration = endTs - startTs;
		_runtimeStats.totalProcessTimeUs.fetch_add(duration, std::memory_order_relaxed);

		// update max (relaxed CAS loop)
		uint64_t prevMax = _runtimeStats.maxProcessTimeUs.load(std::memory_order_relaxed);
		while (duration > prevMax &&
			!_runtimeStats.maxProcessTimeUs.compare_exchange_weak(
				prevMax, duration, std::memory_order_relaxed))
		{
		}

		logger->Telemetry(teleChannelIsProcessing, 0);

		std::this_thread::sleep_until(timeout);
	}
}




