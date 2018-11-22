/*
 * Copyright 2019 Codeplay Software Ltd
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
#ifndef SYCLDNN_INCLUDE_BACKEND_SNN_BACKEND_H_
#define SYCLDNN_INCLUDE_BACKEND_SNN_BACKEND_H_

#include "sycldnn/backend/device_mem_pointer.h"
#include "sycldnn/backend/snn_matmul_provider.h"

namespace sycldnn {
namespace backend {

// Forward declaration to allow the BackendTraits specialisation.
struct SNNBackend;

/**
 * The template specialisation of \ref
 * sycldnn::backend::BackendTraits<SNNBackend>.
 *
 * Provides the pointer types for the SNNBackend.
 */
template <>
struct BackendTraits<SNNBackend> {
  /**
   * The external pointer type for SNNBackend.
   */
  template <typename T>
  using pointer_type = DeviceMemPointer<T>;

  /**
   * The internal pointer type for SNNBackend.
   */
  template <typename T>
  using internal_pointer_type = DeviceMemPointer<T>;
};

/**
 * Standard test backend for SYCL-DNN.
 *
 * Provides pointer handling and matrix multiplies using our internal kernels.
 */
struct SNNBackend final : public SNNMatmulProvider<SNNBackend> {
  /** The pointer type used in interface of the SNNBackend. */
  template <typename T>
  using pointer_type =
      typename BackendTraits<SNNBackend>::template pointer_type<T>;

  /** The internal pointer type used internally by the SNNBackend. */
  template <typename T>
  using internal_pointer_type =
      typename BackendTraits<SNNBackend>::template internal_pointer_type<T>;

  /**
   * Construct an SNNBackend with the given queue. All SYCL-DNN operations
   * launched with this backend will be submitted to this queue.
   *
   * \param queue The SYCL queue to use with this backend.
   */
  SNNBackend(cl::sycl::queue queue) : queue_{std::move(queue)} {}

  /**
   * Allocate a tensor to be used internally.
   * \param n_bytes The size of the allocation in bytes.
   * \return Returns a pointer to allocation, using the internal pointer
   *         representation.
   * */
  template <typename T>
  internal_pointer_type<T> allocate(size_t n_bytes) {
    return internal_pointer_type<T>{n_bytes / sizeof(T)};
  }

  /**
   * Deallocate an internal tensor.
   * \param ptr A pointer to the allocation to deallocate.
   * \return void
   */
  template <typename T>
  void deallocate(internal_pointer_type<T> ptr) {
    SNN_UNUSED_VAR(ptr)
  }

  /**
   * Get a SYCL buffer from an external pointer.
   * \param ptr The pointer for which to retrieve the corresponding SYCL buffer.
   * \param n_elems The number of elements in the buffer.
   * \return Returns a SYCL buffer corresponding to ptr.
   */
  template <typename T>
  auto get_buffer(pointer_type<T> ptr, size_t n_elems)
      -> decltype(ptr.get_buffer()) {
    SNN_UNUSED_VAR(n_elems);
    return ptr.get_buffer();
  }

  /**
   * Get a SYCL buffer from an internal pointer.
   * \param ptr The pointer for which to retrieve the corresponding SYCL buffer.
   * \param n_elems The number of elements in the buffer.
   * \return Returns a SYCL buffer corresponding to ptr.
   */
  template <typename T>
  auto get_buffer_internal(internal_pointer_type<T> ptr, size_t n_elems)
      -> decltype(ptr.get_buffer()) {
    SNN_UNUSED_VAR(n_elems);
    return ptr.get_buffer();
  }

  /**
   * Get the offset from an external pointer. An external pointer may be an
   * offset from some base address, where the base address corresponds to a
   * SYCL buffer, and the offset refers to some address internal to the SYCL
   * buffer. This function enables querying such an offset.
   * \param ptr The external pointer to query the offset for.
   * \return Returns the offset from the buffer base address, in elements.
   */
  template <typename T>
  size_t get_offset(pointer_type<T> ptr) {
    return ptr.get_offset();
  }

  /**
   * Get the offset from an internal pointer. An internal pointer may be an
   * offset from some base address, where the base address corresponds to a
   * SYCL buffer, and the offset refers to some address internal to the SYCL
   * buffer. This function enables querying such an offset.
   * \param ptr The internal pointer to query the offset for.
   * \return Returns the offset from the buffer base address, in elements.
   */
  template <typename T>
  size_t get_offset_internal(internal_pointer_type<T> ptr) {
    return ptr.get_offset();
  }

  /**
   * Maps from external to internal pointer representations. This is a no-op for
   * the SNN backend.
   * \param ptr The external pointer to transform to the corresponding internal
   *            pointer representation.
   * \return Returns an internal pointer representation compatible with \ref
   *         sycldnn::backend::SNNBackend.
   */
  template <typename T>
  internal_pointer_type<T> to_internal_pointer(pointer_type<T> ptr) {
    return ptr;
  }

  /**
   * Release the internal pointer, which has previously been returned from \ref
   * sycldnn::backend::SNNBackend::to_internal_pointer.
   *
   * In this case it is a no-op.
   *
   * \param ptr The internal pointer to release.
   */
  template <typename T>
  void release_internal_pointer(internal_pointer_type<T> ptr) {
    SNN_UNUSED_VAR(ptr);
  }

  /**
   * Gets the SYCL queue that the backend is bound to.
   * \return Returns the SYCL queue that the backend is bound to.
   */
  cl::sycl::queue& get_queue() { return queue_; }

  /**
   * Gets a descriptive name for this backend.
   * \return a descriptive name for this backend.
   */
  char const* name() { return "SNNBackend"; }

 private:
  cl::sycl::queue queue_;
};

}  // namespace backend
}  // namespace sycldnn

#endif  // SYCLDNN_INCLUDE_BACKEND_SNN_BACKEND_H_