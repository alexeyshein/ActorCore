#include "ActorEventBased.h"
#include "Logger.h"

using rf::ActorEventBased;
using rf::ActorLocal;
using rf::IPort;
using rf::Logger;

using nlohmann::json;

ActorEventBased::ActorEventBased(const std::string& id) 
: ActorLocal(id)
,  myFutureQueue(255, ModeQueueFull::Nothing)
,  isAsync(true)

{
    _type = "ActorEventBased" ;
   if(logger->telemetry)
   {
    std::wstring telemetryName{Logger::StrToWstr(id)+L"_activeTasks"};
    logger->telemetry->Create(telemetryName.c_str(), 0,-1,255,255,true, &teleChannelFutureQueueSizeId);
  }
}


bool ActorEventBased::Init(const json& config)
{
  if(!ActorLocal::Init(config))
   return false;
  if(config.contains("isAsync"))
   if(config.at("isAsync").is_boolean())
    isAsync = config.at("isAsync").get<bool>();
  if(config.contains("queueSize"))
   if(config.at("queueSize").is_number_integer())
    myFutureQueue.setMaxSize(config.at("queueSize").get<int>());
  
  return true;
}

json ActorEventBased::Configuration()
{
  auto config = ActorLocal::Configuration();
  config["isAsync"] = isAsync;
  if(isAsync)
  {
    config["queueSize"] = myFutureQueue.size();
  }
  return  config;
}

std::variant<std::monostate, bool, int, double, std::string> ActorEventBased::GetProperty(const std::string &propertyName) 
{ 
  if(propertyName.compare("isAsync") == 0)
    return isAsync;
  else if(propertyName.compare("queueSize") == 0)
    return static_cast<int>(myFutureQueue.getMaxSize());
  return ActorLocal::GetProperty(propertyName);
}

bool ActorEventBased::SetProperty(const std::string& propertyName, bool value) 
{
    if(propertyName.compare("isAsync"))
    {
      isAsync = value;
      return true;
    }
  return ActorLocal::SetProperty(propertyName, value);
}

bool ActorEventBased::SetProperty(const std::string& propertyName, int value) 
{
  if(propertyName.compare("queueSize"))
    {
      myFutureQueue.setMaxSize(value);
      return true;
    }
  return ActorLocal::SetProperty(propertyName, value);
}



void ActorEventBased::OnInputReceive(const std::string& portId, std::shared_ptr<IMessage>& dataPtr)
{
  if(!ApproveTask(portId, dataPtr))
    return;
	SanitizeQueue();
  if(isAsync)
  {
    	myFutureQueue.emplace_back(std::async(std::launch::async, &ActorEventBased::Process, this, portId, dataPtr));
      if(logger->telemetry)
        logger->telemetry->Add(teleChannelFutureQueueSizeId, myFutureQueue.size());
  }else
  {
       Process(portId, dataPtr);
  }
} 

void ActorEventBased::SanitizeQueue()
{
	bool ready = true;
	while (ready)
	{
		ready = false;
		if (!myFutureQueue.empty())
		{
			std::future<void> &front = myFutureQueue.front();
			std::future_status status = front.wait_for(std::chrono::nanoseconds(40));
			if (status == std::future_status::ready)
			 {
			 	myFutureQueue.pop_front();
			 	ready = true;
        if(logger->telemetry)
          logger->telemetry->Add(teleChannelFutureQueueSizeId, myFutureQueue.size());
			 }
		}
	}
}


