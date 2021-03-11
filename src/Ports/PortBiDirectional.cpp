#include "PortBiDirectional.h"

using  rf::PortBiDirectional;
using nlohmann::json;

PortBiDirectional::PortBiDirectional(std::string id)
: rf::PortBase(id)
, rf::PortInput(id)
, rf::PortOutput(id)
{
  _type = "PortBiDirectional";

}

bool PortBiDirectional::Init(const json& initJson) 
{
  return rf::PortInput::Init(initJson)&&rf::PortOutput::Init(initJson);
}


json PortBiDirectional::Configuration() 
{
   auto config = rf::PortInput::Configuration();
   auto configOutput = rf::PortInput::Configuration();
    //Merge two json object
    config.insert(configOutput.begin(), configOutput.end()); // --> a=1
    // for (const auto &j : json::iterator_wrapper(configOutput)) {
    //     config[j.key()] = j.value();
    // }
   return config;
}





