#pragma once
#include "IDataPublisher.h"
#include "SharedQueue.hpp"
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
struct IData;

class DataPublisher :
	public IDataPublisher
{
public:
	DataPublisher();
	~DataPublisher() override;
	void Attach(IDataObserver* ) override;
	void Detach(IDataObserver* ) override;
	void Notify(const std::shared_ptr<IData>) override;
	size_t NumObservers() override;
	void CleanObservers() override;
protected:
    void SanitizeQueue();
protected:
    //TODO защитить вектор мъютексами для одновременного использования из разных потоков
	//Список наблюдателей
	std::vector< IDataObserver*> subscribers;
	SharedQueue<std::future<void>> myFutureQueue;
	std::mutex mutex_; //Защита от изменения количества подписчиков
};
};

