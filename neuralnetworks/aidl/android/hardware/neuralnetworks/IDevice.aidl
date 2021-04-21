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

import android.hardware.neuralnetworks.BufferDesc;
import android.hardware.neuralnetworks.BufferRole;
import android.hardware.neuralnetworks.Capabilities;
import android.hardware.neuralnetworks.DeviceBuffer;
import android.hardware.neuralnetworks.DeviceType;
import android.hardware.neuralnetworks.ExecutionPreference;
import android.hardware.neuralnetworks.Extension;
import android.hardware.neuralnetworks.IPreparedModel;
import android.hardware.neuralnetworks.IPreparedModelCallback;
import android.hardware.neuralnetworks.IPreparedModelParcel;
import android.hardware.neuralnetworks.Model;
import android.hardware.neuralnetworks.NumberOfCacheFiles;
import android.hardware.neuralnetworks.Priority;

/**
 * This interface represents a device driver.
 */
@VintfStability
interface IDevice {
    /**
     * The byte size of the cache token.
     */
    const int BYTE_SIZE_OF_CACHE_TOKEN = 32;
    /**
     * The maximum number of files for each type of cache in compilation caching.
     */
    const int MAX_NUMBER_OF_CACHE_FILES = 32;

    /**
     * Numeric values of extension operand and operation types have the following structure:
     * - The sign bit is always 0.
     * - 15 high bits represent the "prefix", which corresponds uniquely to the extension name.
     * - 16 low bits represent the type ID within the extension.
     */
    const int EXTENSION_TYPE_HIGH_BITS_PREFIX = 15;
    const int EXTENSION_TYPE_LOW_BITS_TYPE = 16;
    /**
     * OperandType with any value above {@link IDevice::OPERAND_TYPE_BASE_MAX} must be interpreted
     * as an extension type according to {@link Model::extensionNameToPrefix}.
     */
    const int OPERAND_TYPE_BASE_MAX = 0xFFFF;
    /**
     * OperationType with any value above {@link IDevice::OPERATION_TYPE_BASE_MAX} must be
     * interpreted as an extension type according to {@link Model::extensionNameToPrefix}.
     */
    const int OPERATION_TYPE_BASE_MAX = 0xFFFF;

    /**
     * Allocates a driver-managed buffer with the properties specified by the buffer descriptor as
     * well as the input and output roles.
     *
     * The allocate function must verify its inputs are correct. If there is an error, or if a
     * certain role or property is not supported by the driver, the allocate function must return a
     * service specific exception with an appropriate ErrorStatus. If the allocation is successful,
     * this method must return a DeviceBuffer object with the produced IBuffer and a positive token
     * identifying the allocated buffer. A successful allocation must accommodate all of the
     * specified roles and buffer properties.
     *
     * The buffer is allocated to an uninitialized state. An uninitialized buffer may only be used
     * in ways that are specified by outputRoles. A buffer is initialized after it is used as an
     * output in a successful execution, or after a successful invocation of IBuffer::copyFrom on
     * the buffer. An initialized buffer may be used according to all roles specified in inputRoles
     * and outputRoles. A buffer will return to the uninitialized state if it is used as an output
     * in a failed execution, or after a failed invocation of IBuffer::copyFrom on the buffer.
     *
     * The dimensions of the buffer can be deduced from the buffer descriptor as well as the
     * dimensions of the corresponding model operands of the input and output roles. The dimensions
     * or rank of the buffer may be unknown at this stage. As such, some driver services may only
     * create a placeholder and defer the actual allocation until execution time. Note that the same
     * buffer may be used for different shapes of outputs on different executions. When the buffer
     * is used as an input, the input shape must be the same as the output shape from the last
     * execution using this buffer as an output.
     *
     * The driver must apply proper validatation upon every usage of the buffer, and must fail the
     * execution immediately if the usage is illegal.
     *
     * @param desc A buffer descriptor specifying the properties of the buffer to allocate.
     * @param preparedModels A vector of IPreparedModel objects. Must only contain IPreparedModel
     *                       objects from the same IDevice as this method is being invoked on.
     * @param inputRoles A vector of roles with each specifying an input to a prepared model.
     * @param outputRoles A vector of roles with each specifying an output to a prepared model. Each
     *                    role specified in inputRoles and outputRoles must be unique. The
     *                    corresponding model operands of the roles must have the same OperandType,
     *                    scale, zero point, and ExtraParams. The dimensions of the operands and the
     *                    dimensions specified in the buffer descriptor must be compatible with each
     *                    other. Two dimensions are incompatible if there is at least one axis that
     *                    is fully specified in both but has different values.
     * @return DeviceBuffer object containing the allocated IBuffer object and a positive token that
     *     can be used to reference the buffer as one of the memory pools.
     * @throws ServiceSpecificException with one of the following ErrorStatus values:
     *     - DEVICE_UNAVAILABLE if driver is offline or busy
     *     - GENERAL_FAILURE if a certain buffer property or a certain role is not supported,
     *       or if there is an unspecified error
     *     - INVALID_ARGUMENT if one of the input arguments is invalid
     *     - RESOURCE_EXHAUSTED_* if the task was aborted by the driver
     */
    DeviceBuffer allocate(in BufferDesc desc, in IPreparedModelParcel[] preparedModels,
            in BufferRole[] inputRoles, in BufferRole[] outputRoles);

