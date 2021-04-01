#pragma once
#include <string>
#include "IPort.h"

namespace rf
{
class IAbstractActor;
class PortFactory
{
public:
	static  std::unique_ptr<IPort>  Create(std::string type, std::string id, std::weak_ptr<IUnit> parent = std::weak_ptr<IUnit>());
	static  std::vector<std::string> GetTypes();
};
}
