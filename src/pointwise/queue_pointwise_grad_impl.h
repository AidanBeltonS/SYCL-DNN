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

#ifndef SYCLDNN_SRC_POINTWISE_QUEUE_POINTWISE_GRAD_IMPL_H_
#define SYCLDNN_SRC_POINTWISE_QUEUE_POINTWISE_GRAD_IMPL_H_

#include "sycldnn/helpers/ratio.h"
#include "sycldnn/mem_object.h"
#include "sycldnn/pointwise/direction.h"
#include "sycldnn/pointwise/operators.h"
#include "sycldnn/status.h"

#include "src/pointwise/kernels.h"
#include "src/pointwise/queue_pointwise_grad.h"

#include <CL/sycl.hpp>

namespace sycldnn {
namespace pointwise {
namespace internal {

/**
 * Submits a pointwise grad operation to the queue with number of threads equal
 * to the output gradient size scaled by the vector size.
 * */
template <typename T, typename Index, template <typename> class PointwiseType,
          typename Direction, int VectorWidth>
SNNStatus queue_pointwise(BaseMemObject<T const>& in_forward_mem,
                          BaseMemObject<T const>& in_backprop_mem,
                          BaseMemObject<T>& out_backprop_mem,
                          Index const n_items, cl::sycl::queue& queue) {
  auto event = queue.submit([&](cl::sycl::handler& cgh) {
    auto input_forward = in_forward_mem.read_accessor(cgh);
    auto input_backprop = in_backprop_mem.read_accessor(cgh);
    auto output_backprop = out_backprop_mem.write_accessor(cgh);
    Index const n_vecs = n_items / VectorWidth;
    size_t const n_threads = helpers::round_up_to_nearest_multiple(n_vecs, 64);
    PointwiseOp<T, Index, PointwiseType, Direction, VectorWidth> pointwise_op{
        input_forward, input_backprop, output_backprop, n_vecs};

    cgh.parallel_for(cl::sycl::range<1>{n_threads}, pointwise_op);
  });

  return {event, StatusCode::OK};
}

}  // namespace internal
}  // namespace pointwise
}  // namespace sycldnn

#endif  // SYCLDNN_SRC_POINTWISE_QUEUE_POINTWISE_GRAD_IMPL_H_
