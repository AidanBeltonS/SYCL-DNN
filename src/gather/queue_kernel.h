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

#ifndef SYCLDNN_SRC_GATHER_QUEUE_H_
#define SYCLDNN_SRC_GATHER_QUEUE_H_

#include "sycldnn/mem_object.h"
#include "sycldnn/status.h"

#include "sycldnn/gather/sizes.h"

namespace sycldnn {
namespace gather {
namespace internal {

template <typename T, typename Index>
SNNStatus queue_gather(BaseMemObject<T const>& in_mem,
                       BaseMemObject<Index const>& indices_mem,
                       BaseMemObject<T>& out_mem, const GatherSizes& gs,
                       cl::sycl::queue& queue);

}  // namespace internal
}  // namespace gather
}  // namespace sycldnn

#endif  // SYCLDNN_SRC_GATHER_QUEUE_H_
