#pragma once

#include "LockFreeQueue.h"

#include <vector>
#include <thread>
#include <future>
#include <atomic>
#include <functional>

template <typename T=std::packaged_task<void(void)>>
class ThreadPool {
 public:
    ThreadPool(size_t threads_number, size_t queue_size);

    ThreadPool(const ThreadPool& rhs) = delete;
    ThreadPool& operator=(const ThreadPool& rhs) = delete;

    void push(T&& task);

    ~ThreadPool();

 private:
    void execute();

 private:
    NotLockFreeQueue<T> queue;
    std::atomic<bool> is_working;
    std::vector<std::thread> threads;
};

template <typename T>
ThreadPool<T>::ThreadPool(size_t threads_number, size_t queue_size) : 
            queue(queue_size), is_working(true) {
    for (size_t i = 0; i < threads_number; i++) {
        threads.push_back(std::thread(std::bind(&ThreadPool::execute, this)));
    }
}

template <typename T>
void ThreadPool<T>::push(T&& task) {
    queue.push_and_wait(std::move(task));
}

template <typename T>
ThreadPool<T>::~ThreadPool() {
    is_working = false;
    for (auto& thread : threads) {
        thread.join();
    }
}

template <typename T>
void ThreadPool<T>::execute() {
    while (is_working) {
        T task;
        queue.pop_and_wait(task);
        task();
    }
}
