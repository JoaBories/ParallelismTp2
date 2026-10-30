#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

class ThreadPool {
 public:
  explicit ThreadPool(std::size_t worker_count);
  ~ThreadPool();

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  template <class F>
  auto submit(F&& f) -> std::future<std::invoke_result_t<F>>;

  void close();

 private:
  void worker_loop();

  std::mutex mutex_;
  std::condition_variable available_;
  std::queue<std::function<void()>> tasks_;
  std::vector<std::thread> workers_;
  bool closed_ = false;
};

template <class F>
auto ThreadPool::submit(F&& f) -> std::future<std::invoke_result_t<F>> {
  using R = std::invoke_result_t<F>;

  // Adaptateur fourni : packaged_task n'est pas copiable, std::function exige
  // CopyConstructible → on la garde dans un shared_ptr capturé par la tâche enveloppe.
  auto task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(f));
  std::future<R> future = task->get_future();

  // TODO:
  // 1. sous mutex : si closed_, lever std::runtime_error("pool closed")
  // 2. pousser une tâche [task]{ (*task)(); } dans tasks_
  // 3. notify_one (après le unlock)
  // 4. retourner future
  (void)task;
  (void)future;
  throw std::runtime_error("TODO: implémenter ThreadPool::submit");
}
