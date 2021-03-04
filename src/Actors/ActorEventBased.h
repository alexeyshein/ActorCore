#pragma once

#include "ActorLocal.h"

namespace rf
{
  class ActorEventBased : public ActorLocal
  {
  public:
    ActorEventBased(std::string id);

    virtual ~ActorEventBased() = default;

    bool Init(json) override;

    std::variant<bool, int, double> GetProperty(std::string) override;

  protected:

  };
}
