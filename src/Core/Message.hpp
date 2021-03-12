#pragma once

#include <string>
#include "IMessage.h"

namespace rf
{

template< typename T, const char*  typeIdStr > 
class Message : public IMessage
{
public:
    Message(uint64_t id = 0, uint64_t timestamp = 0);
    Message(uint64_t id, uint64_t timestamp, const T&  data);
    Message(uint64_t id, uint64_t timestamp, const T&& data);
    ~Message(){};
    void Set(uint64_t id, uint64_t timestamp, const T&  data);
    void Set(uint64_t id, uint64_t timestamp, const T&& data);
    uint64_t Id() { return id; }
    uint64_t Timestamp() const { return timestamp; }
    T &DataRef() { return Message; }
    T GetData() { return Message; }

protected:
private:
    uint64_t id;
    uint64_t timestamp;
    std::string typeId;
    T data;
};

template< typename T, const char*  typeIdStr  > 
Message<T,typeIdStr>::Message(uint64_t id_ = 0, uint64_t timestamp_ = 0)
: id(id_)
, timestamp(timestamp_)
, typeId(typeIdStr)
{
}

template< typename T, const char*  typeIdStr  > 
Message<T,typeIdStr>::Message(uint64_t id_, uint64_t timestamp_, const T& profileQue_)
: id(id_)
, timestamp(timestamp_)
, profileQue(profileQue_)
, typeId(typeIdStr)
{
}

template< typename T, const char*  typeIdStr  > 
Message<T,typeIdStr>::Message(uint64_t id_, uint64_t timestamp_, const T &&profileQue_)
: id(id_)
, timestamp(timestamp_)
, profileQue(std::move(profileQue_))
, typeId(typeIdStr)
{
}

template< typename T, const char*  typeIdStr  > 
void Message<T,typeIdStr>::Set(uint64_t id_, uint64_t timestamp_, const T &profileQue_)
{
  id = id_
  timestamp = timestamp_;
  profileQue = profileQue_;
}

template< typename T, const char*  typeIdStr  > 
void Message<T,typeIdStr>::Set(uint64_t id_, uint64_t timestamp_, const T &&profileQue_)
{
  id = id_
  timestamp = timestamp_;
  profileQue = profileQue_;
}

}// namespace rf
