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

}