    /**
     * Gets the capabilities of a driver.
     *
     * @return Capabilities of the driver.
     * @throws ServiceSpecificException with one of the following ErrorStatus values:
     *     - DEVICE_UNAVAILABLE if driver is offline or busy
     *     - GENERAL_FAILURE if there is an unspecified error
     */
    Capabilities getCapabilities();

    /**
     * Gets the caching requirements of the driver implementation.
     *
     * There are two types of cache file descriptors provided to the driver: model cache and data
     * cache.
     *
     * The data cache is for caching constant data, possibly including preprocessed and transformed
     * tensor buffers. Any modification to the data cache should have no worse effect than
     * generating bad output values at execution time.
     *
     * The model cache is for caching security-sensitive data such as compiled executable machine
     * code in the device's native binary format. A modification to the model cache may affect the
     * driver's execution behavior, and a malicious client could make use of this to execute beyond
     * the granted permission. Thus, the driver must always check whether the model cache is
     * corrupted before preparing the model from cache.
     *
     * getNumberOfCacheFilesNeeded returns how many of each type of cache files the driver
     * implementation needs to cache a single prepared model. Returning 0 for both types indicates
     * compilation caching is not supported by this driver. The driver may still choose not to cache
     * certain compiled models even if it reports that caching is supported.
     *
     * If the device reports that caching is not supported, the user may avoid calling
     * IDevice::prepareModelFromCache or providing cache file descriptors to
     * IDevice::prepareModel.
     *
     * @return NumberOfCacheFiles structure indicating how many files for model and data cache the
     *     driver needs to cache a single prepared model. It must be less than or equal to
     *     MAX_NUMBER_OF_CACHE_FILES.
     * @throws ServiceSpecificException with one of the following ErrorStatus values:
     *     - DEVICE_UNAVAILABLE if driver is offline or busy
     *     - GENERAL_FAILURE if there is an unspecified error
     */
    NumberOfCacheFiles getNumberOfCacheFilesNeeded();

    /**
     * Gets information about extensions supported by the driver implementation.
     *
     * All extension operations and operands must be fully supported for the extension to appear in
     * the list of supported extensions.
     *
     * @return A list of supported extensions.
     * @throws ServiceSpecificException with one of the following ErrorStatus values:
     *     - DEVICE_UNAVAILABLE if driver is offline or busy
     *     - GENERAL_FAILURE if there is an unspecified error
     */
    Extension[] getSupportedExtensions();

    /**
     * Gets the supported operations in a model.
     *
     * getSupportedOperations indicates which operations of the top-level subgraph are fully
     * supported by the vendor driver. If an operation may not be supported for any reason,
     * getSupportedOperations must return false for that operation.
     *
     * The {@link OperationType::IF} and {@link OperationType::WHILE} operations may only be fully
     * supported if the vendor driver fully supports all operations in the referenced subgraphs.
     *
     * @param model A model whose operations -- and their corresponding operands -- are to be
     *              verified by the driver.
     * @return A list of supported operations, where true indicates the operation is supported and
     *     false indicates the operation is not supported. The index of "supported" corresponds with
     *     the index of the operation it is describing in the main subgraph.
     * @throws ServiceSpecificException with one of the following ErrorStatus values:
     *     - DEVICE_UNAVAILABLE if driver is offline or busy
     *     - GENERAL_FAILURE if there is an unspecified error
     *     - INVALID_ARGUMENT if provided model is invalid
     */
    boolean[] getSupportedOperations(in Model model);

