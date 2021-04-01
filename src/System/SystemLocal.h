#pragma once

#include <memory>
#include <vector>
#include "IAbstractActor.h"
#include <set>
#include <map>


namespace rf
{
  class Logger;
  using ActorCreatorFunction = std::function<IAbstractActor *(const std::string &, const std::string &)>;
  class SystemLocal :public virtual IUnit
  {
  public:
    SystemLocal();
    virtual ~SystemLocal();
    json Scheme();
    void Clear();
    std::weak_ptr<IAbstractActor> Spawn(json);        //spawn one actor
    std::weak_ptr<IAbstractActor> Spawn(std::string); //by name
    std::weak_ptr<IAbstractActor> GetActorById(std::string);
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
    
    ///////////////////////////////////////////////////////////////
    //IUnit
    //////////////////////////////////////////////////////////////
    std::string Id()  override {return std::string("SystemLocal");}
    std::string Type() override {return std::string("SystemLocal");}
    bool Init(const json&) override;
    json Configuration() override {return Scheme();}
    std::weak_ptr<IUnit> Parent() override {return parent;}
    std::vector<std::weak_ptr<IUnit>> Children() override;
    std::variant<std::monostate, bool, int, double, std::string> GetProperty(const std::string &) override {return std::monostate{};} 
    bool SetProperty(const std::string&, bool) override{ return true; }
    bool SetProperty(const std::string&, int) override { return true; }
    bool SetProperty(const std::string&, double) override { return true; }
    bool SetProperty(const std::string&, std::string) override { return true; }

  private:
    void RemoveAllConectionsWithActor(std::weak_ptr<IAbstractActor>);
    void InitLogger(std::wstring initParams);
    
  protected:
    std::map<std::string, std::shared_ptr<IAbstractActor>> _mapActors;
    std::unique_ptr<Logger> logger;
    
    std::weak_ptr<IUnit> parent;
  };
}
