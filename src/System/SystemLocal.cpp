#include "SystemLocal.h"

#include <iostream>

#include "IAbstractActor.h"
#include "ActorFactoryCollection.hpp"
#include "UidGenerator.hpp"
#include "Logger.h"
#include "PortOutput.h" //access for specific functions, like setLinkUserData

using nlohmann::json;
using rf::ActorCreatorFunction;
using rf::IUnit;
using rf::IAbstractActor;
using rf::SystemLocal;
using rf::Logger;


SystemLocal::SystemLocal(const std::string& loggerInitParam) :
    logger(new Logger())
    //, userData{ json({}) }
{
  if(logger)
  {
     logger->Create(loggerInitParam, "Actor Scheme Trace channel", "Actor Scheme Telemetry channel");
     logger->Share("ActorSystem log client", "Actor System Trace channel", "Actor System Telemetry channel");
  }
}


SystemLocal::~SystemLocal()
{
    logger->client->Flush();
    Clear();
}


bool SystemLocal::Init(const nlohmann::json &scheme)
{
  Clear();
  // Init Actors
  const auto  actorsJson = scheme.find("actors");
  bool res{ true };
  if (actorsJson == scheme.end())
   {
      logger->TRACE(0, TM("Actors Not Found"));
      return true;
   }
  for (const auto &actorJson : *actorsJson)
  {
    auto actor = Spawn(actorJson);
    if (!actor.lock())
    {
      logger->WARNING(0, TM("Actor wasn`t spawned:%s"),actorJson.dump().c_str());
      //Clear();
      res = false;
    }
    else
    {
        actor.lock()->SetParent(this);
    }
  }
  // Init Links
  auto const linksJson = scheme.find("links");
  if (linksJson == scheme.end())
   {
      logger->TRACE(0, TM("Links Not Found"));
  }
  else
  {
      for (const auto& linkJson : *linksJson)
      {
          if (!Connect(linkJson))
          {
              logger->WARNING(0, TM("Connection problem %s"), linkJson.dump().c_str());
              res = false;
          }
      }
  }
  if (scheme.contains("userData"))
  {
      userData = scheme.at("userData");
  }
  //if(logger->trace)
  if (res)
  {
      this->Activate();
      logger->INFO(0, TM("Actor System was Init successfully %d"), 0);
  }
  return res;
}


json SystemLocal::Scheme()
{
  json jsonScheme{};
  auto jsonActors = json::array();
  auto jsonLinks = json::array();
  for(const auto& [actorId, actor]:_mapActors)
  {
      jsonActors.emplace_back(actor->Configuration());
      auto links = actor->Links();
      //jsonConnections+=connections;
      for(const auto &link : links)
      {
         jsonLinks.emplace_back(link);
      }
      
  }
  jsonScheme["actors"] = jsonActors;
  jsonScheme["links"] = jsonLinks;
  jsonScheme["userData"] = userData;
  return jsonScheme;
}


void SystemLocal::Clear()
{
  Deactivate();
  //_mapActors.clear();
  for (auto it = _mapActors.begin(); it != _mapActors.end();)
  {
    auto actor = it->second;
    this->RemoveAllConectionsWithActor(actor);
    it = _mapActors.erase(it);
  }
  userData = json::object();
}


//by json
std::weak_ptr<IAbstractActor> SystemLocal::Spawn(json jsonActor)
{
  std::shared_ptr<IAbstractActor> actorPtr = nullptr;

  try
  {
    std::string typeName = jsonActor["type"].get<std::string>();
    
    std::string id = UidGenerator::Generate(typeName);
    if (jsonActor.contains("id"))
        if (jsonActor.at("id").is_string())
             id = jsonActor["id"].get<std::string>();

    actorPtr =  std::shared_ptr<IAbstractActor>(ActorFactoryCollection::Create(typeName, id));
    if(!actorPtr)
      return actorPtr;
    Attach(actorPtr);
    actorPtr->Init(jsonActor);
  }
  catch (...)
  {
      logger->TRACE(0, TM("Spawn error %s"), jsonActor);
  }
  return actorPtr;
}


//by name
std::weak_ptr<IAbstractActor> SystemLocal::Spawn(std::string typeName)
{
  std::string id = UidGenerator::Generate(typeName);
  std::shared_ptr<IAbstractActor> actorPtr(ActorFactoryCollection::Create(typeName, id));
  if(actorPtr)
    Attach(actorPtr);
  return actorPtr;
}


bool SystemLocal::Attach(std::shared_ptr<IAbstractActor> actorPtr)
{
  if (_mapActors.count(actorPtr->Id()) != 0)
    return false;
  _mapActors.emplace(std::make_pair(actorPtr->Id(), actorPtr));
  actorPtr->SetParent(this);
  return true;
}


std::shared_ptr<IAbstractActor> SystemLocal::Detach(std::string id)
{
  std::shared_ptr<IAbstractActor> actor{nullptr};
  auto it = _mapActors.find(id);
  if (it != _mapActors.end())
  {
    actor = it->second;
    this->RemoveAllConectionsWithActor(actor);
    actor->SetParent(nullptr);
    it = _mapActors.erase(it);
  }
  return actor;
}


//spawn copy of existing
std::weak_ptr<IAbstractActor> SystemLocal::GetActorById(std::string id)
{
  auto it = _mapActors.find(id);
  if (it != _mapActors.end())
  {
    return it->second;
  }
  return std::weak_ptr<IAbstractActor>();
}


