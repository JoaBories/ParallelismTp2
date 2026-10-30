#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>

class TaskQueue {
 public:
  void push(std::function<void()> task);

  // Bloque jusqu'à une tâche ou un arrêt. Retourne false si arrêté et file vide.
  bool pop(std::function<void()>& task);

  void request_stop();

 private:
  std::mutex mutex_;
  std::condition_variable available_;
  std::queue<std::function<void()>> tasks_;
  bool stop_ = false;
};
