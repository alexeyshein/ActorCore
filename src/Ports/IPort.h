#pragma once

//A port is an object that is used to establish a connection between a node

#include "json.hpp"
#include "IMessage.h"
#include <memory>
#include <functional>
#include <set>
#include <utility> //std::apir
#include <variant>
#include "IUnit.h"

namespace rf
{
using json =  nlohmann::json;

class IPort:public virtual IUnit
{
  public:
  //IPort(IAbstractActor *owner)
  virtual ~IPort()=default;
  virtual json Connections() = 0;

  //for oututs specific
  virtual void Attach( const  std::string& remotePortOwnerId, std::weak_ptr<IPort>& ptrRemotePort) = 0;
	virtual void Detach( const  std::string& remotePortOwnerId, std::weak_ptr<IPort>& ptrRemotePort) = 0;
  virtual void Detach( const  std::string& remotePortOwnerId, const std::string& remotePortId)  = 0;

	virtual void Notify(const std::shared_ptr<IMessage> &data) = 0;
  virtual std::set<std::pair<std::string, std::string>>  IdentifiersOfNotifiable() =0;
	virtual size_t NumObservers() = 0;
	virtual void CleanObservers() = 0;

  //for inputs specific
  virtual void Receive(std::shared_ptr<IMessage> data) = 0;
  virtual void SetEveventOnReceive(std::function<void(std::string,std::shared_ptr<IMessage>)>) = 0;
};
}

