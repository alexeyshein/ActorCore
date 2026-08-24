#pragma once

#include "PortBase.h"
#include "SharedQueue.h"
#include "IMessage.h"
#include "PortRuntimeStats.hpp"

namespace rf
{
class PortInput: virtual public  PortBase
{
  public:
  PortInput(std::string id, IUnit* parent = nullptr);

  virtual ~PortInput()=default;

  bool Init(const json&) override;
  json Configuration() override;
  
  std::variant<std::monostate, bool, int, double, std::string> GetProperty(const std::string&) override;
  bool SetProperty(const std::string&, bool) override;
  bool SetProperty(const std::string&, int) override;
  bool SetProperty(const std::string&, std::string) override;

  void Receive(std::shared_ptr<IMessage> data) override;
  void SetEventOnReceive(std::function<void(std::string,std::shared_ptr<IMessage>)>)  override;
  
  SharedQueue<std::shared_ptr<IMessage>>& GetMessageQueueRef() {return _queuePtrData;} 
  
  json GetRuntimeStatus()  const override;

protected:
   bool isTrigger;
   SharedQueue<std::shared_ptr<IMessage>> _queuePtrData;
   std::function<void(std::string,std::shared_ptr<IMessage>)> functionOnRecive;


  private:
  uint16_t      teleChannelQueueSizeId;
};
}

