#!python
#
# Copyright 2018 Codeplay Software Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use these files except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#
# Automatically generate the pooling test cases using TensorFlow to provide
# the expected values.

from __future__ import print_function

try:
    # With python3 `zip` returns an iterator, however with python2, use
    # `itertools.izip` instead
    import itertools.izip as zip
except ImportError:
    pass

import itertools
import os
from collections import namedtuple

import tensorflow as tf
import numpy as np

import helpers

WINDOW_LIST = [1, 3, 3, 5, 5, 7, 7, 11, 11]
STRIDE_LIST = [1, 1, 2, 1, 2, 1, 4,  1,  4]
BATCHES = [1, 3]
CHANNELS = [1, 2, 4]
PADDING_VALUES = ["SAME", "VALID"]
TEST_TYPES = ["max", "avg"]

INCLUDES = r"""
#include <gtest/gtest.h>

#include "sycldnn/pooling/launch.h"
#include "sycldnn/pooling/operators.h"
#include "sycldnn/pooling/params.h"

#include "test/pooling/pooling_fixture.h"
#include "test/types/kernel_data_types.h"

#include <CL/sycl.hpp>

#include <vector>"""
TYPED_TEST_CASE_DECL_TPL = r"""
using namespace sycldnn;
template <typename DataType>
using {test_case} = PoolingFixture<DataType>;
TYPED_TEST_CASE({test_case}, types::GTestKernelDataTypes);"""

TestCaseParams = namedtuple('TestCaseParams',
                            ['test_type', 'window', 'stride'])
TestParams = namedtuple('TestParams', TestCaseParams._fields +
                        ('in_shape', 'padding'))

def get_pool_results(max_val, pool_op, input_shape, window_shape,
                     stride_shape, padding):
    """
    Construct and run a Tensorflow graph to compute pooling.

    Will create an input tensor of the required size filled with values 1, 2,
    3... and use these to compute the pooling.
    Returns the computed values in a numpy array.
    """
    with tf.Graph().as_default():
        total_inp_size = np.product(input_shape)

        input_vals = helpers.get_tensor_data(total_inp_size, max_val)

        inp_tensor = tf.constant(
            input_vals, shape=input_shape, dtype=np.float64)
        output = tf.nn.pool(
            inp_tensor,
            window_shape=window_shape,
            pooling_type=pool_op.upper(),
            strides=stride_shape,
            padding=padding,
            data_format="NHWC")

        with tf.Session() as sess:
            init = tf.global_variables_initializer()
            sess.run(init)
            sess.graph.finalize()
            return sess.run(output)

def get_result_and_size(test_params):
    """
    Get the result of the specified convolution and max input value.

    Ensures that the resulting values are less than the REQUIRED_MAX, and if
    not will adjust the maximum value to allow in the input tensors.
    """
    window_shape = [test_params.window, test_params.window]
    stride_shape = [test_params.stride, test_params.stride]
    return helpers.get_result_and_size(
        get_pool_results,
        pool_op=test_params.test_type,
        input_shape=test_params.in_shape,
        window_shape=window_shape,
        stride_shape=stride_shape,
        padding=test_params.padding)

TEST_CASE_TPL = "{test_type}Window{window}Stride{stride}"
TEST_NAME_TPL = "{padding}{in_s[0]}x{in_s[1]}x{in_s[2]}x{in_s[3]}"
IN_SHAPE_INIT_TPL = "{{{{ {0[0]}, {0[1]}, {0[2]}, {0[3]} }}}}"

operator_map = { 'max' : 'pooling::Max',
                 'avg' : 'pooling::Average',
               }

