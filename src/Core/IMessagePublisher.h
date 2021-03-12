#pragma once
#include <cstddef> //size_t
#include <memory>
/*! \class IMessagePublisher Паттерн Observer
\brief  Интерфейс оповещения видеоинформацией от одного канала видео  всех подписчиков

*/
namespace rf
{
class IMessageObserver;

struct IMessage;

class IMessagePublisher
{
public:
    virtual ~IMessagePublisher() = default;
// Attach(), Detach(), Notify() для реализации наблюдаемого
	virtual void Attach(IMessageObserver* ) = 0;
	virtual void Detach(IMessageObserver* ) = 0;
	virtual void Notify(const std::shared_ptr<IMessage> ) = 0;
	virtual size_t NumObservers()=0;
	virtual void CleanObservers()=0;
};
};