#pragma once

#include <string>

namespace rf
{
struct IData
{
  virtual ~IData()=default;
  virtual std::string typeId() {return std::string("IData");};

};
}