void SystemLocal::RegisterFactory(std::set<std::string> keySet, ActorCreatorFunction functor)
{
  //FactoryAllActors::Register("key", funcor);
  std::for_each(keySet.begin(), keySet.end(),
                [&](auto const &key) { ActorFactoryCollection::Register(key, functor); });
}


std::set<std::string> SystemLocal::GetRegisteredActorTypes()
{
  return ActorFactoryCollection::getRegisteredTypes();
}


bool SystemLocal::Connect(std::string idActor1, std::string idPortActor1, std::string idActor2, std::string idPortActor2)
{
  auto actor1 = GetActorById(idActor1);
  if (!actor1.lock())
    return false;

  auto actor2 = GetActorById(idActor2).lock();
  if (!actor2)
    return false;

  return actor2->ConnectTo(actor1, idPortActor1, idPortActor2);
  //actor2->
}


bool SystemLocal::Connect(json connection)
{
  bool res{ false };
  try
  {
    std::string idActor1 = connection["idActorSrc"].get<std::string>();
    std::string idPortActor1 = connection["idPortSrc"].get<std::string>();
    std::string idActor2 = connection["idActorDst"].get<std::string>();
    std::string idPortActor2 = connection["idPortDst"].get<std::string>();
    res =  Connect(idActor1, idPortActor1, idActor2, idPortActor2);
    if (res && connection.contains("userData"))
    {
        auto&  userData = connection.at("userData");
        SetLinkUserData(idActor1, idPortActor1, idActor2, idPortActor2, userData);
    }
  }
  catch (...)
  {
  }
  return res;
}



//TODO 
void SystemLocal::Disconnect(std::string idActor1, std::string idPortActor1, std::string idActor2, std::string idPortActor2)
{
  auto actor1 = GetActorById(idActor1).lock();
  auto actor2 = GetActorById(idActor2).lock();
  if (actor1 && actor2)
  {
    auto port2 = actor2->GetPortById(idPortActor2);
    if (port2.lock())
      actor1->Disconnect(idActor2, port2, idPortActor1);

    //TODO проверить почему port1 берется с actor2
    auto port1 = actor2->GetPortById(idPortActor2);
    if (port1.lock())
      actor2->Disconnect(idActor1, port1, idPortActor2);
  }
}


void SystemLocal::Disconnect(json connection)
{
  try
  {
    std::string idActor1 = connection["idActorSrc"].get<std::string>();
    std::string idPortActor1 = connection["idPortSrc"].get<std::string>();
    std::string idActor2 = connection["idActorDst"].get<std::string>();
    std::string idPortActor2 = connection["idPortDst"].get<std::string>();
    Disconnect(idActor1, idPortActor1, idActor2, idPortActor2);
  }
  catch (...)
  {
  }
}


void SystemLocal::RemoveAllConectionsWithActor(std::weak_ptr<IAbstractActor> ptrWeakActorTarget)
{
  auto actorTarget = ptrWeakActorTarget.lock();
  if(!actorTarget)
     return;
  auto actorPorts = actorTarget->GetPorts();
  for (auto actorPort : actorPorts)
  {
    actorPort.lock()->CleanObservers();
    for (auto [actorId, actor] : _mapActors)
    {
      actor->DisconnectAll(actorTarget->Id(),actorPort.lock()->Id());
    }
  }
}


void SystemLocal::SetLinkUserData(std::string idActorSrc, std::string idPortSrc, std::string idActorDst, std::string idPortDst, json userData)
{
    auto actor = GetActorById(idActorSrc).lock();
    if (actor)
    {
        auto port = actor->GetPortById(idPortSrc).lock();
        if (port)
        {
            if (PortOutput* portOut = dynamic_cast<PortOutput*>(port.get())) {
                portOut->SetLinkUserData(idActorDst, idPortDst, userData);
            }
        }
    }
    return;
}

json SystemLocal::GetLinkUserData(std::string idActorSrc, std::string idPortSrc, std::string idActorDst, std::string idPortDst )
{
    json userData;
    auto actor = GetActorById(idActorSrc).lock();
    if (actor)
    {
        auto port = actor->GetPortById(idPortSrc).lock();
        if (port)
        {
            if (PortOutput* portOut = dynamic_cast<PortOutput*>(port.get())) {
                userData = portOut->GetLinkUserData(idActorDst, idPortDst);
            }
        }
    }
    return userData;
}

void SystemLocal::Activate()
{
  std::for_each(_mapActors.cbegin(), _mapActors.cend(),[](auto & recInMap){ recInMap.second->Activate(); });
}


void SystemLocal::Deactivate()
{
    std::for_each(_mapActors.cbegin(), _mapActors.cend(),[](auto & recInMap){ recInMap.second->Deactivate(); });
}

std::map<std::string, bool> SystemLocal::ActorsActivationState()
{
    std::map<std::string, bool> activation;
    for (auto [actorId, actor] : _mapActors)
    {
        activation[actorId] = actor->IsActive();
    }
    return activation;
}


std::vector<std::weak_ptr<IUnit>> SystemLocal::Children() 
{
  std::vector<std::weak_ptr<IUnit>> children;
  std::for_each(_mapActors.cbegin(), _mapActors.cend(),[&children](auto & recInMap){children.emplace_back(recInMap.second); });
  return children;
}



 
