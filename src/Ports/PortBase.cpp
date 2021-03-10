#include "PortBase.h"

using  rf::PortBase;
using nlohmann::json;

PortBase::PortBase(std::string id):
 _id(id)
, _type("PortBase")
{

}

json PortBase::Configuration()
{
     return {
      {"id", _id},
      {"type", _type},
  };
}

json PortBase::Connections()
{
    json connections = json::array();
    auto &mapExternals = this->IdentifiersOfNotifiable();
    for (const auto &[actorIdExternal, portIdExternal] : mapExternals)
    {
      connections.emplace_back(json({actorIdExternal, portIdExternal}));
    }
    return connections;
}

bool PortBase::Init(json) 
{
    return false;
}




