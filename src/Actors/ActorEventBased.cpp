
#include "ActorEventBased.h"
using rf::ActorEventBased;
using rf::ActorLocal;
using rf::IPort;

ActorEventBased::ActorEventBased(const std::string& id) : ActorLocal(id)
{
    _type = "ActorEventBased" ;
}


bool ActorEventBased::Init(const json&)
{
  return false;
}


std::variant<bool, int, double> ActorEventBased::GetProperty(const std::string&)
{
  return std::variant<bool, int, double>();
}


void ActorEventBased::OnInputReceive(const std::string& portId, std::shared_ptr<IData>& dataPtr)
{
  if(!ApproveTask(portId, dataPtr))
    return;

  const std::lock_guard<std::mutex> lock(taskMutex);
  if (isCalcPrev.valid())
     {  // If  busy then return
         //std::shared_ptr<std::vector<std::deque<rf::Point2d<double>>>>
         auto status = isCalcPrev.wait_for(std::chrono::microseconds(100));
         if(status == std::future_status::timeout){
             return;
         }
         else if(status == std::future_status::ready){
             //currentProfilePtr.reset(); //очищаем
         }
     }

      isCalcPrev =   std::async(std::launch::async, &ActorEventBased::Process, this, portId, dataPtr);
} 


