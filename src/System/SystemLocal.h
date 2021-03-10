#pragma once

#include <memory>
#include <vector>
#include "IAbstractActor.h"
#include <set>
#include <map>

namespace rf
{
  using ActorCreatorFunction = std::function<IAbstractActor *(const std::string &, const std::string &)>;
  class SystemLocal
  {
  public:
    SystemLocal();

    virtual ~SystemLocal() = default;
    bool Init(json);
    json Scheme();
    void Clear();
    std::shared_ptr<IAbstractActor> Spawn(json);        //spawn one actor
    std::shared_ptr<IAbstractActor> Spawn(std::string); //by name
    std::shared_ptr<IAbstractActor> GetActorById(std::string);
    bool Attach(std::shared_ptr<IAbstractActor> pointer);
    std::shared_ptr<IAbstractActor> Detach(std::string id);
    void RegisterFactory(std::set<std::string>, ActorCreatorFunction);
    std::set<std::string> GetRegisteredActorTypes();
    size_t countActors(){return _mapActors.size();}
    bool Connect(std::string idActor1, std::string idPortActor1, std::string idActor2, std::string idPortActor2);
    bool Connect(json);
    void Disconnect(std::string idActor1, std::string idPortActor1, std::string idActor2, std::string idPortActor2);
    void Disconnect(json);
    void Activate();
    void Deactivate();

  protected:
    void RemoveAllConectionsWithActor(std::shared_ptr<IAbstractActor>);
  protected:
    std::map<std::string, std::shared_ptr<IAbstractActor>> _mapActors;
  };
}
