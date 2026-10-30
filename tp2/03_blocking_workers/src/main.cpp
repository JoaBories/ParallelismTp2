#include "task_queue.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

namespace {

constexpr int kTaskCount = 1500;
constexpr int kWorkerCount = 4;

struct Observation {
  std::mutex mutex;
  std::vector<int> counts;
  std::atomic<int> done{0};

  explicit Observation(int n) : counts(static_cast<std::size_t>(n), 0) {}

  void record(int id) {
    {
      std::lock_guard<std::mutex> lock(mutex);
      ++counts[static_cast<std::size_t>(id)];
    }
    done.fetch_add(1);
  }

  bool every_id_once() const {
    for (int c : counts) {
      if (c != 1) {
        return false;
      }
    }
    return true;
  }
};

}  // namespace

int main() {
  TaskQueue queue;
  Observation obs(kTaskCount);

  std::vector<std::thread> workers;
  workers.reserve(kWorkerCount);
  for (int w = 0; w < kWorkerCount; ++w) {
    workers.emplace_back([&] {
      for (;;) {
        std::function<void()> task;
        if (!queue.pop(task)) {
          break;
        }
        task();
      }
    });
  }

  for (int i = 0; i < kTaskCount; ++i) {
    queue.push([i, &obs] { obs.record(i); });
  }

  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (obs.done.load() < kTaskCount && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::yield();
  }

  queue.request_stop();

  for (auto& worker : workers) {
    worker.join();
  }

  const bool ok = obs.every_id_once();
  std::cout << (ok ? "[OK]   " : "[FAIL] ")
            << "workers bloquants, chaque tâche une fois (done=" << obs.done.load()
            << ")\n";
  return ok ? 0 : 1;
}
