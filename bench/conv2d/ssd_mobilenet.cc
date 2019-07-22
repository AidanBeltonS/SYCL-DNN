/*
 * Copyright 2019 Codeplay Software Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use these files except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "benchmark_config.h"
#include "param_set.h"

#include <vector>

char const* get_benchmark_name() { return "SSD + MobileNet"; }

#define CONFIG(N, WIN, STR, H, W, C, F, MOD) \
  benchmark_params::serialize(N, WIN, STR, H, W, C, F, MOD)

std::vector<std::vector<int>> const& get_benchmark_configs() {
  static std::vector<std::vector<int>> const configs = {

// Standard benchmark sizes (batch size: 1, 4, optionally 32)
#define SSD_MOBILENET_PARAMS(WIN, STR, H, W, C, F, MOD) \
  CONFIG(1, WIN, STR, H, W, C, F, MOD),
#include "bench/conv2d/ssd_mobilenet_params.def"
#undef SSD_MOBILENET_PARAMS

#define SSD_MOBILENET_PARAMS(WIN, STR, H, W, C, F, MOD) \
  CONFIG(4, WIN, STR, H, W, C, F, MOD),
#include "bench/conv2d/ssd_mobilenet_params.def"
#undef SSD_MOBILENET_PARAMS

#ifdef SNN_LARGE_BATCH_BENCHMARKS
#define SSD_MOBILENET_PARAMS(WIN, STR, H, W, C, F, MOD) \
  CONFIG(32, WIN, STR, H, W, C, F, MOD),
#include "bench/conv2d/ssd_mobilenet_params.def"
#undef SSD_MOBILENET_PARAMS
#endif  // SNN_LARGE_BATCH_BENCHMARKS

// Extended benchmarks (batch size: 2, optionally 8, 16, 64)
#ifdef SNN_EXTENDED_BENCHMARKS
#define SSD_MOBILENET_PARAMS(WIN, STR, H, W, C, F, MOD) \
  CONFIG(2, WIN, STR, H, W, C, F, MOD),
#include "bench/conv2d/ssd_mobilenet_params.def"
#undef SSD_MOBILENET_PARAMS

#ifdef SNN_LARGE_BATCH_BENCHMARKS
#define SSD_MOBILENET_PARAMS(WIN, STR, H, W, C, F, MOD) \
  CONFIG(8, WIN, STR, H, W, C, F, MOD),
#include "bench/conv2d/ssd_mobilenet_params.def"
#undef SSD_MOBILENET_PARAMS

#define SSD_MOBILENET_PARAMS(WIN, STR, H, W, C, F, MOD) \
  CONFIG(16, WIN, STR, H, W, C, F, MOD),
#include "bench/conv2d/ssd_mobilenet_params.def"
#undef SSD_MOBILENET_PARAMS

#define SSD_MOBILENET_PARAMS(WIN, STR, H, W, C, F, MOD) \
  CONFIG(64, WIN, STR, H, W, C, F, MOD),
#include "bench/conv2d/ssd_mobilenet_params.def"
#undef SSD_MOBILENET_PARAMS
#endif  // SNN_LARGE_BATCH_BENCHMARKS
#endif  // SNN_EXTENDED_BENCHMARKS

  };
  return configs;
}
