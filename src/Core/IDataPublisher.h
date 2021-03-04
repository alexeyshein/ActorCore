#pragma once
#include <cstddef> //size_t
#include <memory>
/*! \class IDataPublisher Паттерн Observer
\brief  Интерфейс оповещения видеоинформацией от одного канала видео  всех подписчиков

*/
namespace rf
{
class IDataObserver;

struct IData;

class IDataPublisher
{
public:
    virtual ~IDataPublisher() = default;
// Attach(), Detach(), Notify() для реализации наблюдаемого
	virtual void Attach(IDataObserver* ) = 0;
	virtual void Detach(IDataObserver* ) = 0;
	virtual void Notify(const std::shared_ptr<IData> ) = 0;
	virtual size_t NumObservers()=0;
	virtual void CleanObservers()=0;
};
};