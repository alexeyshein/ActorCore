#pragma once

#include <future>
#include "ActorLocal.h"
#include "SharedQueue.hpp"

namespace rf
{
  class ActorEventBased : public ActorLocal
  {
  public:
    ActorEventBased(const std::string &id);

    virtual ~ActorEventBased() = default;

    bool Init(const json &) override;

    json Configuration() override;

    std::variant<bool, int, double> GetProperty(const std::string &) override;

    virtual void OnInputReceive(const std::string &, std::shared_ptr<IData> &) final;

  protected:
    virtual bool ApproveTask(const std::string &, std::shared_ptr<IData> &) { return true; };

    virtual void Process(const std::string &portId, std::shared_ptr<IData> &dataPtr) = 0;

  private:
    void SanitizeQueue();

  protected:
    SharedQueue<std::future<void>> myFutureQueue;
    bool isAsync;
  };
}
