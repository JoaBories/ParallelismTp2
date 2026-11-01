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
  // Les jointures se font hors de la section critique du mutex de la file.
  for (auto& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

void ThreadPool::close() {
  // TODO: sous mutex, closed_ = true ; puis notify_all().
  mutex_.lock();
  closed_ = true;
  mutex_.unlock();
  available_.notify_all();
}

void ThreadPool::worker_loop() {
  for (;;)
  {
    std::unique_lock<std::mutex> lock(mutex_);
    available_.wait(lock, [this] -> bool { return closed_ || !tasks_.empty(); });
    if (closed_ && tasks_.empty())
    {
      lock.unlock();
      return;
    }
    else
    {
      auto task = tasks_.front();
      tasks_.pop();
      lock.unlock();
      task();
    }
  }
  // TODO: boucle
  //   unique_lock + wait(prédicat: closed_ || !tasks_.empty())
  //   si closed_ && tasks_.empty() → return
  //   sinon extraire une tâche, unlock, exécuter hors section critique
  //
  // Tant que cette boucle n'est pas écrite, les workers sortent immédiatement
  // (le destructeur pourra joindre, mais aucune tâche ne s'exécute).
}
