#include "PortOutput.h"
#include <unordered_map> //std::hash<std::string>

using rf::PortOutput;

PortOutput::PortOutput(std::string id)
    : rf::PortBase(id)
{
  _typeId = "PortOutput";
  publisher.SetAsyncMode(true);
  publisher.SetAsyncQueueSize(0);
}

bool PortOutput::Init(json)
{
  // AsyncMode
  // AsyncQueueSize
  return true;
}

void PortOutput::Attach(std::shared_ptr<rf::IPort> ptrRemotePort)
{
  std::size_t linkId = std::hash<std::string>{}(ptrRemotePort->id());
  std::function<void(const std::shared_ptr<IData> &)> f = std::bind(&rf::IPort::Receive, ptrRemotePort.get(), std::placeholders::_1);
  publisher.Attach(linkId, f);
}

void PortOutput::Detach(std::shared_ptr<IPort> ptrRemotePort)
{
  this->Detach(ptrRemotePort->id());
}

void PortOutput::Detach(std::string remotePortId)
{
  std::size_t linkId = std::hash<std::string>{}(remotePortId);
  publisher.Detach(linkId);
}

void PortOutput::Notify(const std::shared_ptr<IData> &data)
{
  publisher.Notify(data);
}