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

package android.hardware.media.bufferpool2;

import android.hardware.media.bufferpool2.Buffer;
import android.hardware.media.bufferpool2.ResultStatus;

/**
 * A connection to a buffer pool which handles requests from a buffer pool
 * client. The connection must be made in order to receive buffers from
 * other buffer pool clients.
 */
@VintfStability
interface IConnection {

    parcelable FetchInfo {
        /**
         * Unique transaction id for buffer transferring.
         */
        long transactionId;
        /**
         * Id of the buffer to be fetched.
         */
        int bufferId;
    }

    union FetchResult {
        /**
         * The fetched buffer on successful fetch.
         */
        Buffer buffer;
        /**
         * The reason of the request failure. Possible values are below.
         *
         * ResultStatus::NOT_FOUND        - A buffer was not found due to invalidation.
         * ResultStatus::CRITICAL_ERROR   - Other errors.
         */
        int failure;
    }

    /**
     * Retrieves buffers using an array of FetchInfo.
     * Each element of FetchInfo array contains a bufferId and a transactionId
     * for each buffer to fetch. The method must be called from receiving side of buffers
     * during transferring only when the specified buffer is neither cached nor used.
     *
     * The method could have partial failures, in the case other successfully fetched buffers
     * will be in returned result along with the failures. The order of the returned result
     * will be the same with the fetchInfos.
     *
     * @param fetchInfos information of buffers to fetch
     * @return Requested buffers.
     *         If there are failures, reasons of failures are also included.
     * @throws ServiceSpecificException with one of the following values:
     *     ResultStatus::NO_MEMORY        - Memory allocation failure occurred.
     *     ResultStatus::CRITICAL_ERROR   - Other errors.
     */
    FetchResult[] fetch(in FetchInfo[] fetchInfos);

    /**
     * Enforce processing of unprocessed bufferpool messages.
     *
     * BufferPool implementation optimizes message processing by piggy-backing approach.
     * This method can ensure pending bufferpool messages being processed timely.
     */
    void sync();
}
