#pragma once

#include <string>

namespace rf
{
struct IMessage
{
  virtual ~IMessage()=default;
  virtual std::string type() {return std::string("IMessage");};

};
}

