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

import android.hardware.common.NativeHandle;
import android.hardware.neuralnetworks.ErrorStatus;
import android.hardware.neuralnetworks.ExecutionResult;
import android.hardware.neuralnetworks.FencedExecutionResult;
import android.hardware.neuralnetworks.IBurst;
import android.hardware.neuralnetworks.Request;

/**
 * IPreparedModel describes a model that has been prepared for execution and is used to launch
 * executions.
 */
@VintfStability
interface IPreparedModel {
    /**
     * Each {@link OperationType::WHILE} operation in the model has an implicit execution timeout
     * duration associated with it ("loop timeout duration"). This duration is configurable on a
     * per-execution basis and must not exceed 15 seconds. The default value is 2 seconds. The units
     * are nanoseconds.
     */
    const long DEFAULT_LOOP_TIMEOUT_DURATION_NS = 2000000000;
    const long MAXIMUM_LOOP_TIMEOUT_DURATION_NS = 15000000000;

    /**
     * Performs a synchronous execution on a prepared model.
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
     * If the prepared model was prepared from a model wherein all tensor operands have fully
     * specified dimensions, and the inputs to the function are valid, and at execution time every
     * operation's input operands have legal values, then the execution should complete
     * successfully: there must be no failure unless the device itself is in a bad state.
     *
     * executeSynchronously may be called with an optional deadline. If the execution is not able to
     * be completed before the provided deadline, the execution may be aborted, and either
     * {@link ErrorStatus::MISSED_DEADLINE_TRANSIENT} or {@link
     * ErrorStatus::MISSED_DEADLINE_PERSISTENT} may be returned. The error due to an abort must be
     * sent the same way as other errors, described above.
     *
     * Any number of calls to the execute* functions, in any combination, may be made concurrently,
     * even on the same IPreparedModel object.
     *
     * @param request The input and output information on which the prepared model is to be
     *                executed.
     * @param measure Specifies whether or not to measure duration of the execution. The duration
     *                runs from the time the driver sees the call to the executeSynchronously
     *                function to the time the driver returns from the function.
     * @param deadlineNs The time by which the execution is expected to complete. The time is
     *                   measured in nanoseconds since boot (as from clock_gettime(CLOCK_BOOTTIME,
     *                   &ts) or ::android::base::boot_clock). If the execution cannot be finished
     *                   by the deadline, the execution may be aborted. Passing -1 means the
     *                   deadline is omitted. Other negative values are invalid.
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
    ExecutionResult executeSynchronously(in Request request, in boolean measureTiming,
            in long deadlineNs, in long loopTimeoutDurationNs);

    /**
     * Launch a fenced asynchronous execution on a prepared model.
     *
     * The execution is performed asynchronously with respect to the caller. executeFenced must
     * verify the inputs to the function are correct, and the usages of memory pools allocated by
     * IDevice::allocate are valid. If there is an error, executeFenced must immediately return a
     * service specific exception with the corresponding ErrorStatus. If the inputs to the function
     * are valid and there is no error, executeFenced must dispatch an asynchronous task to perform
     * the execution in the background, assign a sync fence that will be signaled once the execution
     * is completed and immediately return a callback that can be used by the client to query the
     * duration and runtime error status. If the task has finished before the call returns,
     * syncFence file descriptor may be set to -1. The execution must wait for all the sync fences
     * (if any) in waitFor to be signaled before starting the actual execution.
     *
     * When the asynchronous task has finished its execution, it must immediately signal the
     * syncFence returned from the executeFenced call. After the syncFence is signaled, the task
     * must not modify the content of any data object referenced by 'request' (described by the
     * {@link DataLocation} of a {@link RequestArgument}).
     *
     * executeFenced may be called with an optional deadline and an optional duration. If the
     * execution is not able to be completed before the provided deadline or within the timeout
     * duration (measured from when all sync fences in waitFor are signaled), whichever comes
     * earlier, the execution may be aborted, and either
     * {@link ErrorStatus::MISSED_DEADLINE_TRANSIENT} or {@link
     * ErrorStatus::MISSED_DEADLINE_PERSISTENT} may be returned. The error due to an abort must be
     * sent the same way as other errors, described above.
     *
     * If any of the sync fences in waitFor changes to error status after the executeFenced call
     * succeeds, or the execution is aborted because it cannot finish before the deadline has been
     * reached or the duration has elapsed, the driver must immediately set the returned syncFence
     * to error status.
     *
     * Any number of calls to the execute* functions, in any combination, may be made concurrently,
     * even on the same IPreparedModel object.
     *
     * @param request The input and output information on which the prepared model is to be
     *                executed. The outputs in the request must have fully specified dimensions.
     * @param waitFor A vector of sync fence file descriptors. Execution must not start until all
     *                sync fences have been signaled.
     * @param measure Specifies whether or not to measure duration of the execution.
     * @param deadlineNs The time by which the execution is expected to complete. The time is
     *                   measured in nanoseconds since boot (as from clock_gettime(CLOCK_BOOTTIME,
     *                   &ts) or ::android::base::boot_clock). If the execution cannot be finished
     *                   by the deadline, the execution may be aborted. Passing -1 means the
     *                   deadline is omitted. Other negative values are invalid.
     * @param loopTimeoutDurationNs The maximum amount of time in nanoseconds that should be spent
     *                              executing a {@link OperationType::WHILE} operation. If a loop
     *                              condition model does not output false within this duration, the
     *                              execution must be aborted. If -1 is provided, the maximum amount
     *                              of time is {@link DEFAULT_LOOP_TIMEOUT_DURATION_NS}. Other
     *                              negative values are invalid. When provided, the duration must
     *                              not exceed {@link MAXIMUM_LOOP_TIMEOUT_DURATION_NS}.
     * @param durationNs The length of time in nanoseconds within which the execution is expected to
     *                   complete after all sync fences in waitFor are signaled. If the execution
     *                   cannot be finished within the duration, the execution may be aborted.
     *                   Passing -1 means the duration is omitted. Other negative values are
     *                   invalid.
     * @return The FencedExecutionResult parcelable, containing IFencedExecutionCallback and the
     *         sync fence.
     * @throws ServiceSpecificException with one of the following ErrorStatus values:
     *     - DEVICE_UNAVAILABLE if driver is offline or busy
     *     - GENERAL_FAILURE if there is an unspecified error
     *     - INVALID_ARGUMENT if one of the input arguments is invalid, including fences in error
     *       states.
     *     - MISSED_DEADLINE_* if the execution is aborted because it cannot be completed by the
     *       deadline
     *     - RESOURCE_EXHAUSTED_* if the task was aborted by the driver
     */
    FencedExecutionResult executeFenced(in Request request, in ParcelFileDescriptor[] waitFor,
            in boolean measureTiming, in long deadlineNs, in long loopTimeoutDurationNs,
            in long durationNs);

    /**
     * Configure a Burst object used to execute multiple inferences on a prepared model in rapid
     * succession.
     *
     * If the prepared model was prepared from a model wherein all tensor operands have fully
     * specified dimensions, and a valid serialized Request is sent to the Burst for execution, and
     * at execution time every operation's input operands have legal values, then the execution
     * should complete successfully (ErrorStatus::NONE): There must be no failure unless the device
     * itself is in a bad state.
     *
     * @return burst Execution burst controller object.
     * @throws ServiceSpecificException with one of the following ErrorStatus values:
     *     - DEVICE_UNAVAILABLE if driver is offline or busy
     *     - GENERAL_FAILURE if there is an unspecified error
     *     - RESOURCE_EXHAUSTED_* if the task was aborted by the driver
     */
    IBurst configureExecutionBurst();
}
