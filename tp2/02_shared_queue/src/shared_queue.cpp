#include "shared_queue.hpp"

void SharedQueue::push(int id) {
  std::lock_guard<std::mutex> lock(mutex_);
  tasks_.push(id);
}

bool SharedQueue::try_pop(int& id) {
  // TODO: sous mutex_, retirer la tête dans id.
  //       File vide : retourner false.
  mutex_.lock();
  if (tasks_.empty())
  {
    mutex_.unlock();
    return false;
  }
  else
  {
    id = tasks_.front();
    tasks_.pop();
    mutex_.unlock();
    return true;
  }
}
