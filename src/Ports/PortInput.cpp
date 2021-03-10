#include "PortInput.h"

using rf::PortInput;

PortInput::PortInput(std::string id) 
: PortBase(id)
, functionOnRecive(nullptr)
, isTrigger(true)
, _queuePtrData(0)
{
  _type = "PortInput";
}

bool PortInput::Init(json)
{
  // Задать размер входной очереди
  // Задать триггерный ли вход
  return true;
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

