#pragma once
#include <cstddef> //size_t
#include <memory>
/*! \class IDataPublisher Паттерн Observer
\brief  Интерфейс оповещения видеоинформацией от одного канала видео  всех подписчиков

*/
namespace rf
{
struct IData;
class IDataObserver
{
public:
  
  virtual ~IDataObserver()=default;
  virtual void Update(const std::shared_ptr<IData>)=0;
};
};
