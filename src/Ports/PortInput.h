#pragma once

#include "PortBase.h"
#include "SharedQueue.hpp"
#include "IData.h"

namespace rf
{
class PortInput: public virtual PortBase
{
  public:
  PortInput(std::string id);

  virtual ~PortInput()=default;

  bool Init(const json&) override;
  json Configuration() override;

  void Receive(std::shared_ptr<IData> data) override;
  void SetEveventOnReceive(std::function<void(std::string,std::shared_ptr<IData>)>)  override;

  protected:
   bool isTrigger;
   rf::SharedQueue<std::shared_ptr<IData>> _queuePtrData;
   std::function<void(std::string,std::shared_ptr<IData>)> functionOnRecive;

};
}

