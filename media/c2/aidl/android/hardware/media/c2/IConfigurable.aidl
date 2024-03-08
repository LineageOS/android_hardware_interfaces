/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.media.c2;

import android.hardware.media.c2.FieldSupportedValuesQuery;
import android.hardware.media.c2.FieldSupportedValuesQueryResult;
import android.hardware.media.c2.ParamDescriptor;
import android.hardware.media.c2.Params;
import android.hardware.media.c2.SettingResult;

/**
 * Generic configuration interface presented by all configurable Codec2 objects.
 *
 * This interface must be supported in all states of the owning object, and must
 * not change the state of the owning object.
 */
@VintfStability
interface IConfigurable {
    /**
     * Return parcelable for config() interface.
     *
     * This includes the successful config settings along with the failure reasons of
     * the specified setting.
     */
    @VintfStability
    parcelable ConfigResult {
        Params params;
        SettingResult[] failures;
    }

    /**
     * Sets a set of parameters for the object.
     *
     * Tuning is performed at best effort: the object must update all supported
     * configurations at best effort and skip unsupported parameters. Any errors
     * are communicated in the return value along with the failures.
     *
     * A non-strict parameter update with an unsupported value shall cause an
     * update to the closest supported value. A strict parameter update with an
     * unsupported value shall be skipped and a failure shall be returned.
     *
     * If @p mayBlock is false, this method must not block. An update that
     * requires blocking shall be skipped and a failure shall be returned.
     *
     * If @p mayBlock is true, an update may block, but the whole method call
     * has to complete in a timely manner, or `Status::TIMED_OUT` is thrown.
     *
     * The final values for all parameters set are propagated back to the caller
     * in @p params.
     *
     * \par For IComponent
     *
     * When the object type is @ref IComponent, this method must be supported in
     * any state except released.
     *
     * The blocking behavior of this method differs among states:
     *   - In the stopped state, this must be non-blocking. @p mayBlock is
     *     ignored. (The method operates as if @p mayBlock was false.)
     *   - In any of the running states, this method may block momentarily if
     *     @p mayBlock is true. However, if the call cannot be completed in a
     *     timely manner, `Status::TIMED_OUT` is thrown.
     *
     * @note Parameter tuning @e does depend on the order of the tuning
     * parameters, e.g., some parameter update may enable some subsequent
     * parameter update.
     *
     * @param inParams Requested parameter updates.
     * @param mayBlock Whether this call may block or not.
     * @return result of config. Params in the result should be in same order
     *     with @p inParams.
     * @throws ServiceSpecificException with one of the following values:
     *   - `Status::NO_MEMORY` - Some supported parameters could not be updated
     *                   successfully because they contained unsupported values.
     *                   These are returned in @p failures.
     *   - `Status::BLOCKING`  - Setting some parameters requires blocking, but
     *                   @p mayBlock is false.
     *   - `Status::TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `Status::CORRUPTED` - Some unknown error occurred.
     */
    ConfigResult config(in Params inParams, in boolean mayBlock);

    /**
     * Returns the id of the object. This must be unique among all objects of
     * the same type hosted by the same store.
     *
     * @return Id of the object.
     */
    int getId();

    /**
     * Returns the name of the object.
     *
     * This must match the name that was supplied during the creation of the
     * object.
     *
     * @return Name of the object.
     */
    String getName();

    /**
     * Queries a set of parameters from the object.
     *
     * Querying is performed at best effort: the object must query all supported
     * parameters and skip unsupported ones (which may include parameters that
     * could not be allocated).
     *
     * If @p mayBlock is true, a query may block, but the whole method call
     * has to complete in a timely manner, or `Status::TIMED_OUT` is thrown.
     *
     * If @p mayBlock is false, this method must not block(All parameter queries
     * that require blocking must be skipped). Otherwise, this
     * method is allowed to block for a certain period of time before completing
     * the operation. If the operation is not completed in a timely manner,
     * `Status::TIMED_OUT` is thrown.
     *
     * @note Since unsupported parameters will be skipped, the returned results
     *     does not have every settings from @p indices, but the result will preserve
     *     the original order from @p indices though unsupported settings are skipped.
     *
     * \par For IComponent
     *
     * When the object type is @ref IComponent, this method must be supported in
     * any state except released. This call must not change the state nor the
     * internal configuration of the component.
     *
     * The blocking behavior of this method differs among states:
     *   - In the stopped state, this must be non-blocking. @p mayBlock is
     *     ignored. (The method operates as if @p mayBlock was false.)
     *   - In any of the running states, this method may block momentarily if
     *     @p mayBlock is true. However, if the call cannot be completed in a
     *     timely manner, `Status::status` is thrown.
     *
     * @param indices List of C2Param structure indices to query.
     * @param mayBlock Whether this call may block or not.
     * @return Flattened representation of std::vector<C2Param> object.
     *     Unsupported settings are skipped in the results. The order in @p indices
     *     still be preserved except skipped settings.
     * @throws ServiceSpecificException with one of the following values:
     *   - `Status::NO_MEMORY` - Could not allocate memory for a supported parameter.
     *   - `Status::BLOCKING`  - Querying some parameters requires blocking, but
     *                   @p mayBlock is false.
     *   - `Status::TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `Status::CORRUPTED` - Some unknown error occurred.
     */
    Params query(in int[] indices, in boolean mayBlock);

    /**
     * Returns a list of supported parameters within a selected range of C2Param
     * structure indices.
     *
     * @param start The first index of the selected range.
     * @param count The length of the selected range.
     * @return List of supported parameters in the selected range. This
     *     list may have fewer than @p count elements if some indices in the
     *     range are not supported.
     * @throws ServiceSpecificException with one of the following values:
     *   - `Status::NO_MEMORY` - Not enough memory to complete this method.
     *
     */
    ParamDescriptor[] querySupportedParams(in int start, in int count);

    /**
     * Retrieves the supported values for the queried fields.
     *
     * The object must process all fields queried even if some queries fail.
     *
     * If @p mayBlock is false, this method must not block. Otherwise, this
     * method is allowed to block for a certain period of time before completing
     * the operation. If the operation cannot be completed in a timely manner,
     * `Status::TIMED_OUT` is thrown.
     *
     * \par For IComponent
     *
     * When the object type is @ref IComponent, this method must be supported in
     * any state except released.
     *
     * The blocking behavior of this method differs among states:
     *   - In the stopped state, this must be non-blocking. @p mayBlock is
     *     ignored. (The method operates as if @p mayBlock was false.)
     *   - In any of the running states, this method may block momentarily if
     *     @p mayBlock is true. However, if the call cannot be completed in a
     *     timely manner, `Status::TIMED_OUT` is thrown.
     *
     * @param inFields List of field queries.
     * @param mayBlock Whether this call may block or not.
     * @return List of supported values and results for the
     *     supplied queries.
     * @throws ServiceSpecificException with one of the following values:
     *   - `Status::BLOCKING`  - Querying some parameters requires blocking, but
     *                   @p mayBlock is false.
     *   - `Status::NO_MEMORY` - Not enough memory to complete this method.
     *   - `Status::BLOCKING`  - Querying some fields requires blocking, but @p mayblock
     *                   is false.
     *   - `Status::TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `Status::CORRUPTED` - Some unknown error occurred.
     */
    FieldSupportedValuesQueryResult[] querySupportedValues(
            in FieldSupportedValuesQuery[] inFields, in boolean mayBlock);
}
