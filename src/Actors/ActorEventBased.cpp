#include "ActorEventBased.h"

#include <thread>

#include "Logger.h"
#include "RuntimeClock.hpp"
#include "RuntimeTypes.hpp"
#include "FlowTraceRecorder.h"
#include "FlowTraceTypes.hpp"

using rf::ActorEventBased;
using rf::ActorLocal;
using rf::IPort;
using rf::Logger;

using nlohmann::json;

ActorEventBased::ActorEventBased(const std::string& id, IUnit* parent)
	: ActorLocal(id, parent)
	, myFutureQueue(255, ModeQueueFull::Nothing)
	, isAsync(true)

{
	_type = "ActorEventBased";
	//std::wstring telemetryName{Logger::StrToWstr(id)+L"_activeTasks"};
	std::string telemetryName{ id + "_activeTasks" };
	logger->CreateTelemetryChannel(telemetryName.c_str(), 0, -1, 255, 255, true, &teleChannelActiveTasks);

	//telemetryName = std::wstring{Logger::StrToWstr(id)+L"_isProcess"};
	telemetryName = std::string{ id + "_isProcess" };
	logger->CreateTelemetryChannel(telemetryName.c_str(), 0, -1, 2, 1, true, &teleChannelIsProcessing);
}

ActorEventBased::~ActorEventBased()
{
	Deactivate();
	WaitForTasks();
}

json ActorEventBased::Configuration()
{
	auto config = ActorLocal::Configuration();
	auto& configProps = config["properties"];
	configProps["isAsync"] = isAsync;
	if (isAsync)
	{
		configProps["queueSize"] = myFutureQueue.getMaxSize();
	}
	return  config;
}


bool ActorEventBased::SetProperties(const json& properties)
{
	if (!ActorLocal::SetProperties(properties))
		return false;
	if (properties.contains("isAsync"))
		if (properties.at("isAsync").is_boolean())
			isAsync = properties.at("isAsync").get<bool>();
	if (properties.contains("queueSize"))
		if (properties.at("queueSize").is_number_integer())
			myFutureQueue.setMaxSize(properties.at("queueSize").get<int>());
	return true;
}


std::variant<std::monostate, bool, int, double, std::string> ActorEventBased::GetProperty(const std::string& propertyName)
{
	if (propertyName.compare("isAsync") == 0)
		return isAsync;
	else if (propertyName.compare("queueSize") == 0)
		return static_cast<int>(myFutureQueue.getMaxSize());
	return ActorLocal::GetProperty(propertyName);
}

bool ActorEventBased::SetProperty(const std::string& propertyName, bool value)
{
	if (propertyName.compare("isAsync"))
	{
		isAsync = value;
		return true;
	}
	return ActorLocal::SetProperty(propertyName, value);
}

bool ActorEventBased::SetProperty(const std::string& propertyName, int value)
{
	if (propertyName.compare("queueSize"))
	{
		myFutureQueue.setMaxSize(value);
		return true;
	}
	return ActorLocal::SetProperty(propertyName, value);
}



void ActorEventBased::OnInputReceive(const std::string& portId, std::shared_ptr<IMessage> dataPtr)
{
	if (!dataPtr)
		return;

	// --- record input ---
	_runtimeStats.receivedTotal.fetch_add(1, std::memory_order_relaxed);
	_runtimeStats.lastInputTs.store(SteadyTimeUs(), std::memory_order_relaxed);
	_runtimeStats.Touch();

	logger->TRACE(0, TM("%s received message ID:%lld on input-> %s"), Id().c_str(), dataPtr->Id(), portId.c_str());
	if (!ApproveTask(portId, dataPtr))
		return;

	if (isAsync)
	{
		std::lock_guard<std::mutex> lock(onInputTask);
		SanitizeQueue();
		if (myFutureQueue.isFull() && myFutureQueue.getModeFull() == ModeQueueFull::Nothing)
		{
			// ѕропускаем обработку ()
			_runtimeStats.droppedTotal.fetch_add(1, std::memory_order_relaxed);
			_runtimeStats.Touch();
			logger->DEBUG(0, TM("%s skip message ID:%lld on input-> %s"), Id().c_str(), dataPtr->Id(), portId.c_str());
		}
		else
		{
			auto testfuture = std::async(std::launch::async, [portId, dataPtr, this]()
				{
					auto dataSharedPtr = dataPtr;       //специально по значению, поскольку объект уже может быть удален к момнету начала обработки
					this->ProcessWrap(portId, dataSharedPtr);
				});
			//auto testfuture = std::async(std::launch::async, &ActorEventBased::ProcessWrap, this, portId, std::ref(dataPtr));
			myFutureQueue.emplace_back(std::move(testfuture));
		}

		logger->Telemetry(teleChannelActiveTasks, myFutureQueue.size());
	}
	else
	{
		std::lock_guard<std::mutex> lock(onInputTask);
		ProcessWrap(portId, dataPtr);
	}
}



