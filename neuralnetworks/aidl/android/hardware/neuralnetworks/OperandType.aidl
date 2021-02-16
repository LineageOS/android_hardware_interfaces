/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.neuralnetworks;

/**
 * Operand types.
 *
 * The type of an operand in a model.
 *
 * Types prefaced with TENSOR_* must be used for tensor data (i.e., tensors
 * with at least one dimension). Types not prefaced by TENSOR_* represent
 * scalar values and must have no dimensions.
 */
@VintfStability
@Backing(type="int")
enum OperandType {
    /**
     * A 32 bit floating point scalar value.
     */
    FLOAT32 = 0,
    /**
     * A signed 32 bit integer scalar value.
     */
    INT32 = 1,
    /**
     * An unsigned 32 bit integer scalar value.
     */
    UINT32 = 2,
    /**
     * A tensor of 32 bit floating point values.
     */
    TENSOR_FLOAT32 = 3,
    /**
     * A tensor of 32 bit integer values.
     */
    TENSOR_INT32 = 4,
    /**
     * A tensor of 8 bit unsigned integers that represent real numbers.
     *
     * Attached to this tensor are two numbers that can be used to convert the 8 bit integer to the
     * real value and vice versa. These two numbers are:
     * - scale: a 32 bit floating point value greater than zero.
     * - zeroPoint: a 32 bit integer, in range [0, 255].
     *
     * The formula is:
     *   real_value = (integer_value - zeroPoint) * scale.
     */
    TENSOR_QUANT8_ASYMM = 5,
    /**
     * An 8 bit boolean scalar value.
     *
     * Values of this operand type are either true or false. A zero value represents false; any
     * other value represents true.
     */
    BOOL = 6,
    /**
     * A tensor of 16 bit signed integers that represent real numbers.
     *
     * Attached to this tensor is a number representing real value scale that is used to convert the
     * 16 bit number to a real value in the following way:
     * realValue = integerValue * scale.
     *
     * scale is a 32 bit floating point with value greater than zero.
     */
    TENSOR_QUANT16_SYMM = 7,
    /**
     * A tensor of IEEE 754 16 bit floating point values.
     */
    TENSOR_FLOAT16 = 8,
    /**
     * A tensor of 8 bit boolean values.
     *
     * Values of this operand type are either true or false. A zero value represents false; any
     * other value represents true.
     */
    TENSOR_BOOL8 = 9,
    /**
     * An IEEE 754 16 bit floating point scalar value.
     */
    FLOAT16 = 10,
    /**
     * A tensor of 8 bit signed integers that represent real numbers.
     *
     * This tensor is associated with additional fields that can be used to convert the 8 bit signed
     * integer to the real value and vice versa. These fields are:
     * - channelDim: a 32 bit unsigned integer indicating channel dimension.
     * - scales: an array of positive 32 bit floating point values.
     * The size of the scales array must be equal to dimensions[channelDim].
     *
     * {@link SymmPerChannelQuantParams} must hold the parameters for an Operand of this type.
     * The channel dimension of this tensor must not be unknown (dimensions[channelDim] != 0).
     *
     * The formula is:
     * realValue[..., C, ...] =
     *     integerValue[..., C, ...] * scales[C]
     * where C is an index in the Channel dimension.
     */
    TENSOR_QUANT8_SYMM_PER_CHANNEL = 11,
    /**
     * A tensor of 16 bit unsigned integers that represent real numbers.
     *
     * Attached to this tensor are two numbers that can be used to convert the 16 bit integer to the
     * real value and vice versa. These two numbers are:
     * - scale: a 32 bit floating point value greater than zero.
     * - zeroPoint: a 32 bit integer, in range [0, 65535].
     *
     * The formula is:
     * real_value = (integer_value - zeroPoint) * scale.
     */
    TENSOR_QUANT16_ASYMM = 12,
    /**
     * A tensor of 8 bit signed integers that represent real numbers.
     *
     * Attached to this tensor is a number representing real value scale that is used to convert the
     * 8 bit number to a real value in the following way:
     * realValue = integerValue * scale.
     *
     * scale is a 32 bit floating point with value greater than zero.
     */
    TENSOR_QUANT8_SYMM = 13,
    /**
     * A tensor of 8 bit signed integers that represent real numbers.
     *
     * Attached to this tensor are two numbers that can be used to convert the 8 bit integer to the
     * real value and vice versa. These two numbers are:
     * - scale: a 32 bit floating point value greater than zero.
     * - zeroPoint: a 32 bit integer, in range [-128, 127].
     *
     * The formula is:
     * real_value = (integer_value - zeroPoint) * scale.
     */
    TENSOR_QUANT8_ASYMM_SIGNED = 14,
    /**
     * A reference to a subgraph.
     *
     * Must have the lifetime {@link OperandLifeTime::SUBGRAPH}.
     */
    SUBGRAPH = 15,
}