    /**
     * Get the type of a given device.
     *
     * The device type can be used to help application developers to distribute Machine Learning
     * workloads and other workloads such as graphical rendering. E.g., for an app which renders AR
     * scenes based on real time object detection results, the developer could choose an ACCELERATOR
     * type device for ML workloads, and reserve GPU for graphical rendering.
     *
     * @return The DeviceType of the device. Please note, this is not a bitfield of DeviceTypes.
     *     Each device must only be of a single DeviceType.
     * @throws ServiceSpecificException with one of the following ErrorStatus values:
     *     - DEVICE_UNAVAILABLE if driver is offline or busy
     *     - GENERAL_FAILURE if the query resulted in an unspecified error
     */
    DeviceType getType();

    /**
     * Get the version string of the driver implementation.
     *
     * The version string must be a unique token among the set of version strings of drivers of a
     * specific device. The token identifies the device driver's implementation. The token must not
     * be confused with the feature level which is solely defined by the interface version. This API
     * is opaque to the Android framework, but the Android framework may use the information for
     * debugging or to pass on to NNAPI applications.
     *
     * Application developers sometimes have specific requirements to ensure good user experiences,
     * and they need more information to make intelligent decisions when the Android framework
     * cannot. For example, combined with the device name and other information, the token can help
     * NNAPI applications filter devices based on their needs:
     *     - An application demands a certain level of performance, but a specific version of the
     *       driver cannot meet that requirement because of a performance regression.
     *       The application can disallow the driver based on the version provided.
     *     - An application has a minimum precision requirement, but certain versions of
     *       the driver cannot meet that requirement because of bugs or certain optimizations.
     *       The application can filter out versions of these drivers.
     *
     * @return The version string of the device implementation. Must have nonzero length.
     * @throws ServiceSpecificException with one of the following ErrorStatus values:
     *     - DEVICE_UNAVAILABLE if driver is offline or busy
     *     - GENERAL_FAILURE if the query resulted in an unspecified error
     */
    String getVersionString();

