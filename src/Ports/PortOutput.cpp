#include "PortOutput.h"
#include <unordered_map> //std::hash<std::string>

using rf::PortOutput;

PortOutput::PortOutput(std::string id)
    : rf::PortBase(id)
{
  _type = "PortOutput";
  publisher.SetAsyncMode(true);
  publisher.SetAsyncQueueSize(0);
}

bool PortOutput::Init(json)
{
  // AsyncMode
  // AsyncQueueSize
  return true;
}


void PortOutput::Attach(const  std::string& remotePortOwnerId, std::shared_ptr<IPort>& ptrRemotePort)
{
  std::size_t linkId = std::hash<std::string>{}(remotePortOwnerId+ptrRemotePort->Id());
  std::function<void(const std::shared_ptr<IData> &)> f = std::bind(&rf::IPort::Receive, ptrRemotePort.get(), std::placeholders::_1);
  publisher.Attach(linkId, f);
  setIdentifiersOfNotifiable.emplace(std::pair<std::string,std::string>(remotePortOwnerId, ptrRemotePort->Id()));
}

void PortOutput::Detach(const  std::string& remotePortOwnerId, std::shared_ptr<IPort>& ptrRemotePort)
{
  this->Detach( remotePortOwnerId, ptrRemotePort->Id());
}

void PortOutput::Detach(const  std::string& remotePortOwnerId, const std::string& remotePortId)
{
  std::size_t linkId = std::hash<std::string>{}(remotePortOwnerId+remotePortId);
  publisher.Detach(linkId);
  setIdentifiersOfNotifiable.erase(std::pair<std::string,std::string>(remotePortOwnerId, remotePortId));
  // auto it = setIdentifiersOfNotifiable.find(std::pair<std::string,std::string>(remotePortOwnerId, remotePortId));
  // if(it!=setIdentifiersOfNotifiable.end())
  //   it = setIdentifiersOfNotifiable.erase(it);
}

void PortOutput::Notify(const std::shared_ptr<IData> &data)
{
  publisher.Notify(data);
}

