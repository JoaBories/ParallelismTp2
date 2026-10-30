#include "task_queue.hpp"

void TaskQueue::push(std::function<void()> task) {
  // TODO: sous lock_guard, pousser la tâche.
  // TODO: notify_one (idéalement après avoir libéré le mutex).
  (void)task;
}

bool TaskQueue::pop(std::function<void()>& task) {
  // TODO: unique_lock + available_.wait(lock, prédicat stop_ || !tasks_.empty()).
  // TODO: si stop_ && tasks_.empty(), retourner false.
  // TODO: sinon déplacer la tête dans task, pop, retourner true.
  (void)task;
  return false;
}

void TaskQueue::request_stop() {
  // TODO: sous mutex, stop_ = true ; puis notify_all.
}
