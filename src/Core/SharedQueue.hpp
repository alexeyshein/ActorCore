#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

namespace rf
{
    enum class ModeQueueFull:int
    {
        Nothing = 0,
        PopOld = 1
    };

    template <typename T>
    class SharedQueue
    {
    public:
        SharedQueue();
        SharedQueue(size_t maxSize, ModeQueueFull mode=ModeQueueFull::PopOld);
        ~SharedQueue();

        T& front();

        template< class Rep, class Period >
        T& front(const std::chrono::duration<Rep,Period>& timeout_duration);
        
        void pop_front();

        void push_back(const T& item);
        void push_back(T&& item);
        void emplace_back(T&& item);
        void setMaxSize(size_t maxSize);
        size_t getMaxSize(){return maxSize;}
        void setModeFull(ModeQueueFull);
        ModeQueueFull getModeFull(){return modeFull;}
        void clear();

        int  size();
        bool empty();

    protected:
      void WhenFull();

    protected:
        std::deque<T> queue_;
        std::mutex mutex_;
        std::condition_variable cond_;
        size_t maxSize;
        ModeQueueFull modeFull;
    }; 

    template <typename T>
    SharedQueue<T>::SharedQueue():
    maxSize(0),
    modeFull(ModeQueueFull::Nothing){}

    template <typename T>
    SharedQueue<T>::SharedQueue(size_t maxSize_, ModeQueueFull mode):
    maxSize(maxSize_)
    , modeFull(mode){}

    template <typename T>
    SharedQueue<T>::~SharedQueue(){}

    template <typename T>
    T& SharedQueue<T>::front()
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty())
        {
            cond_.wait(mlock);
        }
        return queue_.front();
    }

    template< typename T> 
    template< class Rep, class Period >
    T& SharedQueue<T>::front(const std::chrono::duration<Rep,Period>& timeout_duration)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        if(queue_.empty())
        {
            if(cond_.wait_for(mlock, timeout_duration)==std::cv_status::timeout)
                throw std::runtime_error("Timeout");
               //return queue_.front();//TODO undefined behavior
        }
        return queue_.front();
    }

    template <typename T>
    void SharedQueue<T>::pop_front()
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty())
        {
            cond_.wait(mlock);
        }
        queue_.pop_front();
        return;
    }    

    template <typename T>
    void SharedQueue<T>::push_back(const T& item)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        if((maxSize > 0) && (maxSize <= queue_.size()))
        {
            WhenFull();
            if (modeFull==ModeQueueFull::Nothing)
                return;
        }
          
        queue_.push_back(item);
        mlock.unlock();     // unlock before notificiation to minimize mutex con
        cond_.notify_one(); // notify one waiting thread
        return;
    }

    template <typename T>
    void SharedQueue<T>::push_back(T&& item)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        if((maxSize > 0) && (maxSize < queue_.size()))
        {
           WhenFull();
            if (modeFull==ModeQueueFull::Nothing)
                return;
        }
        queue_.push_back(std::move(item));
        mlock.unlock();     // unlock before notificiation to minimize mutex con
        cond_.notify_one(); // notify one waiting thread
        return;
    }

    template <typename T>
    void SharedQueue<T>::emplace_back(T&& item)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        if((maxSize > 0) && (maxSize < queue_.size()))
        {
           WhenFull();
            if (modeFull==ModeQueueFull::Nothing)
                return;
        }
        queue_.emplace_back(std::move(item));
        mlock.unlock();     // unlock before notificiation to minimize mutex con
        cond_.notify_one(); // notify one waiting thread
        return; 
    }

    template <typename T> 
    void SharedQueue<T>::setMaxSize(size_t maxSize_)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        maxSize=maxSize_;
    }

    template <typename T>
    void SharedQueue<T>::clear()
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        std::deque<T> empty;
        std::swap( queue_, empty );
        return;
    }


    template <typename T>
    int SharedQueue<T>::size()
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        int size = queue_.size();
        return size;
    } 
    
    template <typename T>
    bool SharedQueue<T>::empty()  
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        return queue_.empty();
    }

    template <typename T>
    void SharedQueue<T>::setModeFull(ModeQueueFull mode)
    {
        modeFull = mode;
    }

    template <typename T>
    void SharedQueue<T>::WhenFull()
    { 
      switch(modeFull)
      {
          case ModeQueueFull::Nothing : break;
          case ModeQueueFull::PopOld : 
                        {
                        while((queue_.size() >= maxSize ) && (queue_.size()>0))
                            {
                               queue_.pop_front();
                            }
                            break;
                        }
        default: break;
      }

    }

}