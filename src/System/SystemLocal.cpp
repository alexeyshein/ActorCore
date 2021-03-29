#include "SystemLocal.h"

#include <iostream>

#include <P7_Trace.h>
#include <P7_Telemetry.h>

#include "IAbstractActor.h"
#include "ActorFactoryCollection.hpp"
#include "UidGenerator.hpp"

using nlohmann::json;
using rf::ActorCreatorFunction;
using rf::IAbstractActor;
using rf::SystemLocal;

SystemLocal::SystemLocal():
logClient(nullptr)
,logTrace(nullptr)
, logTelemetry(nullptr)
{
  InitLogger();
}

SystemLocal::~SystemLocal()
{
  logTelemetry.release();
  
  if(logTrace)
  {
  logTrace->Unregister_Thread(0);
  logTrace.release();
  }


  logClient.release();
}

bool SystemLocal::Init(nlohmann::json scheme)
{
  Clear();
  // Init Actors
  const auto  actorsJson = scheme.find("actors");
  if (actorsJson == scheme.end())
   {
      std::cerr << "Actors Not Found";
      return false;
   }
  for (const auto &actorJson : *actorsJson)
  {
    auto actor = Spawn(actorJson);
    if (!actor)
    {
      std::cerr << "Actor wasn`t spawned:"<<actorJson;
      Clear();
      return false;
    }  
  }
  // Init Connections
  auto const connectionsJson = scheme.find("connections");
  if (connectionsJson == scheme.end())
   {
      //std::cerr << "Connections Not Found";
      logTrace->P7_WARNING(0, TM("Connections Not Found"));
      return false;
   }
  for (const auto &connectionJson : *connectionsJson)
  {
      if(!Connect(connectionJson))
      {
        //std::cerr << "Connection problem:"<<connectionJson;
        logTrace->P7_WARNING(0, TM("Connection problem %s"), connectionJson);
        return false;
      }
  }
  logTrace->P7_INFO(0, TM("Actor System was Init successfully #%d"), 0);
  return true;
}


json SystemLocal::Scheme()
{
  json jsonScheme{};
  auto jsonActors = json::array();
  auto jsonConnections = json::array();
  for(const auto& [actorId, actor]:_mapActors)
  {
      jsonActors.emplace_back(actor->Configuration());
      auto connections = actor->Connections();
      //jsonConnections+=connections;
      for(const auto &connection : connections)
      {
         jsonConnections.emplace_back(connection);
      }
      
  }
  jsonScheme["actors"] = jsonActors;
  jsonScheme["connections"] = jsonConnections;
  return jsonScheme;
}

void SystemLocal::Clear()
{
  //_mapActors.clear();
  for (auto it = _mapActors.begin(); it != _mapActors.end();)
  {
    auto actor = it->second;
    this->RemoveAllConectionsWithActor(actor);
    it = _mapActors.erase(it);
  }
}

//by json
std::shared_ptr<IAbstractActor> SystemLocal::Spawn(json jsonActor)
{
  std::shared_ptr<IAbstractActor> actorPtr = nullptr;
  try
  {
    std::string typeName = jsonActor["type"].get<std::string>();
    std::string id = jsonActor["id"].get<std::string>();
    actorPtr =  std::shared_ptr<IAbstractActor>(ActorFactoryCollection::Create(typeName, id));
    if(!actorPtr)
      return actorPtr;
    Attach(actorPtr);
    actorPtr->Init(jsonActor);
  }
  catch (...)
  {
  }
  return actorPtr;
}

//by name
std::shared_ptr<IAbstractActor> SystemLocal::Spawn(std::string typeName)
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
    it = _mapActors.erase(it);
  }
  return actor;
}

//spawn copy of existing
std::shared_ptr<IAbstractActor> SystemLocal::GetActorById(std::string id)
{
  auto it = _mapActors.find(id);
  if (it != _mapActors.end())
  {
    return it->second;
  }
  return nullptr;
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
  if (!actor1)
    return false;

  auto actor2 = GetActorById(idActor2);
  if (!actor2)
    return false;

  return actor2->ConnectTo(actor1, idPortActor1, idPortActor2);
  //actor2->
}

bool SystemLocal::Connect(json connection)
{
  try
  {
    std::string idActor1 = connection.at(0).get<std::string>();
    std::string idPortActor1 = connection.at(1).get<std::string>();
    std::string idActor2 = connection.at(2).get<std::string>();
    std::string idPortActor2 = connection.at(3).get<std::string>();
    return Connect(idActor1, idPortActor1, idActor2, idPortActor2);
  }
  catch (...)
  {
  }
  return false;
}


void SystemLocal::Disconnect(std::string idActor1, std::string idPortActor1, std::string idActor2, std::string idPortActor2)
{
  auto actor1 = GetActorById(idActor1);
  auto actor2 = GetActorById(idActor2);
  if (actor1 && actor2)
  {
    auto port2 = actor2->GetPortById(idPortActor2);
    if (port2)
      actor1->Disconnect(idActor2, port2, idPortActor1);

    auto port1 = actor2->GetPortById(idPortActor2);
    if (port1)
      actor2->Disconnect(idActor1, port1, idPortActor2);
  }
}

void SystemLocal::Disconnect(json connection)
{
  try
  {
    std::string idActor1 = connection.at(0).get<std::string>();
    std::string idPortActor1 = connection.at(1).get<std::string>();
    std::string idActor2 = connection.at(2).get<std::string>();
    std::string idPortActor2 = connection.at(3).get<std::string>();
    Disconnect(idActor1, idPortActor1, idActor2, idPortActor2);
  }
  catch (...)
  {
  }
}


void SystemLocal::RemoveAllConectionsWithActor(std::shared_ptr<IAbstractActor> actorTarget)
{
  auto actorPorts = actorTarget->GetPorts();
  for (auto actorPort : actorPorts)
  {
    actorPort->CleanObservers();
    for (auto [actorId, actor] : _mapActors)
    {
      actor->DisconnectAll(actorTarget->Id(),actorPort->Id());
    }
  }
}


void SystemLocal::Activate()
{
  std::for_each(_mapActors.cbegin(), _mapActors.cend(),[](auto & recInMap){ recInMap.second->Activate(); });
}

void SystemLocal::Deactivate()
{
    std::for_each(_mapActors.cbegin(), _mapActors.cend(),[](auto & recInMap){ recInMap.second->Deactivate(); });
}


  void SystemLocal::InitLogger()
  {
    P7_Set_Crash_Handler();

    logClient.reset(P7_Create_Client(TM("/P7.Sink=Baical /P7.Addr=127.0.0.1")));
    if(!logClient)
      return;
    
    logTrace.reset(P7_Create_Trace(logClient.get(), TM("Trace channel 1")));
    if(logTrace)
    {
    logTrace->Register_Thread(TM("ActorSystem"), 0);
    //IP7_Trace::hModule l_hModule    = NULL;
    //logTrace->Register_Module(TM("SystemLocal"), &l_hModule);
    }

    // stTelemetry_Conf   l_stConf     = {};
    // l_stConf.pContext              = nullptr;
    // l_stConf.pEnable_Callback      = nullptr;
    // l_stConf.pTimestamp_Callback   = nullptr;
    // l_stConf.qwTimestamp_Frequency = 0ull;
    // l_stConf.pConnect_Callback     = &Connect;

    logTelemetry.reset(P7_Create_Telemetry(logClient.get(), TM("Telemetry channel 1")));
  }
