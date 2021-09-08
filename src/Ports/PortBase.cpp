#include "PortBase.h"
#include "Logger.h"

using  rf::PortBase;
using rf::Logger;
using nlohmann::json;

PortBase::PortBase(std::string id, IUnit* parent):
 _parent(parent)
,_id(id)
, _type("PortBase")
, logger(new Logger())
{
  logger->ConnectToShare("ActorSystem log client", "Actor System Trace channel", "Actor System Telemetry channel");
}

PortBase::~PortBase(){
  
};

json PortBase::Configuration()
{
     return {
      {"id", _id},
      {"type", _type},
  };
}

json PortBase::Links()
{
    json connections = json::array();
    //auto &mapExternals = this->IdentifiersOfNotifiable(); TMP not work
    auto mapExternals = this->IdentifiersOfNotifiable();

    for (const auto &[actorIdExternal, portIdExternal] : mapExternals)
    {
      connections.emplace_back(json({actorIdExternal, portIdExternal}));
    }
    return connections;
}

bool PortBase::Init(const json&) 
{
    return true;
}


void PortBase::SetTypesMessages(std::set<std::string> typesMessages_)
{
  typesMessages = typesMessages_;
}



