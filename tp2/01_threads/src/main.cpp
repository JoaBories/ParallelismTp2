#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

constexpr int kBench[] = {10, 100, 500, 1000, 2000};
constexpr long nBench[] = {1000, 1000000};

int compute(int id, int n) {
  // Travail artificiel, déterministe, indépendant des autres tâches.
  volatile std::uint64_t acc = 0;
  for (long i = 0; i < n; ++i) {
    acc += static_cast<std::uint64_t>(id + i) * 2654435761ull;
  }
  return static_cast<int>(acc & 0xffff);
}

using Clock = std::chrono::steady_clock;

double ms_since(Clock::time_point start) {
  return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

// 1. Séquentiel : un seul incrément global. C'est la vérité.
long long run_sequential(int k, int n) {
  long long sum = 0;
  for (int i = 0; i < k; ++i) {
    sum += compute(i, n);
  }
  return sum;
}

// 2. k threads, un compute(i, n) chacun.
//    TODO: sum += compute(i, n) protégé par un mutex. Joindre après le lancement.
long long run_mutex(int k, int n) {
  long long sum = 0;
  std::mutex mutex;
  std::vector<std::thread> workers;
  workers.reserve(k);
  
  for (int i = 0; i < k; ++i)
  {
    workers.emplace_back([i,n,&sum,&mutex]
    {
      const int res = compute(i, n);
      mutex.lock();
      sum += res;
      mutex.unlock();
    });
  }
  
  for (auto& w : workers) { w.join(); }
  
  return sum;
}

// 3. Même découpage. TODO: sum est un std::atomic<long long>, fetch_add, pas de mutex.
long long run_atomic(int k, int n) {
  std::atomic<long long> sum = 0;
  std::vector<std::thread> workers;
  workers.reserve(k);
  
  for (int i = 0; i < k; ++i)
  {
    workers.emplace_back([i,n,&sum]
    {
      const int res = compute(i, n);
      sum.fetch_add(res);
    });
  }
  
  for (auto& w : workers) { w.join(); }
  
  return sum;
}

// 4. Même découpage. TODO: chaque thread accumule dans une variable locale,
//    puis une réduction après les join. Aucun mutex, aucun atomique sur la somme.
long long run_local(int k, int n) {
  std::vector<long long> partial;
  std::vector<std::thread> workers;
  workers.reserve(k);
  partial.resize(k);

  for (int i = 0; i < k; ++i)
  {
    workers.emplace_back([i, n, &partial]
    {
      partial[i] = compute(i, n);
    });
  }

  for (auto& w : workers)
  {
    w.join();
  }

  long long sum = 0;
  for (const long long p : partial) { sum += p; }

  return sum;
}

struct Sample {
  long long sum = 0;
  double ms = 0;
};

Sample measure(long long (*run)(int, int), int k, int n) {
  const auto start = Clock::now();
  Sample sample;
  sample.sum = run(k, n);
  sample.ms = ms_since(start);
  return sample;
}

void print_cell(const Sample& sample, long long truth) {
  std::cout << std::setw(14);
  if (sample.sum == truth) {
    std::cout << sample.ms;
  } else {
    std::cout << "FAIL";
  }
}

int main() {
  int failures = 0;

  std::cout << std::fixed << std::setprecision(1);
  std::cout << std::setw(8) << "k" << std::setw(8) << "n" << std::setw(14) << "séquentiel"
            << std::setw(14) << "mutex" << std::setw(14) << "atomic" << std::setw(14) << "local"
            << "\n";

  for (int k : kBench) {
    for (int n : nBench) {
      const Sample sequential = measure(run_sequential, k, n);
      const Sample mutex = measure(run_mutex, k, n);
      const Sample atomic = measure(run_atomic, k, n);
      const Sample local = measure(run_local, k, n);

      std::cout << std::setw(8) << k << std::setw(8) << n;
      print_cell(sequential, sequential.sum);
      print_cell(mutex, sequential.sum);
      print_cell(atomic, sequential.sum);
      print_cell(local, sequential.sum);
      std::cout << "\n";

      failures += mutex.sum != sequential.sum;
      failures += atomic.sum != sequential.sum;
      failures += local.sum != sequential.sum;
    }
  }

  return failures == 0 ? 0 : 1;
}
