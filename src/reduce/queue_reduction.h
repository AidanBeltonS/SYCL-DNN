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
#ifndef SYCLDNN_SRC_REDUCE_QUEUE_KERNEL_H_
#define SYCLDNN_SRC_REDUCE_QUEUE_KERNEL_H_

#include <unordered_map>

#include "sycldnn/internal/helpers/types.h"
#include "sycldnn/mem_object.h"
#include "sycldnn/status.h"

namespace sycldnn {
namespace reduce {
namespace internal {

/** Add a reduce kernel to the provided SYCL queue. */
template <typename T, typename Index, typename Op>
SNNStatus queue_default_kernel(BaseMemObject<T const>& input,
                               BaseMemObject<T>& output, int batches, int outer,
                               int inner, int finalizeParam,
                               cl::sycl::queue& queue);

#ifndef SNN_DISABLE_SYCL_PROGRAM
template <typename T, typename Index, typename Op>
SNNStatus queue_subgroup_kernel(
    BaseMemObject<T const>& input_mem, BaseMemObject<T>& output_mem,
    int batches, int outer, int inner, cl::sycl::queue& queue,
    cl::sycl::program& program,
    sycldnn::internal::types::KernelSubgroupSizesMap&
        max_kernel_sub_group_sizes);
#endif
}  // namespace internal
}  // namespace reduce
}  // namespace sycldnn

#endif  // SYCLDNN_SRC_REDUCE_QUEUE_KERNEL_H_
