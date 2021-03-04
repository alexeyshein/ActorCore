#pragma once

#include <memory>
#include <vector>
#include <string>
#include <variant>

#include "IPort.h"

namespace rf
{
  class IAbstractActor
  {
  public:
    virtual ~IAbstractActor() = default;

    virtual bool Init(json) = 0;

    //Returns the ID of this actor.
    virtual std::string id() = 0;

    virtual std::vector<std::shared_ptr<IPort>> GetPorts() = 0;

    virtual std::shared_ptr<IPort> GetPortById(std::string) = 0;

    virtual std::variant<bool, int, double> GetProperty(std::string) = 0;

    virtual bool ConnectTo(std::shared_ptr<IPort> port, std::string portId) = 0;

    virtual bool ConnectTo(std::shared_ptr<IAbstractActor> actorExternal, std::string portIdExternal, std::string portIdInternal) = 0;

    virtual void Disconnect(std::shared_ptr<IPort> portExternal, std::string portIdInternal) = 0; //Удаляет соединение и на своем порту и на удаленном
        
    virtual void Disconnect(std::string portIdExternal, std::string portIdInternal) = 0; //Удаляет соединение только на своем порту
 
    // Disconnect all internall wports with external (portIdExternal)
    virtual void DisconnectAll(std::string portIdExternal) = 0;

    virtual bool IsActive() = 0;

    virtual void Activate() = 0;

    virtual void Deactivate() = 0;

    /*!
	  \brief Функция для периодического сбора статистической информации о канале
	  \return объект json со статистической информацией
	  */
    virtual json GetStatus() = 0;
  };
}
