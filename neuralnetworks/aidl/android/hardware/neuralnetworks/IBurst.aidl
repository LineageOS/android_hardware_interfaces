/*
 * Copyright (C) 2021 The Android Open Source Project
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

import android.hardware.neuralnetworks.ErrorStatus;
import android.hardware.neuralnetworks.ExecutionResult;
import android.hardware.neuralnetworks.Request;

/**
 * IBurst represents a burst execution object.
 *
 * Burst executions are a sequence of executions of the same prepared model that occur in rapid
 * succession, such as frames of a camera capture or successive audio samples. A burst object is
 * used to control a set of burst executions, and to preserve resources between executions, enabling
 * executions to have lower overhead. Burst objects enable some optimizations:
 * (1) A burst object is created before a sequence of executions, and freed when the sequence has
 *     ended. Because of this, the lifetime of the burst object hints to a driver how long it should
 *     remain in a high performance state.
 * (2) A burst object can preserve resources between executions. For example, a driver can map a
 *     memory object on the first execution and cache the mapping in the burst object for reuse in
 *     subsequent executions. Any cached resource can be released when the burst object is destroyed
 *     or when the NNAPI runtime notifies the burst object that the resource is no longer required.
 * (3) A burst object may be used for at most one execution at a time. This enables any transient
 *     execution resources such as intermediate tensors to be allocated once when the burst object
 *     is created and freed when the burst object is destroyed.
 */
@VintfStability
interface IBurst {
    /**
     * Performs a synchronous execution on a burst object.
     *
     * The execution is performed synchronously with respect to the caller. executeSynchronously
     * must verify the inputs to the function are correct, and the usages of memory pools allocated
     * by IDevice::allocate are valid. If there is an error, executeSynchronously must immediately
     * return a service specific exception with the appropriate ErrorStatus value. If the inputs to
     * the function are valid and there is no error, executeSynchronously must perform the
     * execution, and must not return until the execution is complete.
     *
     * The caller must not change the content of any data object referenced by 'request' (described
     * by the {@link DataLocation} of a {@link RequestArgument}) until executeSynchronously returns.
     * executeSynchronously must not change the content of any of the data objects corresponding to
     * 'request' inputs.
     *
     * If the burst object was configured from a prepared model wherein all tensor operands have
     * fully specified dimensions, and the inputs to the function are valid, and at execution time
     * every operation's input operands have legal values, then the execution should complete
     * successfully: there must be no failure unless the device itself is in a bad state.
     *
     * executeSynchronously may be called with an optional deadline. If the execution is not able to
     * be completed before the provided deadline, the execution may be aborted, and either
     * {@link ErrorStatus::MISSED_DEADLINE_TRANSIENT} or {@link
     * ErrorStatus::MISSED_DEADLINE_PERSISTENT} may be returned. The error due to an abort must be
     * sent the same way as other errors, described above.
     *
     * Only a single execution on a given burst object may be active at any time.
     *
     * @param request The input and output information on which the prepared model is to be
     *                executed.
     * @param memoryIdentifierTokens A list of tokens where each token is a non-negative number
     *                               that uniquely identifies a memory object. Each memory
     *                               identifier token corresponds to an element of request.pools. A
     *                               value of -1 indicates no identity.
     * @param measure Specifies whether or not to measure duration of the execution. The duration
     *                runs from the time the driver sees the call to the executeSynchronously
     *                function to the time the driver returns from the function.
     * @param deadlineNs The time by which the execution is expected to complete. The time is
     *                   measured in nanoseconds since epoch of the steady clock (as from
     *                   std::chrono::steady_clock). If the execution cannot be finished by the
     *                   deadline, the execution may be aborted. Passing -1 means the deadline is
     *                   omitted. Other negative values are invalid.
     * @param loopTimeoutDurationNs The maximum amount of time in nanoseconds that should be spent
     *                              executing a {@link OperationType::WHILE} operation. If a loop
     *                              condition model does not output false within this duration, the
     *                              execution must be aborted. If -1 is provided, the maximum amount
     *                              of time is {@link DEFAULT_LOOP_TIMEOUT_DURATION_NS}. Other
     *                              negative values are invalid. When provided, the duration must
     *                              not exceed {@link MAXIMUM_LOOP_TIMEOUT_DURATION_NS}.
     * @return ExecutionResult parcelable, containing the status of the execution, output shapes and
     *     timing information.
     * @throws ServiceSpecificException with one of the following ErrorStatus values:
     *     - DEVICE_UNAVAILABLE if driver is offline or busy
     *     - GENERAL_FAILURE if there is an unspecified error
     *     - INVALID_ARGUMENT if one of the input arguments is invalid
     *     - MISSED_DEADLINE_* if the execution is aborted because it cannot be completed by the
     *       deadline
     *     - RESOURCE_EXHAUSTED_* if the task was aborted by the driver
     */
    ExecutionResult executeSynchronously(in Request request, in long[] memoryIdentifierTokens,
            in boolean measureTiming, in long deadlineNs, in long loopTimeoutDurationNs);

    /**
     * releaseMemoryResource is used by the client to signal to the service that a memory buffer
     * corresponding to a slot number is no longer needed by the client, and any cached resources
     * associated with that memory object may be released.
     *
     * The identifier tokens are unique to the burst object.
     *
     * @param memoryIdentifierToken Value uniquely identifying a memory object that is no longer
     *                              used.
     * @throws ServiceSpecificException with one of the following ErrorStatus values:
     *     - DEVICE_UNAVAILABLE if driver is offline or busy
     *     - GENERAL_FAILURE if there is an unspecified error
     *     - INVALID_ARGUMENT if one of the input arguments is invalid
     */
    void releaseMemoryResource(in long memoryIdentifierToken);
}
