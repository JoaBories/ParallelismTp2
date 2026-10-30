#include "thread_pool.hpp"

#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

int failures = 0;
using namespace std::chrono_literals;

void expect(bool ok, const char* msg) {
  std::cout << (ok ? "[OK]   " : "[FAIL] ") << msg << '\n';
  failures += !ok;
}

template <class T>
bool ready(std::future<T>& f, std::chrono::milliseconds timeout = 1500ms) {
  return f.wait_for(timeout) == std::future_status::ready;
}

void test_empty_pool() {
  ThreadPool pool(2);
  expect(true, "créer puis détruire un pool vide");
}

void test_submit_result() {
  ThreadPool pool(2);
  auto f = pool.submit([] { return 7 * 6; });
  expect(ready(f) && f.get() == 42, "soumettre une tâche → résultat");
}

void test_many_once() {
  constexpr int n = 2000;
  ThreadPool pool(4);
  std::vector<std::future<int>> futures;
  futures.reserve(n);
  for (int i = 0; i < n; ++i) {
    futures.push_back(pool.submit([i] { return i; }));
  }
  std::vector<int> seen(n, 0);
  bool ok = true;
  for (auto& f : futures) {
    if (!ready(f)) {
      ok = false;
      break;
    }
    const int v = f.get();
    if (v >= 0 && v < n) {
      ++seen[static_cast<std::size_t>(v)];
    }
  }
  for (int c : seen) {
    if (c != 1) {
      ok = false;
      break;
    }
  }
  expect(ok, "nombreuses tâches, chacune une fois");
}

void test_exception() {
  ThreadPool pool(2);
  auto bad = pool.submit([]() -> int { throw std::runtime_error("x"); });
  auto good = pool.submit([] { return 1; });
  bool got = false;
  if (ready(bad)) {
    try {
      (void)bad.get();
    } catch (const std::runtime_error&) {
      got = true;
    }
  }
  expect(got && ready(good) && good.get() == 1, "exception via future, pool continue");
}

void test_drain() {
  std::atomic<int> done{0};
  {
    ThreadPool pool(2);
    for (int i = 0; i < 100; ++i) {
      pool.submit([&done] {
        std::this_thread::sleep_for(1ms);
        done.fetch_add(1);
      });
    }
    pool.close();
  }
  expect(done.load() == 100, "fermeture drainante (tâches déjà acceptées)");
}

void test_reject_after_close() {
  ThreadPool pool(2);
  pool.close();
  bool rejected = false;
  try {
    (void)pool.submit([] { return 1; });
  } catch (const std::runtime_error&) {
    rejected = true;
  }
  expect(rejected, "soumettre après close → refus");
}

}  // namespace

int main() {
  try {
    test_empty_pool();
    test_submit_result();
    test_many_once();
    test_exception();
    test_drain();
    test_reject_after_close();
  } catch (const std::exception& e) {
    std::cout << "[FAIL] exception inattendue: " << e.what() << '\n';
    return 1;
  }
  return failures == 0 ? 0 : 1;
}
