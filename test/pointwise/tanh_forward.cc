/*
 * Copyright 2018 Codeplay Software Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use these files except in compliance with the License.
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

// DO NOT MODIFY BY HAND
// This file was automatically generated by generate_pointwise_tests.py.
// Results calculated using Tensorflow v1.12.0.

#include <gtest/gtest.h>

#include "sycldnn/pointwise/direction.h"
#include "sycldnn/pointwise/launch.h"
#include "sycldnn/pointwise/operators.h"

#include "test/pointwise/pointwise_fixture.h"
#include "test/types/kernel_data_types.h"

#include <CL/sycl.hpp>

#include <vector>

using namespace sycldnn;
template <typename DataType>
using TanhForward =
    PointwiseFixture<DataType, pointwise::Tanh, pointwise::Forward>;
TYPED_TEST_CASE(TanhForward, types::GTestKernelDataTypes);
TYPED_TEST(TanhForward, Shape_8x1) {
  using DataType = typename TestFixture::DataType;
  const std::vector<DataType> exp_out = {-0.999329299739067,
                                         -0.9950547536867305,
                                         -0.9640275800758169,
                                         -0.7615941559557649,
                                         0.,
                                         0.7615941559557649,
                                         0.9640275800758169,
                                         0.9950547536867305};
  this->test_pointwise(exp_out);
}
TYPED_TEST(TanhForward, Shape_9x1) {
  using DataType = typename TestFixture::DataType;
  const std::vector<DataType> exp_out = {
      -0.9999092042625951, -0.999329299739067,  -0.9950547536867305,
      -0.9640275800758169, -0.7615941559557649, 0.,
      0.7615941559557649,  0.9640275800758169,  0.9950547536867305};
  this->test_pointwise(exp_out);
}
TYPED_TEST(TanhForward, Shape_10x1) {
  using DataType = typename TestFixture::DataType;
  const std::vector<DataType> exp_out = {
      -0.9999092042625951, -0.999329299739067,  -0.9950547536867305,
      -0.9640275800758169, -0.7615941559557649, 0.,
      0.7615941559557649,  0.9640275800758169,  0.9950547536867305,
      0.999329299739067};
  this->test_pointwise(exp_out);
}