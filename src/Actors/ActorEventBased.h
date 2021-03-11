#pragma once

#include <future>

#include "ActorLocal.h"

namespace rf
{
  class ActorEventBased : public ActorLocal
  {
  public:
    ActorEventBased(const std::string& id);

    virtual ~ActorEventBased() = default;

    bool Init(const json&) override;

    std::variant<bool, int, double> GetProperty(const std::string&) override;
    
    virtual void OnInputReceive(const std::string&, std::shared_ptr<IData>&) final;

  protected:
   
   virtual bool ApproveTask(const std::string&, std::shared_ptr<IData>&) {return true;};

   virtual void Process(const std::string& portId, std::shared_ptr<IData>& dataPtr) = 0;

  protected:
      std::future<void> isCalcPrev;
      std::mutex taskMutex;

  };
}
