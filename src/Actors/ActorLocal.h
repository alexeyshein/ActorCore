#pragma once

#include "IAbstractActor.h"
#include "IPort.h"
namespace rf
{
  class ActorLocal : public IAbstractActor
  {
  public:
    ActorLocal(std::string id); 

    virtual ~ActorLocal() = default;

    bool Init(json) override;

    json Configuration() override;

    //Returns the ID of this actor.
    std::string id() override { return _id; }
    std::string typeId() {return  _typeId;}
    std::vector<std::shared_ptr<IPort>> GetPorts() override;
    std::shared_ptr<IPort> GetPortById(std::string portId) override;

    std::variant<bool, int, double> GetProperty(std::string) override;

    json GetStatus() override;
    virtual void onInputReceive(std::string, std::shared_ptr<IData>){};
    bool ConnectTo(std::shared_ptr<IPort> port, std::string portId) override;
    bool ConnectTo(std::shared_ptr<IAbstractActor> actorExternal, std::string portIdExternal, std::string portIdInternal) override;
    
    void Disconnect(std::shared_ptr<IPort> portExternal, std::string portIdInternal) override;
   
    void Disconnect(std::string portIdExternal, std::string portIdInternal) override; 

    void DisconnectAll(std::string portIdExternal) override;


    bool IsActive() final {return _flagActive;}

    void Activate() final {_flagActive = true;OnActivate();}

    void Deactivate() final {_flagActive = false;OnDeactivate();}

  protected:
    //void Nottify();
    std::shared_ptr<IPort> addPort(std::string typePort);
    std::shared_ptr<IPort> addPort(json);
    virtual void OnActivate(){};
    virtual void OnDeactivate(){};
    std::string _id;
    std::string _typeId;
    std::map<std::string, std::shared_ptr<IPort>> _mapPorts;
    //Флаг активации актора
    bool _flagActive;
  };
}
