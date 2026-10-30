#include "shared_queue.hpp"

void SharedQueue::push(int id) {
  std::lock_guard<std::mutex> lock(mutex_);
  tasks_.push(id);
}

bool SharedQueue::try_pop(int& id) {
  // TODO: sous mutex_, retirer la tête dans id.
  //       File vide : retourner false.
  (void)id;
  return false;
}
