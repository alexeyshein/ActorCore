#include "PortOutput.h"
#include <unordered_map> //std::hash<std::string>

using rf::PortOutput;
using nlohmann::json;

PortOutput::PortOutput(std::string id)
    : rf::PortBase(id)
{
  _type = "PortOutput";
  publisher.SetAsyncMode(true);
  publisher.SetAsyncQueueSize(0);
}

bool PortOutput::Init(const json& config)
{
  if(!PortBase::Init(config))
   return false;
  if(config.contains("isAsync"))
   if(config.at("isAsync").is_boolean())
     publisher.SetAsyncMode(config.at("isAsync").get<bool>());
  if(config.contains("queueNotifiersSize"))
   if(config.at("queueNotifiersSize").is_number_integer())
    publisher.SetAsyncQueueSize(config.at("queueNotifiersSize").get<int>());
}

json PortOutput::Configuration()
{
  auto config = PortBase::Configuration();
  config["isAsync"] = publisher.IsAsyncMode();
  if(publisher.IsAsyncMode())
  {
    config["queueNotifiersSize"] =  publisher.AsyncQueueSize();
  }
  return  config;
}


void PortOutput::Attach(const  std::string& remotePortOwnerId, std::shared_ptr<IPort>& ptrRemotePort)
{
  std::size_t linkId = std::hash<std::string>{}(remotePortOwnerId+ptrRemotePort->Id());
  std::function<void(const std::shared_ptr<IData> &)> f = std::bind(&rf::IPort::Receive, ptrRemotePort.get(), std::placeholders::_1);
  publisher.Attach(linkId, f);
  setIdentifiersOfNotifiable.emplace(std::pair<std::string,std::string>(remotePortOwnerId, ptrRemotePort->Id()));
}

void PortOutput::Detach(const  std::string& remotePortOwnerId, std::shared_ptr<IPort>& ptrRemotePort)
{
  this->Detach( remotePortOwnerId, ptrRemotePort->Id());
}

void PortOutput::Detach(const  std::string& remotePortOwnerId, const std::string& remotePortId)
{
  std::size_t linkId = std::hash<std::string>{}(remotePortOwnerId+remotePortId);
  publisher.Detach(linkId);
  setIdentifiersOfNotifiable.erase(std::pair<std::string,std::string>(remotePortOwnerId, remotePortId));
  // auto it = setIdentifiersOfNotifiable.find(std::pair<std::string,std::string>(remotePortOwnerId, remotePortId));
  // if(it!=setIdentifiersOfNotifiable.end())
  //   it = setIdentifiersOfNotifiable.erase(it);
}

void PortOutput::Notify(const std::shared_ptr<IData> &data)
{
  publisher.Notify(data);
}

