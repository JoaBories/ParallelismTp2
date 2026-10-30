#include "thread_pool.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

void demo_promise_future() {
  std::promise<int> promise;
  auto future = promise.get_future();

  std::thread worker([p = std::move(promise)]() mutable { p.set_value(42); });

  const int result = future.get();
  worker.join();
  std::cout << (result == 42 ? "[OK]   " : "[FAIL] ")
            << "promise/future hors pool → " << result << '\n';
}

int failures = 0;

void expect(bool ok, const char* msg) {
  std::cout << (ok ? "[OK]   " : "[FAIL] ") << msg << '\n';
  failures += !ok;
}

}  // namespace

int main() {
  demo_promise_future();

  try {
    ThreadPool pool(4);

    auto a = pool.submit([] { return 21; });
    auto b = pool.submit([] { return std::string("pool"); });
    auto c = pool.submit([]() -> int {
      throw std::runtime_error("échec calcul");
    });
    auto d = pool.submit([] { return 21; });

    expect(a.get() == 21, "future<int>");
    expect(b.get() == "pool", "future<string>");

    bool got_exception = false;
    try {
      (void)c.get();
    } catch (const std::runtime_error& e) {
      got_exception = std::string(e.what()) == "échec calcul";
    }
    expect(got_exception, "exception relancée par get()");
    expect(d.get() == 21, "pool continue après exception");

  } catch (const std::exception& e) {
    std::cout << "[FAIL] exception inattendue: " << e.what() << '\n';
    return 1;
  }

  return failures == 0 ? 0 : 1;
}