def get_test_lines(test_params):
    """
    Create a list of strings corresponding to the lines in a single test case.

    Uses TensorFlow to compute the expected results for the given parameters,
    and provides the code to call the test fixture to run the test.
    """
    output, max_input_val = get_result_and_size(test_params)
    camel_case_type = helpers.to_camel_case(test_params.test_type)
    test_case = TEST_CASE_TPL.format(
        test_type=camel_case_type,
        window=test_params.window,
        stride=test_params.stride)
    test_name = TEST_NAME_TPL.format(
        padding=test_params.padding,
        in_s=test_params.in_shape)
    in_shape_init = IN_SHAPE_INIT_TPL.format(test_params.in_shape)
    test_lines = [
        "TYPED_TEST({}, {}) {{".format(test_case, test_name),
        "  using DataType = typename TestFixture::DataType;",
        "  const std::vector<DataType> exp_out = {};".format(
            helpers.format_tensor(output)),
        "  const std::array<int, 4> in_shape = {};".format(in_shape_init),
        "  const auto padding = PaddingMode::{};".format(
            test_params.padding),
        "  const auto params = getPoolingParams<{}, {}>(in_shape, padding);".format(
            test_params.window, test_params.stride),
        "  const DataType max_input_val = {:.1f};".format(max_input_val),
        "  this->template test_pool<pooling::Forward, {}>(".format(
            operator_map[test_params.test_type]),
        "       exp_out, params, max_input_val);"
        "}"
    ]
    return test_lines

def get_input_sizes(test_case):
    """
    Want to test with sizes that are:
        a) Divisible by 4
        b) Divisible by 2 but not 4
        c) Not Divisible by 2
    And we also require the sizes to be large enough that there are at least
    two entries in the output tensor, so the minimum size is (window + stride)
    and the other sizes need to be calculated to ensure that the above criteria
    are satisfied.
    """
    start = test_case.window + test_case.stride
    if start % 2 == 1:
        return [start, start + 1, start + 3]
    else:
        return [start, start + 1, start + 2]

def test_params_for_test_case(test_case):
    "Test params generator for all different tests in a given test case."
    in_sizes = get_input_sizes(test_case)
    for in_shape in itertools.product(BATCHES, in_sizes, in_sizes, CHANNELS):
        for padding in PADDING_VALUES:
            yield TestParams(
                test_type=test_case.test_type,
                window=test_case.window,
                stride=test_case.stride,
                in_shape=in_shape,
                padding=padding)

def output_for_test_case(test_case):
    """
    Create a list of strings corresponding to separate lines in the full test
    case. The output contains headers, includes, setup and all the tests for
    the test case.
    """
    scriptname = os.path.basename(__file__)
    camel_case_type = helpers.to_camel_case(test_case.test_type)
    test_case_name = TEST_CASE_TPL.format(
        test_type=camel_case_type,
        window=test_case.window,
        stride=test_case.stride)
    output = [
        helpers.get_license(),
        helpers.get_dont_modify_comment(scriptname=scriptname),
        INCLUDES,
        TYPED_TEST_CASE_DECL_TPL.format(test_case=test_case_name)
    ]

    for test_params in test_params_for_test_case(test_case):
        output.extend(get_test_lines(test_params))
    output.append("\n")
    return output

FILENAME_TPL = "pooling/{test_type}_window{window}_stride{stride}.cc"

def get_test_case_filename(test_case):
    "Get filename for test case."
    return FILENAME_TPL.format(
        test_type=test_case.test_type,
        window=test_case.window,
        stride=test_case.stride)

def test_cases():
    "Test case generator giving all possible test cases."
    for window, stride in zip(WINDOW_LIST, STRIDE_LIST):
        for test_type in TEST_TYPES:
            if test_type == "avg" and (window == 11 or (window == 7 and stride == 4)):
                continue
            yield TestCaseParams(test_type=test_type, window=window, stride=stride)

def generate_pooling_tests():
    np.set_printoptions(
        suppress=True, precision=10, threshold=1000000, linewidth=1000000)
    test_dir = helpers.get_test_directory()
    os.chdir(test_dir)
    for test_case in test_cases():
        filename = get_test_case_filename(test_case)
        output = output_for_test_case(test_case)
        with open(filename, 'w') as f:
            f.write('\n'.join(output))
        print("File '{}' written".format(filename))

if __name__ == "__main__":
    generate_pooling_tests()