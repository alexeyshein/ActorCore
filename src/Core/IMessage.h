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
  virtual uint16_t Type() const { return 0; }
  virtual std::string IdSender() const { return ""; }
  virtual std::string IdPortSender() const { return ""; }
  virtual void SetSender(const std::string& idSnd_, const std::string& idPortSnd) { return; }

};
}

