#include "MessagePublisher.h"

#include <iostream>
#include <algorithm>
#include <future>
#include "IMessageObserver.h"

using rf::MessagePublisher;
using rf::IMessageObserver;

MessagePublisher::MessagePublisher() : myFutureQueue(255, ModeQueueFull::Nothing)
{
	//myFutureQueue.setModeFull(ModeQueueFull::Nothing);
}

MessagePublisher::~MessagePublisher()
{
}

//Подключение наблюдателя
void MessagePublisher::Attach(IMessageObserver *subscriberCandidate)
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
void MessagePublisher::Detach(IMessageObserver *subscriberCandidate)
{
	// Удаляем все ссылки на processingChannel
	std::lock_guard<std::mutex> mlock(mutex_);
	subscribers.erase(std::remove(subscribers.begin(),
								  subscribers.end(), subscriberCandidate),
					  subscribers.end());
}

//Оповещение наблюдателя
void MessagePublisher::Notify(const std::shared_ptr<IMessage> framePtr)
{
	std::lock_guard<std::mutex> mlock(mutex_);
	SanitizeQueue();
	for (auto subscriber : subscribers)
	{
		//Если  myFutureQueue заполняется до MaxQueue, то выполнение future не помещается в очередь, а ожидается до выхода из данной функции
		myFutureQueue.emplace_back(std::async(
			std::launch::async, [](IMessageObserver *subscriber, const std::shared_ptr<IMessage> framePtr) { subscriber->Update(framePtr); },
			subscriber, framePtr));
	}
}

size_t MessagePublisher::NumObservers()
{
	//std::lock_guard<std::mutex> mlock(mutex_);
	return subscribers.size();
}

void MessagePublisher::CleanObservers()
{
	std::lock_guard<std::mutex> mlock(mutex_);
	subscribers.clear();
}

void MessagePublisher::SanitizeQueue()
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