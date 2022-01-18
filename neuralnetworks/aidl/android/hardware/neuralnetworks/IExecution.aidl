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

import android.hardware.neuralnetworks.ExecutionResult;
import android.hardware.neuralnetworks.FencedExecutionResult;

/**
 * IExecution represents a reusable execution object with request and most other execution
 * properties fixed. It is used to launch executions.
 *
 * At most one execution may occur on a reusable execution object at any given time, either by
 * means of executeSynchronously or executeFenced.
 *
 * An IExecution object is used to control a set of executions on the same prepared model with
 * the same request and properties. IExecution objects enable some optimizations:
 * (1) An IExecution object can preserve resources between executions. For example, a driver can
 *     map a memory object when the IExecution object is created and cache the mapping for reuse in
 *     subsequent executions. Any cached resource can be released when the IExecution object is
 *     destroyed.
 * (2) Because an IExecution object may be used for at most one execution at a time, any transient
 *     execution resources such as intermediate tensors can be allocated once when the IExecution
 *     object is created and freed when the IExecution object is destroyed.
 * (3) An IExecution object is created for a fixed request. This enables the implementation to apply
 *     request-specific optimizations. For example, an implementation can avoid request validation
 *     and conversions when the IExecution object is reused. An implementation may also choose to
 *     specialize the dynamic tensor shapes in the IExecution object according to the request.
 */
@VintfStability
interface IExecution {
    /**
     * Performs a synchronous execution on the reusable execution object.
     *
     * The execution is performed synchronously with respect to the caller. executeSynchronously
     * must verify the inputs to the function are correct, and the usages of memory pools allocated
     * by IDevice::allocate are valid. If there is an error, executeSynchronously must immediately
     * return a service specific exception with the appropriate ErrorStatus value. If the inputs to
     * the function are valid and there is no error, executeSynchronously must perform the
     * execution, and must not return until the execution is complete.
     *
     * The caller must not change the content of any data object referenced by the 'request'
     * provided in {@link IPreparedModel::createReusableExecution} (described by the
     * {@link DataLocation} of a {@link RequestArgument}) until executeSynchronously returns.
     * executeSynchronously must not change the content of any of the data objects corresponding to
     * 'request' inputs.
     *
     * If the execution object was configured from a prepared model wherein all tensor operands have
     * fully specified dimensions, and the inputs to the function are valid, and at execution time
     * every operation's input operands have legal values, then the execution should complete
     * successfully: there must be no failure unless the device itself is in a bad state.
     *
     * If the execution object was created with measureTiming being true and the execution is
     * successful, the driver may report the timing information in the returning
     * {@link ExecutionResult}. The duration runs from the time the driver sees the call to the time
     * the driver returns from the function.
     *
     * executeSynchronously may be called with an optional deadline. If the execution is not able to
     * be completed before the provided deadline, the execution may be aborted, and either
     * {@link ErrorStatus::MISSED_DEADLINE_TRANSIENT} or {@link
     * ErrorStatus::MISSED_DEADLINE_PERSISTENT} may be returned. The error due to an abort must be
     * sent the same way as other errors, described above.
     *
     * @param deadlineNs The time by which the execution is expected to complete. The time is
     *                   measured in nanoseconds since boot (as from clock_gettime(CLOCK_BOOTTIME,
     *                   &ts) or ::android::base::boot_clock). If the execution cannot be finished
     *                   by the deadline, the execution may be aborted. Passing -1 means the
     *                   deadline is omitted. Other negative values are invalid.
     * @return ExecutionResult parcelable, containing the status of the execution, output shapes
     *     and timing information.
     * @throws ServiceSpecificException with one of the following ErrorStatus values:
     *     - DEVICE_UNAVAILABLE if driver is offline or busy
     *     - GENERAL_FAILURE if there is an unspecified error
     *     - INVALID_ARGUMENT if one of the input arguments is invalid
     *     - MISSED_DEADLINE_* if the execution is aborted because it cannot be completed by the
     *       deadline
     *     - RESOURCE_EXHAUSTED_* if the task was aborted by the driver
     */
    ExecutionResult executeSynchronously(in long deadlineNs);

    /**
     * Launch a fenced asynchronous execution on the reusable execution object.
     *
     * The execution is performed asynchronously with respect to the caller. executeFenced must
     * verify the inputs to the function are correct, and the usages of memory pools allocated by
     * IDevice::allocate are valid. If there is an error, executeFenced must immediately return a
     * service specific exception with the corresponding ErrorStatus. If the inputs to the function
     * are valid and there is no error, executeFenced must dispatch an asynchronous task to perform
     * the execution in the background, and immediately return a {@link FencedExecutionResult}
     * containing two fields: a callback (which can be used by the client to query the duration and
     * runtime error status) and a sync fence (which will be signaled once the execution is
     * completed). If the task has finished before the call returns, syncFence file descriptor may
     * be set to -1. The execution must wait for all the sync fences (if any) in waitFor to be
     * signaled before starting the actual execution.
     *
     * When the asynchronous task has finished its execution, it must immediately signal the
     * syncFence returned from the executeFenced call. After the syncFence is signaled, the task
     * must not modify the content of any data object referenced by the 'request' provided in
     * IPreparedModel::createReusableExecution (described by the {@link DataLocation} of a
     * {@link RequestArgument}).
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
     * @param waitFor A vector of sync fence file descriptors. Execution must not start until all
     *                sync fences have been signaled.
     * @param deadlineNs The time by which the execution is expected to complete. The time is
     *                   measured in nanoseconds since boot (as from clock_gettime(CLOCK_BOOTTIME,
     *                   &ts) or ::android::base::boot_clock). If the execution cannot be finished
     *                   by the deadline, the execution may be aborted. Passing -1 means the
     *                   deadline is omitted. Other negative values are invalid.
     * @param durationNs The length of time in nanoseconds within which the execution is expected
     *                   to complete after all sync fences in waitFor are signaled. If the
     *                   execution cannot be finished within the duration, the execution may be
     *                   aborted. Passing -1 means the duration is omitted. Other negative values
     *                   are invalid.
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
    FencedExecutionResult executeFenced(
            in ParcelFileDescriptor[] waitFor, in long deadlineNs, in long durationNs);
}
