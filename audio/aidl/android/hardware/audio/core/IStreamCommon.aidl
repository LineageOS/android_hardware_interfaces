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

package android.hardware.audio.core;

import android.hardware.audio.core.VendorParameter;

/**
 * This interface contains operations that are common to input and output
 * streams (IStreamIn and IStreamOut). The lifetime of the server-side
 * implementation object is the same as of the "parent" IStreamIn/Out object.
 * The client must release all references to this object together with
 * references to the "parent" object.
 */
@VintfStability
interface IStreamCommon {
    /**
     * Close the stream.
     *
     * Releases any resources allocated for this stream on the HAL module side.
     * This includes the fast message queues and shared memories returned via
     * the StreamDescriptor. Thus, the stream can not be operated anymore after
     * it has been closed. The client needs to release the audio data I/O
     * objects after the call to this method returns.
     *
     * Methods of IStream* interfaces throw EX_ILLEGAL_STATE for a closed stream.
     *
     * @throws EX_ILLEGAL_STATE If the stream has already been closed.
     */
    void close();

    /**
     * Update the HW AV Sync identifier for the stream.
     *
     * The argument to this method must be one of the identifiers previously
     * returned by the 'IModule.generateHwAvSyncId' method. By tagging streams
     * with the same identifier, the client indicates to the HAL that they all
     * use the same HW AV Sync timestamps sequence.
     *
     * HW AV Sync timestamps are used for "tunneled" I/O modes and thus
     * are not mandatory.
     *
     * @throws EX_ILLEGAL_ARGUMENT If the provided ID is unknown to the HAL module.
     * @throws EX_ILLEGAL_STATE If the stream is closed.
     * @throws EX_UNSUPPORTED_OPERATION If synchronization with HW AV Sync markers
     *                                  is not supported.
     */
    void updateHwAvSyncId(int hwAvSyncId);

    /**
     * Get current values of vendor parameters.
     *
     * Return current values for the parameters corresponding to the provided ids.
     *
     * @param ids Ids of the parameters to retrieve values of.
     * @return Current values of parameters.
     * @throws EX_ILLEGAL_ARGUMENT If the stream does not recognize provided ids.
     * @throws EX_ILLEGAL_STATE If parameter values can not be retrieved at the moment.
     * @throws EX_UNSUPPORTED_OPERATION If the stream does not support vendor parameters.
     */
    VendorParameter[] getVendorParameters(in @utf8InCpp String[] ids);
    /**
     * Set vendor parameters.
     *
     * Update values for provided vendor parameters. If the 'async' parameter
     * is set to 'true', the implementation must return the control back without
     * waiting for the application of parameters to complete.
     *
     * @param parameters Ids and values of parameters to set.
     * @param async Whether to return from the method as early as possible.
     * @throws EX_ILLEGAL_ARGUMENT If the stream does not recognize provided parameters.
     * @throws EX_ILLEGAL_STATE If parameters can not be set at the moment.
     * @throws EX_UNSUPPORTED_OPERATION If the stream does not support vendor parameters.
     */
    void setVendorParameters(in VendorParameter[] parameters, boolean async);
}
