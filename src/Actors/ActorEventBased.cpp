#include "ActorEventBased.h"

#include "Logger.h"

using rf::ActorEventBased;
using rf::ActorLocal;
using rf::IPort;
using rf::Logger;

using nlohmann::json;

ActorEventBased::ActorEventBased(const std::string& id)
	: ActorLocal(id)
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


bool ActorEventBased::Init(const json& config)
{
	if (!ActorLocal::Init(config))
		return false;
	if (config.contains("properties"))
	{
		//const auto& properties = config.at("properties");
		this->SetProperties(config.at("properties"));
	}

	return true;
}


json ActorEventBased::Configuration()
{
	auto config = ActorLocal::Configuration();
	auto& configProps = config["properies"];
	configProps["isAsync"] = isAsync;
	if (isAsync)
	{
		configProps["queueSize"] = myFutureQueue.size();
	}
	return  config;
}


bool ActorEventBased::SetProperties(const json& properties)
{
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



void ActorEventBased::OnInputReceive(const std::string& portId, std::shared_ptr<IMessage>& dataPtr)
{
	logger->TRACE(0, TM("%s received message ID:%i on input-> %s"), Id().c_str(), dataPtr->Id(), portId.c_str());
	if (!ApproveTask(portId, dataPtr))
		return;
	SanitizeQueue();
	if (isAsync)
	{
		auto testfuture = std::async(std::launch::async, [portId, dataPtr, this]()
			{
				auto dataSharedPtr = dataPtr;       //специально по значению, поскольку объект уже может быть удален к момнету начала обработки
				this->ProcessWrap(portId, dataSharedPtr);
			});
		//auto testfuture = std::async(std::launch::async, &ActorEventBased::ProcessWrap, this, portId, std::ref(dataPtr));
		myFutureQueue.emplace_back(std::move(testfuture));
		logger->Telemetry(teleChannelActiveTasks, myFutureQueue.size());
	}
	else
	{
		ProcessWrap(portId, dataPtr);
	}
}



void ActorEventBased::ProcessWrap(const std::string& portId, std::shared_ptr<rf::IMessage>& dataPtr)
{
	logger->Telemetry(teleChannelIsProcessing, 1);
	Process(portId, dataPtr);
	logger->Telemetry(teleChannelIsProcessing, 0);
}


void ActorEventBased::SanitizeQueue()
{
	bool ready = true;
	while (ready)
	{
		ready = false;
		if (!myFutureQueue.empty())
		{
			std::future<void>& front = myFutureQueue.front();
			std::future_status status = front.wait_for(std::chrono::nanoseconds(40));
			if (status == std::future_status::ready)
			{
				myFutureQueue.pop_front();
				ready = true;
				logger->Telemetry(teleChannelActiveTasks, myFutureQueue.size());
			}
		}
	}
}


