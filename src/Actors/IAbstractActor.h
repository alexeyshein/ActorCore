#pragma once

#include <memory>
#include <vector>
#include <string>
#include <variant>

#include "IPort.h"

namespace rf
{
  class IAbstractActor:public virtual IUnit
  {
  public:
    virtual ~IAbstractActor() = default;

    virtual json Links() = 0;
    //Returns the ID of this actor.

    virtual std::vector<std::weak_ptr<IPort>> GetPorts() = 0;

    virtual std::weak_ptr<IPort> GetPortById(const std::string&) = 0;

    virtual bool ConnectTo(const std::string& actorIdExternal, std::weak_ptr<IPort>& portExternal, const std::string& portIdInternal) = 0;

    virtual bool ConnectTo(std::weak_ptr<IAbstractActor>& actorExternal, const std::string& portIdExternal, const std::string& portIdInternal) = 0;

    virtual void Disconnect(const std::string& actorIdExternal, std::weak_ptr<IPort>& portExternal, const std::string& portIdInternal) = 0; //Удаляет соединение и на своем порту и на удаленном
        
    virtual void Disconnect(const std::string& actorIdExternal, const std::string& portIdExternal, const std::string& portIdInternal) = 0; //Удаляет соединение только на своем порту
 
    // Disconnect all internall wports with external (portIdExternal)
    virtual void DisconnectAll(const std::string& actorIdExternal, const std::string& portIdExternal) = 0;

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
