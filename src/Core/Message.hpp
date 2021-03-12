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
    Message(uint64_t id, uint64_t timestamp, const T&  data);
    Message(uint64_t id, uint64_t timestamp, const T&& data);
    ~Message(){};
    void Set(uint64_t id, uint64_t timestamp, const T&  data);
    void Set(uint64_t id, uint64_t timestamp, const T&& data);
    uint64_t Id() { return id; }
    uint64_t Timestamp() const { return timestamp; }
    T &DataRef() { return data; }
    T GetData() { return data; }

protected:
private:
    static const char* typeName;//="Message";
    uint64_t id;
    uint64_t timestamp;
    //std::string typeName;
    T data;
};

//std::string Message::TypeName { return std::string(typeName);}



template< typename T  > 
Message<T>::Message(uint64_t id_, uint64_t timestamp_)
: id(id_)
, timestamp(timestamp_)
{
}

template< typename T > 
Message<T>::Message(uint64_t id_, uint64_t timestamp_, const T& data_)
: id(id_)
, timestamp(timestamp_)
, data(data_)
{
}

template< typename T > 
Message<T>::Message(uint64_t id_, uint64_t timestamp_, const T &&data_)
: id(id_)
, timestamp(timestamp_)
, data(std::move(data_))
{
}

template< typename T > 
void Message<T>::Set(uint64_t id_, uint64_t timestamp_, const T &data_)
{
  id = id_
  timestamp = timestamp_;
  data = data_;
}

template< typename T > 
void Message<T>::Set(uint64_t id_, uint64_t timestamp_, const T &&data_)
{
  id = id_
  timestamp = timestamp_;
  data = data_;
}

}// namespace rf
