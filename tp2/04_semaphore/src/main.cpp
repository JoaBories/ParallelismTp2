#include <atomic>
#include <chrono>
#include <iostream>
#include <semaphore>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {

constexpr int kSlots = 2;
constexpr int kWorkers = 8;
constexpr int kJobsPerWorker = 40;

std::counting_semaphore<kSlots> slots(kSlots);
std::atomic<int> in_use{0};
std::atomic<int> max_in_use{0};
std::atomic<bool> failed{false};

class SemaphoreGuard {
 public:
  explicit SemaphoreGuard(std::counting_semaphore<kSlots>& sem) : sem_(sem) {
    // TODO: acquire
    sem_.acquire();
  }

  ~SemaphoreGuard() {
    // TODO: release
    sem_.release();
  }

  SemaphoreGuard(const SemaphoreGuard&) = delete;
  SemaphoreGuard& operator=(const SemaphoreGuard&) = delete;

 private:
  std::counting_semaphore<kSlots>& sem_;
};

void use_resource(bool throw_once) {
  // TODO: remplacer l'accès direct par un SemaphoreGuard (ou acquire/release manuels).
  // Le corps ci-dessous doit s'exécuter sous au plus kSlots permissions.
  SemaphoreGuard guard(slots);
  const int now = in_use.fetch_add(1) + 1;
  int prev = max_in_use.load();
  while (now > prev && !max_in_use.compare_exchange_weak(prev, now)) {
  }
  if (now > kSlots) {
    failed.store(true);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  if (throw_once) {
    in_use.fetch_sub(1);
    throw std::runtime_error("boom");
  }
  in_use.fetch_sub(1);
}

}  // namespace

int main() {
  std::vector<std::thread> workers;
  workers.reserve(kWorkers);

  for (int w = 0; w < kWorkers; ++w) {
    workers.emplace_back([w] {
      for (int j = 0; j < kJobsPerWorker; ++j) {
        try {
          use_resource(w == 0 && j == 3);
        } catch (const std::runtime_error&) {
          // attendu une fois : le garde doit quand même restituer la permission
        }
      }
    });
  }

  for (auto& t : workers) {
    t.join();
  }

  const bool ok = !failed.load() && max_in_use.load() <= kSlots && in_use.load() == 0;
  std::cout << (ok ? "[OK]   " : "[FAIL] ")
            << "capacité max observée=" << max_in_use.load()
            << " (limite " << kSlots << "), in_use final=" << in_use.load() << '\n';
  return ok ? 0 : 1;
}
