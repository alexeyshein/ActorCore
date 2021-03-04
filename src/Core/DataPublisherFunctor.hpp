#pragma once

#include "SharedQueue.hpp"
#include <memory>
#include <functional>
#include <unordered_map>            // std::unordered_map
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
//struct IData;
template<class T>
using Callback = std::function<void(const T&)>;

template<class T>
class DataPublisherFunctor //:	public IDataPublisher
{
public:
	DataPublisherFunctor();
	~DataPublisherFunctor();
	void Attach(std::size_t linkId, Callback<T>);
	void Detach(std::size_t linkId);
	void Notify(const T&);
	size_t NumObservers();
	void CleanObservers();
	void SetAsyncMode(bool async){_isAsync = async;}
	bool IsAsyncMode(){return _isAsync;}
	void SetAsyncQueueSize(size_t size) {_myFutureQueue.setMaxSize(size);}
	size_t AsyncQueueSize() {return _myFutureQueue.getMaxSize();}
protected:
    void SanitizeQueue();
protected:
    //TODO защитить вектор мъютексами для одновременного использования из разных потоков
	//Список наблюдателей
	std::unordered_map<uint64_t, Callback<T>> _mapCallbacks;
	SharedQueue<std::future<void>> _myFutureQueue;
	std::mutex _mutex; //Защита от изменения количества подписчиков
	bool _isAsync;
};



template<class T>
DataPublisherFunctor<T>::DataPublisherFunctor() 
:  _myFutureQueue(255, ModeQueueFull::Nothing)
,  _isAsync(true)
{
	//_myFutureQueue.setModeFull(ModeQueueFull::Nothing);
}

template<class T>
DataPublisherFunctor<T>::~DataPublisherFunctor()
{
}

//Подключение наблюдателя
template<class T>
void DataPublisherFunctor<T>::Attach(std::size_t linkId, Callback<T> subscriberCandidate)
{
	std::lock_guard<std::mutex> mlock(_mutex);
	auto search  = _mapCallbacks.find(linkId);
	if (search  == _mapCallbacks.end()) 
	{
        _mapCallbacks.emplace(linkId, std::move(subscriberCandidate));
	}
}

//Отключение наблюдателя
template<class T>
void DataPublisherFunctor<T>::Detach(std::size_t linkId)
{
	// Удаляем все ссылки на processingChannel
	std::lock_guard<std::mutex> mlock(_mutex);
	_mapCallbacks.erase(linkId);
}

//Оповещение наблюдателя
template<class T>
void DataPublisherFunctor<T>::Notify(const T& data)
{
	std::lock_guard<std::mutex> mlock(_mutex);
	SanitizeQueue();
	for (const auto & [key, subscriberFunc] : _mapCallbacks)
	{
		if (subscriberFunc != nullptr)
		{
		// //Если  _myFutureQueue заполняется до MaxQueue, то выполнение future не помещается в очередь и отбрасывается
		if(_isAsync)
		{
              		_myFutureQueue.emplace_back(std::async(
			std::launch::async, [](const Callback<T> &subscriber, const T& dataSend) { subscriber(dataSend); },
			subscriberFunc, data));
		}else
		{
			 subscriberFunc(data);
		}

		}

	}
}

template<class T>
size_t DataPublisherFunctor<T>::NumObservers()
{
	//std::lock_guard<std::mutex> mlock(_mutex);
	return _mapCallbacks.size();
}

template<class T>
void DataPublisherFunctor<T>::CleanObservers()
{
	std::lock_guard<std::mutex> mlock(_mutex);
	_mapCallbacks.clear();
}

template<class T>
void DataPublisherFunctor<T>::SanitizeQueue()
{
	bool ready = true;
	while (ready)
	{
		ready = false;
		if (!_myFutureQueue.empty())
		{
			std::future<void> &front = _myFutureQueue.front();
			std::future_status status = front.wait_for(std::chrono::nanoseconds(40));
			if (status == std::future_status::ready)
			 {
			 	_myFutureQueue.pop_front();
			 	ready = true;
			 }
		}
	}
}

}; //namespace rf