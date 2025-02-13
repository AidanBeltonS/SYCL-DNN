/*
 * Copyright Codeplay Software Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
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

#ifndef SYCLDNN_INCLUDE_BINARYOP_LAUNCH_INTERNAL_H_
#define SYCLDNN_INCLUDE_BINARYOP_LAUNCH_INTERNAL_H_

#include <vector>

#include "sycldnn/mem_object.h"
#include "sycldnn/status.h"

#include "sycldnn/export.h"

namespace sycldnn {
namespace binaryop {
namespace internal {

/**
 * @brief Compute binary op out dimensions after performing a
 * multidirectional broadcast on input operands.
 *
 * @param[in] lhs_dims
 * @param[in] rhs_dims
 * @param[out] out_dims
 * @return Status code
 */
inline SNNStatus compute_out_dims(const std::vector<int>& lhs_dims,
                                  const std::vector<int>& rhs_dims,
                                  std::vector<int>& out_dims) {
  std::vector<int> smallest_dims =
      lhs_dims.size() <= rhs_dims.size() ? lhs_dims : rhs_dims;
  const std::vector<int>& largest_dims =
      lhs_dims.size() <= rhs_dims.size() ? rhs_dims : lhs_dims;

  // Prepend smallest_dims with 1s to match the number of dimensions
  size_t num_dims = largest_dims.size();
  while (smallest_dims.size() < num_dims) {
    smallest_dims.insert(smallest_dims.begin(), 1);
  }

  for (size_t i = 0; i < num_dims; ++i) {
    SNN_VALIDATE_PARAM(smallest_dims[i] == largest_dims[i] ||
                           smallest_dims[i] == 1 || largest_dims[i] == 1,
                       "Dimensions cannot be broadcasted.");
    out_dims.push_back(std::max(smallest_dims[i], largest_dims[i]));
  }
  return StatusCode::OK;
}

template <typename Op, typename T>
SNN_EXPORT SNNStatus launch_binaryop(
    BaseMemObject<T const>& lhs, BaseMemObject<T const>& rhs,
    BaseMemObject<T>& out, std::vector<int> lhs_dims, std::vector<int> rhs_dims,
    const std::vector<int>& out_dims, cl::sycl::queue& queue);

template <typename Op, typename T>
SNNStatus launch_binaryop(BaseMemObject<T const>& lhs,
                          BaseMemObject<T const>& rhs, BaseMemObject<T>& out,
                          const std::vector<int>& lhs_dims,
                          const std::vector<int>& rhs_dims,
                          cl::sycl::queue& queue) {
  std::vector<int> out_dims;
  auto status = compute_out_dims(lhs_dims, rhs_dims, out_dims);
  if (status.status != StatusCode::OK) return status;
  return launch_binaryop<Op>(lhs, rhs, out, lhs_dims, rhs_dims, out_dims,
                             queue);
}

template <typename Op, typename T>
SNNStatus launch_binaryop(BaseMemObject<T const>& lhs,
                          BaseMemObject<T const>& rhs, BaseMemObject<T>& out,
                          const std::vector<int>& dims,
                          cl::sycl::queue& queue) {
  return launch_binaryop<Op>(lhs, rhs, out, dims, dims, dims, queue);
}

template <typename Op, typename T>
SNNStatus launch_binaryop(BaseMemObject<T const>& lhs,
                          BaseMemObject<T const>& rhs, BaseMemObject<T>& out,
                          int size, cl::sycl::queue& queue) {
  return launch_binaryop<Op>(lhs, rhs, out, std::vector<int>{size}, queue);
}

}  // namespace internal
}  // namespace binaryop
}  // namespace sycldnn

#endif  // SYCLDNN_INCLUDE_BINARYOP_LAUNCH_INTERNAL_H_
