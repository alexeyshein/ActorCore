#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace rf
{
    enum class ModeQueueFull : int
    {
        Nothing = 0, // Drop incoming item if queue is full
        PopOld = 1  // Drop oldest item if queue is full
    };

    template <typename T>
    class SharedQueue
    {
    public:
        SharedQueue()
            : maxSize(0)
            , modeFull(ModeQueueFull::Nothing)
        {}

        explicit SharedQueue(size_t maxSize_, ModeQueueFull mode = ModeQueueFull::PopOld)
            : maxSize(maxSize_)
            , modeFull(mode)
        {}

        ~SharedQueue() = default;

        // Disable copying (prevents mutex/CV copy issues)
        SharedQueue(const SharedQueue&) = delete;
        SharedQueue& operator=(const SharedQueue&) = delete;

        // ===================================================================
        // 1. BLOCKING POP/PEEK OPERATIONS (Legacy API compatible, no dangling refs)
        // ===================================================================

        /// Waits for an item and returns a COPY (safe from Use-After-Free).
        T front() const
        {
            std::unique_lock<std::mutex> lk(mutex_);
            cond_.wait(lk, [this] { return !queue_.empty(); });
            return queue_.front();
        }

        /// Waits for an item with a timeout. Throws std::runtime_error on timeout (legacy compatible).
        template <class Rep, class Period>
        T front(const std::chrono::duration<Rep, Period>& timeout) const
        {
            std::unique_lock<std::mutex> lk(mutex_);
            if (!cond_.wait_for(lk, timeout, [this] { return !queue_.empty(); }))
                throw std::runtime_error("Timeout");
            return queue_.front();
        }

        /// Waits for an item, pops and returns it by value.
        /// If the caller ignores the return value (legacy void-style), it continues to compile.
        T pop_front()
        {
            T item;
            {
                std::unique_lock<std::mutex> lk(mutex_);
                cond_.wait(lk, [this] { return !queue_.empty(); });
                item = std::move(queue_.front());
                queue_.pop_front();
            }
            return item;
        }

        /// Pops the front item into the provided output reference.
        void pop_front(T& outValue)
        {
            std::unique_lock<std::mutex> lk(mutex_);
            cond_.wait(lk, [this] { return !queue_.empty(); });
            outValue = std::move(queue_.front());
            queue_.pop_front();
        }

        /// Pops the front item into the provided output reference with a timeout. Returns false on timeout.
        template <class Rep, class Period>
        bool pop_front(T& outValue, const std::chrono::duration<Rep, Period>& timeout)
        {
            std::unique_lock<std::mutex> lk(mutex_);
            if (!cond_.wait_for(lk, timeout, [this] { return !queue_.empty(); }))
                return false;

            outValue = std::move(queue_.front());
            queue_.pop_front();
            return true;
        }

        // ===================================================================
        // 2. NON-BLOCKING POP/PEEK OPERATIONS (C++17 std::optional & Output Ref)
        // ===================================================================

        bool try_pop_front(T& out)
        {
            std::lock_guard<std::mutex> lk(mutex_);
            if (queue_.empty()) return false;
            out = std::move(queue_.front());
            queue_.pop_front();
            return true;
        }

        std::optional<T> try_pop_front()
        {
            std::lock_guard<std::mutex> lk(mutex_);
            if (queue_.empty()) return std::nullopt;
            std::optional<T> item(std::move(queue_.front()));
            queue_.pop_front();
            return item;
        }

        std::optional<T> try_front() const
        {
            std::lock_guard<std::mutex> lk(mutex_);
            if (queue_.empty()) return std::nullopt;
            return queue_.front();
        }

        // ===================================================================
        // 3. ATOMIC PREDICATE POP OPERATIONS (For SanitizeQueue / std::future)
        // ===================================================================

        /// Inspects the queue head with a predicate UNDER LOCK.
        /// If the predicate returns true, pops and returns the item as std::optional.
        template <typename Predicate>
        std::optional<T> pop_front_if(Predicate pred)
        {
            std::lock_guard<std::mutex> lk(mutex_);
            if (queue_.empty()) return std::nullopt;

            if (pred(queue_.front()))
            {
                std::optional<T> item(std::move(queue_.front()));
                queue_.pop_front();
                return item;
            }
            return std::nullopt;
        }

        /// Version with output parameter passed by reference.
        template <typename Predicate>
        bool pop_front_if(Predicate pred, T& outValue)
        {
            std::lock_guard<std::mutex> lk(mutex_);
            if (queue_.empty()) return false;

            if (pred(queue_.front()))
            {
                outValue = std::move(queue_.front());
                queue_.pop_front();
                return true;
            }
            return false;
        }

        // ===================================================================
// TAIL (BACK) OPERATIONS — SAFE FROM USE-AFTER-FREE
// ===================================================================

/// Waits for an item and returns a COPY of the last item in the queue.
        T back() const
        {
            std::unique_lock<std::mutex> lk(mutex_);
            cond_.wait(lk, [this] { return !queue_.empty(); });
            return queue_.back();
        }

        /// Waits for an item with a timeout and returns a COPY of the last item.
        /// Throws std::runtime_error on timeout (legacy API compatible).
        template <class Rep, class Period>
        T back(const std::chrono::duration<Rep, Period>& timeoutDuration) const
        {
            std::unique_lock<std::mutex> lk(mutex_);
            if (!cond_.wait_for(lk, timeoutDuration, [this] { return !queue_.empty(); }))
                throw std::runtime_error("Timeout");
            return queue_.back();
        }

        /// Non-blocking peek at the back item (C++17 std::optional).
        std::optional<T> try_back() const
        {
            std::lock_guard<std::mutex> lk(mutex_);
            if (queue_.empty()) return std::nullopt;
            return queue_.back();
        }

        // ===================================================================
        // TAIL POP OPERATIONS (LIFO / Stack-like extraction)
        // ===================================================================

        /// Waits for an item, pops and returns the LAST item (tail) by value.
        T pop_back()
        {
            T item;
            {
                std::unique_lock<std::mutex> lk(mutex_);
                cond_.wait(lk, [this] { return !queue_.empty(); });
                item = std::move(queue_.back());
                queue_.pop_back();
            }
            return item;
        }

        /// Non-blocking pop from tail into output reference. Returns true if popped.
        bool try_pop_back(T& out)
        {
            std::lock_guard<std::mutex> lk(mutex_);
            if (queue_.empty()) return false;
            out = std::move(queue_.back());
            queue_.pop_back();
            return true;
        }

        /// Non-blocking pop from tail (C++17 std::optional).
        std::optional<T> try_pop_back()
        {
            std::lock_guard<std::mutex> lk(mutex_);
            if (queue_.empty()) return std::nullopt;
            std::optional<T> item(std::move(queue_.back()));
            queue_.pop_back();
            return item;
        }

        // ===================================================================
        // 4. BULK & COMPOSITE OPERATIONS (Deadlock-free destruction)
        // ===================================================================

        /// Atomically drains the entire queue. Items are destroyed outside the mutex lock!
        std::deque<T> drain()
        {
            std::deque<T> out;
            {
                std::lock_guard<std::mutex> lk(mutex_);
                out.swap(queue_);
            }
            return out; // Items are destroyed when out goes out of scope (outside the lock)
        }

        /// Executes a user-provided callable directly under the queue lock.
        template <class F>
        auto with_lock(F&& f) -> decltype(f(std::declval<std::deque<T>&>()))
        {
            std::lock_guard<std::mutex> lk(mutex_);
            return f(queue_);
        }

        /// Clears the queue. Items are destroyed outside the mutex lock!
        void clear()
        {
            std::deque<T> emptyQueue;
            {
                std::lock_guard<std::mutex> lk(mutex_);
                emptyQueue.swap(queue_);
            }
        }

        // ===================================================================
        // 5. PUSH OPERATIONS (Return bool for accurate telemetry/drop tracking)
        // ===================================================================

        /// Returns true if the item was added, false if dropped (when ModeQueueFull::Nothing).
        bool push_back(const T& item)
        {
            T droppedItem;
            bool hasDropped = false;
            {
                std::lock_guard<std::mutex> lk(mutex_);
                if (!MakeRoomUnlocked(hasDropped, droppedItem))
                    return false;

                queue_.push_back(item);
            }
            cond_.notify_one();
            return true; // droppedItem is destroyed outside the mutex lock
        }

        bool push_back(T&& item)
        {
            T droppedItem;
            bool hasDropped = false;
            {
                std::lock_guard<std::mutex> lk(mutex_);
                if (!MakeRoomUnlocked(hasDropped, droppedItem))
                    return false;

                queue_.push_back(std::move(item));
            }
            cond_.notify_one();
            return true;
        }

        template <class... Args>
        bool emplace_back(Args&&... args)
        {
            T droppedItem;
            bool hasDropped = false;
            {
                std::lock_guard<std::mutex> lk(mutex_);
                if (!MakeRoomUnlocked(hasDropped, droppedItem))
                    return false;

                queue_.emplace_back(std::forward<Args>(args)...);
            }
            cond_.notify_one();
            return true;
        }

        // ===================================================================
        // 6. STATE & CONFIGURATION (Const & Thread-Safe)
        // ===================================================================

        void setMaxSize(size_t maxSize_)
        {
            std::deque<T> trimmedItems;
            {
                std::lock_guard<std::mutex> lk(mutex_);
                maxSize = maxSize_;
                TrimOverflowUnlocked(trimmedItems);
            }
        }

        size_t getMaxSize() const
        {
            std::lock_guard<std::mutex> lk(mutex_);
            return maxSize;
        }

        void setModeFull(ModeQueueFull mode)
        {
            std::lock_guard<std::mutex> lk(mutex_);
            modeFull = mode;
        }

        ModeQueueFull getModeFull() const
        {
            std::lock_guard<std::mutex> lk(mutex_);
            return modeFull;
        }

        bool isFull() const
        {
            std::lock_guard<std::mutex> lk(mutex_);
            return maxSize > 0 && queue_.size() >= maxSize;
        }

        size_t size() const
        {
            std::lock_guard<std::mutex> lk(mutex_);
            return queue_.size();
        }

        bool empty() const
        {
            std::lock_guard<std::mutex> lk(mutex_);
            return queue_.empty();
        }

    private:
        /// Checks and makes room in the queue if full. Must be called UNDER lock.
        /// If an old item is displaced (PopOld), it is moved to outDropped for destruction outside the lock.
        bool MakeRoomUnlocked(bool& outHasDropped, T& outDropped)
        {
            if (maxSize == 0 || queue_.size() < maxSize)
                return true; // Room available

            switch (modeFull)
            {
            case ModeQueueFull::PopOld:
                if (!queue_.empty() && queue_.size() >= maxSize)
                {
                    outDropped = std::move(queue_.front());
                    queue_.pop_front();
                    outHasDropped = true;
                }
                return true;

            case ModeQueueFull::Nothing:
            default:
                return false; // Drop incoming item
            }
        }

        /// Trims the queue if setMaxSize reduces capacity below the current size.
        void TrimOverflowUnlocked(std::deque<T>& outTrimmed)
        {
            if (maxSize > 0 && modeFull == ModeQueueFull::PopOld)
            {
                while (queue_.size() > maxSize && !queue_.empty())
                {
                    outTrimmed.push_back(std::move(queue_.front()));
                    queue_.pop_front();
                }
            }
        }

    private:
        std::deque<T> queue_;
        mutable std::mutex mutex_;
        mutable std::condition_variable cond_;
        size_t maxSize;
        ModeQueueFull modeFull;
    };
}