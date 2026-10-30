#include "shared_queue.hpp"

#include <iostream>
#include <thread>
#include <vector>

namespace {

constexpr int kTaskCount = 2000;
constexpr int kWorkerCount = 8;

class Worker {
 public:
  Worker(SharedQueue& queue, std::vector<int>& counts)
      : queue_(queue), counts_(counts) {}

  void operator()() {
    for (;;) {
      int id = 0;
      if (!queue_.try_pop(id)) {
        continue;
      }
      if (id < 0) {
        break;
      }
      ++counts_[static_cast<std::size_t>(id)];
    }
  }

 private:
  SharedQueue& queue_;
  std::vector<int>& counts_;
};

}  // namespace

int main() {
  SharedQueue queue;
  for (int i = 0; i < kTaskCount; ++i) {
    queue.push(i);
  }
  for (int w = 0; w < kWorkerCount; ++w) {
    queue.push(-1);
  }

  std::vector<int> counts(static_cast<std::size_t>(kTaskCount), 0);
  std::vector<std::thread> workers;
  workers.reserve(kWorkerCount);

  for (int w = 0; w < kWorkerCount; ++w) {
    workers.emplace_back(Worker{queue, counts});
  }
  for (auto& worker : workers) {
    worker.join();
  }

  bool ok = true;
  for (int count : counts) {
    if (count != 1) {
      ok = false;
      break;
    }
  }
  std::cout << (ok ? "[OK]   " : "[FAIL] ")
            << "chaque tâche extraite exactement une fois\n";
  return ok ? 0 : 1;
}
