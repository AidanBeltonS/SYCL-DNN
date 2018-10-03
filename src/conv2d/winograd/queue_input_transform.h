/*
 * Copyright 2018 Codeplay Software Ltd
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
#ifndef SYCLDNN_SRC_CONV2D_WINOGRAD_QUEUE_INPUT_TRANSFORM_H_
#define SYCLDNN_SRC_CONV2D_WINOGRAD_QUEUE_INPUT_TRANSFORM_H_

#include "sycldnn/accessor_types.h"
#include "sycldnn/status.h"

#include "sycldnn/conv2d/conv_type.h"
#include "sycldnn/conv2d/params.h"

#include "sycldnn/internal/conv2d/winograd/tile_info.h"

namespace sycldnn {
namespace conv2d {
namespace internal {
namespace winograd {

template <typename T, typename Index, typename ConvType, int M, int N, int R,
          int S, int ChannelVector>
SNNStatus queue_input_transform(ReadAccessor<T const> input,
                                WriteAccessor<T> in_transform,
                                Conv2DParams const& params,
                                TileInfo const& tile_info,
                                cl::sycl::queue& queue);

}  // namespace winograd
}  // namespace internal
}  // namespace conv2d
}  // namespace sycldnn

#endif  // SYCLDNN_SRC_CONV2D_WINOGRAD_QUEUE_INPUT_TRANSFORM_H_