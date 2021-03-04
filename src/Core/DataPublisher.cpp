#include "DataPublisher.h"

#include <iostream>
#include <algorithm>
#include <future>
#include "IDataObserver.h"

using rf::DataPublisher;
using rf::IDataObserver;

DataPublisher::DataPublisher() : myFutureQueue(255, ModeQueueFull::Nothing)
{
	//myFutureQueue.setModeFull(ModeQueueFull::Nothing);
}

DataPublisher::~DataPublisher()
{
}

//Подключение наблюдателя
void DataPublisher::Attach(IDataObserver *subscriberCandidate)
{
	//Поиск, есть ли этот наблюдатель в списке
	std::lock_guard<std::mutex> mlock(mutex_);
	bool isPresent = false;
	for (auto subscriber : subscribers)
	{
		if (subscriber == subscriberCandidate)
			isPresent = true;
	}

	//Если такого наблюдателя нет в спике, то добавляем
	if (!isPresent)
		subscribers.push_back(subscriberCandidate);
}

//Отключение наблюдателя
void DataPublisher::Detach(IDataObserver *subscriberCandidate)
{
	// Удаляем все ссылки на processingChannel
	std::lock_guard<std::mutex> mlock(mutex_);
	subscribers.erase(std::remove(subscribers.begin(),
								  subscribers.end(), subscriberCandidate),
					  subscribers.end());
}

//Оповещение наблюдателя
void DataPublisher::Notify(const std::shared_ptr<IData> framePtr)
{
	std::lock_guard<std::mutex> mlock(mutex_);
	SanitizeQueue();
	for (auto subscriber : subscribers)
	{
		//Если  myFutureQueue заполняется до MaxQueue, то выполнение future не помещается в очередь, а ожидается до выхода из данной функции
		myFutureQueue.emplace_back(std::async(
			std::launch::async, [](IDataObserver *subscriber, const std::shared_ptr<IData> framePtr) { subscriber->Update(framePtr); },
			subscriber, framePtr));
	}
}

size_t DataPublisher::NumObservers()
{
	//std::lock_guard<std::mutex> mlock(mutex_);
	return subscribers.size();
}

void DataPublisher::CleanObservers()
{
	std::lock_guard<std::mutex> mlock(mutex_);
	subscribers.clear();
}

void DataPublisher::SanitizeQueue()
{
	bool ready = true;
	while (ready)
	{
		ready = false;
		if (!myFutureQueue.empty())
		{
			std::future<void> &front = myFutureQueue.front();
			std::future_status status = front.wait_for(std::chrono::nanoseconds(40));
			if (status == std::future_status::ready)
			 {
			 	myFutureQueue.pop_front();
			 	ready = true;
			 }
		}
	}
}