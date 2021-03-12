#pragma once
#include <cstddef> //size_t
#include <memory>
/*! \class IMessagePublisher Паттерн Observer
\brief  Интерфейс оповещения видеоинформацией от одного канала видео  всех подписчиков

*/
namespace rf
{
struct IMessage;
class IMessageObserver
{
public:
  
  virtual ~IMessageObserver()=default;
  virtual void Update(const std::shared_ptr<IMessage>)=0;
};
};
