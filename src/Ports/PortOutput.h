#pragma once

#include "PortBase.h"
#include "IData.h"
#include "IAbstractActor.h"
#include "DataPublisherFunctor.hpp"
#include <memory>

namespace rf
{
  class PortOutput : public virtual PortBase
  {
  public:
    PortOutput(std::string id);
    virtual ~PortOutput() = default;

    virtual bool Init(json) override;

    void Attach( const  std::string& remotePortOwnerId, std::shared_ptr<IPort>& ptrRemotePort) override;
	  void Detach( const  std::string& remotePortOwnerId, std::shared_ptr<IPort>& ptrRemotePort) override;
    void Detach( const  std::string& remotePortOwnerId, const std::string& remotePortId)  override;
    
	  void Notify(const std::shared_ptr<IData> &data) override;
    std::set<std::pair<std::string, std::string>>  IdentifiersOfNotifiable() override {return setIdentifiersOfNotifiable;}
    size_t NumObservers() override {return publisher.NumObservers();} 
	  void CleanObservers() override {publisher.CleanObservers();} 

    void SetAsyncMode(bool async){publisher.SetAsyncMode(async);}
	  bool IsAsyncMode(){return publisher.IsAsyncMode();}
	  void SetAsyncQueueSize(size_t size) {publisher.SetAsyncQueueSize(size);}
	  size_t AsyncQueueSize() {return publisher.AsyncQueueSize();}
  
  protected:
    rf::DataPublisherFunctor<std::shared_ptr<IData>> publisher;
    //Needs only for information matters
    std::set<std::pair<std::string, std::string>>  setIdentifiersOfNotifiable;
  };
}
