#pragma once

#include "PortBase.h"
#include "SharedQueue.h"
#include "IMessage.h"

namespace rf
{
class PortInput: public virtual PortBase
{
  public:
  PortInput(std::string id, std::weak_ptr<IUnit> parent = std::weak_ptr<IUnit>());

  virtual ~PortInput()=default;

  bool Init(const json&) override;
  json Configuration() override;
  
  std::variant<std::monostate, bool, int, double, std::string> GetProperty(const std::string&) override;
  bool SetProperty(const std::string&, bool) override;
  bool SetProperty(const std::string&, int) override;
  bool SetProperty(const std::string&, std::string) override;

  void Receive(std::shared_ptr<IMessage> data) override;
  void SetEveventOnReceive(std::function<void(std::string,std::shared_ptr<IMessage>)>)  override;
  
  SharedQueue<std::shared_ptr<IMessage>>& GetMessageQueueRef(){return _queuePtrData;} 
  protected:
   bool isTrigger;
   SharedQueue<std::shared_ptr<IMessage>> _queuePtrData;
   std::function<void(std::string,std::shared_ptr<IMessage>)> functionOnRecive;
  private:
  uint16_t      teleChannelQueueSizeId;
};
}

