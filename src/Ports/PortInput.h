#pragma once

#include "PortBase.h"
#include "SharedQueue.hpp"
#include "IMessage.h"

namespace rf
{
class PortInput: public virtual PortBase
{
  public:
  PortInput(std::string id);

  virtual ~PortInput()=default;

  bool Init(const json&) override;
  json Configuration() override;
  
  std::variant<std::monostate, bool, int, double, std::string> GetProperty(const std::string&) override;
  bool SetProperty(const std::string&, bool) override;
  bool SetProperty(const std::string&, int) override;
  bool SetProperty(const std::string&, std::string) override;

  void Receive(std::shared_ptr<IMessage> data) override;
  void SetEveventOnReceive(std::function<void(std::string,std::shared_ptr<IMessage>)>)  override;

  protected:
   bool isTrigger;
   rf::SharedQueue<std::shared_ptr<IMessage>> _queuePtrData;
   std::function<void(std::string,std::shared_ptr<IMessage>)> functionOnRecive;

};
}

