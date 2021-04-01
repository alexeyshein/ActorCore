#include "PortBiDirectional.h"

using nlohmann::json;
using rf::PortBiDirectional;

PortBiDirectional::PortBiDirectional(std::string id, IUnit* parent)
    : rf::PortBase(id, parent), rf::PortInput(id, parent), rf::PortOutput(id, parent)
{
  _type = "PortBiDirectional";
}

bool PortBiDirectional::Init(const json &initJson)
{
  return rf::PortInput::Init(initJson) && rf::PortOutput::Init(initJson);
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

std::variant<std::monostate, bool, int, double, std::string> PortBiDirectional::GetProperty(const std::string &propertyName)
{
  auto res = PortInput::GetProperty(propertyName);
  if(std::holds_alternative<std::monostate>(res))
    res = PortInput::GetProperty(propertyName);
  return res;
}

bool PortBiDirectional::SetProperty(const std::string &propertyName, bool value)
{
  if(!PortInput::SetProperty(propertyName, value))
    return PortOutput::SetProperty(propertyName, value);
  return false;
}

bool PortBiDirectional::SetProperty(const std::string &propertyName, int value)
{
  if(!PortInput::SetProperty(propertyName, value))
    return PortOutput::SetProperty(propertyName, value);
  return false;
}