    /**
     * Asynchronously creates a prepared model for execution and optionally saves it into cache
     * files.
     *
     * prepareModel is used to make any necessary transformations to or alternative representations
     * to a model for execution, possibly including transformations on the constant data,
     * optimization on the model's graph, or compilation into the device's native binary format. The
     * model itself is not changed.
     *
     * Optionally, caching information may be provided for the driver to save the prepared model to
     * cache files for faster model compilation time when the same model preparation is requested in
     * the future. There are two types of cache file descriptors provided to the driver: model cache
     * and data cache. For more information on the two types of cache, refer to
     * getNumberOfCacheFilesNeeded.
     *
     * The file descriptors must be opened with read and write permission. A file may have any size,
     * and the corresponding file descriptor may have any offset. The driver must truncate a file to
     * zero size before writing to that file. The file descriptors may be closed by the client once
     * the asynchronous preparation has finished. The driver must dup a file descriptor if it wants
     * to get access to the cache file later.
     *
     * The model is prepared asynchronously with respect to the caller. The prepareModel function
     * must verify the inputs to the preparedModel function related to preparing the model (as
     * opposed to saving the prepared model to cache) are correct. If there is an error,
     * prepareModel must immediately invoke the callback with the appropriate ErrorStatus value and
     * nullptr for the IPreparedModel, then return a status with a service specific exception with
     * the same ErrorStatus. If the inputs to the prepareModel function that are related to
     * preparing the model are valid and there is no error, prepareModel must launch an asynchronous
     * task to prepare the model in the background, and immediately return from prepareModel. If the
     * asynchronous task fails to launch, prepareModel must immediately invoke the callback with
     * ErrorStatus::GENERAL_FAILURE and nullptr for the IPreparedModel, then return a service
     * specific exception with ErrorStatus::GENERAL_FAILURE.
     *
     * When the asynchronous task has finished preparing the model, it must immediately invoke the
     * callback function provided as an input to prepareModel. If the model was prepared
     * successfully, the callback object must be invoked with an error status of ErrorStatus::NONE
     * and the produced IPreparedModel object. If an error occurred preparing the model, the
     * callback object must be invoked with the appropriate ErrorStatus value and nullptr for the
     * IPreparedModel.
     *
     * The model is prepared with a priority. This priority is relative to other prepared models
     * owned by the same client. Higher priority executions may use more compute resources than
     * lower priority executions, and may preempt or starve lower priority executions.
     *
     * prepareModel can be called with an optional deadline. If the model is not able to be prepared
     * before the provided deadline, the model preparation may be aborted, and either
     * {@link ErrorStatus::MISSED_DEADLINE_TRANSIENT} or {@link
     * ErrorStatus::MISSED_DEADLINE_PERSISTENT} may be returned. The error due to an abort must be
     * sent the same way as other errors, described above.
     *
     * Optionally, the driver may save the prepared model to cache during the asynchronous
     * preparation. Any error that occurs when saving to cache must not affect the status of
     * preparing the model. Even if the input arguments related to the cache may be invalid, or the
     * driver may fail to save to cache, the prepareModel function must finish preparing the model.
     * The driver may choose not to save to cache even if the caching information is provided and
     * valid.
     *
     * The only information that may be unknown to the model at this stage is the shape of the
     * tensors, which may only be known at execution time. As such, some driver services may return
     * partially prepared models, where the prepared model may only be finished when it is paired
     * with a set of inputs to the model. Note that the same prepared model object may be used with
     * different shapes of inputs on different (possibly concurrent) executions.
     *
     * Multiple threads may call prepareModel on the same model concurrently.
     *
     * @param model The model to be prepared for execution.
     * @param preference Indicates the intended execution behavior of a prepared model.
     * @param priority The priority of the prepared model relative to other prepared models owned by
     *                 the client.
     * @param deadlineNs The time by which the model is expected to be prepared. The time is
     *                   measured in nanoseconds since boot (as from clock_gettime(CLOCK_BOOTTIME,
     *                   &ts) or ::android::base::boot_clock). If the model cannot be prepared by
     *                   the deadline, the preparation may be aborted. Passing -1 means the deadline
     *                   is omitted. Other negative values are invalid.
     * @param modelCache A vector of file descriptors for the security-sensitive cache. The length
     *                   of the vector must either be 0 indicating that caching information is not
     *                   provided, or match the numModelCache returned from
     *                   getNumberOfCacheFilesNeeded. The cache file descriptors will be provided in
     *                   the same order when retrieving the preparedModel from cache files with
     *                   prepareModelFromCache.
     * @param dataCache A vector of file descriptors for the constants' cache. The length of the
     *                  vector must either be 0 indicating that caching information is not provided,
     *                  or match the numDataCache returned from getNumberOfCacheFilesNeeded. The
     *                  cache file descriptors will be provided in the same order when retrieving
     *                  the preparedModel from cache files with prepareModelFromCache.
     * @param token A caching token of length BYTE_SIZE_OF_CACHE_TOKEN identifying the prepared
     *              model. The same token will be provided when retrieving the prepared model from
     *              the cache files with prepareModelFromCache.  Tokens should be chosen to have a
     *              low rate of collision for a particular application. The driver cannot detect a
     *              collision; a collision will result in a failed execution or in a successful
     *              execution that produces incorrect output values. If both modelCache and
     *              dataCache are empty indicating that caching information is not provided, this
     *              token must be ignored.
     * @param callback A callback object used to return the error status of preparing the model for
     *                 execution and the prepared model if successful, nullptr otherwise. The
     *                 callback object's notify function must be called exactly once, even if the
     *                 model could not be prepared.
     * @throws ServiceSpecificException with one of the following ErrorStatus values:
     *     - DEVICE_UNAVAILABLE if driver is offline or busy
     *     - GENERAL_FAILURE if there is an unspecified error
     *     - INVALID_ARGUMENT if one of the input arguments related to preparing the model is
     *       invalid
     *     - MISSED_DEADLINE_* if the preparation is aborted because the model cannot be prepared by
     *       the deadline
     *     - RESOURCE_EXHAUSTED_* if the task was aborted by the driver
     */
    void prepareModel(in Model model, in ExecutionPreference preference, in Priority priority,
            in long deadlineNs, in ParcelFileDescriptor[] modelCache,
            in ParcelFileDescriptor[] dataCache, in byte[] token,
            in IPreparedModelCallback callback);

