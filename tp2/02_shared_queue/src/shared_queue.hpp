#pragma once

#include <mutex>
#include <queue>

class SharedQueue {
 public:
  void push(int id);
  bool try_pop(int& id);

 private:
  std::queue<int> tasks_;
  std::mutex mutex_;
};
