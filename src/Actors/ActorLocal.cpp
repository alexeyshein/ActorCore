
#include "ActorLocal.h"
#include "UidGenerator.hpp"
#include "PortFactory.h"

using rf::ActorLocal;
using rf::IPort;

using nlohmann::json;

ActorLocal::ActorLocal(std::string id) : _id(id), _typeId("ActorLocal")
{
}

bool ActorLocal::Init(json)
{
  return false;
}

std::vector<std::shared_ptr<IPort>> ActorLocal::GetPorts()
{
  std::vector<std::shared_ptr<IPort>> ports;
  for(const auto& [portIdInternal,portInternal] : _mapPorts)
      ports.push_back(portInternal);
  return ports;
}

std::variant<bool, int, double> ActorLocal::GetProperty(std::string)
{
  return std::variant<bool, int, double>();
}

json ActorLocal::GetStatus()
{
  json res;
  res["ID"] = _id;
  // res["Recived frames"] = recivedFramesByPeriod;
  // recivedFramesByPeriod = 0;
  // res["Processed frames"] = processedFramesByPeriod;
  // processedFramesByPeriod = 0;

  return res;
}

std::shared_ptr<IPort> ActorLocal::addPort(std::string typePort)
{
  std::string id = rf::UidGenerator::Generate(typePort);
  std::shared_ptr<IPort> portPtr = rf::PortFactory::Create(typePort, id);
  if (portPtr)
  {
    //portPtr->SetEveventOnReceive(std::bind(&ActorLocal::onInputReceive, this, std::placeholders::_1, std::placeholders::_2));
    // std::function<void(std::string,std::shared_ptr<IData>)> functionOnRecive;
    portPtr->SetEveventOnReceive([&](std::string idPort, std::shared_ptr<IData> ptrData){
      if(this->_flagActive)
        this->onInputReceive(idPort, ptrData );
    });
    _mapPorts.emplace(std::make_pair(portPtr->id(), portPtr));
  }
  return portPtr;
}

std::shared_ptr<IPort> ActorLocal::addPort(json portJson)
{
  std::shared_ptr<IPort> port = nullptr;
  try
  {
    std::string typePort = portJson["type"].get<std::string>();
    port = addPort(typePort);
    if (port)
      port->Init(portJson);
  }
  catch (...)
  {
  }
  return port;
}

bool ActorLocal::ConnectTo(std::shared_ptr<IPort> portExternal, std::string portIdInternal)
{
  auto portInternal = GetPortById(portIdInternal);
  if (!portInternal)
    return false;
  // можно не проверять тип порта, т.к. для входа Attach ничего не делает
  // а лучше вызвать для входа и выхода сразу
  portExternal->Attach(portInternal);
  portInternal->Attach(portExternal);
  return true;
}

bool ActorLocal::ConnectTo(std::shared_ptr<IAbstractActor> actorExternal, std::string portIdExternal, std::string portIdInternal)
{
    auto portExternal = actorExternal->GetPortById(portIdExternal);
    if(!portExternal)
      return false;
    
    return ConnectTo(portExternal, portIdInternal);
}

void ActorLocal::Disconnect(std::shared_ptr<IPort> portExternal, std::string portIdInternal)
{
  auto portInternal = GetPortById(portIdInternal);
  if (!portInternal)
    return;
  // можно не проверять тип порта, т.к. для входа Attach ничего не делает
  portInternal->Detach(portExternal);
  portExternal->Detach(portInternal);
}

 void   ActorLocal::Disconnect(std::string portIdExternal, std::string portIdInternal)
 {
  auto portInternal = GetPortById(portIdInternal);
  if (portInternal)
      portInternal->Detach(portIdExternal);
 }

void ActorLocal::DisconnectAll(std::string portIdExternal)
{
  for(const auto& [portIdInternal,portInternal] : _mapPorts)
  {
      portInternal->Detach(portIdExternal);
  }
}


std::shared_ptr<IPort> ActorLocal::GetPortById(std::string portId)
{
  auto it = _mapPorts.find(portId);
  if (it == _mapPorts.end())
    return nullptr;

  return it->second;
}
