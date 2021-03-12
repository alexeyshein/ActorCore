#pragma once

#include "IPort.h"
#include "IMessage.h"
#include "MessagePublisherFunctor.hpp"
#include <memory>
#include <string>

namespace rf
{
  class PortBase : public IPort
  {
  public:
    PortBase(std::string id);
    virtual ~PortBase() = default;
    std::string Id() override { return _id; }
    std::string Type() {return  _type;}
    bool Init(const json&) override;
    json Configuration() override;
    json Connections() override;

    void Attach( const  std::string& remotePortOwnerId, std::shared_ptr<IPort>& ptrRemotePort) override{}
	  void Detach( const  std::string& remotePortOwnerId, std::shared_ptr<IPort>& ptrRemotePort) override{}
    void Detach( const  std::string& remotePortOwnerId, const std::string& remotePortId)  override{}

	  void Notify(const std::shared_ptr<IMessage> &data) override{}
	  size_t NumObservers() override{return 0;}
	  void CleanObservers() override{}
    std::set<std::pair<std::string, std::string>>  IdentifiersOfNotifiable() override {return std::set<std::pair<std::string, std::string>>();}

       //for inputs specific
    void Receive(std::shared_ptr<IMessage> data) override{}
    void SetEveventOnReceive(std::function<void(std::string,std::shared_ptr<IMessage>)>)  override{}

  protected:
    std::string _id;
    std::string _type;
  };
}
