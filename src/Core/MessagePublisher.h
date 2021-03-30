#pragma once
#include "IMessagePublisher.h"
#include "SharedQueue.h"
#include <memory>
#include <vector>
#include <future>
#include <mutex>
/*! \class FramePublisher Паттерн Observer
\brief  Класс оповещения видеоинформацией от одного канала видео  всех подписчиков

Класс реализует процедуру оповещения видео кадрами всех подписчиков ,
наследуемых от ProcessingChannel.
Данный класс является источником оповещения при реализации паттерна "Observer"
*/

namespace rf
{
struct IMessage;

class MessagePublisher :
	public IMessagePublisher
{
public:
	MessagePublisher();
	~MessagePublisher() override;
	void Attach(IMessageObserver* ) override;
	void Detach(IMessageObserver* ) override;
	void Notify(const std::shared_ptr<IMessage>) override;
	size_t NumObservers() override;
	void CleanObservers() override;
protected:
    void SanitizeQueue();
protected:
    //TODO защитить вектор мъютексами для одновременного использования из разных потоков
	//Список наблюдателей
	std::vector< IMessageObserver*> subscribers;
	SharedQueue<std::future<void>> myFutureQueue;
	std::mutex mutex_; //Защита от изменения количества подписчиков
};
};

