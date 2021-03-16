#include "PortInput.h"

using rf::PortInput;
using nlohmann::json;

PortInput::PortInput(std::string id) 
: PortBase(id)
, functionOnRecive(nullptr)
, isTrigger(true)
, _queuePtrData(0)
{
  _type = "PortInput";
}

bool PortInput::Init(const json& config)
{
  if(!PortBase::Init(config))
   return false;
  // Задать триггерный ли вход
  if(config.contains("isTrigger"))
   if(config.at("isTrigger").is_boolean())
     isTrigger = config.at("isTrigger").get<bool>();
  // Задать размер входной очереди
  if(config.contains("queueMessagesSize"))
   if(config.at("queueMessagesSize").is_number_integer())
   _queuePtrData.setMaxSize(config.at("queueMessagesSize").get<int>());

  if(config.contains("queueMessagesModeFull"))
   if(config.at("queueMessagesModeFull").is_string())
   {
     std::string modeString =  config.at("queueMessagesModeFull").get<std::string>();
     if(modeString.compare("Skip") == 0)
        _queuePtrData.setModeFull(ModeQueueFull::Nothing);
      else if(modeString.compare("PopOld") == 0)
        _queuePtrData.setModeFull(ModeQueueFull::PopOld);
   }
  return true;
}

json PortInput::Configuration()
{
  auto config = PortBase::Configuration();
  config["isTrigger"] = isTrigger;
  config["queueMessagesSize"] =  _queuePtrData.getMaxSize();
  std::string strMode{"Skip"};
  if(_queuePtrData.getModeFull() ==  ModeQueueFull::PopOld)
    strMode = "PopOld";
  config["queueMessagesModeFull"] = strMode;
  return  config;
}

std::variant<std::monostate, bool, int, double, std::string> PortInput::GetProperty(const std::string& propertyName)
{
  if(propertyName.compare("isTrigger") == 0)
    return isTrigger;
  else if(propertyName.compare("queueMessagesSize") == 0)
    return static_cast<int>(_queuePtrData.getMaxSize());
  else if(propertyName.compare("queueMessagesModeFull") == 0)
  {
      std::string strMode{"Skip"};
        if(_queuePtrData.getModeFull() ==  ModeQueueFull::PopOld)
          strMode = "PopOld";
      return strMode;
  }
  return PortBase::GetProperty(propertyName);
}


bool PortInput::SetProperty(const std::string& propertyName, bool value) 
{
  if(propertyName.compare("isTrigger"))
    {
      isTrigger = value;
      return true;
    }
  return PortBase::SetProperty(propertyName, value);
}

bool  PortInput::SetProperty(const std::string& propertyName, int value)
{
  if(propertyName.compare("queueMessagesSize"))
    {
      _queuePtrData.setMaxSize(value);
      return true;
    }
   return PortBase::SetProperty(propertyName, value);
}

bool PortInput::SetProperty(const std::string& propertyName, std::string value) 
{
  if(propertyName.compare("queueMessagesModeFull"))
    {
      if(value.compare("Skip") == 0)
        _queuePtrData.setModeFull(ModeQueueFull::Nothing);
      else if(value.compare("PopOld") == 0)
        _queuePtrData.setModeFull(ModeQueueFull::PopOld);
      return true;
    }
   return PortBase::SetProperty(propertyName, value);
}


void PortInput::Receive(std::shared_ptr<IMessage> dataPtr)
{
   _queuePtrData.push_back(dataPtr);
   if(isTrigger && functionOnRecive)
     functionOnRecive(_id, dataPtr);
}

void PortInput::SetEveventOnReceive(std::function<void(std::string,std::shared_ptr<IMessage>)> func)
{
   functionOnRecive = func;
}

