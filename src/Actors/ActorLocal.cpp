
#include "ActorLocal.h"
#include "UidGenerator.hpp"
#include "PortFactory.h"
#include "Logger.h"

using rf::ActorLocal;
using rf::IPort;
using rf::Logger;

using nlohmann::json;

ActorLocal::ActorLocal(const std::string &id) : _id(id), _type("ActorLocal"), logger(new Logger())
{
  //logger->ConnectToShare(L"ActorSystem log client", L"Actor System Trace channel", L"Actor System Telemetry channel");
  logger->ConnectToShare( "ActorSystem log client", "Actor System Trace channel", "Actor System Telemetry channel");
}

ActorLocal::~ActorLocal()
{
}

bool ActorLocal::Init(const json &actorConfig)
{
  return true;
}

json ActorLocal::Configuration()
{
  auto portJson = json::array();

  for (const auto &[portIdInternal, portInternal] : _mapPorts)
  {
    portJson.emplace_back(portInternal->Configuration());
  }
  return json{
      {"id", _id},
      {"type", _type},
      {"ports", portJson},
  };
}

json ActorLocal::Links()
{
  json connections = json::array();
  for (const auto &[portIdInternal, portInternal] : _mapPorts)
  {
    auto mapExternals = portInternal->IdentifiersOfNotifiable();
    for (const auto &[actorIdExternal, portIdExternal] : mapExternals)
    {
      //connections.emplace_back(json::array({_id, portIdInternal, actorIdExternal, portIdExternal}));
        connections.emplace_back(json{ {"idActorSrc", _id}, {"idPortSrc", portIdInternal}, {"idActorDst",actorIdExternal}, {"idPortDst",portIdExternal} });

    }
  }
  return connections;
}

std::vector<std::weak_ptr<IPort>> ActorLocal::GetPorts()
{
  std::vector<std::weak_ptr<IPort>> ports;
  for (const auto &[portIdInternal, portInternal] : _mapPorts)
    ports.push_back(portInternal);
  return ports;
}

json ActorLocal::GetStatus()
{
  json res;
  res["id"] = _id;
  // res["Recived frames"] = recivedFramesByPeriod;
  // recivedFramesByPeriod = 0;
  // res["Processed frames"] = processedFramesByPeriod;
  // processedFramesByPeriod = 0;

  return res;
}

std::shared_ptr<IPort> ActorLocal::addPort(const std::string &typePort, const std::string &portId)
{
  std::shared_ptr<IPort> portPtr = rf::PortFactory::Create(typePort, portId, this);
  if (portPtr)
  {
    //portPtr->SetEveventOnReceive(std::bind(&ActorLocal::OnInputReceive, this, std::placeholders::_1, std::placeholders::_2));
    // std::function<void(std::string,std::shared_ptr<IMessage>)> functionOnRecive;
    portPtr->SetEveventOnReceive([&](std::string idPort, std::shared_ptr<IMessage> ptrData) {
      if (this->_flagActive)
        this->OnInputReceive(idPort, ptrData);
    });
    _mapPorts.emplace(std::make_pair(portPtr->Id(), portPtr));
  }
  return portPtr;
}

std::shared_ptr<IPort> ActorLocal::addPort(const std::string &typePort)
{
  std::string portId = rf::UidGenerator::Generate(typePort);
  return addPort(typePort, portId);
}

std::shared_ptr<IPort> ActorLocal::addPort(const json &portJson)
{
  std::shared_ptr<IPort> port = nullptr;
  try
  {
    std::string typePort = portJson.at("type").get<std::string>();
    std::string portId = portJson.at("id").get<std::string>();

    port = addPort(typePort, portId);
    if (port)
      port->Init(portJson);
  }
  catch (...)
  {
  }
  return port;
}

bool ActorLocal::ConnectTo(const std::string &actorIdExternal, std::weak_ptr<IPort> &portExternalWeakPtr, const std::string &portIdInternal)
{
  auto portInternalWeakPtr = GetPortById(portIdInternal);
  auto portInternal = portInternalWeakPtr.lock();
  if (!portInternal)
    return false;
  // можно не проверять тип порта, т.к. для входа Attach ничего не делает
  // а лучше вызвать для входа и выхода сразу
  auto portExternal = portExternalWeakPtr.lock();
  if(!portExternal)
    return false;
  portExternal->Attach(_id, portInternalWeakPtr);
  portInternal->Attach(actorIdExternal, portExternalWeakPtr);
  return true;
}

bool ActorLocal::ConnectTo(std::weak_ptr<IAbstractActor> &actorExternalWeakPtr, const std::string &portIdExternal, const std::string &portIdInternal)
{
  auto actorExternal = actorExternalWeakPtr.lock();
  if(!actorExternal)
   return false;
  auto portExternal = actorExternal->GetPortById(portIdExternal);
  if (!portExternal.lock())
    return false;

  return ConnectTo(actorExternal->Id(), portExternal, portIdInternal);
}

void ActorLocal::Disconnect(const std::string &actorIdExternal, std::weak_ptr<IPort> &portExternalWeakPtr, const std::string &portIdInternal)
{
  auto portInternalWeakPtr = GetPortById(portIdInternal);
  auto portInternal = portInternalWeakPtr.lock();
  if (!portInternal)
    return;
  // можно не проверять тип порта, т.к. для входа Attach ничего не делает
  portInternal->Detach(actorIdExternal, portExternalWeakPtr);
  auto portExternal = portExternalWeakPtr.lock();
  if(!portExternal)
     return;
  portExternal->Detach(_id, portInternalWeakPtr);
}

void ActorLocal::Disconnect(const std::string &actorIdExternal, const std::string &portIdExternal, const std::string &portIdInternal)
{
  auto portInternal = GetPortById(portIdInternal).lock();
  if (portInternal)
    portInternal->Detach(actorIdExternal, portIdExternal);
}

void ActorLocal::DisconnectAll(const std::string &actorIdExternal, const std::string &portIdExternal)
{
  for (const auto &[portIdInternal, portInternal] : _mapPorts)
  {
    portInternal->Detach(actorIdExternal, portIdExternal);
  }
}

std::weak_ptr<IPort> ActorLocal::GetPortById(const std::string &portId)
{
  auto it = _mapPorts.find(portId);
  if (it == _mapPorts.end())
    return std::weak_ptr<IPort>();

  return it->second;
}