void ActorEventBased::ProcessWrap(const std::string& portId, std::shared_ptr<rf::IMessage> dataPtr)
{
	_runtimeStats.isProcessingNow.store(true, std::memory_order_relaxed);
	uint64_t startTs = SteadyTimeUs();
	_runtimeStats.lastProcessStartTs.store(startTs, std::memory_order_relaxed);
	// ---  flow trace ProcessStart ---
	FlowTraceRecorder* recorder = GetFlowTraceRecorder();
	if (recorder && recorder->IsEnabled())
	{
		recorder->Record({
			startTs,
			FlowTraceEventType::ProcessStart,
			FlowTraceHash(_id),
			FlowTraceHash(portId),
			dataPtr->Id(),
			dataPtr->Type(),
			0, 0, 0
			});
	}
	logger->Telemetry(teleChannelIsProcessing, 1);
	bool hasError = false;
	try
	{
		Process(portId, dataPtr);

		// --- record success ---
		_runtimeStats.processedTotal.fetch_add(1, std::memory_order_relaxed);
	}
	catch (const std::exception& e)
	{
		// --- record error ---
		hasError = true;
		_runtimeStats.errorTotal.fetch_add(1, std::memory_order_relaxed);
		_runtimeStats.lastErrorTs.store(SteadyTimeUs(), std::memory_order_relaxed);
		logger->WARNING(0, TM("%s Process exception: %s"), Id().c_str(), e.what());
	}
	catch (...)
	{
		// --- record error (unknown) ---
		hasError = true;
		_runtimeStats.errorTotal.fetch_add(1, std::memory_order_relaxed);
		_runtimeStats.lastErrorTs.store(SteadyTimeUs(), std::memory_order_relaxed);
		logger->WARNING(0, TM("%s Process unknown exception"), Id().c_str());
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
		// prevMax is updated by compare_exchange_weak on failure
	}
	_runtimeStats.Touch();

	// flow trace ProcessEnd or ProcessError ---
	if (recorder && recorder->IsEnabled())
	{
		recorder->Record({
			endTs,
			hasError ? FlowTraceEventType::ProcessError : FlowTraceEventType::ProcessEnd,
			FlowTraceHash(_id),
			FlowTraceHash(portId),
			dataPtr->Id(),
			dataPtr->Type(),
			0, 0,
			static_cast<uint32_t>(duration)
			});
	}

	logger->Telemetry(teleChannelIsProcessing, 0);
}


void ActorEventBased::WaitForTasks()
{
	Deactivate();
	std::lock_guard<std::mutex> lock(onInputTask);
	while (!myFutureQueue.empty())
	{
		SanitizeQueue();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void ActorEventBased::SanitizeQueue()
{
	bool ready = true;
	while (ready)
	{
		auto readyFuture = myFutureQueue.pop_front_if([](auto& frontFuture) {
			return frontFuture.wait_for(std::chrono::nanoseconds(40)) == std::future_status::ready;
			});

		if (readyFuture.has_value())
		{
			try
			{
				readyFuture->get();
			}
			catch (const std::exception& e)
			{
				logger->WARNING(0, TM("%s Async task exception :%s"), Id().c_str(), e.what());
			}
			catch (...)
			{
				logger->WARNING(0, TM("%s Async task unknown exception"), Id().c_str());
			}
			ready = true;
			logger->Telemetry(teleChannelActiveTasks, myFutureQueue.size());
		}
		else
		{
			ready = false;
		}
	}
}

json ActorEventBased::GetStatus()
{
	// start from base
	json res = ActorLocal::GetStatus();

	// --- runtime state ---
	bool processing = _runtimeStats.isProcessingNow.load(std::memory_order_relaxed);
	int  queueSize = myFutureQueue.size();
	int  queueCap = static_cast<int>(myFutureQueue.getMaxSize());

	RuntimeState state = RuntimeState::Idle;
	if (!_flagActive)
		state = RuntimeState::Inactive;
	else if (processing || queueSize > 0)
		state = RuntimeState::Processing;

	if (_flagActive && queueCap > 0 && queueSize >= queueCap)
		state = RuntimeState::Backpressured;

	res["runtimeState"] = ToString(state);

	// --- async-specific fields ---
	res["isAsync"] = isAsync;
	res["isProcessingNow"] = processing;
	res["activeTasks"] = queueSize;
	res["taskQueueSize"] = queueSize;
	res["taskQueueCapacity"] = queueCap;

	return res;
}

