#include "ActorEventBased.h"

using rf::ActorEventBased;
using rf::ActorLocal;
using rf::IPort;
using nlohmann::json;

ActorEventBased::ActorEventBased(const std::string& id) 
: ActorLocal(id)
,  myFutureQueue(255, ModeQueueFull::Nothing)
,  isAsync(true)

{
    _type = "ActorEventBased" ;
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

std::variant<bool, int, double> ActorEventBased::GetProperty(const std::string&)
{
  return std::variant<bool, int, double>();
}


void ActorEventBased::OnInputReceive(const std::string& portId, std::shared_ptr<IData>& dataPtr)
{
  if(!ApproveTask(portId, dataPtr))
    return;
	SanitizeQueue();
  if(isAsync)
  {
    	myFutureQueue.emplace_back(std::async(std::launch::async, &ActorEventBased::Process, this, portId, dataPtr));
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
			 }
		}
	}
}


