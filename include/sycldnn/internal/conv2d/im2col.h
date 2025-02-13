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
#ifndef SYCLDNN_INCLUDE_INTERNAL_CONV2D_IM2COL_H_
#define SYCLDNN_INCLUDE_INTERNAL_CONV2D_IM2COL_H_

#include "sycldnn/conv2d/params.h"
#include "sycldnn/helpers/macros.h"
#include "sycldnn/status.h"

#include "sycldnn/internal/conv2d/alloc_info.h"
#include "sycldnn/internal/conv2d/batch_info.h"
#include "sycldnn/internal/conv2d/internal_pointer_set.h"

#include "sycldnn/internal/conv2d/im2col/allocated_pointer_set.h"
#include "sycldnn/internal/conv2d/im2col/full_pointer_set.h"
#include "sycldnn/internal/conv2d/im2col/kernel_params.h"
#include "sycldnn/internal/conv2d/im2col/launch_filter_transform.h"
#include "sycldnn/internal/conv2d/im2col/launch_input_transform.h"
#include "sycldnn/internal/conv2d/im2col/offsets.h"
#include "sycldnn/internal/conv2d/im2col/tile_info.h"
#include "sycldnn/internal/conv2d/im2col/workspace_pointer_set.h"

namespace sycldnn {
namespace conv2d {
namespace internal {
namespace im2col {

/** Launch the input transform and matmul to compute im2col. */
template <typename T, typename ConvType, typename Backend,
          typename std::enable_if<
              !std::is_same<ConvType, conv_type::FilterBackprop>::value,
              int>::type = 0>
static SNNStatus launch_im2col_for_minibatch(
    FullPointerSet<T, Backend, ConvType> const& pointers, size_t in_offset,
    size_t out_offset, TileInfo const& tile_info, Conv2DParams const& params,
    Backend& backend) {
  using ConstPointer =
      typename FullPointerSet<T, Backend, ConvType>::ConstPointer;
  auto status =
      launch_input_transform(pointers, in_offset, tile_info, params, backend);
  if (status.status != StatusCode::OK) {
    return status;
  }

  int matmul_size;
  if (std::is_same<ConvType, conv_type::InputBackprop>::value) {
    matmul_size = params.channels;
  } else {
    matmul_size = params.features;
  }
  int const n_tiles = params.batch * tile_info.number;
  int const tile_size = tile_info.size;
  auto event = backend.template matmul<false, false>(
      ConstPointer{pointers.transform}, ConstPointer{pointers.filter},
      pointers.output + out_offset, static_cast<T>(0), n_tiles, tile_size,
      matmul_size);
  return {event, StatusCode::OK};
}

/**
 * Launch the input transform and matmul to compute im2col for the filter
 * backprop pass.
 */
template <typename T, typename ConvType, typename Backend,
          typename std::enable_if<
              std::is_same<ConvType, conv_type::FilterBackprop>::value,
              int>::type = 0>
static SNNStatus launch_im2col_for_minibatch(
    FullPointerSet<T, Backend, ConvType> const& pointers, size_t in_offset,
    size_t out_offset, TileInfo const& tile_info, Conv2DParams const& params,
    Backend& backend) {
  using ConstPointer =
      typename FullPointerSet<T, Backend, ConvType>::ConstPointer;
  auto status =
      launch_input_transform(pointers, in_offset, tile_info, params, backend);
  if (status.status != StatusCode::OK) {
    return status;
  }

  const int n_tiles = tile_info.number;
  const int tile_size = params.batch * tile_info.size;
  cl::sycl::event matmul_event;
  if (in_offset == 0) {
    matmul_event = backend.template matmul<false, false>(
        ConstPointer{pointers.transform}, pointers.filter + out_offset,
        pointers.output, static_cast<T>(0), n_tiles, tile_size,
        params.features);
  } else {
    matmul_event = backend.template matmul<false, false>(
        ConstPointer{pointers.transform}, pointers.filter + out_offset,
        pointers.output, static_cast<T>(1), n_tiles, tile_size,
        params.features);
  }
  return {matmul_event, StatusCode::OK};
}

/** Loop over the minibatches to compute im2col. */
template <typename T, typename ConvType, typename Backend>
static SNNStatus launch_im2col_for_all_minibatches(
    FullPointerSet<T, Backend, ConvType> const& pointers,
    TileInfo const& tile_info, BatchInfo const& batch_info,
    Conv2DParams const& params, Backend& backend) {
  auto filter_status = launch_filter_transform(pointers, params, backend);
  if (filter_status.status != StatusCode::OK) {
    return filter_status;
  }
  auto kernel_params = get_kernel_params<ConvType>(params);
  kernel_params.batch = batch_info.images_per_batch;
  cl::sycl::event event;
  for (size_t i = 0; i < batch_info.n_batches; ++i) {
    auto offset =
        calculate_offsets<ConvType>(i, batch_info.images_per_batch, params);
    if (i == batch_info.n_batches - 1) {
      kernel_params.batch = batch_info.last_batch_size;
    }
    auto status = launch_im2col_for_minibatch(
        pointers, offset.in, offset.out, tile_info, kernel_params, backend);
    event = status.event;
    if (status.status != StatusCode::OK) {
      return status;
    }
  }
  return SNNStatus{event, StatusCode::OK};
}

/**
 * Split the input tensor into minibatches to ensure that the temporary
 * transform buffer can be safely allocated and create SYCL buffers using the
 * Backend. Then use im2col to compute the convolution for each minibatch.
 */
template <typename T, typename ConvType, typename Backend>
SNNStatus allocate_and_launch_im2col(
    typename Backend::template pointer_type<T const> input,
    typename Backend::template pointer_type<T const> filter,
    typename Backend::template pointer_type<T> output,
    Conv2DParams const& params, Backend& backend) {
  InternalPointerSet<T, Backend> pointers{input, filter, output, backend};

  auto const tile_info = im2col::get_tile_info<ConvType>(params);
  size_t const size_per_image = tile_info.number * tile_info.size;
  im2col::AllocatedPointerSet<T, Backend, ConvType> all_pointers{
      pointers, size_per_image, params, backend};

  auto const batch_info = get_batch_info(all_pointers.allocated_transform_size,
                                         params.batch, size_per_image);

  return im2col::launch_im2col_for_all_minibatches(
      all_pointers.to_full_pointer_set(), tile_info, batch_info, params,
      backend);
}

/**
 * Use the provided workspace for the transform data. As the user may provide a
 * smaller workspace than is ideal, check the maximum size of the minibatch to
 * use, and split up the workspace as required. Then use im2col to compute the
 * convolution for each minibatch.
 */
template <typename T, typename ConvType, typename Backend>
SNNStatus launch_im2col_with_workspace(
    typename Backend::template pointer_type<T const> input,
    typename Backend::template pointer_type<T const> filter,
    typename Backend::template pointer_type<T> output,
    typename Backend::template pointer_type<T> workspace,
    Conv2DParams const& params, size_t workspace_size, Backend& backend) {
  InternalPointerSet<T, Backend> pointers{input, filter, output, backend};

  auto const tile_info = im2col::get_tile_info<ConvType>(params);
  size_t const size_per_image = tile_info.number * tile_info.size;
  im2col::WorkspacePointerSet<T, Backend, ConvType> all_pointers{
      pointers, workspace, size_per_image, params, workspace_size, backend};

  auto const batch_info =
      get_batch_info(all_pointers.minibatch_size, params.batch);

  return im2col::launch_im2col_for_all_minibatches(
      all_pointers.to_full_pointer_set(), tile_info, batch_info, params,
      backend);
}

}  // namespace im2col

/**
 * The internal im2col convolution launcher.
 *
 * Use im2col to compute a convolution, by transforming the input data then
 * computing a matrix multiply with the filter to give the output.
 */
template <typename T, typename ConvType, typename Backend>
SNNStatus launch_im2col(typename Backend::template pointer_type<T const> input,
                        typename Backend::template pointer_type<T const> filter,
                        typename Backend::template pointer_type<T> output,
                        typename Backend::template pointer_type<T> workspace,
                        Conv2DParams const& params, size_t workspace_size,
                        Backend& backend) {
  if (workspace_size == 0) {
    return im2col::allocate_and_launch_im2col<T, ConvType>(
        input, filter, output, params, backend);
  } else {
    return im2col::launch_im2col_with_workspace<T, ConvType>(
        input, filter, output, workspace, params, workspace_size, backend);
  }
}

}  // namespace internal
}  // namespace conv2d
}  // namespace sycldnn

#endif  // SYCLDNN_INCLUDE_INTERNAL_CONV2D_IM2COL_H_
