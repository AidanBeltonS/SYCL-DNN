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
#ifndef SYCLDNN_SRC_REDUCE_QUEUE_REDUCTION_IMPL_H_
#define SYCLDNN_SRC_REDUCE_QUEUE_REDUCTION_IMPL_H_

#include <limits>
#include <type_traits>

#include "sycldnn/mem_object.h"
#include "sycldnn/status.h"

#include "src/helpers/math.h"
#include "src/reduce/default_kernel.h"
#include "src/reduce/queue_reduction.h"

#ifndef SNN_DISABLE_SYCL_PROGRAM
#include "src/reduce/subgroup_kernel.h"
#endif

namespace sycldnn {
namespace reduce {
namespace internal {

template <class T, class Op>
static constexpr T init_val = 0;

template <class T>
static constexpr T init_val<T, Max> = std::numeric_limits<T>::min();

template <class T>
static constexpr T init_val<T, Min> = std::numeric_limits<T>::max();

template <typename T, typename Index, typename Op>
SNNStatus queue_default_kernel(BaseMemObject<T const>& input_mem,
                               BaseMemObject<T>& output_mem, int batches,
                               int outer, int inner, int finalizeParam,
                               cl::sycl::queue& queue) {
  auto event = queue.submit([&](cl::sycl::handler& cgh) {
    auto input = input_mem.read_accessor(cgh);
    auto output = output_mem.write_accessor(cgh);

    ReduceKernel<T, Index, Op> functor{
        input, output, batches, outer, inner, finalizeParam, init_val<T, Op>};

    cgh.parallel_for(cl::sycl::range<2>(batches, inner), functor);
  });
  return {event, StatusCode::OK};
}

#ifndef SNN_DISABLE_SYCL_PROGRAM
template <typename T, typename Index, typename Op>
SNNStatus queue_subgroup_kernel(
    BaseMemObject<T const>& input_mem, BaseMemObject<T>& output_mem,
    int batches, int outer, int inner, cl::sycl::queue& queue,
    cl::sycl::program& program,
    sycldnn::internal::types::KernelSubgroupSizesMap&
        max_kernel_sub_group_sizes) {
  using Kernel = ReduceSubgroupKernel<T, Index, Op>;
  using namespace sycldnn::helpers::math;
  auto device = queue.get_device();
  const size_t max_work_group_size =
      device.get_info<cl::sycl::info::device::max_work_group_size>();
  const auto max_work_item_sizes =
      device.get_info<cl::sycl::info::device::max_work_item_sizes>();
  size_t alignment = std::min(max_work_item_sizes[0], max_work_group_size);

  auto fallback = [&](BaseMemObject<T const>& input, size_t outer_size) {
    return queue_default_kernel<T, Index, Op>(input, output_mem, batches,
                                              outer_size, inner, outer, queue);
  };
  auto query_subgroup_size = [&](cl::sycl::kernel kernel,
                                 const cl::sycl::range<2>& local_range) {
    return kernel.template get_sub_group_info<
        cl::sycl::info::kernel_sub_group::max_sub_group_size_for_ndrange>(
        device, cl::sycl::range<3>(1, local_range[0], local_range[1]));
  };

  size_t max_sub_group_size;
  static const std::string kernelName = typeid(Kernel).name();
  if (max_kernel_sub_group_sizes.find(kernelName) !=
      max_kernel_sub_group_sizes.end()) {
    max_sub_group_size = max_kernel_sub_group_sizes[kernelName];
  } else {
    program.build_with_kernel_type<Kernel>();
    max_sub_group_size = query_subgroup_size(program.get_kernel<Kernel>(),
                                             cl::sycl::range<2>(1, alignment));
    max_kernel_sub_group_sizes.insert({kernelName, max_sub_group_size});
  }
  cl::sycl::kernel kernel = program.get_kernel<Kernel>();

  if (max_sub_group_size == 1) return fallback(input_mem, outer);

  cl::sycl::range<2> input_range(batches, outer);
  cl::sycl::range<2> kernel_range = input_range;
  cl::sycl::range<2> local_wg_range(1, 1);
  auto update_local_range = [&]() {
    if (kernel_range[1] < alignment) {
      local_wg_range[1] = kernel_range[1];
    } else {
      kernel_range[1] = align(kernel_range[1], max_sub_group_size);
      local_wg_range[1] = max_sub_group_size;
      size_t multiple = kernel_range[1] / max_sub_group_size;
      for (int i = multiple; i > 1; --i) {
        size_t new_size = local_wg_range[1] * i;
        if (new_size <= alignment && kernel_range[1] % new_size == 0) {
          local_wg_range[1] = new_size;
          break;
        }
      }
    }
  };
  update_local_range();

  size_t sub_group_size = query_subgroup_size(kernel, local_wg_range);
  if (sub_group_size <= 1) return fallback(input_mem, outer);

  size_t reduce_size = input_range[1];
  size_t next_reduce_size = divide_ceil(input_range[1], sub_group_size);
  cl::sycl::range<2> buffer1_size(input_range[0], next_reduce_size);
  cl::sycl::range<2> buffer2_size(
      input_range[0], divide_ceil(next_reduce_size, sub_group_size));
  cl::sycl::buffer<T, 1> buffer(
      cl::sycl::range<1>(buffer1_size.size() + buffer2_size.size()));
  auto buffer_mem1 = make_mem_object(buffer, buffer1_size.size(), 0);
  auto buffer_mem2 =
      make_mem_object(buffer, buffer2_size.size(), buffer1_size.size());

  cl::sycl::nd_range<2> nd_range0(kernel_range, local_wg_range);
  auto event = queue.submit([&](cl::sycl::handler& cgh) {
    auto in_acc = input_mem.read_accessor(cgh);
    auto out_acc = next_reduce_size == 1 ? output_mem.write_accessor(cgh)
                                         : buffer_mem1.write_accessor(cgh);
    size_t out_size1 = out_acc.get_extent() / input_range[0];
    Kernel functor(in_acc, out_acc, sub_group_size, reduce_size, input_range[1],
                   out_size1);
    cgh.parallel_for(kernel, nd_range0, functor);
  });
  int iter = 0;
  while (next_reduce_size > 1) {
    reduce_size = next_reduce_size;
    next_reduce_size = divide_ceil(next_reduce_size, sub_group_size);
    kernel_range[1] = divide_ceil(kernel_range[1], sub_group_size);
    update_local_range();
    sub_group_size = query_subgroup_size(kernel, local_wg_range);
    auto buffer_in =
        iter % 2 == 0 ? buffer_mem1.as_const() : buffer_mem2.as_const();
    // Finish the reduction with the default kernel if the local_wg_range is not
    // suitable to subgroups anymore.
    if (sub_group_size <= 1) return fallback(buffer_in, reduce_size);
    cl::sycl::nd_range<2> nd_range_iter(kernel_range, local_wg_range);
    event = queue.submit([&](cl::sycl::handler& cgh) {
      auto& buffer_out = iter % 2 == 0 ? buffer_mem2 : buffer_mem1;
      auto in_acc = buffer_in.read_accessor(cgh);
      auto out_acc =
          (next_reduce_size == 1 ? output_mem : buffer_out).write_accessor(cgh);
      size_t in_size1 = in_acc.get_extent() / input_range[0];
      size_t out_size1 = out_acc.get_extent() / input_range[0];
      Kernel functor(in_acc, out_acc, sub_group_size, reduce_size, in_size1,
                     out_size1);
      cgh.parallel_for(kernel, nd_range_iter, functor);
    });
    ++iter;
  }
  if (SubgroupReducer<T, Index, Op>::RequireFinalize) {
    event = queue.submit([&](cl::sycl::handler& cgh) {
      auto out_acc = output_mem.read_write_accessor(cgh);
      ReduceFinalize<T, Index, Op> functor(out_acc, outer);
      cgh.parallel_for(cl::sycl::range<1>(out_acc.get_extent()), functor);
    });
  }
  return {event, StatusCode::OK};
}
#endif

}  // namespace internal
}  // namespace reduce
}  // namespace sycldnn
#endif  // SYCLDNN_SRC_REDUCE_QUEUE_REDUCTION_IMPL_H_
