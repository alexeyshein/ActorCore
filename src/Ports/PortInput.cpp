#include "PortInput.h"

#include <limits>

#include "Logger.h"
#include "RuntimeClock.hpp"   
#include "FlowTraceRecorder.h"   
#include "FlowTraceTypes.hpp"

using rf::PortInput;
using rf::Logger;
using nlohmann::json;

PortInput::PortInput(std::string id, IUnit* parent) 
: PortBase(id, parent )
, functionOnRecive(nullptr)
, isTrigger(true)
, _queuePtrData(2)
{
  _type = "PortInput";
  std::string idParent{""};
  if(parent != nullptr)
     idParent = parent->Id();
  //std::wstring telemetryName{Logger::StrToWstr(idParent)+L"=>"+Logger::StrToWstr(id)+L"_queueSize"};
  std::string telemetryName{ idParent + "=>" + id + "_queueSize" };
  logger->CreateTelemetryChannel(telemetryName.c_str(), 0,-1,255,255,true, &teleChannelQueueSizeId);
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
  if(propertyName.compare("isTrigger")==0)
    {
      isTrigger = value;
      return true;
    }
  return PortBase::SetProperty(propertyName, value);
}

bool  PortInput::SetProperty(const std::string& propertyName, int value)
{
  if(propertyName.compare("queueMessagesSize")==0)
    {
      _queuePtrData.setMaxSize(value);
      return true;
    }
   return PortBase::SetProperty(propertyName, value);
}

bool PortInput::SetProperty(const std::string& propertyName, std::string value) 
{
  if(propertyName.compare("queueMessagesModeFull")==0)
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
    //TODO add check for compliance with data and typesMessages
    if (_queuePtrData.getMaxSize() > 0)
    {
        // --- track drops ---
        bool wasFull = _queuePtrData.isFull()
            && _queuePtrData.getModeFull() == ModeQueueFull::Nothing;

        _queuePtrData.push_back(dataPtr);
        logger->Telemetry(teleChannelQueueSizeId, _queuePtrData.size());
        if (wasFull)
            _runtimeStats.RecordDrop();
        else
            _runtimeStats.RecordActivity();
    }
    else
    {
        // 
        _runtimeStats.RecordActivity();
    }
    // --- update runtime stats ---
    if (_flowTraceRecorder && _flowTraceRecorder->IsEnabled())
    {
        _flowTraceRecorder->Record({
            SteadyTimeUs(),
            FlowTraceEventType::PortReceive,
            FlowTraceHash(_parent ? _parent->Id() : ""),
            FlowTraceHash(_id),
            dataPtr->Id(),
            dataPtr->Type(),
            FlowTraceHash(dataPtr->IdSender()),
            FlowTraceHash(dataPtr->IdPortSender()),
            static_cast<uint32_t>(_queuePtrData.getMaxSize() > 0 ? _queuePtrData.size() : 0)
            });
    }

   if(isTrigger && functionOnRecive)
     functionOnRecive(_id, dataPtr);
}

void PortInput::SetEventOnReceive(std::function<void(std::string,std::shared_ptr<IMessage>)> func)
{
   functionOnRecive = func;
}

json PortInput::GetRuntimeStatus() const
{
    auto j = PortBase::GetRuntimeStatus();
    j["direction"] = "input";
    j["isTrigger"] = isTrigger;
    j["queueSize"] = _queuePtrData.size();
    j["queueCapacity"] = static_cast<int>(_queuePtrData.getMaxSize());
    return j;
}