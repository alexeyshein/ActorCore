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

    virtual bool Init(const json&) = 0;

    virtual json Configuration() = 0;

    //Returns the ID of this actor.
    virtual std::string Id() = 0;

    virtual std::vector<std::shared_ptr<IPort>> GetPorts() = 0;

    virtual std::shared_ptr<IPort> GetPortById(const std::string&) = 0;

    virtual std::variant<bool, int, double> GetProperty(const std::string&) = 0;

    virtual bool ConnectTo(std::shared_ptr<IPort>& port, const std::string& portId) = 0;

    virtual bool ConnectTo(std::shared_ptr<IAbstractActor>& actorExternal, const std::string& portIdExternal, const std::string& portIdInternal) = 0;

    virtual void Disconnect(std::shared_ptr<IPort>& portExternal, const std::string& portIdInternal) = 0; //Удаляет соединение и на своем порту и на удаленном
        
    virtual void Disconnect(const std::string& portIdExternal, const std::string& portIdInternal) = 0; //Удаляет соединение только на своем порту
 
    // Disconnect all internall wports with external (portIdExternal)
    virtual void DisconnectAll(const std::string& portIdExternal) = 0;

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
