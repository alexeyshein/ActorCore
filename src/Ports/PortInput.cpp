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
  if(config.contains("queueSize"))
   if(config.at("queueSize").is_number_integer())
   _queuePtrData.setMaxSize(config.at("queueSize").get<int>());

  if(config.contains("queueModeFull"))
   if(config.at("queueModeFull").is_string())
   {
     std::string modeString =  config.at("isTrigger").get<std::string>();
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
  config["queueSize"] =  _queuePtrData.getMaxSize();
  std::string strMode{"Skip"};
  if(_queuePtrData.getModeFull() ==  ModeQueueFull::PopOld)
    strMode = "PopOld";
  config["queueModeFull"] = strMode;
  return  config;
}

void PortInput::Receive(std::shared_ptr<IData> dataPtr)
{
   _queuePtrData.push_back(dataPtr);
   if(isTrigger && functionOnRecive)
     functionOnRecive(_id, dataPtr);
}

void PortInput::SetEveventOnReceive(std::function<void(std::string,std::shared_ptr<IData>)> func)
{
   functionOnRecive = func;
}

