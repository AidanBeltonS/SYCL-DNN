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
#ifndef SYCLDNN_INCLUDE_SCATTER_ND_LAUNCH_H_
#define SYCLDNN_INCLUDE_SCATTER_ND_LAUNCH_H_

/**
 * \file
 * Implements the \ref sycldnn::scatter_nd::launch() function, which
 * asynchronously dispatches a SYCL kernel to compute a scatter_nd operation
 * along a single dimension of a N-dimensional tensor.
 */
#include "sycldnn/status.h"

#include "sycldnn/scatter_nd/operators.h"
#include "sycldnn/scatter_nd/sizes.h"

#include "sycldnn/internal/scatter_nd/launch_internal.h"

#include "sycldnn/helpers/macros.h"

namespace sycldnn {
/** Namespace containing the scatter_nd operator. */
namespace scatter_nd {
/** Namespace containing internal implementation details for scatter_nd. */
namespace internal {

/**
 * Validate that the user-provided scatter_nd parameters are consistent with
 * what is expected by SYCL-DNN.
 *
 * If compiled with asserts, any invalid parameter will fail with an assert.
 * Otherwise a status code \ref StatusCode::InvalidParameter will be returned.
 *
 * \param params  ScatterND parameters to validate.
 * \return        A SNNStatus object containing either \ref StatusCode::OK if
 * all parameters are valid, or \ref StatusCode::InvalidParameter otherwise.
 */
SNNStatus inline validate_params(ScatterNDParams const& params) {
  int rank = params.input_dims.size();

  SNN_VALIDATE_PARAM(rank > 0, "Rank of input must be greater than 0.");
  SNN_VALIDATE_PARAM(
      rank < 5,
      "SYCL-DNN only supports up to 4 dimensional tensors currently.");
  SNN_VALIDATE_PARAM(params.input_dims[0] > 0, "dim_0 must be positive.");
  if (rank > 1) {
    SNN_VALIDATE_PARAM(params.input_dims[1] > 0, "dim_1 must be positive.");
  }
  if (rank > 2) {
    SNN_VALIDATE_PARAM(params.input_dims[2] > 0, "dim_2 must be positive.");
  }
  if (rank > 3) {
    SNN_VALIDATE_PARAM(params.input_dims[3] > 0, "dim_3 must be positive.");
  }

  auto index_rank = params.index_dims.size();
  SNN_VALIDATE_PARAM(index_rank == 2, "Rank of index tensor must equal 2.");

  auto index_depth = params.index_dims[1];
  SNN_VALIDATE_PARAM(index_depth <= rank,
                     "Index depth must be less than or equal to the rank");
  return SNNStatus(StatusCode::OK);
}

}  // namespace internal

/**
 * Launch the scatter_nd operation kernel.
 *
 * \tparam T           The data type of the input tensor.
 * \tparam Indices     The data type of the indices tensor.
 * \tparam ScatterNDType The update operator used, such as Assign, Add, Mul etc.
 * \tparam Backend      The type of backend.
 * \param input         A pointer to the memory representing the input tensor.
 * \param indices       A pointer to the memory representing the indices tensor.
 * \param update        A pointer to the memory representing the updates tensor.
 * \param output        A pointer to the memory representing the output tensor.
 * \param params        The scatter_nd parameters, which describe the tensor
 * shape and layout.
 * \param backend       The backend implementation, used to
 * map between pointer representations.
 * \return Returns a SNNStatus containing
 * the SYCL event tied to the kernel launches and a StatusCode enum showing if
 * the launch was OK or whether it encountered some problem.
 */
template <typename T, typename Index, typename ScatterNDType, typename Backend>
SNNStatus launch(typename Backend::template pointer_type<T const> input,
                 typename Backend::template pointer_type<Index const> indices,
                 typename Backend::template pointer_type<T const> update,
                 typename Backend::template pointer_type<T> output,
                 ScatterNDParams const& params, Backend& backend) {
  auto validation_status = internal::validate_params(params);
  if (validation_status.status != StatusCode::OK) {
    return validation_status;
  }
  const auto sizes = get_sizes(params);
  const auto num_updates = sizes.num_updates;
  const auto index_depth = sizes.index_depth;
  const auto tensor_size = sizes.output_size;
  const auto slice_size = sizes.slice_size;
  auto in_mem = backend.get_mem_object(input, tensor_size);
  auto out_mem = backend.get_mem_object(output, tensor_size);
  auto ind_mem = backend.get_mem_object(indices, num_updates * index_depth);
  auto upd_mem = backend.get_mem_object(update, num_updates * slice_size);
  auto queue = backend.get_queue();

  switch (index_depth) {
    case 1:
      return internal::launch<T, Index, ScatterNDType, 1>(
          in_mem, ind_mem, upd_mem, out_mem, sizes, queue);
    case 2:
      return internal::launch<T, Index, ScatterNDType, 2>(
          in_mem, ind_mem, upd_mem, out_mem, sizes, queue);
    case 3:
      return internal::launch<T, Index, ScatterNDType, 3>(
          in_mem, ind_mem, upd_mem, out_mem, sizes, queue);
    case 4:
      return internal::launch<T, Index, ScatterNDType, 4>(
          in_mem, ind_mem, upd_mem, out_mem, sizes, queue);
  }
  return SNNStatus(StatusCode::InvalidParameter);
}

}  // namespace scatter_nd
}  // namespace sycldnn

#endif  // SYCLDNN_INCLUDE_SCATTER_ND_LAUNCH_H_
