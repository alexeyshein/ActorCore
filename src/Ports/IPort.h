#pragma once

//A port is an object that is used to establish a connection between a node

#include "json.hpp"
#include "IData.h"
#include <memory>
#include <functional>

namespace rf
{
using json =  nlohmann::json;

class IPort
{
  public:
  //IPort(IAbstractActor *owner)
  virtual ~IPort()=default;
  virtual std::string id()  = 0;
  virtual bool Init(json) = 0;

  //for oututs specific
  virtual void Attach(std::shared_ptr<IPort> ptrRemotePort) = 0;
	virtual void Detach(std::shared_ptr<IPort> ptrRemotePort) = 0;
  virtual void Detach(std::string remotePortId)  = 0;

	virtual void Notify(const std::shared_ptr<IData> &data) = 0;
	virtual size_t NumObservers() = 0;
	virtual void CleanObservers() = 0;

  //for inputs specific
  virtual void Receive(std::shared_ptr<IData> data) = 0;
  virtual void SetEveventOnReceive(std::function<void(std::string,std::shared_ptr<IData>)>) = 0;
};
}

