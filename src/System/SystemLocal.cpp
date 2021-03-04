#include "SystemLocal.h"

#include "IAbstractActor.h"
#include "ActorFactoryCollection.hpp"
#include "UidGenerator.hpp"

using nlohmann::json;
using rf::ActorCreatorFunction;
using rf::IAbstractActor;
using rf::SystemLocal;

SystemLocal::SystemLocal()
{
}

//by json
std::shared_ptr<IAbstractActor> SystemLocal::Spawn(json jsonActor)
{
  std::shared_ptr<IAbstractActor> actorPtr = nullptr;
  try
  {
    std::string typeName = jsonActor["type"].get<std::string>();
    std::string id = jsonActor["id"].get<std::string>();
    std::shared_ptr<IAbstractActor> actorPtr(ActorFactoryCollection::Create(typeName, id));
    _mapActors.emplace(std::make_pair(actorPtr->id(), actorPtr));
    auto res = SystemLocal::Spawn(typeName);
    res->Init(jsonActor);
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
  if (actorPtr)
    _mapActors.emplace(std::make_pair(actorPtr->id(), actorPtr));
  return actorPtr;
}

bool SystemLocal::Attach(std::shared_ptr<IAbstractActor> actorPtr)
{
  if (_mapActors.count(actorPtr->id()) != 0)
    return false;
  _mapActors.emplace(std::make_pair(actorPtr->id(), actorPtr));
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

void SystemLocal::Disconnect(std::string idActor1, std::string idPortActor1, std::string idActor2, std::string idPortActor2)
{
  auto actor1 = GetActorById(idActor1);
  auto actor2 = GetActorById(idActor2);
  if (actor1 && actor2)
  {
    auto port2 = actor2->GetPortById(idPortActor2);
    if (port2)
      actor1->Disconnect(port2, idPortActor1);

    auto port1 = actor2->GetPortById(idPortActor2);
    if (port1)
      actor2->Disconnect(port1, idPortActor2);
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
      actor->DisconnectAll(actorPort->id());
    }
  }
}
