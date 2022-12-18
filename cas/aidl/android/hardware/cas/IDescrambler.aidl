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

package android.hardware.cas;

import android.hardware.cas.DestinationBuffer;
import android.hardware.cas.ScramblingControl;
import android.hardware.cas.SharedBuffer;
import android.hardware.cas.SubSample;

/**
 * IDescrambler is the API to control the descrambling operations.
 * @hide
 */
@VintfStability
interface IDescrambler {
    /**
     * Descramble the data in a source SharedBuffer, described by an array of
     * SubSample structures.
     *
     * @param scramblingControl an enumeration indicating the key that the subsamples
     * were scrambled with.
     * @param subSamples an array of SubSample structures describing the number of
     * clear and scrambled bytes within each subsample.
     * @param srcBuffer the SharedBuffer containing the source scrambled data.
     * @param srcOffset the position where the source scrambled data starts at.
     * @param dstBuffer the DestinationBuffer to hold the descrambled data.
     * @param dstOffset the position where the descrambled data should start at.
     *
     * @return bytesWritten Number of bytes that have been successfully written.
     */
    int descramble(in ScramblingControl scramblingControl, in SubSample[] subSamples,
            in SharedBuffer srcBuffer, in long srcOffset, in DestinationBuffer dstBuffer,
            in long dstOffset);

    /**
     * Release the descrambler instance.
     */
    void release();

    /**
     * Query if the scrambling scheme requires the use of a secure decoder
     * to decode data of the given mime type.
     *
     * @param mime the mime type of the media data.
     * @return whether the descrambler requires a secure decoder.
     */
    boolean requiresSecureDecoderComponent(in String mime);

    /**
     * Associate a MediaCas session with this MediaDescrambler instance.
     *
     * @param sessionId the id of the session to associate with this descrambler instance.
     */
    void setMediaCasSession(in byte[] sessionId);
}
