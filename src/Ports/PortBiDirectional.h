#pragma once

#include "PortInput.h"
#include "PortOutput.h"
#include "IAbstractActor.h"

namespace rf
{
class PortBiDirectional:public PortInput , public PortOutput
{
  public:

  PortBiDirectional(std::string id);

  virtual ~PortBiDirectional()=default;

  bool Init(const json&) override;
  
  json Configuration() override;

};
}

