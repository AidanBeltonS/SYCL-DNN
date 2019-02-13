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
#ifndef SYCLDNN_SRC_BACKEND_EIGEN_BACKEND_PROVIDER_H_
#define SYCLDNN_SRC_BACKEND_EIGEN_BACKEND_PROVIDER_H_

// TODO(jwlawson): remove cassert when no longer needed before Eigen include
#include <cassert>

#include <unsupported/Eigen/CXX11/Tensor>
#include "sycldnn/backend/eigen_backend.h"

#include "src/backend/backend_provider.h"

namespace sycldnn {
namespace backend {

/**
 * Specialisation of the backend provider using Eigen.
 *
 * Provides access to sycldnn::backend::BackendProvider using Eigen and its
 * helper methods to allocate and deallocate memory.
 */
template <>
struct BackendProvider<EigenBackend> {
 public:
  BackendProvider() : backend_{get_eigen_device()} {}

  template <typename T>
  using Pointer = EigenBackend::pointer_type<T>;

  /** Return this backend. */
  EigenBackend& get_backend() { return backend_; }
  /** Allocate memory on the device and initialise it with the provided data. */
  template <typename T>
  Pointer<T> get_initialised_device_memory(size_t size,
                                           std::vector<T> const& data) {
    auto device = get_eigen_device();
    size_t n_bytes = size * sizeof(T);
    auto* gpu_ptr = static_cast<T*>(device.allocate(n_bytes));
    device.memcpyHostToDevice(gpu_ptr, data.data(), n_bytes);
    return gpu_ptr;
  }
  /** Copy the device memory into the provided host vector. */
  template <typename T>
  void copy_device_data_to_host(size_t size, Pointer<T> gpu_ptr,
                                std::vector<T>& host_data) {
    host_data.resize(size);
    auto device = get_eigen_device();
    size_t n_bytes = size * sizeof(T);
    device.memcpyDeviceToHost(host_data.data(), gpu_ptr, n_bytes);
  }
  /** Deallocate a device pointer. */
  template <typename T>
  void deallocate_ptr(Pointer<T> ptr) {
    auto device = get_eigen_device();
    device.deallocate(ptr);
  }
  /** Returns the selected device that Eigen executes on. */
  Eigen::SyclDevice& get_eigen_device() {
    // By making the Eigen device static any compiled kernels will be cached,
    // and so do not need to be recompiled for each test.
    static Eigen::QueueInterface queue_interface{cl::sycl::default_selector{}};
    static Eigen::SyclDevice device{&queue_interface};
    return device;
  }

 private:
  EigenBackend backend_;
};

}  // namespace backend
}  // namespace sycldnn

#endif  // SYCLDNN_SRC_BACKEND_EIGEN_BACKEND_PROVIDER_H_