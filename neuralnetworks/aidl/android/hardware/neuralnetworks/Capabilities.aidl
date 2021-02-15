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

import android.hardware.neuralnetworks.OperandPerformance;
import android.hardware.neuralnetworks.PerformanceInfo;

/**
 * The capabilities of a driver.
 *
 * This represents performance of non-extension operations.
 *
 * Performance of an operation other than {@link OperationType::IF} and {@link OperationType::WHILE}
 * comes from the type of its first operand.
 */
@VintfStability
parcelable Capabilities {
    /**
     * Driver performance when operating on float32 data but performing calculations with range
     * and/or precision as low as that of the IEEE 754 16-bit floating-point format.
     */
    PerformanceInfo relaxedFloat32toFloat16PerformanceScalar;
    PerformanceInfo relaxedFloat32toFloat16PerformanceTensor;
    /**
     * Performance by operand type. Must be sorted by OperandType.
     *
     * If a particular {@link OperandType} is not present in operandPerformance, its performance is
     * treated as { .execTime = FLT_MAX, .powerUsage = FLT_MAX }.
     *
     * Performance does not apply to {@link OperandType::SUBGRAPH}, and a driver must not report
     * operand performance for {@link OperandType::SUBGRAPH}.
     */
    OperandPerformance[] operandPerformance;
    /**
     * Performance of an {@link OperationType::IF} operation is the sum of
     * {@link Capabilities::ifPerformance} and the mean of performance for the two branch subgraphs,
     * where performance for a subgraph is the sum of the performance of all operations within the
     * subgraph.
     */
    PerformanceInfo ifPerformance;
    /**
     * Performance of a {@link OperationType::WHILE} operation is the sum of
     * {@link Capabilities::whilePerformance}, performance for the condition subgraph and
     * performance for the body subgraph, where performance for a subgraph is the sum of the
     * performance of all operations within the subgraph.
     */
    PerformanceInfo whilePerformance;
}
