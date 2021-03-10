#pragma once

#include <string>

namespace rf
{
struct IData
{
  virtual ~IData()=default;
  virtual std::string type() {return std::string("IData");};

};
}

