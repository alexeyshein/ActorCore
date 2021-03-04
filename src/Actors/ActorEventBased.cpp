
#include "ActorEventBased.h"

using rf::ActorEventBased;
using rf::ActorLocal;
using rf::IPort;

ActorEventBased::ActorEventBased(std::string id) : ActorLocal(id)
{
    _typeId = "ActorEventBased" ;
}



bool ActorEventBased::Init(json)
{
  return false;
}

std::variant<bool, int, double> ActorEventBased::GetProperty(std::string)
{
  return std::variant<bool, int, double>();
}