    /**
     * Creates a prepared model from cache files for execution.
     *
     * prepareModelFromCache is used to retrieve a prepared model directly from cache files to avoid
     * slow model compilation time. There are two types of cache file descriptors provided to the
     * driver: model cache and data cache. For more information on the two types of cache files,
     * refer to getNumberOfCacheFilesNeeded.
     *
     * The file descriptors must be opened with read and write permission. A file may have any size,
     * and the corresponding file descriptor may have any offset. The driver must truncate a file to
     * zero size before writing to that file. The file descriptors may be closed by the client once
     * the asynchronous preparation has finished. The driver must dup a file descriptor if it wants
     * to get access to the cache file later.
     *
     * The model is prepared asynchronously with respect to the caller. The prepareModelFromCache
     * function must verify the inputs to the prepareModelFromCache function are correct, and that
     * the security-sensitive cache has not been modified since it was last written by the driver.
     * If there is an error, or if compilation caching is not supported, or if the
     * security-sensitive cache has been modified, prepareModelFromCache must immediately invoke the
     * callback with the appropriate ErrorStatus value and nullptr for the IPreparedModel, then
     * return a status with a service specific exception with the same ErrorStatus. If the inputs to
     * the prepareModelFromCache function are valid, the security-sensitive cache is not modified,
     * and there is no error, prepareModelFromCache must launch an asynchronous task to prepare the
     * model in the background, and immediately return from prepareModelFromCache. If the
     * asynchronous task fails to launch, prepareModelFromCache must immediately invoke the callback
     * with ErrorStatus::GENERAL_FAILURE and nullptr for the IPreparedModel, then return a service
     * specific exception with ErrorStatus::GENERAL_FAILURE.
     *
     * When the asynchronous task has finished preparing the model, it must immediately invoke the
     * callback function provided as an input to prepareModelFromCache. If the model was prepared
     * successfully, the callback object must be invoked with an error status of ErrorStatus::NONE
     * and the produced IPreparedModel object. If an error occurred preparing the model, the
     * callback object must be invoked with the appropriate ErrorStatus value and nullptr for the
     * IPreparedModel.
     *
     * prepareModelFromCache can be called with an optional deadline. If the model is not able to
     * prepared before the provided deadline, the model preparation may be aborted, and either
     * {@link ErrorStatus::MISSED_DEADLINE_TRANSIENT} or
     * {@link ErrorStatus::MISSED_DEADLINE_PERSISTENT} may be returned. The error due to an abort
     * must be sent the same way as other errors, described above.
     *
     * The only information that may be unknown to the model at this stage is the shape of the
     * tensors, which may only be known at execution time. As such, some driver services may return
     * partially prepared models, where the prepared model may only be finished when it is paired
     * with a set of inputs to the model. Note that the same prepared model object may be used with
     * different shapes of inputs on different (possibly concurrent) executions.
     *
     * @param deadlineNs The time by which the model is expected to be prepared. The time is
     *                   measured in nanoseconds since boot (as from clock_gettime(CLOCK_BOOTTIME,
     *                   &ts) or ::android::base::boot_clock). If the model cannot be prepared by
     *                   the deadline, the preparation may be aborted. Passing -1 means the deadline
     *                   is omitted. Other negative values are invalid.
     * @param modelCache A vector of file descriptors for the security-sensitive cache. The length
     *                   of the vector must match the numModelCache returned from
     *                   getNumberOfCacheFilesNeeded. The cache file descriptors will be provided in
     *                   the same order as with prepareModel.
     * @param dataCache A vector of file descriptors for the constants' cache. The length of the
     *                  vector must match the numDataCache returned from
     *                  getNumberOfCacheFilesNeeded. The cache file descriptors will be provided in
     *                  the same order as with prepareModel.
     * @param token A caching token of length BYTE_SIZE_OF_CACHE_TOKEN identifying the prepared
     *              model. It is the same token provided when saving the cache files with
     *              prepareModel. Tokens should be chosen to have a low rate of collision for a
     *              particular application. The driver cannot detect a collision; a collision will
     *              result in a failed execution or in a successful execution that produces
     *              incorrect output values.
     * @param callback A callback object used to return the error status of preparing the model for
     *                 execution and the prepared model if successful, nullptr otherwise. The
     *                 callback object's notify function must be called exactly once, even if the
     *                 model could not be prepared.
     * @throws ServiceSpecificException with one of the following ErrorStatus values:
     *     - DEVICE_UNAVAILABLE if driver is offline or busy
     *     - GENERAL_FAILURE if caching is not supported or if there is an unspecified error
     *     - INVALID_ARGUMENT if one of the input arguments is invalid
     *     - MISSED_DEADLINE_* if the preparation is aborted because the model cannot be prepared by
     *       the deadline
     *     - RESOURCE_EXHAUSTED_* if the task was aborted by the driver
     */
    void prepareModelFromCache(in long deadlineNs, in ParcelFileDescriptor[] modelCache,
            in ParcelFileDescriptor[] dataCache, in byte[] token,
            in IPreparedModelCallback callback);
}
