#pragma once

#include <future>

#include "ActorLocal.h"
#include "SharedQueue.h"

namespace rf
{
  class ActorEventBased : public ActorLocal
  {
  public:
    ActorEventBased(const std::string &id);
    virtual ~ActorEventBased() = default;
    bool Init(const json &) override;
    json Configuration() override;
    std::variant<std::monostate, bool, int, double, std::string> GetProperty(const std::string &) override ;
    bool SetProperty(const std::string&, bool) override ;
    bool SetProperty(const std::string&, int) override ;
    virtual void OnInputReceive(const std::string &, std::shared_ptr<IMessage> &) final;
   
    void Activate() final {ActorLocal::Activate();}
    void Deactivate() final {ActorLocal::Deactivate();}
  protected:
    virtual bool ApproveTask(const std::string &, std::shared_ptr<IMessage> &) { return true; };
    virtual void Process(const std::string &portId, std::shared_ptr<IMessage> &dataPtr) = 0;
    
    void ProcessWrap(const std::string &portId, std::shared_ptr<IMessage> &dataPtr);

  private:
    void SanitizeQueue();

  protected:
    SharedQueue<std::future<void>> myFutureQueue;
    bool isAsync;

    // For Telemetry purpose
    uint16_t      teleChannelActiveTasks;
    uint16_t      teleChannelIsProcessing;

  };
}
