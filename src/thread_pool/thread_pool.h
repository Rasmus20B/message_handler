
#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <future>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ThreadPool {
public:
    explicit ThreadPool(size_t thread_count)
        : shutdown_flag(false)
    {
        for (size_t i = 0; i < thread_count; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    Task task;
                    {
                        std::unique_lock lock(queue_mutex);
                        condition.wait(lock, [this] {
                            return shutdown_flag || !tasks.empty();
                        });
                        if (shutdown_flag && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard lock(queue_mutex);
            shutdown_flag = true;
        }
        condition.notify_all();
        for (auto& worker : workers) {
            if (worker.joinable()) worker.join();
        }
    }

    template<typename Func>
    auto submit(Func&& f) -> std::future<decltype(f())> {
        using ReturnType = decltype(f());
        auto task_ptr = std::make_shared<std::packaged_task<ReturnType()>>(std::forward<Func>(f));
        {
            std::lock_guard lock(queue_mutex);
            tasks.emplace([task_ptr]() { (*task_ptr)(); });
        }
        condition.notify_one();
        return task_ptr->get_future();
    }

private:
    using Task = std::function<void()>;
    std::vector<std::thread> workers;
    std::queue<Task> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> shutdown_flag;
};
