#pragma once

#include <string>

namespace rf
{
struct IMessage
{
  virtual ~IMessage()=default;
  //static std::string TypeName(){return std::string("IMessage");};
  virtual uint64_t Id() const {return 0;}
  virtual uint64_t Timestamp() const {return 0;}
};
}

