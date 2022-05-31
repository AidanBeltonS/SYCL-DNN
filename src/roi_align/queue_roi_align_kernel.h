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

#ifndef SYCLDNN_SRC_ROI_ALIGN_QUEUE_H_
#define SYCLDNN_SRC_ROI_ALIGN_QUEUE_H_

#include "sycldnn/mem_object.h"
#include "sycldnn/status.h"

#include "sycldnn/roi_align/params.h"

namespace sycldnn {
namespace roi_align {
namespace internal {

template <typename T, typename BatchIndicesT, typename Index,
          template <typename> class PoolType>
SNNStatus queue_roi_align(BaseMemObject<T const>& input,
                          BaseMemObject<T const>& rois,
                          BaseMemObject<BatchIndicesT const>& batch_indices,
                          BaseMemObject<T>& output, const RoiAlignParams& rap,
                          size_t threads, cl::sycl::queue& queue);

}  // namespace internal
}  // namespace roi_align
}  // namespace sycldnn

#endif  // SYCLDNN_SRC_ROI_ALIGN_QUEUE_H_