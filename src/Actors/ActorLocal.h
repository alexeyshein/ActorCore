#pragma once

#include "IAbstractActor.h"
#include "IPort.h"
namespace rf
{
  class ActorLocal : public IAbstractActor
  {
  public:
    ActorLocal(const std::string& id); 

    virtual ~ActorLocal() = default;

    bool Init(const json&) override;

    json Configuration() override;
    json Connections() override;

    //Returns the ID of this actor.
    std::string Id() override { return _id; }
    std::string Type() {return  _type;}
    std::vector<std::shared_ptr<IPort>> GetPorts() override;
    std::shared_ptr<IPort> GetPortById(const std::string& portId) override;

    std::variant<bool, int, double> GetProperty(const std::string &) override;

    json GetStatus() override;
    virtual void OnInputReceive(const std::string&, std::shared_ptr<IMessage>&){};
   
    bool ConnectTo(const std::string& actorIdExternal, std::shared_ptr<IPort>& portExternal, const std::string& portIdInternal) override;

    bool ConnectTo(std::shared_ptr<IAbstractActor>& actorExternal, const std::string& portIdExternal, const std::string& portIdInternal) override;

    void Disconnect(const std::string& actorIdExternal, std::shared_ptr<IPort>& portExternal, const std::string& portIdInternal) override;
        
    void Disconnect(const std::string& actorIdExternal, const std::string& portIdExternal, const std::string& portIdInternal) override;

    void DisconnectAll(const std::string& actorIdExternal, const std::string& portIdExternal) override;

    bool IsActive() final {return _flagActive;}

    void Activate() final {_flagActive = true;OnActivate();}

    void Deactivate() final {_flagActive = false;OnDeactivate();}

  protected:
    //void Nottify();
    std::shared_ptr<IPort> addPort(const std::string& typePort, const std::string& portId);
    std::shared_ptr<IPort> addPort(const std::string& typePort);
    std::shared_ptr<IPort> addPort(const json&);
    virtual void OnActivate(){};
    virtual void OnDeactivate(){};
    std::string _id;
    std::string _type;
    std::map<std::string, std::shared_ptr<IPort>> _mapPorts;
    //Флаг активации актора
    bool _flagActive;
  };
}
