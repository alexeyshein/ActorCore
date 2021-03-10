#pragma once

#include "IPort.h"
#include "IData.h"
#include "DataPublisherFunctor.hpp"
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
    virtual bool Init(json) override;
    
    void Attach(std::shared_ptr<IPort> ptrRemotePort) override{}
	  void Detach(std::shared_ptr<IPort> ptrRemotePort) override{}
    void Detach(std::string remotePortId)  override {}

	  void Notify(const std::shared_ptr<IData> &data) override{}
	  size_t NumObservers() override{return 0;}
	  void CleanObservers() override{}
       //for inputs specific
    void Receive(std::shared_ptr<IData> data) override{}
    void SetEveventOnReceive(std::function<void(std::string,std::shared_ptr<IData>)>)  override{}

  protected:
    std::string _id;
    std::string _type;
  };
}
