#pragma once

#include "IAbstractActor.h"
#include "IPort.h"
#include <shared_mutex>
namespace rf
{
  class Logger;

  class ActorLocal : public IAbstractActor
  {
  public:
    ActorLocal(const std::string& id, IUnit* parent = nullptr);
    virtual ~ActorLocal();
    //Returns the ID of this actor.
    std::string Id() override { return _id; }
    std::string Type() override {return  _type;}
    std::string Label() override { return label; }
    void SetLabel(const std::string& lab) override { label = lab; }

    bool Init(const json&) override;
    json Configuration() override;
    bool SetUserData(const json&) override;
    json UserData() { return userData; }

    json Links() override;
    IUnit* Parent() override {return _parent;}
    void SetParent(IUnit* parent) override { _parent = parent; }

    virtual std::vector<std::weak_ptr<IUnit>> Children() override {return std::vector<std::weak_ptr<IUnit>>();}
    std::vector<std::weak_ptr<IPort>> GetPorts() override;
    std::weak_ptr<IPort> GetPortById(const std::string& portId) override;
    std::set<std::string> GetPortsIdSet() override final;

    std::variant<std::monostate, bool, int, double, std::string> GetProperty(const std::string &) override {return std::monostate{};} 
    bool SetProperty(const std::string&, bool) override{ return true; }
    bool SetProperty(const std::string&, int) override { return true; }
    bool SetProperty(const std::string&, double) override { return true; }
    bool SetProperty(const std::string&, std::string) override { return true; }
    bool SetProperties(const json&) override;// { return true; }


    json GetStatus() override;
    
    virtual void OnInputReceive(const std::string&,  std::shared_ptr<IMessage>){};
    bool ConnectTo(const std::string& actorIdExternal, std::weak_ptr<IPort>& portExternal, const std::string& portIdInternal) override;
    bool ConnectTo(std::weak_ptr<IAbstractActor>& actorExternal, const std::string& portIdExternal, const std::string& portIdInternal) override;
    void Disconnect(const std::string& actorIdExternal, std::weak_ptr<IPort>& portExternal, const std::string& portIdInternal) override; 
    void Disconnect(const std::string& actorIdExternal, const std::string& portIdExternal, const std::string& portIdInternal) override;
    void DisconnectAll(const std::string& actorIdExternal, const std::string& portIdExternal) override;

    bool IsActive() override final {return _flagActive;}
    void Activate() override {_flagActive = true;OnActivate();}
    void Deactivate() override {_flagActive = false;OnDeactivate();}

    Logger* GetLogger() { return logger? logger.get():nullptr; }

  protected:
    //void Nottify();
    std::shared_ptr<IPort> addPort(const std::string& typePort, const std::string& portId);
    std::shared_ptr<IPort> addPort(const std::string& typePort);
    std::shared_ptr<IPort> addPort(const json&);
    void deletePort(const std::string& portId);
    virtual void OnActivate(){};
    virtual void OnDeactivate(){};
  protected:
    IUnit* _parent;
    std::string _id;
    std::string _type;
    std::string label;
    std::map<std::string, std::shared_ptr<IPort>> _mapPorts;
    mutable std::shared_mutex mtx_mapPort;
    //Флаг активации актора
    bool _flagActive;

    std::unique_ptr<Logger> logger;
    json userData;
    std::string description;

  };
}
