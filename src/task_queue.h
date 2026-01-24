#ifndef Z8_TASK_QUEUE_H
#define Z8_TASK_QUEUE_H

#include "v8.h"
#include <queue>
#include <mutex>
#include <functional>

namespace z8 {

struct Task {
    v8::Global<v8::Function> callback;
    v8::Global<v8::Promise::Resolver> resolver;
    bool is_promise;
    std::function<void(v8::Isolate*, v8::Local<v8::Context>, Task*)> runner;
    
    // Data (managed by the specific task)
    void* data;
    int error_code;
};

class TaskQueue {
public:
    static TaskQueue& GetInstance() {
        static TaskQueue instance;
        return instance;
    }

    void Enqueue(Task* task) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(task);
    }

    Task* Dequeue() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty()) return nullptr;
        Task* task = queue_.front();
        queue_.pop();
        return task;
    }

    bool IsEmpty() {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.empty();
    }

private:
    std::queue<Task*> queue_;
    std::mutex mutex_;
};

} // namespace z8

#endif
