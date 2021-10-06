#pragma once

#include <string>
#include <variant>

#include "json.hpp"

namespace rf
{
  using json =  nlohmann::json;

class IUnit
{
  public:
  virtual ~IUnit() = default;
  virtual std::string Id()  = 0;
  virtual std::string Type() = 0;//{return std::string("IUnit");};
  virtual bool Init(const json&) = 0;
  virtual json Configuration() = 0;
  virtual json UserData() = 0;
  virtual IUnit* Parent() = 0;
  virtual std::vector<std::weak_ptr<IUnit>> Children() = 0;
  virtual std::variant<std::monostate, bool, int, double, std::string> GetProperty(const std::string&) = 0;
  virtual bool SetProperty(const std::string&, bool) = 0;
  virtual bool SetProperty(const std::string&, int) = 0;
  virtual bool SetProperty(const std::string&, double) = 0;
  virtual bool SetProperty(const std::string&, std::string) = 0;  
  virtual bool SetProperties(const json&) = 0;

};
}

