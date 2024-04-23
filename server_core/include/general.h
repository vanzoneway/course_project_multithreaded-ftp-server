#pragma once

#include <cstdint>
#include <cstring>
#include <cinttypes>
#include <malloc.h>

#include <queue>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>


/// Simple thread pool implementation
class ThreadPool {
    std::vector<std::thread> thread_pool;
    std::queue<std::function<void()>> job_queue;
    std::mutex queue_mtx;
    std::condition_variable condition;
    std::atomic<bool> pool_terminated = false;

    void setupThreadPool(uint thread_count) {
        thread_pool.clear();
        for(uint i = 0; i < thread_count; ++i)
            thread_pool.emplace_back(&ThreadPool::workerLoop, this);
    }

    void workerLoop() {
        std::function<void()> job;
        while (!pool_terminated) {
            {
                std::unique_lock lock(queue_mtx);
                condition.wait(lock, [this](){return !job_queue.empty() || pool_terminated;});
                if(pool_terminated) return;
                job = job_queue.front();
                job_queue.pop();
            }
            job();
        }
    }
public:
    explicit ThreadPool(uint thread_count = 1) {setupThreadPool(thread_count);}

    ~ThreadPool() {
        pool_terminated = true;
        join();
    }

    template<typename F>
    void addJob(F job) {
        if(pool_terminated) return;
        {
            std::unique_lock lock(queue_mtx);
            job_queue.push(std::function<void()>(job));
        }
        condition.notify_one();
    }

    template<typename F, typename... Arg>
    void addJob(const F& job, const Arg&... args) {addJob([job, args...]{job(args...);});}

    void join() {for(auto& thread : thread_pool) thread.join();}

    uint getThreadCount() const {return thread_pool.size();}

    void dropUnstartedJobs() {
        pool_terminated = true;
        join();
        pool_terminated = false;
        // Clear jobs in queue
        std::queue<std::function<void()>> empty;
        std::swap(job_queue, empty);
        // reset thread pool
        setupThreadPool(thread_pool.size());
    }

    void stop() {
        pool_terminated = true;
        join();
    }

    void add_thread() {
        thread_pool.emplace_back(&ThreadPool::workerLoop, this);
    }

    void start(uint thread_count = std::thread::hardware_concurrency()) {
        if(!pool_terminated) return;
        pool_terminated = false;
        setupThreadPool(thread_count);
    }

};

typedef std::vector<uint8_t> DataBuffer;

enum class SocketType : uint8_t {
    client_socket = 0,
    server_socket = 1
};
