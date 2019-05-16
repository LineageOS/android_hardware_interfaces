#
# Copyright (C) 2019 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# This test makes sure that executing STRIDED_SLICE results in a failure when
# the output dimensions do not match shrinkAxisMask.
#
# The test generator does not support generating tests resulting in execution
# failure, so the gTest part of this test has been written by hand.
# TODO(b/132155416): Move this under frameworks/ml/nn/runtime/test/specs/V1_2.
#
# Based on strided_slice_float_11.mod.py.

model = Model()
i1 = Input("input", "TENSOR_FLOAT32", "{2, 3}")
begins = Parameter("begins", "TENSOR_INT32", "{2}", [0, 0])
# The value "2" below makes the test invalid. See http://b/79856511#comment2.
ends = Parameter("ends", "TENSOR_INT32", "{2}", [2, 3])
strides = Parameter("strides", "TENSOR_INT32", "{2}", [1, 1])
beginMask = Int32Scalar("beginMask", 0)
endMask = Int32Scalar("endMask", 0)
shrinkAxisMask = Int32Scalar("shrinkAxisMask", 1)

output = Output("output", "TENSOR_FLOAT32", "{3}")

model = model.Operation("STRIDED_SLICE", i1, begins, ends, strides, beginMask, endMask, shrinkAxisMask).To(output)

Example({
    i1: [1, 2, 3, 4, 5, 6],
    output: [1, 2, 3],
})
