/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.tv.tuner;

import android.hardware.tv.tuner.DemuxPid;
import android.hardware.tv.tuner.IFilter;

/**
 * Descrambler is used to descramble input data.
 * @hide
 */
@VintfStability
interface IDescrambler {
    /**
     * Set a demux as source of the descrambler
     *
     * It is used by the client to specify a demux as source of this
     * descrambler. A descrambler instance can have only one source, and
     * this method can be only called once.
     *
     * @param demuxId the id of the demux to be used as descrambler's source.
     */
    void setDemuxSource(in int demuxId);

    /**
     * Set a key token to link descrambler to a key slot
     *
     * It is used by the client to link a hardware key slot to a descrambler.
     * A descrambler instance can have only one key slot to link, but a key
     * slot can hold a few keys for different purposes.
     *
     * @param keyToken the token to be used to link the key slot.
     */
    void setKeyToken(in byte[] keyToken);

    /**
     * Add packets' PID to the descrambler for descrambling
     *
     * It is used by the client to specify Package ID (PID) of packets which the
     * descrambler start to descramble. Multiple PIDs can be added into one
     * descrambler instance because descambling can happen simultaneously on
     * packets from different PIDs.
     *
     * @param pid the PID of packets to start to be descrambled.
     * @param filter an optional filter instance to identify upper stream.
     */
    void addPid(in DemuxPid pid, in IFilter optionalSourceFilter);

    /**
     * Remove packets' PID from the descrambler
     *
     * It is used by the client to specify Package ID (PID) of packets which the
     * descrambler stop to descramble.
     *
     * @param pid the PID of packets to stop to be descrambled.
     * @param filter an optional filter instance to identify upper stream.
     */
    void removePid(in DemuxPid pid, in IFilter optionalSourceFilter);

    /**
     * Release the descrambler instance
     *
     * It is used by the client to release the descrambler instance. HAL clear
     * underneath resource. client mustn't access the instance any more.
     */
    void close();
}
