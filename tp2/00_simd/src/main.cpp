#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>

namespace {

constexpr int kLanes = 8;
constexpr int kRepeat = 100000;
constexpr int kSamples = 50;

float relu(float z) { return z > 0.f ? z : 0.f; }

// 8 flottants : exactement un registre 256 bits.
// y[i] = relu(w[i] * x[i] + b[i])
void run_scalar(const float* x, const float* w, const float* b, float* y) {
  for (int i = 0; i < kLanes; ++i) {
    y[i] = relu(w[i] * x[i] + b[i]);
  }
}

// TODO: le même calcul en un registre __m256.
//       produit-addition (FMA), puis ReLU avec _mm256_max_ps.
void run_avx2(const float* x, const float* w, const float* b, float* y) {
  const __m256 xReg = _mm256_loadu_ps(x);
  const __m256 wReg = _mm256_loadu_ps(w);
  const __m256 bReg = _mm256_loadu_ps(b);

  const __m256 zeroReg = _mm256_setzero_ps();
  
  __m256 yReg = _mm256_mul_ps(xReg, wReg);
  yReg = _mm256_add_ps(yReg, bReg);
  yReg = _mm256_max_ps(yReg, zeroReg);
  
  _mm256_storeu_ps(y, yReg);
}

using Clock = std::chrono::steady_clock;

double ms_since(Clock::time_point start) {
  return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

bool same(const float* got, const float* truth) {
  for (int i = 0; i < kLanes; ++i) {
    if (std::fabs(got[i] - truth[i]) > 1e-5f) {
      return false;
    }
  }
  return true;
}

struct Stats {
  double mean = 0;
  double stddev = 0;
};

Stats summarize(const double* samples, int n) {
  double mean = 0;
  for (int i = 0; i < n; ++i) {
    mean += samples[i];
  }
  mean /= n;
  double var = 0;
  for (int i = 0; i < n; ++i) {
    const double d = samples[i] - mean;
    var += d * d;
  }
  return {mean, std::sqrt(var / (n - 1))};
}

}  // namespace

int main() {
  const float x[kLanes] = {1.f, -2.f, 0.5f, -0.25f, 3.f, -4.f, 0.f, 1.5f};
  const float w[kLanes] = {0.5f, 1.f, -1.f, 2.f, 0.25f, -0.5f, 3.f, -2.f};
  const float b[kLanes] = {0.1f, -0.1f, 0.f, 1.f, -1.f, 0.5f, -0.5f, 0.2f};
  float truth[kLanes]{};
  float got[kLanes]{};

  volatile float sink = 0.f;
  double scalar_ms[kSamples]{};
  double avx2_ms[kSamples]{};
  for (int s = 0; s < kSamples; ++s) {
    const auto t0 = Clock::now();
    for (int r = 0; r < kRepeat; ++r) {
      run_scalar(x, w, b, truth);
      sink += truth[0];
    }
    scalar_ms[s] = ms_since(t0);

    const auto t1 = Clock::now();
    for (int r = 0; r < kRepeat; ++r) {
      run_avx2(x, w, b, got);
      sink += got[0];
    }
    avx2_ms[s] = ms_since(t1);
  }
  (void)sink;
  const bool ok = same(got, truth);
  const Stats scalar = summarize(scalar_ms, kSamples);
  const Stats avx2 = summarize(avx2_ms, kSamples);

  std::cout << std::fixed << std::setprecision(2);
  std::cout << std::setw(12) << "" << std::setw(14) << "moyenne" << std::setw(14) << "ecart-type"
            << "\n";
  std::cout << std::setw(12) << "scalaire" << std::setw(14) << scalar.mean << std::setw(14)
            << scalar.stddev << "\n";
  std::cout << std::setw(12) << "avx2";
  if (ok) {
    std::cout << std::setw(14) << avx2.mean << std::setw(14) << avx2.stddev << "\n";
  } else {
    std::cout << std::setw(14) << "FAIL" << "\n";
  }

  return ok ? 0 : 1;
}
