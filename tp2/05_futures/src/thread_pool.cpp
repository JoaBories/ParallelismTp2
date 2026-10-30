#include "thread_pool.hpp"

ThreadPool::ThreadPool(std::size_t worker_count) {
  if (worker_count == 0) {
    throw std::invalid_argument("worker_count must be > 0");
  }
  workers_.reserve(worker_count);
  for (std::size_t i = 0; i < worker_count; ++i) {
    workers_.emplace_back([this] { worker_loop(); });
  }
}

ThreadPool::~ThreadPool() {
  close();
  for (auto& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

void ThreadPool::close() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    closed_ = true;
  }
  available_.notify_all();
}

void ThreadPool::worker_loop() {
  for (;;) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      available_.wait(lock, [&] { return closed_ || !tasks_.empty(); });
      if (closed_ && tasks_.empty()) {
        return;
      }
      task = std::move(tasks_.front());
      tasks_.pop();
    }
    task();
  }
}
