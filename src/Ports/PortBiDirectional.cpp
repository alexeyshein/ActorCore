#include "PortBiDirectional.h"

using  rf::PortBiDirectional;

PortBiDirectional::PortBiDirectional(std::string id)
: rf::PortBase(id)
, rf::PortInput(id)
, rf::PortOutput(id)
{
  _typeId = "PortBiDirectional";

}

bool PortBiDirectional::Init(json initJson) 
{
  return rf::PortInput::Init(initJson)&&rf::PortOutput::Init(initJson);
}




