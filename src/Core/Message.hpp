#pragma once

#include <string>
#include "IMessage.h"

namespace rf
{

template< typename T > 
class Message : public IMessage
{
public:
    static std::string TypeName(){ return std::string(typeName);}

    Message(uint64_t id = 0, uint64_t timestamp = 0);
    Message(uint64_t id, uint64_t timestamp, uint16_t type);
    Message(uint64_t id, uint64_t timestamp, uint16_t type,  const T&  data);
    Message(uint64_t id, uint64_t timestamp, uint16_t type,   T&& data);
    ~Message(){};
    IMessage* Copy() const override { return new Message<T>(*this); }
    void Set(uint64_t id, uint64_t timestamp, uint16_t type, const T&  data);
    void Set(uint64_t id, uint64_t timestamp, uint16_t type,  T&& data);
    uint64_t Id() const override { return id; }
    void SetId(uint64_t _id) override { id = _id; }
    uint64_t Timestamp() const override { return timestamp; }
    void SetTimestamp(uint64_t ts) override { timestamp = ts; }
    uint16_t Type() const override { return type; }
    std::string IdSender() const { return idSender; }
    std::string IdPortSender() const { return idPortSender; }
    void SetSender(const std::string& idSnd_, const std::string& idPortSnd) override { idSender = idSnd_; idPortSender = idPortSnd; }
    T& DataRef()  { return data; }
    T* DataPtr()  { return &data;}
    const T& DataConstRef() const { return data; }
    const T* DataConstPtr() const { return &data;}
    T GetData() const { return data; }

protected:
private:
    static const char* typeName;//="Message";
    uint64_t id;
    uint64_t timestamp;
    uint16_t type;
    std::string idSender;
    std::string idPortSender;
    //std::string typeName;
    T data;
};

//std::string Message::TypeName { return std::string(typeName);}



template< typename T  > 
Message<T>::Message(uint64_t id_, uint64_t timestamp_)
: id(id_)
, timestamp(timestamp_)
, type(0)
{
}

template< typename T  >
Message<T>::Message(uint64_t id_ , uint64_t timestamp_, uint16_t type_)
    : id(id_)
    , timestamp(timestamp_)
    , type(type_)
{
}


template< typename T > 
Message<T>::Message(uint64_t id_, uint64_t timestamp_, uint16_t type_, const T& data_)
: id(id_)
, timestamp(timestamp_)
, type(type_)
, data(data_)
{
}

template< typename T > 
Message<T>::Message(uint64_t id_, uint64_t timestamp_, uint16_t type_, T &&data_)
: id(id_)
, timestamp(timestamp_)
, type(type_)
, data(std::move(data_))
{
}

template< typename T > 
void Message<T>::Set(uint64_t id_, uint64_t timestamp_, uint16_t type_, const T &data_)
{
  id = id_;
  timestamp = timestamp_;
  type = type_;
  data = data_;
}

template< typename T > 
void Message<T>::Set(uint64_t id_, uint64_t timestamp_, uint16_t type_,  T &&data_)
{
  id = id_;
  timestamp = timestamp_;
  type = type_;
  data = data_;
}

}// namespace rf
