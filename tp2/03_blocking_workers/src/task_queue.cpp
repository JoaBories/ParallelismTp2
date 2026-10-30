#include "task_queue.hpp"

void TaskQueue::push(std::function<void()> task) {
  // TODO: sous lock_guard, pousser la tâche.
  // TODO: notify_one (idéalement après avoir libéré le mutex).
  {
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.push(task);
  }
  available_.notify_one();
}

bool TaskQueue::pop(std::function<void()>& task) {
  // TODO: unique_lock + available_.wait(lock, prédicat stop_ || !tasks_.empty()).
  // TODO: si stop_ && tasks_.empty(), retourner false.
  // TODO: sinon déplacer la tête dans task, pop, retourner true.
  std::unique_lock<std::mutex> lock(mutex_);
  available_.wait(lock, [this]() -> bool { return stop_ || !tasks_.empty(); });
  if (stop_ && tasks_.empty())
  {
    lock.unlock();
    return false;
  }
  else
  {
    task = tasks_.front();
    tasks_.pop();
    lock.unlock();
    return true;
  }
}

void TaskQueue::request_stop() {
  // TODO: sous mutex, stop_ = true ; puis notify_all.
  mutex_.lock();
  stop_ = true;
  mutex_.unlock();
  available_.notify_all();
}
