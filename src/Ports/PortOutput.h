#pragma once

#include "PortBase.h"
#include "IMessage.h"
#include "IAbstractActor.h"
#include "MessagePublisherFunctor.hpp"
#include <memory>

namespace rf
{
  class PortOutput : public virtual PortBase
  {
  public:
    PortOutput(std::string id, IUnit* parent = nullptr);
    virtual ~PortOutput() = default;

    bool Init(const json&) override;
    json Configuration() override;

    std::variant<std::monostate, bool, int, double, std::string> GetProperty(const std::string&) override;
    bool SetProperty(const std::string&, bool) override;
    bool SetProperty(const std::string&, int) override;

    void Attach( const  std::string& remotePortOwnerId, std::weak_ptr<IPort>& ptrRemotePort) override;
	void Detach( const  std::string& remotePortOwnerId, std::weak_ptr<IPort>& ptrRemotePort) override;
    void Detach( const  std::string& remotePortOwnerId, const std::string& remotePortId)  override;
    
	void Notify(const std::shared_ptr<IMessage> &data) override;
    std::set<std::pair<std::string, std::string>>  IdentifiersOfNotifiable() override {return setIdentifiersOfNotifiable;}
    size_t NumObservers() override {return publisher.NumObservers();} 
	void CleanObservers() override {publisher.CleanObservers();} 
    void SetEventOnAttach(std::function<void(std::string, std::string, std::string)>) override;

    void SetAsyncMode(bool async){publisher.SetAsyncMode(async);}
	bool IsAsyncMode(){return publisher.IsAsyncMode();}
	void SetAsyncQueueSize(size_t size) {publisher.SetAsyncQueueSize(size);}
	size_t AsyncQueueSize() {return publisher.AsyncQueueSize();}

    void SetLinkUserData(const  std::string& remotePortOwnerId, const std::string& remotePortId, const json& userData);
    json GetLinkUserData(const  std::string& remotePortOwnerId, const std::string& remotePortId);
  
  protected:
    std::size_t CalculateLinkId(const  std::string& remotePortOwnerId, const std::string& remotePortId);
    void RemoveLinkUserDataFromMap(const  std::string& remotePortOwnerId, const std::string& remotePortId);

  protected:
    rf::MessagePublisherFunctor<std::shared_ptr<IMessage>> publisher;
    //Needs only for information matters
    std::set<std::pair<std::string, std::string>>  setIdentifiersOfNotifiable;
    std::map<std::size_t,json>  linkUserData;
    //For Telemetry Purpose
    uint16_t      teleChannelIsNotifying;
    std::function<void(std::string, std::string, std::string)> functionOnAttach; //portId, remotePortOwnerId, remotePortId

  };
}
