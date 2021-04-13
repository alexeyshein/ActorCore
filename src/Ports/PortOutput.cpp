#include "PortOutput.h"
#include <unordered_map> //std::hash<std::string>

#include "Logger.h"

using rf::PortOutput;
using rf::Logger;

using nlohmann::json;

PortOutput::PortOutput(std::string id, IUnit* parent)
    : rf::PortBase(id, parent)
{
  _type = "PortOutput";
  publisher.SetAsyncMode(true);
  publisher.SetAsyncQueueSize(0);

  std::wstring telemetryName{Logger::StrToWstr(id)+L"_isNotifying"};
  logger->CreateTelemetryChannel(telemetryName.c_str(), 0,-1,2,1,true, &teleChannelIsNotifying);
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

std::variant<std::monostate, bool, int, double, std::string> PortOutput::GetProperty(const std::string& propertyName)
{
  if(propertyName.compare("isAsync") == 0)
    return publisher.IsAsyncMode();
  else if(propertyName.compare("queueNotifiersSize") == 0)
    return static_cast<int>(publisher.AsyncQueueSize());
  return PortBase::GetProperty(propertyName);
}


bool PortOutput::SetProperty(const std::string& propertyName, bool value) 
{
  if(propertyName.compare("isAsync"))
    {
      publisher.SetAsyncMode(value);
      return true;
    }
  return PortBase::SetProperty(propertyName, value);
}

bool  PortOutput::SetProperty(const std::string& propertyName, int value)
{
  if(propertyName.compare("queueNotifiersSize"))
    {
       publisher.SetAsyncQueueSize(value);
      return true;
    }
   return PortBase::SetProperty(propertyName, value);
}

void PortOutput::Attach(const  std::string& remotePortOwnerId, std::weak_ptr<IPort>& ptrWeakRemotePort)
{
  auto ptrRemotePort = ptrWeakRemotePort.lock();
  if(!ptrRemotePort)
    return;
  //TODO add check by typesMessages
  std::size_t linkId = std::hash<std::string>{}(remotePortOwnerId+ptrRemotePort->Id());
  std::function<void(const std::shared_ptr<IMessage> &)> f = std::bind(&rf::IPort::Receive, ptrRemotePort.get(), std::placeholders::_1);
  publisher.Attach(linkId, f);
  setIdentifiersOfNotifiable.emplace(std::pair<std::string,std::string>(remotePortOwnerId, ptrRemotePort->Id()));
}

void PortOutput::Detach(const  std::string& remotePortOwnerId, std::weak_ptr<IPort>& ptrWeakRemotePort)
{
  auto ptrRemotePort = ptrWeakRemotePort.lock();
  if(!ptrRemotePort)
    return;
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

void PortOutput::Notify(const std::shared_ptr<IMessage> &data)
{
  logger->Telemetry(teleChannelIsNotifying, 1);
  //TODO add check for compliance with data and typesMessages
  publisher.Notify(data);
  logger->Telemetry(teleChannelIsNotifying, 0);
}

